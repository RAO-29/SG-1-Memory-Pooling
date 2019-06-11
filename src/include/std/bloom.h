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

#ifndef NG5_BLOOM_H
#define NG5_BLOOM_H

#include "shared/common.h"
#include "bitmap.h"

NG5_BEGIN_DECL

typedef struct bitmap bloom_t;

#define NG5_BLOOM_SET(filter, key, keySize)                 \
({                                                          \
    size_t nbits = bitmap_nbits(filter);                    \
    size_t b0 = NG5_HASH_ADDITIVE(keySize, key) % nbits;    \
    size_t b1 = NG5_HASH_XOR(keySize, key) % nbits;         \
    size_t b2 = NG5_HASH_ROT(keySize, key) % nbits;         \
    size_t b3 = NG5_HASH_SAX(keySize, key) % nbits;         \
    bitmap_set(filter, b0, true);                           \
    bitmap_set(filter, b1, true);                           \
    bitmap_set(filter, b2, true);                           \
    bitmap_set(filter, b3, true);                           \
})

#define NG5_BLOOM_TEST(filter, key, keySize)                \
({                                                          \
    size_t nbits = bitmap_nbits(filter);                    \
    size_t b0 = NG5_HASH_ADDITIVE(keySize, key) % nbits;    \
    size_t b1 = NG5_HASH_XOR(keySize, key) % nbits;         \
    size_t b2 = NG5_HASH_ROT(keySize, key) % nbits;         \
    size_t b3 = NG5_HASH_SAX(keySize, key) % nbits;         \
    bool b0set = bitmap_get(filter, b0);                    \
    bool b1set = bitmap_get(filter, b1);                    \
    bool b2set = bitmap_get(filter, b2);                    \
    bool b3set = bitmap_get(filter, b3);                    \
    (b0set && b1set && b2set && b3set);                     \
})

#define NG5_BLOOM_TEST_AND_SET(filter, key, keySize)        \
({                                                          \
    size_t nbits = bitmap_nbits(filter);                    \
    size_t b0 = NG5_HASH_ADDITIVE(keySize, key) % nbits;    \
    size_t b1 = NG5_HASH_XOR(keySize, key) % nbits;         \
    size_t b2 = NG5_HASH_ROT(keySize, key) % nbits;         \
    size_t b3 = NG5_HASH_SAX(keySize, key) % nbits;         \
    bool b0set = bitmap_get(filter, b0);                    \
    bool b1set = bitmap_get(filter, b1);                    \
    bool b2set = bitmap_get(filter, b2);                    \
    bool b3set = bitmap_get(filter, b3);                    \
    bitmap_set(filter, b0, true);                           \
    bitmap_set(filter, b1, true);                           \
    bitmap_set(filter, b2, true);                           \
    bitmap_set(filter, b3, true);                           \
    (b0set && b1set && b2set && b3set);                     \
})

NG5_EXPORT(bool) bloom_create(bloom_t *filter, size_t size);

NG5_EXPORT(bool) bloom_drop(bloom_t *filter);

NG5_EXPORT(bool) bloom_clear(bloom_t *filter);

NG5_EXPORT(size_t) bloom_nbits(bloom_t *filter);

NG5_EXPORT(unsigned) bloom_nhashs();

NG5_END_DECL

#endif