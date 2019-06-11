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

#ifndef NG5_STRING_PRED_H
#define NG5_STRING_PRED_H

#include "shared/common.h"

NG5_BEGIN_DECL

typedef bool
(*string_pred_func_t)(size_t *idxs_matching, size_t *num_matching, char **strings, size_t num_strings, void *capture);

struct string_pred_t {
        string_pred_func_t func;
        i64 limit;
};

NG5_BUILT_IN(static bool) string_pred_validate(struct err *err, const struct string_pred_t *pred)
{
        error_if_null(pred);
        ng5_implemented_or_error(err, pred, func)
        return true;
}

NG5_BUILT_IN(static bool) string_pred_eval(const struct string_pred_t *pred, size_t *idxs_matching,
        size_t *num_matching, char **strings, size_t num_strings, void *capture)
{
        assert(pred);
        assert(idxs_matching);
        assert(num_matching);
        assert(strings);
        assert(pred->func);
        return pred->func(idxs_matching, num_matching, strings, num_strings, capture);
}

NG5_BUILT_IN(static bool) string_pred_get_limit(i64 *limit, const struct string_pred_t *pred)
{
        error_if_null(limit);
        error_if_null(pred);
        *limit = pred->limit;
        return true;
}

NG5_END_DECL

#endif
