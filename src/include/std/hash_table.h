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

#ifndef NG5_FIX_MAP_H
#define NG5_FIX_MAP_H

#include "shared/common.h"
#include "std/vec.h"
#include "core/async/spin.h"

NG5_BEGIN_DECL

struct hashtable_bucket {
        bool in_use_flag;  /* flag indicating if bucket is in use */
        i32 displacement; /* difference between intended position during insert, and actual position in table */
        u32 num_probs;    /* number of probe calls to this bucket */
        u64 data_idx;      /* position of key element in owning struct hashtable structure */
};

/**
 * Hash table implementation specialized for key and value types of fixed-length size, and where comparision
 * for equals is byte-compare. With this, calling a (type-dependent) compare function becomes obsolete.
 *
 * Example: mapping of u64 to u32.
 *
 * This hash table is optimized to reduce access time to elements. Internally, a robin-hood hashing technique is used.
 *
 * Note: this implementation does not support string or pointer types. The structure is thread-safe by a spinlock
 * lock implementation.
 */
struct hashtable {
        struct vector key_data;
        struct vector value_data;
        struct vector ofType(struct hashtable_bucket) table;
        struct spinlock lock;
        u32 size;
        struct err err;
};

NG5_DEFINE_GET_ERROR_FUNCTION(hashtable, struct hashtable, table);

NG5_EXPORT(bool) hashtable_create(struct hashtable *map, struct err *err, size_t key_size, size_t value_size,
        size_t capacity);

NG5_EXPORT(struct hashtable *) hashtable_cpy(struct hashtable *src);

NG5_EXPORT(bool) hashtable_drop(struct hashtable *map);

NG5_EXPORT(bool) hashtable_clear(struct hashtable *map);

NG5_EXPORT(bool) hashtable_avg_displace(float *displace, const struct hashtable *map);

NG5_EXPORT(bool) hashtable_lock(struct hashtable *map);

NG5_EXPORT(bool) hashtable_unlock(struct hashtable *map);

NG5_EXPORT(bool) hashtable_insert_or_update(struct hashtable *map, const void *keys, const void *values,
        uint_fast32_t num_pairs);

NG5_EXPORT(bool) hashtable_serialize(FILE *file, struct hashtable *table);

NG5_EXPORT(bool) hashtable_deserialize(struct hashtable *table, struct err *err, FILE *file);

NG5_EXPORT(bool) hashtable_remove_if_contained(struct hashtable *map, const void *keys, size_t num_pairs);

NG5_EXPORT(const void *) hashtable_get_value(struct hashtable *map, const void *key);

NG5_EXPORT(const void *) hashtable_get_value_or_default(struct hashtable *map, const void *key, const void *value);

NG5_EXPORT(bool) hashtable_get_fload_factor(float *factor, struct hashtable *map);

NG5_EXPORT(bool) hashtable_rehash(struct hashtable *map);

NG5_END_DECL

#endif
