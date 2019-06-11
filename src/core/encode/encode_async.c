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

#include "core/alloc/alloc.h"
#include "std/vec.h"
#include "core/encode/encode_sync.h"
#include "core/encode/encode_async.h"
#include "core/async/spin.h"
#include "stdx/strhash.h"
#include "utils/time.h"
#include "core/async/parallel.h"
#include "stdx/slicelist.h"
#include "hash/sax.h"

#define STRING_DIC_ASYNC_TAG "strdic_async"

#define HASH_FUNCTION                  NG5_HASH_SAX

struct carrier {
        struct strdic local_dictionary;
        pthread_t thread;
        size_t id;
};

struct async_extra {
        struct vector ofType(carrier) carriers;
        struct vector ofType(struct carrier *) carrier_mapping;
        struct spinlock lock;
};

struct parallel_insert_arg {
        struct vector ofType(char *) strings;
        field_sid_t *out;
        struct carrier *carrier;
        bool enable_write_out;
        bool did_work;
        uint_fast16_t insert_num_threads;
};

struct parallel_remove_arg {
        struct vector ofType(field_sid_t) *local_ids;
        struct carrier *carrier;
        int result;
        bool did_work;
};

struct parallel_locate_arg {
        struct carrier *carrier;
        field_sid_t *ids_out;
        bool *found_mask_out;
        size_t num_not_found_out;
        struct vector ofType(char *) keys_in;
        int result;
        bool did_work;
};

struct parallel_extract_arg {
        struct vector ofType(field_sid_t) local_ids_in;
        char **strings_out;
        struct carrier *carrier;
        bool did_work;
};

#define HASHCODE_OF(string)                                                                                            \
    HASH_FUNCTION(strlen(string), string)

#define MAKE_GLOBAL(thread_id, localstring_id_t)                                                                \
    ((thread_id << 54) | localstring_id_t)

#define GET_OWNER(globalId)                                                                                            \
    (globalId >> 54)

#define GET_string_id_t(globalId)                                                                               \
    ((~((field_sid_t) 0)) >> 10 & global_string_id);

static bool this_drop(struct strdic *self);
static bool this_insert(struct strdic *self, field_sid_t **out, char *const *strings, size_t num_strings,
        size_t __num_threads);
static bool this_remove(struct strdic *self, field_sid_t *strings, size_t num_strings);
static bool this_locate_safe(struct strdic *self, field_sid_t **out, bool **found_mask, size_t *num_not_found,
        char *const *keys, size_t num_keys);
static bool this_locate_fast(struct strdic *self, field_sid_t **out, char *const *keys, size_t num_keys);
static char **this_extract(struct strdic *self, const field_sid_t *ids, size_t num_ids);
static bool this_free(struct strdic *self, void *ptr);

static bool this_num_distinct(struct strdic *self, size_t *num);
static bool this_get_contents(struct strdic *self, struct vector ofType (char *) *strings,
        struct vector ofType(field_sid_t) *string_ids);

static bool this_reset_counters(struct strdic *self);
static bool this_counters(struct strdic *self, struct strhash_counters *counters);

static bool this_lock(struct strdic *self);
static bool this_unlock(struct strdic *self);

static bool this_create_extra(struct strdic *self, size_t capacity, size_t num_index_buckets,
        size_t approx_num_unique_str, size_t num_threads);

static bool this_setup_carriers(struct strdic *self, size_t capacity, size_t num_index_buckets,
        size_t approx_num_unique_str, size_t num_threads);

#define THIS_EXTRAS(self)                                                                                              \
({                                                                                                                     \
    ng5_check_tag(self->tag, ASYNC);                                                             \
    (struct async_extra *) self->extra;                                                                                       \
})

NG5_EXPORT (int) encode_async_create(struct strdic *dic, size_t capacity, size_t num_index_buckets,
        size_t approx_num_unique_strs, size_t num_threads, const struct allocator *alloc)
{
        ng5_check_success(alloc_this_or_std(&dic->alloc, alloc));

        dic->tag = ASYNC;
        dic->drop = this_drop;
        dic->insert = this_insert;
        dic->remove = this_remove;
        dic->locate_safe = this_locate_safe;
        dic->locate_fast = this_locate_fast;
        dic->extract = this_extract;
        dic->free = this_free;
        dic->resetCounters = this_reset_counters;
        dic->counters = this_counters;
        dic->num_distinct = this_num_distinct;
        dic->get_contents = this_get_contents;

        ng5_check_success(this_create_extra(dic, capacity, num_index_buckets, approx_num_unique_strs, num_threads));
        return true;
}

