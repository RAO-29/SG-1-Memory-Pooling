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

#ifndef NG5_HISTOGRAM_H
#define NG5_HISTOGRAM_H

#include "shared/error.h"
#include "shared/common.h"
#include "std/vec.h"
#include "std/hash_table.h"

NG5_BEGIN_DECL

struct histogram_builder
{
        struct err err;
        struct vector ofType(u32) values;
        u32 min, max;
};

struct histogram
{
        struct err err;
        float bucket_width;
        u32 nbuckets;
        struct hashtable table;
};

NG5_DEFINE_GET_ERROR_FUNCTION(histogram_builder, struct histogram_builder, builder);
NG5_DEFINE_GET_ERROR_FUNCTION(histogram, struct histogram, hist);

NG5_EXPORT(bool) histogram_builder_create(struct histogram_builder *builder);
NG5_EXPORT(bool) histogram_builder_drop(struct histogram_builder *builder);
NG5_EXPORT(bool) histogram_builder_add(struct histogram_builder *builder, u32 value);
NG5_EXPORT(bool) histogram_builder_build(struct histogram *hist, struct histogram_builder *builder, u32 nbuckets);
NG5_EXPORT(bool) histogram_builder_print(FILE *file, struct histogram_builder *builder);
NG5_EXPORT(bool) histogram_get_num_buckets(u32 *n, struct histogram *hist);
NG5_EXPORT(bool) histogram_get_num_bucket_value(u32 *value, u32 bucket_idx, struct histogram *hist, u32 default_value);
NG5_EXPORT(bool) histogram_get_bucket_width(float *width, struct histogram *hist);
NG5_EXPORT(bool) histogram_drop(struct histogram *hist);
NG5_EXPORT(bool) histogram_print(FILE *file, struct histogram *hist);

NG5_END_DECL

#endif
