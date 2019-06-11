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

#ifndef NG5_ARCHIVE_ITER_H
#define NG5_ARCHIVE_ITER_H

#include "shared/common.h"
#include "shared/error.h"
#include "archive.h"

NG5_BEGIN_DECL

enum prop_iter_state {
        PROP_ITER_INIT,
        PROP_ITER_NULLS,
        PROP_ITER_BOOLS,
        PROP_ITER_INT8S,
        PROP_ITER_INT16S,
        PROP_ITER_INT32S,
        PROP_ITER_INT64S,
        PROP_ITER_UINT8S,
        PROP_ITER_UINT16S,
        PROP_ITER_UINT32S,
        PROP_ITER_UINT64S,
        PROP_ITER_FLOATS,
        PROP_ITER_STRINGS,
        PROP_ITER_OBJECTS,
        PROP_ITER_NULL_ARRAYS,
        PROP_ITER_BOOL_ARRAYS,
        PROP_ITER_INT8_ARRAYS,
        PROP_ITER_INT16_ARRAYS,
        PROP_ITER_INT32_ARRAYS,
        PROP_ITER_INT64_ARRAYS,
        PROP_ITER_UINT8_ARRAYS,
        PROP_ITER_UINT16_ARRAYS,
        PROP_ITER_UINT32_ARRAYS,
        PROP_ITER_UINT64_ARRAYS,
        PROP_ITER_FLOAT_ARRAYS,
        PROP_ITER_STRING_ARRAYS,
        PROP_ITER_OBJECT_ARRAYS,
        PROP_ITER_DONE
};

struct archive_object {
        object_id_t object_id;                  /* unique object id */
        offset_t offset;                        /* this objects header offset */
        struct archive_prop_offs prop_offsets;  /* per-property type offset in the record table byte stream */
        offset_t next_obj_off;                  /* offset to next object in list, or NULL if no such exists */
        struct memfile memfile;
        struct err err;
};

enum prop_iter_mode {
        PROP_ITER_MODE_OBJECT,
        PROP_ITER_MODE_COLLECTION,
};

struct object_iter_state {
        offset_t prop_type_off_data;            /* offset of type-dependent data in memfile */
        struct fixed_prop prop_group_header;    /* type, num props and keys */
        offset_t current_prop_group_off;
        offset_t prop_data_off;
        const field_sid_t *keys;                /* current property key in this iteration */
        enum field_type type;                   /* property basic value type (e.g., int8, or object) */
        bool is_array;                          /* flag indicating that property is an array type */
};

struct collection_iter_state {
        offset_t collection_start_off;
        u32 num_column_groups;
        u32 current_column_group_idx;
        const field_sid_t *column_group_keys;
        const offset_t *column_group_offsets;

        struct {
                u32 num_columns;
                u32 num_objects;
                const object_id_t *object_ids;
                const offset_t *column_offs;
                struct {
                        u32 idx;
                        field_sid_t name;
                        enum field_type type;
                        u32 num_elem;
                        const offset_t *elem_offsets;
                        const u32 *elem_positions;
                        struct {
                                u32 idx;
                                u32 array_length;
                                const void *array_base;
                        } current_entry;
                } current_column;
        } current_column_group;
};

struct archive_value_vector {
        struct prop_iter *prop_iter;            /* pointer to property iterator that created this iterator */
        struct memfile record_table_memfile;    /* iterator-local read-only memfile on archive record table */
        enum field_type prop_type;              /* property basic value type (e.g., int8, or object) */
        bool is_array;                          /* flag indicating whether value type is an array or not */
        offset_t data_off;                      /* offset in memfile where type-dependent data begins */
        u32 value_max_idx;                      /* maximum index of a value callable by 'at' functions */
        struct err err;                         /* error information */
        object_id_t object_id;                  /* current object id */
        const field_sid_t *keys;
        union {
                struct {
                        const offset_t *offsets;
                        struct archive_object object;
                } object;
                struct {
                        union {
                                const field_i8_t *int8s;
                                const field_i16_t *int16s;
                                const field_i32_t *int32s;
                                const field_i64_t *int64s;
                                const field_u8_t *uint8s;
                                const field_u16_t *uint16s;
                                const field_u32_t *uint32s;
                                const field_u64_t *uint64s;
                                const field_number_t *numbers;
                                const field_sid_t *strings;
                                const FIELD_BOOLEANean_t *booleans;
                        } values;
                } basic;
                struct {
                        union {
                                const u32 *array_lengths;
                                const u32 *num_nulls_contained;
                        } meta;

                        union {
                                const field_i8_t *int8s_base;
                                const field_i16_t *int16s_base;
                                const field_i32_t *int32s_base;
                                const field_i64_t *int64s_base;
                                const field_u8_t *uint8s_base;
                                const field_u16_t *uint16s_base;
                                const field_u32_t *uint32s_base;
                                const field_u64_t *uint64s_base;
                                const field_number_t *numbers_base;
                                const field_sid_t *strings_base;
                                const FIELD_BOOLEANean_t *booleans_base;
                        } values;
                } arrays;
        } data;
};