static bool this_create_extra(struct strdic *self, size_t capacity, size_t num_index_buckets,
        size_t approx_num_unique_str, size_t num_threads)
{
        assert(self);

        self->extra = alloc_malloc(&self->alloc, sizeof(struct async_extra));
        struct async_extra *extra = THIS_EXTRAS(self);
        spin_init(&extra->lock);
        vec_create(&extra->carriers, &self->alloc, sizeof(struct carrier), num_threads);
        this_setup_carriers(self, capacity, num_index_buckets, approx_num_unique_str, num_threads);
        vec_create(&extra->carrier_mapping, &self->alloc, sizeof(struct carrier *), capacity);

        return true;
}

static bool this_drop(struct strdic *self)
{
        ng5_check_tag(self->tag, ASYNC);
        struct async_extra *extra = THIS_EXTRAS(self);
        for (size_t i = 0; i < extra->carriers.num_elems; i++) {
                struct carrier *carrier = vec_get(&extra->carriers, i, struct carrier);
                strdic_drop(&carrier->local_dictionary);
        }
        ng5_check_success(vec_drop(&extra->carriers));
        ng5_check_success(vec_drop(&extra->carrier_mapping));
        ng5_check_success(alloc_free(&self->alloc, extra));
        return true;
}

void *parallel_remove_function(void *args)
{
        struct parallel_remove_arg *carrier_arg = (struct parallel_remove_arg *) args;
        field_sid_t len = vec_length(carrier_arg->local_ids);
        carrier_arg->did_work = len > 0;

        ng5_debug(STRING_DIC_ASYNC_TAG,
                "thread %zu spawned for remove task (%zu elements)",
                carrier_arg->carrier->id,
                vec_length(carrier_arg->local_ids));
        if (len > 0) {
                struct strdic *dic = &carrier_arg->carrier->local_dictionary;
                field_sid_t *ids = vec_all(carrier_arg->local_ids, field_sid_t);
                carrier_arg->result = strdic_remove(dic, ids, len);
                ng5_debug(STRING_DIC_ASYNC_TAG, "thread %zu task done", carrier_arg->carrier->id);
        } else {
                carrier_arg->result = true;
                ng5_warn(STRING_DIC_ASYNC_TAG, "thread %zu had nothing to do", carrier_arg->carrier->id);
        }

        return NULL;
}

void *parallel_insert_function(void *args)
{
        struct parallel_insert_arg *restrict this_args = (struct parallel_insert_arg *restrict) args;
        this_args->did_work = this_args->strings.num_elems > 0;

        ng5_trace(STRING_DIC_ASYNC_TAG, "thread-local insert function started (thread %zu)", this_args->carrier->id);
        ng5_debug(STRING_DIC_ASYNC_TAG,
                "thread %zu spawned for insert task (%zu elements)",
                this_args->carrier->id,
                vec_length(&this_args->strings));

        if (this_args->did_work) {
                ng5_trace(STRING_DIC_ASYNC_TAG,
                        "thread %zu starts insertion of %zu strings",
                        this_args->carrier->id,
                        vec_length(&this_args->strings));
                char **data = (char **) vec_data(&this_args->strings);

                int status = strdic_insert(&this_args->carrier->local_dictionary,
                        this_args->enable_write_out ? &this_args->out : NULL,
                        data,
                        vec_length(&this_args->strings),
                        this_args->insert_num_threads);

                /** internal error during thread-local string dictionary building process */
                error_print_and_die_if(status != true, NG5_ERR_INTERNALERR);
                ng5_debug(STRING_DIC_ASYNC_TAG, "thread %zu done", this_args->carrier->id);
        } else {
                ng5_warn(STRING_DIC_ASYNC_TAG, "thread %zu had nothing to do", this_args->carrier->id);
        }

        return NULL;
}

void *parallel_locate_safe_function(void *args)
{
        struct parallel_locate_arg *restrict this_args = (struct parallel_locate_arg *restrict) args;
        this_args->did_work = vec_length(&this_args->keys_in) > 0;

        ng5_trace(STRING_DIC_ASYNC_TAG,
                "thread-local 'locate' function invoked for thread %zu...",
                this_args->carrier->id)

        ng5_debug(STRING_DIC_ASYNC_TAG,
                "thread %zu spawned for locate (safe) task (%zu elements)",
                this_args->carrier->id,
                vec_length(&this_args->keys_in));

        if (this_args->did_work) {
                this_args->result = strdic_locate_safe(&this_args->ids_out,
                        &this_args->found_mask_out,
                        &this_args->num_not_found_out,
                        &this_args->carrier->local_dictionary,
                        vec_all(&this_args->keys_in, char *),
                        vec_length(&this_args->keys_in));

                ng5_debug(STRING_DIC_ASYNC_TAG, "thread %zu done", this_args->carrier->id);
        } else {
                ng5_warn(STRING_DIC_ASYNC_TAG, "thread %zu had nothing to do", this_args->carrier->id);
        }

        return NULL;
}

