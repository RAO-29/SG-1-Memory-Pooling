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

#ifndef NG5_DATA_PTR_H
#define NG5_DATA_PTR_H

#include "shared/common.h"
#include "shared/types.h"

NG5_BEGIN_DECL

/**
 * A data pointer is a pointer that uses 16 high order bits to store additional data, and the remaining 48 bit
 * to store the actual memory address. Such a pointer requires an x86_64 architecture that use the lower 48 bits
 * to implement the address in hardware, i.e., current hardware. Note that in future architectures this feature
 * might vanish.
 *
 * Thanks to Adrian Vogelsgesang from Tableau Software for pointers in this direction.
 */
typedef void * data_ptr_t;

NG5_EXPORT(bool) data_ptr_create(data_ptr_t *dst, const void *adr);

NG5_EXPORT(bool) data_ptr_update(data_ptr_t *dst, const void *adr);

NG5_EXPORT(bool) data_ptr_get_data(u16 *data, const data_ptr_t ptr);

NG5_EXPORT(bool) data_ptr_set_data(data_ptr_t *ptr, u16 data);

NG5_EXPORT(bool) data_ptr_has_data(data_ptr_t ptr);

NG5_EXPORT(void *) data_ptr_get_pointer(const data_ptr_t ptr);

#define data_ptr_get(type, ptr) ((type *) data_ptr_get_pointer(ptr))

#define data_ptr_set(type, ptr, value) (*((type *) data_ptr_get_pointer(ptr)) = value)

NG5_END_DECL

#endif
