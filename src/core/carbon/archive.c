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

#include "core/oid/oid.h"
#include "core/encode/encode_async.h"
#include "core/pack/pack.h"
#include "core/carbon/archive_strid_iter.h"
#include "core/carbon/archive_int.h"
#include "core/carbon/archive_query.h"
#include "core/carbon/archive_sid_cache.h"
#include "err.h"
#include "core/carbon/archive.h"
#include "core/encode/encode_sync.h"
#include "shared/common.h"
#include "core/mem/block.h"
#include "core/mem/file.h"
#include "coding/coding_huffman.h"
#include "core/carbon/archive.h"

#define WRITE_PRIMITIVE_VALUES(memfile, values_vec, type)                                                              \
{                                                                                                                      \
    type *values = vec_all(values_vec, type);                                                                \
    memfile_write(memfile, values, values_vec->num_elems * sizeof(type));                                       \
}

#define WRITE_ARRAY_VALUES(memfile, values_vec, type)                                                                  \
{                                                                                                                      \
    for (u32 i = 0; i < values_vec->num_elems; i++) {                                                             \
        struct vector ofType(type) *nested_values = vec_get(values_vec, i, struct vector);                     \
        WRITE_PRIMITIVE_VALUES(memfile, nested_values, type);                                                          \
    }                                                                                                                  \
}

#define PRINT_SIMPLE_PROPS(file, memfile, offset, nesting_level, value_type, type_string, format_string)               \
{                                                                                                                      \
    struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);             \
    field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop_header->num_entries *          \
                                   sizeof(field_sid_t));                                                        \
    value_type *values = (value_type *) NG5_MEMFILE_READ(memfile, prop_header->num_entries * sizeof(value_type));   \
    fprintf(file, "0x%04x ", (unsigned) offset);                                                                       \
    INTENT_LINE(nesting_level)                                                                                         \
    fprintf(file, "[marker: %c (" type_string ")] [num_entries: %d] [", entryMarker, prop_header->num_entries);        \
    for (u32 i = 0; i < prop_header->num_entries; i++) {                                                          \
        fprintf(file, "key: %"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");                      \
    }                                                                                                                  \
    fprintf(file, "] [");                                                                                              \
    for (u32 i = 0; i < prop_header->num_entries; i++) {                                                          \
      fprintf(file, "value: "format_string"%s", values[i], i + 1 < prop_header->num_entries ? ", " : "");              \
    }                                                                                                                  \
    fprintf(file, "]\n");                                                                                              \
}

#define PRINT_ARRAY_PROPS(memfile, offset, nesting_level, entryMarker, type, type_string, format_string)               \
{                                                                                                                      \
    struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);             \
                                                                                                                       \
    field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop_header->num_entries *          \
                                        sizeof(field_sid_t));                                                   \
    u32 *array_lengths;                                                                                           \
                                                                                                                       \
    fprintf(file, "0x%04x ", (unsigned) offset);                                                                       \
    INTENT_LINE(nesting_level)                                                                                         \
    fprintf(file, "[marker: %c ("type_string")] [num_entries: %d] [", entryMarker, prop_header->num_entries);          \
                                                                                                                       \
    for (u32 i = 0; i < prop_header->num_entries; i++) {                                                          \
        fprintf(file, "key: %"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");                      \
    }                                                                                                                  \
    fprintf(file, "] [");                                                                                              \
                                                                                                                       \
    array_lengths = (u32 *) NG5_MEMFILE_READ(memfile, prop_header->num_entries * sizeof(u32));            \
                                                                                                                       \
    for (u32 i = 0; i < prop_header->num_entries; i++) {                                                          \
        fprintf(file, "num_entries: %d%s", array_lengths[i], i + 1 < prop_header->num_entries ? ", " : "");            \
    }                                                                                                                  \
                                                                                                                       \
    fprintf(file, "] [");                                                                                              \
                                                                                                                       \
    for (u32 array_idx = 0; array_idx < prop_header->num_entries; array_idx++) {                                  \
        type *values = (type *) NG5_MEMFILE_READ(memfile, array_lengths[array_idx] * sizeof(type));                 \
        fprintf(file, "[");                                                                                            \
        for (u32 i = 0; i < array_lengths[array_idx]; i++) {                                                      \
            fprintf(file, "value: "format_string"%s", values[i], i + 1 < array_lengths[array_idx] ? ", " : "");        \
        }                                                                                                              \
        fprintf(file, "]%s", array_idx + 1 < prop_header->num_entries ? ", " : "");                                    \
    }                                                                                                                  \
                                                                                                                       \
    fprintf(file, "]\n");                                                                                              \
}

#define INTENT_LINE(nesting_level)                                                                                     \
{                                                                                                                      \
    for (unsigned nest_level = 0; nest_level < nesting_level; nest_level++) {                                          \
        fprintf(file, "   ");                                                                                          \
    }                                                                                                                  \
}

#define PRINT_VALUE_ARRAY(type, memfile, header, format_string)                                                        \
{                                                                                                                      \
    u32 num_elements = *NG5_MEMFILE_READ_TYPE(memfile, u32);                                              \
    const type *values = (const type *) NG5_MEMFILE_READ(memfile, num_elements * sizeof(type));                     \
    fprintf(file, "0x%04x ", (unsigned) offset);                                                                       \
    INTENT_LINE(nesting_level);                                                                                        \
    fprintf(file, "   [num_elements: %d] [values: [", num_elements);                                                   \
    for (size_t i = 0; i < num_elements; i++) {                                                                        \
        fprintf(file, "value: "format_string"%s", values[i], i + 1 < num_elements ? ", " : "");                        \
    }                                                                                                                  \
    fprintf(file, "]\n");                                                                                              \
}

static offset_t skip_record_header(struct memfile *memfile);
static void update_record_header(struct memfile *memfile, offset_t root_object_header_offset, struct columndoc *model,
        u64 record_size);
static bool __serialize(offset_t *offset, struct err *err, struct memfile *memfile, struct columndoc_obj *columndoc,
        offset_t root_object_header_offset);
static union object_flags *get_flags(union object_flags *flags, struct columndoc_obj *columndoc);
static void update_file_header(struct memfile *memfile, offset_t root_object_header_offset);
static void skip_file_header(struct memfile *memfile);
static bool serialize_string_dic(struct memfile *memfile, struct err *err, const struct doc_bulk *context,
        enum packer_type compressor);
static bool print_archive_from_memfile(FILE *file, struct err *err, struct memfile *memfile);

NG5_EXPORT(bool) archive_from_json(struct archive *out, const char *file, struct err *err, const char *json_string,
        enum packer_type compressor, enum strdic_tag dictionary, size_t num_async_dic_threads, bool read_optimized,
        bool bake_string_id_index, struct archive_callback *callback)
{
        error_if_null(out);
        error_if_null(file);
        error_if_null(err);
        error_if_null(json_string);

        ng5_optional_call(callback, begin_create_from_json);

        struct memblock *stream;
        FILE *out_file;

        if (!archive_stream_from_json(&stream,
                err,
                json_string,
                compressor,
                dictionary,
                num_async_dic_threads,
                read_optimized,
                bake_string_id_index,
                callback)) {
                return false;
        }

        ng5_optional_call(callback, begin_write_archive_file_to_disk);

        if ((out_file = fopen(file, "w")) == NULL) {
                error(err, NG5_ERR_FOPENWRITE);
                memblock_drop(stream);
                return false;
        }

        if (!archive_write(out_file, stream)) {
                error(err, NG5_ERR_WRITEARCHIVE);
                fclose(out_file);
                memblock_drop(stream);
                return false;
        }

        fclose(out_file);

        ng5_optional_call(callback, end_write_archive_file_to_disk);

        ng5_optional_call(callback, begin_load_archive);

        if (!archive_open(out, file)) {
                error(err, NG5_ERR_ARCHIVEOPEN);
                return false;
        }

        ng5_optional_call(callback, end_load_archive);

        memblock_drop(stream);

        ng5_optional_call(callback, end_create_from_json);

        return true;
}

NG5_EXPORT(bool) archive_stream_from_json(struct memblock **stream, struct err *err, const char *json_string,
        enum packer_type compressor, enum strdic_tag dictionary, size_t num_async_dic_threads, bool read_optimized,
        bool bake_id_index, struct archive_callback *callback)
{
        error_if_null(stream);
        error_if_null(err);
        error_if_null(json_string);

        struct strdic dic;
        struct json_parser parser;
        struct json_err error_desc;
        struct doc_bulk bulk;
        struct doc_entries *partition;
        struct columndoc *columndoc;
        struct json json;

        ng5_optional_call(callback, begin_archive_stream_from_json)

        ng5_optional_call(callback, begin_setup_string_dictionary);
        if (dictionary == SYNC) {
                encode_sync_create(&dic, 1000, 1000, 1000, 0, NULL);
        } else if (dictionary == ASYNC) {
                encode_async_create(&dic, 1000, 1000, 1000, num_async_dic_threads, NULL);
        } else {
                error(err, NG5_ERR_UNKNOWN_DIC_TYPE);
        }

        ng5_optional_call(callback, end_setup_string_dictionary);

        ng5_optional_call(callback, begin_parse_json);
        json_parser_create(&parser, &bulk);
        if (!(json_parse(&json, &error_desc, &parser, json_string))) {
                char buffer[2048];
                if (error_desc.token) {
                        sprintf(buffer,
                                "%s. Token %s was found in line %u column %u",
                                error_desc.msg,
                                error_desc.token_type_str,
                                error_desc.token->line,
                                error_desc.token->column);
                        error_with_details(err, NG5_ERR_JSONPARSEERR, &buffer[0]);
                } else {
                        sprintf(buffer, "%s", error_desc.msg);
                        error_with_details(err, NG5_ERR_JSONPARSEERR, &buffer[0]);
                }
                return false;
        }
        ng5_optional_call(callback, end_parse_json);

        ng5_optional_call(callback, begin_test_json);
        if (!json_test(err, &json)) {
                return false;
        }
        ng5_optional_call(callback, end_test_json);

        ng5_optional_call(callback, begin_import_json);
        if (!doc_bulk_create(&bulk, &dic)) {
                error(err, NG5_ERR_BULKCREATEFAILED);
                return false;
        }

        partition = doc_bulk_new_entries(&bulk);
        doc_bulk_add_json(partition, &json);

        json_drop(&json);

        doc_bulk_shrink(&bulk);

        columndoc = doc_entries_columndoc(&bulk, partition, read_optimized);

        if (!archive_from_model(stream, err, columndoc, compressor, bake_id_index, callback)) {
                return false;
        }

        ng5_optional_call(callback, end_import_json);

        ng5_optional_call(callback, begin_cleanup);
        strdic_drop(&dic);
        doc_bulk_Drop(&bulk);
        doc_entries_drop(partition);
        columndoc_free(columndoc);
        free(columndoc);
        ng5_optional_call(callback, end_cleanup);

        ng5_optional_call(callback, end_archive_stream_from_json)

        return true;
}

static bool run_string_id_baking(struct err *err, struct memblock **stream)
{
        struct archive archive;
        char tmp_file_name[512];
        object_id_t rand_part;
        object_id_create(&rand_part);
        sprintf(tmp_file_name, "/tmp/types-tool-temp-%"
                PRIu64
                ".carbon", rand_part);
        FILE *tmp_file;

        if ((tmp_file = fopen(tmp_file_name, "w")) == NULL) {
                error(err, NG5_ERR_TMP_FOPENWRITE);
                return false;
        }

        if (!archive_write(tmp_file, *stream)) {
                error(err, NG5_ERR_WRITEARCHIVE);
                fclose(tmp_file);
                remove(tmp_file_name);
                return false;
        }

        fflush(tmp_file);
        fclose(tmp_file);

        if (!archive_open(&archive, tmp_file_name)) {
                error(err, NG5_ERR_ARCHIVEOPEN);
                return false;
        }

        bool has_index;
        archive_has_query_index_string_id_to_offset(&has_index, &archive);
        if (has_index) {
                error(err, NG5_ERR_INTERNALERR);
                remove(tmp_file_name);
                return false;
        }

        struct sid_to_offset *index;
        struct archive_query query;
        query_create(&query, &archive);
        query_create_index_string_id_to_offset(&index, &query);
        query_drop(&query);
        archive_close(&archive);

        if ((tmp_file = fopen(tmp_file_name, "rb+")) == NULL) {
                error(err, NG5_ERR_TMP_FOPENWRITE);
                return false;
        }

        fseek(tmp_file, 0, SEEK_END);
        offset_t index_pos = ftell(tmp_file);
        query_index_id_to_offset_serialize(tmp_file, err, index);
        offset_t file_length = ftell(tmp_file);
        fseek(tmp_file, 0, SEEK_SET);

        struct archive_header header;
        size_t nread = fread(&header, sizeof(struct archive_header), 1, tmp_file);
        error_if(nread != 1, err, NG5_ERR_FREAD_FAILED);
        header.string_id_to_offset_index_offset = index_pos;
        fseek(tmp_file, 0, SEEK_SET);
        int nwrite = fwrite(&header, sizeof(struct archive_header), 1, tmp_file);
        error_if(nwrite != 1, err, NG5_ERR_FWRITE_FAILED);
        fseek(tmp_file, 0, SEEK_SET);

        query_drop_index_string_id_to_offset(index);

        memblock_drop(*stream);
        memblock_from_file(stream, tmp_file, file_length);

        remove(tmp_file_name);

        return true;
}

bool archive_from_model(struct memblock **stream, struct err *err, struct columndoc *model, enum packer_type compressor,
        bool bake_string_id_index, struct archive_callback *callback)
{
        error_if_null(model)
        error_if_null(stream)
        error_if_null(err)

        ng5_optional_call(callback, begin_create_from_model)

        memblock_create(stream, 1024 * 1024 * 1024);
        struct memfile memfile;
        memfile_open(&memfile, *stream, READ_WRITE);

        ng5_optional_call(callback, begin_write_string_table);
        skip_file_header(&memfile);
        if (!serialize_string_dic(&memfile, err, model->bulk, compressor)) {
                return false;
        }
        ng5_optional_call(callback, end_write_string_table);

        ng5_optional_call(callback, begin_write_record_table);
        offset_t record_header_offset = skip_record_header(&memfile);
        update_file_header(&memfile, record_header_offset);
        offset_t root_object_header_offset = memfile_tell(&memfile);
        if (!__serialize(NULL, err, &memfile, &model->columndoc, root_object_header_offset)) {
                return false;
        }
        u64 record_size = memfile_tell(&memfile) - (record_header_offset + sizeof(struct record_header));
        update_record_header(&memfile, record_header_offset, model, record_size);
        ng5_optional_call(callback, end_write_record_table);

        memfile_shrink(&memfile);

        if (bake_string_id_index) {
                /* create string id to offset index, and append it to the CARBON file */
                ng5_optional_call(callback, begin_string_id_index_baking);
                if (!run_string_id_baking(err, stream)) {
                        return false;
                }
                ng5_optional_call(callback, end_string_id_index_baking);
        } else {
                ng5_optional_call(callback, skip_string_id_index_baking);
        }

        ng5_optional_call(callback, end_create_from_model)

        return true;
}

NG5_EXPORT(struct io_context *)archive_io_context_create(struct archive *archive)
{
        error_if_null(archive);
        struct io_context *context;
        if (io_context_create(&context, &archive->err, archive->diskFilePath)) {
                return context;
        } else {
                error(&archive->err, NG5_ERR_IO)
                return NULL;
        }
}

bool archive_write(FILE *file, const struct memblock *stream)
{
        return memblock_write_to_file(file, stream);
}

bool archive_load(struct memblock **stream, FILE *file)
{
        long start = ftell(file);
        fseek(file, 0, SEEK_END);
        long end = ftell(file);
        fseek(file, start, SEEK_SET);
        long fileSize = (end - start);

        return memblock_from_file(stream, file, fileSize);
}

bool archive_print(FILE *file, struct err *err, struct memblock *stream)
{
        struct memfile memfile;
        memfile_open(&memfile, stream, READ_ONLY);
        if (memfile_size(&memfile)
                < sizeof(struct archive_header) + sizeof(struct string_table_header) + sizeof(struct object_header)) {
                error(err, NG5_ERR_NOCARBONSTREAM);
                return false;
        } else {
                return print_archive_from_memfile(file, err, &memfile);
        }
}

bool print_object(FILE *file, struct err *err, struct memfile *memfile, unsigned nesting_level);

static u32 flags_to_int32(union object_flags *flags)
{
        return *((i32 *) flags);
}

static const char *array_value_type_to_string(struct err *err, field_e type)
{
        switch (type) {
        case FIELD_NULL:
                return "Null Array";
        case FIELD_BOOLEAN:
                return "Boolean Array";
        case FIELD_INT8:
                return "Int8 Array";
        case FIELD_INT16:
                return "Int16 Array";
        case FIELD_INT32:
                return "Int32 Array";
        case FIELD_INT64:
                return "Int64 Array";
        case FIELD_UINT8:
                return "UInt8 Array";
        case FIELD_UINT16:
                return "UInt16 Array";
        case FIELD_UINT32:
                return "UInt32 Array";
        case FIELD_UINT64:
                return "UInt64 Array";
        case FIELD_FLOAT:
                return "UIntFloat Array";
        case FIELD_STRING:
                return "Text Array";
        case FIELD_OBJECT:
                return "Object Array";
        default: {
                error(err, NG5_ERR_NOVALUESTR)
                return NULL;
        }
        }
}

static void write_primitive_key_column(struct memfile *memfile, struct vector ofType(field_sid_t) *keys)
{
        field_sid_t *string_ids = vec_all(keys, field_sid_t);
        memfile_write(memfile, string_ids, keys->num_elems * sizeof(field_sid_t));
}

static offset_t skip_var_value_offset_column(struct memfile *memfile, size_t num_keys)
{
        offset_t result = memfile_tell(memfile);
        memfile_skip(memfile, num_keys * sizeof(offset_t));
        return result;
}

static void write_var_value_offset_column(struct memfile *file, offset_t where, offset_t after, const offset_t *values,
        size_t n)
{
        memfile_seek(file, where);
        memfile_write(file, values, n * sizeof(offset_t));
        memfile_seek(file, after);
}

static bool write_primitive_fixed_value_column(struct memfile *memfile, struct err *err, field_e type,
        struct vector ofType(T) *values_vec)
{
        assert (type != FIELD_OBJECT); /** use 'write_primitive_var_value_column' instead */

        switch (type) {
        case FIELD_NULL:
                break;
        case FIELD_BOOLEAN: WRITE_PRIMITIVE_VALUES(memfile, values_vec, FIELD_BOOLEANean_t);
                break;
        case FIELD_INT8: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_i8_t);
                break;
        case FIELD_INT16: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_i16_t);
                break;
        case FIELD_INT32: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_i32_t);
                break;
        case FIELD_INT64: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_i64_t);
                break;
        case FIELD_UINT8: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_u8_t);
                break;
        case FIELD_UINT16: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_u16_t);
                break;
        case FIELD_UINT32: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_u32_t);
                break;
        case FIELD_UINT64: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_u64_t);
                break;
        case FIELD_FLOAT: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_number_t);
                break;
        case FIELD_STRING: WRITE_PRIMITIVE_VALUES(memfile, values_vec, field_sid_t);
                break;
        default: error(err, NG5_ERR_NOTYPE);
                return false;
        }
        return true;
}

