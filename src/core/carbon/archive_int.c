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

#include <assert.h>
#include "core/carbon/archive_int.h"

struct archive_header this_file_header = {.version = CARBON_ARCHIVE_VERSION, .root_object_header_offset = 0};

struct marker_symbol_entry marker_symbols[] =
        {{MARKER_TYPE_OBJECT_BEGIN, MARKER_SYMBOL_OBJECT_BEGIN}, {MARKER_TYPE_OBJECT_END, MARKER_SYMBOL_OBJECT_END},
         {MARKER_TYPE_PROP_NULL, MARKER_SYMBOL_PROP_NULL}, {MARKER_TYPE_PROP_BOOLEAN, MARKER_SYMBOL_PROP_BOOLEAN},
         {MARKER_TYPE_PROP_INT8, MARKER_SYMBOL_PROP_INT8}, {MARKER_TYPE_PROP_INT16, MARKER_SYMBOL_PROP_INT16},
         {MARKER_TYPE_PROP_INT32, MARKER_SYMBOL_PROP_INT32}, {MARKER_TYPE_PROP_INT64, MARKER_SYMBOL_PROP_INT64},
         {MARKER_TYPE_PROP_UINT8, MARKER_SYMBOL_PROP_UINT8}, {MARKER_TYPE_PROP_UINT16, MARKER_SYMBOL_PROP_UINT16},
         {MARKER_TYPE_PROP_UINT32, MARKER_SYMBOL_PROP_UINT32}, {MARKER_TYPE_PROP_UINT64, MARKER_SYMBOL_PROP_UINT64},
         {MARKER_TYPE_PROP_REAL, MARKER_SYMBOL_PROP_REAL}, {MARKER_TYPE_PROP_TEXT, MARKER_SYMBOL_PROP_TEXT},
         {MARKER_TYPE_PROP_OBJECT, MARKER_SYMBOL_PROP_OBJECT},
         {MARKER_TYPE_PROP_NULL_ARRAY, MARKER_SYMBOL_PROP_NULL_ARRAY},
         {MARKER_TYPE_PROP_BOOLEAN_ARRAY, MARKER_SYMBOL_PROP_BOOLEAN_ARRAY},
         {MARKER_TYPE_PROP_INT8_ARRAY, MARKER_SYMBOL_PROP_INT8_ARRAY},
         {MARKER_TYPE_PROP_INT16_ARRAY, MARKER_SYMBOL_PROP_INT16_ARRAY},
         {MARKER_TYPE_PROP_INT32_ARRAY, MARKER_SYMBOL_PROP_INT32_ARRAY},
         {MARKER_TYPE_PROP_INT64_ARRAY, MARKER_SYMBOL_PROP_INT64_ARRAY},
         {MARKER_TYPE_PROP_UINT8_ARRAY, MARKER_SYMBOL_PROP_UINT8_ARRAY},
         {MARKER_TYPE_PROP_UINT16_ARRAY, MARKER_SYMBOL_PROP_UINT16_ARRAY},
         {MARKER_TYPE_PROP_UINT32_ARRAY, MARKER_SYMBOL_PROP_UINT32_ARRAY},
         {MARKER_TYPE_PROP_UINT64_ARRAY, MARKER_SYMBOL_PROP_UINT64_ARRAY},
         {MARKER_TYPE_PROP_REAL_ARRAY, MARKER_SYMBOL_PROP_REAL_ARRAY},
         {MARKER_TYPE_PROP_TEXT_ARRAY, MARKER_SYMBOL_PROP_TEXT_ARRAY},
         {MARKER_TYPE_PROP_OBJECT_ARRAY, MARKER_SYMBOL_PROP_OBJECT_ARRAY},
         {MARKER_TYPE_EMBEDDED_STR_DIC, MARKER_SYMBOL_EMBEDDED_STR_DIC},
         {MARKER_TYPE_EMBEDDED_UNCOMP_STR, MARKER_SYMBOL_EMBEDDED_STR},
         {MARKER_TYPE_COLUMN_GROUP, MARKER_SYMBOL_COLUMN_GROUP}, {MARKER_TYPE_COLUMN, MARKER_SYMBOL_COLUMN},
         {MARKER_TYPE_HUFFMAN_DIC_ENTRY, MARKER_SYMBOL_HUFFMAN_DIC_ENTRY},
         {MARKER_TYPE_RECORD_HEADER, MARKER_SYMBOL_RECORD_HEADER}};

