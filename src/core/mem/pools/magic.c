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

#include "core/mem/pool.h"
#include "core/mem/pools/magic.h"

#define REQUIRE_INSTANCE_OF_THIS() ng5_check_tag(self->tag, POOL_IMPL_MAGIC);

static data_ptr_t this_alloc(struct pool_strategy *self, u64 nbytes);
static data_ptr_t this_realloc(struct pool_strategy *self, data_ptr_t ptr, u64 nbytes);
static bool this_free(struct pool_strategy *self, data_ptr_t ptr);
static bool this_gc(struct pool_strategy *self);
static bool this_update_counters(struct pool_strategy *self);
static bool this_reset_counters(struct pool_strategy *self);

void pool_strategy_magic_create(struct pool_strategy *dst)
{
        assert(dst);

        dst->_alloc = this_alloc;
        dst->_realloc = this_realloc;
        dst->_free = this_free;
        dst->_gc = this_gc;
        dst->_update_counters = this_update_counters;
        dst->_reset_counters = this_reset_counters;

        dst->tag = POOL_IMPL_MAGIC;
        dst->impl_name = POOL_STRATEGY_MAGIC_NAME;
}

static data_ptr_t this_alloc(struct pool_strategy *self, u64 nbytes)
{
        REQUIRE_INSTANCE_OF_THIS()

        void *ptr = malloc(nbytes);
        assert(ptr);

        self->counters.num_alloc_calls++;
        self->counters.num_bytes_allocd += nbytes;

        return pool_internal_new(self, ptr, nbytes);
}

static data_ptr_t this_realloc(struct pool_strategy *self, data_ptr_t ptr, u64 nbytes)
{
        REQUIRE_INSTANCE_OF_THIS()

        void *stored_adr, *new_adr;

        struct pool_ptr_info *info = pool_internal_get_info(self, ptr);

        self->counters.num_bytes_reallocd = nbytes;
        self->counters.num_bytes_allocd += ng5_span(info->bytes_total, nbytes);

        stored_adr = data_ptr_get_pointer(ptr);
        new_adr = realloc(stored_adr, nbytes);

        if (unlikely(!new_adr)) {
                error_print(NG5_ERR_REALLOCERR);
        } else {
                data_ptr_update(&ptr, new_adr);
                data_ptr_update(&info->ptr, new_adr);
                info->bytes_used = nbytes;
                info->bytes_total = nbytes;
                self->counters.num_realloc_calls++;
        }

        return ptr;
}

static bool this_free(struct pool_strategy *self, data_ptr_t ptr)
{
        REQUIRE_INSTANCE_OF_THIS()

        struct pool_ptr_info *info = pool_internal_get_info(self, ptr);
        void *adr = data_ptr_get_pointer(ptr);

        free (adr);
        pool_internal_delete(self, ptr);

        self->counters.num_free_calls++;
        self->counters.num_bytes_freed += info->bytes_total;

        return true;
}

static bool this_gc(struct pool_strategy *self)
{
        REQUIRE_INSTANCE_OF_THIS()
        ng5_unused(self);
        return true;
}

static bool this_update_counters(struct pool_strategy *self)
{
        REQUIRE_INSTANCE_OF_THIS()
        self->counters.impl_mem_footprint = 0;
        return true;
}

static bool this_reset_counters(struct pool_strategy *self)
{
        ng5_zero_memory(&self->counters, sizeof(struct pool_counters));
        return true;
}