static offset_t *__write_primitive_column(struct memfile *memfile, struct err *err,
        struct vector ofType(struct columndoc_obj) *values_vec, offset_t root_offset)
{
        offset_t *result = malloc(values_vec->num_elems * sizeof(offset_t));
        struct columndoc_obj *mapped = vec_all(values_vec, struct columndoc_obj);
        for (u32 i = 0; i < values_vec->num_elems; i++) {
                struct columndoc_obj *obj = mapped + i;
                result[i] = memfile_tell(memfile) - root_offset;
                if (!__serialize(NULL, err, memfile, obj, root_offset)) {
                        return NULL;
                }
        }
        return result;
}

static bool __write_array_len_column(struct err *err, struct memfile *memfile, field_e type,
        struct vector ofType(...) *values)
{
        switch (type) {
        case FIELD_NULL:
                break;
        case FIELD_BOOLEAN:
        case FIELD_INT8:
        case FIELD_INT16:
        case FIELD_INT32:
        case FIELD_INT64:
        case FIELD_UINT8:
        case FIELD_UINT16:
        case FIELD_UINT32:
        case FIELD_UINT64:
        case FIELD_FLOAT:
        case FIELD_STRING:
                for (u32 i = 0; i < values->num_elems; i++) {
                        struct vector *arrays = vec_get(values, i, struct vector);
                        memfile_write(memfile, &arrays->num_elems, sizeof(u32));
                }
                break;
        case FIELD_OBJECT: print_error_and_die(NG5_ERR_ILLEGALIMPL)
                return false;
                break;
        default: error(err, NG5_ERR_NOTYPE);
                return false;
        }
        return true;
}

static bool write_array_value_column(struct memfile *memfile, struct err *err, field_e type,
        struct vector ofType(...) *values_vec)
{