struct value_array_marker_mapping_entry value_array_marker_mapping[] =
        {{FIELD_NULL, MARKER_TYPE_PROP_NULL_ARRAY}, {FIELD_BOOLEAN, MARKER_TYPE_PROP_BOOLEAN_ARRAY},
         {FIELD_INT8, MARKER_TYPE_PROP_INT8_ARRAY}, {FIELD_INT16, MARKER_TYPE_PROP_INT16_ARRAY},
         {FIELD_INT32, MARKER_TYPE_PROP_INT32_ARRAY}, {FIELD_INT64, MARKER_TYPE_PROP_INT64_ARRAY},
         {FIELD_UINT8, MARKER_TYPE_PROP_UINT8_ARRAY}, {FIELD_UINT16, MARKER_TYPE_PROP_UINT16_ARRAY},
         {FIELD_UINT32, MARKER_TYPE_PROP_UINT32_ARRAY}, {FIELD_UINT64, MARKER_TYPE_PROP_UINT64_ARRAY},
         {FIELD_FLOAT, MARKER_TYPE_PROP_REAL_ARRAY}, {FIELD_STRING, MARKER_TYPE_PROP_TEXT_ARRAY},
         {FIELD_OBJECT, MARKER_TYPE_PROP_OBJECT_ARRAY}};

struct value_array_marker_mapping_entry valueMarkerMapping[] =
        {{FIELD_NULL, MARKER_TYPE_PROP_NULL}, {FIELD_BOOLEAN, MARKER_TYPE_PROP_BOOLEAN},
         {FIELD_INT8, MARKER_TYPE_PROP_INT8}, {FIELD_INT16, MARKER_TYPE_PROP_INT16},
         {FIELD_INT32, MARKER_TYPE_PROP_INT32}, {FIELD_INT64, MARKER_TYPE_PROP_INT64},
         {FIELD_UINT8, MARKER_TYPE_PROP_UINT8}, {FIELD_UINT16, MARKER_TYPE_PROP_UINT16},
         {FIELD_UINT32, MARKER_TYPE_PROP_UINT32}, {FIELD_UINT64, MARKER_TYPE_PROP_UINT64},
         {FIELD_FLOAT, MARKER_TYPE_PROP_REAL}, {FIELD_STRING, MARKER_TYPE_PROP_TEXT},
         {FIELD_OBJECT, MARKER_TYPE_PROP_OBJECT}};

void int_read_prop_offsets(struct archive_prop_offs *prop_offsets, struct memfile *memfile,
        const union object_flags *flags)
{
        ng5_zero_memory(prop_offsets, sizeof(struct archive_prop_offs));
        if (flags->bits.has_null_props) {
                prop_offsets->nulls = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_bool_props) {
                prop_offsets->bools = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int8_props) {
                prop_offsets->int8s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int16_props) {
                prop_offsets->int16s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int32_props) {
                prop_offsets->int32s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int64_props) {
                prop_offsets->int64s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint8_props) {
                prop_offsets->uint8s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint16_props) {
                prop_offsets->uint16s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint32_props) {
                prop_offsets->uint32s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint64_props) {
                prop_offsets->uint64s = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_float_props) {
                prop_offsets->floats = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_string_props) {
                prop_offsets->strings = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_object_props) {
                prop_offsets->objects = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_null_array_props) {
                prop_offsets->null_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_bool_array_props) {
                prop_offsets->bool_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int8_array_props) {
                prop_offsets->int8_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int16_array_props) {
                prop_offsets->int16_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int32_array_props) {
                prop_offsets->int32_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_int64_array_props) {
                prop_offsets->int64_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint8_array_props) {
                prop_offsets->uint8_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint16_array_props) {
                prop_offsets->uint16_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint32_array_props) {
                prop_offsets->uint32_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_uint64_array_props) {
                prop_offsets->uint64_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_float_array_props) {
                prop_offsets->float_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_string_array_props) {
                prop_offsets->string_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
        if (flags->bits.has_object_array_props) {
                prop_offsets->object_arrays = *NG5_MEMFILE_READ_TYPE(memfile, offset_t);
        }
}

void int_embedded_fixed_props_read(struct fixed_prop *prop, struct memfile *memfile)
{
        prop->header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
        prop->keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(field_sid_t));
        prop->values = memfile_peek(memfile, 1);
}

