/**
 * Copyright 2018 Marcus Pinnecke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of
 * the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "core/strhash/strhash_mem.h"
#include "core/async/spin.h"
#include "std/sort.h"
#include "core/alloc/trace.h"
#include "utils/time.h"
#include "std/bloom.h"
#include "stdx/slicelist.h"
#include "hash/bern.h"

#define HASHCODE_OF(key)      NG5_HASH_BERNSTEIN(strlen(key), key)

#define SMART_MAP_TAG "strhash-mem"

struct bucket {
        slice_list_t slice_list;
};

struct mem_extra {
        struct vector ofType(bucket) buckets;
};

static int this_drop(struct strhash *self);
static int this_put_safe_bulk(struct strhash *self, char *const *keys, const field_sid_t *values, size_t num_pairs);
static int this_put_fast_bulk(struct strhash *self, char *const *keys, const field_sid_t *values, size_t num_pairs);
static int this_put_safe_exact(struct strhash *self, const char *key, field_sid_t value);
static int this_put_fast_exact(struct strhash *self, const char *key, field_sid_t value);
static int this_get_safe(struct strhash *self, field_sid_t **out, bool **found_mask, size_t *num_not_found,
        char *const *keys, size_t num_keys);
static int this_get_safe_exact(struct strhash *self, field_sid_t *out, bool *found_mask, const char *key);
static int this_get_fast(struct strhash *self, field_sid_t **out, char *const *keys, size_t num_keys);
static int this_update_key_fast(struct strhash *self, const field_sid_t *values, char *const *keys, size_t num_keys);
static int this_remove(struct strhash *self, char *const *keys, size_t num_keys);
static int this_free(struct strhash *self, void *ptr);

static int this_insert_bulk(struct vector ofType(bucket) *buckets, char *const *restrict keys,
        const field_sid_t *restrict values, size_t *restrict bucket_idxs, size_t num_pairs, struct allocator *alloc,
        struct strhash_counters *counter);

static int this_insert_exact(struct vector ofType(bucket) *buckets, const char *restrict key, field_sid_t value,
        size_t bucket_idx, struct allocator *alloc, struct strhash_counters *counter);
static int this_fetch_bulk(struct vector ofType(bucket) *buckets, field_sid_t *values_out, bool *key_found_mask,
        size_t *num_keys_not_found, size_t *bucket_idxs, char *const *keys, size_t num_keys, struct allocator *alloc,
        struct strhash_counters *counter);
static int this_fetch_single(struct vector ofType(bucket) *buckets, field_sid_t *value_out, bool *key_found,
        const size_t bucket_idx, const char *key, struct strhash_counters *counter);

static int this_create_extra(struct strhash *self, size_t num_buckets, size_t cap_buckets);
static struct mem_extra *this_get_exta(struct strhash *self);
static int bucket_create(struct bucket *buckets, size_t num_buckets, size_t bucket_cap, struct allocator *alloc);
static int bucket_drop(struct bucket *buckets, size_t num_buckets, struct allocator *alloc);
static int bucket_insert(struct bucket *bucket, const char *restrict key, field_sid_t value, struct allocator *alloc,
        struct strhash_counters *counter);

bool strhash_create_inmemory(struct strhash *parallel_map_exec, const struct allocator *alloc, size_t num_buckets,
        size_t cap_buckets)
{
        ng5_check_success(alloc_this_or_std(&parallel_map_exec->allocator, alloc));

        num_buckets = num_buckets < 1 ? 1 : num_buckets;
        cap_buckets = cap_buckets < 1 ? 1 : cap_buckets;

        parallel_map_exec->tag = MEMORY_RESIDENT;
        parallel_map_exec->drop = this_drop;
        parallel_map_exec->put_bulk_safe = this_put_safe_bulk;
        parallel_map_exec->put_bulk_fast = this_put_fast_bulk;
        parallel_map_exec->put_exact_safe = this_put_safe_exact;
        parallel_map_exec->put_exact_fast = this_put_fast_exact;
        parallel_map_exec->get_bulk_safe = this_get_safe;
        parallel_map_exec->get_fast = this_get_fast;
        parallel_map_exec->update_key_fast = this_update_key_fast;
        parallel_map_exec->remove = this_remove;
        parallel_map_exec->free = this_free;
        parallel_map_exec->get_exact_safe = this_get_safe_exact;
        error_init(&parallel_map_exec->err);

        strhash_reset_counters(parallel_map_exec);
        ng5_check_success(this_create_extra(parallel_map_exec, num_buckets, cap_buckets));
        return true;
}

static int this_drop(struct strhash *self)
{
        assert(self->tag == MEMORY_RESIDENT);
        struct mem_extra *extra = this_get_exta(self);
        struct bucket *data = (struct bucket *) vec_data(&extra->buckets);
        ng5_check_success(bucket_drop(data, extra->buckets.cap_elems, &self->allocator));
        vec_drop(&extra->buckets);
        alloc_free(&self->allocator, self->extra);
        return true;
}

static int this_put_safe_bulk(struct strhash *self, char *const *keys, const field_sid_t *values, size_t num_pairs)
{
        assert(self->tag == MEMORY_RESIDENT);
        struct mem_extra *extra = this_get_exta(self);
        size_t *bucket_idxs = alloc_malloc(&self->allocator, num_pairs * sizeof(size_t));

        prefetch_write(bucket_idxs);

        for (size_t i = 0; i < num_pairs; i++) {
                const char *key = keys[i];
                hash32_t hash = HASHCODE_OF(key);
                bucket_idxs[i] = hash % extra->buckets.cap_elems;
        }

        prefetch_read(bucket_idxs);
        prefetch_read(keys);
        prefetch_read(values);

        ng5_check_success(this_insert_bulk(&extra->buckets,
                keys,
                values,
                bucket_idxs,
                num_pairs,
                &self->allocator,
                &self->counters));
        ng5_check_success(alloc_free(&self->allocator, bucket_idxs));
        return true;
}

static int this_put_safe_exact(struct strhash *self, const char *key, field_sid_t value)
{
        assert(self->tag == MEMORY_RESIDENT);
        struct mem_extra *extra = this_get_exta(self);

        hash32_t hash = strcmp("", key) != 0 ? HASHCODE_OF(key) : 0;
        size_t bucket_idx = hash % extra->buckets.cap_elems;

        prefetch_read(key);

        ng5_check_success(this_insert_exact(&extra->buckets,
                key,
                value,
                bucket_idx,
                &self->allocator,
                &self->counters));

        return true;
}

static int this_put_fast_exact(struct strhash *self, const char *key, field_sid_t value)
{
        return this_put_safe_exact(self, key, value);
}

static int this_put_fast_bulk(struct strhash *self, char *const *keys, const field_sid_t *values, size_t num_pairs)
{
        return this_put_safe_bulk(self, keys, values, num_pairs);
}

static int this_fetch_bulk(struct vector ofType(bucket) *buckets, field_sid_t *values_out, bool *key_found_mask,
        size_t *num_keys_not_found, size_t *bucket_idxs, char *const *keys, size_t num_keys, struct allocator *alloc,
        struct strhash_counters *counter)
{
        ng5_unused(counter);
        ng5_unused(alloc);

        slice_handle_t result_handle;
        size_t num_not_found = 0;
        struct bucket *data = (struct bucket *) vec_data(buckets);

        prefetch_write(values_out);

        for (size_t i = 0; i < num_keys; i++) {
                struct bucket *bucket = data + bucket_idxs[i];
                const char *key = keys[i];
                if (likely(key != NULL)) {
                        slice_list_lookup(&result_handle, &bucket->slice_list, key);
                } else {
                        result_handle.is_contained = true;
                        result_handle.value = NG5_NULL_ENCODED_STRING;
                }

                num_not_found += result_handle.is_contained ? 0 : 1;
                key_found_mask[i] = result_handle.is_contained;
                values_out[i] = result_handle.is_contained ? result_handle.value : ((field_sid_t) -1);
        }

        *num_keys_not_found = num_not_found;
        return true;
}

static int this_fetch_single(struct vector ofType(bucket) *buckets, field_sid_t *value_out, bool *key_found,
        const size_t bucket_idx, const char *key, struct strhash_counters *counter)
{
        ng5_unused(counter);

        slice_handle_t handle;
        struct bucket *data = (struct bucket *) vec_data(buckets);

        prefetch_write(value_out);
        prefetch_write(key_found);

        struct bucket *bucket = data + bucket_idx;

        /** Optimization 1/5: EMPTY GUARD (but before "find" call); if this bucket has no occupied slots, do not perform any lookup and comparison */
        slice_list_lookup(&handle, &bucket->slice_list, key);
        *key_found = !SliceListIsEmpty(&bucket->slice_list) && handle.is_contained;
        *value_out = (*key_found) ? handle.value : ((field_sid_t) -1);

        return true;
}

