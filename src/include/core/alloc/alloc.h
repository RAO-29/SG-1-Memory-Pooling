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

#ifndef NG5_ALLOC_H
#define NG5_ALLOC_H

#include "shared/common.h"
#include "shared/error.h"

NG5_BEGIN_DECL

/**
 * Allocates <code>num</code> elements of size <code>sizeof(type)</code> using the allocator <code>alloc</code> and
 * creates a new stack variable <code>type *name</code>.
 */
#define ng5_malloc(type, name, num, alloc)                                                                          \
    type *name = alloc_malloc(alloc, num *sizeof(type))

/**
 * Invokes a free operation in <code>alloc</code> allocator to free up memory assigned to pointer <code>name</code>
 */
#define ng5_free(name, alloc)                                                                                       \
    alloc_free(alloc, name)

struct allocator {
        /**
         *  Implementation-specific data (private fields etc.)
         *  This pointer may point to NULL.
         */
        void *extra;

        /**
         *  Error information
         */
        struct err err;

        /**
         *  Implementation to call memory allocation.
         */
        void *(*malloc)(struct allocator *self, size_t size);

        /**
         *  Implementation to call memory re-allocation.
         */
        void *(*realloc)(struct allocator *self, void *ptr, size_t size);

        /**
         *  Implementation to call freeing up memory.
         *  Depending on the strategy, freeing up memory might be lazy.
         */
        void (*free)(struct allocator *self, void *ptr);

        /**
         *  Perform a deep copy of this allocator including implementation-specific data stored in 'extra'
         *
         * @param dst non-null target in which 'self' should be cloned
         * @param self non-null source which should be clones in 'dst'
         */
        void (*clone)(struct allocator *dst, const struct allocator *self);
};

/**
 * Returns standard c-lib allocator (malloc, realloc, free)
 *
 * @param alloc must be non-null
 * @return STATUS_OK in case of non-null parameter alloc, STATUS_NULLPTR otherwise
 */
NG5_EXPORT (bool) alloc_create_std(struct allocator *alloc);

/**
 * Creates a new allocator 'dst' with default constructor (in case of 'this' is null), or as copy of
 * 'this' (in case 'this' is non-null)
 * @param dst non-null destination in which the allocator should be stored
 * @param self possibly null-pointer to an allocator implementation
 * @return a value unequal to STATUS_OK in case the operation is not successful
 */
NG5_EXPORT (bool) alloc_this_or_std(struct allocator *dst, const struct allocator *self);

/**
 * Performs a deep copy of the allocator 'src' into the allocator 'dst'.
 *
 * @param dst non-null pointer to allocator implementation (of same implementation as src)
 * @param src non-null pointer to allocator implementation (of same implementation as dst)
 * @return STATUS_OK in case of success, otherwise a value unequal to STATUS_OK describing the error
 */
NG5_EXPORT (bool) alloc_clone(struct allocator *dst, const struct allocator *src);

/**
 * Invokes memory allocation of 'size' bytes using the allocator 'alloc'.
 *
 * If allocation fails, the system may panic.
 *
 * @param alloc non-null pointer to allocator implementation
 * @param size number of bytes requested
 * @return non-null pointer to memory allocated with 'alloc'
 */
NG5_EXPORT (void *) alloc_malloc(struct allocator *alloc, size_t size);

/**
 * Invokes memory re-allocation for pointer 'ptr' (that is managed by 'alloc') to size 'size' in bytes.
 *
 * @param alloc non-null pointer to allocator implementation
 * @param ptr non-null pointer manged by 'alloc'
 * @param size new number of bytes for 'ptr'
 * @return non-null pointer that points to reallocated memory for 'ptr'
 */
NG5_EXPORT (void *) alloc_realloc(struct allocator *alloc, void *ptr, size_t size);

/**
 * Invokes memory freeing for pointer 'ptr' (that is managed by 'alloc').
 * Depending on the implementation, this operation might be lazy.
 *
 * @param alloc non-null pointer to allocator implementation
 * @param ptr non-null pointer manged by 'alloc'
 * @return STATUS_OK if success, STATUS_NULLPTR if <code>alloc</code> or <code>ptr</ptr> is <b>NULL</b>
 */
NG5_EXPORT (bool) alloc_free(struct allocator *alloc, void *ptr);

NG5_END_DECL

#endif
