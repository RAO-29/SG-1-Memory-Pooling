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

#include "hash/bern.h"
#include "std/hash_set.h"

#define HASHCODE_OF(size, x) NG5_HASH_BERNSTEIN(size, x)
#define FIX_MAP_AUTO_REHASH_LOADFACTOR 0.9f

NG5_EXPORT(bool) hashset_create(struct hashset *map, struct err *err, size_t key_size, size_t capacity)
{
        error_if_null(map)
        error_if_null(key_size)

        int err_code = NG5_ERR_INITFAILED;

        map->size = 0;

        ng5_success_or_jump(vec_create(&map->key_data, NULL, key_size, capacity), error_handling);
        ng5_success_or_jump(vec_create(&map->table, NULL, sizeof(struct hashset_bucket), capacity),
                cleanup_key_data_and_error);
        ng5_success_or_jump(vec_enlarge_size_to_capacity(&map->table), cleanup_key_value_table_and_error);
        ng5_success_or_jump(vec_zero_memory(&map->table), cleanup_key_value_table_and_error);
        ng5_success_or_jump(spin_init(&map->lock), cleanup_key_value_table_and_error);
        ng5_success_or_jump(error_init(&map->err), cleanup_key_value_table_and_error);

        return true;

        cleanup_key_value_table_and_error:
        if (!vec_drop(&map->table)) {
                err_code = NG5_ERR_DROPFAILED;
        }
        cleanup_key_data_and_error:
        if (!vec_drop(&map->key_data)) {
                err_code = NG5_ERR_DROPFAILED;
        }
        error_handling:
        error(err, err_code);
        return false;
}

NG5_EXPORT(bool) hashset_drop(struct hashset *map)
{
        error_if_null(map)

        bool status = true;

        status &= vec_drop(&map->table);
        status &= vec_drop(&map->key_data);

        if (!status) {
                error(&map->err, NG5_ERR_DROPFAILED);
        }

        return status;
}

NG5_EXPORT(struct vector *)hashset_keys(struct hashset *map)
{
        if (map) {
                struct vector *result = malloc(sizeof(struct vector));
                vec_create(result, NULL, map->key_data.elem_size, map->key_data.num_elems);
                for (u32 i = 0; i < map->table.num_elems; i++) {
                        struct hashset_bucket *bucket = vec_get(&map->table, i, struct hashset_bucket);
                        if (bucket->in_use_flag) {
                                const void *data = vec_at(&map->key_data, bucket->key_idx);
                                vec_push(result, data, 1);
                        }
                }
                return result;
        } else {
                return NULL;
        }
}

NG5_EXPORT(struct hashset *)hashset_cpy(struct hashset *src)
{
        if (src) {
                struct hashset *cpy = malloc(sizeof(struct hashset));

                hashset_lock(src);

                hashset_create(cpy, &src->err, src->key_data.elem_size, src->table.cap_elems);

                assert(src->key_data.cap_elems == src->table.cap_elems);
                assert(src->key_data.num_elems <= src->table.num_elems);

                vec_cpy_to(&cpy->key_data, &src->key_data);
                vec_cpy_to(&cpy->table, &src->table);
                cpy->size = src->size;
                error_cpy(&cpy->err, &src->err);

                assert(cpy->key_data.cap_elems == cpy->table.cap_elems);
                assert(cpy->key_data.num_elems <= cpy->table.num_elems);

                hashset_unlock(src);
                return cpy;
        } else {
                error(&src->err, NG5_ERR_NULLPTR);
                return NULL;
        }
}

NG5_EXPORT(bool) hashset_clear(struct hashset *map)
{
        error_if_null(map)
        assert(map->key_data.cap_elems == map->table.cap_elems);
        assert(map->key_data.num_elems <= map->table.num_elems);

        hashset_lock(map);

        bool status = vec_clear(&map->key_data) && vec_zero_memory(&map->table);

        map->size = 0;

        assert(map->key_data.cap_elems == map->table.cap_elems);
        assert(map->key_data.num_elems <= map->table.num_elems);

        if (!status) {
                error(&map->err, NG5_ERR_OPPFAILED);
        }

        hashset_unlock(map);

        return status;
}

NG5_EXPORT(bool) hashset_avg_displace(float *displace, const struct hashset *map)
{
        error_if_null(displace);
        error_if_null(map);

        size_t sum_dis = 0;
        for (size_t i = 0; i < map->table.num_elems; i++) {
                struct hashset_bucket *bucket = vec_get(&map->table, i, struct hashset_bucket);
                sum_dis += abs(bucket->displacement);
        }
        *displace = (sum_dis / (float) map->table.num_elems);

        return true;
}

