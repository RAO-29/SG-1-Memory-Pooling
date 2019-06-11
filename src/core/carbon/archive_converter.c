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

#include "inttypes.h"

#include "shared/common.h"
#include "core/carbon/archive_visitor.h"
#include "core/carbon/archive_converter.h"

struct converter_capture {
        struct encoded_doc_list *collection;
};

#define IMPORT_BASIC_PAIR(name)                                                                                        \
{                                                                                                                      \
    ng5_unused(archive);                                                                                            \
    ng5_unused(path_stack);                                                                                         \
    assert(capture);                                                                                                   \
                                                                                                                       \
    struct converter_capture *extra = (struct converter_capture *) capture;                                                                          \
    struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, oid);                          \
    for (u32 i = 0; i < num_pairs; i++) {                                                                         \
        encoded_doc_add_prop_##name(doc, keys[i], values[i]);                                                   \
    }                                                                                                                  \
}

#define DECLARE_VISIT_BASIC_TYPE_PAIR(name, built_in_type)                                                             \
static void                                                                                                            \
visit_##name##_pairs (struct archive *archive, path_stack_t path_stack, object_id_t oid,                      \
                  const field_sid_t *keys, const built_in_type *values, u32 num_pairs, void *capture)      \
{                                                                                                                      \
    IMPORT_BASIC_PAIR(name)                                                                                            \
}

#define DECLARE_VISIT_ARRAY_TYPE(name, built_in_type)                                                                  \
static enum visit_policy                                                                                         \
visit_enter_##name##_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,                  \
                                 const field_sid_t *keys, u32 num_pairs, void *capture)                    \
{                                                                                                                      \
    ng5_unused(archive);                                                                                            \
    ng5_unused(path);                                                                                               \
    ng5_unused(id);                                                                                                 \
    ng5_unused(keys);                                                                                               \
    ng5_unused(num_pairs);                                                                                          \
    ng5_unused(capture);                                                                                            \
                                                                                                                       \
    assert(capture);                                                                                                   \
                                                                                                                       \
    struct converter_capture *extra = (struct converter_capture *) capture;                                                                          \
    struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, id);                           \
    for (u32 i = 0; i < num_pairs; i++)                                                                           \
    {                                                                                                                  \
        encoded_doc_add_prop_array_##name(doc, keys[i]);                                                        \
    }                                                                                                                  \
                                                                                                                       \
    return VISIT_INCLUDE;                                                                              \
}                                                                                                                      \
                                                                                                                       \
static void                                                                                                            \
visit_##name##_array_pair(struct archive *archive, path_stack_t path, object_id_t id,                         \
                          const field_sid_t key, u32 entry_idx, u32 max_entries,                      \
                          const built_in_type *array, u32 array_length, void *capture)                            \
{                                                                                                                      \
    ng5_unused(archive);                                                                                            \
    ng5_unused(path);                                                                                               \
    ng5_unused(id);                                                                                                 \
    ng5_unused(key);                                                                                                \
    ng5_unused(entry_idx);                                                                                          \
    ng5_unused(max_entries);                                                                                        \
    ng5_unused(capture);                                                                                            \
                                                                                                                       \
    assert(capture);                                                                                                   \
                                                                                                                       \
    struct converter_capture *extra = (struct converter_capture *) capture;                                                                          \
    struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, id);                           \
    encoded_doc_array_push_##name(doc, key, array, array_length);                                               \
}                                                                                                                      \


static void visit_root_object(struct archive *archive, object_id_t id, void *capture)
{
        ng5_unused(archive);
        assert(capture);

        struct converter_capture *extra = (struct converter_capture *) capture;
        encoded_doc_collection_get_or_append(extra->collection, id);
}

