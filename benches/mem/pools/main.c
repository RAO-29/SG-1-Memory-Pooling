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

#include <stdlib.h>

#include "shared/common.h"
#include "shared/types.h"
#include "utils/time.h"
#include "core/mem/pool.h"

#include "core/mem/pools/none.h"

#define CALL_SAMPLES 60000

static void bench_pool_realloc_free_ratio(const char *impl_name);
static void bench_clib_realloc_free_ratio();

int main(int argc, char *argv[])
{
        if (argc < 2) {
                printf("usage: <allocator>\n\n"
                        "<allocator> is the identifier of an allocator implementation to bench.\n"
                        "Use 'clib/allocator' to benchmark clibs allocator, and <mem pool names>\n"
                        "to bench those implementations.\n\n");
                printf("The following <mem pool names> are registered:\n");
                for (u32 i = 0; i < pool_get_num_registered_strategies(); i++) {
                        struct pool_register_entry *e = pool_register + i;
                        struct pool_strategy s;
                        e->_create(&s);
                        printf("\t'%s'\n", s.impl_name);
                        if (e->_drop) {
                                e->_drop(&s);
                        }
                }
                printf("\nExample: %s clib/allocator\n\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        const char *allocator = argv[1];
        if (strcmp(allocator, "clib/allocator") == 0) {
                bench_clib_realloc_free_ratio();
        } else {
                bench_pool_realloc_free_ratio(allocator);
        }

        return EXIT_SUCCESS;
}

ng5_func_unused
static void bench_pool_realloc_free_ratio(const char *impl_name)
{
        struct pool pool;
        struct vector ofType(data_ptr_t) data;

        timestamp_t call_start,
                    call_end;

        float call_duration;

        printf("impl_name, rerun, alpha, realloc_calls, free_calls, called, call_duration_ms, num_allocd, num_alloc_calls,"
                "num_realloc_calls, num_free_calls, num_gc_calls, num_managed_alloc_calls, num_managed_realloc_calls, "
                "num_free_realloc_calls, impl_mem_footprint, num_bytes_allocd, num_bytes_reallocd, num_bytes_freed,"
                "num_bytes_alloc_cache, num_bytes_realloc_cache, num_bytes_free_cache, num_bytes_alloc_blocked,"
                "num_bytes_realloc_blocked, num_bytes_free_blocked\n");

        for (u32 rerun = 0; rerun < 5; rerun++) {
                for (float alpha = 0.0f; alpha <= 1.0f; alpha += 0.04f) {

                        pool_create_by_name(&pool, impl_name);
                        vec_create(&data, NULL, sizeof(data_ptr_t), 100000);


                        for (int i = 0; i < 65000; i++) {
                                data_ptr_t ptr = pool_alloc(&pool, 1 + rand() % 2048);
                                vec_push(&data, &ptr, 1);
                        }

                        /* measure realloc/free ratio impact */
                        {
                                u32 realloc_calls = 100 * alpha;
                                u32 free_calls = 100 * (1 - alpha);
                                pool_reset_counters(&pool);

                                while (realloc_calls + free_calls > 0) {
                                        enum call_function { CALL_REALLOC, CALL_FREE } call;
                                        bool b = rand() % 2 == 0;
                                        if (b && realloc_calls > 0) {
                                                call = CALL_REALLOC;
                                                realloc_calls--;
                                        } else if (!b && free_calls > 0){
                                                call = CALL_FREE;
                                                free_calls--;
                                        } else {
                                                if (free_calls) {
                                                        call = CALL_FREE;
                                                        free_calls--;
                                                } else if (realloc_calls) {
                                                        call = CALL_REALLOC;
                                                        realloc_calls--;
                                                } else {
                                                        abort();
                                                }
                                        }

                                        assert(!vec_is_empty(&data));
                                        data_ptr_t *ptrs = (data_ptr_t *) vec_data(&data);
                                        data_ptr_t *ptrs_start = ptrs;

                                        if (call == CALL_REALLOC) {
                                                call_start = time_now_wallclock();
                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        *ptrs = pool_realloc(&pool, *ptrs, 1 + rand() % 2048);
                                                        ptrs++;
                                                }
                                                call_end = time_now_wallclock();

                                        } else {
                                                call_start = time_now_wallclock();
                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        pool_free(&pool, *ptrs);
                                                        ptrs++;
                                                }
                                                call_end = time_now_wallclock();

                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        *ptrs_start = pool_alloc(&pool, 1 + rand() % 2048);
                                                        ptrs_start++;
                                                }
                                        }
                                        call_duration = (call_end - call_start)/(float) CALL_SAMPLES;


                                        struct pool_counters counters;
                                        pool_get_counters(&counters, &pool);
                                        pool_reset_counters(&pool);

                                        printf("%s, %" PRIu32 ", %0.2f, %" PRIu32 ", %" PRIu32 ", %s, %0.8f, %" PRIu32 ", %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f, %0.02f\n",
                                                pool_impl_name(&pool),
                                                rerun, alpha, realloc_calls, free_calls,
                                                call == CALL_REALLOC ? "realloc" : "free", call_duration, data.num_elems,
                                                counters.num_alloc_calls/(float) CALL_SAMPLES,
                                                counters.num_realloc_calls/(float) CALL_SAMPLES,
                                                counters.num_free_calls/(float) CALL_SAMPLES,
                                                counters.num_gc_calls/(float) CALL_SAMPLES,
                                                counters.num_managed_alloc_calls/(float) CALL_SAMPLES,
                                                counters.num_managed_realloc_calls/(float) CALL_SAMPLES,
                                                counters.num_free_realloc_calls/(float) CALL_SAMPLES,
                                                (float)counters.impl_mem_footprint,
                                                (float)counters.num_bytes_allocd,
                                                (float)counters.num_bytes_reallocd,
                                                (float)counters.num_bytes_freed,
                                                (float)counters.num_bytes_alloc_cache,
                                                (float)counters.num_bytes_realloc_cache,
                                                (float)counters.num_bytes_free_cache,
                                                (float)counters.num_bytes_alloc_blocked,
                                                (float)counters.num_bytes_realloc_blocked,
                                                (float)counters.num_bytes_free_blocked);
                                }
                        }
                        pool_free_all(&pool);
                        pool_drop(&pool);
                        vec_drop(&data);
                }
        }
}