NG5_EXPORT(bool) hashset_lock(struct hashset *map)
{
        error_if_null(map)
        spin_acquire(&map->lock);
        return true;
}

NG5_EXPORT(bool) hashset_unlock(struct hashset *map)
{
        error_if_null(map)
        spin_release(&map->lock);
        return true;
}

static inline const void *get_bucket_key(const struct hashset_bucket *bucket, const struct hashset *map)
{
        return map->key_data.base + bucket->key_idx * map->key_data.elem_size;
}

static void insert(struct hashset_bucket *bucket, struct hashset *map, const void *key, i32 displacement)
{
        u64 idx = map->key_data.num_elems;
        void *key_datum = vec_new_and_get(&map->key_data, void *);
        memcpy(key_datum, key, map->key_data.elem_size);
        bucket->key_idx = idx;
        bucket->in_use_flag = true;
        bucket->displacement = displacement;
        map->size++;
}

static inline uint_fast32_t insert_or_update(struct hashset *map, const u32 *bucket_idxs, const void *keys,
        uint_fast32_t num_pairs)
{
        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                u32 intended_bucket_idx = bucket_idxs[i];

                u32 bucket_idx = intended_bucket_idx;

                struct hashset_bucket *bucket = vec_get(&map->table, bucket_idx, struct hashset_bucket);
                if (bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) != 0) {
                        bool fitting_bucket_found = false;
                        u32 displace_idx;
                        for (displace_idx = bucket_idx + 1; displace_idx < map->table.num_elems; displace_idx++) {
                                struct hashset_bucket
                                        *bucket = vec_get(&map->table, displace_idx, struct hashset_bucket);
                                fitting_bucket_found = !bucket->in_use_flag || (bucket->in_use_flag
                                        && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0);
                                if (fitting_bucket_found) {
                                        break;
                                } else {
                                        i32 displacement = displace_idx - bucket_idx;
                                        const void *swap_key = get_bucket_key(bucket, map);

                                        if (bucket->displacement < displacement) {
                                                insert(bucket, map, key, displacement);
                                                insert_or_update(map, &displace_idx, swap_key, 1);
                                                goto next_round;
                                        }
                                }
                        }
                        if (!fitting_bucket_found) {
                                for (displace_idx = 0; displace_idx < bucket_idx - 1; displace_idx++) {
                                        const struct hashset_bucket
                                                *bucket = vec_get(&map->table, displace_idx, struct hashset_bucket);
                                        fitting_bucket_found = !bucket->in_use_flag || (bucket->in_use_flag
                                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size)
                                                        == 0);
                                        if (fitting_bucket_found) {
                                                break;
                                        }
                                }
                        }

                        assert(fitting_bucket_found == true);
                        bucket_idx = displace_idx;
                        bucket = vec_get(&map->table, bucket_idx, struct hashset_bucket);
                }

                bool is_update =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                if (!is_update) {
                        i32 displacement = intended_bucket_idx - bucket_idx;
                        insert(bucket, map, key, displacement);
                }

                next_round:
                if (map->size >= FIX_MAP_AUTO_REHASH_LOADFACTOR * map->table.cap_elems) {
                        return i + 1; /* tell the caller that pair i was inserted, but it successors not */
                }

        }
        return 0;
}

NG5_EXPORT(bool) hashset_insert_or_update(struct hashset *map, const void *keys, uint_fast32_t num_pairs)
{
        error_if_null(map)
        error_if_null(keys)

        assert(map->key_data.cap_elems == map->table.cap_elems);
        assert(map->key_data.num_elems <= map->table.num_elems);

        hashset_lock(map);

        u32 *bucket_idxs = malloc(num_pairs * sizeof(u32));
        if (!bucket_idxs) {
                error(&map->err, NG5_ERR_MALLOCERR);
                return false;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                hash32_t hash = HASHCODE_OF(map->key_data.elem_size, key);
                bucket_idxs[i] = hash % map->table.num_elems;
        }

        uint_fast32_t cont_idx = 0;
        do {
                cont_idx = insert_or_update(map,
                        bucket_idxs + cont_idx,
                        keys + cont_idx * map->key_data.elem_size,
                        num_pairs - cont_idx);
                if (cont_idx != 0) {
                        /* rehashing is required, and [status, num_pairs) are left to be inserted */
                        if (!hashset_rehash(map)) {
                                hashset_unlock(map);
                                return false;
                        }
                }
        }
        while (cont_idx != 0);

        free(bucket_idxs);
        hashset_unlock(map);

        return true;
}

