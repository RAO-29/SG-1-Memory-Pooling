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

#include <inttypes.h>
#include <ctype.h>
#include <locale.h>
#include "json/json.h"
#include "json/doc.h"
#include "utils/convert.h"

static struct {
        enum json_token_type token;
        const char *string;
} TOKEN_STRING[] = {{.token = OBJECT_OPEN, .string = "OBJECT_OPEN"}, {.token = OBJECT_CLOSE, .string = "OBJECT_CLOSE"},
                    {.token = LITERAL_STRING, .string = "JSON_TOKEN_STRING"},
                    {.token = LITERAL_INT, .string = "LITERAL_INT"},
                    {.token = LITERAL_FLOAT, .string = "LITERAL_FLOAT"},
                    {.token = LITERAL_TRUE, .string = "LITERAL_TRUE"},
                    {.token = LITERAL_FALSE, .string = "LITERAL_FALSE"},
                    {.token = LITERAL_NULL, .string = "LITERAL_NULL"}, {.token = COMMA, .string = "COMMA"},
                    {.token = ASSIGN, .string = "JSON_TOKEN_ASSIGMENT"}, {.token = ARRAY_OPEN, .string = "ARRAY_OPEN"},
                    {.token = ARRAY_CLOSE, .string = "ARRAY_CLOSE"}, {.token = JSON_UNKNOWN, .string = "JSON_UNKNOWN"}};

struct token_memory {
        enum json_token_type type;
        bool init;
};

static int process_token(struct err *err, struct json_err *error_desc, const struct json_token *token,
        struct vector ofType(enum json_token_type) *brackets, struct token_memory *token_mem);

static int set_error(struct json_err *error_desc, const struct json_token *token, const char *msg);

NG5_EXPORT(bool) json_tokenizer_init(struct json_tokenizer *tokenizer, const char *input)
{
        error_if_null(tokenizer)
        error_if_null(input)
        tokenizer->cursor = input;
        tokenizer->token =
                (struct json_token) {.type = JSON_UNKNOWN, .length = 0, .column = 0, .line = 1, .string = NULL};
        error_init(&tokenizer->err);
        return true;
}

