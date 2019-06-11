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

#include <math.h>
#include "std/bitmap.h"

NG5_EXPORT(bool) bitmap_create(struct bitmap *bitmap, u16 num_bits)
{
        error_if_null(bitmap);

        struct allocator alloc;
        alloc_create_std(&alloc);
        vec_create(&bitmap->data, &alloc, sizeof(u32), ceil(num_bits / (double) ng5_bit_num_of(u32)));
        size_t cap = vec_capacity(&bitmap->data);
        u32 zero = 0;
        vec_repeated_push(&bitmap->data, &zero, cap);
        bitmap->num_bits = num_bits;

        return true;
}

NG5_EXPORT(bool) bitmap_cpy(struct bitmap *dst, const struct bitmap *src)
{
        dst->num_bits = src->num_bits;
        return vec_cpy(&dst->data, &src->data);
}

NG5_EXPORT(bool) bitmap_drop(struct bitmap *bitset)
{
        return vec_drop(&bitset->data);
}

size_t bitmap_nbits(const struct bitmap *bitset)
{
        error_if_null(bitset);
        return bitset->num_bits;
}

NG5_EXPORT(bool) bitmap_clear(struct bitmap *bitset)
{
        error_if_null(bitset);
        void *data = (void *) vec_data(&bitset->data);
        memset(data, 0, sizeof(u32) * vec_capacity(&bitset->data));
        return true;
}

NG5_EXPORT(bool) bitmap_set(struct bitmap *bitset, u16 bit_position, bool on)
{
        error_if_null(bitset)
        size_t block_pos = floor(bit_position / (double) ng5_bit_num_of(u32));
        size_t block_bit = bit_position % ng5_bit_num_of(u32);
        u32 block = *vec_get(&bitset->data, block_pos, u32);
        u32 mask = ng5_set_bit(block_bit);
        if (on) {
                ng5_set_bits(block, mask);
        } else {
                ng5_unset_bits(block, mask);
        }
        vec_set(&bitset->data, block_pos, &block);
        return true;
}

bool bitmap_get(struct bitmap *bitset, u16 bit_position)
{
        error_if_null(bitset)
        size_t block_pos = floor(bit_position / (double) ng5_bit_num_of(u32));
        size_t block_bit = bit_position % ng5_bit_num_of(u32);
        u32 block = *vec_get(&bitset->data, block_pos, u32);
        u32 mask = ng5_set_bit(block_bit);
        return ((mask & block) >> bit_position) == true;
}

NG5_EXPORT(bool) bitmap_lshift(struct bitmap *map)
{
        error_if_null(map)
        for (int i = map->num_bits - 1; i >= 0; i--) {
                bool f = i > 0 ? bitmap_get(map, i - 1) : false;
                bitmap_set(map, i, f);
        }
        return true;
}

void bitmap_print_bits(FILE *file, u32 n)
{
        for (int i = 31; i >= 0; i--) {
                u32 mask = 1 << i;
                u32 k = n & mask;
                fprintf(file, "%s", k == 0 ? "0" : "1");
        }
}

void bitmap_print_bits_in_char(FILE *file, char n)
{
        fprintf(file, "0b");
        for (int i = 7; i >= 0; i--) {
                char mask = 1 << i;
                char k = n & mask;
                fprintf(file, "%s", k == 0 ? "0" : "1");
        }
}

bool bitmap_blocks(u32 **blocks, u32 *num_blocks, const struct bitmap *map)
{
        error_if_null(blocks)
        error_if_null(num_blocks)
        error_if_null(map)

        u32 *result = malloc(map->data.num_elems * sizeof(u32));
        i32 k = 0;
        for (i32 i = map->data.num_elems - 1; i >= 0; i--) {
                result[k++] = *vec_get(&map->data, i, u32);
        }
        *blocks = result;
        *num_blocks = map->data.num_elems;

        return true;
}

bool bitmap_print(FILE *file, const struct bitmap *map)
{
        error_if_null(map)

        u32 *blocks, num_blocks;

        bitmap_blocks(&blocks, &num_blocks, map);

        for (u32 i = 0; i < num_blocks; i++) {
                fprintf(file, " %"PRIu32 " |", blocks[i]);
        }

        free(blocks);

        for (i32 i = map->data.num_elems - 1; i >= 0; i--) {
                u32 block = *vec_get(&map->data, i, u32);
                bitmap_print_bits(stdout, block);
                fprintf(file, " |");
        }

        fprintf(file, "\n");
        return true;
}