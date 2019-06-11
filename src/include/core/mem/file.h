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

#ifndef NG5_MEMFILE_H
#define NG5_MEMFILE_H

#include "shared/common.h"
#include "block.h"

NG5_BEGIN_DECL

struct memfile {
        struct memblock *memblock;
        offset_t pos;
        bool bit_mode;
        size_t current_read_bit, current_write_bit, bytes_completed;
        enum access_mode mode;
        struct err err;
};

#define NG5_MEMFILE_PEEK(file, type)                                                                                   \
({                                                                                                                     \
    assert (memfile_remain_size(file) >= sizeof(type));                                                                \
    (type*) memfile_peek(file, sizeof(type));                                                                          \
})

#define NG5_MEMFILE_READ_TYPE(file, type)                                                                              \
({                                                                                                                     \
    assert (memfile_remain_size(file) >= sizeof(type));                                                                \
    (type*) memfile_read(file, sizeof(type));                                                                          \
})

#define NG5_MEMFILE_READ_TYPE_LIST(file, type, how_many)                                                               \
    (const type *) NG5_MEMFILE_READ(file, how_many * sizeof(type))

#define NG5_MEMFILE_READ(file, nbytes)                                                                                 \
({                                                                                                                     \
    assert (memfile_remain_size(file) >= nbytes);                                                                      \
    memfile_read(file, nbytes);                                                                                        \
})

#define memfile_tell(file)                                                                                             \
({                                                                                                                     \
    offset_t offset;                                                                                                   \
    memfile_get_offset(&offset, file);                                                                                 \
    offset;                                                                                                            \
})

NG5_EXPORT(bool) memfile_open(struct memfile *file, struct memblock *block, enum access_mode mode);

NG5_EXPORT(bool) memfile_seek(struct memfile *file, offset_t pos);

NG5_EXPORT(bool) memfile_rewind(struct memfile *file);

NG5_EXPORT(bool) memfile_get_offset(offset_t *pos, const struct memfile *file);

NG5_EXPORT(size_t) memfile_size(struct memfile *file);

NG5_EXPORT(size_t) memfile_remain_size(struct memfile *file);

NG5_EXPORT(bool) memfile_shrink(struct memfile *file);

NG5_EXPORT(const char *)memfile_read(struct memfile *file, offset_t nbytes);

NG5_EXPORT(bool) memfile_skip(struct memfile *file, offset_t nbytes);

NG5_EXPORT(const char *)memfile_peek(struct memfile *file, offset_t nbytes);

NG5_EXPORT(bool) memfile_write(struct memfile *file, const void *data, offset_t nbytes);

NG5_EXPORT(bool) memfile_begin_bit_mode(struct memfile *file);

NG5_EXPORT(bool) memfile_write_bit(struct memfile *file, bool flag);

NG5_EXPORT(bool) memfile_read_bit(struct memfile *file);

NG5_EXPORT(bool) memfile_end_bit_mode(size_t *num_bytes_written, struct memfile *file);

NG5_EXPORT(void *) memfile_current_pos(struct memfile *file, offset_t nbytes);

NG5_END_DECL

#endif