const struct json_token *json_tokenizer_next(struct json_tokenizer *tokenizer)
{
        if (likely(*tokenizer->cursor != '\0')) {
                char c = *tokenizer->cursor;
                tokenizer->token.string = tokenizer->cursor;
                tokenizer->token.column += tokenizer->token.length;
                tokenizer->token.length = 0;
                if (c == '\n' || c == '\r') {
                        tokenizer->token.line += c == '\n' ? 1 : 0;
                        tokenizer->token.column = c == '\n' ? 0 : tokenizer->token.column;
                        tokenizer->cursor++;
                        return json_tokenizer_next(tokenizer);
                } else if (isspace(c)) {
                        do {
                                tokenizer->cursor++;
                                tokenizer->token.column++;
                        }
                        while (isspace(c = *tokenizer->cursor) && c != '\n');
                        return json_tokenizer_next(tokenizer);
                } else if (c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',') {
                        tokenizer->token.type =
                                c == '{' ? OBJECT_OPEN : c == '}' ? OBJECT_CLOSE : c == '[' ? ARRAY_OPEN : c == ']'
                                                                                                           ? ARRAY_CLOSE
                                                                                                           : c == ':'
                                                                                                             ? ASSIGN
                                                                                                             : COMMA;
                        tokenizer->token.column++;
                        tokenizer->token.length = 1;
                        tokenizer->cursor++;
                } else if (c == '"') {
                        bool escapeQuote = false;
                        tokenizer->token.type = LITERAL_STRING;
                        tokenizer->token.string++;
                        tokenizer->token.column++;
                        char last_1_c = '\0', last_2_c = '\0', last_3_c = '\0', last_4_c = '\0';
                        c = *(++tokenizer->cursor);
                        while ((escapeQuote || c != '"') && c != '\r' && c != '\n') {
                                next_char:
                                tokenizer->token.length++;
                                last_4_c = last_3_c;
                                last_3_c = last_2_c;
                                last_2_c = last_1_c;
                                last_1_c = c;
                                c = *(++tokenizer->cursor);
                                if (unlikely(c == '\\' && last_1_c == '\\')) {
                                        goto next_char;
                                }
                                escapeQuote = c == '"' && last_1_c == '\\'
                                        && ((last_2_c == '\\' && last_3_c == '\\' && last_4_c != '\\')
                                                || (last_2_c != '\\' && last_3_c == '\\')
                                                || (last_2_c != '\\' && last_3_c != '\\'));
                        }
                        tokenizer->cursor++;
                        tokenizer->cursor += (c == '\r' || c == '\n') ? 1 : 0;
                } else if (c == 't' || c == 'f' || c == 'n') {
                        const unsigned lenTrueNull = 4;
                        const unsigned lenFalse = 5;
                        const unsigned cursorLen = strlen(tokenizer->cursor);
                        if (cursorLen > lenTrueNull && strncmp(tokenizer->cursor, "true", lenTrueNull) == 0) {
                                tokenizer->token.type = LITERAL_TRUE;
                                tokenizer->token.length = lenTrueNull;
                        } else if (cursorLen > lenFalse && strncmp(tokenizer->cursor, "false", lenFalse) == 0) {
                                tokenizer->token.type = LITERAL_FALSE;
                                tokenizer->token.length = lenFalse;
                        } else if (cursorLen > lenTrueNull && strncmp(tokenizer->cursor, "null", lenTrueNull) == 0) {
                                tokenizer->token.type = LITERAL_NULL;
                                tokenizer->token.length = lenTrueNull;
                        } else {
                                goto caseTokenUnknown;
                        }
                        tokenizer->token.column++;
                        tokenizer->cursor += tokenizer->token.length;
                } else if (c == '-' || isdigit(c)) {
                        unsigned fracFound = 0, expFound = 0, plusMinusFound = 0;
                        bool plusMinusAllowed = false;
                        bool onlyDigitsAllowed = false;
                        do {
                                onlyDigitsAllowed |= plusMinusAllowed;
                                plusMinusAllowed = (expFound == 1);
                                c = *(++tokenizer->cursor);
                                fracFound += c == '.';
                                expFound += (c == 'e') || (c == 'E');
                                plusMinusFound += plusMinusAllowed && ((c == '+') || (c == '-')) ? 1 : 0;
                                tokenizer->token.length++;
                        }
                        while ((((isdigit(c)) || (c == '.' && fracFound <= 1)
                                || (plusMinusAllowed && (plusMinusFound <= 1) && ((c == '+') || (c == '-')))
                                || ((c == 'e' || c == 'E') && expFound <= 1))) && c != '\n' && c != '\r');

                        if (!isdigit(*(tokenizer->cursor - 1))) {
                                tokenizer->token.column -= tokenizer->token.length;
                                goto caseTokenUnknown;
                        }
                        tokenizer->cursor += (c == '\r' || c == '\n') ? 1 : 0;
                        tokenizer->token.type = fracFound ? LITERAL_FLOAT : LITERAL_INT;
                } else {
                        caseTokenUnknown:
                        tokenizer->token.type = JSON_UNKNOWN;
                        tokenizer->token.column++;
                        tokenizer->token.length = strlen(tokenizer->cursor);
                        tokenizer->cursor += tokenizer->token.length;
                }
                return &tokenizer->token;
        } else {
                return NULL;
        }
}

void json_token_dup(struct json_token *dst, const struct json_token *src)
{
        assert(dst);
        assert(src);
        memcpy(dst, src, sizeof(struct json_token));
}

void json_token_print(FILE *file, const struct json_token *token)
{
        char *string = malloc(token->length + 1);
        strncpy(string, token->string, token->length);
        string[token->length] = '\0';
        fprintf(file,
                "{\"type\": \"%s\", \"line\": %d, \"column\": %d, \"length\": %d, \"text\": \"%s\"}",
                TOKEN_STRING[token->type].string,
                token->line,
                token->column,
                token->length,
                string);
        free(string);
}

static bool parse_object(struct json_object_t *object, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx);
static bool parse_array(struct json_array *array, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx);
static void parse_string(struct json_string *string, struct vector ofType(struct json_token) *token_stream,
        size_t *token_idx);
static void parse_number(struct json_number *number, struct vector ofType(struct json_token) *token_stream,
        size_t *token_idx);
static bool parse_element(struct json_element *element, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx);
static bool parse_elements(struct json_elements *elements, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx);
static bool parse_token_stream(struct json *json, struct err *err,
        struct vector ofType(struct json_token) *token_stream);
