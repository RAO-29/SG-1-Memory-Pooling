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
#include <inttypes.h>
#include "json/json.h"
#include "json/doc.h"
#include "json/columndoc.h"
#include "json/json.h"
#include "std/sort.h"

char VALUE_NULL = '\0';

static void create_doc(struct doc_obj *model, struct doc *doc);

static void create_typed_vector(struct doc_entries *entry);

static void entries_drop(struct doc_entries *entry);

static bool print_value(FILE *file, field_e type, const struct vector ofType(<T>) *values);

static void print_object(FILE *file, const struct doc_obj *model);

static bool import_json_object(struct doc_obj *target, struct err *err, const struct json_object_t *json_obj);

static void sort_columndoc_entries(struct columndoc_obj *columndoc);

NG5_EXPORT(bool) doc_bulk_create(struct doc_bulk *bulk, struct strdic *dic)
{
        error_if_null(bulk)
        error_if_null(dic)
        bulk->dic = dic;
        vec_create(&bulk->keys, NULL, sizeof(char *), 500);
        vec_create(&bulk->values, NULL, sizeof(char *), 1000);
        vec_create(&bulk->models, NULL, sizeof(struct doc), 50);
        return true;
}

struct doc_obj *doc_bulk_new_obj(struct doc *model)
{
        if (!model) {
                return NULL;
        } else {
                struct doc_obj *retval = vec_new_and_get(&model->obj_model, struct doc_obj);
                create_doc(retval, model);
                return retval;
        }
}

NG5_EXPORT(bool) doc_bulk_get_dic_contents(struct vector ofType (const char *) **strings,
        struct vector ofType(field_sid_t) **string_ids, const struct doc_bulk *context)
{
        error_if_null(context)

        size_t num_distinct_values;
        strdic_num_distinct(&num_distinct_values, context->dic);
        struct vector ofType (const char *) *result_strings = malloc(sizeof(struct vector));
        struct vector ofType (field_sid_t) *resultstring_id_ts = malloc(sizeof(struct vector));
        vec_create(result_strings, NULL, sizeof(const char *), num_distinct_values);
        vec_create(resultstring_id_ts, NULL, sizeof(field_sid_t), num_distinct_values);

        int status = strdic_get_contents(result_strings, resultstring_id_ts, context->dic);
        ng5_check_success(status);
        *strings = result_strings;
        *string_ids = resultstring_id_ts;

        return status;
}

struct doc *doc_bulk_new_doc(struct doc_bulk *context, field_e type)
{
        if (!context) {
                return NULL;
        }

        struct doc template, *model;
        size_t idx = vec_length(&context->models);
        vec_push(&context->models, &template, 1);
        model = vec_get(&context->models, idx, struct doc);
        model->context = context;
        model->type = type;

        vec_create(&model->obj_model, NULL, sizeof(struct doc_obj), 500);

        return model;
}

NG5_EXPORT(bool) doc_bulk_Drop(struct doc_bulk *bulk)
{
        error_if_null(bulk)
        for (size_t i = 0; i < bulk->keys.num_elems; i++) {
                char *string = *vec_get(&bulk->keys, i, char *);
                free(string);
        }
        for (size_t i = 0; i < bulk->values.num_elems; i++) {
                char *string = *vec_get(&bulk->values, i, char *);
                free(string);
        }
        for (size_t i = 0; i < bulk->models.num_elems; i++) {
                struct doc *model = vec_get(&bulk->models, i, struct doc);
                for (size_t j = 0; j < model->obj_model.num_elems; j++) {
                        struct doc_obj *doc = vec_get(&model->obj_model, j, struct doc_obj);
                        doc_drop(doc);
                }
                vec_drop(&model->obj_model);
        }

        vec_drop(&bulk->keys);
        vec_drop(&bulk->values);
        vec_drop(&bulk->models);
        return true;
}

NG5_EXPORT(bool) doc_bulk_shrink(struct doc_bulk *bulk)
{
        error_if_null(bulk)
        vec_shrink(&bulk->keys);
        vec_shrink(&bulk->values);
        return true;
}

NG5_EXPORT(bool) doc_bulk_print(FILE *file, struct doc_bulk *bulk)
{
        error_if_null(file)
        error_if_null(bulk)

        fprintf(file, "{");
        char **key_strings = vec_all(&bulk->keys, char *);
        fprintf(file, "\"Key Strings\": [");
        for (size_t i = 0; i < bulk->keys.num_elems; i++) {
                fprintf(file, "\"%s\"%s", key_strings[i], i + 1 < bulk->keys.num_elems ? ", " : "");
        }
        fprintf(file, "], ");

        char **valueStrings = vec_all(&bulk->values, char *);
        fprintf(file, "\"Value Strings\": [");
        for (size_t i = 0; i < bulk->values.num_elems; i++) {
                fprintf(file, "\"%s\"%s", valueStrings[i], i + 1 < bulk->values.num_elems ? ", " : "");
        }
        fprintf(file, "]}");

        return true;
}

NG5_EXPORT(bool) doc_print(FILE *file, const struct doc *doc)
{
        error_if_null(file)
        error_if_null(doc)

        if (doc->obj_model.num_elems == 0) {
                fprintf(file, "{ }");
        }

        if (doc->obj_model.num_elems > 1) {
                fprintf(file, "[");
        }

        for (size_t num_entries = 0; num_entries < doc->obj_model.num_elems; num_entries++) {
                struct doc_obj *object = vec_get(&doc->obj_model, num_entries, struct doc_obj);
                print_object(file, object);
                fprintf(file, "%s", num_entries + 1 < doc->obj_model.num_elems ? ", " : "");
        }

        if (doc->obj_model.num_elems > 1) {
                fprintf(file, "]");
        }

        return true;
}

const struct vector ofType(struct doc_entries) *doc_get_entries(const struct doc_obj *model)
{
        return &model->entries;
}

void doc_print_entries(FILE *file, const struct doc_entries *entries)
{
        fprintf(file, "{\"Key\": \"%s\"", entries->key);
}

void doc_drop(struct doc_obj *model)
{
        for (size_t i = 0; i < model->entries.num_elems; i++) {
                struct doc_entries *entry = vec_get(&model->entries, i, struct doc_entries);
                entries_drop(entry);
        }
        vec_drop(&model->entries);
}

bool doc_obj_add_key(struct doc_entries **out, struct doc_obj *obj, const char *key, field_e type)
{
        error_if_null(out)
        error_if_null(obj)
        error_if_null(key)

        size_t entry_idx;
        char *key_dup = strdup(key);

        struct doc_entries entry_model = {.type = type, .key = key_dup, .context = obj};

        create_typed_vector(&entry_model);
        vec_push(&obj->doc->context->keys, &key_dup, 1);

        entry_idx = vec_length(&obj->entries);
        vec_push(&obj->entries, &entry_model, 1);

        *out = vec_get(&obj->entries, entry_idx, struct doc_entries);

        return true;
}