void *parallel_extract_function(void *args)
{
        struct parallel_extract_arg *restrict this_args = (struct parallel_extract_arg *restrict) args;
        this_args->did_work = vec_length(&this_args->local_ids_in) > 0;

        ng5_debug(STRING_DIC_ASYNC_TAG,
                "thread %zu spawned for extract task (%zu elements)",
                this_args->carrier->id,
                vec_length(&this_args->local_ids_in));

        if (this_args->did_work) {
                this_args->strings_out = strdic_extract(&this_args->carrier->local_dictionary,
                        vec_all(&this_args->local_ids_in, field_sid_t),
                        vec_length(&this_args->local_ids_in));
                ng5_debug(STRING_DIC_ASYNC_TAG, "thread %zu done", this_args->carrier->id);
        } else {
                ng5_warn(STRING_DIC_ASYNC_TAG, "thread %zu had nothing to do", this_args->carrier->id);
        }

        return NULL;
}

static void synchronize(struct vector ofType(carrier) *carriers, size_t num_threads)
{
        ng5_debug(STRING_DIC_ASYNC_TAG, "barrier installed for %d threads", num_threads);

        timestamp_t begin = time_now_wallclock();
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                volatile struct carrier *carrier = vec_get(carriers, thread_id, struct carrier);
                pthread_join(carrier->thread, NULL);
                ng5_debug(STRING_DIC_ASYNC_TAG, "thread %d joined", carrier->id);
        }
        timestamp_t end = time_now_wallclock();
        timestamp_t duration = (end - begin);
        ng5_unused(duration);

        ng5_debug(STRING_DIC_ASYNC_TAG,
                "barrier passed for %d threads after %f seconds",
                num_threads,
                duration / 1000.0f);
}

static void create_thread_assignment(atomic_uint_fast16_t **str_carrier_mapping, atomic_size_t **carrier_num_strings,
        size_t **str_carrier_idx_mapping, struct allocator *alloc, size_t num_strings, size_t num_threads)
{
        /** parallel_map_exec string depending on hash values to a particular carrier */
        *str_carrier_mapping = alloc_malloc(alloc, num_strings * sizeof(atomic_uint_fast16_t));
        memset(*str_carrier_mapping, 0, num_strings * sizeof(atomic_uint_fast16_t));

        /** counters to compute how many strings go to a particular carrier */
        *carrier_num_strings = alloc_malloc(alloc, num_threads * sizeof(atomic_size_t));
        memset(*carrier_num_strings, 0, num_threads * sizeof(atomic_size_t));

        /** an inverted index that contains the i-th position for string k that was assigned to carrier m.
         * With this, given a (global) string and and its carrier, one can have directly the position of the
         * string in the carriers "thread-local locate" args */
        *str_carrier_idx_mapping = alloc_malloc(alloc, num_strings * sizeof(size_t));
}

static void drop_thread_assignment(struct allocator *alloc, atomic_uint_fast16_t *str_carrier_mapping,
        atomic_size_t *carrier_num_strings, size_t *str_carrier_idx_mapping)
{
        alloc_free(alloc, carrier_num_strings);
        alloc_free(alloc, str_carrier_mapping);
        alloc_free(alloc, str_carrier_idx_mapping);
}

struct thread_assign_arg {
        atomic_uint_fast16_t *str_carrier_mapping;
        size_t num_threads;
        atomic_size_t *carrier_num_strings;
        char *const *base_strings;
};

static void parallel_compute_thread_assignment_function(const void *restrict start, size_t width, size_t len,
        void *restrict args, thread_id_t tid)
{
        ng5_unused(tid);
        ng5_unused(width);

        char *const *strings = (char *const *) start;

        struct thread_assign_arg *func_args = (struct thread_assign_arg *) args;

        while (len--) {
                size_t i = strings - func_args->base_strings;
                const char *key = *strings;
                /** re-using this hashcode for the thread-local dictionary is more costly than to compute it fresh
                 * (due to more I/O with the RAM) */
                size_t thread_id = HASHCODE_OF(key) % func_args->num_threads;
                atomic_fetch_add(&func_args->str_carrier_mapping[i], thread_id);
                atomic_fetch_add(&func_args->carrier_num_strings[thread_id], 1);
                strings++;
        }
}

static void compute_thread_assignment(atomic_uint_fast16_t *str_carrier_mapping, atomic_size_t *carrier_num_strings,
        char *const *strings, size_t num_strings, size_t num_threads)
{
        struct thread_assign_arg args =
                {.base_strings = strings, .carrier_num_strings = carrier_num_strings, .num_threads = num_threads, .str_carrier_mapping = str_carrier_mapping};
        parallel_for(strings,
                sizeof(char *const *),
                num_strings,
                parallel_compute_thread_assignment_function,
                &args,
                THREADING_HINT_MULTI,
                num_threads);

}