static struct json_token get_token(struct vector ofType(struct json_token) *token_stream, size_t token_idx);
static void connect_child_and_parents_member(struct json_prop *member);
static void connect_child_and_parents_object(struct json_object_t *object);
static void connect_child_and_parents_array(struct json_array *array);
static void connect_child_and_parents_value(struct json_node_value *value);
static void connect_child_and_parents_element(struct json_element *element);
static void connect_child_and_parents(struct json *json);
static bool json_ast_node_member_print(FILE *file, struct err *err, struct json_prop *member);
static bool json_ast_node_object_print(FILE *file, struct err *err, struct json_object_t *object);
static bool json_ast_node_array_print(FILE *file, struct err *err, struct json_array *array);
static void json_ast_node_string_print(FILE *file, struct json_string *string);
static bool json_ast_node_number_print(FILE *file, struct err *err, struct json_number *number);
static bool json_ast_node_value_print(FILE *file, struct err *err, struct json_node_value *value);
static bool json_ast_node_element_print(FILE *file, struct err *err, struct json_element *element);

#define NEXT_TOKEN(x) { *x = *x + 1; }
#define PREV_TOKEN(x) { *x = *x - 1; }

NG5_EXPORT(bool) json_parser_create(struct json_parser *parser, struct doc_bulk *partition)
{
        error_if_null(parser)
        error_if_null(partition)

        parser->partition = partition;
        error_init(&parser->err);

        return true;
}

bool json_parse(struct json *json, struct json_err *error_desc, struct json_parser *parser, const char *input)
{
        error_if_null(parser)
        error_if_null(input)

        struct vector ofType(enum json_token_type) brackets;
        struct vector ofType(struct json_token) token_stream;

        struct json retval = {.element = malloc(sizeof(struct json_element))};
        error_init(&retval.err);
        const struct json_token *token;
        int status;

        json_tokenizer_init(&parser->tokenizer, input);
        vec_create(&brackets, NULL, sizeof(enum json_token_type), 15);
        vec_create(&token_stream, NULL, sizeof(struct json_token), 200);

        struct token_memory token_mem = {.init = true, .type = JSON_UNKNOWN};

        while ((token = json_tokenizer_next(&parser->tokenizer))) {
                if (likely(
                        (status = process_token(&parser->err, error_desc, token, &brackets, &token_mem)) == true)) {
                        struct json_token *newToken = vec_new_and_get(&token_stream, struct json_token);
                        json_token_dup(newToken, token);
                } else {
                        goto cleanup;
                }
        }
        if (!vec_is_empty(&brackets)) {
                enum json_token_type type = *VECTOR_PEEK(&brackets, enum json_token_type);
                char buffer[1024];
                sprintf(&buffer[0],
                        "Unexpected end of file: missing '%s' to match unclosed '%s' (if any)",
                        type == OBJECT_OPEN ? "}" : "]",
                        type == OBJECT_OPEN ? "{" : "[");
                status = set_error(error_desc, token, &buffer[0]);
                goto cleanup;
        }

        if (!parse_token_stream(&retval, &parser->err, &token_stream)) {
                return false;
        }

        ng5_optional_set_or_else(json, retval, json_drop(json));
        status = true;

        cleanup:
        vec_drop(&brackets);
        vec_drop(&token_stream);
        return status;
}

bool test_condition_value(struct err *err, struct json_node_value *value)
{
        switch (value->value_type) {
        case JSON_VALUE_OBJECT:
                for (size_t i = 0; i < value->value.object->value->members.num_elems; i++) {
                        struct json_prop *member = vec_get(&value->value.object->value->members, i, struct json_prop);
                        if (!test_condition_value(err, &member->value.value)) {
                                return false;
                        }
                }
                break;
        case JSON_VALUE_ARRAY: {
                struct json_elements *elements = &value->value.array->elements;
                enum json_value_type value_type = JSON_VALUE_NULL;

                for (size_t i = 0; i < elements->elements.num_elems; i++) {
                        struct json_element *element = vec_get(&elements->elements, i, struct json_element);
                        value_type =
                                ((i == 0 || value_type == JSON_VALUE_NULL) ? element->value.value_type : value_type);

                        /** Test "All elements in array of same type" condition */
                        if ((element->value.value_type != JSON_VALUE_NULL) && (value_type == JSON_VALUE_TRUE
                                && (element->value.value_type != JSON_VALUE_TRUE
                                        || element->value.value_type != JSON_VALUE_FALSE))
                                && (value_type == JSON_VALUE_FALSE && (element->value.value_type != JSON_VALUE_TRUE
                                        || element->value.value_type != JSON_VALUE_FALSE))
                                && ((value_type != JSON_VALUE_TRUE && value_type != JSON_VALUE_FALSE)
                                        && value_type != element->value.value_type)) {
                                char message[] = "JSON file constraint broken: arrays of mixed types detected";
                                char *result = malloc(strlen(message) + 1);
                                strcpy(result, &message[0]);
                                error_with_details(err, NG5_ERR_ARRAYOFMIXEDTYPES, result);
                                free(result);
                                return false;
                        }

                        switch (element->value.value_type) {
                        case JSON_VALUE_OBJECT: {
                                struct json_object_t *object = element->value.value.object;
                                for (size_t i = 0; i < object->value->members.num_elems; i++) {
                                        struct json_prop
                                                *member = vec_get(&object->value->members, i, struct json_prop);
                                        if (!test_condition_value(err, &member->value.value)) {
                                                return false;
                                        }
                                }
                        }
                                break;
                        case JSON_VALUE_ARRAY: {/** Test "No Array of Arrays" condition */
                                char message[] = "JSON file constraint broken: arrays of arrays detected";
                                char *result = malloc(strlen(message) + 1);
                                strcpy(result, &message[0]);
                                error_with_details(err, NG5_ERR_ARRAYOFARRAYS, result);
                                free(result);
                                return false;
                        }
                        default:
                                break;
                        }
                }
        }
                break;
        default:
                break;
        }
        return true;
}