NG5_EXPORT(bool) hashset_remove_if_contained(struct hashset *map, const void *keys, size_t num_pairs)
{
        error_if_null(map)
        error_if_null(keys)

        hashset_lock(map);

        u32 *bucket_idxs = malloc(num_pairs * sizeof(u32));
        if (!bucket_idxs) {
                error(&map->err, NG5_ERR_MALLOCERR);
                hashset_unlock(map);
                return false;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                bucket_idxs[i] = HASHCODE_OF(map->key_data.elem_size, key) % map->table.num_elems;
        }

        for (uint_fast32_t i = 0; i < num_pairs; i++) {
                const void *key = keys + i * map->key_data.elem_size;
                u32 bucket_idx = bucket_idxs[i];
                u32 actual_idx = bucket_idx;
                bool bucket_found = false;

                for (u32 k = bucket_idx; !bucket_found && k < map->table.num_elems; k++) {
                        const struct hashset_bucket *bucket = vec_get(&map->table, k, struct hashset_bucket);
                        bucket_found = bucket->in_use_flag
                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                        actual_idx = k;
                }
                for (u32 k = 0; !bucket_found && k < bucket_idx; k++) {
                        const struct hashset_bucket *bucket = vec_get(&map->table, k, struct hashset_bucket);
                        bucket_found = bucket->in_use_flag
                                && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
                        actual_idx = k;
                }

                if (bucket_found) {
                        struct hashset_bucket *bucket = vec_get(&map->table, actual_idx, struct hashset_bucket);
                        bucket->in_use_flag = false;
                        bucket->key_idx = 0;
                }
        }

        free(bucket_idxs);

        hashset_unlock(map);

        return true;
}

NG5_EXPORT(bool) hashset_contains_key(struct hashset *map, const void *key)
{
        error_if_null(map)
        error_if_null(key)

        bool result = false;

        hashset_lock(map);

        u32 bucket_idx = HASHCODE_OF(map->key_data.elem_size, key) % map->table.num_elems;
        bool bucket_found = false;

        for (u32 k = bucket_idx; !bucket_found && k < map->table.num_elems; k++) {
                const struct hashset_bucket *bucket = vec_get(&map->table, k, struct hashset_bucket);
                bucket_found =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
        }
        for (u32 k = 0; !bucket_found && k < bucket_idx; k++) {
                const struct hashset_bucket *bucket = vec_get(&map->table, k, struct hashset_bucket);
                bucket_found =
                        bucket->in_use_flag && memcmp(get_bucket_key(bucket, map), key, map->key_data.elem_size) == 0;
        }

        result = bucket_found;
        hashset_unlock(map);

        return result;
}

NG5_EXPORT(bool) hashset_get_fload_factor(float *factor, struct hashset *map)
{
        error_if_null(factor)
        error_if_null(map)

        hashset_lock(map);

        *factor = map->size / (float) map->table.num_elems;

        hashset_unlock(map);

        return true;
}

NG5_EXPORT(bool) hashset_rehash(struct hashset *map)
{
        error_if_null(map)

        hashset_lock(map);

        struct hashset *cpy = hashset_cpy(map);
        hashset_clear(map);

        size_t new_cap = (cpy->key_data.cap_elems + 1) * 1.7f;

        vec_grow_to(&map->key_data, new_cap);
        vec_grow_to(&map->table, new_cap);
        vec_enlarge_size_to_capacity(&map->table);
        vec_zero_memory(&map->table);

        assert(map->key_data.cap_elems == map->table.cap_elems);
        assert(map->key_data.num_elems <= map->table.num_elems);

        for (size_t i = 0; i < cpy->table.num_elems; i++) {
                struct hashset_bucket *bucket = vec_get(&cpy->table, i, struct hashset_bucket);
                if (bucket->in_use_flag) {
                        const void *old_key = get_bucket_key(bucket, cpy);
                        if (!hashset_insert_or_update(map, old_key, 1)) {
                                error(&map->err, NG5_ERR_REHASH_NOROLLBACK)
                                hashset_unlock(map);
                                return false;
                        }
                }
        }

        hashset_unlock(map);
        return true;
}