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

#ifndef NG5_DOC_H
#define NG5_DOC_H

#include "shared/common.h"
#include "std/vec.h"
#include "stdx/strdic.h"
#include "json.h"

NG5_BEGIN_DECL

struct doc_obj;

struct columndoc;

struct doc_entries {
        struct doc_obj *context;
        const char *key;
        field_e type;
        struct vector ofType(<T>) values;
};

struct doc_bulk {
        struct strdic *dic;
        struct vector ofType(char *) keys, values;
        struct vector ofType(struct doc) models;
};

struct doc {
        struct doc_bulk *context;
        struct vector ofType(struct doc_obj) obj_model;
        field_e type;
};

struct doc_obj {
        struct vector ofType(struct doc_entries) entries;
        struct doc *doc;
};

NG5_EXPORT(bool) doc_bulk_create(struct doc_bulk *bulk, struct strdic *dic);

NG5_EXPORT(bool) doc_bulk_Drop(struct doc_bulk *bulk);

NG5_EXPORT(bool) doc_bulk_shrink(struct doc_bulk *bulk);

NG5_EXPORT(bool) doc_bulk_print(FILE *file, struct doc_bulk *bulk);

NG5_EXPORT(struct doc *)doc_bulk_new_doc(struct doc_bulk *context, field_e type);

NG5_EXPORT(struct doc_obj *)doc_bulk_new_obj(struct doc *model);

NG5_EXPORT(bool) doc_bulk_get_dic_contents(struct vector ofType (const char *) **strings,
        struct vector ofType(field_sid_t) **string_ids, const struct doc_bulk *context);

NG5_EXPORT(bool) doc_print(FILE *file, const struct doc *doc);

NG5_EXPORT(const struct vector ofType(struct doc_entries)
        *)doc_get_entries(const struct doc_obj *model);

NG5_EXPORT(void) doc_print_entries(FILE *file, const struct doc_entries *entries);

NG5_EXPORT(void) doc_drop(struct doc_obj *model);

NG5_EXPORT(bool) doc_obj_add_key(struct doc_entries **out, struct doc_obj *obj, const char *key, field_e type);

NG5_EXPORT(bool) doc_obj_push_primtive(struct doc_entries *entry, const void *value);

NG5_EXPORT(bool) doc_obj_push_object(struct doc_obj **out, struct doc_entries *entry);

NG5_EXPORT(struct doc_entries *)doc_bulk_new_entries(struct doc_bulk *dst);

NG5_EXPORT(struct doc_obj *)doc_bulk_add_json(struct doc_entries *partition, struct json *json);

NG5_EXPORT(struct doc_obj *)doc_entries_get_root(const struct doc_entries *partition);

NG5_EXPORT(struct columndoc *)doc_entries_columndoc(const struct doc_bulk *bulk, const struct doc_entries *partition,
        bool read_optimized);

NG5_EXPORT(bool) doc_entries_drop(struct doc_entries *partition);

NG5_END_DECL

#endif