static bool this_insert(struct strdic *self, field_sid_t **out, char *const *strings, size_t num_strings,
        size_t __num_threads)
{
        timestamp_t begin = time_now_wallclock();
        ng5_info(STRING_DIC_ASYNC_TAG, "insert operation invoked: %zu strings in total", num_strings)

        ng5_check_tag(self->tag, ASYNC);
        /** parameter 'num_threads' must be set to 0 for async dictionary */
        error_print_and_die_if(__num_threads != 0, NG5_ERR_INTERNALERR);

        this_lock(self);

        struct async_extra *extra = THIS_EXTRAS(self);
        uint_fast16_t num_threads = vec_length(&extra->carriers);

        atomic_uint_fast16_t *str_carrier_mapping;
        size_t *str_carrier_idx_mapping;
        atomic_size_t *carrier_num_strings;

        create_thread_assignment(&str_carrier_mapping,
                &carrier_num_strings,
                &str_carrier_idx_mapping,
                &self->alloc,
                num_strings,
                num_threads);

        struct vector ofType(struct parallel_insert_arg *) carrier_args;
        vec_create(&carrier_args, &self->alloc, sizeof(struct parallel_insert_arg *), num_threads);

        /** compute which carrier is responsible for which string */
        compute_thread_assignment(str_carrier_mapping, carrier_num_strings, strings, num_strings, num_threads);

        /** prepare to move string subsets to carriers */
        for (uint_fast16_t i = 0; i < num_threads; i++) {
                struct parallel_insert_arg *entry = alloc_malloc(&self->alloc, sizeof(struct parallel_insert_arg));
                entry->carrier = vec_get(&extra->carriers, i, struct carrier);
                entry->insert_num_threads = num_threads;

                vec_create(&entry->strings, &self->alloc, sizeof(char *), ng5_max(1, carrier_num_strings[i]));
                vec_push(&carrier_args, &entry, 1);
                assert (entry->strings.base != NULL);

                struct parallel_insert_arg *carrier_arg = *vec_get(&carrier_args, i, struct parallel_insert_arg *);
                carrier_arg->out = NULL;
        }

        /** create per-carrier string subset */
        /** parallizing this makes no sense but waste of resources and energy */
        for (size_t i = 0; i < num_strings; i++) {
                uint_fast16_t thread_id = str_carrier_mapping[i];
                struct parallel_insert_arg
                        *carrier_arg = *vec_get(&carrier_args, thread_id, struct parallel_insert_arg *);
                carrier_arg->enable_write_out = out != NULL;

                /** store local index of string i inside the thread */
                str_carrier_idx_mapping[i] = vec_length(&carrier_arg->strings);

                vec_push(&carrier_arg->strings, &strings[i], 1);
        }


        /** schedule insert operation per carrier */
        ng5_trace(STRING_DIC_ASYNC_TAG, "schedule insert operation to %zu threads", num_threads)
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_insert_arg
                        *carrier_arg = *vec_get(&carrier_args, thread_id, struct parallel_insert_arg *);
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                ng5_trace(STRING_DIC_ASYNC_TAG, "create thread %zu...", thread_id)
                pthread_create(&carrier->thread, NULL, parallel_insert_function, carrier_arg);
                ng5_trace(STRING_DIC_ASYNC_TAG, "thread %zu created", thread_id)
        }
        ng5_trace(STRING_DIC_ASYNC_TAG, "scheduling done for %zu threads", num_threads)

        /** synchronize */
        ng5_trace(STRING_DIC_ASYNC_TAG, "start synchronizing %zu threads", num_threads)
        synchronize(&extra->carriers, num_threads);
        ng5_trace(STRING_DIC_ASYNC_TAG, "%zu threads in sync", num_threads)

        /** compute string ids; the string id produced by this implementation is a compound identifier encoding
         * both the owning thread id and the thread-local string id. For this, the returned (global) string identifier
         * is split into 10bits encoded the thread (given a maximum of 1024 threads that can be handled by this
         * implementation), and 54bits used to encode the thread-local string id
         *
         * TECHNICAL LIMIT: 1024 threads
         */

        /** optionally, return the created string ids. In case 'out' is NULL, nothing has to be done (especially
         * none of the carrier threads allocated thread-local 'out's which mean that no cleanup must be done */

        /** parallelizing the following block makes no sense but waste of compute power and energy */
        if (likely(out != NULL)) {
                field_sid_t *total_out = alloc_malloc(&self->alloc, num_strings * sizeof(field_sid_t));
                size_t currentOut = 0;

                for (size_t string_idx = 0; string_idx < num_strings; string_idx++) {
                        uint_fast16_t thread_id = str_carrier_mapping[string_idx];
                        size_t localIdx = str_carrier_idx_mapping[string_idx];
                        struct parallel_insert_arg
                                *carrier_arg = *vec_get(&carrier_args, thread_id, struct parallel_insert_arg *);
                        field_sid_t global_string_owner_id = thread_id;
                        field_sid_t global_string_local_id = carrier_arg->out[localIdx];
                        field_sid_t global_string_id = MAKE_GLOBAL(global_string_owner_id, global_string_local_id);
                        total_out[currentOut++] = global_string_id;
                }

                *out = total_out;
        }

        /** cleanup */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_insert_arg
                        *carrier_arg = *vec_get(&carrier_args, thread_id, struct parallel_insert_arg *);
                if (carrier_arg->did_work) {
                        strdic_free(&carrier_arg->carrier->local_dictionary, carrier_arg->out);
                }
                vec_drop(&carrier_arg->strings);
                alloc_free(&self->alloc, carrier_arg);
        }

        /** cleanup */
        drop_thread_assignment(&self->alloc, str_carrier_mapping, carrier_num_strings, str_carrier_idx_mapping);
        vec_drop(&carrier_args);

        this_unlock(self);

        timestamp_t end = time_now_wallclock();
        ng5_unused(begin);
        ng5_unused(end);
        ng5_info(STRING_DIC_ASYNC_TAG, "insertion operation done: %f seconds spent here", (end - begin) / 1000.0f)

        return true;
}

