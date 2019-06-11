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

#ifndef NG5_IO_CONTEXT_H
#define NG5_IO_CONTEXT_H

#include "shared/common.h"
#include "shared/error.h"

NG5_BEGIN_DECL

struct archive; /* forwarded */
struct io_context; /* forwarded */

NG5_EXPORT(bool) io_context_create(struct io_context **context, struct err *err, const char *file_path);

NG5_EXPORT(struct err *) io_context_get_error(struct io_context *context);

NG5_EXPORT(FILE *) io_context_lock_and_access(struct io_context *context);

NG5_EXPORT(bool) io_context_unlock(struct io_context *context);

NG5_EXPORT(bool) io_context_drop(struct io_context *context);

NG5_END_DECL

#endif
