/**
 * Copyright 2019 Marcus Pinnecke
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

#include "hash/bern.h"
#include "std/hash_table.h"

#define HASHCODE_OF(size, x) NG5_HASH_BERNSTEIN(size, x)
#define FIX_MAP_AUTO_REHASH_LOADFACTOR 0.9f

NG5_EXPORT(bool) hashtable_create(struct hashtable *map, struct err *err, size_t key_size, size_t value_size,
        size_t capacity)
{
        error_if_null(map)
        error_if_null(key_size)
        error_if_null(value_size)

        int err_code = NG5_ERR_INITFAILED;

        map->size = 0;

        ng5_success_or_jump(vec_create(&map->key_data, NULL, key_size, capacity), error_handling);
        ng5_success_or_jump(vec_create(&map->value_data, NULL, value_size, capacity), cleanup_key_data_and_error);
        ng5_success_or_jump(vec_create(&map->table, NULL, sizeof(struct hashtable_bucket), capacity),
                cleanup_value_key_data_and_error);
        ng5_success_or_jump(vec_enlarge_size_to_capacity(&map->table), cleanup_key_value_table_and_error);
        ng5_success_or_jump(vec_zero_memory(&map->table), cleanup_key_value_table_and_error);
        ng5_success_or_jump(spin_init(&map->lock), cleanup_key_value_table_and_error);
        ng5_success_or_jump(error_init(&map->err), cleanup_key_value_table_and_error);

        return true;

        cleanup_key_value_table_and_error:
        if (!vec_drop(&map->table)) {
                err_code = NG5_ERR_DROPFAILED;
        }
        cleanup_value_key_data_and_error:
        if (!vec_drop(&map->value_data)) {
                err_code = NG5_ERR_DROPFAILED;
        }
        cleanup_key_data_and_error:
        if (!vec_drop(&map->key_data)) {
                err_code = NG5_ERR_DROPFAILED;
        }
        error_handling:
        error(err, err_code);
        return false;
}

NG5_EXPORT(bool) hashtable_drop(struct hashtable *map)
{
        error_if_null(map)

        bool status = true;

        status &= vec_drop(&map->table);
        status &= vec_drop(&map->value_data);
        status &= vec_drop(&map->key_data);

        if (!status) {
                error(&map->err, NG5_ERR_DROPFAILED);
        }

        return status;
}

NG5_EXPORT(struct hashtable *)hashtable_cpy(struct hashtable *src)
{
        if (src) {
                struct hashtable *cpy = malloc(sizeof(struct hashtable));

                hashtable_lock(src);

                hashtable_create(cpy,
                        &src->err,
                        src->key_data.elem_size,
                        src->value_data.elem_size,
                        src->table.cap_elems);

                assert(src->key_data.cap_elems == src->value_data.cap_elems
                        && src->value_data.cap_elems == src->table.cap_elems);
                assert((src->key_data.num_elems == src->value_data.num_elems)
                        && src->value_data.num_elems <= src->table.num_elems);

                vec_cpy_to(&cpy->key_data, &src->key_data);
                vec_cpy_to(&cpy->value_data, &src->value_data);
                vec_cpy_to(&cpy->table, &src->table);
                cpy->size = src->size;
                error_cpy(&cpy->err, &src->err);

                assert(cpy->key_data.cap_elems == src->value_data.cap_elems
                        && src->value_data.cap_elems == cpy->table.cap_elems);
                assert((cpy->key_data.num_elems == src->value_data.num_elems)
                        && src->value_data.num_elems <= cpy->table.num_elems);

                hashtable_unlock(src);
                return cpy;
        } else {
                error(&src->err, NG5_ERR_NULLPTR);
                return NULL;
        }
}

NG5_EXPORT(bool) hashtable_clear(struct hashtable *map)
{
        error_if_null(map)
        assert(map->key_data.cap_elems == map->value_data.cap_elems
                && map->value_data.cap_elems == map->table.cap_elems);
        assert((map->key_data.num_elems == map->value_data.num_elems)
                && map->value_data.num_elems <= map->table.num_elems);

        hashtable_lock(map);

        bool status = vec_clear(&map->key_data) && vec_clear(&map->value_data) && vec_zero_memory(&map->table);

        map->size = 0;

        assert(map->key_data.cap_elems == map->value_data.cap_elems
                && map->value_data.cap_elems == map->table.cap_elems);
        assert((map->key_data.num_elems == map->value_data.num_elems)
                && map->value_data.num_elems <= map->table.num_elems);

        if (!status) {
                error(&map->err, NG5_ERR_OPPFAILED);
        }

        hashtable_unlock(map);

        return status;
}

NG5_EXPORT(bool) hashtable_avg_displace(float *displace, const struct hashtable *map)
{
        error_if_null(displace);
        error_if_null(map);

        size_t sum_dis = 0;
        for (size_t i = 0; i < map->table.num_elems; i++) {
                struct hashtable_bucket *bucket = vec_get(&map->table, i, struct hashtable_bucket);
                sum_dis += abs(bucket->displacement);
        }
        *displace = (sum_dis / (float) map->table.num_elems);

        return true;
}

NG5_EXPORT(bool) hashtable_lock(struct hashtable *map)
{
        error_if_null(map)
        //spin_acquire(&map->lock);
        return true;
}

NG5_EXPORT(bool) hashtable_unlock(struct hashtable *map)
{
        error_if_null(map)
        //spin_release(&map->lock);
        return true;
}

static inline const void *get_bucket_key(const struct hashtable_bucket *bucket, const struct hashtable *map)
{
        return map->key_data.base + bucket->data_idx * map->key_data.elem_size;
}

static inline const void *get_bucket_value(const struct hashtable_bucket *bucket, const struct hashtable *map)
{
        return map->value_data.base + bucket->data_idx * map->value_data.elem_size;
}

static void insert(struct hashtable_bucket *bucket, struct hashtable *map, const void *key, const void *value,
        i32 displacement)
{
        assert(map->key_data.num_elems == map->value_data.num_elems);
        u64 idx = map->key_data.num_elems;
        void *key_datum = vec_new_and_get(&map->key_data, void *);
        void *value_datum = vec_new_and_get(&map->value_data, void *);
        memcpy(key_datum, key, map->key_data.elem_size);
        memcpy(value_datum, value, map->value_data.elem_size);
        bucket->data_idx = idx;
        bucket->in_use_flag = true;
        bucket->displacement = displacement;
        map->size++;
}

static inline uint_fast32_t insert_or_update(struct hashtable *map, const u32 *bucket_idxs, const void *keys,
        const void *values, uint_fast32_t num_pairs)
{
        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                const void *value = values + i * map->key_data.elem_size;
                u32 intended_bucket_idx = bucket_idxs[i];

                u32 bucket_idx = intended_bucket_idx;

                struct hashtable_bucket *bucket = vec_get(&map->table, bucket_idx, struct hashtable_bucket);
                if (bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) != 0) {
                        bool fitting_bucket_found = false;
                        u32 displace_idx;
                        for (displace_idx = bucket_idx + 1; displace_idx < map->table.num_elems; displace_idx++) {
                                struct hashtable_bucket
                                        *bucket = vec_get(&map->table, displace_idx, struct hashtable_bucket);
                                fitting_bucket_found = !bucket->in_use_flag || (bucket->in_use_flag
                                        && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0);
                                if (fitting_bucket_found) {
                                        break;
                                } else {
                                        i32 displacement = displace_idx - bucket_idx;
                                        const void *swap_key = get_bucket_key(bucket, map);
                                        const void *swap_value = get_bucket_value(bucket, map);

                                        if (bucket->displacement < displacement) {
                                                insert(bucket, map, key, value, displacement);
                                                insert_or_update(map, &displace_idx, swap_key, swap_value, 1);
                                                goto next_round;
                                        }
                                }
                        }
                        if (!fitting_bucket_found) {
                                for (displace_idx = 0; displace_idx < bucket_idx - 1; displace_idx++) {
                                        const struct hashtable_bucket
                                                *bucket = vec_get(&map->table, displace_idx, struct hashtable_bucket);
                                        fitting_bucket_found = !bucket->in_use_flag || (bucket->in_use_flag
                                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size)
                                                        == 0);
                                        if (fitting_bucket_found) {
                                                break;
                                        }
                                }
                        }

                        assert(fitting_bucket_found == true);
                        bucket_idx = displace_idx;
                        bucket = vec_get(&map->table, bucket_idx, struct hashtable_bucket);
                }

                bool is_update =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                if (is_update) {
                        void *bucket_value = (void *) get_bucket_value(bucket, map);
                        memcpy(bucket_value, value, map->value_data.elem_size);
                } else {
                        i32 displacement = intended_bucket_idx - bucket_idx;
                        insert(bucket, map, key, value, displacement);
                }

                next_round:
                if (map->size >= FIX_MAP_AUTO_REHASH_LOADFACTOR * map->table.cap_elems) {
                        return i + 1; /* tell the caller that pair i was inserted, but it successors not */
                }

        }
        return 0;
}