struct prop_iter {
        struct archive_object object;                 /* current object */
        struct memfile record_table_memfile;          /* iterator-local read-only memfile on archive record table */

        u16 mask;                                     /* user-defined mask which properties to include */
        enum prop_iter_mode mode;                     /* determines whether to iterating over object or collection */
        struct err err;                               /* error information */
        enum prop_iter_state prop_cursor;             /* current property type in iteration */

        struct object_iter_state mode_object;
        struct collection_iter_state mode_collection;

};

struct independent_iter_state {
        struct memfile record_table_memfile;           /* iterator-local read-only memfile on archive record table */
        struct collection_iter_state state;            /* iterator-local state */
        struct err err;                                /* error information */
};

typedef struct independent_iter_state archive_collection_iter_t;

typedef struct independent_iter_state archive_column_group_iter_t;

typedef struct independent_iter_state archive_column_iter_t;

typedef struct independent_iter_state archive_column_entry_iter_t;

struct column_object_iter {
        struct memfile memfile;
        struct collection_iter_state entry_state;
        struct archive_object obj;
        offset_t next_obj_off;
        struct err err;
};

#define NG5_ARCHIVE_ITER_MASK_PRIMITIVES             (1 << 1)
#define NG5_ARCHIVE_ITER_MASK_ARRAYS                 (1 << 2)

#define NG5_ARCHIVE_ITER_MASK_INT8                   (1 << 3)
#define NG5_ARCHIVE_ITER_MASK_INT16                  (1 << 4)
#define NG5_ARCHIVE_ITER_MASK_INT32                  (1 << 5)
#define NG5_ARCHIVE_ITER_MASK_INT64                  (1 << 6)
#define NG5_ARCHIVE_ITER_MASK_UINT8                  (1 << 7)
#define NG5_ARCHIVE_ITER_MASK_UINT16                 (1 << 8)
#define NG5_ARCHIVE_ITER_MASK_UINT32                 (1 << 9)
#define NG5_ARCHIVE_ITER_MASK_UINT64                 (1 << 10)
#define NG5_ARCHIVE_ITER_MASK_NUMBER                 (1 << 11)
#define NG5_ARCHIVE_ITER_MASK_STRING                 (1 << 12)
#define NG5_ARCHIVE_ITER_MASK_BOOLEAN                (1 << 13)
#define NG5_ARCHIVE_ITER_MASK_NULL                   (1 << 14)
#define NG5_ARCHIVE_ITER_MASK_OBJECT                 (1 << 15)

#define NG5_ARCHIVE_ITER_MASK_INTEGER               NG5_ARCHIVE_ITER_MASK_INT8       |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_INT16      |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_INT32      |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_INT64      |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_UINT8      |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_UINT16     |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_UINT32     |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_UINT64

#define NG5_ARCHIVE_ITER_MASK_ANY                   NG5_ARCHIVE_ITER_MASK_PRIMITIVES |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_ARRAYS     |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_INTEGER    |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_NUMBER     |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_STRING     |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_BOOLEAN    |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_NULL       |                                 \
                                                    NG5_ARCHIVE_ITER_MASK_OBJECT

NG5_DEFINE_GET_ERROR_FUNCTION(archive_value_vector, struct archive_value_vector, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_prop_iter, struct prop_iter, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_collection_iter, archive_collection_iter_t, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_column_group_iter, archive_column_group_iter_t, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_column_iter, archive_column_iter_t, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_column_entry_iter, archive_column_entry_iter_t, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_column_entry_object_iter, struct column_object_iter, iter)

NG5_DEFINE_GET_ERROR_FUNCTION(archive_object, struct archive_object, obj)

NG5_EXPORT(bool) archive_prop_iter_from_archive(struct prop_iter *iter, struct err *err, u16 mask,
        struct archive *archive);

NG5_EXPORT(bool) archive_prop_iter_from_object(struct prop_iter *iter, u16 mask, struct err *err,
        const struct archive_object *obj);

NG5_EXPORT(bool) archive_value_vector_from_prop_iter(struct archive_value_vector *value, struct err *err,
        struct prop_iter *prop_iter);

NG5_EXPORT(bool) archive_prop_iter_next(enum prop_iter_mode *type, struct archive_value_vector *value_vector,
        archive_collection_iter_t *collection_iter, struct prop_iter *prop_iter);

NG5_EXPORT(const field_sid_t *)archive_collection_iter_get_keys(u32 *num_keys, archive_collection_iter_t *iter);

NG5_EXPORT(bool) archive_collection_next_column_group(archive_column_group_iter_t *group_iter,
        archive_collection_iter_t *iter);