        switch (type) {
        case FIELD_NULL: WRITE_PRIMITIVE_VALUES(memfile, values_vec, u32);
                break;
        case FIELD_BOOLEAN: WRITE_ARRAY_VALUES(memfile, values_vec, FIELD_BOOLEANean_t);
                break;
        case FIELD_INT8: WRITE_ARRAY_VALUES(memfile, values_vec, field_i8_t);
                break;
        case FIELD_INT16: WRITE_ARRAY_VALUES(memfile, values_vec, field_i16_t);
                break;
        case FIELD_INT32: WRITE_ARRAY_VALUES(memfile, values_vec, field_i32_t);
                break;
        case FIELD_INT64: WRITE_ARRAY_VALUES(memfile, values_vec, field_i64_t);
                break;
        case FIELD_UINT8: WRITE_ARRAY_VALUES(memfile, values_vec, field_u64_t);
                break;
        case FIELD_UINT16: WRITE_ARRAY_VALUES(memfile, values_vec, field_u16_t);
                break;
        case FIELD_UINT32: WRITE_ARRAY_VALUES(memfile, values_vec, field_u32_t);
                break;
        case FIELD_UINT64: WRITE_ARRAY_VALUES(memfile, values_vec, field_u64_t);
                break;
        case FIELD_FLOAT: WRITE_ARRAY_VALUES(memfile, values_vec, field_number_t);
                break;
        case FIELD_STRING: WRITE_ARRAY_VALUES(memfile, values_vec, field_sid_t);
                break;
        case FIELD_OBJECT: print_error_and_die(NG5_ERR_NOTIMPL)
                return false;
        default: error(err, NG5_ERR_NOTYPE)
                return false;
        }
        return true;
}

static bool write_array_prop(offset_t *offset, struct err *err, struct memfile *memfile,
        struct vector ofType(field_sid_t) *keys, field_e type, struct vector ofType(...) *values,
        offset_t root_object_header_offset)
{
        assert(keys->num_elems == values->num_elems);

        if (keys->num_elems > 0) {
                struct prop_header header =
                        {.marker = marker_symbols[value_array_marker_mapping[type].marker].symbol, .num_entries = keys
                                ->num_elems};
                offset_t prop_ofOffset = memfile_tell(memfile);
                memfile_write(memfile, &header, sizeof(struct prop_header));

                write_primitive_key_column(memfile, keys);
                if (!__write_array_len_column(err, memfile, type, values)) {
                        return false;
                }
                if (!write_array_value_column(memfile, err, type, values)) {
                        return false;
                }
                *offset = (prop_ofOffset - root_object_header_offset);
        } else {
                *offset = 0;
        }
        return true;
}