static int this_get_safe(struct strhash *self, field_sid_t **out, bool **found_mask, size_t *num_not_found,
        char *const *keys, size_t num_keys)
{
        assert(self->tag == MEMORY_RESIDENT);

        timestamp_t begin = time_now_wallclock();
        ng5_trace(SMART_MAP_TAG, "'get_safe' function invoked for %zu strings", num_keys)

        struct allocator hashtable_alloc;
#if defined(NG5_CONFIG_TRACE_STRING_DIC_ALLOC) && !defined(NDEBUG)
        CHECK_SUCCESS(allocator_TRACE(&hashtable_alloc));
#else
        ng5_check_success(alloc_this_or_std(&hashtable_alloc, &self->allocator));
#endif

        struct mem_extra *extra = this_get_exta(self);
        size_t *bucket_idxs = alloc_malloc(&self->allocator, num_keys * sizeof(size_t));
        field_sid_t *values_out = alloc_malloc(&self->allocator, num_keys * sizeof(field_sid_t));
        bool *found_mask_out = alloc_malloc(&self->allocator, num_keys * sizeof(bool));

        assert(bucket_idxs != NULL);
        assert(values_out != NULL);
        assert(found_mask_out != NULL);

        for (register size_t i = 0; i < num_keys; i++) {
                const char *key = keys[i];
                hash32_t hash = key && strcmp("", key) != 0 ? HASHCODE_OF(key) : 0;
                bucket_idxs[i] = hash % extra->buckets.cap_elems;
                prefetch_read((struct bucket *) vec_data(&extra->buckets) + bucket_idxs[i]);
        }

        ng5_trace(SMART_MAP_TAG, "'get_safe' function invoke fetch...for %zu strings", num_keys)
        ng5_check_success(this_fetch_bulk(&extra->buckets,
                values_out,
                found_mask_out,
                num_not_found,
                bucket_idxs,
                keys,
                num_keys,
                &self->allocator,
                &self->counters));
        ng5_check_success(alloc_free(&self->allocator, bucket_idxs));
        ng5_trace(SMART_MAP_TAG, "'get_safe' function invok fetch: done for %zu strings", num_keys)

        assert(values_out != NULL);
        assert(found_mask_out != NULL);

        *out = values_out;
        *found_mask = found_mask_out;

        timestamp_t end = time_now_wallclock();
        ng5_unused(begin);
        ng5_unused(end);
        ng5_trace(SMART_MAP_TAG, "'get_safe' function done: %f seconds spent here", (end - begin) / 1000.0f)

        return true;
}

