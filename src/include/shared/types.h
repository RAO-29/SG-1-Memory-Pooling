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

#ifndef NG5_TYPES_H
#define NG5_TYPES_H

#include "shared/common.h"

NG5_BEGIN_DECL

typedef uint8_t u3; /* an 8 bit unsigned integer from which only 3 bits are used (see 'tagged_ptr.h') */

typedef uint8_t u8;

typedef uint16_t u16;

typedef uint32_t u32;

typedef uint64_t u64;

typedef int8_t i8;

typedef int16_t i16;

typedef int32_t i32;

typedef int64_t i64;

struct doc_obj;

typedef u64 field_sid_t;  /* string identifier, resolvable by a string dictionary */
typedef char FIELD_NULL_t;

typedef i8 FIELD_BOOLEANean_t;

typedef i8 field_i8_t;

typedef i16 field_i16_t;

typedef i32 field_i32_t;

typedef i64 field_i64_t;

typedef u8 field_u8_t;

typedef u16 field_u16_t;

typedef u32 field_u32_t;

typedef u64 field_u64_t;

typedef float field_number_t;

typedef const char *FIELD_STRING_t;

#define NG5_NULL_ENCODED_STRING            0
#define NG5_NULL_BOOLEAN                   INT8_MAX
#define NG5_NULL_INT8                      INT8_MAX
#define NG5_NULL_INT16                     INT16_MAX
#define NG5_NULL_INT32                     INT32_MAX
#define NG5_NULL_INT64                     INT64_MAX
#define NG5_NULL_UINT8                     UINT8_MAX
#define NG5_NULL_UINT16                    UINT16_MAX
#define NG5_NULL_UINT32                    UINT32_MAX
#define NG5_NULL_UINT64                    UINT64_MAX
#define NG5_NULL_FLOAT                     NAN
#define NG5_NULL_OBJECT_MODEL(objectModel) (objectModel->entries.num_elems == 0)

#define NG5_IS_NULL_STRING(str)   (str == NG5_NULL_ENCODED_STRING)
#define NG5_IS_NULL_BOOLEAN(val)  (val == NG5_NULL_BOOLEAN)
#define NG5_IS_NULL_INT8(val)     (val == NG5_NULL_INT8)
#define NG5_IS_NULL_INT16(val)    (val == NG5_NULL_INT16)
#define NG5_IS_NULL_INT32(val)    (val == NG5_NULL_INT32)
#define NG5_IS_NULL_INT64(val)    (val == NG5_NULL_INT64)
#define NG5_IS_NULL_UINT8(val)    (val == NG5_NULL_UINT8)
#define NG5_IS_NULL_UINT16(val)   (val == NG5_NULL_UINT16)
#define NG5_IS_NULL_UINT32(val)   (val == NG5_NULL_UINT32)
#define NG5_IS_NULL_UINT64(val)   (val == NG5_NULL_UINT64)
#define NG5_IS_NULL_NUMBER(val)   (val == NG5_NULL_FLOAT)

#define NG5_LIMITS_INT8_MAX                (NG5_NULL_INT8 - 1)
#define NG5_LIMITS_INT16_MAX               (NG5_NULL_INT16 - 1)
#define NG5_LIMITS_INT32_MAX               (NG5_NULL_INT32 - 1)
#define NG5_LIMITS_INT64_MAX               (NG5_NULL_INT64 - 1)
#define NG5_LIMITS_UINT8_MAX               (NG5_NULL_UINT8 - 1)
#define NG5_LIMITS_UINT16_MAX              (NG5_NULL_UINT16 - 1)
#define NG5_LIMITS_UINT32_MAX              (NG5_NULL_UINT32 - 1)
#define NG5_LIMITS_UINT64_MAX              (NG5_NULL_UINT64 - 1)

#define NG5_LIMITS_INT8_MIN                INT8_MIN
#define NG5_LIMITS_INT16_MIN               INT16_MIN
#define NG5_LIMITS_INT32_MIN               INT32_MIN
#define NG5_LIMITS_INT64_MIN               INT64_MIN
#define NG5_LIMITS_UINT8_MIN               0
#define NG5_LIMITS_UINT16_MIN              0
#define NG5_LIMITS_UINT32_MIN              0
#define NG5_LIMITS_UINT64_MIN              0

#define NG5_NULL_TEXT "null"

#define NG5_BOOLEAN_FALSE 0
#define NG5_BOOLEAN_TRUE  1

#define GET_TYPE_SIZE(value_type)                                                                                      \
({                                                                                                                     \
    size_t value_size;                                                                                                 \
    switch (value_type) {                                                                                              \
        case FIELD_NULL:                                                                                               \
            value_size = sizeof(u16);                                                                                  \
            break;                                                                                                     \
        case FIELD_BOOLEAN:                                                                                            \
            value_size = sizeof(FIELD_BOOLEANean_t);                                                                   \
            break;                                                                                                     \
        case FIELD_INT8:                                                                                               \
            value_size = sizeof(field_i8_t);                                                                           \
            break;                                                                                                     \
        case FIELD_INT16:                                                                                              \
            value_size = sizeof(field_i16_t);                                                                          \
            break;                                                                                                     \
        case FIELD_INT32:                                                                                              \
            value_size = sizeof(field_i32_t);                                                                          \
            break;                                                                                                     \
        case FIELD_INT64:                                                                                              \
            value_size = sizeof(field_i64_t);                                                                          \
            break;                                                                                                     \
        case FIELD_UINT8:                                                                                              \
            value_size = sizeof(field_u8_t);                                                                           \
            break;                                                                                                     \
        case FIELD_UINT16:                                                                                             \
            value_size = sizeof(field_u16_t);                                                                          \
            break;                                                                                                     \
        case FIELD_UINT32:                                                                                             \
            value_size = sizeof(field_u32_t);                                                                          \
            break;                                                                                                     \
        case FIELD_UINT64:                                                                                             \
            value_size = sizeof(field_u64_t);                                                                          \
            break;                                                                                                     \
        case FIELD_FLOAT:                                                                                              \
            value_size = sizeof(field_number_t);                                                                       \
            break;                                                                                                     \
        case FIELD_STRING:                                                                                             \
            value_size = sizeof(field_sid_t);                                                                          \
            break;                                                                                                     \
        case FIELD_OBJECT:                                                                                             \
            value_size = sizeof(struct columndoc_obj);                                                                 \
            break;                                                                                                     \
        default:                                                                                                       \
        print_error_and_die(NG5_ERR_NOTYPE);                                                                           \
    }                                                                                                                  \
    value_size;                                                                                                        \
})

NG5_END_DECL

#endif