static bool write_array_props(struct memfile *memfile, struct err *err, struct columndoc_obj *columndoc,
        struct archive_prop_offs *offsets, offset_t root_object_header_offset)
{
        if (!write_array_prop(&offsets->null_arrays,
                err,
                memfile,
                &columndoc->null_array_prop_keys,
                FIELD_NULL,
                &columndoc->null_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->bool_arrays,
                err,
                memfile,
                &columndoc->bool_array_prop_keys,
                FIELD_BOOLEAN,
                &columndoc->bool_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->int8_arrays,
                err,
                memfile,
                &columndoc->int8_array_prop_keys,
                FIELD_INT8,
                &columndoc->int8_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->int16_arrays,
                err,
                memfile,
                &columndoc->int16_array_prop_keys,
                FIELD_INT16,
                &columndoc->int16_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->int32_arrays,
                err,
                memfile,
                &columndoc->int32_array_prop_keys,
                FIELD_INT32,
                &columndoc->int32_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->int64_arrays,
                err,
                memfile,
                &columndoc->int64_array_prop_keys,
                FIELD_INT64,
                &columndoc->int64_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->uint8_arrays,
                err,
                memfile,
                &columndoc->uint8_array_prop_keys,
                FIELD_UINT8,
                &columndoc->uint8_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->uint16_arrays,
                err,
                memfile,
                &columndoc->uint16_array_prop_keys,
                FIELD_UINT16,
                &columndoc->uint16_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->uint32_arrays,
                err,
                memfile,
                &columndoc->uint32_array_prop_keys,
                FIELD_UINT32,
                &columndoc->uint32_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->uint64_arrays,
                err,
                memfile,
                &columndoc->uint64_array_prop_keys,
                FIELD_UINT64,
                &columndoc->ui64_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->float_arrays,
                err,
                memfile,
                &columndoc->float_array_prop_keys,
                FIELD_FLOAT,
                &columndoc->float_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        if (!write_array_prop(&offsets->string_arrays,
                err,
                memfile,
                &columndoc->string_array_prop_keys,
                FIELD_STRING,
                &columndoc->string_array_prop_vals,
                root_object_header_offset)) {
                return false;
        }
        return true;
}

/** Fixed-length property lists; value position can be determined by size of value and position of key in key column.
 * In contrast, variable-length property list require an additional offset column (see 'write_var_props') */
static bool write_fixed_props(offset_t *offset, struct err *err, struct memfile *memfile,
        struct vector ofType(field_sid_t) *keys, field_e type, struct vector ofType(T) *values)
{
        assert(!values || keys->num_elems == values->num_elems);
        assert(type != FIELD_OBJECT); /** use 'write_var_props' instead */

        if (keys->num_elems > 0) {
                struct prop_header header =
                        {.marker = marker_symbols[valueMarkerMapping[type].marker].symbol, .num_entries = keys
                                ->num_elems};

                offset_t prop_ofOffset = memfile_tell(memfile);
                memfile_write(memfile, &header, sizeof(struct prop_header));

                write_primitive_key_column(memfile, keys);
                if (!write_primitive_fixed_value_column(memfile, err, type, values)) {
                        return false;
                }
                *offset = prop_ofOffset;
        } else {
                *offset = 0;
        }
        return true;
}

/** Variable-length property lists; value position cannot be determined by position of key in key column, since single
 * value has unknown size. Hence, a dedicated offset column is added to these properties allowing to seek directly
 * to a particular property. Due to the move of strings (i.e., variable-length values) to a dedicated string table,
 * the only variable-length value for properties are "JSON objects".
 * In contrast, fixed-length property list doesn't require an additional offset column (see 'write_fixed_props') */
static bool write_var_props(offset_t *offset, struct err *err, struct memfile *memfile,
        struct vector ofType(field_sid_t) *keys, struct vector ofType(struct columndoc_obj) *objects,
        offset_t root_object_header_offset)
{
        assert(!objects || keys->num_elems == objects->num_elems);

        if (keys->num_elems > 0) {
                struct prop_header header = {.marker = MARKER_SYMBOL_PROP_OBJECT, .num_entries = keys->num_elems};

                offset_t prop_ofOffset = memfile_tell(memfile);
                memfile_write(memfile, &header, sizeof(struct prop_header));

                write_primitive_key_column(memfile, keys);
                offset_t value_offset = skip_var_value_offset_column(memfile, keys->num_elems);
                offset_t *value_offsets = __write_primitive_column(memfile, err, objects, root_object_header_offset);
                if (!value_offsets) {
                        return false;
                }

                offset_t last = memfile_tell(memfile);
                write_var_value_offset_column(memfile, value_offset, last, value_offsets, keys->num_elems);
                free(value_offsets);
                *offset = prop_ofOffset;
        } else {
                *offset = 0;
        }
        return true;
}

static bool write_primitive_props(struct memfile *memfile, struct err *err, struct columndoc_obj *columndoc,
        struct archive_prop_offs *offsets, offset_t root_object_header_offset)
{
        if (!write_fixed_props(&offsets->nulls, err, memfile, &columndoc->null_prop_keys, FIELD_NULL, NULL)) {
                return false;
        }
        if (!write_fixed_props(&offsets->bools,
                err,
                memfile,
                &columndoc->bool_prop_keys,
                FIELD_BOOLEAN,
                &columndoc->bool_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->int8s,
                err,
                memfile,
                &columndoc->int8_prop_keys,
                FIELD_INT8,
                &columndoc->int8_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->int16s,
                err,
                memfile,
                &columndoc->int16_prop_keys,
                FIELD_INT16,
                &columndoc->int16_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->int32s,
                err,
                memfile,
                &columndoc->int32_prop_keys,
                FIELD_INT32,
                &columndoc->int32_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->int64s,
                err,
                memfile,
                &columndoc->int64_prop_keys,
                FIELD_INT64,
                &columndoc->int64_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->uint8s,
                err,
                memfile,
                &columndoc->uint8_prop_keys,
                FIELD_UINT8,
                &columndoc->uint8_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->uint16s,
                err,
                memfile,
                &columndoc->uint16_prop_keys,
                FIELD_UINT16,
                &columndoc->uint16_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->uint32s,
                err,
                memfile,
                &columndoc->uin32_prop_keys,
                FIELD_UINT32,
                &columndoc->uint32_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->uint64s,
                err,
                memfile,
                &columndoc->uint64_prop_keys,
                FIELD_UINT64,
                &columndoc->uint64_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->floats,
                err,
                memfile,
                &columndoc->float_prop_keys,
                FIELD_FLOAT,
                &columndoc->float_prop_vals)) {
                return false;
        }
        if (!write_fixed_props(&offsets->strings,
                err,
                memfile,
                &columndoc->string_prop_keys,
                FIELD_STRING,
                &columndoc->string_prop_vals)) {
                return false;
        }
        if (!write_var_props(&offsets->objects,
                err,
                memfile,
                &columndoc->obj_prop_keys,
                &columndoc->obj_prop_vals,
                root_object_header_offset)) {
                return false;
        }

        offsets->nulls -= root_object_header_offset;
        offsets->bools -= root_object_header_offset;
        offsets->int8s -= root_object_header_offset;
        offsets->int16s -= root_object_header_offset;
        offsets->int32s -= root_object_header_offset;
        offsets->int64s -= root_object_header_offset;
        offsets->uint8s -= root_object_header_offset;
        offsets->uint16s -= root_object_header_offset;
        offsets->uint32s -= root_object_header_offset;
        offsets->uint64s -= root_object_header_offset;
        offsets->floats -= root_object_header_offset;
        offsets->strings -= root_object_header_offset;
        offsets->objects -= root_object_header_offset;
        return true;
}

static bool write_column_entry(struct memfile *memfile, struct err *err, field_e type,
        struct vector ofType(<T>) *column, offset_t root_object_header_offset)
{
        memfile_write(memfile, &column->num_elems, sizeof(u32));
        switch (type) {
        case FIELD_NULL:
                memfile_write(memfile, column->base, column->num_elems * sizeof(u32));
                break;
        case FIELD_BOOLEAN:
        case FIELD_INT8:
        case FIELD_INT16:
        case FIELD_INT32:
        case FIELD_INT64:
        case FIELD_UINT8:
        case FIELD_UINT16:
        case FIELD_UINT32:
        case FIELD_UINT64:
        case FIELD_FLOAT:
        case FIELD_STRING:
                memfile_write(memfile, column->base, column->num_elems * GET_TYPE_SIZE(type));
                break;
        case FIELD_OBJECT: {
                offset_t preObjectNext = 0;
                for (size_t i = 0; i < column->num_elems; i++) {
                        struct columndoc_obj *object = vec_get(column, i, struct columndoc_obj);
                        if (likely(preObjectNext != 0)) {
                                offset_t continuePos = memfile_tell(memfile);
                                offset_t relativeContinuePos = continuePos - root_object_header_offset;
                                memfile_seek(memfile, preObjectNext);
                                memfile_write(memfile, &relativeContinuePos, sizeof(offset_t));
                                memfile_seek(memfile, continuePos);
                        }
                        if (!__serialize(&preObjectNext, err, memfile, object, root_object_header_offset)) {
                                return false;
                        }
                }
        }
                break;
        default: error(err, NG5_ERR_NOTYPE)
                return false;
        }
        return true;
}

static bool write_column(struct memfile *memfile, struct err *err, struct columndoc_column *column,
        offset_t root_object_header_offset)
{
        assert(column->array_positions.num_elems == column->values.num_elems);

        struct column_header header = {.marker = marker_symbols[MARKER_TYPE_COLUMN].symbol, .column_name = column
                ->key_name, .value_type = marker_symbols[value_array_marker_mapping[column->type].marker]
                .symbol, .num_entries = column->values.num_elems};

        memfile_write(memfile, &header, sizeof(struct column_header));

        /** skip offset column to value entry points */
        offset_t value_entry_offsets = memfile_tell(memfile);
        memfile_skip(memfile, column->values.num_elems * sizeof(offset_t));

        memfile_write(memfile, column->array_positions.base, column->array_positions.num_elems * sizeof(u32));

        for (size_t i = 0; i < column->values.num_elems; i++) {
                struct vector ofType(<T>) *column_data = vec_get(&column->values, i, struct vector);
                offset_t column_entry_offset = memfile_tell(memfile);
                offset_t relative_entry_offset = column_entry_offset - root_object_header_offset;
                memfile_seek(memfile, value_entry_offsets + i * sizeof(offset_t));
                memfile_write(memfile, &relative_entry_offset, sizeof(offset_t));
                memfile_seek(memfile, column_entry_offset);
                if (!write_column_entry(memfile, err, column->type, column_data, root_object_header_offset)) {
                        return false;
                }
        }
        return true;
}

static bool write_object_array_props(struct memfile *memfile, struct err *err,
        struct vector ofType(struct columndoc_group) *object_key_columns, struct archive_prop_offs *offsets,
        offset_t root_object_header_offset)
{
        if (object_key_columns->num_elems > 0) {
                struct object_array_header header = {.marker = marker_symbols[MARKER_TYPE_PROP_OBJECT_ARRAY]
                        .symbol, .num_entries = object_key_columns->num_elems};

                offsets->object_arrays = memfile_tell(memfile) - root_object_header_offset;
                memfile_write(memfile, &header, sizeof(struct object_array_header));

                for (size_t i = 0; i < object_key_columns->num_elems; i++) {
                        struct columndoc_group *column_group = vec_get(object_key_columns, i, struct columndoc_group);
                        memfile_write(memfile, &column_group->key, sizeof(field_sid_t));
                }

                // skip offset column to column groups
                offset_t column_offsets = memfile_tell(memfile);
                memfile_skip(memfile, object_key_columns->num_elems * sizeof(offset_t));

                for (size_t i = 0; i < object_key_columns->num_elems; i++) {
                        struct columndoc_group *column_group = vec_get(object_key_columns, i, struct columndoc_group);
                        offset_t this_column_offset_relative = memfile_tell(memfile) - root_object_header_offset;

                        /* write an object-id for each position number */
                        size_t max_pos = 0;
                        for (size_t k = 0; k < column_group->columns.num_elems; k++) {
                                struct columndoc_column
                                        *column = vec_get(&column_group->columns, k, struct columndoc_column);
                                const u32 *array_pos = vec_all(&column->array_positions, u32);
                                for (size_t m = 0; m < column->array_positions.num_elems; m++) {
                                        max_pos = ng5_max(max_pos, array_pos[m]);
                                }
                        }
                        struct column_group_header column_group_header =
                                {.marker = marker_symbols[MARKER_TYPE_COLUMN_GROUP].symbol, .num_columns = column_group
                                        ->columns.num_elems, .num_objects = max_pos + 1};
                        memfile_write(memfile, &column_group_header, sizeof(struct column_group_header));

                        for (size_t i = 0; i < column_group_header.num_objects; i++) {
                                object_id_t oid;
                                if (!object_id_create(&oid)) {
                                        error(err, NG5_ERR_THREADOOOBJIDS);
                                        return false;
                                }
                                memfile_write(memfile, &oid, sizeof(object_id_t));
                        }

                        offset_t continue_write = memfile_tell(memfile);
                        memfile_seek(memfile, column_offsets + i * sizeof(offset_t));
                        memfile_write(memfile, &this_column_offset_relative, sizeof(offset_t));
                        memfile_seek(memfile, continue_write);

                        offset_t offset_column_to_columns = continue_write;
                        memfile_skip(memfile, column_group->columns.num_elems * sizeof(offset_t));

                        for (size_t k = 0; k < column_group->columns.num_elems; k++) {
                                struct columndoc_column
                                        *column = vec_get(&column_group->columns, k, struct columndoc_column);
                                offset_t continue_write = memfile_tell(memfile);
                                offset_t column_off = continue_write - root_object_header_offset;
                                memfile_seek(memfile, offset_column_to_columns + k * sizeof(offset_t));
                                memfile_write(memfile, &column_off, sizeof(offset_t));
                                memfile_seek(memfile, continue_write);
                                if (!write_column(memfile, err, column, root_object_header_offset)) {
                                        return false;
                                }
                        }

                }
        } else {
                offsets->object_arrays = 0;
        }

        return true;
}

static offset_t skip_record_header(struct memfile *memfile)
{
        offset_t offset = memfile_tell(memfile);
        memfile_skip(memfile, sizeof(struct record_header));
        return offset;
}

static void update_record_header(struct memfile *memfile, offset_t root_object_header_offset, struct columndoc *model,
        u64 record_size)
{
        struct record_flags flags = {.bits.is_sorted = model->read_optimized};
        struct record_header
                header = {.marker = MARKER_SYMBOL_RECORD_HEADER, .flags = flags.value, .record_size = record_size};
        offset_t offset;
        memfile_get_offset(&offset, memfile);
        memfile_seek(memfile, root_object_header_offset);
        memfile_write(memfile, &header, sizeof(struct record_header));
        memfile_seek(memfile, offset);
}

static void propOffsetsWrite(struct memfile *memfile, const union object_flags *flags,
        struct archive_prop_offs *prop_offsets)
{
        if (flags->bits.has_null_props) {
                memfile_write(memfile, &prop_offsets->nulls, sizeof(offset_t));
        }
        if (flags->bits.has_bool_props) {
                memfile_write(memfile, &prop_offsets->bools, sizeof(offset_t));
        }
        if (flags->bits.has_int8_props) {
                memfile_write(memfile, &prop_offsets->int8s, sizeof(offset_t));
        }
        if (flags->bits.has_int16_props) {
                memfile_write(memfile, &prop_offsets->int16s, sizeof(offset_t));
        }
        if (flags->bits.has_int32_props) {
                memfile_write(memfile, &prop_offsets->int32s, sizeof(offset_t));
        }
        if (flags->bits.has_int64_props) {
                memfile_write(memfile, &prop_offsets->int64s, sizeof(offset_t));
        }
        if (flags->bits.has_uint8_props) {
                memfile_write(memfile, &prop_offsets->uint8s, sizeof(offset_t));
        }
        if (flags->bits.has_uint16_props) {
                memfile_write(memfile, &prop_offsets->uint16s, sizeof(offset_t));
        }
        if (flags->bits.has_uint32_props) {
                memfile_write(memfile, &prop_offsets->uint32s, sizeof(offset_t));
        }
        if (flags->bits.has_uint64_props) {
                memfile_write(memfile, &prop_offsets->uint64s, sizeof(offset_t));
        }
        if (flags->bits.has_float_props) {
                memfile_write(memfile, &prop_offsets->floats, sizeof(offset_t));
        }
        if (flags->bits.has_string_props) {
                memfile_write(memfile, &prop_offsets->strings, sizeof(offset_t));
        }
        if (flags->bits.has_object_props) {
                memfile_write(memfile, &prop_offsets->objects, sizeof(offset_t));
        }
        if (flags->bits.has_null_array_props) {
                memfile_write(memfile, &prop_offsets->null_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_bool_array_props) {
                memfile_write(memfile, &prop_offsets->bool_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_int8_array_props) {
                memfile_write(memfile, &prop_offsets->int8_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_int16_array_props) {
                memfile_write(memfile, &prop_offsets->int16_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_int32_array_props) {
                memfile_write(memfile, &prop_offsets->int32_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_int64_array_props) {
                memfile_write(memfile, &prop_offsets->int64_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_uint8_array_props) {
                memfile_write(memfile, &prop_offsets->uint8_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_uint16_array_props) {
                memfile_write(memfile, &prop_offsets->uint16_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_uint32_array_props) {
                memfile_write(memfile, &prop_offsets->uint32_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_uint64_array_props) {
                memfile_write(memfile, &prop_offsets->uint64_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_float_array_props) {
                memfile_write(memfile, &prop_offsets->float_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_string_array_props) {
                memfile_write(memfile, &prop_offsets->string_arrays, sizeof(offset_t));
        }
        if (flags->bits.has_object_array_props) {
                memfile_write(memfile, &prop_offsets->object_arrays, sizeof(offset_t));
        }
}

static void prop_offsets_skip_write(struct memfile *memfile, const union object_flags *flags)
{
        unsigned num_skip_offset_bytes = 0;
        if (flags->bits.has_null_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_bool_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int8_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int16_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int32_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int64_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint8_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint16_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint32_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint64_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_float_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_string_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_object_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_null_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_bool_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int8_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int16_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int32_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_int64_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint8_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint16_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint32_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_uint64_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_float_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_string_array_props) {
                num_skip_offset_bytes++;
        }
        if (flags->bits.has_object_array_props) {
                num_skip_offset_bytes++;
        }

        memfile_skip(memfile, num_skip_offset_bytes * sizeof(offset_t));
}

static bool __serialize(offset_t *offset, struct err *err, struct memfile *memfile, struct columndoc_obj *columndoc,
        offset_t root_object_header_offset)
{
        union object_flags flags;
        struct archive_prop_offs prop_offsets;
        get_flags(&flags, columndoc);

        offset_t header_offset = memfile_tell(memfile);
        memfile_skip(memfile, sizeof(struct object_header));

        prop_offsets_skip_write(memfile, &flags);
        offset_t next_offset = memfile_tell(memfile);
        offset_t default_next_nil = 0;
        memfile_write(memfile, &default_next_nil, sizeof(offset_t));

        if (!write_primitive_props(memfile, err, columndoc, &prop_offsets, root_object_header_offset)) {
                return false;
        }
        if (!write_array_props(memfile, err, columndoc, &prop_offsets, root_object_header_offset)) {
                return false;
        }
        if (!write_object_array_props(memfile,
                err,
                &columndoc->obj_array_props,
                &prop_offsets,
                root_object_header_offset)) {
                return false;
        }

        memfile_write(memfile, &marker_symbols[MARKER_TYPE_OBJECT_END].symbol, 1);

        offset_t object_end_offset = memfile_tell(memfile);
        memfile_seek(memfile, header_offset);

        object_id_t oid;
        if (!object_id_create(&oid)) {
                error(err, NG5_ERR_THREADOOOBJIDS);
                return false;
        }

        struct object_header header =
                {.marker = marker_symbols[MARKER_TYPE_OBJECT_BEGIN].symbol, .oid = oid, .flags = flags_to_int32(&flags),

                };

        memfile_write(memfile, &header, sizeof(struct object_header));

        propOffsetsWrite(memfile, &flags, &prop_offsets);

        memfile_seek(memfile, object_end_offset);
        ng5_optional_set(offset, next_offset);
        return true;
}

static char *embedded_dic_flags_to_string(const union string_tab_flags *flags)
{
        size_t max = 2048;
        char *string = malloc(max + 1);
        size_t length = 0;

        if (flags->value == 0) {
                strcpy(string, " uncompressed");
                length = strlen(string);
                assert(length <= max);
        } else {

                for (size_t i = 0; i < pack_get_num_registered_strategies(); i++) {
                        if (flags->value & compressor_strategy_register[i].flag_bit) {
                                strcpy(string + length, compressor_strategy_register[i].name);
                                length = strlen(string);
                                strcpy(string + length, " ");
                                length = strlen(string);
                        }
                }
        }
        string[length] = '\0';
        return string;
}

static char *record_header_flags_to_string(const struct record_flags *flags)
{
        size_t max = 2048;
        char *string = malloc(max + 1);
        size_t length = 0;

        if (flags->value == 0) {
                strcpy(string, " none");
                length = strlen(string);
                assert(length <= max);
        } else {
                if (flags->bits.is_sorted) {
                        strcpy(string + length, " sorted");
                        length = strlen(string);
                        assert(length <= max);
                }
        }
        string[length] = '\0';
        return string;
}

static bool serialize_string_dic(struct memfile *memfile, struct err *err, const struct doc_bulk *context,
        enum packer_type compressor)
{
        union string_tab_flags flags;
        struct packer strategy;
        struct string_table_header header;

        struct vector ofType (const char *) *strings;
        struct vector ofType(field_sid_t) *string_ids;

        doc_bulk_get_dic_contents(&strings, &string_ids, context);

        assert(strings->num_elems == string_ids->num_elems);

        flags.value = 0;
        if (!pack_by_type(err, &strategy, compressor)) {
                return false;
        }
        u8 flag_bit = pack_flagbit_by_type(compressor);
        ng5_set_bits(flags.value, flag_bit);

        offset_t header_pos = memfile_tell(memfile);
        memfile_skip(memfile, sizeof(struct string_table_header));

        offset_t extra_begin_off = memfile_tell(memfile);
        pack_write_extra(err, &strategy, memfile, strings);
        offset_t extra_end_off = memfile_tell(memfile);

        header = (struct string_table_header) {.marker = marker_symbols[MARKER_TYPE_EMBEDDED_STR_DIC]
                .symbol, .flags = flags.value, .num_entries = strings
                ->num_elems, .first_entry = memfile_tell(memfile), .compressor_extra_size = (extra_end_off
                - extra_begin_off)};

        for (size_t i = 0; i < strings->num_elems; i++) {
                field_sid_t id = *vec_get(string_ids, i, field_sid_t);
                const char *string = *vec_get(strings, i, char *);

                struct string_entry_header header = {.marker = marker_symbols[MARKER_TYPE_EMBEDDED_UNCOMP_STR]
                        .symbol, .next_entry_off = 0, .string_id = id, .string_len = strlen(string)};

                offset_t header_pos_off = memfile_tell(memfile);
                memfile_skip(memfile, sizeof(struct string_entry_header));

                if (!pack_encode(err, &strategy, memfile, string)) {
                        error_print(err.code);
                        return false;
                }
                offset_t continue_off = memfile_tell(memfile);
                memfile_seek(memfile, header_pos_off);
                header.next_entry_off = i + 1 < strings->num_elems ? continue_off : 0;
                memfile_write(memfile, &header, sizeof(struct string_entry_header));
                memfile_seek(memfile, continue_off);
        }

        offset_t continue_pos = memfile_tell(memfile);
        memfile_seek(memfile, header_pos);
        memfile_write(memfile, &header, sizeof(struct string_table_header));
        memfile_seek(memfile, continue_pos);

        vec_drop(strings);
        vec_drop(string_ids);
        free(strings);
        free(string_ids);

        return pack_drop(err, &strategy);
}

static void skip_file_header(struct memfile *memfile)
{
        memfile_skip(memfile, sizeof(struct archive_header));
}

static void update_file_header(struct memfile *memfile, offset_t record_header_offset)
{
        offset_t current_pos;
        memfile_get_offset(&current_pos, memfile);
        memfile_seek(memfile, 0);
        memcpy(&this_file_header.magic, CARBON_ARCHIVE_MAGIC, strlen(CARBON_ARCHIVE_MAGIC));
        this_file_header.root_object_header_offset = record_header_offset;
        this_file_header.string_id_to_offset_index_offset = 0;
        memfile_write(memfile, &this_file_header, sizeof(struct archive_header));
        memfile_seek(memfile, current_pos);
}

static bool print_column_form_memfile(FILE *file, struct err *err, struct memfile *memfile, unsigned nesting_level)
{
        offset_t offset;
        memfile_get_offset(&offset, memfile);
        struct column_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct column_header);
        if (header->marker != MARKER_SYMBOL_COLUMN) {
                char buffer[256];
                sprintf(buffer, "expected marker [%c] but found [%c]", MARKER_SYMBOL_COLUMN, header->marker);
                error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                return false;
        }
        fprintf(file, "0x%04x ", (unsigned) offset);
        INTENT_LINE(nesting_level);
        const char *type_name = array_value_type_to_string(err, int_marker_to_field_type(header->value_type));
        if (!type_name) {
                return false;
        }

        fprintf(file,
                "[marker: %c (Column)] [column_name: '%"PRIu64"'] [value_type: %c (%s)] [nentries: %d] [",
                header->marker,
                header->column_name,
                header->value_type,
                type_name,
                header->num_entries);

        for (size_t i = 0; i < header->num_entries; i++) {
                offset_t entry_off = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
                fprintf(file, "offset: 0x%04x%s", (unsigned) entry_off, i + 1 < header->num_entries ? ", " : "");
        }

        u32 *positions = (u32 *) NG5_MEMFILE_READ(memfile, header->num_entries * sizeof(u32));
        fprintf(file, "] [positions: [");
        for (size_t i = 0; i < header->num_entries; i++) {
                fprintf(file, "%d%s", positions[i], i + 1 < header->num_entries ? ", " : "");
        }

        fprintf(file, "]]\n");

        field_e data_type = int_marker_to_field_type(header->value_type);

        //fprintf(file, "[");
        for (size_t i = 0; i < header->num_entries; i++) {
                switch (data_type) {
                case FIELD_NULL: {
                        PRINT_VALUE_ARRAY(u32, memfile, header, "%d");
                }
                        break;
                case FIELD_BOOLEAN: {
                        PRINT_VALUE_ARRAY(FIELD_BOOLEANean_t, memfile, header, "%d");
                }
                        break;
                case FIELD_INT8: {
                        PRINT_VALUE_ARRAY(field_i8_t, memfile, header, "%d");
                }
                        break;
                case FIELD_INT16: {
                        PRINT_VALUE_ARRAY(field_i16_t, memfile, header, "%d");
                }
                        break;
                case FIELD_INT32: {
                        PRINT_VALUE_ARRAY(field_i32_t, memfile, header, "%d");
                }
                        break;
                case FIELD_INT64: {
                        PRINT_VALUE_ARRAY(field_i64_t, memfile, header, "%"
                                PRIi64);
                }
                        break;
                case FIELD_UINT8: {
                        PRINT_VALUE_ARRAY(field_u8_t, memfile, header, "%d");
                }
                        break;
                case FIELD_UINT16: {
                        PRINT_VALUE_ARRAY(field_u16_t, memfile, header, "%d");
                }
                        break;
                case FIELD_UINT32: {
                        PRINT_VALUE_ARRAY(field_u32_t, memfile, header, "%d");
                }
                        break;
                case FIELD_UINT64: {
                        PRINT_VALUE_ARRAY(field_u64_t, memfile, header, "%"
                                PRIu64);
                }
                        break;
                case FIELD_FLOAT: {
                        PRINT_VALUE_ARRAY(field_number_t, memfile, header, "%f");
                }
                        break;
                case FIELD_STRING: {
                        PRINT_VALUE_ARRAY(field_sid_t, memfile, header, "%"
                                PRIu64
                                "");
                }
                        break;
                case FIELD_OBJECT: {
                        u32 num_elements = *NG5_MEMFILE_READ_TYPE(memfile, u32);
                        INTENT_LINE(nesting_level);
                        fprintf(file, "   [num_elements: %d] [values: [\n", num_elements);
                        for (size_t i = 0; i < num_elements; i++) {
                                if (!print_object(file, err, memfile, nesting_level + 2)) {
                                        return false;
                                }
                        }
                        INTENT_LINE(nesting_level);
                        fprintf(file, "   ]\n");
                }
                        break;
                default: error(err, NG5_ERR_NOTYPE)
                        return false;
                }
        }
        return true;
}

static bool print_object_array_from_memfile(FILE *file, struct err *err, struct memfile *memfile,
        unsigned nesting_level)
{
        unsigned offset = (unsigned) memfile_tell(memfile);
        struct object_array_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct object_array_header);
        if (header->marker != MARKER_SYMBOL_PROP_OBJECT_ARRAY) {
                char buffer[256];
                sprintf(buffer, "expected marker [%c] but found [%c]", MARKER_SYMBOL_PROP_OBJECT_ARRAY, header->marker);
                error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                return false;
        }

        fprintf(file, "0x%04x ", offset);
        INTENT_LINE(nesting_level);
        fprintf(file, "[marker: %c (Object Array)] [nentries: %d] [", header->marker, header->num_entries);

        for (size_t i = 0; i < header->num_entries; i++) {
                field_sid_t string_id = *NG5_MEMFILE_READ_TYPE(memfile, field_sid_t);
                fprintf(file, "key: %"PRIu64"%s", string_id, i + 1 < header->num_entries ? ", " : "");
        }
        fprintf(file, "] [");
        for (size_t i = 0; i < header->num_entries; i++) {
                offset_t columnGroupOffset = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
                fprintf(file,
                        "offset: 0x%04x%s",
                        (unsigned) columnGroupOffset,
                        i + 1 < header->num_entries ? ", " : "");
        }

        fprintf(file, "]\n");
        nesting_level++;

        for (size_t i = 0; i < header->num_entries; i++) {
                offset = memfile_tell(memfile);
                struct column_group_header
                        *column_group_header = NG5_MEMFILE_READ_TYPE(memfile, struct column_group_header);
                if (column_group_header->marker != MARKER_SYMBOL_COLUMN_GROUP) {
                        char buffer[256];
                        sprintf(buffer,
                                "expected marker [%c] but found [%c]",
                                MARKER_SYMBOL_COLUMN_GROUP,
                                column_group_header->marker);
                        error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                        return false;
                }
                fprintf(file, "0x%04x ", offset);
                INTENT_LINE(nesting_level);
                fprintf(file,
                        "[marker: %c (Column Group)] [num_columns: %d] [num_objects: %d] [object_ids: ",
                        column_group_header->marker,
                        column_group_header->num_columns,
                        column_group_header->num_objects);
                const object_id_t
                        *oids = NG5_MEMFILE_READ_TYPE_LIST(memfile, object_id_t, column_group_header->num_objects);
                for (size_t k = 0; k < column_group_header->num_objects; k++) {
                        fprintf(file, "%"PRIu64"%s", oids[k], k + 1 < column_group_header->num_objects ? ", " : "");
                }
                fprintf(file, "] [offsets: ");
                for (size_t k = 0; k < column_group_header->num_columns; k++) {
                        offset_t column_off = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
                        fprintf(file,
                                "0x%04x%s",
                                (unsigned) column_off,
                                k + 1 < column_group_header->num_columns ? ", " : "");
                }

                fprintf(file, "]\n");

                for (size_t k = 0; k < column_group_header->num_columns; k++) {
                        if (!print_column_form_memfile(file, err, memfile, nesting_level + 1)) {
                                return false;
                        }
                }

                fprintf(file, "0x%04x ", offset);
                INTENT_LINE(nesting_level);
                fprintf(file, "]\n");
        }
        return true;
}

static void print_prop_offsets(FILE *file, const union object_flags *flags,
        const struct archive_prop_offs *prop_offsets)
{
        if (flags->bits.has_null_props) {
                fprintf(file, " nulls: 0x%04x", (unsigned) prop_offsets->nulls);
        }
        if (flags->bits.has_bool_props) {
                fprintf(file, " bools: 0x%04x", (unsigned) prop_offsets->bools);
        }
        if (flags->bits.has_int8_props) {
                fprintf(file, " int8s: 0x%04x", (unsigned) prop_offsets->int8s);
        }
        if (flags->bits.has_int16_props) {
                fprintf(file, " int16s: 0x%04x", (unsigned) prop_offsets->int16s);
        }
        if (flags->bits.has_int32_props) {
                fprintf(file, " int32s: 0x%04x", (unsigned) prop_offsets->int32s);
        }
        if (flags->bits.has_int64_props) {
                fprintf(file, " int64s: 0x%04x", (unsigned) prop_offsets->int64s);
        }
        if (flags->bits.has_uint8_props) {
                fprintf(file, " uint8s: 0x%04x", (unsigned) prop_offsets->uint8s);
        }
        if (flags->bits.has_uint16_props) {
                fprintf(file, " uint16s: 0x%04x", (unsigned) prop_offsets->uint16s);
        }
        if (flags->bits.has_uint32_props) {
                fprintf(file, " uint32s: 0x%04x", (unsigned) prop_offsets->uint32s);
        }
        if (flags->bits.has_uint64_props) {
                fprintf(file, " uint64s: 0x%04x", (unsigned) prop_offsets->uint64s);
        }
        if (flags->bits.has_float_props) {
                fprintf(file, " floats: 0x%04x", (unsigned) prop_offsets->floats);
        }
        if (flags->bits.has_string_props) {
                fprintf(file, " texts: 0x%04x", (unsigned) prop_offsets->strings);
        }
        if (flags->bits.has_object_props) {
                fprintf(file, " objects: 0x%04x", (unsigned) prop_offsets->objects);
        }
        if (flags->bits.has_null_array_props) {
                fprintf(file, " nullArrays: 0x%04x", (unsigned) prop_offsets->null_arrays);
        }
        if (flags->bits.has_bool_array_props) {
                fprintf(file, " boolArrays: 0x%04x", (unsigned) prop_offsets->bool_arrays);
        }
        if (flags->bits.has_int8_array_props) {
                fprintf(file, " int8Arrays: 0x%04x", (unsigned) prop_offsets->int8_arrays);
        }
        if (flags->bits.has_int16_array_props) {
                fprintf(file, " int16Arrays: 0x%04x", (unsigned) prop_offsets->int16_arrays);
        }
        if (flags->bits.has_int32_array_props) {
                fprintf(file, " int32Arrays: 0x%04x", (unsigned) prop_offsets->int32_arrays);
        }
        if (flags->bits.has_int64_array_props) {
                fprintf(file, " int16Arrays: 0x%04x", (unsigned) prop_offsets->int64_arrays);
        }
        if (flags->bits.has_uint8_array_props) {
                fprintf(file, " uint8Arrays: 0x%04x", (unsigned) prop_offsets->uint8_arrays);
        }
        if (flags->bits.has_uint16_array_props) {
                fprintf(file, " uint16Arrays: 0x%04x", (unsigned) prop_offsets->uint16_arrays);
        }
        if (flags->bits.has_uint32_array_props) {
                fprintf(file, " uint32Arrays: 0x%04x", (unsigned) prop_offsets->uint32_arrays);
        }
        if (flags->bits.has_uint64_array_props) {
                fprintf(file, " uint64Arrays: 0x%04x", (unsigned) prop_offsets->uint64_arrays);
        }
        if (flags->bits.has_float_array_props) {
                fprintf(file, " floatArrays: 0x%04x", (unsigned) prop_offsets->float_arrays);
        }
        if (flags->bits.has_string_array_props) {
                fprintf(file, " textArrays: 0x%04x", (unsigned) prop_offsets->string_arrays);
        }
        if (flags->bits.has_object_array_props) {
                fprintf(file, " objectArrays: 0x%04x", (unsigned) prop_offsets->object_arrays);
        }
}

bool print_object(FILE *file, struct err *err, struct memfile *memfile, unsigned nesting_level)
{
        unsigned offset = (unsigned) memfile_tell(memfile);
        struct object_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct object_header);

        struct archive_prop_offs prop_offsets;
        union object_flags flags = {.value = header->flags};

        int_read_prop_offsets(&prop_offsets, memfile, &flags);
        offset_t nextObjectOrNil = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);

        if (header->marker != MARKER_SYMBOL_OBJECT_BEGIN) {
                char buffer[256];
                sprintf(buffer, "Parsing error: expected object marker [{] but found [%c]\"", header->marker);
                error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                return false;
        }

        fprintf(file, "0x%04x ", offset);
        INTENT_LINE(nesting_level);
        nesting_level++;
        fprintf(file,
                "[marker: %c (BeginObject)] [object-id: %"PRIu64"] [flags: %u] [propertyOffsets: [",
                header->marker,
                header->oid,
                header->flags);
        print_prop_offsets(file, &flags, &prop_offsets);
        fprintf(file, " ] [next: 0x%04x] \n", (unsigned) nextObjectOrNil);

        bool continue_read = true;
        while (continue_read) {
                offset = memfile_tell(memfile);
                char entryMarker = *NG5_MEMFILE_PEEK(memfile, char);

                switch (entryMarker) {
                case MARKER_SYMBOL_PROP_NULL: {
                        struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
                        field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile,
                                prop_header->num_entries * sizeof(field_sid_t));
                        fprintf(file, "0x%04x ", offset);
                        INTENT_LINE(nesting_level)
                        fprintf(file, "[marker: %c (null)] [nentries: %d] [", entryMarker, prop_header->num_entries);

                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file, "%"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");
                        }
                        fprintf(file, "]\n");
                }
                        break;
                case MARKER_SYMBOL_PROP_BOOLEAN: {
                        struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
                        field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile,
                                prop_header->num_entries * sizeof(field_sid_t));
                        FIELD_BOOLEANean_t *values = (FIELD_BOOLEANean_t *) NG5_MEMFILE_READ(memfile,
                                prop_header->num_entries * sizeof(FIELD_BOOLEANean_t));
                        fprintf(file, "0x%04x ", offset);
                        INTENT_LINE(nesting_level)
                        fprintf(file, "[marker: %c (boolean)] [nentries: %d] [", entryMarker, prop_header->num_entries);
                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file, "%"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");
                        }
                        fprintf(file, "] [");
                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file,
                                        "%s%s",
                                        values[i] ? "true" : "false",
                                        i + 1 < prop_header->num_entries ? ", " : "");
                        }
                        fprintf(file, "]\n");
                }
                        break;
                case MARKER_SYMBOL_PROP_INT8: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_i8_t,
                        "Int8",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_INT16: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_i16_t,
                        "Int16",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_INT32: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_i32_t,
                        "Int32",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_INT64: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_i64_t,
                        "Int64",
                        "%"
                        PRIi64);
                        break;
                case MARKER_SYMBOL_PROP_UINT8: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_u8_t,
                        "UInt8",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT16: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_u16_t,
                        "UInt16",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT32: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_u32_t,
                        "UInt32",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT64: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_u64_t,
                        "UInt64",
                        "%"
                        PRIu64);
                        break;
                case MARKER_SYMBOL_PROP_REAL: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_number_t,
                        "Float",
                        "%f");
                        break;
                case MARKER_SYMBOL_PROP_TEXT: PRINT_SIMPLE_PROPS(file,
                        memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        field_sid_t,
                        "Text",
                        "%"
                        PRIu64
                        "");
                        break;
                case MARKER_SYMBOL_PROP_OBJECT: {
                        struct var_prop prop;
                        int_embedded_var_props_read(&prop, memfile);
                        fprintf(file, "0x%04x ", offset);
                        INTENT_LINE(nesting_level)
                        fprintf(file, "[marker: %c (Object)] [nentries: %d] [", entryMarker, prop.header->num_entries);
                        for (u32 i = 0; i < prop.header->num_entries; i++) {
                                fprintf(file,
                                        "key: %"PRIu64"%s",
                                        prop.keys[i],
                                        i + 1 < prop.header->num_entries ? ", " : "");
                        }
                        fprintf(file, "] [");
                        for (u32 i = 0; i < prop.header->num_entries; i++) {
                                fprintf(file,
                                        "offsets: 0x%04x%s",
                                        (unsigned) prop.offsets[i],
                                        i + 1 < prop.header->num_entries ? ", " : "");
                        }
                        fprintf(file, "] [\n");

                        char nextEntryMarker;
                        do {
                                if (!print_object(file, err, memfile, nesting_level + 1)) {
                                        return false;
                                }
                                nextEntryMarker = *NG5_MEMFILE_PEEK(memfile, char);
                        }
                        while (nextEntryMarker == MARKER_SYMBOL_OBJECT_BEGIN);

                }
                        break;
                case MARKER_SYMBOL_PROP_NULL_ARRAY: {
                        struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);

                        field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile,
                                prop_header->num_entries * sizeof(field_sid_t));
                        u32 *nullArrayLengths;

                        fprintf(file, "0x%04x ", offset);
                        INTENT_LINE(nesting_level)
                        fprintf(file,
                                "[marker: %c (Null Array)] [nentries: %d] [",
                                entryMarker,
                                prop_header->num_entries);

                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file, "%"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");
                        }
                        fprintf(file, "] [");

