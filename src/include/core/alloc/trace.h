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

#ifndef NG5_ALLOC_TRACER_H
#define NG5_ALLOC_TRACER_H

#include "alloc.h"

NG5_BEGIN_DECL

/**
 * Returns standard c-lib allocator (malloc, realloc, free) that collects some statistics
 * for inspection purposes. Generally, this implementation is slow and should not be used
 * in productive mode
 *
 * @param alloc must be non-null
 * @return STATUS_OK in case of non-null parameter alloc, STATUS_NULLPTR otherwise
 */
NG5_EXPORT (int) trace_alloc_create(struct allocator *alloc);

NG5_END_DECL

#endif
