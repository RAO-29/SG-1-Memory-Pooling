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

/* A handy macro to check whether an object used as parameter to internal functions (i.e., 'this_*' functions) is
 * actually an instance of this pool strategy */
#define REQUIRE_INSTANCE_OF_THIS() ng5_check_tag(self->tag, POOL_IMPL_NONE);

/* Prototype functions, see comments below */
static data_ptr_t this_alloc(struct pool_strategy *self, u64 nbytes);
static data_ptr_t this_realloc(struct pool_strategy *self, data_ptr_t ptr, u64 nbytes);
static bool this_free(struct pool_strategy *self, data_ptr_t ptr);
static bool this_gc(struct pool_strategy *self);
static bool this_update_counters(struct pool_strategy *self);
static bool this_reset_counters(struct pool_strategy *self);

/* This functions just binds functions to "the interface" of the pool strategy, such that a call to "the interface"'s
 * function is delegated to 'this_*' functions. */
void pool_strategy_none_create(struct pool_strategy *dst)
{
        /* Assert is fine in this case, since 'dst' must be non-null here (check is performed in "the interface") */
        assert(dst);

        /* Binding of functions to functions in "the interface" */
        dst->_alloc = this_alloc;
        dst->_realloc = this_realloc;
        dst->_free = this_free;
        dst->_gc = this_gc;
        dst->_update_counters = this_update_counters;
        dst->_reset_counters = this_reset_counters;

        /* Setting a 'tag' (an unique identifier) to perform a error checking that implementation-specific functions
         * (i.e., 'this_*' functions) are called on an "instance" of this pool strategy */
        dst->tag = POOL_IMPL_NONE;

        /* Set the name constant; required */
        dst->impl_name = POOL_STRATEGY_NONE_NAME;
}

/* Implementation of 'alloc' of the pool strategy. Allocation in this strategy is done by just calling 'malloc' from
 * the clib. The function returns a 'data pointer' that is a pointer carrying additional 16 bit of user-defined
 * data. This user-defined data is used to determine the position of the pointer in the memory pool's list of
 * currently allocated pointers. */
static data_ptr_t this_alloc(struct pool_strategy *self, u64 nbytes)
{
        /* Check that 'self' is an "instance" of this pool strategy */
        REQUIRE_INSTANCE_OF_THIS()

        /* Just call memory allocation with standard clib allocator */
        void *ptr = malloc(nbytes);
        assert(ptr);

        /* Do some statistics, used for evaluation */
        self->counters.num_alloc_calls++;
        self->counters.num_bytes_allocd += nbytes;

        /* Pool strategy implementation-independent required call to a macro that stores the newly allocated pointer
         * in the memory pool at a particular position. With this position information, a 'data pointer' is created
         * and returned. */
        return pool_internal_new(self, ptr, nbytes);
}

/* Implementation of 'realloc' of the pool strategy. Reallocation in this strategy is done by just calling 'realloc'
 * from the clib. As 'this_alloc', this function returns a 'data pointer' having the (new) address to the reallocated
 * memory block and an internal position inside the memory pool. */