                        nullArrayLengths = (u32 *) NG5_MEMFILE_READ(memfile, prop_header->num_entries * sizeof(u32));

                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file,
                                        "nentries: %d%s",
                                        nullArrayLengths[i],
                                        i + 1 < prop_header->num_entries ? ", " : "");
                        }

                        fprintf(file, "]\n");
                }
                        break;
                case MARKER_SYMBOL_PROP_BOOLEAN_ARRAY: {
                        struct prop_header *prop_header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);

                        field_sid_t *keys = (field_sid_t *) NG5_MEMFILE_READ(memfile,
                                prop_header->num_entries * sizeof(field_sid_t));
                        u32 *array_lengths;

                        fprintf(file, "0x%04x ", offset);
                        INTENT_LINE(nesting_level)
                        fprintf(file,
                                "[marker: %c (Boolean Array)] [nentries: %d] [",
                                entryMarker,
                                prop_header->num_entries);

                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file, "%"PRIu64"%s", keys[i], i + 1 < prop_header->num_entries ? ", " : "");
                        }
                        fprintf(file, "] [");

                        array_lengths = (u32 *) NG5_MEMFILE_READ(memfile, prop_header->num_entries * sizeof(u32));

                        for (u32 i = 0; i < prop_header->num_entries; i++) {
                                fprintf(file,
                                        "arrayLength: %d%s",
                                        array_lengths[i],
                                        i + 1 < prop_header->num_entries ? ", " : "");
                        }

                        fprintf(file, "] [");

                        for (u32 array_idx = 0; array_idx < prop_header->num_entries; array_idx++) {
                                FIELD_BOOLEANean_t *values = (FIELD_BOOLEANean_t *) NG5_MEMFILE_READ(memfile,
                                        array_lengths[array_idx] * sizeof(FIELD_BOOLEANean_t));
                                fprintf(file, "[");
                                for (u32 i = 0; i < array_lengths[array_idx]; i++) {
                                        fprintf(file,
                                                "value: %s%s",
                                                values[i] ? "true" : "false",
                                                i + 1 < array_lengths[array_idx] ? ", " : "");
                                }
                                fprintf(file, "]%s", array_idx + 1 < prop_header->num_entries ? ", " : "");
                        }

                        fprintf(file, "]\n");
                }
                        break;
                        break;
                case MARKER_SYMBOL_PROP_INT8_ARRAY: {
                        PRINT_ARRAY_PROPS(memfile,
                                memfile_tell(memfile),
                                nesting_level,
                                entryMarker,
                                field_i8_t,
                                "Int8 Array",
                                "%d");
                }
                        break;
                case MARKER_SYMBOL_PROP_INT16_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_i16_t,
                        "Int16 Array",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_INT32_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_i32_t,
                        "Int32 Array",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_INT64_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_i64_t,
                        "Int64 Array",
                        "%"
                        PRIi64);
                        break;
                case MARKER_SYMBOL_PROP_UINT8_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_u8_t,
                        "UInt8 Array",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT16_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_u16_t,
                        "UInt16 Array",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT32_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_u32_t,
                        "UInt32 Array",
                        "%d");
                        break;
                case MARKER_SYMBOL_PROP_UINT64_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_u64_t,
                        "UInt64 Array",
                        "%"
                        PRIu64);
                        break;
                case MARKER_SYMBOL_PROP_REAL_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_number_t,
                        "Float Array",
                        "%f");
                        break;
                case MARKER_SYMBOL_PROP_TEXT_ARRAY: PRINT_ARRAY_PROPS(memfile,
                        memfile_tell(memfile),
                        nesting_level,
                        entryMarker,
                        field_sid_t,
                        "Text Array",
                        "%"
                        PRIu64
                        "");
                        break;
                case MARKER_SYMBOL_PROP_OBJECT_ARRAY:
                        if (!print_object_array_from_memfile(file, err, memfile, nesting_level)) {
                                return false;
                        }
                        break;
                case MARKER_SYMBOL_OBJECT_END:
                        continue_read = false;
                        break;
                default: {
                        char buffer[256];
                        sprintf(buffer,
                                "Parsing error: unexpected marker [%c] was detected in file %p",
                                entryMarker,
                                memfile);
                        error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                        return false;
                }
                }
        }

        offset = memfile_tell(memfile);
        char end_marker = *NG5_MEMFILE_READ_TYPE(memfile, char);
        assert (end_marker == MARKER_SYMBOL_OBJECT_END);
        nesting_level--;
        fprintf(file, "0x%04x ", offset);
        INTENT_LINE(nesting_level);
        fprintf(file, "[marker: %c (EndObject)]\n", end_marker);
        return true;
}