NG5_EXPORT(bool) json_test(struct err *err, struct json *json)
{
        return (test_condition_value(err, &json->element->value));
}

static struct json_token get_token(struct vector ofType(struct json_token) *token_stream, size_t token_idx)
{
        return *(struct json_token *) vec_at(token_stream, token_idx);
}

bool parse_members(struct err *err, struct json_members *members, struct vector ofType(struct json_token) *token_stream,
        size_t *token_idx)
{
        vec_create(&members->members, NULL, sizeof(struct json_prop), 20);
        struct json_token delimiter_token;

        do {
                struct json_prop *member = vec_new_and_get(&members->members, struct json_prop);
                struct json_token keyNameToken = get_token(token_stream, *token_idx);

                member->key.value = malloc(keyNameToken.length + 1);
                strncpy(member->key.value, keyNameToken.string, keyNameToken.length);
                member->key.value[keyNameToken.length] = '\0';

                NEXT_TOKEN(token_idx); /** skip assignment token */
                NEXT_TOKEN(token_idx);
                struct json_token valueToken = get_token(token_stream, *token_idx);

                switch (valueToken.type) {
                case OBJECT_OPEN:
                        member->value.value.value_type = JSON_VALUE_OBJECT;
                        member->value.value.value.object = malloc(sizeof(struct json_object_t));
                        if (!parse_object(member->value.value.value.object, err, token_stream, token_idx)) {
                                return false;
                        }
                        break;
                case ARRAY_OPEN:
                        member->value.value.value_type = JSON_VALUE_ARRAY;
                        member->value.value.value.array = malloc(sizeof(struct json_array));
                        if (!parse_array(member->value.value.value.array, err, token_stream, token_idx)) {
                                return false;
                        }
                        break;
                case LITERAL_STRING:
                        member->value.value.value_type = JSON_VALUE_STRING;
                        member->value.value.value.string = malloc(sizeof(struct json_string));
                        parse_string(member->value.value.value.string, token_stream, token_idx);
                        break;
                case LITERAL_INT:
                case LITERAL_FLOAT:
                        member->value.value.value_type = JSON_VALUE_NUMBER;
                        member->value.value.value.number = malloc(sizeof(struct json_number));
                        parse_number(member->value.value.value.number, token_stream, token_idx);
                        break;
                case LITERAL_TRUE:
                        member->value.value.value_type = JSON_VALUE_TRUE;
                        NEXT_TOKEN(token_idx);
                        break;
                case LITERAL_FALSE:
                        member->value.value.value_type = JSON_VALUE_FALSE;
                        NEXT_TOKEN(token_idx);
                        break;
                case LITERAL_NULL:
                        member->value.value.value_type = JSON_VALUE_NULL;
                        NEXT_TOKEN(token_idx);
                        break;
                default: error(err, NG5_ERR_PARSETYPE)
                        return false;
                }

                delimiter_token = get_token(token_stream, *token_idx);
                NEXT_TOKEN(token_idx);
        }
        while (delimiter_token.type == COMMA);
        PREV_TOKEN(token_idx);
        return true;
}