static bool this_remove(struct strdic *self, field_sid_t *strings, size_t num_strings)
{
        timestamp_t begin = time_now_wallclock();
        ng5_info(STRING_DIC_ASYNC_TAG, "remove operation started: %zu strings to remove", num_strings);

        ng5_check_tag(self->tag, ASYNC);

        this_lock(self);

        struct parallel_remove_arg empty;
        struct async_extra *extra = THIS_EXTRAS(self);
        uint_fast16_t num_threads = vec_length(&extra->carriers);
        size_t approx_num_strings_per_thread = ng5_max(1, num_strings / num_threads);
        struct vector ofType(field_sid_t) *string_map = alloc_malloc(&self->alloc, num_threads * sizeof(struct vector));

        struct vector ofType(struct parallel_remove_arg) carrier_args;
        vec_create(&carrier_args, &self->alloc, sizeof(struct parallel_remove_arg), num_threads);

        /** prepare thread-local subset of string ids */
        vec_repeated_push(&carrier_args, &empty, num_threads);
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                vec_create(string_map + thread_id, &self->alloc, sizeof(field_sid_t), approx_num_strings_per_thread);
        }

        /** compute subset of string ids per thread  */
        for (size_t i = 0; i < num_strings; i++) {
                field_sid_t global_string_id = strings[i];
                uint_fast16_t owning_thread_id = GET_OWNER(global_string_id);
                field_sid_t localstring_id_t = GET_string_id_t(global_string_id);
                assert(owning_thread_id < num_threads);

                vec_push(string_map + owning_thread_id, &localstring_id_t, 1);
        }

        /** schedule remove operation per carrier */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                struct parallel_remove_arg *carrier_arg = vec_get(&carrier_args, thread_id, struct parallel_remove_arg);
                carrier_arg->carrier = carrier;
                carrier_arg->local_ids = string_map + thread_id;

                pthread_create(&carrier->thread, NULL, parallel_remove_function, carrier_arg);
        }

        /** synchronize */
        synchronize(&extra->carriers, num_threads);

        /** cleanup */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                vec_drop(string_map + thread_id);
        }

        alloc_free(&self->alloc, string_map);
        vec_data(&carrier_args);

        this_unlock(self);

        timestamp_t end = time_now_wallclock();
        ng5_unused(begin);
        ng5_unused(end);
        ng5_info(STRING_DIC_ASYNC_TAG, "remove operation done: %f seconds spent here", (end - begin) / 1000.0f)

        return true;
}