static bool is_valid_file(const struct archive_header *header)
{
        if (NG5_ARRAY_LENGTH(header->magic) != strlen(CARBON_ARCHIVE_MAGIC)) {
                return false;
        } else {
                for (size_t i = 0; i < NG5_ARRAY_LENGTH(header->magic); i++) {
                        if (header->magic[i] != CARBON_ARCHIVE_MAGIC[i]) {
                                return false;
                        }
                }
                if (header->version != CARBON_ARCHIVE_VERSION) {
                        return false;
                }
                if (header->root_object_header_offset == 0) {
                        return false;
                }
                return true;
        }
}

static void print_record_header_from_memfile(FILE *file, struct memfile *memfile)
{
        unsigned offset = memfile_tell(memfile);
        struct record_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct record_header);
        struct record_flags flags;
        memset(&flags, 0, sizeof(struct record_flags));
        flags.value = header->flags;
        char *flags_string = record_header_flags_to_string(&flags);
        fprintf(file, "0x%04x ", offset);
        fprintf(file,
                "[marker: %c] [flags: %s] [record_size: 0x%04x]\n",
                header->marker,
                flags_string,
                (unsigned) header->record_size);
        free(flags_string);
}

static bool print_header_from_memfile(FILE *file, struct err *err, struct memfile *memfile)
{
        unsigned offset = memfile_tell(memfile);
        assert(memfile_size(memfile) > sizeof(struct archive_header));
        struct archive_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct archive_header);
        if (!is_valid_file(header)) {
                error(err, NG5_ERR_NOARCHIVEFILE)
                return false;
        }

        fprintf(file, "0x%04x ", offset);
        fprintf(file,
                "[magic: " CARBON_ARCHIVE_MAGIC "] [version: %d] [recordOffset: 0x%04x] [string-id-offset-index: 0x%04x]\n",
                header->version,
                (unsigned) header->root_object_header_offset,
                (unsigned) header->string_id_to_offset_index_offset);
        return true;
}