NG5_EXPORT(bool) hashtable_insert_or_update(struct hashtable *map, const void *keys, const void *values,
        uint_fast32_t num_pairs)
{
        error_if_null(map)
        error_if_null(keys)
        error_if_null(values)

        assert(map->key_data.cap_elems == map->value_data.cap_elems
                && map->value_data.cap_elems == map->table.cap_elems);
        assert((map->key_data.num_elems == map->value_data.num_elems)
                && map->value_data.num_elems <= map->table.num_elems);

        hashtable_lock(map);

        u32 *bucket_idxs = malloc(num_pairs * sizeof(u32));
        if (!bucket_idxs) {
                error(&map->err, NG5_ERR_MALLOCERR);
                return false;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                hash32_t hash = HASHCODE_OF(map->key_data.elem_size, key);
                bucket_idxs[i] = hash % map->table.num_elems;
        }

        uint_fast32_t cont_idx = 0;
        do {
                cont_idx = insert_or_update(map,
                        bucket_idxs + cont_idx,
                        keys + cont_idx * map->key_data.elem_size,
                        values + cont_idx * map->value_data.elem_size,
                        num_pairs - cont_idx);
                if (cont_idx != 0) {
                        /* rehashing is required, and [status, num_pairs) are left to be inserted */
                        if (!hashtable_rehash(map)) {
                                hashtable_unlock(map);
                                return false;
                        }
                }
        }
        while (cont_idx != 0);

        free(bucket_idxs);
        hashtable_unlock(map);

        return true;
}

