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

#ifndef NG5_MEMBLOCK_H
#define NG5_MEMBLOCK_H

#include "shared/common.h"
#include "shared/error.h"

NG5_BEGIN_DECL

struct memblock;

NG5_EXPORT(bool) memblock_create(struct memblock **block, size_t size);

NG5_EXPORT(bool) memblock_from_file(struct memblock **block, FILE *file, size_t nbytes);

NG5_EXPORT(bool) memblock_drop(struct memblock *block);

NG5_EXPORT(bool) memblock_get_error(struct err *out, struct memblock *block);

NG5_EXPORT(bool) memblock_size(offset_t *size, const struct memblock *block);

NG5_EXPORT(bool) memblock_write_to_file(FILE *file, const struct memblock *block);

NG5_EXPORT(const char *) memblock_raw_data(const struct memblock *block);

NG5_EXPORT(bool) memblock_resize(struct memblock *block, size_t size);

NG5_EXPORT(bool) memblock_write(struct memblock *block, offset_t position, const char *data, offset_t nbytes);

NG5_EXPORT(bool) memblock_cpy(struct memblock **dst, struct memblock *src);

NG5_EXPORT(bool) memblock_shrink(struct memblock *block);

NG5_EXPORT(void *)memblock_move_contents_and_drop(struct memblock *block);

NG5_END_DECL

#endif
