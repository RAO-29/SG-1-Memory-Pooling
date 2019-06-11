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

#include <core/mem/pool.h>
#include "core/mem/pool.h"

struct pool_register_entry pool_register[] = {
        {
                .ops.pooled     = false,
                .ops.gc_sync    = false,
                .ops.gc_async   = false,
                .ops.pressure   = false,
                .ops.linear     = false,
                .ops.chunked    = false,
                .ops.balanced   = false,
                .ops.first_fit  = false,
                .ops.best_fit   = false,
                .ops.random_fit = false,
                .ops.cracked    = false,
                .ops.parallel   = false,
                .ops.simd       = false,
                .ops.dedup      = false,
                ._create = pool_strategy_none_create,
                ._drop = NULL
        },
        {
                .ops.pooled     = true,
                .ops.gc_sync    = false,
                .ops.gc_async   = false,
                .ops.pressure   = false,
                .ops.linear     = false,
                .ops.chunked    = false,
                .ops.balanced   = false,
                .ops.first_fit  = false,
                .ops.best_fit   = false,
                .ops.random_fit = false,
                .ops.cracked    = false,
                .ops.parallel   = false,
                .ops.simd       = false,
                .ops.dedup      = false,
                ._create = pool_strategy_magic_create,
                ._drop = NULL
        }
};

static void lock(struct pool *pool);
static void unlock(struct pool *pool);
static bool strategy_by_options(struct pool *pool, struct pool_strategy *strategy, enum pool_options options);
static data_ptr_t strategy_alloc(struct pool *pool, u64 nbytes);
static data_ptr_t strategy_realloc(struct pool *pool, data_ptr_t ptr, u64 nbytes);
static bool strategy_free(struct pool *pool, data_ptr_t ptr);
static bool strategy_gc(struct pool *pool);
static bool strategy_get_counters(struct pool *pool, struct pool_counters *counters);
static bool strategy_reset_counters(struct pool *pool);

static bool pool_setup(struct pool *pool)
{
        error_if_null(pool)
        error_init(&pool->err);
        ng5_check_success(vec_create(&pool->in_use, NULL, sizeof(struct pool_ptr_info), 100));
        ng5_check_success(vec_create(&pool->in_use_pos_freelist, NULL, sizeof(u16), 100));
        ng5_check_success(spin_init(&pool->lock));
        return true;
}

NG5_EXPORT(size_t) pool_get_num_registered_strategies()
{
        return NG5_ARRAY_LENGTH(pool_register);
}

NG5_EXPORT(bool) pool_create(struct pool *pool, enum pool_options options)
{
        pool_setup(pool);

        if (!strategy_by_options(pool, &pool->strategy, options)) {
                print_error_and_die(NG5_ERR_NOTIMPLEMENTED);
                return false;
        } else {
                return true;
        }
}

NG5_EXPORT(bool) pool_create_by_name(struct pool *pool, const char *name)
{
        pool_setup(pool);

        for (u32 i = 0; i < NG5_ARRAY_LENGTH(pool_register); i++) {
                struct pool_register_entry *entry = pool_register + i;
                entry->_create(&pool->strategy);
                if (strcmp(name, pool->strategy.impl_name) == 0) {
                        pool->strategy.context = pool;
                        pool->strategy._reset_counters(&pool->strategy);
                        return true;
                } else if (entry->_drop) {
                        entry->_drop(&pool->strategy);
                }
        }

        print_error_with_details_and_die(NG5_ERR_NOTFOUND, "no memory pool found by name '%s'", name);
        return false;
}

NG5_EXPORT(bool) pool_drop(struct pool *pool)
{
        error_if_null(pool)
        lock(pool);

        pool_free_all(pool);
        vec_drop(&pool->in_use);
        vec_drop(&pool->in_use_pos_freelist);

        unlock(pool);
        return true;
}

NG5_EXPORT(bool) pool_reset_counters(struct pool *pool)
{
        error_if_null(pool);
        lock(pool);

        strategy_reset_counters(pool);

        unlock(pool);
        return true;
}

NG5_EXPORT(bool) pool_get_counters(struct pool_counters *counters, struct pool *pool)
{
        error_if_null(pool);
        error_if_null(counters);
        lock(pool);

        ng5_zero_memory(counters, sizeof(struct pool_counters));
        strategy_get_counters(pool, counters);

        unlock(pool);
        return true;
}

