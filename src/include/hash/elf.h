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

#ifndef NG5_ELF_H
#define NG5_ELF_H

#include "hash.h"
#include "../shared/common.h"

NG5_BEGIN_DECL

#define NG5_HASH_ELF(key_size, key)                                                                                    \
({                                                                                                                     \
    assert ((key != NULL) && (key_size > 0));                                                                          \
                                                                                                                       \
    hash32_t hash = 0, g;                                                                                              \
    for (size_t i = 0; i < key_size; i++) {                                                                            \
        hash = (hash << 4) + ((unsigned char* )key)[i];                                                                \
        if ((g = hash & 0xf0000000L) != 0) {                                                                           \
            hash ^= g >> 24;                                                                                           \
        }                                                                                                              \
        hash &= ~g;                                                                                                    \
    }                                                                                                                  \
    hash;                                                                                                              \
})

NG5_END_DECL

#endif