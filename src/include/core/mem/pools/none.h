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

/* Inclusion guard; required to avoid multiple inclusions of the same header */
#ifndef NG5_POOL_NONE_H
#define NG5_POOL_NONE_H

/**
 * This implementation of a memory pool is actually a template, since it does not manage memory by itself
 * but delegates allocation, reallocation, and free to the standard clib function. In addition, is properly calls
 * internal memory pool functions to register resp. unregister pointers managed by the pool.
 */

#include "shared/common.h"

/* A macro that allows to call this C code from C++ */
NG5_BEGIN_DECL

/* A constant to provide an unique name for the implementation */
#define POOL_STRATEGY_NONE_NAME "mempool/none"

/* Forwarded struct tag from "core/mem/pool.h"; that's just how it must be done to avoid cyclic inclusions of headers */
struct pool_strategy;

/* The constructor function that bind implementation-specific functionallity to "the interface" of a pool strategy */
void pool_strategy_none_create(struct pool_strategy *dst);

/* End of macro that allows to call this C code from C++ */
NG5_END_DECL

/* End of inclusion guard; required to avoid multiple inclusions of the same header */
#endif