void int_embedded_var_props_read(struct var_prop *prop, struct memfile *memfile)
{
        prop->header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
        prop->keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(field_sid_t));
        prop->offsets = (offset_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(offset_t));
        prop->values = memfile_peek(memfile, 1);
}

void int_embedded_null_props_read(struct null_prop *prop, struct memfile *memfile)
{
        prop->header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
        prop->keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(field_sid_t));
}

void int_embedded_array_props_read(struct array_prop *prop, struct memfile *memfile)
{
        prop->header = NG5_MEMFILE_READ_TYPE(memfile, struct prop_header);
        prop->keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(field_sid_t));
        prop->lengths = (u32 *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(u32));
        prop->values_begin = memfile_tell(memfile);
}

void int_embedded_table_props_read(struct table_prop *prop, struct memfile *memfile)
{
        prop->header->marker = *NG5_MEMFILE_READ_TYPE(memfile, char);
        prop->header->num_entries = *NG5_MEMFILE_READ_TYPE(memfile, u8);
        prop->keys = (field_sid_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(field_sid_t));
        prop->groupOffs = (offset_t *) NG5_MEMFILE_READ(memfile, prop->header->num_entries * sizeof(offset_t));
}

field_e int_get_value_type_of_char(char c)
{
        size_t len = sizeof(value_array_marker_mapping) / sizeof(value_array_marker_mapping[0]);
        for (size_t i = 0; i < len; i++) {
                if (marker_symbols[value_array_marker_mapping[i].marker].symbol == c) {
                        return value_array_marker_mapping[i].value_type;
                }
        }
        return FIELD_NULL;
}

field_e // TODO: check whether 'field_e' can be replaced by 'enum field_type'
int_marker_to_field_type(char symbol)
{
        switch (symbol) {
        case MARKER_SYMBOL_PROP_NULL:
        case MARKER_SYMBOL_PROP_NULL_ARRAY:
                return FIELD_NULL;
        case MARKER_SYMBOL_PROP_BOOLEAN:
        case MARKER_SYMBOL_PROP_BOOLEAN_ARRAY:
                return FIELD_BOOLEAN;
        case MARKER_SYMBOL_PROP_INT8:
        case MARKER_SYMBOL_PROP_INT8_ARRAY:
                return FIELD_INT8;
        case MARKER_SYMBOL_PROP_INT16:
        case MARKER_SYMBOL_PROP_INT16_ARRAY:
                return FIELD_INT16;
        case MARKER_SYMBOL_PROP_INT32:
        case MARKER_SYMBOL_PROP_INT32_ARRAY:
                return FIELD_INT32;
        case MARKER_SYMBOL_PROP_INT64:
        case MARKER_SYMBOL_PROP_INT64_ARRAY:
                return FIELD_INT64;
        case MARKER_SYMBOL_PROP_UINT8:
        case MARKER_SYMBOL_PROP_UINT8_ARRAY:
                return FIELD_UINT8;
        case MARKER_SYMBOL_PROP_UINT16:
        case MARKER_SYMBOL_PROP_UINT16_ARRAY:
                return FIELD_UINT16;
        case MARKER_SYMBOL_PROP_UINT32:
        case MARKER_SYMBOL_PROP_UINT32_ARRAY:
                return FIELD_UINT32;
        case MARKER_SYMBOL_PROP_UINT64:
        case MARKER_SYMBOL_PROP_UINT64_ARRAY:
                return FIELD_UINT64;
        case MARKER_SYMBOL_PROP_REAL:
        case MARKER_SYMBOL_PROP_REAL_ARRAY:
                return FIELD_FLOAT;
        case MARKER_SYMBOL_PROP_TEXT:
        case MARKER_SYMBOL_PROP_TEXT_ARRAY:
                return FIELD_STRING;
        case MARKER_SYMBOL_PROP_OBJECT:
        case MARKER_SYMBOL_PROP_OBJECT_ARRAY:
                return FIELD_OBJECT;
        default: {
                print_error_and_die(NG5_ERR_MARKERMAPPING);
        }
        }
}