bool doc_obj_push_primtive(struct doc_entries *entry, const void *value)
{
        error_if_null(entry)
        error_if_null((entry->type == FIELD_NULL) || (value != NULL))

        switch (entry->type) {
        case FIELD_NULL:
                vec_push(&entry->values, &VALUE_NULL, 1);
                break;
        case FIELD_STRING: {
                char *string = value ? strdup((char *) value) : NULL;
                vec_push(&entry->context->doc->context->values, &string, 1);
                vec_push(&entry->values, &string, 1);
        }
                break;
        default:
                vec_push(&entry->values, value, 1);
                break;
        }
        return true;
}

bool doc_obj_push_object(struct doc_obj **out, struct doc_entries *entry)
{
        error_if_null(out);
        error_if_null(entry);

        assert(entry->type == FIELD_OBJECT);

        struct doc_obj objectModel;

        create_doc(&objectModel, entry->context->doc);
        size_t length = vec_length(&entry->values);
        vec_push(&entry->values, &objectModel, 1);

        *out = vec_get(&entry->values, length, struct doc_obj);

        return true;
}

static field_e value_type_for_json_number(bool *success, struct err *err, const struct json_number *number)
{
        *success = true;
        switch (number->value_type) {
        case JSON_NUMBER_FLOAT:
                return FIELD_FLOAT;
        case JSON_NUMBER_UNSIGNED: {
                u64 test = number->value.unsigned_integer;
                if (test <= NG5_LIMITS_UINT8_MAX) {
                        return FIELD_UINT8;
                } else if (test <= NG5_LIMITS_UINT16_MAX) {
                        return FIELD_UINT16;
                } else if (test <= NG5_LIMITS_UINT32_MAX) {
                        return FIELD_UINT32;
                } else {
                        return FIELD_UINT64;
                }
        }
        case JSON_NUMBER_SIGNED: {
                i64 test = number->value.signed_integer;
                if (test >= NG5_LIMITS_INT8_MIN && test <= NG5_LIMITS_INT8_MAX) {
                        return FIELD_INT8;
                } else if (test >= NG5_LIMITS_INT16_MIN && test <= NG5_LIMITS_INT16_MAX) {
                        return FIELD_INT16;
                } else if (test >= NG5_LIMITS_INT32_MIN && test <= NG5_LIMITS_INT32_MAX) {
                        return FIELD_INT32;
                } else {
                        return FIELD_INT64;
                }
        }
        default: error(err, NG5_ERR_NOJSONNUMBERT);
                *success = false;
                return FIELD_INT8;
        }
}

static void import_json_object_string_prop(struct doc_obj *target, const char *key, const struct json_string *string)
{
        struct doc_entries *entry;
        doc_obj_add_key(&entry, target, key, FIELD_STRING);
        doc_obj_push_primtive(entry, string->value);
}

static bool import_json_object_number_prop(struct doc_obj *target, struct err *err, const char *key,
        const struct json_number *number)
{
        struct doc_entries *entry;
        bool success;
        field_e number_type = value_type_for_json_number(&success, err, number);
        if (!success) {
                return false;
        }
        doc_obj_add_key(&entry, target, key, number_type);
        doc_obj_push_primtive(entry, &number->value);
        return true;
}

static void import_json_object_bool_prop(struct doc_obj *target, const char *key, FIELD_BOOLEANean_t value)
{
        struct doc_entries *entry;
        doc_obj_add_key(&entry, target, key, FIELD_BOOLEAN);
        doc_obj_push_primtive(entry, &value);
}

static void import_json_object_null_prop(struct doc_obj *target, const char *key)
{
        struct doc_entries *entry;
        doc_obj_add_key(&entry, target, key, FIELD_NULL);
        doc_obj_push_primtive(entry, NULL);
}

static bool import_json_object_object_prop(struct doc_obj *target, struct err *err, const char *key,
        const struct json_object_t *object)
{
        struct doc_entries *entry;
        struct doc_obj *nested_object = NULL;
        doc_obj_add_key(&entry, target, key, FIELD_OBJECT);
        doc_obj_push_object(&nested_object, entry);
        return import_json_object(nested_object, err, object);
}