static bool parse_object(struct json_object_t *object, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx)
{
        assert(get_token(token_stream, *token_idx).type == OBJECT_OPEN);
        NEXT_TOKEN(token_idx);  /** Skip '{' */
        object->value = malloc(sizeof(struct json_members));

        /** test whether this is an empty object */
        struct json_token token = get_token(token_stream, *token_idx);

        if (token.type != OBJECT_CLOSE) {
                if (!parse_members(err, object->value, token_stream, token_idx)) {
                        return false;
                }
        } else {
                vec_create(&object->value->members, NULL, sizeof(struct json_prop), 20);
        }

        NEXT_TOKEN(token_idx);  /** Skip '}' */
        return true;
}

static bool parse_array(struct json_array *array, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx)
{
        struct json_token token = get_token(token_stream, *token_idx);
        ng5_unused(token);
        assert(token.type == ARRAY_OPEN);
        NEXT_TOKEN(token_idx); /** Skip '[' */

        vec_create(&array->elements.elements, NULL, sizeof(struct json_element), 250);
        if (!parse_elements(&array->elements, err, token_stream, token_idx)) {
                return false;
        }

        NEXT_TOKEN(token_idx); /** Skip ']' */
        return true;
}

static void parse_string(struct json_string *string, struct vector ofType(struct json_token) *token_stream,
        size_t *token_idx)
{
        struct json_token token = get_token(token_stream, *token_idx);
        assert(token.type == LITERAL_STRING);

        string->value = malloc(token.length + 1);
        if (likely(token.length > 0)) {
                strncpy(string->value, token.string, token.length);
        }
        string->value[token.length] = '\0';
        NEXT_TOKEN(token_idx);
}

static void parse_number(struct json_number *number, struct vector ofType(struct json_token) *token_stream,
        size_t *token_idx)
{
        struct json_token token = get_token(token_stream, *token_idx);
        assert(token.type == LITERAL_FLOAT || token.type == LITERAL_INT);

        char *value = malloc(token.length + 1);
        strncpy(value, token.string, token.length);
        value[token.length] = '\0';

        if (token.type == LITERAL_INT) {
                i64 assumeSigned = convert_atoi64(value);
                if (value[0] == '-') {
                        number->value_type = JSON_NUMBER_SIGNED;
                        number->value.signed_integer = assumeSigned;
                } else {
                        u64 assumeUnsigned = convert_atoiu64(value);
                        if (assumeUnsigned > (u64) assumeSigned) {
                                number->value_type = JSON_NUMBER_UNSIGNED;
                                number->value.unsigned_integer = assumeUnsigned;
                        } else {
                                number->value_type = JSON_NUMBER_SIGNED;
                                number->value.signed_integer = assumeSigned;
                        }
                }
        } else {
                number->value_type = JSON_NUMBER_FLOAT;
                setlocale(LC_ALL | ~LC_NUMERIC, "");
                number->value.float_number = strtof(value, NULL);
        }

        free(value);
        NEXT_TOKEN(token_idx);
}

static bool parse_element(struct json_element *element, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx)
{
        struct json_token token = get_token(token_stream, *token_idx);

        if (token.type == OBJECT_OPEN) { /** Parse object */
                element->value.value_type = JSON_VALUE_OBJECT;
                element->value.value.object = malloc(sizeof(struct json_object_t));
                if (!parse_object(element->value.value.object, err, token_stream, token_idx)) {
                        return false;
                }
        } else if (token.type == ARRAY_OPEN) { /** Parse array */
                element->value.value_type = JSON_VALUE_ARRAY;
                element->value.value.array = malloc(sizeof(struct json_array));
                if (!parse_array(element->value.value.array, err, token_stream, token_idx)) {
                        return false;
                }
        } else if (token.type == LITERAL_STRING) { /** Parse string */
                element->value.value_type = JSON_VALUE_STRING;
                element->value.value.string = malloc(sizeof(struct json_string));
                parse_string(element->value.value.string, token_stream, token_idx);
        } else if (token.type == LITERAL_FLOAT || token.type == LITERAL_INT) { /** Parse number */
                element->value.value_type = JSON_VALUE_NUMBER;
                element->value.value.number = malloc(sizeof(struct json_number));
                parse_number(element->value.value.number, token_stream, token_idx);
        } else if (token.type == LITERAL_TRUE) {
                element->value.value_type = JSON_VALUE_TRUE;
                NEXT_TOKEN(token_idx);
        } else if (token.type == LITERAL_FALSE) {
                element->value.value_type = JSON_VALUE_FALSE;
                NEXT_TOKEN(token_idx);
        } else if (token.type == LITERAL_NULL) {
                element->value.value_type = JSON_VALUE_NULL;
                NEXT_TOKEN(token_idx);
        } else {
                element->value.value_type = JSON_VALUE_NULL;
        }
        return true;
}