static bool this_locate_safe(struct strdic *self, field_sid_t **out, bool **found_mask, size_t *num_not_found,
        char *const *keys, size_t num_keys)
{
        timestamp_t begin = time_now_wallclock();
        ng5_info(STRING_DIC_ASYNC_TAG, "locate (safe) operation started: %zu strings to locate", num_keys)

        ng5_check_tag(self->tag, ASYNC);

        this_lock(self);

        struct async_extra *extra = THIS_EXTRAS(self);
        uint_fast16_t num_threads = vec_length(&extra->carriers);

        /** global result output */
        ng5_malloc(field_sid_t, global_out, num_keys, &self->alloc);
        ng5_malloc(bool, global_found_mask, num_keys, &self->alloc);

        size_t global_num_not_found = 0;

        atomic_uint_fast16_t *str_carrier_mapping;
        size_t *str_carrier_idx_mapping;
        atomic_size_t *carrier_num_strings;

        struct parallel_locate_arg carrier_args[num_threads];

        create_thread_assignment(&str_carrier_mapping,
                &carrier_num_strings,
                &str_carrier_idx_mapping,
                &self->alloc,
                num_keys,
                num_threads);

        /** compute which carrier is responsible for which string */
        compute_thread_assignment(str_carrier_mapping, carrier_num_strings, keys, num_keys, num_threads);

        /** prepare to move string subsets to carriers */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_locate_arg *arg = carrier_args + thread_id;
                vec_create(&arg->keys_in, &self->alloc, sizeof(char *), carrier_num_strings[thread_id]);
                assert (&arg->keys_in.base != NULL);
        }

        ng5_trace(STRING_DIC_ASYNC_TAG, "computing per-thread string subset for %zu strings", num_keys)
        /** create per-carrier string subset */
        for (size_t i = 0; i < num_keys; i++) {
                /** get thread responsible for this particular string */
                uint_fast16_t thread_id = str_carrier_mapping[i];

                /** get the thread-local argument list for the thread that is responsible for this particular string */
                struct parallel_locate_arg *arg = carrier_args + thread_id;

                /** store local index of string i inside the thread */
                str_carrier_idx_mapping[i] = vec_length(&arg->keys_in);

                /** push that string into the thread-local vector */
                vec_push(&arg->keys_in, &keys[i], 1);
        }

        ng5_trace(STRING_DIC_ASYNC_TAG, "schedule operation to threads to %zu threads...", num_threads)
        /** schedule operation to threads */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                struct parallel_locate_arg *arg = carrier_args + thread_id;
                carrier_args[thread_id].carrier = carrier;
                pthread_create(&carrier->thread, NULL, parallel_locate_safe_function, arg);
        }

        /** synchronize */
        ng5_trace(STRING_DIC_ASYNC_TAG, "start syncing %zu threads...", num_threads)
        synchronize(&extra->carriers, num_threads);
        ng5_trace(STRING_DIC_ASYNC_TAG, "%zu threads in sync.", num_threads)

        /** collect and merge results */
        ng5_trace(STRING_DIC_ASYNC_TAG, "merging results of %zu threads", num_threads)
        for (size_t i = 0; i < num_keys; i++) {
                /** get thread responsible for this particular string, and local position of that string inside the
                 * thread storage */
                uint_fast16_t thread_id = str_carrier_mapping[i];
                size_t local_thread_idx = str_carrier_idx_mapping[i];

                /** get the thread-local argument list for the thread that is responsible for this particular string */
                struct parallel_locate_arg *arg = carrier_args + thread_id;

                /** merge into global result */
                field_sid_t string_id_owner = thread_id;
                field_sid_t string_id_local_idx = arg->ids_out[local_thread_idx];
                field_sid_t global_string_id = MAKE_GLOBAL(string_id_owner, string_id_local_idx);
                global_out[i] = global_string_id;
                global_found_mask[i] = arg->found_mask_out[local_thread_idx];
        }
        for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
                /** compute total number of not-found elements */
                struct parallel_locate_arg *arg = carrier_args + thread_id;
                global_num_not_found += arg->num_not_found_out;

                /** cleanup */
                if (likely(arg->did_work)) {
                        strdic_free(&arg->carrier->local_dictionary, arg->found_mask_out);
                        strdic_free(&arg->carrier->local_dictionary, arg->ids_out);
                }
        }

        ng5_trace(STRING_DIC_ASYNC_TAG, "cleanup%s", "...")

        /** cleanup */
        drop_thread_assignment(&self->alloc, str_carrier_mapping, carrier_num_strings, str_carrier_idx_mapping);

        for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_locate_arg *arg = carrier_args + thread_id;
                vec_drop(&arg->keys_in);
        }

        /** return results */
        *out = global_out;
        *found_mask = global_found_mask;
        *num_not_found = global_num_not_found;

        this_unlock(self);

        timestamp_t end = time_now_wallclock();
        ng5_unused(begin);
        ng5_unused(end);
        ng5_info(STRING_DIC_ASYNC_TAG, "locate (safe) operation done: %f seconds spent here", (end - begin) / 1000.0f)

        return true;
}

static bool this_locate_fast(struct strdic *self, field_sid_t **out, char *const *keys, size_t num_keys)
{
        ng5_check_tag(self->tag, ASYNC);

        this_lock(self);

        bool *found_mask;
        size_t num_not_found;
        int result;

        /** use safer but in principle more slower implementation */
        result = this_locate_safe(self, out, &found_mask, &num_not_found, keys, num_keys);

        /** cleanup */
        this_free(self, found_mask);

        this_unlock(self);

        return result;
}

