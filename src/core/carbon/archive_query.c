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

#include "core/carbon/archive_int.h"
#include "core/carbon/archive_string_pred.h"
#include "core/carbon/archive_sid_cache.h"
#include "core/carbon/archive_query.h"

struct sid_to_offset_arg {
        offset_t offset;
        u32 strlen;
};

struct sid_to_offset {
        struct hashtable ofMapping(field_sid_t, struct sid_to_offset_arg) mapping;
        FILE *disk_file;
        size_t disk_file_size;

};

#define OBJECT_GET_KEYS_TO_FIX_TYPE_GENERIC(num_pairs, obj, bit_flag_name, offset_name)                                \
{                                                                                                                      \
    if (!obj) {                                                                                                        \
        print_error_and_die(NG5_ERR_NULLPTR)                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    if (obj->flags.bits.bit_flag_name) {                                                                               \
        assert(obj->props.offset_name != 0);                                                                           \
        memfile_seek(&obj->file, obj->props.offset_name);                                                       \
        struct fixed_prop prop;                                                                                      \
        int_embedded_fixed_props_read(&prop, &obj->file);                                                       \
        int_reset_cabin_object_mem_file(obj);                                                                   \
        ng5_optional_set(num_pairs, prop.header->num_entries);                                                      \
        return prop.keys;                                                                                              \
    } else {                                                                                                           \
        ng5_optional_set(num_pairs, 0);                                                                             \
        return NULL;                                                                                                   \
    }                                                                                                                  \
}

NG5_EXPORT(bool) query_create(struct archive_query *query, struct archive *archive)
{
        error_if_null(query)
        error_if_null(archive)
        query->archive = archive;
        query->context = archive_io_context_create(archive);
        error_init(&query->err);
        return query->context != NULL;
}

NG5_EXPORT(bool) query_drop(struct archive_query *query)
{
        error_if_null(query)
        return io_context_drop(query->context);
}

NG5_EXPORT(bool) query_scan_strids(struct strid_iter *it, struct archive_query *query)
{
        error_if_null(it)
        error_if_null(query)
        return strid_iter_open(it, &query->err, query->archive);
}

static bool index_string_id_to_offset_open_file(struct sid_to_offset *index, struct err *err, const char *file)
{
        index->disk_file = fopen(file, "r");
        if (!index->disk_file) {
                error(err, NG5_ERR_FOPEN_FAILED)
                return false;
        } else {
                fseek(index->disk_file, 0, SEEK_END);
                index->disk_file_size = ftell(index->disk_file);
                fseek(index->disk_file, 0, SEEK_SET);
                return true;
        }
}

NG5_EXPORT(bool) query_create_index_string_id_to_offset(struct sid_to_offset **index, struct archive_query *query)
{
        error_if_null(index)
        error_if_null(query)

        struct strid_iter strid_iter;
        struct strid_info *info;
        size_t vector_len;
        bool status;
        bool success;
        u32 capacity;
        struct archive_info archive_info;
        archive_get_info(&archive_info, query->archive);
        capacity = archive_info.num_embeddded_strings;

        struct sid_to_offset *result = malloc(sizeof(struct sid_to_offset));
        hashtable_create(&result->mapping,
                &query->err,
                sizeof(field_sid_t),
                sizeof(struct sid_to_offset_arg),
                capacity);

        if (!index_string_id_to_offset_open_file(result, &query->err, query->archive->diskFilePath)) {
                return false;
        }

        status = query_scan_strids(&strid_iter, query);

        if (status) {
                while (strid_iter_next(&success, &info, &query->err, &vector_len, &strid_iter)) {
                        for (size_t i = 0; i < vector_len; i++) {
                                struct sid_to_offset_arg arg = {.offset = info[i].offset, .strlen = info[i].strlen};
                                hashtable_insert_or_update(&result->mapping, &info[i].id, &arg, 1);
                        }
                }
                *index = result;
                strid_iter_close(&strid_iter);
                return true;
        } else {
                error(&query->err, NG5_ERR_SCAN_FAILED);
                return false;
        }

}

NG5_EXPORT(void) query_drop_index_string_id_to_offset(struct sid_to_offset *index)
{
        if (index) {
                hashtable_drop(&index->mapping);
                fclose(index->disk_file);
                free(index);
        }
}

NG5_EXPORT(bool) query_index_id_to_offset_serialize(FILE *file, struct err *err, struct sid_to_offset *index)
{
        ng5_unused(file);
        ng5_unused(err);
        ng5_unused(index);
        return hashtable_serialize(file, &index->mapping);
}

NG5_EXPORT(bool) query_index_id_to_offset_deserialize(struct sid_to_offset **index, struct err *err,
        const char *file_path, offset_t offset)
{
        error_if_null(index)
        error_if_null(err)
        error_if_null(file_path)
        error_if_null(offset)

        struct sid_to_offset *result = malloc(sizeof(struct sid_to_offset));
        if (!result) {
                error(err, NG5_ERR_MALLOCERR);
                return false;
        }

        if (!index_string_id_to_offset_open_file(result, err, file_path)) {
                return false;
        }

        FILE *index_reader_file = fopen(file_path, "r");
        if (!index_reader_file) {
                error(err, NG5_ERR_FOPEN_FAILED)
                return false;
        } else {
                fseek(index_reader_file, 0, SEEK_END);
                offset_t file_size = ftell(index_reader_file);

                if (offset >= file_size) {
                        error(err, NG5_ERR_INTERNALERR)
                        return false;
                }

                fseek(index_reader_file, offset, SEEK_SET);

                if (!hashtable_deserialize(&result->mapping, err, index_reader_file)) {
                        error(err, NG5_ERR_HASTABLE_DESERIALERR);
                        fclose(index_reader_file);
                        *index = NULL;
                        return false;
                }

                fclose(index_reader_file);
                *index = result;
                return true;
        }
}

static char *fetch_string_from_file(bool *decode_success, FILE *disk_file, size_t offset, size_t string_len,
        struct err *err, struct archive *archive)
{
        char *result = malloc(string_len + 1);
        memset(result, 0, string_len + 1);

        fseek(disk_file, offset, SEEK_SET);

        bool decode_result = pack_decode(err, &archive->string_table.compressor, result, string_len, disk_file);

        *decode_success = decode_result;
        return result;
}

static char *fetch_string_by_id_via_scan(struct archive_query *query, field_sid_t id)
{
        assert(query);

        struct strid_iter strid_iter;
        struct strid_info *info;
        size_t vector_len;
        bool status;
        bool success;

        status = query_scan_strids(&strid_iter, query);

        if (status) {
                while (strid_iter_next(&success, &info, &query->err, &vector_len, &strid_iter)) {
                        for (size_t i = 0; i < vector_len; i++) {
                                if (info[i].id == id) {
                                        bool decode_result;
                                        char *result = fetch_string_from_file(&decode_result,
                                                strid_iter.disk_file,
                                                info[i].offset,
                                                info[i].strlen,
                                                &query->err,
                                                query->archive);

                                        bool close_iter_result = strid_iter_close(&strid_iter);

                                        if (!success || !close_iter_result) {
                                                if (result) {
                                                        free(result);
                                                }
                                                error(&query->err,
                                                        !decode_result ? NG5_ERR_DECOMPRESSFAILED
                                                                       : NG5_ERR_ITERATORNOTCLOSED);
                                                return NULL;
                                        } else {
                                                return result;
                                        }
                                }
                        }
                }
                strid_iter_close(&strid_iter);
                error(&query->err, NG5_ERR_NOTFOUND);
                return NULL;
        } else {
                error(&query->err, NG5_ERR_SCAN_FAILED);
                return NULL;
        }
}

static char *fetch_string_by_id_via_index(struct archive_query *query, struct sid_to_offset *index, field_sid_t id)
{
        const struct sid_to_offset_arg *args = hashtable_get_value(&index->mapping, &id);
        if (args) {
                if (args->offset < index->disk_file_size) {
                        bool decode_result;
                        char *result = fetch_string_from_file(&decode_result,
                                index->disk_file,
                                args->offset,
                                args->strlen,
                                &query->err,
                                query->archive);
                        if (decode_result) {
                                return result;
                        } else {
                                error(&query->err, NG5_ERR_DECOMPRESSFAILED);
                                return NULL;
                        }

                } else {
                        error(&query->err, NG5_ERR_INDEXCORRUPTED_OFFSET);
                        return NULL;
                }
        } else {
                error(&query->err, NG5_ERR_NOTFOUND);
                return NULL;
        }
}

NG5_EXPORT(char *)query_fetch_string_by_id(struct archive_query *query, field_sid_t id)
{
        assert(query);

        bool has_cache = false;
        archive_hash_query_string_id_cache(&has_cache, query->archive);
        if (has_cache) {
                return string_id_cache_get(query->archive->string_id_cache, id);
        } else {
                return query_fetch_string_by_id_nocache(query, id);
        }
}

NG5_EXPORT(char *)query_fetch_string_by_id_nocache(struct archive_query *query, field_sid_t id)
{
        bool has_index;
        archive_has_query_index_string_id_to_offset(&has_index, query->archive);
        if (has_index) {
                return fetch_string_by_id_via_index(query, query->archive->query_index_string_id_to_offset, id);
        } else {
                return fetch_string_by_id_via_scan(query, id);
        }
}

NG5_EXPORT(char **)query_fetch_strings_by_offset(struct archive_query *query, offset_t *offs, u32 *strlens,
        size_t num_offs)
{
        assert(query);
        assert(offs);
        assert(strlens);

        FILE *file;

        if (num_offs == 0) {
                return NULL;
        }

        char **result = malloc(num_offs * sizeof(char *));
        if (!result) {
                error(&query->err, NG5_ERR_MALLOCERR);
                return NULL;
        }
        for (size_t i = 0; i < num_offs; i++) {
                if ((result[i] = malloc((strlens[i] + 1) * sizeof(char))) == NULL) {
                        for (size_t k = 0; k < i; k++) {
                                free(result[k]);
                        }
                        free(result);
                        return NULL;
                }
                memset(result[i], 0, (strlens[i] + 1) * sizeof(char));
        }

        if (!result) {
                error(io_context_get_error(query->context), NG5_ERR_MALLOCERR);
                return NULL;
        } else {
                if (!(file = io_context_lock_and_access(query->context))) {
                        error_cpy(&query->err, io_context_get_error(query->context));
                        goto cleanup_and_error;
                }

                for (size_t i = 0; i < num_offs; i++) {
                        fseek(file, offs[i], SEEK_SET);
                        if (!pack_decode(&query->err,
                                &query->archive->string_table.compressor,
                                result[i],
                                strlens[i],
                                file)) {
                                io_context_unlock(query->context);
                                goto cleanup_and_error;
                        }
                }
                io_context_unlock(query->context);
                return result;
        }

        cleanup_and_error:
        for (size_t i = 0; i < num_offs; i++) {
                free(result[i]);
        }
        free(result);
        return NULL;
}

NG5_EXPORT(field_sid_t *)query_find_ids(size_t *num_found, struct archive_query *query,
        const struct string_pred_t *pred, void *capture, i64 limit)
{
        if (unlikely(string_pred_validate(&query->err, pred) == false)) {
                return NULL;
        }
        i64 pred_limit;
        string_pred_get_limit(&pred_limit, pred);
        pred_limit = pred_limit < 0 ? limit : ng5_min(pred_limit, limit);

        struct strid_iter it;
        struct strid_info *info = NULL;
        size_t info_len = 0;
        size_t step_len = 0;
        offset_t *str_offs = NULL;
        u32 *str_lens = NULL;
        size_t *idxs_matching = NULL;
        size_t num_matching = 0;
        void *tmp = NULL;
        size_t str_cap = 1024;
        field_sid_t *step_ids = NULL;
        field_sid_t *result_ids = NULL;
        size_t result_len = 0;
        size_t result_cap = pred_limit < 0 ? str_cap : (size_t) pred_limit;
        bool success = false;

        if (unlikely(pred_limit == 0)) {
                *num_found = 0;
                return NULL;
        }

        if (unlikely(!num_found || !query || !pred)) {
                error(&query->err, NG5_ERR_NULLPTR);
                return NULL;
        }

        if (unlikely((step_ids = malloc(str_cap * sizeof(field_sid_t))) == NULL)) {
                error(&query->err, NG5_ERR_MALLOCERR);
                return NULL;
        }

        if (unlikely((str_offs = malloc(str_cap * sizeof(offset_t))) == NULL)) {
                error(&query->err, NG5_ERR_MALLOCERR);
                goto cleanup_result_and_error;
                return NULL;
        }

        if (unlikely((str_lens = malloc(str_cap * sizeof(u32))) == NULL)) {
                error(&query->err, NG5_ERR_MALLOCERR);
                free(str_offs);
                goto cleanup_result_and_error;
                return NULL;
        }

        if (unlikely((idxs_matching = malloc(str_cap * sizeof(size_t))) == NULL)) {
                error(&query->err, NG5_ERR_MALLOCERR);
                free(str_offs);
                free(str_lens);
                goto cleanup_result_and_error;
                return NULL;
        }

        if (unlikely(query_scan_strids(&it, query) == false)) {
                free(str_offs);
                free(str_lens);
                free(idxs_matching);
                goto cleanup_result_and_error;
        }

        if (unlikely((result_ids = malloc(result_cap * sizeof(field_sid_t))) == NULL)) {
                error(&query->err, NG5_ERR_MALLOCERR);
                free(str_offs);
                free(str_lens);
                free(idxs_matching);
                strid_iter_close(&it);
                goto cleanup_result_and_error;
                return NULL;
        }

        while (strid_iter_next(&success, &info, &query->err, &info_len, &it)) {
                if (unlikely(info_len > str_cap)) {
                        str_cap = (info_len + 1) * 1.7f;
                        if (unlikely((tmp = realloc(str_offs, str_cap * sizeof(offset_t))) == NULL)) {
                                goto realloc_error;
                        } else {
                                str_offs = tmp;
                        }
                        if (unlikely((tmp = realloc(str_lens, str_cap * sizeof(u32))) == NULL)) {
                                goto realloc_error;
                        } else {
                                str_lens = tmp;
                        }
                        if (unlikely((tmp = realloc(idxs_matching, str_cap * sizeof(size_t))) == NULL)) {
                                goto realloc_error;
                        } else {
                                idxs_matching = tmp;
                        }
                }
                assert(info_len <= str_cap);
                for (step_len = 0; step_len < info_len; step_len++) {
                        assert(step_len < str_cap);
                        str_offs[step_len] = info[step_len].offset;
                        str_lens[step_len] = info[step_len].strlen;
                }

                char **strings = query_fetch_strings_by_offset(query,
                        str_offs,
                        str_lens,
                        step_len); // TODO: buffer + cleanup buffer

                if (unlikely(
                        string_pred_eval(pred, idxs_matching, &num_matching, strings, step_len, capture) == false)) {
                        error(&query->err, NG5_ERR_PREDEVAL_FAILED);
                        strid_iter_close(&it);
                        goto cleanup_intermediate;
                }

                for (size_t i = 0; i < step_len; i++) {
                        free(strings[i]);
                }
                free(strings);

                for (size_t i = 0; i < num_matching; i++) {
                        assert (idxs_matching[i] < info_len);
                        result_ids[result_len++] = info[idxs_matching[i]].id;
                        if (pred_limit > 0 && result_len == (size_t) pred_limit) {
                                goto stop_search_and_return;
                        }
                        if (unlikely(result_len > result_cap)) {
                                result_cap = (result_len + 1) * 1.7f;
                                if (unlikely(
                                        (tmp = realloc(result_ids, result_cap * sizeof(field_sid_t))) == NULL)) {
                                        strid_iter_close(&it);
                                        goto cleanup_intermediate;
                                } else {
                                        result_ids = tmp;
                                }
                        }
                }
        }

        stop_search_and_return:
        if (unlikely(success == false)) {
                strid_iter_close(&it);
                goto cleanup_intermediate;
        }

        *num_found = result_len;
        return result_ids;

        realloc_error:
        error(&query->err, NG5_ERR_REALLOCERR);

        cleanup_intermediate:
        free(str_offs);
        free(str_lens);
        free(idxs_matching);
        free(result_ids);

        cleanup_result_and_error:
        free(step_ids);
        return NULL;
}