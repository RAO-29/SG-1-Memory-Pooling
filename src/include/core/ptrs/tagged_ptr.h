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

#ifndef NG5_TAGGED_PTR_H
#define NG5_TAGGED_PTR_H

#include "shared/common.h"
#include "shared/types.h"

NG5_BEGIN_DECL

/**
 * Tagged pointer implementation.
 */
typedef void * tagged_ptr_t;

NG5_EXPORT(bool) tagged_ptr_create(tagged_ptr_t *dst, const void *adr);

NG5_EXPORT(bool) tagged_ptr_update(tagged_ptr_t *dst, const void *adr);

NG5_EXPORT(bool) tagged_ptr_set_tag(tagged_ptr_t *dst, u3 tag);

NG5_EXPORT(bool) tagged_ptr_get_tag(u3 *tag, tagged_ptr_t ptr);

NG5_EXPORT(bool) tagged_ptr_is_tagged(tagged_ptr_t ptr);

NG5_EXPORT(void *) tagged_ptr_get_pointer(tagged_ptr_t ptr);

#define tagged_ptr_deref(type, ptr) ((type *) tagged_ptr_get_pointer(ptr))

NG5_END_DECL

#endif