DECLARE_VISIT_BASIC_TYPE_PAIR(int8, field_i8_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(int16, field_i16_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(int32, field_i32_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(int64, field_i64_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(uint8, field_u8_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(uint16, field_u16_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(uint32, field_u32_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(uint64, field_u64_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(number, field_number_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(boolean, FIELD_BOOLEANean_t)

DECLARE_VISIT_BASIC_TYPE_PAIR(string, field_sid_t)

static void visit_null_pairs(struct archive *archive, path_stack_t path, object_id_t oid, const field_sid_t *keys,
        u32 num_pairs, void *capture)
{
        ng5_unused(archive);
        ng5_unused(path);
        assert(capture);

        struct converter_capture *extra = (struct converter_capture *) capture;
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, oid);
        for (u32 i = 0; i < num_pairs; i++) {
                encoded_doc_add_prop_null(doc, keys[i]);
        }
}

static enum visit_policy before_object_visit(struct archive *archive, path_stack_t path_stack, object_id_t parent_id,
        object_id_t value_id, u32 object_idx, u32 num_objects, field_sid_t key, void *capture)
{
        ng5_unused(archive);
        ng5_unused(path_stack);
        ng5_unused(object_idx);
        ng5_unused(num_objects);
        ng5_unused(key);
        ng5_unused(capture);

        struct converter_capture *extra = (struct converter_capture *) capture;
        struct encoded_doc *parent_doc = encoded_doc_collection_get_or_append(extra->collection, parent_id);
        struct encoded_doc *child_doc = encoded_doc_collection_get_or_append(extra->collection, value_id);
        encoded_doc_add_prop_object(parent_doc, key, child_doc);

        return VISIT_INCLUDE;
}

DECLARE_VISIT_ARRAY_TYPE(int8, field_i8_t)

DECLARE_VISIT_ARRAY_TYPE(int16, field_i16_t)

DECLARE_VISIT_ARRAY_TYPE(int32, field_i32_t)

DECLARE_VISIT_ARRAY_TYPE(int64, field_i64_t)

DECLARE_VISIT_ARRAY_TYPE(uint8, field_u8_t)

DECLARE_VISIT_ARRAY_TYPE(uint16, field_u16_t)

DECLARE_VISIT_ARRAY_TYPE(uint32, field_u32_t)

DECLARE_VISIT_ARRAY_TYPE(uint64, field_u64_t)

DECLARE_VISIT_ARRAY_TYPE(number, field_number_t)

DECLARE_VISIT_ARRAY_TYPE(boolean, FIELD_BOOLEANean_t)

DECLARE_VISIT_ARRAY_TYPE(string, field_sid_t)

static enum visit_policy visit_enter_null_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
        const field_sid_t *keys, u32 num_pairs, void *capture)
{
        ng5_unused(archive);
        ng5_unused(path);
        ng5_unused(id);
        ng5_unused(keys);
        ng5_unused(num_pairs);
        ng5_unused(capture);

        assert(capture);

        struct converter_capture *extra = (struct converter_capture *) capture;
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, id);
        for (u32 i = 0; i < num_pairs; i++) {
                encoded_doc_add_prop_array_null(doc, keys[i]);
        }

        return VISIT_INCLUDE;
}

static void visit_null_array_pair(struct archive *archive, path_stack_t path, object_id_t id, const field_sid_t key,
        u32 entry_idx, u32 max_entries, u32 num_nulls, void *capture)
{
        ng5_unused(archive);
        ng5_unused(path);
        ng5_unused(id);
        ng5_unused(key);
        ng5_unused(entry_idx);
        ng5_unused(max_entries);
        ng5_unused(num_nulls);
        ng5_unused(capture);

        assert(capture);

        struct converter_capture *extra = (struct converter_capture *) capture;
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, id);
        encoded_doc_array_push_null(doc, key, &num_nulls, 1);
}

static void before_visit_object_array_objects(bool *skip_group_object_ids, struct archive *archive, path_stack_t path,
        object_id_t parent_id, field_sid_t key, const object_id_t *group_object_ids, u32 num_group_object_ids,
        void *capture)
{
        ng5_unused(archive);
        ng5_unused(path);
        ng5_unused(parent_id);
        ng5_unused(capture);
        ng5_unused(group_object_ids);
        ng5_unused(skip_group_object_ids);
        ng5_unused(num_group_object_ids);

        struct converter_capture *extra = (struct converter_capture *) capture;
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, parent_id);
        encoded_doc_add_prop_array_object(doc, key);
        for (u32 i = 0; i < num_group_object_ids; i++) {
                encoded_doc_array_push_object(doc, key, group_object_ids[i]);
        }
}

#define DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(name, built_in_type)                                             \
static void                                                                                                            \
visit_object_array_object_property_##name(struct archive *archive, path_stack_t path,                                \
                                           object_id_t parent_id,                                               \
                                           field_sid_t key,                                                     \
                                           object_id_t nested_object_id,                                        \
                                           field_sid_t nested_key,                                              \
                                           const built_in_type *nested_values,                                         \
                                           u32 num_nested_values, void *capture)                                  \
{                                                                                                                      \
    ng5_unused(archive);                                                                                            \
    ng5_unused(path);                                                                                               \
    ng5_unused(parent_id);                                                                                          \
    ng5_unused(key);                                                                                                \
    ng5_unused(nested_key);                                                                                         \
    ng5_unused(nested_values);                                                                                      \
                                                                                                                       \
    struct converter_capture *extra = (struct converter_capture *) capture;                                                                          \
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(extra->collection, nested_object_id);             \
        encoded_doc_add_prop_array_##name(doc, nested_key);                                                                                                   \
        encoded_doc_array_push_##name(doc, nested_key, nested_values, num_nested_values);                           \
}

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(int8, field_i8_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(int16, field_i16_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(int32, field_i32_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(int64, field_i64_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(uint8, field_u8_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(uint16, field_u16_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(uint32, field_u32_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(uint64, field_u64_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(number, field_number_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(string, field_sid_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(boolean, FIELD_BOOLEANean_t);

DEFINE_VISIT_OBJECT_ARRAY_OBJECT_PROP_HANDLER(null, field_u32_t);

NG5_EXPORT(bool) archive_converter(struct encoded_doc_list *collection, struct archive *archive)
{

        error_if_null(collection);
        error_if_null(archive);

        encoded_doc_collection_create(collection, &archive->err, archive);

        struct archive_visitor visitor = {0};
        struct archive_visitor_desc desc = {.visit_mask = NG5_ARCHIVE_ITER_MASK_ANY};
        struct converter_capture capture = {.collection = collection};

        visitor.visit_root_object = visit_root_object;
        visitor.before_object_visit = before_object_visit;
        visitor.visit_int8_pairs = visit_int8_pairs;
        visitor.visit_int16_pairs = visit_int16_pairs;
        visitor.visit_int32_pairs = visit_int32_pairs;
        visitor.visit_int64_pairs = visit_int64_pairs;
        visitor.visit_uint8_pairs = visit_uint8_pairs;
        visitor.visit_uint16_pairs = visit_uint16_pairs;
        visitor.visit_uint32_pairs = visit_uint32_pairs;
        visitor.visit_uint64_pairs = visit_uint64_pairs;
        visitor.visit_number_pairs = visit_number_pairs;
        visitor.visit_string_pairs = visit_string_pairs;
        visitor.visit_boolean_pairs = visit_boolean_pairs;
        visitor.visit_null_pairs = visit_null_pairs;

        visitor.visit_enter_int8_array_pairs = visit_enter_int8_array_pairs;
        visitor.visit_int8_array_pair = visit_int8_array_pair;
        visitor.visit_enter_int16_array_pairs = visit_enter_int16_array_pairs;
        visitor.visit_int16_array_pair = visit_int16_array_pair;
        visitor.visit_enter_int32_array_pairs = visit_enter_int32_array_pairs;
        visitor.visit_int32_array_pair = visit_int32_array_pair;
        visitor.visit_enter_int64_array_pairs = visit_enter_int64_array_pairs;
        visitor.visit_int64_array_pair = visit_int64_array_pair;
        visitor.visit_enter_uint8_array_pairs = visit_enter_uint8_array_pairs;
        visitor.visit_uint8_array_pair = visit_uint8_array_pair;
        visitor.visit_enter_uint16_array_pairs = visit_enter_uint16_array_pairs;
        visitor.visit_uint16_array_pair = visit_uint16_array_pair;
        visitor.visit_enter_uint32_array_pairs = visit_enter_uint32_array_pairs;
        visitor.visit_uint32_array_pair = visit_uint32_array_pair;
        visitor.visit_enter_uint64_array_pairs = visit_enter_uint64_array_pairs;
        visitor.visit_uint64_array_pair = visit_uint64_array_pair;
        visitor.visit_enter_boolean_array_pairs = visit_enter_boolean_array_pairs;
        visitor.visit_boolean_array_pair = visit_boolean_array_pair;
        visitor.visit_enter_number_array_pairs = visit_enter_number_array_pairs;
        visitor.visit_number_array_pair = visit_number_array_pair;
        visitor.visit_enter_null_array_pairs = visit_enter_null_array_pairs;
        visitor.visit_null_array_pair = visit_null_array_pair;
        visitor.visit_enter_string_array_pairs = visit_enter_string_array_pairs;
        visitor.visit_string_array_pair = visit_string_array_pair;

        visitor.before_visit_object_array_objects = before_visit_object_array_objects;

        visitor.visit_object_array_object_property_int8s = visit_object_array_object_property_int8;
        visitor.visit_object_array_object_property_int16s = visit_object_array_object_property_int16;
        visitor.visit_object_array_object_property_int32s = visit_object_array_object_property_int32;
        visitor.visit_object_array_object_property_int64s = visit_object_array_object_property_int64;
        visitor.visit_object_array_object_property_uint8s = visit_object_array_object_property_uint8;
        visitor.visit_object_array_object_property_uint16s = visit_object_array_object_property_uint16;
        visitor.visit_object_array_object_property_uint32s = visit_object_array_object_property_uint32;
        visitor.visit_object_array_object_property_uint64s = visit_object_array_object_property_uint64;
        visitor.visit_object_array_object_property_numbers = visit_object_array_object_property_number;
        visitor.visit_object_array_object_property_strings = visit_object_array_object_property_string;
        visitor.visit_object_array_object_property_booleans = visit_object_array_object_property_boolean;
        visitor.visit_object_array_object_property_nulls = visit_object_array_object_property_null;

        archive_visit_archive(archive, &desc, &visitor, &capture);

        return true;
}