static bool parse_elements(struct json_elements *elements, struct err *err,
        struct vector ofType(struct json_token) *token_stream, size_t *token_idx)
{
        struct json_token delimiter;
        do {
                if (!parse_element(vec_new_and_get(&elements->elements, struct json_element),
                        err,
                        token_stream,
                        token_idx)) {
                        return false;
                }
                delimiter = get_token(token_stream, *token_idx);
                NEXT_TOKEN(token_idx);
        }
        while (delimiter.type == COMMA);
        PREV_TOKEN(token_idx);
        return true;
}

static bool parse_token_stream(struct json *json, struct err *err,
        struct vector ofType(struct json_token) *token_stream)
{
        size_t token_idx = 0;
        if (!parse_element(json->element, err, token_stream, &token_idx)) {
                return false;
        }
        connect_child_and_parents(json);
        return true;
}

static void connect_child_and_parents_member(struct json_prop *member)
{
        connect_child_and_parents_element(&member->value);
}

static void connect_child_and_parents_object(struct json_object_t *object)
{
        object->value->parent = object;
        for (size_t i = 0; i < object->value->members.num_elems; i++) {
                struct json_prop *member = vec_get(&object->value->members, i, struct json_prop);
                member->parent = object->value;

                member->key.parent = member;

                member->value.parent_type = JSON_PARENT_MEMBER;
                member->value.parent.member = member;

                connect_child_and_parents_member(member);
        }
}

static void connect_child_and_parents_array(struct json_array *array)
{
        array->elements.parent = array;
        for (size_t i = 0; i < array->elements.elements.num_elems; i++) {
                struct json_element *element = vec_get(&array->elements.elements, i, struct json_element);
                element->parent_type = JSON_PARENT_ELEMENTS;
                element->parent.elements = &array->elements;
                connect_child_and_parents_element(element);
        }
}

static void connect_child_and_parents_value(struct json_node_value *value)
{
        switch (value->value_type) {
        case JSON_VALUE_OBJECT:
                connect_child_and_parents_object(value->value.object);
                break;
        case JSON_VALUE_ARRAY:
                connect_child_and_parents_array(value->value.array);
                break;
        default:
                break;
        }
}

static void connect_child_and_parents_element(struct json_element *element)
{
        element->value.parent = element;
        connect_child_and_parents_value(&element->value);
}

static void connect_child_and_parents(struct json *json)
{
        json->element->parent_type = JSON_PARENT_OBJECT;
        json->element->parent.json = json;
        connect_child_and_parents_element(json->element);
}

static bool isValue(enum json_token_type token)
{
        return (token == LITERAL_STRING || token == LITERAL_FLOAT || token == LITERAL_INT || token == LITERAL_TRUE
                || token == LITERAL_FALSE || token == LITERAL_NULL);
}

static int process_token(struct err *err, struct json_err *error_desc, const struct json_token *token,
        struct vector ofType(enum json_token_type) *brackets, struct token_memory *token_mem)
{
        switch (token->type) {
        case OBJECT_OPEN:
        case ARRAY_OPEN:
                vec_push(brackets, &token->type, 1);
                break;
        case OBJECT_CLOSE:
        case ARRAY_CLOSE: {
                if (!vec_is_empty(brackets)) {
                        enum json_token_type bracket = *VECTOR_PEEK(brackets, enum json_token_type);
                        if ((token->type == ARRAY_CLOSE && bracket == ARRAY_OPEN)
                                || (token->type == OBJECT_CLOSE && bracket == OBJECT_OPEN)) {
                                vec_pop(brackets);
                        } else {
                                goto pushEntry;
                        }
                } else {
                        pushEntry:
                        vec_push(brackets, &token->type, 1);
                }
        }
                break;
        default:
                break;
        }