static bool import_json_object_array_prop(struct doc_obj *target, struct err *err, const char *key,
        const struct json_array *array)
{
        struct doc_entries *entry;

        if (!vec_is_empty(&array->elements.elements)) {
                size_t num_elements = array->elements.elements.num_elems;

                /** Find first type that is not null unless the entire array is of type null */
                enum json_value_type array_data_type = JSON_VALUE_NULL;
                field_e field_type;

                for (size_t i = 0; i < num_elements && array_data_type == JSON_VALUE_NULL; i++) {
                        const struct json_element *element = vec_get(&array->elements.elements, i, struct json_element);
                        array_data_type = element->value.value_type;
                }

                switch (array_data_type) {
                case JSON_VALUE_OBJECT:
                        field_type = FIELD_OBJECT;
                        break;
                case JSON_VALUE_STRING:
                        field_type = FIELD_STRING;
                        break;
                case JSON_VALUE_NUMBER: {
                        /** find smallest fitting physical number type */
                        field_e array_number_type = FIELD_NULL;
                        for (size_t i = 0; i < num_elements; i++) {
                                const struct json_element
                                        *element = vec_get(&array->elements.elements, i, struct json_element);
                                if (unlikely(element->value.value_type == JSON_VALUE_NULL)) {
                                        continue;
                                } else {
                                        bool success;
                                        field_e element_number_type =
                                                value_type_for_json_number(&success, err, element->value.value.number);
                                        if (!success) {
                                                return false;
                                        }
                                        assert(element_number_type == FIELD_INT8 || element_number_type == FIELD_INT16
                                                || element_number_type == FIELD_INT32
                                                || element_number_type == FIELD_INT64
                                                || element_number_type == FIELD_UINT8
                                                || element_number_type == FIELD_UINT16
                                                || element_number_type == FIELD_UINT32
                                                || element_number_type == FIELD_UINT64
                                                || element_number_type == FIELD_FLOAT);
                                        if (unlikely(array_number_type == FIELD_NULL)) {
                                                array_number_type = element_number_type;
                                        } else {
                                                if (array_number_type == FIELD_INT8) {
                                                        array_number_type = element_number_type;
                                                } else if (array_number_type == FIELD_INT16) {
                                                        if (element_number_type != FIELD_INT8) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_INT32) {
                                                        if (element_number_type != FIELD_INT8
                                                                && element_number_type != FIELD_INT16) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_INT64) {
                                                        if (element_number_type != FIELD_INT8
                                                                && element_number_type != FIELD_INT16
                                                                && element_number_type != FIELD_INT32) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_UINT8) {
                                                        array_number_type = element_number_type;
                                                } else if (array_number_type == FIELD_UINT16) {
                                                        if (element_number_type != FIELD_UINT16) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_UINT32) {
                                                        if (element_number_type != FIELD_UINT8
                                                                && element_number_type != FIELD_UINT16) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_UINT64) {
                                                        if (element_number_type != FIELD_UINT8
                                                                && element_number_type != FIELD_UINT16
                                                                && element_number_type != FIELD_UINT32) {
                                                                array_number_type = element_number_type;
                                                        }
                                                } else if (array_number_type == FIELD_FLOAT) {
                                                        break;
                                                }
                                        }
                                }
                        }
                        assert(array_number_type != FIELD_NULL);
                        field_type = array_number_type;
                }
                        break;
                case JSON_VALUE_FALSE:
                case JSON_VALUE_TRUE:
                        field_type = FIELD_BOOLEAN;
                        break;
                case JSON_VALUE_NULL:
                        field_type = FIELD_NULL;
                        break;
                case JSON_VALUE_ARRAY: error(err, NG5_ERR_ERRINTERNAL) /** array type is illegal here */
                        return false;
                default: error(err, NG5_ERR_NOTYPE)
                        return false;
                }

                doc_obj_add_key(&entry, target, key, field_type);

                for (size_t i = 0; i < num_elements; i++) {
                        const struct json_element *element = vec_get(&array->elements.elements, i, struct json_element);
                        enum json_value_type ast_node_data_type = element->value.value_type;

                        switch (field_type) {
                        case FIELD_OBJECT: {
                                struct doc_obj *nested_object = NULL;
                                doc_obj_push_object(&nested_object, entry);
                                if (ast_node_data_type != JSON_VALUE_NULL) {
                                        /** the object is null by definition, if no entries are contained */
                                        if (!import_json_object(nested_object, err, element->value.value.object)) {
                                                return false;
                                        }
                                }
                        }
                                break;
                        case FIELD_STRING: {
                                assert(ast_node_data_type == array_data_type || ast_node_data_type == JSON_VALUE_NULL);
                                doc_obj_push_primtive(entry,
                                        ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_ENCODED_STRING : element->value
                                                .value.string->value);
                        }
                                break;
                        case FIELD_INT8:
                        case FIELD_INT16:
                        case FIELD_INT32:
                        case FIELD_INT64:
                        case FIELD_UINT8:
                        case FIELD_UINT16:
                        case FIELD_UINT32:
                        case FIELD_UINT64:
                        case FIELD_FLOAT: {
                                assert(ast_node_data_type == array_data_type || ast_node_data_type == JSON_VALUE_NULL);
                                switch (field_type) {
                                case FIELD_INT8: {
                                        field_i8_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_INT8
                                                                                                 : (field_i8_t) element
                                                        ->value.value.number->value.signed_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_INT16: {
                                        field_i16_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_INT16
                                                                                                  : (field_i16_t) element
                                                        ->value.value.number->value.signed_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_INT32: {
                                        field_i32_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_INT32
                                                                                                  : (field_i32_t) element
                                                        ->value.value.number->value.signed_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_INT64: {
                                        field_i64_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_INT64
                                                                                                  : (field_i64_t) element
                                                        ->value.value.number->value.signed_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_UINT8: {
                                        field_u8_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_UINT8
                                                                                                 : (field_u8_t) element
                                                        ->value.value.number->value.unsigned_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_UINT16: {
                                        field_u16_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_UINT16
                                                                                                  : (field_u16_t) element
                                                        ->value.value.number->value.unsigned_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_UINT32: {
                                        field_u32_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_UINT32
                                                                                                  : (field_u32_t) element
                                                        ->value.value.number->value.unsigned_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_UINT64: {
                                        field_u64_t value = ast_node_data_type == JSON_VALUE_NULL ? NG5_NULL_UINT64
                                                                                                  : (field_u64_t) element
                                                        ->value.value.number->value.unsigned_integer;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                case FIELD_FLOAT: {
                                        field_number_t value = NG5_NULL_FLOAT;
                                        if (ast_node_data_type != JSON_VALUE_NULL) {
                                                enum json_number_type
                                                        element_number_type = element->value.value.number->value_type;
                                                if (element_number_type == JSON_NUMBER_FLOAT) {
                                                        value = element->value.value.number->value.float_number;
                                                } else if (element_number_type == JSON_NUMBER_UNSIGNED) {
                                                        value = element->value.value.number->value.unsigned_integer;
                                                } else if (element_number_type == JSON_NUMBER_SIGNED) {
                                                        value = element->value.value.number->value.signed_integer;
                                                } else {
                                                        print_error_and_die(NG5_ERR_INTERNALERR) /** type mismatch */
                                                        return false;
                                                }
                                        }
                                        doc_obj_push_primtive(entry, &value);
                                }
                                        break;
                                default: print_error_and_die(NG5_ERR_INTERNALERR) /** not a number type  */
                                        return false;
                                }
                        }
                                break;
                        case FIELD_BOOLEAN:
                                if (likely(ast_node_data_type == JSON_VALUE_TRUE
                                        || ast_node_data_type == JSON_VALUE_FALSE)) {
                                        FIELD_BOOLEANean_t value =
                                                ast_node_data_type == JSON_VALUE_TRUE ? NG5_BOOLEAN_TRUE
                                                                                      : NG5_BOOLEAN_FALSE;
                                        doc_obj_push_primtive(entry, &value);
                                } else {
                                        assert(ast_node_data_type == JSON_VALUE_NULL);
                                        FIELD_BOOLEANean_t value = NG5_NULL_BOOLEAN;
                                        doc_obj_push_primtive(entry, &value);
                                }
                                break;
                        case FIELD_NULL:
                                assert(ast_node_data_type == array_data_type);
                                doc_obj_push_primtive(entry, NULL);
                                break;
                        default: error(err, NG5_ERR_NOTYPE)
                                return false;
                        }
                }
        } else {
                import_json_object_null_prop(target, key);
        }
        return true;
}

static bool import_json_object(struct doc_obj *target, struct err *err, const struct json_object_t *json_obj)
{
        for (size_t i = 0; i < json_obj->value->members.num_elems; i++) {
                struct json_prop *member = vec_get(&json_obj->value->members, i, struct json_prop);
                enum json_value_type value_type = member->value.value.value_type;
                switch (value_type) {
                case JSON_VALUE_STRING:
                        import_json_object_string_prop(target, member->key.value, member->value.value.value.string);
                        break;
                case JSON_VALUE_NUMBER:
                        if (!import_json_object_number_prop(target,
                                err,
                                member->key.value,
                                member->value.value.value.number)) {
                                return false;
                        }
                        break;
                case JSON_VALUE_TRUE:
                case JSON_VALUE_FALSE: {
                        FIELD_BOOLEANean_t value = value_type == JSON_VALUE_TRUE ? NG5_BOOLEAN_TRUE : NG5_BOOLEAN_FALSE;
                        import_json_object_bool_prop(target, member->key.value, value);
                }
                        break;
                case JSON_VALUE_NULL:
                        import_json_object_null_prop(target, member->key.value);
                        break;
                case JSON_VALUE_OBJECT:
                        if (!import_json_object_object_prop(target,
                                err,
                                member->key.value,
                                member->value.value.value.object)) {
                                return false;
                        }
                        break;
                case JSON_VALUE_ARRAY:
                        if (!import_json_object_array_prop(target,
                                err,
                                member->key.value,
                                member->value.value.value.array)) {
                                return false;
                        }
                        break;
                default: error(err, NG5_ERR_NOTYPE);
                        return false;
                }
        }
        return true;
}

static bool import_json(struct doc_obj *target, struct err *err, const struct json *json, struct doc_entries *partition)
{
        enum json_value_type value_type = json->element->value.value_type;
        switch (value_type) {
        case JSON_VALUE_OBJECT:
                if (!import_json_object(target, err, json->element->value.value.object)) {
                        return false;
                }
                break;
        case JSON_VALUE_ARRAY: {
                const struct vector ofType(struct json_element)
                        *arrayContent = &json->element->value.value.array->elements.elements;
                if (!vec_is_empty(arrayContent)) {
                        const struct json_element *first = vec_get(arrayContent, 0, struct json_element);
                        switch (first->value.value_type) {
                        case JSON_VALUE_OBJECT:
                                if (!import_json_object(target, err, first->value.value.object)) {
                                        return false;
                                }
                                for (size_t i = 1; i < arrayContent->num_elems; i++) {
                                        const struct json_element
                                                *element = vec_get(arrayContent, i, struct json_element);
                                        struct doc_obj *nested;
                                        doc_obj_push_object(&nested, partition);
                                        if (!import_json_object(nested, err, element->value.value.object)) {
                                                return false;
                                        }
                                }
                                break;
                        case JSON_VALUE_ARRAY:
                        case JSON_VALUE_STRING:
                        case JSON_VALUE_NUMBER:
                        case JSON_VALUE_TRUE:
                        case JSON_VALUE_FALSE:
                        case JSON_VALUE_NULL:
                        default: print_error_and_die(NG5_ERR_INTERNALERR) /** Unsupported operation in arrays */
                                break;
                        }
                }
        }
                break;
        case JSON_VALUE_STRING:
        case JSON_VALUE_NUMBER:
        case JSON_VALUE_TRUE:
        case JSON_VALUE_FALSE:
        case JSON_VALUE_NULL:
        default: error(err, NG5_ERR_JSONTYPE);
                return false;
        }
        return true;
}

struct doc_obj *doc_bulk_add_json(struct doc_entries *partition, struct json *json)
{
        if (!partition || !json) {
                return NULL;
        }

        struct doc_obj *converted_json;
        doc_obj_push_object(&converted_json, partition);
        if (!import_json(converted_json, &json->err, json, partition)) {
                return NULL;
        }

        return converted_json;
}

struct doc_obj *doc_entries_get_root(const struct doc_entries *partition)
{
        return partition ? partition->context : NULL;
}

struct doc_entries *doc_bulk_new_entries(struct doc_bulk *dst)
{
        struct doc_entries *partition = NULL;
        struct doc *model = doc_bulk_new_doc(dst, FIELD_OBJECT);
        struct doc_obj *object = doc_bulk_new_obj(model);
        doc_obj_add_key(&partition, object, "/", FIELD_OBJECT);
        return partition;
}

#define DEFINE_NG5_TYPE_LQ_FUNC(type)                                                                               \
static bool compare_##type##_leq(const void *lhs, const void *rhs)                                                \
{                                                                                                                      \
    type a = *(type *) lhs;                                                                                            \
    type b = *(type *) rhs;                                                                                            \
    return (a <= b);                                                                                                   \
}

DEFINE_NG5_TYPE_LQ_FUNC(FIELD_BOOLEANean_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_number_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_i8_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_i16_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_i32_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_i64_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_u8_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_u16_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_u32_t)

DEFINE_NG5_TYPE_LQ_FUNC(field_u64_t)

static bool compare_encoded_string_less_eq_func(const void *lhs, const void *rhs, void *args)
{
        struct strdic *dic = (struct strdic *) args;
        field_sid_t *a = (field_sid_t *) lhs;
        field_sid_t *b = (field_sid_t *) rhs;
        char **a_string = strdic_extract(dic, a, 1);
        char **b_string = strdic_extract(dic, b, 1);
        bool lq = strcmp(*a_string, *b_string) <= 0;
        strdic_free(dic, a_string);
        strdic_free(dic, b_string);
        return lq;
}

static void sort_nested_primitive_object(struct columndoc_obj *columndoc)
{
        if (columndoc->parent->read_optimized) {
                for (size_t i = 0; i < columndoc->obj_prop_vals.num_elems; i++) {
                        struct columndoc_obj *nestedModel = vec_get(&columndoc->obj_prop_vals, i, struct columndoc_obj);
                        sort_columndoc_entries(nestedModel);
                }
        }
}

#define DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(type)                                                                         \
static bool compare_##type##_array_leq(const void *lhs, const void *rhs)                                           \
{                                                                                                                      \
    struct vector ofType(type) *a = (struct vector *) lhs;                                                               \
    struct vector ofType(type) *b = (struct vector *) rhs;                                                               \
    const type *aValues = vec_all(a, type);                                                                  \
    const type *bValues = vec_all(b, type);                                                                  \
    size_t max_compare_idx = a->num_elems < b->num_elems ? a->num_elems : b->num_elems;                                \
    for (size_t i = 0; i < max_compare_idx; i++) {                                                                     \
        if (aValues[i] > bValues[i]) {                                                                                 \
            return false;                                                                                              \
        }                                                                                                              \
    }                                                                                                                  \
    return true;                                                                                                       \
}

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(FIELD_BOOLEANean_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_i8_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_i16_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_i32_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_i64_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_u8_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_u16_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_u32_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_u64_t)

DEFINE_NG5_ARRAY_TYPE_LQ_FUNC(field_number_t)

static bool compare_encoded_string_array_less_eq_func(const void *lhs, const void *rhs, void *args)
{
        struct strdic *dic = (struct strdic *) args;
        struct vector ofType(field_sid_t) *a = (struct vector *) lhs;
        struct vector ofType(field_sid_t) *b = (struct vector *) rhs;
        const field_sid_t *aValues = vec_all(a, field_sid_t);
        const field_sid_t *bValues = vec_all(b, field_sid_t);
        size_t max_compare_idx = a->num_elems < b->num_elems ? a->num_elems : b->num_elems;
        for (size_t i = 0; i < max_compare_idx; i++) {
                char **aString = strdic_extract(dic, aValues + i, 1);
                char **bString = strdic_extract(dic, bValues + i, 1);
                bool greater = strcmp(*aString, *bString) > 0;
                strdic_free(dic, aString);
                strdic_free(dic, bString);
                if (greater) {
                        return false;
                }
        }
        return true;
}

static void sorted_nested_array_objects(struct columndoc_obj *columndoc)
{
        if (columndoc->parent->read_optimized) {
                for (size_t i = 0; i < columndoc->obj_array_props.num_elems; i++) {
                        struct columndoc_group
                                *array_columns = vec_get(&columndoc->obj_array_props, i, struct columndoc_group);
                        for (size_t j = 0; j < array_columns->columns.num_elems; j++) {
                                struct columndoc_column
                                        *column = vec_get(&array_columns->columns, j, struct columndoc_column);
                                struct vector ofType(u32) *array_indices = &column->array_positions;
                                struct vector ofType(struct vector ofType(<T>)) *values_for_indicies = &column->values;
                                assert (array_indices->num_elems == values_for_indicies->num_elems);

                                for (size_t k = 0; k < array_indices->num_elems; k++) {
                                        struct vector ofType(<T>)
                                                *values_for_index = vec_get(values_for_indicies, k, struct vector);
                                        if (column->type == FIELD_OBJECT) {
                                                for (size_t l = 0; l < values_for_index->num_elems; l++) {
                                                        struct columndoc_obj *nested_object =
                                                                vec_get(values_for_index, l, struct columndoc_obj);
                                                        sort_columndoc_entries(nested_object);
                                                }
                                        }
                                }
                        }
                }
        }
}

#define SORT_META_MODEL_VALUES(key_vector, value_vector, value_type, compareValueFunc)                                 \
{                                                                                                                      \
    size_t num_elements = vec_length(&key_vector);                                                              \
                                                                                                                       \
    if (num_elements > 0) {                                                                                            \
        size_t *value_indicies = malloc(sizeof(size_t) * num_elements);                                                \
        for (size_t i = 0; i < num_elements; i++) {                                                                    \
            value_indicies[i] = i;                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        struct vector ofType(field_sid_t) key_cpy;                                                               \
        struct vector ofType(value_type) value_cpy;                                                                     \
                                                                                                                       \
        vec_cpy(&key_cpy, &key_vector);                                                                         \
        vec_cpy(&value_cpy, &value_vector);                                                                     \
                                                                                                                       \
        value_type *values = vec_all(&value_cpy, value_type);                                                \
                                                                                                                       \
        sort_qsort_indicies(value_indicies, values, sizeof(value_type), compareValueFunc, num_elements,         \
                      key_vector.allocator);                                                                           \
                                                                                                                       \
        for (size_t i = 0; i < num_elements; i++) {                                                                    \
            vec_set(&key_vector, i, vec_get(&key_cpy, value_indicies[i], field_sid_t));        \
            vec_set(&value_vector, i, vec_get(&value_cpy, value_indicies[i], value_type));            \
        }                                                                                                              \
                                                                                                                       \
                                                                                                                       \
        free(value_indicies);                                                                                          \
        vec_drop(&key_cpy);                                                                                     \
        vec_drop(&value_cpy);                                                                                   \
    }                                                                                                                  \
}

static void sort_meta_model_string_values(struct vector ofType(field_sid_t) *key_vector,
        struct vector ofType(field_sid_t) *value_vector, struct strdic *dic)
{
        size_t num_elements = vec_length(key_vector);

        if (num_elements > 0) {
                size_t *value_indicies = malloc(sizeof(size_t) * num_elements);
                for (size_t i = 0; i < num_elements; i++) {
                        value_indicies[i] = i;
                }

                struct vector ofType(field_sid_t) key_cpy;
                struct vector ofType(field_sid_t) value_cpy;

                vec_cpy(&key_cpy, key_vector);
                vec_cpy(&value_cpy, value_vector);

                field_sid_t *values = vec_all(&value_cpy, field_sid_t);

                sort_qsort_indicies_wargs(value_indicies,
                        values,
                        sizeof(field_sid_t),
                        compare_encoded_string_less_eq_func,
                        num_elements,
                        key_vector->allocator,
                        dic);

                for (size_t i = 0; i < num_elements; i++) {
                        vec_set(key_vector, i, vec_get(&key_cpy, value_indicies[i], field_sid_t));
                        vec_set(value_vector, i, vec_get(&value_cpy, value_indicies[i], field_sid_t));
                }

                free(value_indicies);
                vec_drop(&key_cpy);
                vec_drop(&value_cpy);
        }
}

#define SORT_META_MODEL_ARRAYS(key_vector, value_array_vector, compare_func)                                           \
{                                                                                                                      \
    size_t num_elements = vec_length(&key_vector);                                                              \
                                                                                                                       \
    if (num_elements > 0) {                                                                                            \
        size_t *value_indicies = malloc(sizeof(size_t) * num_elements);                                                \
        for (size_t i = 0; i < num_elements; i++) {                                                                    \
            value_indicies[i] = i;                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        struct vector ofType(field_sid_t) key_cpy;                                                               \
        struct vector ofType(struct vector) value_cpy;                                                                   \
                                                                                                                       \
        vec_cpy(&key_cpy, &key_vector);                                                                         \
        vec_cpy(&value_cpy, &value_array_vector);                                                               \
                                                                                                                       \
        const struct vector *values = vec_all(&value_array_vector, struct vector);                             \
                                                                                                                       \
        sort_qsort_indicies(value_indicies, values, sizeof(struct vector), compare_func, num_elements,           \
                      key_vector.allocator);                                                                           \
                                                                                                                       \
        for (size_t i = 0; i < num_elements; i++) {                                                                    \
            vec_set(&key_vector, i, vec_get(&key_cpy, value_indicies[i], field_sid_t));        \
            vec_set(&value_array_vector, i, vec_get(&value_cpy, value_indicies[i], struct vector));    \
        }                                                                                                              \
                                                                                                                       \
        free(value_indicies);                                                                                          \
        vec_drop(&key_cpy);                                                                                     \
        vec_drop(&value_cpy);                                                                                   \
    }                                                                                                                  \
}

static void sort_columndoc_strings_arrays(struct vector ofType(field_sid_t) *key_vector,
        struct vector ofType(field_sid_t) *value_array_vector, struct strdic *dic)
{
        size_t num_elements = vec_length(key_vector);

        if (num_elements > 0) {
                size_t *value_indicies = malloc(sizeof(size_t) * num_elements);
                for (size_t i = 0; i < num_elements; i++) {
                        value_indicies[i] = i;
                }

                struct vector ofType(field_sid_t) key_cpy;
                struct vector ofType(struct vector) value_cpy;

                vec_cpy(&key_cpy, key_vector);
                vec_cpy(&value_cpy, value_array_vector);

                const struct vector *values = vec_all(value_array_vector, struct vector);

                sort_qsort_indicies_wargs(value_indicies,
                        values,
                        sizeof(struct vector),
                        compare_encoded_string_array_less_eq_func,
                        num_elements,
                        key_vector->allocator,
                        dic);

                for (size_t i = 0; i < num_elements; i++) {
                        vec_set(key_vector, i, vec_get(&key_cpy, value_indicies[i], field_sid_t));
                        vec_set(value_array_vector, i, vec_get(&value_cpy, value_indicies[i], struct vector));
                }

                free(value_indicies);
                vec_drop(&key_cpy);
                vec_drop(&value_cpy);
        }
}

static bool compare_object_array_key_columns_less_eq_func(const void *lhs, const void *rhs, void *args)
{
        struct strdic *dic = (struct strdic *) args;
        struct columndoc_group *a = (struct columndoc_group *) lhs;
        struct columndoc_group *b = (struct columndoc_group *) rhs;
        char **a_column_name = strdic_extract(dic, &a->key, 1);
        char **b_column_name = strdic_extract(dic, &b->key, 1);
        bool column_name_leq = strcmp(*a_column_name, *b_column_name) <= 0;
        strdic_free(dic, a_column_name);
        strdic_free(dic, b_column_name);
        return column_name_leq;
}

static bool compare_object_array_key_column_less_eq_func(const void *lhs, const void *rhs, void *args)
{
        struct strdic *dic = (struct strdic *) args;
        struct columndoc_column *a = (struct columndoc_column *) lhs;
        struct columndoc_column *b = (struct columndoc_column *) rhs;
        char **a_column_name = strdic_extract(dic, &a->key_name, 1);
        char **b_column_name = strdic_extract(dic, &b->key_name, 1);
        int cmpResult = strcmp(*a_column_name, *b_column_name);
        bool column_name_leq = cmpResult < 0 ? true : (cmpResult == 0 ? (a->type <= b->type) : false);
        strdic_free(dic, a_column_name);
        strdic_free(dic, b_column_name);
        return column_name_leq;
}

struct com_column_leq_arg {
        struct strdic *dic;
        field_e value_type;
};

#define ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, type, valueVectorAPtr, valueVectorBPtr)                                 \
{                                                                                                                      \
    for (size_t i = 0; i < max_num_elem; i++) {                                                                        \
        type o1 = *vec_get(valueVectorAPtr, i, type);                                                        \
        type o2 = *vec_get(valueVectorBPtr, i, type);                                                        \
        if (o1 > o2) {                                                                                                 \
            return false;                                                                                              \
        }                                                                                                              \
    }                                                                                                                  \
    return true;                                                                                                       \
}

static bool compare_column_less_eq_func(const void *lhs, const void *rhs, void *args)
{
        struct vector ofType(<T>) *a = (struct vector *) lhs;
        struct vector ofType(<T>) *b = (struct vector *) rhs;
        struct com_column_leq_arg *func_arg = (struct com_column_leq_arg *) args;

        size_t max_num_elem = ng5_min(a->num_elems, b->num_elems);

        switch (func_arg->value_type) {
        case FIELD_NULL:
                return (a->num_elems <= b->num_elems);
                break;
        case FIELD_BOOLEAN: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, FIELD_BOOLEANean_t, a, b);
                break;
        case FIELD_INT8: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_i8_t, a, b);
                break;
        case FIELD_INT16: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_i16_t, a, b);
                break;
        case FIELD_INT32: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_i32_t, a, b);
                break;
        case FIELD_INT64: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_i64_t, a, b);
                break;
        case FIELD_UINT8: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_u8_t, a, b);
                break;
        case FIELD_UINT16: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_u16_t, a, b);
                break;
        case FIELD_UINT32: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_u32_t, a, b);
                break;
        case FIELD_UINT64: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_u64_t, a, b);
                break;
        case FIELD_FLOAT: ARRAY_LEQ_PRIMITIVE_FUNC(max_num_elem, field_number_t, a, b);
                break;
        case FIELD_STRING:
                for (size_t i = 0; i < max_num_elem; i++) {
                        field_sid_t o1 = *vec_get(a, i, field_sid_t);
                        field_sid_t o2 = *vec_get(b, i, field_sid_t);
                        char **o1_string = strdic_extract(func_arg->dic, &o1, 1);
                        char **o2_string = strdic_extract(func_arg->dic, &o2, 1);
                        bool greater = strcmp(*o1_string, *o2_string) > 0;
                        strdic_free(func_arg->dic, o1_string);
                        strdic_free(func_arg->dic, o2_string);
                        if (greater) {
                                return false;
                        }
                }
                return true;
        case FIELD_OBJECT:
                return true;
                break;
        default: print_error_and_die(NG5_ERR_NOTYPE)
                return false;
        }
}

static void sort_columndoc_column(struct columndoc_column *column, struct strdic *dic)
{
        /** Sort column by its value, and re-arrange the array position list according this new order */
        struct vector ofType(u32) array_position_cpy;
        struct vector ofType(struct vector ofType(<T>)) values_cpy;

        vec_cpy(&array_position_cpy, &column->array_positions);
        vec_cpy(&values_cpy, &column->values);

        assert(column->array_positions.num_elems == column->values.num_elems);
        assert(array_position_cpy.num_elems == values_cpy.num_elems);
        assert(values_cpy.num_elems == column->array_positions.num_elems);

        size_t *indices = malloc(values_cpy.num_elems * sizeof(size_t));
        for (size_t i = 0; i < values_cpy.num_elems; i++) {
                indices[i] = i;
        }

        struct com_column_leq_arg func_arg = {.dic = dic, .value_type = column->type};

        sort_qsort_indicies_wargs(indices,
                values_cpy.base,
                values_cpy.elem_size,
                compare_column_less_eq_func,
                values_cpy.num_elems,
                values_cpy.allocator,
                &func_arg);

        for (size_t i = 0; i < values_cpy.num_elems; i++) {
                vec_set(&column->values, i, vec_at(&values_cpy, indices[i]));
                vec_set(&column->array_positions, i, vec_at(&array_position_cpy, indices[i]));
        }

        free(indices);
        vec_drop(&array_position_cpy);
        vec_drop(&values_cpy);
}

static void sort_columndoc_column_arrays(struct columndoc_obj *columndoc)
{
        struct vector ofType(struct columndoc_group) cpy;
        vec_cpy(&cpy, &columndoc->obj_array_props);
        size_t *indices = malloc(cpy.num_elems * sizeof(size_t));
        for (size_t i = 0; i < cpy.num_elems; i++) {
                indices[i] = i;
        }
        sort_qsort_indicies_wargs(indices,
                cpy.base,
                sizeof(struct columndoc_group),
                compare_object_array_key_columns_less_eq_func,
                cpy.num_elems,
                cpy.allocator,
                columndoc->parent->dic);
        for (size_t i = 0; i < cpy.num_elems; i++) {
                vec_set(&columndoc->obj_array_props, i, vec_get(&cpy, indices[i], struct columndoc_group));
        }
        free(indices);

        for (size_t i = 0; i < cpy.num_elems; i++) {
                struct columndoc_group *key_columns = vec_get(&columndoc->obj_array_props, i, struct columndoc_group);
                size_t *columnIndices = malloc(key_columns->columns.num_elems * sizeof(size_t));
                struct vector ofType(struct columndoc_column) columnCpy;
                vec_cpy(&columnCpy, &key_columns->columns);
                for (size_t i = 0; i < key_columns->columns.num_elems; i++) {
                        columnIndices[i] = i;
                }

                /** First, sort by column name; Then, sort by columns with same name by type */
                sort_qsort_indicies_wargs(columnIndices,
                        columnCpy.base,
                        sizeof(struct columndoc_column),
                        compare_object_array_key_column_less_eq_func,
                        key_columns->columns.num_elems,
                        key_columns->columns.allocator,
                        columndoc->parent->dic);
                for (size_t i = 0; i < key_columns->columns.num_elems; i++) {
                        vec_set(&key_columns->columns,
                                i,
                                vec_get(&columnCpy, columnIndices[i], struct columndoc_column));
                        struct columndoc_column *column = vec_get(&key_columns->columns, i, struct columndoc_column);
                        sort_columndoc_column(column, columndoc->parent->dic);
                }

                vec_drop(&columnCpy);
                free(columnIndices);
        }
        vec_drop(&cpy);
}

static void sort_columndoc_values(struct columndoc_obj *columndoc)
{
        if (columndoc->parent->read_optimized) {
                SORT_META_MODEL_VALUES(columndoc->bool_prop_keys,
                        columndoc->bool_prop_vals,
                        FIELD_BOOLEANean_t,
                        compare_FIELD_BOOLEANean_t_leq);
                SORT_META_MODEL_VALUES(columndoc->int8_prop_keys,
                        columndoc->int8_prop_vals,
                        field_i8_t,
                        compare_field_i8_t_leq);
                SORT_META_MODEL_VALUES(columndoc->int16_prop_keys,
                        columndoc->int16_prop_vals,
                        field_i16_t,
                        compare_field_i16_t_leq);
                SORT_META_MODEL_VALUES(columndoc->int32_prop_keys,
                        columndoc->int32_prop_vals,
                        field_i32_t,
                        compare_field_i32_t_leq);
                SORT_META_MODEL_VALUES(columndoc->int64_prop_keys,
                        columndoc->int64_prop_vals,
                        field_i64_t,
                        compare_field_i64_t_leq);
                SORT_META_MODEL_VALUES(columndoc->uint8_prop_keys,
                        columndoc->uint8_prop_vals,
                        field_u8_t,
                        compare_field_u8_t_leq);
                SORT_META_MODEL_VALUES(columndoc->uint16_prop_keys,
                        columndoc->uint16_prop_vals,
                        field_u16_t,
                        compare_field_u16_t_leq);
                SORT_META_MODEL_VALUES(columndoc->uin32_prop_keys,
                        columndoc->uint32_prop_vals,
                        field_u32_t,
                        compare_field_u32_t_leq);
                SORT_META_MODEL_VALUES(columndoc->uint64_prop_keys,
                        columndoc->uint64_prop_vals,
                        field_u64_t,
                        compare_field_u64_t_leq);
                SORT_META_MODEL_VALUES(columndoc->float_prop_keys,
                        columndoc->float_prop_vals,
                        field_number_t,
                        compare_field_number_t_leq);
                sort_meta_model_string_values(&columndoc->string_prop_keys,
                        &columndoc->string_prop_vals,
                        columndoc->parent->dic);

                SORT_META_MODEL_ARRAYS(columndoc->bool_array_prop_keys,
                        columndoc->bool_array_prop_vals,
                        compare_FIELD_BOOLEANean_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->int8_array_prop_keys,
                        columndoc->int8_array_prop_vals,
                        compare_field_i8_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->int16_array_prop_keys,
                        columndoc->int16_array_prop_vals,
                        compare_field_i16_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->int32_array_prop_keys,
                        columndoc->int32_array_prop_vals,
                        compare_field_i32_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->int64_array_prop_keys,
                        columndoc->int64_array_prop_vals,
                        compare_field_i64_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->uint8_array_prop_keys,
                        columndoc->uint8_array_prop_vals,
                        compare_field_u8_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->uint16_array_prop_keys,
                        columndoc->uint16_array_prop_vals,
                        compare_field_u16_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->uint32_array_prop_keys,
                        columndoc->uint32_array_prop_vals,
                        compare_field_u32_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->uint64_array_prop_keys,
                        columndoc->ui64_array_prop_vals,
                        compare_field_u64_t_array_leq);
                SORT_META_MODEL_ARRAYS(columndoc->float_array_prop_keys,
                        columndoc->float_array_prop_vals,
                        compare_field_number_t_array_leq);
                sort_columndoc_strings_arrays(&columndoc->string_array_prop_keys,
                        &columndoc->string_array_prop_vals,
                        columndoc->parent->dic);

                sort_columndoc_column_arrays(columndoc);
        }
}

static void sort_columndoc_entries(struct columndoc_obj *columndoc)
{
        if (columndoc->parent->read_optimized) {
                sort_columndoc_values(columndoc);
                sort_nested_primitive_object(columndoc);
                sorted_nested_array_objects(columndoc);
        }
}

struct columndoc *doc_entries_columndoc(const struct doc_bulk *bulk, const struct doc_entries *partition,
        bool read_optimized)
{
        if (!bulk || !partition) {
                return NULL;
        }

        // Step 1: encode all strings at once in a bulk
        char *const *key_strings = vec_all(&bulk->keys, char *);
        char *const *valueStrings = vec_all(&bulk->values, char *);
        strdic_insert(bulk->dic, NULL, key_strings, vec_length(&bulk->keys), 0);
        strdic_insert(bulk->dic, NULL, valueStrings, vec_length(&bulk->values), 0);

        // Step 2: for each document doc, create a meta doc, and construct a binary compressed document
        const struct doc *models = vec_all(&bulk->models, struct doc);
        assert (bulk->models.num_elems == 1);

        const struct doc *model = models;

        // TODO: DEBUG remove these lines
        // fprintf(stdout, "\nDocument Model:\n");
        // DocumentModelPrint(stdout, doc);

        struct columndoc *columndoc = malloc(sizeof(struct columndoc));
        columndoc->read_optimized = read_optimized;
        struct err err;
        if (!columndoc_create(columndoc, &err, model, bulk, partition, bulk->dic)) {
                error_print_and_abort(&err);
        }

        if (columndoc->read_optimized) {
                sort_columndoc_entries(&columndoc->columndoc);
        }


        //  fprintf(stdout, "\nDocument Meta Model:\n");
        //  DocumentMetaModelPrint(stdout, columndoc);

        return columndoc;
}

NG5_EXPORT(bool) doc_entries_drop(struct doc_entries *partition)
{
        ng5_unused(partition);
        return true;
}

static void create_doc(struct doc_obj *model, struct doc *doc)
{
        vec_create(&model->entries, NULL, sizeof(struct doc_entries), 50);
        model->doc = doc;
}

static void create_typed_vector(struct doc_entries *entry)
{
        size_t size;
        switch (entry->type) {
        case FIELD_NULL:
                size = sizeof(FIELD_NULL_t);
                break;
        case FIELD_BOOLEAN:
                size = sizeof(FIELD_BOOLEANean_t);
                break;
        case FIELD_INT8:
                size = sizeof(field_i8_t);
                break;
        case FIELD_INT16:
                size = sizeof(field_i16_t);
                break;
        case FIELD_INT32:
                size = sizeof(field_i32_t);
                break;
        case FIELD_INT64:
                size = sizeof(field_i64_t);
                break;
        case FIELD_UINT8:
                size = sizeof(field_u8_t);
                break;
        case FIELD_UINT16:
                size = sizeof(field_u16_t);
                break;
        case FIELD_UINT32:
                size = sizeof(field_u32_t);
                break;
        case FIELD_UINT64:
                size = sizeof(field_u64_t);
                break;
        case FIELD_FLOAT:
                size = sizeof(field_number_t);
                break;
        case FIELD_STRING:
                size = sizeof(FIELD_STRING_t);
                break;
        case FIELD_OBJECT:
                size = sizeof(struct doc_obj);
                break;
        default: print_error_and_die(NG5_ERR_INTERNALERR) /** unknown type */
                return;
        }
        vec_create(&entry->values, NULL, size, 50);
}

static void entries_drop(struct doc_entries *entry)
{
        if (entry->type == FIELD_OBJECT) {
                for (size_t i = 0; i < entry->values.num_elems; i++) {
                        struct doc_obj *model = vec_get(&entry->values, i, struct doc_obj);
                        doc_drop(model);
                }
        }
        vec_drop(&entry->values);
}

static bool print_value(FILE *file, field_e type, const struct vector ofType(<T>) *values)
{
        size_t num_values = values->num_elems;
        if (num_values == 0) {
                fprintf(file, "null");
                return true;
        }
        if (num_values > 1) {
                fprintf(file, "[");
        }
        switch (type) {
        case FIELD_NULL: {
                for (size_t i = 0; i < num_values; i++) {
                        fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                }
        }
                break;
        case FIELD_BOOLEAN: {
                for (size_t i = 0; i < num_values; i++) {
                        FIELD_BOOLEANean_t value = *(vec_get(values, i, FIELD_BOOLEANean_t));
                        if (value != NG5_NULL_BOOLEAN) {
                                fprintf(file, "%s%s", value == 0 ? "false" : "true", i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_INT8: {
                for (size_t i = 0; i < num_values; i++) {
                        field_i8_t value = *(vec_get(values, i, field_i8_t));
                        if (value != NG5_NULL_INT8) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_INT16: {
                for (size_t i = 0; i < num_values; i++) {
                        field_i16_t value = *(vec_get(values, i, field_i16_t));
                        if (value != NG5_NULL_INT16) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_INT32: {
                for (size_t i = 0; i < num_values; i++) {
                        field_i32_t value = *(vec_get(values, i, field_i32_t));
                        if (value != NG5_NULL_INT32) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_INT64: {
                for (size_t i = 0; i < num_values; i++) {
                        field_i64_t value = *(vec_get(values, i, field_i64_t));
                        if (value != NG5_NULL_INT64) {
                                fprintf(file, "%" PRIi64 "%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_UINT8: {
                for (size_t i = 0; i < num_values; i++) {
                        field_u8_t value = *(vec_get(values, i, field_u8_t));
                        if (value != NG5_NULL_UINT8) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_UINT16: {
                for (size_t i = 0; i < num_values; i++) {
                        field_u16_t value = *(vec_get(values, i, field_u16_t));
                        if (value != NG5_NULL_UINT16) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_UINT32: {
                for (size_t i = 0; i < num_values; i++) {
                        field_u32_t value = *(vec_get(values, i, field_u32_t));
                        if (value != NG5_NULL_UINT32) {
                                fprintf(file, "%d%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_UINT64: {
                for (size_t i = 0; i < num_values; i++) {
                        field_u64_t value = *(vec_get(values, i, field_u64_t));
                        if (value != NG5_NULL_UINT64) {
                                fprintf(file, "%" PRIu64 "%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_FLOAT: {
                for (size_t i = 0; i < num_values; i++) {
                        field_number_t value = *(vec_get(values, i, field_number_t));
                        if (!isnan(value)) {
                                fprintf(file, "%f%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_STRING: {
                for (size_t i = 0; i < num_values; i++) {
                        FIELD_STRING_t value = *(vec_get(values, i, FIELD_STRING_t));
                        if (value) {
                                fprintf(file, "\"%s\"%s", value, i + 1 < num_values ? ", " : "");
                        } else {
                                fprintf(file, "null%s", i + 1 < num_values ? ", " : "");
                        }
                }
        }
                break;
        case FIELD_OBJECT: {
                for (size_t i = 0; i < num_values; i++) {
                        struct doc_obj *obj = vec_get(values, i, struct doc_obj);
                        if (!NG5_NULL_OBJECT_MODEL(obj)) {
                                print_object(file, obj);
                        } else {
                                fprintf(file, "null");
                        }
                        fprintf(file, "%s", i + 1 < num_values ? ", " : "");
                }
        }
                break;
        default: NG5_NOT_IMPLEMENTED;
        }
        if (num_values > 1) {
                fprintf(file, "]");
        }
        return true;
}

static void print_object(FILE *file, const struct doc_obj *model)
{
        fprintf(file, "{");
        for (size_t i = 0; i < model->entries.num_elems; i++) {
                struct doc_entries *entry = vec_get(&model->entries, i, struct doc_entries);
                fprintf(file, "\"%s\": ", entry->key);
                print_value(file, entry->type, &entry->values);
                fprintf(file, "%s", i + 1 < model->entries.num_elems ? ", " : "");
        }
        fprintf(file, "}");
}

