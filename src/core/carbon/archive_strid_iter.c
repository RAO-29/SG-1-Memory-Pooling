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

#include "core/carbon/archive_strid_iter.h"

NG5_EXPORT(bool) strid_iter_open(struct strid_iter *it, struct err *err, struct archive *archive)
{
        error_if_null(it)
        error_if_null(archive)

        memset(&it->vector, 0, sizeof(it->vector));
        it->disk_file = fopen(archive->diskFilePath, "r");
        if (!it->disk_file) {
                ng5_optional(err, error(err, NG5_ERR_FOPEN_FAILED))
                it->is_open = false;
                return false;
        }
        fseek(it->disk_file, archive->string_table.first_entry_off, SEEK_SET);
        it->is_open = true;
        it->disk_offset = archive->string_table.first_entry_off;
        return true;
}

NG5_EXPORT(bool) strid_iter_next(bool *success, struct strid_info **info, struct err *err, size_t *info_length,
        struct strid_iter *it)
{
        error_if_null(info)
        error_if_null(info_length)
        error_if_null(it)

        if (it->disk_offset != 0 && it->is_open) {
                struct string_entry_header header;
                size_t vec_pos = 0;
                do {
                        fseek(it->disk_file, it->disk_offset, SEEK_SET);
                        int num_read = fread(&header, sizeof(struct string_entry_header), 1, it->disk_file);
                        if (header.marker != '-') {
                                error_print(NG5_ERR_INTERNALERR);
                                return false;
                        }
                        if (num_read != 1) {
                                ng5_optional(err, error(err, NG5_ERR_FREAD_FAILED))
                                *success = false;
                                return false;
                        } else {
                                it->vector[vec_pos].id = header.string_id;
                                it->vector[vec_pos].offset = ftell(it->disk_file);
                                it->vector[vec_pos].strlen = header.string_len;
                                it->disk_offset = header.next_entry_off;
                                vec_pos++;
                        }
                }
                while (header.next_entry_off != 0 && vec_pos < NG5_ARRAY_LENGTH(it->vector));

                *info_length = vec_pos;
                *success = true;
                *info = &it->vector[0];
                return true;
        } else {
                return false;
        }
}

NG5_EXPORT(bool) strid_iter_close(struct strid_iter *it)
{
        error_if_null(it)
        if (it->is_open) {
                fclose(it->disk_file);
                it->is_open = false;
        }
        return true;
}
