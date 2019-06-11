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

#ifndef NG5_JSON_H
#define NG5_JSON_H

#include "shared/common.h"
#include "std/vec.h"

NG5_BEGIN_DECL

/** forwarded */

struct json;

struct json_element;

struct json_object_t;

struct json_array;

struct json_string;

struct json_number;

struct json_members;

struct json_prop;

struct json_elements;

enum json_token_type {
        OBJECT_OPEN,
        OBJECT_CLOSE,
        LITERAL_STRING,
        LITERAL_INT,
        LITERAL_FLOAT,
        LITERAL_TRUE,
        LITERAL_FALSE,
        LITERAL_NULL,
        COMMA,
        ASSIGN,
        ARRAY_OPEN,
        ARRAY_CLOSE,
        JSON_UNKNOWN
};

struct json_token {
        enum json_token_type type;
        const char *string;
        unsigned line;
        unsigned column;
        unsigned length;
};

struct json_err {
        const struct json_token *token;
        const char *token_type_str;
        const char *msg;
};

struct json_tokenizer {
        const char *cursor;
        struct json_token token;
        struct err err;
};

struct json_parser {
        struct json_tokenizer tokenizer;
        struct doc_bulk *partition;
        struct err err;
};

enum json_parent {
        JSON_PARENT_OBJECT, JSON_PARENT_MEMBER, JSON_PARENT_ELEMENTS
};

enum json_value_type {
        JSON_VALUE_OBJECT,
        JSON_VALUE_ARRAY,
        JSON_VALUE_STRING,
        JSON_VALUE_NUMBER,
        JSON_VALUE_TRUE,
        JSON_VALUE_FALSE,
        JSON_VALUE_NULL
};

struct json {
        struct json_element *element;
        struct err err;
};

struct json_node_value {
        struct json_element *parent;

        enum json_value_type value_type;

        union {
                struct json_object_t *object;
                struct json_array *array;
                struct json_string *string;
                struct json_number *number;
                void *ptr;
        } value;
};

struct json_object_t {
        struct json_node_value *parent;
        struct json_members *value;
};

struct json_element {
        enum json_parent parent_type;

        union {
                struct json *json;
                struct json_prop *member;
                struct json_elements *elements;
                void *ptr;
        } parent;

        struct json_node_value value;

};

struct json_string {
        struct json_prop *parent;
        char *value;
};

struct json_prop {
        struct json_members *parent;
        struct json_string key;
        struct json_element value;
};

struct json_members {
        struct json_object_t *parent;
        struct vector ofType(struct json_prop) members;
};

struct json_elements {
        struct json_array *parent;
        struct vector ofType(struct json_element) elements;
};

struct json_array {
        struct json_node_value *parent;
        struct json_elements elements;
};

enum json_number_type {
        JSON_NUMBER_FLOAT, JSON_NUMBER_UNSIGNED, JSON_NUMBER_SIGNED
};

struct json_number {
        struct json_node_value *parent;
        enum json_number_type value_type;

        union {
                float float_number;
                i64 signed_integer;
                u64 unsigned_integer;
        } value;
};

NG5_EXPORT(bool) json_tokenizer_init(struct json_tokenizer *tokenizer, const char *input);

NG5_EXPORT(const struct json_token *)json_tokenizer_next(struct json_tokenizer *tokenizer);

NG5_EXPORT(void) json_token_dup(struct json_token *dst, const struct json_token *src);

NG5_EXPORT(void) json_token_print(FILE *file, const struct json_token *token);

NG5_EXPORT(bool) json_parser_create(struct json_parser *parser, struct doc_bulk *partition);

NG5_EXPORT(bool) json_parse(struct json *json, struct json_err *error_desc, struct json_parser *parser,
        const char *input);

NG5_EXPORT(bool) json_test(struct err *err, struct json *json);

NG5_EXPORT(bool) json_drop(struct json *json);

NG5_EXPORT(bool) json_print(FILE *file, struct json *json);

NG5_DEFINE_GET_ERROR_FUNCTION(json, struct json, json);

NG5_END_DECL

#endif