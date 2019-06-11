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

#include "std/sort.h"

NG5_EXPORT(bool) sort_qsort_indicies(size_t *indices, const void *base, size_t width, less_eq_func_t comp,
        size_t nelemns, struct allocator *alloc)
{
        error_if_null(base);
        error_if_null(alloc);

        if (nelemns <= 1) {
                return true;
        }

        i64 h = nelemns - 1;
        i64 *stack = (i64 *) alloc_malloc(alloc, (h + 1) * sizeof(i64));
        i64 top = -1;
        i64 l = 0;

        stack[++top] = l;
        stack[++top] = h;

        while (top >= 0) {
                h = stack[top--];
                l = stack[top--];

                i64 p = NG5_QSORT_INDICIES_PARTITION(indices, base, width, comp, l, h);

                if (p - 1 > l) {
                        stack[++top] = l;
                        stack[++top] = p - 1;
                }

                if (p + 1 < h) {
                        stack[++top] = p + 1;
                        stack[++top] = h;
                }
        }

        ng5_check_success(alloc_free(alloc, stack));
        return true;
}

NG5_EXPORT(int) sort_qsort_indicies_wargs(size_t *indices, const void *base, size_t width, less_eq_wargs_func_t comp,
        size_t nelemens, struct allocator *alloc, void *args)
{
        error_if_null(base);
        error_if_null(alloc);

        if (nelemens <= 1) {
                return true;
        }

        i64 h = nelemens - 1;
        i64 *stack = alloc_malloc(alloc, (h + 1) * sizeof(i64));
        assert (stack);

        i64 top = -1;
        i64 l = 0;

        stack[++top] = l;
        stack[++top] = h;

        while (top >= 0) {
                h = stack[top--];
                l = stack[top--];

                i64 p = NG5_QSORT_INDICIES_PARTITION_WARGS(indices, base, width, comp, l, h, args);

                if (p - 1 > l) {
                        stack[++top] = l;
                        stack[++top] = p - 1;
                }

                if (p + 1 < h) {
                        stack[++top] = p + 1;
                        stack[++top] = h;
                }
        }

        ng5_check_success(alloc_free(alloc, stack));
        return true;
}

NG5_EXPORT(size_t) sort_bsearch_indicies(const size_t *indicies, const void *base, size_t width, size_t nelemens,
        const void *neelde, eq_func_t compEq, less_func_t compLess)
{
        size_t l = 0;
        size_t r = nelemens - 1;
        while (l <= r && r < SIZE_MAX) {
                size_t m = l + (r - l) / 2;

                // Check if x is present at mid
                if (compEq(base + indicies[m] * width, neelde)) {
                        return m;
                }

                // If x greater, ignore left half
                if (compLess(base + indicies[m] * width, neelde)) {
                        l = m + 1;

                        // If x is smaller, ignore right half
                } else {
                        r = m - 1;
                }
        }

        // if we reach here, then element was
        // not present
        return nelemens;
}

NG5_EXPORT(size_t) sort_get_min(const size_t *elements, size_t nelemens)
{
        size_t min = (size_t) -1;
        while (nelemens--) {
                min = min < *elements ? min : *elements;
                elements++;
        }
        return min;
}

NG5_EXPORT(size_t) sort_get_max(const size_t *elements, size_t nelemens)
{
        size_t max = 0;
        while (nelemens--) {
                max = max > *elements ? max : *elements;
                elements++;
        }
        return max;
}

NG5_EXPORT(double) sort_get_sum(const size_t *elements, size_t nelemens)
{
        double sum = 0;
        while (nelemens--) {
                sum += *elements;
                elements++;
        }
        return sum;
}

NG5_EXPORT(double) sort_get_avg(const size_t *elements, size_t nelemens)
{
        return sort_get_sum(elements, nelemens) / (double) nelemens;
}