NG5_EXPORT(const object_id_t *)archive_column_group_get_object_ids(u32 *num_objects, archive_column_group_iter_t *iter);

NG5_EXPORT(bool) archive_column_group_next_column(archive_column_iter_t *column_iter,
        archive_column_group_iter_t *iter);

NG5_EXPORT(bool) archive_column_get_name(field_sid_t *name, enum field_type *type, archive_column_iter_t *column_iter);

NG5_EXPORT(const u32 *)archive_column_get_entry_positions(u32 *num_entry, archive_column_iter_t *column_iter);

NG5_EXPORT(bool) archive_column_next_entry(archive_column_entry_iter_t *entry_iter, archive_column_iter_t *iter);

NG5_EXPORT(bool) archive_column_entry_get_type(enum field_type *type, archive_column_entry_iter_t *entry);

#define DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(built_in_type, name)                                            \
NG5_EXPORT(const built_in_type *)                                                                                      \
archive_column_entry_get_##name(u32 *array_length, archive_column_entry_iter_t *entry);

DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_i8_t, int8s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_i16_t, int16s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_i32_t, int32s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_i64_t, int64s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_u8_t, uint8s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_u16_t, uint16s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_u32_t, uint32s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_u64_t, uint64s);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_sid_t, strings);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_number_t, numbers);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(FIELD_BOOLEANean_t, booleans);
DEFINE_NG5_ARCHIVE_COLUMN_ENTRY_GET_BASIC_TYPE(field_u32_t, nulls);

NG5_EXPORT(bool) archive_column_entry_get_objects(struct column_object_iter *iter, archive_column_entry_iter_t *entry);

NG5_EXPORT(const struct archive_object *)archive_column_entry_object_iter_next_object(struct column_object_iter *iter);

NG5_EXPORT(bool) archive_object_get_object_id(object_id_t *id, const struct archive_object *object);

NG5_EXPORT(bool) archive_object_get_prop_iter(struct prop_iter *iter, const struct archive_object *object);

NG5_EXPORT(bool) archive_value_vector_get_object_id(object_id_t *id, const struct archive_value_vector *iter);

NG5_EXPORT(const field_sid_t *)archive_value_vector_get_keys(u32 *num_keys, struct archive_value_vector *iter);

NG5_EXPORT(const field_sid_t *)archive_value_vector_get_keys(u32 *num_keys, struct archive_value_vector *iter);

NG5_EXPORT(bool) archive_value_vector_get_basic_type(enum field_type *type, const struct archive_value_vector *value);

NG5_EXPORT(bool) archive_value_vector_is_array_type(bool *is_array, const struct archive_value_vector *value);

NG5_EXPORT(bool) archive_value_vector_get_length(u32 *length, const struct archive_value_vector *value);

NG5_EXPORT(bool) archive_value_vector_is_of_objects(bool *is_object, struct archive_value_vector *value);

NG5_EXPORT(bool) archive_value_vector_get_object_at(struct archive_object *object, u32 idx,
        struct archive_value_vector *value);

#define DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(name)                                                            \
NG5_EXPORT(bool)                                                                                                       \
archive_value_vector_is_##name(bool *type_match, struct archive_value_vector *value);

DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(int8);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(int16);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(int32);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(int64);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(uint8);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(uint16);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(uint32);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(uint64);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(string);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(number);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(boolean);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_IS_BASIC_TYPE(null);

#define DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(name, built_in_type)                                            \
NG5_EXPORT(const built_in_type *)                                                                                      \
archive_value_vector_get_##name(u32 *num_values, struct archive_value_vector *value);

DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(int8s, field_i8_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(int16s, field_i16_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(int32s, field_i32_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(int64s, field_i64_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(uint8s, field_u8_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(uint16s, field_u16_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(uint32s, field_u32_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(uint64s, field_u64_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(strings, field_sid_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(numbers, field_number_t)
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_BASIC_TYPE(booleans, FIELD_BOOLEANean_t)

NG5_EXPORT(const field_u32_t *)archive_value_vector_get_null_arrays(u32 *num_values,
        struct archive_value_vector *value);

#define DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(name, built_in_type)                                         \
NG5_EXPORT(const built_in_type *)                                                                                      \
archive_value_vector_get_##name##_arrays_at(u32 *array_length, u32 idx,                                                \
                                               struct archive_value_vector *value);                                    \

DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(int8, field_i8_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(int16, field_i16_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(int32, field_i32_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(int64, field_i64_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(uint8, field_u8_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(uint16, field_u16_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(uint32, field_u32_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(uint64, field_u64_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(string, field_sid_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(number, field_number_t);
DEFINE_NG5_ARCHIVE_VALUE_VECTOR_GET_ARRAY_TYPE_AT(boolean, FIELD_BOOLEANean_t);

void int_reset_cabin_object_mem_file(struct archive_object *object);

NG5_END_DECL

#endif