static bool print_embedded_dic_from_memfile(FILE *file, struct err *err, struct memfile *memfile)
{
        struct packer strategy;
        union string_tab_flags flags;

        unsigned offset = memfile_tell(memfile);
        struct string_table_header *header = NG5_MEMFILE_READ_TYPE(memfile, struct string_table_header);
        if (header->marker != marker_symbols[MARKER_TYPE_EMBEDDED_STR_DIC].symbol) {
                char buffer[256];
                sprintf(buffer,
                        "expected [%c] marker, but found [%c]",
                        marker_symbols[MARKER_TYPE_EMBEDDED_STR_DIC].symbol,
                        header->marker);
                error_with_details(err, NG5_ERR_CORRUPTED, buffer);
                return false;
        }
        flags.value = header->flags;

        char *flagsStr = embedded_dic_flags_to_string(&flags);
        fprintf(file, "0x%04x ", offset);
        fprintf(file,
                "[marker: %c] [nentries: %d] [flags: %s] [first-entry-off: 0x%04x] [extra-size: %" PRIu64 "]\n",
                header->marker,
                header->num_entries,
                flagsStr,
                (unsigned) header->first_entry,
                header->compressor_extra_size);
        free(flagsStr);

        if (pack_by_flags(&strategy, flags.value) != true) {
                error(err, NG5_ERR_NOCOMPRESSOR);
                return false;
        }

        pack_print_extra(err, &strategy, file, memfile);

        while ((*NG5_MEMFILE_PEEK(memfile, char)) == marker_symbols[MARKER_TYPE_EMBEDDED_UNCOMP_STR].symbol) {
                unsigned offset = memfile_tell(memfile);
                struct string_entry_header header = *NG5_MEMFILE_READ_TYPE(memfile, struct string_entry_header);
                fprintf(file,
                        "0x%04x    [marker: %c] [next-entry-off: 0x%04zx] [string-id: %"PRIu64"] [string-length: %"PRIu32"]",
                        offset,
                        header.marker,
                        (size_t) header.next_entry_off,
                        header.string_id,
                        header.string_len);
                pack_print_encoded(err, &strategy, file, memfile, header.string_len);
                fprintf(file, "\n");
        }

        return pack_drop(err, &strategy);
}