static char **this_extract(struct strdic *self, const field_sid_t *ids, size_t num_ids)
{
        timestamp_t begin = time_now_wallclock();
        ng5_info(STRING_DIC_ASYNC_TAG, "extract (safe) operation started: %zu strings to extract", num_ids)

        if (self->tag != ASYNC) {
                return NULL;
        }

        this_lock(self);

        ng5_malloc(char *, globalResult, num_ids, &self->alloc);

        struct async_extra *extra = (struct async_extra *) self->extra;
        uint_fast16_t num_threads = vec_length(&extra->carriers);
        size_t approx_num_strings_per_thread = ng5_max(1, num_ids / num_threads);

        ng5_malloc(size_t, local_thread_idx, num_ids, &self->alloc);
        ng5_malloc(uint_fast16_t, owning_thread_ids, num_ids, &self->alloc);
        ng5_malloc(struct parallel_extract_arg, thread_args, num_threads, &self->alloc);

        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_extract_arg *arg = thread_args + thread_id;
                vec_create(&arg->local_ids_in, &self->alloc, sizeof(field_sid_t), approx_num_strings_per_thread);
        }

        /** compute subset of string ids per thread  */
        for (size_t i = 0; i < num_ids; i++) {
                field_sid_t global_string_id = ids[i];
                owning_thread_ids[i] = GET_OWNER(global_string_id);
                field_sid_t localstring_id_t = GET_string_id_t(global_string_id);
                assert(owning_thread_ids[i] < num_threads);

                struct parallel_extract_arg *arg = thread_args + owning_thread_ids[i];
                local_thread_idx[i] = vec_length(&arg->local_ids_in);
                vec_push(&arg->local_ids_in, &localstring_id_t, 1);
        }

        /** schedule remove operation per carrier */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                struct parallel_extract_arg *carrier_arg = thread_args + thread_id;
                carrier_arg->carrier = carrier;
                pthread_create(&carrier->thread, NULL, parallel_extract_function, carrier_arg);
        }

        /** synchronize */
        synchronize(&extra->carriers, num_threads);

        for (size_t i = 0; i < num_ids; i++) {
                uint_fast16_t owning_thread_id = owning_thread_ids[i];
                size_t localIdx = local_thread_idx[i];
                struct parallel_extract_arg *carrier_arg = thread_args + owning_thread_id;
                char *extractedString = carrier_arg->strings_out[localIdx];
                globalResult[i] = extractedString;
        }

        /** cleanup */
        for (uint_fast16_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct parallel_extract_arg *carrier_arg = thread_args + thread_id;
                vec_drop(&carrier_arg->local_ids_in);
                if (likely(carrier_arg->did_work)) {
                        strdic_free(&carrier_arg->carrier->local_dictionary, carrier_arg->strings_out);
                }
        }

        ng5_free(local_thread_idx, &self->alloc);
        ng5_free(owning_thread_ids, &self->alloc);
        ng5_free(thread_args, &self->alloc);

        this_unlock(self);

        timestamp_t end = time_now_wallclock();
        ng5_unused(begin);
        ng5_unused(end);
        ng5_info(STRING_DIC_ASYNC_TAG, "extract (safe) operation done: %f seconds spent here", (end - begin) / 1000.0f)

        return globalResult;
}

static bool this_free(struct strdic *self, void *ptr)
{
        ng5_check_tag(self->tag, ASYNC);
        alloc_free(&self->alloc, ptr);
        return true;
}

static bool this_num_distinct(struct strdic *self, size_t *num)
{
        ng5_check_tag(self->tag, ASYNC);
        this_lock(self);

        struct async_extra *extra = THIS_EXTRAS(self);
        size_t num_carriers = vec_length(&extra->carriers);
        struct carrier *carriers = vec_all(&extra->carriers, struct carrier);
        size_t num_distinct = 0;
        while (num_carriers--) {
                size_t local_distinct;
                strdic_num_distinct(&local_distinct, &carriers->local_dictionary);
                num_distinct += local_distinct;
                carriers++;
        }
        *num = num_distinct;
        this_unlock(self);
        return true;
}

static bool this_get_contents(struct strdic *self, struct vector ofType (char *) *strings,
        struct vector ofType(field_sid_t) *string_ids)
{
        ng5_check_tag(self->tag, ASYNC);
        this_lock(self);
        struct async_extra *extra = THIS_EXTRAS(self);
        size_t num_carriers = vec_length(&extra->carriers);
        struct vector ofType (char *) local_string_results;
        struct vector ofType (field_sid_t) local_string_id_results;
        size_t approx_num_distinct_local_values;
        this_num_distinct(self, &approx_num_distinct_local_values);
        approx_num_distinct_local_values = ng5_max(1, approx_num_distinct_local_values / extra->carriers.num_elems);
        approx_num_distinct_local_values *= 1.2f;