static int this_get_safe_exact(struct strhash *self, field_sid_t *out, bool *found_mask, const char *key)
{
        assert(self->tag == MEMORY_RESIDENT);

        struct allocator hashtable_alloc;
#if defined(NG5_CONFIG_TRACE_STRING_DIC_ALLOC) && !defined(NDEBUG)
        CHECK_SUCCESS(allocator_TRACE(&hashtable_alloc));
#else
        ng5_check_success(alloc_this_or_std(&hashtable_alloc, &self->allocator));
#endif

        struct mem_extra *extra = this_get_exta(self);

        hash32_t hash = strcmp("", key) != 0 ? HASHCODE_OF(key) : 0;
        size_t bucket_idx = hash % extra->buckets.cap_elems;
        prefetch_read((struct bucket *) vec_data(&extra->buckets) + bucket_idx);

        ng5_check_success(this_fetch_single(&extra->buckets, out, found_mask, bucket_idx, key, &self->counters));

        return true;
}

static int this_get_fast(struct strhash *self, field_sid_t **out, char *const *keys, size_t num_keys)
{
        bool *found_mask;
        size_t num_not_found;
        int status = this_get_safe(self, out, &found_mask, &num_not_found, keys, num_keys);
        this_free(self, found_mask);
        return status;
}

static int this_update_key_fast(struct strhash *self, const field_sid_t *values, char *const *keys, size_t num_keys)
{
        ng5_unused(self);
        ng5_unused(values);
        ng5_unused(keys);
        ng5_unused(num_keys);
        error(&self->err, NG5_ERR_NOTIMPL);
        error_print_to_stderr(&self->err);
        return false;
}

static int simple_map_remove(struct mem_extra *extra, size_t *bucket_idxs, char *const *keys, size_t num_keys,
        struct allocator *alloc, struct strhash_counters *counter)
{
        ng5_unused(counter);
        ng5_unused(alloc);

        slice_handle_t handle;
        struct bucket *data = (struct bucket *) vec_data(&extra->buckets);

        for (register size_t i = 0; i < num_keys; i++) {
                struct bucket *bucket = data + bucket_idxs[i];
                const char *key = keys[i];

                /** Optimization 1/5: EMPTY GUARD (but before "find" call); if this bucket has no occupied slots, do not perform any lookup and comparison */
                slice_list_lookup(&handle, &bucket->slice_list, key);
                if (likely(handle.is_contained)) {
                        SliceListRemove(&bucket->slice_list, &handle);
                }
        }
        return true;
}

