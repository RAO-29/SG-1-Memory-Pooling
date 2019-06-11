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

#ifndef NG5_HUFFMAN_H
#define NG5_HUFFMAN_H

#include "shared/common.h"
#include "std/vec.h"
#include "core/mem/file.h"
#include "shared/types.h"

NG5_BEGIN_DECL

struct coding_huffman {
        struct vector ofType(struct pack_huffman_entry) table;
        struct err err;
};

struct pack_huffman_entry {
        unsigned char letter;
        u32 *blocks;
        u16 nblocks;
};

struct pack_huffman_info {
        unsigned char letter;
        u8 nbytes_prefix;
        char *prefix_code;
};

struct pack_huffman_str_info {
        u32 nbytes_encoded;
        const char *encoded_bytes;
};

NG5_EXPORT(bool) coding_huffman_create(struct coding_huffman *dic);

NG5_EXPORT(bool) coding_huffman_cpy(struct coding_huffman *dst, struct coding_huffman *src);

NG5_EXPORT(bool) coding_huffman_build(struct coding_huffman *encoder, const string_vector_t *strings);

NG5_EXPORT(bool) coding_huffman_get_error(struct err *err, const struct coding_huffman *dic);

NG5_EXPORT(bool) coding_huffman_encode(struct memfile *file, struct coding_huffman *dic, const char *string);

NG5_EXPORT(bool) coding_huffman_read_string(struct pack_huffman_str_info *info, struct memfile *src);

NG5_EXPORT(bool) coding_huffman_drop(struct coding_huffman *dic);

NG5_EXPORT(bool) coding_huffman_serialize(struct memfile *file, const struct coding_huffman *dic, char marker_symbol);

NG5_EXPORT(bool) coding_huffman_read_entry(struct pack_huffman_info *info, struct memfile *file, char marker_symbol);

NG5_END_DECL

#endif