NG5_EXPORT(data_ptr_t) pool_alloc(struct pool *pool, u64 nbytes)
{
        error_if_null(pool);
        error_if(nbytes == 0, &pool->err, NG5_ERR_ILLEGALARG);
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _alloc)
        lock(pool);
        data_ptr_t result = strategy_alloc(pool, nbytes);
        unlock(pool);
        return result;
}

NG5_EXPORT(data_ptr_t) pool_alloc_array(struct pool *pool, u32 how_many, u64 nbytes)
{
        error_if_null(pool);
        return pool_alloc(pool, how_many * nbytes);
}

NG5_EXPORT(bool) pool_free(struct pool *pool, data_ptr_t ptr)
{
        error_if_null(pool);
        error_if_null(ptr);
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _free)
        lock(pool);
        bool result = strategy_free(pool, ptr);
        unlock(pool);
        return result;
}

NG5_EXPORT(bool) pool_free_all(struct pool *pool)
{
        error_if_null(pool);
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _free)

        lock(pool);
        u32 nptrs = pool->in_use.num_elems;
        struct pool_ptr_info *ptr_infos = vec_all(&pool->in_use, struct pool_ptr_info);
        while (nptrs--) {
                struct pool_ptr_info *ptr_info = ptr_infos++;
                if (!ptr_info->is_free) {
                        bool result = strategy_free(pool, ptr_info->ptr);
                        if (unlikely(!result)) {
                                error_print(NG5_ERR_FREE_FAILED);
                                unlock(pool);
                                return false;
                        }
                }
        }
        unlock(pool);
        return true;
}

NG5_EXPORT(data_ptr_t) pool_realloc(struct pool *pool, data_ptr_t ptr, u64 nbytes)
{
        error_if_null(pool);
        error_if_null(ptr);
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _realloc)
        lock(pool);
        data_ptr_t result = strategy_realloc(pool, ptr, nbytes);
        unlock(pool);
        return result;
}

NG5_EXPORT(const char*) pool_impl_name(struct pool *pool)
{
        if (pool) {
                return pool->strategy.impl_name;
        } else return NULL;
}

NG5_EXPORT(bool) pool_gc(struct pool *pool)
{
        error_if_null(pool);
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _gc)
        lock(pool);
        bool result = strategy_gc(pool);
        unlock(pool);
        return result;
}

NG5_EXPORT(bool) pool_internal_register(data_ptr_t *dst, struct pool *pool, const void *ptr, u32 bytes_used, u32 bytes_total)
{
        assert(dst);
        assert(pool);
        assert(ptr);

        struct pool_ptr_info *pool_ptr_info;
        u16 pos;

        if (!vec_is_empty(&pool->in_use_pos_freelist)) {
                /* update-in-place */
                pos = *(u16 *) vec_pop(&pool->in_use_pos_freelist);
                pool_ptr_info = (struct pool_ptr_info *) vec_at(&pool->in_use, pos);
                assert(pool_ptr_info->is_free);
        } else {
                /* append */
                pos = vec_length(&pool->in_use);
                pool_ptr_info = vec_new_and_get(&pool->in_use, struct pool_ptr_info);

        }

        error_if(pos + 1 == UINT16_MAX, &pool->err, NG5_ERR_MEMPOOL_LIMIT);

        *pool_ptr_info = (struct pool_ptr_info) {
                .is_free = false,
                .bytes_used = bytes_used,
                .bytes_total = bytes_total
        };
        data_ptr_create(&pool_ptr_info->ptr, ptr);
        data_ptr_set_data(&pool_ptr_info->ptr, pos);

        assert(!((struct pool_ptr_info *) vec_at(&pool->in_use, pos))->is_free);
        assert(data_ptr_get(void *, ((struct pool_ptr_info *) vec_at(&pool->in_use, pos))->ptr) == ptr);

        *dst = pool_ptr_info->ptr;
        return true;
}

NG5_EXPORT(void) pool_internal_unregister(struct pool *pool, data_ptr_t ptr)
{
        assert(pool);
        assert(ptr);

        u16 pos;
        data_ptr_get_data(&pos, ptr);
        assert(pos < pool->in_use.num_elems);

        struct pool_ptr_info *pool_ptr_info = (struct pool_ptr_info *) vec_at(&pool->in_use, pos);
        assert(data_ptr_get(void *, pool_ptr_info->ptr) == data_ptr_get(void *, ptr));
        assert(!pool_ptr_info->is_free);

        pool_ptr_info->is_free = true;
        vec_push(&pool->in_use_pos_freelist, &pos, 1);
}

