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

#include "core/ptrs/data_ptr.h"
#include "shared/error.h"

static const uintptr_t addr_mask = ~(1ULL << 48);

NG5_EXPORT(bool) data_ptr_create(data_ptr_t *dst, const void *adr)
{
        error_if_null(dst);
        error_if_null(adr);
        *dst = (void *) adr;
        data_ptr_set_data(dst, 0);
        return true;
}

NG5_EXPORT(bool) data_ptr_update(data_ptr_t *dst, const void *adr)
{
        error_if_null(dst);
        error_if_null(adr);
        u16 data = 0;
        data_ptr_get_data(&data, *dst);
        data_ptr_create(dst, adr);
        data_ptr_set_data(dst, data);
        return true;
}

NG5_EXPORT(bool) data_ptr_get_data(u16 *data, const data_ptr_t ptr)
{
        error_if_null(data);
        error_if_null(ptr);
        *data = (uintptr_t) ptr >> 48;
        return true;
}

NG5_EXPORT(bool) data_ptr_set_data(data_ptr_t *ptr, u16 data)
{
        error_if_null(ptr);
        ng5_unused(data);
        *ptr = (void *) (((uintptr_t) *ptr & addr_mask) | ((uintptr_t) data << 48));
        return true;
}

NG5_EXPORT(bool) data_ptr_has_data(const data_ptr_t ptr)
{
        u16 data;
        return data_ptr_get_data(&data, ptr) ? data != 0 : 0;
}

NG5_EXPORT(void *) data_ptr_get_pointer(const data_ptr_t ptr)
{
        return ptr ? (void *)(((uintptr_t) ptr << 16) >> 16) : NULL;
}