static data_ptr_t this_realloc(struct pool_strategy *self, data_ptr_t ptr, u64 nbytes)
{
        /* Check that 'self' is an "instance" of this pool strategy */
        REQUIRE_INSTANCE_OF_THIS()

        /* The current memory address stored in the 'data pointer', and the (potentially new) address to a
         * memory block that is at least the size of 'nbytes' as requested by the caller. */
        void *stored_adr, *new_adr;

        /* To perform a simple reallocation, the position of the pointer in the memory pool must be known in order
         * to update information related to this pointer (such as size and actual memory address). Using the position
         * information stored in the 'data pointer', information related to the pointer can be fetched from the memory
         * pool. This information is stored in an object of type 'struct pool_ptr_info'. */
        struct pool_ptr_info *info;

        /* Fetch information related to the 'data pointer' from the memory pool (and check that it is not yet freed
         * and perform a small integrity check by comparing the addresses stored in the 'data pointer' given to
         * this function, and the 'data pointer' stored at the particular position in the memory pool */
        info = pool_internal_get_info(self, ptr);

        /* Perform some statistics */
        self->counters.num_bytes_reallocd = nbytes;
        self->counters.num_bytes_allocd += ng5_span(info->bytes_total, nbytes);

        /* This is the actual reallocation logic. In principle, it just calls a 'realloc' and updated the
         * 'data pointer' and the memory pool info with the new address provided by 'realloc'. */

        /* Retrieve the memory address managed by clibs allocator that is stored in the 'data pointer',
         * and perform a reallocation */
        stored_adr = data_ptr_get_pointer(ptr);
        new_adr = realloc(stored_adr, nbytes);

        if (unlikely(!new_adr)) {
                /* In this unlikely case, reallocation failed. */
                error_print(NG5_ERR_REALLOCERR);
        } else {
                /* In this case, reallocation was successful. The 'data pointer' and the memory pool is
                 * updated, and some statistics are done. */
                data_ptr_update(&ptr, new_adr);
                data_ptr_update(&info->ptr, new_adr);
                info->bytes_used = nbytes;
                info->bytes_total = nbytes;
                self->counters.num_realloc_calls++;
        }

        /* Return the (potentially updated) 'data pointer' */
        return ptr;
}

/* Perform the actual 'free' function in this pool strategy by just calling clibs 'free' function. For this,
 * the memory address stored in the 'data pointer' is received, the memory is released by calling 'free' and
 * the 'data pointer' is deleted in the memory pool. */
static bool this_free(struct pool_strategy *self, data_ptr_t ptr)
{
        /* Check that 'self' is an "instance" of this pool strategy */
        REQUIRE_INSTANCE_OF_THIS()

        /* The memory address of clibs allocator */
        void *adr;

        /* Information stored in the memory pool that are related to the current 'data pointer', which must be
         * updated */
        struct pool_ptr_info *info;

        /* Get available information about the 'data pointer' from the memory pool, and extract the actual
         * memory address stored in the 'data pointer' */
        info = pool_internal_get_info(self, ptr);
        adr  = data_ptr_get_pointer(ptr);

        /* Free up the memory addressed by the default allocator, and delete the 'data pointer' from the memory pool */
        free (adr);
        pool_internal_delete(self, ptr);

        /* Perform some statistics */
        self->counters.num_free_calls++;
        self->counters.num_bytes_freed += info->bytes_total;

        return true;
}

/* Not used by this implementation. The intention of this function is to manually trigger freeing up memory that
 * reserved a pool strategy but not used by the called (e.g., if 'this_free' would not actually delete memory
 * blocks but store this memory blocks in a freelist-like data structure). */
static bool this_gc(struct pool_strategy *self)
{
        /* Check that 'self' is an "instance" of this pool strategy */
        REQUIRE_INSTANCE_OF_THIS()

        /* A macro to suppress the compiler warning that the parameter 'self' is not used */
        ng5_unused(self);

        /* nothing to do */

        return true;
}

/* This function is needed to perform some implementation-specific statistics, such as additional memory
 * consumption for book-keeping functionality, postpones calls to 'free' and such operations. See
 * 'struct pool_counters' for more information. */
static bool this_update_counters(struct pool_strategy *self)
{
        /* Check that 'self' is an "instance" of this pool strategy */
        REQUIRE_INSTANCE_OF_THIS()

        /* no additional book-keeping */
        self->counters.impl_mem_footprint = 0;

        return true;
}

/* The intention of this function is to reset the statistic counters. In this implementation, it is just setting
 * all members to zero, but implementation-dependent some members may not be allowed to be set to zero (such as
 * the memory footprint for booking) */
static bool this_reset_counters(struct pool_strategy *self)
{
        ng5_zero_memory(&self->counters, sizeof(struct pool_counters));
        return true;
}