struct hashtable_header {
        char marker;
        offset_t key_data_off;
        offset_t value_data_off;
        offset_t table_off;
        u32 size;
};

NG5_EXPORT(bool) hashtable_serialize(FILE *file, struct hashtable *table)
{
        offset_t header_pos = ftell(file);
        fseek(file, sizeof(struct hashtable_header), SEEK_CUR);

        offset_t key_data_off = ftell(file);
        if (!vec_serialize(file, &table->key_data)) {
                goto error_handling;
        }

        offset_t value_data_off = ftell(file);
        if (!vec_serialize(file, &table->value_data)) {
                goto error_handling;
        }

        offset_t table_off = ftell(file);
        if (!vec_serialize(file, &table->table)) {
                goto error_handling;
        }

        offset_t end = ftell(file);

        fseek(file, header_pos, SEEK_SET);
        struct hashtable_header header = {.marker = MARKER_SYMBOL_HASHTABLE_HEADER, .size = table
                ->size, .key_data_off = key_data_off, .value_data_off = value_data_off, .table_off = table_off};
        int nwrite = fwrite(&header, sizeof(struct hashtable_header), 1, file);
        error_if(nwrite != 1, &table->err, NG5_ERR_FWRITE_FAILED);
        fseek(file, end, SEEK_SET);
        return true;

        error_handling:
        fseek(file, header_pos, SEEK_SET);
        return false;
}

NG5_EXPORT(bool) hashtable_deserialize(struct hashtable *table, struct err *err, FILE *file)
{
        error_if_null(table)
        error_if_null(err)
        error_if_null(file)

        int err_code = NG5_ERR_NOERR;

        struct hashtable_header header;
        offset_t start = ftell(file);
        int nread = fread(&header, sizeof(struct hashtable_header), 1, file);
        if (nread != 1) {
                err_code = NG5_ERR_FREAD_FAILED;
                goto error_handling;
        }
        if (header.marker != MARKER_SYMBOL_HASHTABLE_HEADER) {
                err_code = NG5_ERR_CORRUPTED;
                goto error_handling;
        }

        fseek(file, header.key_data_off, SEEK_SET);
        if (!vec_deserialize(&table->key_data, err, file)) {
                err_code = err->code;
                goto error_handling;
        }

        fseek(file, header.value_data_off, SEEK_SET);
        if (!vec_deserialize(&table->value_data, err, file)) {
                err_code = err->code;
                goto error_handling;
        }

        fseek(file, header.table_off, SEEK_SET);
        if (!vec_deserialize(&table->table, err, file)) {
                err_code = err->code;
                goto error_handling;
        }

        spin_init(&table->lock);
        error_init(&table->err);
        return true;

        error_handling:
        fseek(file, start, SEEK_SET);
        error(err, err_code);
        return false;
}

