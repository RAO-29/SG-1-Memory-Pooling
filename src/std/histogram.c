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

#include <math.h>
#include "std/histogram.h"

NG5_EXPORT(bool) histogram_builder_create(struct histogram_builder *builder)
{
        error_if_null(builder);
        error_init(&builder->err);
        vec_create(&builder->values, NULL, sizeof(u32), 100);
        builder->min = (u32)(-1);
        builder->max = 0;
        return true;
}

NG5_EXPORT(bool) histogram_builder_drop(struct histogram_builder *builder)
{
        error_if_null(builder);
        bool status = vec_drop(&builder->values);
        error_print_if(!status, NG5_ERR_DROPFAILED);
        return status;
}

NG5_EXPORT(bool) histogram_builder_add(struct histogram_builder *builder, u32 value)
{
        error_if_null(builder);
        vec_push(&builder->values, &value, 1);
        builder->min = ng5_min(builder->min, value);
        builder->max = ng5_max(builder->max, value);
        return true;
}

NG5_EXPORT(bool) histogram_builder_build(struct histogram *hist, struct histogram_builder *builder, u32 nbuckets)
{
        error_if_null(hist);
        error_if_null(builder);
        error_if(nbuckets == 0, &builder->err, NG5_ERR_ILLEGALARG);
        nbuckets = ng5_min(nbuckets, builder->values.num_elems);
        u32 span = ng5_span(builder->min, builder->max);
        error_init(&hist->err);
        hist->bucket_width = ng5_max(1, span / (float) nbuckets);
        hist->nbuckets = nbuckets;
        bool status = hashtable_create(&hist->table, &builder->err, sizeof(u32), sizeof(u32), 100);
        if (unlikely(!status)) {
                error(&builder->err, NG5_ERR_INITFAILED);
                return false;
        } else {
                const u32 *values = vec_all(&builder->values, u32);
                u32 nvalues = builder->values.num_elems;
                while (nvalues--) {
                        u32 value = *(values++);
                        u32 bucket_idx = value / hist->bucket_width;
                        bucket_idx = bucket_idx < hist->nbuckets ? bucket_idx : hist->nbuckets - 1;
                        u32 default_val = 0;
                        u32 amount = (*(const u32 *) hashtable_get_value_or_default(&hist->table, &bucket_idx,
                                &default_val)) + 1;
                        hashtable_insert_or_update(&hist->table, &bucket_idx, &amount, 1);
                }
                return true;
        }
}

NG5_EXPORT(bool) histogram_builder_print(FILE *file, struct histogram_builder *builder)
{
        error_if_null(file);
        error_if_null(builder);
        fprintf(file, "{ \"type\": \"histogram-builder\", \"min\": %" PRIu32 ", \"max\": %" PRIu32 ", \"values\": [",
                builder->min, builder->max);
        for (u32 i = 0; i < builder->values.num_elems; i++) {
                u32 n = *vec_get(&builder->values, i, u32);
                fprintf(file, "%" PRIu32 "%s", n, i + 1 < builder->values.num_elems ? ", " : "");
        }
        fprintf(file, "]}");
        fflush(file);
        return true;
}

NG5_EXPORT(bool) histogram_get_num_buckets(u32 *n, struct histogram *hist)
{
        error_if_null(n);
        error_if_null(hist);
        *n = hist->nbuckets;
        return true;
}

NG5_EXPORT(bool) histogram_get_num_bucket_value(u32 *value, u32 bucket_idx, struct histogram *hist, u32 default_value)
{
        error_if_null(value);
        error_if_null(hist);
        if (unlikely(bucket_idx >= hist->nbuckets)) {
                *value = default_value;
        } else {
                *value = (*(const u32 *) hashtable_get_value_or_default(&hist->table, &bucket_idx,
                                                                       &default_value));
        }
        return true;
}

NG5_EXPORT(bool) histogram_get_bucket_width(float *width, struct histogram *hist)
{
        error_if_null(width);
        error_if_null(hist);
        *width = hist->bucket_width;
        return true;
}

NG5_EXPORT(bool) histogram_drop(struct histogram *hist)
{
        error_if_null(hist);
        return hashtable_drop(&hist->table);
}

NG5_EXPORT(bool) histogram_print(FILE *file, struct histogram *hist)
{
        error_if_null(file);
        error_if_null(hist);
        fprintf(file, "{ \"type\": \"histogram\", \"bucket-width\": %0.2f, \"values\": [",
                hist->bucket_width);
        for (u32 bucket_idx = 0; bucket_idx < hist->nbuckets; bucket_idx++) {
                u32 amount = *(const u32 *) hashtable_get_value_or_default(&hist->table, &bucket_idx, 0);
                fprintf(file, "{\"[%0.2f, %0.2f)\": %" PRIu32 "}%s", bucket_idx * hist->bucket_width,
                        (bucket_idx + 1) * hist->bucket_width, amount, bucket_idx + 1 < hist->nbuckets ? ", " : "");
        }
        fprintf(file, "]}");
        fflush(file);
        return true;
}