        vec_create(&local_string_results, NULL, sizeof(char *), approx_num_distinct_local_values);
        vec_create(&local_string_id_results, NULL, sizeof(field_sid_t), approx_num_distinct_local_values);

        for (size_t thread_id = 0; thread_id < num_carriers; thread_id++) {
                vec_clear(&local_string_results);
                vec_clear(&local_string_id_results);

                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);

                strdic_get_contents(&local_string_results, &local_string_id_results, &carrier->local_dictionary);

                assert(local_string_id_results.num_elems == local_string_results.num_elems);
                for (size_t k = 0; k < local_string_results.num_elems; k++) {
                        char *string = *vec_get(&local_string_results, k, char *);
                        field_sid_t localstring_id_t = *vec_get(&local_string_id_results, k, field_sid_t);
                        field_sid_t global_string_id = MAKE_GLOBAL(thread_id, localstring_id_t);
                        vec_push(strings, &string, 1);
                        vec_push(string_ids, &global_string_id, 1);
                }
        }

        vec_drop(&local_string_results);
        vec_drop(&local_string_id_results);
        this_unlock(self);
        return true;
}

static bool this_reset_counters(struct strdic *self)
{
        ng5_check_tag(self->tag, ASYNC);

        this_lock(self);

        struct async_extra *extra = THIS_EXTRAS(self);
        size_t num_threads = vec_length(&extra->carriers);

        for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                strdic_reset_counters(&carrier->local_dictionary);
        }

        this_unlock(self);

        return true;
}

static bool this_counters(struct strdic *self, struct strhash_counters *counters)
{
        ng5_check_tag(self->tag, ASYNC);

        this_lock(self);

        struct async_extra *extra = THIS_EXTRAS(self);
        size_t num_threads = vec_length(&extra->carriers);

        ng5_check_success(strhash_counters_init(counters));

        for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
                struct carrier *carrier = vec_get(&extra->carriers, thread_id, struct carrier);
                struct strhash_counters local_counters;
                strdic_get_counters(&local_counters, &carrier->local_dictionary);
                strhash_counters_add(counters, &local_counters);
        }

        this_unlock(self);

        return true;
}

struct create_carrier_arg {
        size_t local_capacity;
        size_t local_bucket_num;
        size_t local_bucket_cap;
        const struct allocator *alloc;
};

static void parallel_create_carrier(const void *restrict start, size_t width, size_t len, void *restrict args,
        thread_id_t tid)
{
        ng5_unused(tid);
        ng5_unused(width);

        struct carrier *carrier = (struct carrier *) start;
        const struct create_carrier_arg *createArgs = (const struct create_carrier_arg *) args;
        while (len--) {
                encode_sync_create(&carrier->local_dictionary,
                        createArgs->local_capacity,
                        createArgs->local_bucket_num,
                        createArgs->local_bucket_cap,
                        0,
                        createArgs->alloc);
                memset(&carrier->thread, 0, sizeof(pthread_t));
                carrier++;
        }
}

static bool this_setup_carriers(struct strdic *self, size_t capacity, size_t num_index_buckets,
        size_t approx_num_unique_str, size_t num_threads)
{
        struct async_extra *extra = THIS_EXTRAS(self);
        size_t local_bucket_num = ng5_max(1, num_index_buckets / num_threads);
        struct carrier new_carrier;

        struct create_carrier_arg createArgs = {.local_capacity = ng5_max(1,
                capacity / num_threads), .local_bucket_num = local_bucket_num, .local_bucket_cap = ng5_max(1,
                approx_num_unique_str / num_threads / local_bucket_num / SLICE_KEY_COLUMN_MAX_ELEMS), .alloc = &self
                ->alloc};

        for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
                new_carrier.id = thread_id;
                vec_push(&extra->carriers, &new_carrier, 1);
        }

        parallel_for(vec_all(&extra->carriers, struct carrier),
                sizeof(struct carrier),
                num_threads,
                parallel_create_carrier,
                &createArgs,
                THREADING_HINT_MULTI,
                num_threads);

        return true;
}

static bool this_lock(struct strdic *self)
{
        struct async_extra *extra = THIS_EXTRAS(self);
        ng5_check_success(spin_acquire(&extra->lock));
        return true;
}

static bool this_unlock(struct strdic *self)
{
        struct async_extra *extra = THIS_EXTRAS(self);
        ng5_check_success(spin_release(&extra->lock));
        return true;
}