NG5_EXPORT(bool) hashtable_remove_if_contained(struct hashtable *map, const void *keys, size_t num_pairs)
{
        error_if_null(map)
        error_if_null(keys)

        hashtable_lock(map);

        u32 *bucket_idxs = malloc(num_pairs * sizeof(u32));
        if (!bucket_idxs) {
                error(&map->err, NG5_ERR_MALLOCERR);
                hashtable_unlock(map);
                return false;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                bucket_idxs[i] = HASHCODE_OF(map->key_data.elem_size, key) % map->table.num_elems;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                u32 bucket_idx = bucket_idxs[i];
                u32 actual_idx = bucket_idx;
                bool bucket_found = false;

                for (u32 k = bucket_idx; !bucket_found && k < map->table.num_elems; k++) {
                        const struct hashtable_bucket *bucket = vec_get(&map->table, k, struct hashtable_bucket);
                        bucket_found = bucket->in_use_flag
                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                        actual_idx = k;
                }
                for (u32 k = 0; !bucket_found && k < bucket_idx; k++) {
                        const struct hashtable_bucket *bucket = vec_get(&map->table, k, struct hashtable_bucket);
                        bucket_found = bucket->in_use_flag
                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                        actual_idx = k;
                }

                if (bucket_found) {
                        struct hashtable_bucket *bucket = vec_get(&map->table, actual_idx, struct hashtable_bucket);
                        bucket->in_use_flag = false;
                        bucket->data_idx = 0;
                        bucket->num_probs = 0;
                }
        }

        free(bucket_idxs);

        hashtable_unlock(map);

        return true;
}

NG5_EXPORT(const void *)hashtable_get_value(struct hashtable *map, const void *key)
{
        error_if_null(map)
        error_if_null(key)

        const void *result = NULL;

        hashtable_lock(map);

        u32 bucket_idx = HASHCODE_OF(map->key_data.elem_size, key) % map->table.num_elems;
        u32 actual_idx = bucket_idx;
        bool bucket_found = false;

        for (u32 k = bucket_idx; !bucket_found && k < map->table.num_elems; k++) {
                const struct hashtable_bucket *bucket = vec_get(&map->table, k, struct hashtable_bucket);
                bucket_found =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                actual_idx = k;
        }
        for (u32 k = 0; !bucket_found && k < bucket_idx; k++) {
                const struct hashtable_bucket *bucket = vec_get(&map->table, k, struct hashtable_bucket);
                bucket_found =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                actual_idx = k;
        }

        if (bucket_found) {
                struct hashtable_bucket *bucket = vec_get(&map->table, actual_idx, struct hashtable_bucket);
                result = get_bucket_value(bucket, map);
        }

        hashtable_unlock(map);

        return result;
}

NG5_EXPORT(const void *) hashtable_get_value_or_default(struct hashtable *map, const void *key, const void *value)
{
        const void *result = hashtable_get_value(map, key);
        return result ? result : value;
}

NG5_EXPORT(bool) hashtable_get_fload_factor(float *factor, struct hashtable *map)
{
        error_if_null(factor)
        error_if_null(map)

        hashtable_lock(map);

        *factor = map->size / (float) map->table.num_elems;

        hashtable_unlock(map);

        return true;
}

NG5_EXPORT(bool) hashtable_rehash(struct hashtable *map)
{
        error_if_null(map)

        hashtable_lock(map);

        struct hashtable *cpy = hashtable_cpy(map);
        hashtable_clear(map);

        size_t new_cap = (cpy->key_data.cap_elems + 1) * 1.7f;

        vec_grow_to(&map->key_data, new_cap);
        vec_grow_to(&map->value_data, new_cap);
        vec_grow_to(&map->table, new_cap);
        vec_enlarge_size_to_capacity(&map->table);
        vec_zero_memory(&map->table);

        assert(map->key_data.cap_elems == map->value_data.cap_elems
                && map->value_data.cap_elems == map->table.cap_elems);
        assert((map->key_data.num_elems == map->value_data.num_elems)
                && map->value_data.num_elems <= map->table.num_elems);

        for (size_t i = 0; i < cpy->table.num_elems; i++) {
                struct hashtable_bucket *bucket = vec_get(&cpy->table, i, struct hashtable_bucket);
                if (bucket->in_use_flag) {
                        const void *old_key = get_bucket_key(bucket, cpy);
                        const void *old_value = get_bucket_value(bucket, cpy);
                        if (!hashtable_insert_or_update(map, old_key, old_value, 1)) {
                                error(&map->err, NG5_ERR_REHASH_NOROLLBACK)
                                hashtable_unlock(map);
                                return false;
                        }
                }
        }

        hashtable_unlock(map);
        return true;
}