static bool print_archive_from_memfile(FILE *file, struct err *err, struct memfile *memfile)
{
        if (!print_header_from_memfile(file, err, memfile)) {
                return false;
        }
        if (!print_embedded_dic_from_memfile(file, err, memfile)) {
                return false;
        }
        print_record_header_from_memfile(file, memfile);
        if (!print_object(file, err, memfile, 0)) {
                return false;
        }
        return true;
}

static union object_flags *get_flags(union object_flags *flags, struct columndoc_obj *columndoc)
{
        ng5_zero_memory(flags, sizeof(union object_flags));
        flags->bits.has_null_props = (columndoc->null_prop_keys.num_elems > 0);
        flags->bits.has_bool_props = (columndoc->bool_prop_keys.num_elems > 0);
        flags->bits.has_int8_props = (columndoc->int8_prop_keys.num_elems > 0);
        flags->bits.has_int16_props = (columndoc->int16_prop_keys.num_elems > 0);
        flags->bits.has_int32_props = (columndoc->int32_prop_keys.num_elems > 0);
        flags->bits.has_int64_props = (columndoc->int64_prop_keys.num_elems > 0);
        flags->bits.has_uint8_props = (columndoc->uint8_prop_keys.num_elems > 0);
        flags->bits.has_uint16_props = (columndoc->uint16_prop_keys.num_elems > 0);
        flags->bits.has_uint32_props = (columndoc->uin32_prop_keys.num_elems > 0);
        flags->bits.has_uint64_props = (columndoc->uint64_prop_keys.num_elems > 0);
        flags->bits.has_float_props = (columndoc->float_prop_keys.num_elems > 0);
        flags->bits.has_string_props = (columndoc->string_prop_keys.num_elems > 0);
        flags->bits.has_object_props = (columndoc->obj_prop_keys.num_elems > 0);
        flags->bits.has_null_array_props = (columndoc->null_array_prop_keys.num_elems > 0);
        flags->bits.has_bool_array_props = (columndoc->bool_array_prop_keys.num_elems > 0);
        flags->bits.has_int8_array_props = (columndoc->int8_array_prop_keys.num_elems > 0);
        flags->bits.has_int16_array_props = (columndoc->int16_array_prop_keys.num_elems > 0);
        flags->bits.has_int32_array_props = (columndoc->int32_array_prop_keys.num_elems > 0);
        flags->bits.has_int64_array_props = (columndoc->int64_array_prop_keys.num_elems > 0);
        flags->bits.has_uint8_array_props = (columndoc->uint8_array_prop_keys.num_elems > 0);
        flags->bits.has_uint16_array_props = (columndoc->uint16_array_prop_keys.num_elems > 0);
        flags->bits.has_uint32_array_props = (columndoc->uint32_array_prop_keys.num_elems > 0);
        flags->bits.has_uint64_array_props = (columndoc->uint64_array_prop_keys.num_elems > 0);
        flags->bits.has_float_array_props = (columndoc->float_array_prop_keys.num_elems > 0);
        flags->bits.has_string_array_props = (columndoc->string_array_prop_keys.num_elems > 0);
        flags->bits.has_object_array_props = (columndoc->obj_array_props.num_elems > 0);
        //assert(flags->value != 0);
        return flags;
}

static bool init_decompressor(struct packer *strategy, u8 flags);

static bool read_stringtable(struct string_table *table, struct err *err, FILE *disk_file);

static bool read_record(struct record_header *header_read, struct archive *archive, FILE *disk_file,
        offset_t record_header_offset);

static bool read_string_id_to_offset_index(struct err *err, struct archive *archive, const char *file_path,
        offset_t string_id_to_offset_index_offset);

bool archive_open(struct archive *out, const char *file_path)
{
        int status;
        FILE *disk_file;

        error_init(&out->err);
        out->diskFilePath = strdup(file_path);
        disk_file = fopen(out->diskFilePath, "r");
        if (!disk_file) {
                error_print(NG5_ERR_FOPEN_FAILED);
                return false;
        } else {
                struct archive_header header;
                size_t nread = fread(&header, sizeof(struct archive_header), 1, disk_file);
                if (nread != 1) {
                        fclose(disk_file);
                        error_print(NG5_ERR_IO);
                        return false;
                } else {
                        if (!is_valid_file(&header)) {
                                error_print(NG5_ERR_FORMATVERERR);
                                return false;
                        } else {
                                out->query_index_string_id_to_offset = NULL;
                                out->string_id_cache = NULL;

                                struct record_header record_header;

                                if ((status = read_stringtable(&out->string_table, &out->err, disk_file)) != true) {
                                        return status;
                                }
                                if ((status = read_record(&record_header,
                                        out,
                                        disk_file,
                                        header.root_object_header_offset)) != true) {
                                        return status;
                                }

                                if (header.string_id_to_offset_index_offset != 0) {
                                        struct err err;
                                        if ((status = read_string_id_to_offset_index(&err,
                                                out,
                                                file_path,
                                                header.string_id_to_offset_index_offset)) != true) {
                                                error_print(err.code);
                                                return status;
                                        }
                                }

                                fseek(disk_file, sizeof(struct archive_header), SEEK_SET);

                                offset_t data_start = ftell(disk_file);
                                fseek(disk_file, 0, SEEK_END);
                                offset_t file_size = ftell(disk_file);

                                fclose(disk_file);

                                size_t string_table_size = header.root_object_header_offset - data_start;
                                size_t record_table_size = record_header.record_size;
                                size_t string_id_index = file_size - header.string_id_to_offset_index_offset;

                                out->info.string_table_size = string_table_size;
                                out->info.record_table_size = record_table_size;
                                out->info.num_embeddded_strings = out->string_table.num_embeddded_strings;
                                out->info.string_id_index_size = string_id_index;
                                out->default_query = malloc(sizeof(struct archive_query));
                                query_create(out->default_query, out);

                        }
                }
        }

        return true;
}

NG5_EXPORT(bool) archive_get_info(struct archive_info *info, const struct archive *archive)
{
        error_if_null(info);
        error_if_null(archive);
        *info = archive->info;
        return true;
}

NG5_EXPORT(bool) archive_close(struct archive *archive)
{
        error_if_null(archive);
        archive_drop_indexes(archive);
        archive_drop_query_string_id_cache(archive);
        free(archive->diskFilePath);
        memblock_drop(archive->record_table.recordDataBase);
        query_drop(archive->default_query);
        free(archive->default_query);
        return true;
}

NG5_EXPORT(bool) archive_drop_indexes(struct archive *archive)
{
        if (archive->query_index_string_id_to_offset) {
                query_drop_index_string_id_to_offset(archive->query_index_string_id_to_offset);
                archive->query_index_string_id_to_offset = NULL;
        }
        return true;
}

NG5_EXPORT(bool) archive_query(struct archive_query *query, struct archive *archive)
{
        if (query_create(query, archive)) {
                bool has_index = false;
                archive_has_query_index_string_id_to_offset(&has_index, archive);
                if (!has_index) {
                        query_create_index_string_id_to_offset(&archive->query_index_string_id_to_offset, query);
                }
                bool has_cache = false;
                archive_hash_query_string_id_cache(&has_cache, archive);
                if (!has_cache) {
                        string_id_cache_create_LRU(&archive->string_id_cache, archive);
                }
                return true;
        } else {
                return false;
        }
}

NG5_EXPORT(bool) archive_has_query_index_string_id_to_offset(bool *state, struct archive *archive)
{
        error_if_null(state)
        error_if_null(archive)
        *state = (archive->query_index_string_id_to_offset != NULL);
        return true;
}

NG5_EXPORT(bool) archive_hash_query_string_id_cache(bool *has_cache, struct archive *archive)
{
        error_if_null(has_cache)
        error_if_null(archive)
        *has_cache = archive->string_id_cache != NULL;
        return true;
}

NG5_EXPORT(bool) archive_drop_query_string_id_cache(struct archive *archive)
{
        error_if_null(archive)
        if (archive->string_id_cache) {
                string_id_cache_drop(archive->string_id_cache);
                archive->string_id_cache = NULL;
        }
        return true;
}

NG5_EXPORT(struct string_cache *)archive_get_query_string_id_cache(struct archive *archive)
{
        return archive->string_id_cache;
}

NG5_EXPORT(struct archive_query *)archive_query_default(struct archive *archive)
{
        return archive ? archive->default_query : NULL;
}

static bool init_decompressor(struct packer *strategy, u8 flags)
{
        if (pack_by_flags(strategy, flags) != true) {
                return false;
        }
        return true;
}

static bool read_stringtable(struct string_table *table, struct err *err, FILE *disk_file)
{
        assert(disk_file);

        struct string_table_header header;
        union string_tab_flags flags;

        size_t num_read = fread(&header, sizeof(struct string_table_header), 1, disk_file);
        if (num_read != 1) {
                error(err, NG5_ERR_IO);
                return false;
        }
        if (header.marker != marker_symbols[MARKER_TYPE_EMBEDDED_STR_DIC].symbol) {
                error(err, NG5_ERR_CORRUPTED);
                return false;
        }

        flags.value = header.flags;
        table->first_entry_off = header.first_entry;
        table->num_embeddded_strings = header.num_entries;

        if ((init_decompressor(&table->compressor, flags.value)) != true) {
                return false;
        }
        if ((pack_read_extra(err, &table->compressor, disk_file, header.compressor_extra_size)) != true) {
                return false;
        }
        return true;
}

static bool read_record(struct record_header *header_read, struct archive *archive, FILE *disk_file,
        offset_t record_header_offset)
{
        struct err err;
        fseek(disk_file, record_header_offset, SEEK_SET);
        struct record_header header;
        if (fread(&header, sizeof(struct record_header), 1, disk_file) != 1) {
                error(&archive->err, NG5_ERR_CORRUPTED);
                return false;
        } else {
                archive->record_table.flags.value = header.flags;
                bool status = memblock_from_file(&archive->record_table.recordDataBase, disk_file, header.record_size);
                if (!status) {
                        memblock_get_error(&err, archive->record_table.recordDataBase);
                        error_cpy(&archive->err, &err);
                        return false;
                }

                struct memfile memfile;
                if (memfile_open(&memfile, archive->record_table.recordDataBase, READ_ONLY) != true) {
                        error(&archive->err, NG5_ERR_CORRUPTED);
                        status = false;
                }
                if (*NG5_MEMFILE_PEEK(&memfile, char) != MARKER_SYMBOL_OBJECT_BEGIN) {
                        error(&archive->err, NG5_ERR_CORRUPTED);
                        status = false;
                }

                *header_read = header;
                return true;
        }
}

static bool read_string_id_to_offset_index(struct err *err, struct archive *archive, const char *file_path,
        offset_t string_id_to_offset_index_offset)
{
        return query_index_id_to_offset_deserialize(&archive->query_index_string_id_to_offset,
                err,
                file_path,
                string_id_to_offset_index_offset);
}