static int this_remove(struct strhash *self, char *const *keys, size_t num_keys)
{
        assert(self->tag == MEMORY_RESIDENT);

        struct mem_extra *extra = this_get_exta(self);
        size_t *bucket_idxs = alloc_malloc(&self->allocator, num_keys * sizeof(size_t));
        for (register size_t i = 0; i < num_keys; i++) {
                const char *key = keys[i];
                hash32_t hash = HASHCODE_OF(key);
                bucket_idxs[i] = hash % extra->buckets.cap_elems;
        }

        ng5_check_success(simple_map_remove(extra, bucket_idxs, keys, num_keys, &self->allocator, &self->counters));
        ng5_check_success(alloc_free(&self->allocator, bucket_idxs));
        return true;
}

static int this_free(struct strhash *self, void *ptr)
{
        assert(self->tag == MEMORY_RESIDENT);
        ng5_check_success(alloc_free(&self->allocator, ptr));
        return true;
}

ng5_func_unused
static int this_create_extra(struct strhash *self, size_t num_buckets, size_t cap_buckets)
{
        if ((self->extra = alloc_malloc(&self->allocator, sizeof(struct mem_extra))) != NULL) {
                struct mem_extra *extra = this_get_exta(self);
                vec_create(&extra->buckets, &self->allocator, sizeof(struct bucket), num_buckets);

                /** Optimization: notify the kernel that the list of buckets are accessed randomly (since hash based access)*/
                vec_memadvice(&extra->buckets, MADV_RANDOM | MADV_WILLNEED);

                struct bucket *data = (struct bucket *) vec_data(&extra->buckets);
                ng5_check_success(bucket_create(data, num_buckets, cap_buckets, &self->allocator));
                return true;
        } else {
                error(&self->err, NG5_ERR_MALLOCERR);
                return false;
        }
}

ng5_func_unused
static struct mem_extra *this_get_exta(struct strhash *self)
{
        assert (self->tag == MEMORY_RESIDENT);
        return (struct mem_extra *) (self->extra);
}

ng5_func_unused
static int bucket_create(struct bucket *buckets, size_t num_buckets, size_t bucket_cap, struct allocator *alloc)
{
        error_if_null(buckets);

        // TODO: parallize this!
        while (num_buckets--) {
                struct bucket *bucket = buckets++;
                slice_list_create(&bucket->slice_list, alloc, bucket_cap);
        }

        return true;
}

static int bucket_drop(struct bucket *buckets, size_t num_buckets, struct allocator *alloc)
{
        ng5_unused(alloc);
        error_if_null(buckets);

        while (num_buckets--) {
                struct bucket *bucket = buckets++;
                SliceListDrop(&bucket->slice_list);
        }

        return true;
}

static int bucket_insert(struct bucket *bucket, const char *restrict key, field_sid_t value, struct allocator *alloc,
        struct strhash_counters *counter)
{
        ng5_unused(counter);
        ng5_unused(alloc);

        error_if_null(bucket);
        error_if_null(key);

        slice_handle_t handle;

        /** Optimization 1/5: EMPTY GUARD (but before "find" call); if this bucket has no occupied slots, do not perform any lookup and comparison */
        slice_list_lookup(&handle, &bucket->slice_list, key);

        if (handle.is_contained) {
                /** entry found by keys */
                assert(value == handle.value);
                //debug(SMART_MAP_TAG, "debug(SMART_MAP_TAG, \"*** put *** '%s' into bucket [new]\", keys);*** put *** '%s' into bucket [already contained]", keys);
        } else {
                /** no entry found */
                //debug(SMART_MAP_TAG, "*** put *** '%s' into bucket [new]", keys);
                slice_list_insert(&bucket->slice_list, (char **) &key, &value, 1);
        }

        return true;
}

static int this_insert_bulk(struct vector ofType(bucket) *buckets, char *const *restrict keys,
        const field_sid_t *restrict values, size_t *restrict bucket_idxs, size_t num_pairs, struct allocator *alloc,
        struct strhash_counters *counter)
{
        error_if_null(buckets)
        error_if_null(keys)
        error_if_null(values)
        error_if_null(bucket_idxs)

        struct bucket *buckets_data = (struct bucket *) vec_data(buckets);
        int status = true;
        for (register size_t i = 0; status == true && i < num_pairs; i++) {
                size_t bucket_idx = bucket_idxs[i];
                const char *key = keys[i];
                field_sid_t value = values[i];

                struct bucket *bucket = buckets_data + bucket_idx;
                status = bucket_insert(bucket, key, value, alloc, counter);
        }

        return status;
}

static int this_insert_exact(struct vector ofType(bucket) *buckets, const char *restrict key, field_sid_t value,
        size_t bucket_idx, struct allocator *alloc, struct strhash_counters *counter)
{
        error_if_null(buckets)
        error_if_null(key)

        struct bucket *buckets_data = (struct bucket *) vec_data(buckets);
        struct bucket *bucket = buckets_data + bucket_idx;
        return bucket_insert(bucket, key, value, alloc, counter);
}
