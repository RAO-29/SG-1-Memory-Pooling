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

#ifndef NG5_JENKINS_H
#define NG5_JENKINS_H

#include "hash.h"
#include "../shared/common.h"

NG5_BEGIN_DECL

#define NG5_JENKINS_MIX(a, b, c)                                                                                       \
{                                                                                                                      \
    a -= b; a -= c; a ^= (c >> 13);                                                                                    \
    b -= c; b -= a; b ^= (a << 8);                                                                                     \
    c -= a; c -= b; c ^= (b >> 13);                                                                                    \
    a -= b; a -= c; a ^= (c >> 12);                                                                                    \
    b -= c; b -= a; b ^= (a << 16);                                                                                    \
    c -= a; c -= b; c ^= (b >> 5);                                                                                     \
    a -= b; a -= c; a ^= (c >> 3);                                                                                     \
    b -= c; b -= a; b ^= (a << 10);                                                                                    \
    c -= a; c -= b; c ^= (b >> 15);                                                                                    \
}

/** implements: hash32_t hash_jenkins(size_t key_size, const void *key) */
#define NG5_HASH_JENKINS(keySizeIn, key)                                                                               \
({                                                                                                                     \
    size_t key_size = keySizeIn;                                                                                       \
    assert ((key != NULL) && (key_size > 0));                                                                          \
                                                                                                                       \
    unsigned a, b;                                                                                                     \
    unsigned c = 0;                                                                                                    \
    unsigned char *k = (unsigned char *) key;                                                                          \
                                                                                                                       \
    a = b = 0x9e3779b9;                                                                                                \
                                                                                                                       \
    while (key_size >= 12) {                                                                                           \
        a += (k[0] + ((unsigned)k[1] << 8) + ((unsigned)k[2] << 16) + ((unsigned)k[3] << 24));                         \
        b += (k[4] + ((unsigned)k[5] << 8) + ((unsigned)k[6] << 16) + ((unsigned)k[7] << 24));                         \
        c += (k[8] + ((unsigned)k[9] << 8) + ((unsigned)k[10] << 16) + ((unsigned)k[11] << 24));                       \
        JENKINS_MIX(a, b, c);                                                                                          \
        k += 12;                                                                                                       \
        key_size -= 12;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    c += key_size;                                                                                                     \
                                                                                                                       \
    switch (key_size) {                                                                                                \
        case 11: c += ((unsigned)k[10] << 24); break;                                                                  \
        case 10: c += ((unsigned)k[9] << 16); break;                                                                   \
        case 9: c += ((unsigned)k[8] << 8); break;                                                                     \
        case 8: b += ((unsigned)k[7] << 24); break;                                                                    \
        case 7: b += ((unsigned)k[6] << 16); break;                                                                    \
        case 6: b += ((unsigned)k[5] << 8); break;                                                                     \
        case 5: b += k[4]; break;                                                                                      \
        case 4: a += ((unsigned)k[3] << 24); break;                                                                    \
        case 3: a += ((unsigned)k[2] << 16); break;                                                                    \
        case 2: a += ((unsigned)k[1] << 8); break;                                                                     \
        case 1: a += k[0]; break;                                                                                      \
    }                                                                                                                  \
    JENKINS_MIX(a, b, c);                                                                                              \
    c;                                                                                                                 \
})

NG5_END_DECL

#endif