ng5_func_unused
static void bench_clib_realloc_free_ratio()
{
        struct vector ofType(void *) data;

        timestamp_t call_start,
                call_end;

        float call_duration;

        printf("impl_name, rerun, alpha, realloc_calls, free_calls, called, call_duration_ms, num_allocd\n");

        for (u32 rerun = 0; rerun < 5; rerun++) {
                for (float alpha = 0.0f; alpha <= 1.0f; alpha += 0.04f) {

                        vec_create(&data, NULL, sizeof(void *), 100000);


                        for (int i = 0; i < 65000; i++) {
                                void *ptr = malloc(1 + rand() % 2048);
                                vec_push(&data, &ptr, 1);
                        }

                        /* meassure realloc/free ratio impact */
                        {
                                u32 realloc_calls = 100 * alpha;
                                u32 free_calls = 100 * (1 - alpha);

                                while (realloc_calls + free_calls > 0) {
                                        enum call_function { CALL_REALLOC, CALL_FREE } call;
                                        bool b = rand() % 2 == 0;
                                        if (b && realloc_calls > 0) {
                                                call = CALL_REALLOC;
                                                realloc_calls--;
                                        } else if (!b && free_calls > 0){
                                                call = CALL_FREE;
                                                free_calls--;
                                        } else {
                                                if (free_calls) {
                                                        call = CALL_FREE;
                                                        free_calls--;
                                                } else if (realloc_calls) {
                                                        call = CALL_REALLOC;
                                                        realloc_calls--;
                                                } else {
                                                        abort();
                                                }
                                        }

                                        assert(!vec_is_empty(&data));
                                        void **ptr_data_old = (void **) vec_data(&data);
                                        void **ptr_data = (void **) vec_data(&data);

                                        if (call == CALL_REALLOC) {
                                                call_start = time_now_wallclock();
                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        void *ptr = *ptr_data;
                                                        *ptr_data = realloc(ptr, 1 + rand() % 2048);
                                                        ptr_data++;
                                                }
                                                call_end = time_now_wallclock();
                                        } else {
                                                call_start = time_now_wallclock();
                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        void *ptr_to_free = *ptr_data;
                                                        free(ptr_to_free);
                                                        ptr_data++;
                                                }
                                                call_end = time_now_wallclock();
                                                for (u32 x = 0; x < CALL_SAMPLES; x++) {
                                                        *ptr_data_old = malloc(1 + rand() % 2048);
                                                        ptr_data_old++;
                                                }
                                        }
                                        call_duration = (call_end - call_start)/(float) CALL_SAMPLES;

                                        printf("%s, %" PRIu32 ", %0.2f, %" PRIu32 ", %" PRIu32 ", %s, %0.8f, %" PRIu32 "\n",
                                                "clib/allocator",
                                                rerun, alpha, realloc_calls, free_calls,
                                                call == CALL_REALLOC ? "realloc" : "free", call_duration, data.num_elems);
                                }
                        }

                        for (u32 i = 0; i < data.num_elems; i++) {
                                void *ptr = *vec_get(&data, i, void *);
                                free(ptr);
                        }
                        vec_drop(&data);
                }
        }
}