static void lock(struct pool *pool)
{
        spin_acquire(&pool->lock);
}

static void unlock(struct pool *pool)
{
        spin_release(&pool->lock);
}

static bool strategy_by_options(struct pool *pool, struct pool_strategy *strategy, enum pool_options options)
{
        ng5_zero_memory(strategy, sizeof(struct pool_strategy));

        bool opt_pooled    = ng5_are_bits_set(options, MEM_POOLED);
        bool opt_gc_sync   = ng5_are_bits_set(options, MEM_GC_SNYC);
        bool opt_gc_async  = ng5_are_bits_set(options, MEM_GC_ASYNC);
        bool opt_pressure  = ng5_are_bits_set(options, MEM_PRESSURE);
        bool opt_linear    = ng5_are_bits_set(options, MEM_LINEAR);
        bool opt_chunked   = ng5_are_bits_set(options, MEM_CHUNKED);
        bool opt_balanced  = ng5_are_bits_set(options, MEM_BALANCED);
        bool opt_first_fit = ng5_are_bits_set(options, MEM_FIRST_FIT);
        bool opt_best_fit  = ng5_are_bits_set(options, MEM_BEST_FIT);
        bool opt_random    = ng5_are_bits_set(options, MEM_RANDOM_FIT);
        bool opt_cracked   = ng5_are_bits_set(options, MEM_CRACKED);
        bool opt_parallel  = ng5_are_bits_set(options, MEM_PARALLEL);
        bool opt_usesimd   = ng5_are_bits_set(options, MEM_USESIMD);
        bool opt_dedup     = ng5_are_bits_set(options, MEM_AUTO_DEDUP);

        for (size_t i = 0; i < NG5_ARRAY_LENGTH(pool_register); i++) {
                struct pool_register_entry *entry = pool_register + i;
                if ((opt_pooled == entry->ops.pooled) &&
                        (opt_gc_sync == entry->ops.gc_sync) &&
                        (opt_gc_async == entry->ops.gc_async) &&
                        (opt_pressure == entry->ops.pressure) &&
                        (opt_linear == entry->ops.linear) &&
                        (opt_chunked == entry->ops.chunked) &&
                        (opt_balanced == entry->ops.balanced) &&
                        (opt_first_fit == entry->ops.first_fit) &&
                        (opt_best_fit == entry->ops.best_fit) &&
                        (opt_random == entry->ops.random_fit) &&
                        (opt_cracked == entry->ops.cracked) &&
                        (opt_parallel == entry->ops.parallel) &&
                        (opt_usesimd == entry->ops.simd) &&
                        (opt_dedup == entry->ops.dedup)) {
                        entry->_create(strategy);
                        strategy->context = pool;
                        strategy->_reset_counters(strategy);
                        assert(strategy->impl_name != 0 && strlen(strategy->impl_name) > 0);
                        return true;
                }
        }
        return false;
}

static void *strategy_alloc(struct pool *pool, u64 nbytes)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _alloc);
        return pool->strategy._alloc(&pool->strategy, nbytes);
}

static void *strategy_realloc(struct pool *pool, data_ptr_t ptr, u64 nbytes)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _realloc);
        return pool->strategy._realloc(&pool->strategy, ptr, nbytes);
}

static bool strategy_free(struct pool *pool, data_ptr_t ptr)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _free);
        return pool->strategy._free(&pool->strategy, ptr);
}

static bool strategy_gc(struct pool *pool)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _gc);
        return pool->strategy._gc(&pool->strategy);
}

static bool strategy_get_counters(struct pool *pool, struct pool_counters *counters)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _update_counters);
        bool status = pool->strategy._update_counters(&pool->strategy);
        if (likely(status)) {
                *counters = pool->strategy.counters;
                return true;
        } else {
                error_print(NG5_ERR_INTERNALERR);
                return false;
        }
}

static bool strategy_reset_counters(struct pool *pool)
{
        ng5_implemented_or_error(&pool->err, (&pool->strategy), _reset_counters);
        return pool->strategy._reset_counters(&pool->strategy);
}