        switch (token_mem->type) {
        case OBJECT_OPEN:
                switch (token->type) {
                case LITERAL_STRING:
                case OBJECT_CLOSE:
                        break;
                default:
                        return set_error(error_desc, token, "Expected key name or '}'");
                }
                break;
        case LITERAL_STRING:
                switch (token->type) {
                case ASSIGN:
                case COMMA:
                case ARRAY_CLOSE:
                case OBJECT_CLOSE:
                        break;
                default:
                        return set_error(error_desc, token, "Expected key name (missing ':'), enumeration (','), "
                                "end of enumeration (']'), or end of object ('}')");
                }
                break;
        case OBJECT_CLOSE:
        case LITERAL_INT:
        case LITERAL_FLOAT:
        case LITERAL_TRUE:
        case LITERAL_FALSE:
        case LITERAL_NULL:
                switch (token->type) {
                case COMMA:
                case ARRAY_CLOSE:
                case OBJECT_CLOSE:
                        break;
                default:
                        return set_error(error_desc, token, "Expected enumeration (','), end of enumeration (']'), "
                                "or end of object ('})");
                }
                break;
        case ASSIGN:
        case COMMA:
                switch (token->type) {
                case LITERAL_STRING:
                case LITERAL_FLOAT:
                case LITERAL_INT:
                case OBJECT_OPEN:
                case ARRAY_OPEN:
                case LITERAL_TRUE:
                case LITERAL_FALSE:
                case LITERAL_NULL:
                        break;
                default:
                        return set_error(error_desc,
                                token,
                                "Expected key name, or value (string, number, object, enumeration, true, "
                                        "false, or null).");
                }
                break;
        case ARRAY_OPEN:
                switch (token->type) {
                case ARRAY_CLOSE:
                case LITERAL_STRING:
                case LITERAL_FLOAT:
                case LITERAL_INT:
                case OBJECT_OPEN:
                case ARRAY_OPEN:
                case LITERAL_TRUE:
                case LITERAL_FALSE:
                case LITERAL_NULL:
                        break;
                default:
                        return set_error(error_desc, token, "End of enumeration (']'), enumeration (','), or "
                                "end of enumeration (']')");
                }
                break;
        case ARRAY_CLOSE:
                switch (token->type) {
                case COMMA:
                case ARRAY_CLOSE:
                case OBJECT_CLOSE:
                        break;
                default:
                        return set_error(error_desc, token, "End of enumeration (']'), enumeration (','), or "
                                "end of object ('}')");
                }
                break;
        case JSON_UNKNOWN:
                if (token_mem->init) {
                        if (token->type != OBJECT_OPEN && token->type != ARRAY_OPEN && !isValue(token->type)) {
                                return set_error(error_desc, token, "Expected JSON document: missing '{' or '['");
                        }
                        token_mem->init = false;
                } else {
                        return set_error(error_desc, token, "Unexpected token");
                }
                break;
        default: error(err, NG5_ERR_NOJSONTOKEN)
                return false;
        }

        token_mem->type = token->type;
        return true;
}

static int set_error(struct json_err *error_desc, const struct json_token *token, const char *msg)
{
        if (error_desc) {
                error_desc->token = token;
                error_desc->token_type_str = token ? TOKEN_STRING[token->type].string : "(no token)";
                error_desc->msg = msg;
        }
        return false;
}

static bool json_ast_node_member_print(FILE *file, struct err *err, struct json_prop *member)
{
        fprintf(file, "\"%s\": ", member->key.value);
        return json_ast_node_value_print(file, err, &member->value.value);
}

static bool json_ast_node_object_print(FILE *file, struct err *err, struct json_object_t *object)
{
        fprintf(file, "{");
        for (size_t i = 0; i < object->value->members.num_elems; i++) {
                struct json_prop *member = vec_get(&object->value->members, i, struct json_prop);
                if (!json_ast_node_member_print(file, err, member)) {
                        return false;
                }
                fprintf(file, "%s", i + 1 < object->value->members.num_elems ? ", " : "");
        }
        fprintf(file, "}");
        return true;
}

static bool json_ast_node_array_print(FILE *file, struct err *err, struct json_array *array)
{
        fprintf(file, "[");
        for (size_t i = 0; i < array->elements.elements.num_elems; i++) {
                struct json_element *element = vec_get(&array->elements.elements, i, struct json_element);
                if (!json_ast_node_element_print(file, err, element)) {
                        return false;
                }
                fprintf(file, "%s", i + 1 < array->elements.elements.num_elems ? ", " : "");
        }
        fprintf(file, "]");
        return true;
}

static void json_ast_node_string_print(FILE *file, struct json_string *string)
{
        fprintf(file, "\"%s\"", string->value);
}

