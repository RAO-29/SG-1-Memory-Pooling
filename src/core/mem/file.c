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

#include "core/mem/file.h"

bool memfile_open(struct memfile *file, struct memblock *block, enum access_mode mode)
{
        error_if_null(file)
        error_if_null(block)
        file->memblock = block;
        file->pos = 0;
        file->bit_mode = false;
        file->mode = mode;
        error_init(&file->err);
        return true;
}

bool memfile_seek(struct memfile *file, offset_t pos)
{
        error_if_null(file)
        offset_t file_size;
        memblock_size(&file_size, file->memblock);
        if (unlikely(pos >= file_size)) {
                if (file->mode == READ_WRITE) {
                        offset_t new_size = pos + 1;
                        memblock_resize(file->memblock, new_size);
                } else {
                        error(&file->err, NG5_ERR_MEMSTATE)
                        return false;
                }
        }
        file->pos = pos;
        return true;
}

bool memfile_rewind(struct memfile *file)
{
        error_if_null(file)
        file->pos = 0;
        return true;
}

bool memfile_get_offset(offset_t *pos, const struct memfile *file)
{
        error_if_null(pos)
        error_if_null(file)
        *pos = file->pos;
        return true;
}

size_t memfile_size(struct memfile *file)
{
        if (!file || !file->memblock) {
                return 0;
        } else {
                u64 size;
                memblock_size(&size, file->memblock);
                return size;
        }
}

size_t memfile_remain_size(struct memfile *file)
{
        assert(file->pos <= memfile_size(file));
        return memfile_size(file) - file->pos;
}

bool memfile_shrink(struct memfile *file)
{
        error_if_null(file);
        if (file->mode == READ_WRITE) {
                int status = memblock_shrink(file->memblock);
                u64 size;
                memblock_size(&size, file->memblock);
                assert(size == file->pos);
                return status;
        } else {
                error(&file->err, NG5_ERR_WRITEPROT)
                return false;
        }
}

const char *memfile_read(struct memfile *file, offset_t nbytes)
{
        const char *result = memfile_peek(file, nbytes);
        file->pos += nbytes;
        return result;
}

bool memfile_skip(struct memfile *file, offset_t nbytes)
{
        offset_t required_size = file->pos + nbytes;
        file->pos += nbytes;
        offset_t file_size;
        memblock_size(&file_size, file->memblock);

        if (unlikely(required_size >= file_size)) {
                if (file->mode == READ_WRITE) {
                        memblock_resize(file->memblock, required_size * 1.7f);
                } else {
                        error(&file->err, NG5_ERR_WRITEPROT);
                        return false;
                }
        }
        assert(file->pos < memfile_size(file));
        return true;
}

const char *memfile_peek(struct memfile *file, offset_t nbytes)
{
        offset_t file_size;
        memblock_size(&file_size, file->memblock);
        if (unlikely(file->pos + nbytes > file_size)) {
                error(&file->err, NG5_ERR_READOUTOFBOUNDS);
                return NULL;
        } else {
                const char *result = memblock_raw_data(file->memblock) + file->pos;
                return result;
        }
}

bool memfile_write(struct memfile *file, const void *data, offset_t nbytes)
{
        error_if_null(file)
        error_if_null(data)
        if (file->mode == READ_WRITE) {
                if (likely(nbytes != 0)) {
                        offset_t file_size;
                        memblock_size(&file_size, file->memblock);
                        offset_t required_size = file->pos + nbytes;
                        if (unlikely(required_size >= file_size)) {
                                memblock_resize(file->memblock, required_size * 1.7f);
                        }

                        if (unlikely(!memblock_write(file->memblock, file->pos, data, nbytes))) {
                                return false;
                        }
                        file->pos += nbytes;
                }
                return true;
        } else {
                error(&file->err, NG5_ERR_WRITEPROT);
                return false;
        }
}

bool memfile_begin_bit_mode(struct memfile *file)
{
        error_if_null(file);
        if (file->mode == READ_WRITE) {
                file->bit_mode = true;
                file->current_read_bit = file->current_write_bit = file->bytes_completed = 0;
                file->bytes_completed = 0;
                offset_t offset;
                char empty = '\0';
                memfile_get_offset(&offset, file);
                memfile_write(file, &empty, sizeof(char));
                memfile_seek(file, offset);
        } else {
                error(&file->err, NG5_ERR_WRITEPROT);
                return false;
        }

        return true;
}

bool memfile_write_bit(struct memfile *file, bool flag)
{
        error_if_null(file);
        file->current_read_bit = 0;

        if (file->bit_mode) {
                if (file->current_write_bit < 8) {
                        offset_t offset;
                        memfile_get_offset(&offset, file);
                        char byte = *memfile_read(file, sizeof(char));
                        char mask = 1 << file->current_write_bit;
                        if (flag) {
                                ng5_set_bits(byte, mask);
                        } else {
                                ng5_unset_bits(byte, mask);
                        }
                        memfile_seek(file, offset);
                        memfile_write(file, &byte, sizeof(char));
                        memfile_seek(file, offset);
                        file->current_write_bit++;
                } else {
                        file->current_write_bit = 0;
                        file->bytes_completed++;
                        char empty = '\0';
                        offset_t off;
                        memfile_skip(file, 1);
                        memfile_get_offset(&off, file);
                        memfile_write(file, &empty, sizeof(char));
                        memfile_seek(file, off);

                        return memfile_write_bit(file, flag);
                }
                return true;
        } else {
                error(&file->err, NG5_ERR_NOBITMODE);
                return false;
        }
}

bool memfile_read_bit(struct memfile *file)
{
        if (!file) {
                return false;
        }

        file->current_write_bit = 0;

        if (file->bit_mode) {
                if (file->current_read_bit < 8) {
                        offset_t offset;
                        memfile_get_offset(&offset, file);

                        char mask = 1 << file->current_read_bit;
                        char byte = *memfile_read(file, sizeof(char));
                        memfile_seek(file, offset);
                        bool result = ((byte & mask) >> file->current_read_bit) == true;
                        file->current_read_bit++;
                        return result;
                } else {
                        file->current_read_bit = 0;
                        memfile_skip(file, sizeof(char));
                        return memfile_read_bit(file);
                }
        } else {
                error(&file->err, NG5_ERR_NOBITMODE);
                return false;
        }
}

bool memfile_end_bit_mode(size_t *num_bytes_written, struct memfile *file)
{
        error_if_null(file);
        file->bit_mode = false;
        if (file->current_write_bit <= 8) {
                memfile_skip(file, 1);
                file->bytes_completed++;
        }
        ng5_optional_set(num_bytes_written, file->bytes_completed);
        file->current_write_bit = file->bytes_completed = 0;
        return true;
}

void *memfile_current_pos(struct memfile *file, offset_t nbytes)
{
        if (file && nbytes > 0) {
                offset_t file_size;
                memblock_size(&file_size, file->memblock);
                offset_t required_size = file->pos + nbytes;
                if (unlikely(file->pos + nbytes >= file_size)) {
                        if (file->mode == READ_WRITE) {
                                memblock_resize(file->memblock, required_size * 1.7f);
                        } else {
                                error(&file->err, NG5_ERR_WRITEPROT);
                                return NULL;
                        }
                }
                void *data = (void *) memfile_peek(file, nbytes);
                return data;
        } else {
                return NULL;
        }
}