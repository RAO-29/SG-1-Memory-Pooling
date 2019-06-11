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

#ifndef NG5_BITMAP_H
#define NG5_BITMAP_H

#include "shared/common.h"
#include "std/vec.h"

NG5_BEGIN_DECL

struct bitmap {
        struct vector ofType(u64) data;
        u16 num_bits;
};

NG5_EXPORT(bool) bitmap_create(struct bitmap *bitmap, u16 num_bits);

NG5_EXPORT(bool) bitmap_cpy(struct bitmap *dst, const struct bitmap *src);

NG5_EXPORT(bool) bitmap_drop(struct bitmap *map);

NG5_EXPORT(size_t) bitmap_nbits(const struct bitmap *map);

NG5_EXPORT(bool) bitmap_clear(struct bitmap *map);

NG5_EXPORT(bool) bitmap_set(struct bitmap *map, u16 bit_position, bool on);

NG5_EXPORT(bool) bitmap_get(struct bitmap *map, u16 bit_position);

NG5_EXPORT(bool) bitmap_lshift(struct bitmap *map);

NG5_EXPORT(bool) bitmap_print(FILE *file, const struct bitmap *map);

NG5_EXPORT(bool) bitmap_blocks(u32 **blocks, u32 *num_blocks, const struct bitmap *map);

NG5_EXPORT(void) bitmap_print_bits(FILE *file, u32 n);

NG5_EXPORT(void) bitmap_print_bits_in_char(FILE *file, char n);

NG5_END_DECL

#endif