static bool json_ast_node_number_print(FILE *file, struct err *err, struct json_number *number)
{
        switch (number->value_type) {
        case JSON_NUMBER_FLOAT:
                fprintf(file, "%f", number->value.float_number);
                break;
        case JSON_NUMBER_UNSIGNED:
                fprintf(file, "%" PRIu64, number->value.unsigned_integer);
                break;
        case JSON_NUMBER_SIGNED:
                fprintf(file, "%" PRIi64, number->value.signed_integer);
                break;
        default: error(err, NG5_ERR_NOJSONNUMBERT);
                return false;
        }
        return true;
}

static bool json_ast_node_value_print(FILE *file, struct err *err, struct json_node_value *value)
{
        switch (value->value_type) {
        case JSON_VALUE_OBJECT:
                if (!json_ast_node_object_print(file, err, value->value.object)) {
                        return false;
                }
                break;
        case JSON_VALUE_ARRAY:
                if (!json_ast_node_array_print(file, err, value->value.array)) {
                        return false;
                }
                break;
        case JSON_VALUE_STRING:
                json_ast_node_string_print(file, value->value.string);
                break;
        case JSON_VALUE_NUMBER:
                if (!json_ast_node_number_print(file, err, value->value.number)) {
                        return false;
                }
                break;
        case JSON_VALUE_TRUE:
                fprintf(file, "true");
                break;
        case JSON_VALUE_FALSE:
                fprintf(file, "false");
                break;
        case JSON_VALUE_NULL:
                fprintf(file, "null");
                break;
        default: error(err, NG5_ERR_NOTYPE);
                return false;
        }
        return true;
}

static bool json_ast_node_element_print(FILE *file, struct err *err, struct json_element *element)
{
        return json_ast_node_value_print(file, err, &element->value);
}

static bool json_ast_node_value_drop(struct json_node_value *value, struct err *err);

static bool json_ast_node_element_drop(struct json_element *element, struct err *err)
{
        return json_ast_node_value_drop(&element->value, err);
}

static bool json_ast_node_member_drop(struct json_prop *member, struct err *err)
{
        free(member->key.value);
        return json_ast_node_element_drop(&member->value, err);
}

static bool json_ast_node_members_drop(struct json_members *members, struct err *err)
{
        for (size_t i = 0; i < members->members.num_elems; i++) {
                struct json_prop *member = vec_get(&members->members, i, struct json_prop);
                if (!json_ast_node_member_drop(member, err)) {
                        return false;
                }
        }
        vec_drop(&members->members);
        return true;
}

static bool json_ast_node_elements_drop(struct json_elements *elements, struct err *err)
{
        for (size_t i = 0; i < elements->elements.num_elems; i++) {
                struct json_element *element = vec_get(&elements->elements, i, struct json_element);
                if (!json_ast_node_element_drop(element, err)) {
                        return false;
                }
        }
        vec_drop(&elements->elements);
        return true;
}

static bool json_ast_node_object_drop(struct json_object_t *object, struct err *err)
{
        if (!json_ast_node_members_drop(object->value, err)) {
                return false;
        } else {
                free(object->value);
                return true;
        }
}

static bool json_ast_node_array_drop(struct json_array *array, struct err *err)
{
        return json_ast_node_elements_drop(&array->elements, err);
}

static void json_ast_node_string_drop(struct json_string *string)
{
        free(string->value);
}

static void json_ast_node_number_drop(struct json_number *number)
{
        ng5_unused(number);
}

static bool json_ast_node_value_drop(struct json_node_value *value, struct err *err)
{
        switch (value->value_type) {
        case JSON_VALUE_OBJECT:
                if (!json_ast_node_object_drop(value->value.object, err)) {
                        return false;
                } else {
                        free(value->value.object);
                }
                break;
        case JSON_VALUE_ARRAY:
                if (!json_ast_node_array_drop(value->value.array, err)) {
                        return false;
                } else {
                        free(value->value.array);
                }
                break;
        case JSON_VALUE_STRING:
                json_ast_node_string_drop(value->value.string);
                free(value->value.string);
                break;
        case JSON_VALUE_NUMBER:
                json_ast_node_number_drop(value->value.number);
                free(value->value.number);
                break;
        case JSON_VALUE_TRUE:
        case JSON_VALUE_FALSE:
        case JSON_VALUE_NULL:
                break;
        default: error(err, NG5_ERR_NOTYPE)
                return false;

        }
        return true;
}

bool json_drop(struct json *json)
{
        struct json_element *element = json->element;
        if (!json_ast_node_value_drop(&element->value, &json->err)) {
                return false;
        } else {
                free(json->element);
                return true;
        }
}

bool json_print(FILE *file, struct json *json)
{
        return json_ast_node_element_print(file, &json->err, json->element);
}