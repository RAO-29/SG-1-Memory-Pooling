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

#ifndef NG5_SORT_H
#define NG5_SORT_H

#include "shared/common.h"
#include "core/alloc/alloc.h"

#include "stdlib.h"
#include <assert.h>

NG5_BEGIN_DECL

typedef bool (*less_eq_func_t)(const void *lhs, const void *rhs);

typedef bool (*less_eq_wargs_func_t)(const void *lhs, const void *rhs, void *args);

typedef bool (*eq_func_t)(const void *lhs, const void *rhs);

typedef bool (*less_func_t)(const void *lhs, const void *rhs);

#define NG5_QSORT_INDICES_SWAP(x, y)                                                                                   \
{                                                                                                                      \
    size_t *a = x;                                                                                                     \
    size_t *b = y;                                                                                                     \
    size_t tmp = *a;                                                                                                   \
    *a = *b;                                                                                                           \
    *b = tmp;                                                                                                          \
}

#define NG5_QSORT_INDICIES_PARTITION(indices, base, width, comp, l, h)                                                 \
({                                                                                                                     \
    const void   *x       = base + indices[h] * width;                                                                 \
    i64        i       = (l - 1);                                                                                      \
                                                                                                                       \
    for (i64 j = l; j <= h - 1; j++)                                                                                   \
    {                                                                                                                  \
        if (comp(base + indices[j] * width, x))                                                                        \
        {                                                                                                              \
            i++;                                                                                                       \
            NG5_QSORT_INDICES_SWAP (indices + i, indices + j);                                                         \
        }                                                                                                              \
    }                                                                                                                  \
    NG5_QSORT_INDICES_SWAP (indices + (i + 1), indices + h);                                                           \
    (i + 1);                                                                                                           \
})

#define NG5_QSORT_INDICIES_PARTITION_WARGS(indices, base, width, comp, l, h, args)                                     \
({                                                                                                                     \
    const void   *x       = base + indices[h] * width;                                                                 \
    i64        i       = (l - 1);                                                                                      \
                                                                                                                       \
    for (i64 j = l; j <= h - 1; j++)                                                                                   \
    {                                                                                                                  \
        if (comp(base + indices[j] * width, x, args))                                                                  \
        {                                                                                                              \
            i++;                                                                                                       \
            NG5_QSORT_INDICES_SWAP (indices + i, indices + j);                                                         \
        }                                                                                                              \
    }                                                                                                                  \
    NG5_QSORT_INDICES_SWAP (indices + (i + 1), indices + h);                                                           \
    (i + 1);                                                                                                           \
})

NG5_EXPORT(bool) sort_qsort_indicies(size_t *indices, const void *base, size_t width, less_eq_func_t comp,
        size_t nelemns, struct allocator *alloc);

NG5_EXPORT(int) sort_qsort_indicies_wargs(size_t *indices, const void *base, size_t width, less_eq_wargs_func_t comp,
        size_t nelemens, struct allocator *alloc, void *args);

NG5_EXPORT(size_t) sort_bsearch_indicies(const size_t *indicies, const void *base, size_t width, size_t nelemens,
        const void *neelde, eq_func_t compEq, less_func_t compLess);

NG5_EXPORT(size_t) sort_get_min(const size_t *elements, size_t nelemens);

NG5_EXPORT(size_t) sort_get_max(const size_t *elements, size_t nelemens);

NG5_EXPORT(double) sort_get_sum(const size_t *elements, size_t nelemens);

NG5_EXPORT(double) sort_get_avg(const size_t *elements, size_t nelemens);

NG5_END_DECL

#endif
