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

#include "core/ptrs/tagged_ptr.h"
#include "shared/error.h"

static const uintptr_t addr_mask = ~0x03ULL;

NG5_EXPORT(bool) tagged_ptr_create(tagged_ptr_t *dst, const void *adr)
{
        error_if_null(dst);
        error_if_null(adr);
        *dst = (void *) adr;
        tagged_ptr_set_tag(dst, 0);
        return true;
}

NG5_EXPORT(bool) tagged_ptr_update(tagged_ptr_t *dst, const void *adr)
{
        error_if_null(dst);
        error_if_null(adr);
        u3 tag = 0;
        tagged_ptr_get_tag(&tag, *dst);
        tagged_ptr_create(dst, adr);
        tagged_ptr_set_tag(dst, tag);
        return true;
}

NG5_EXPORT(bool) tagged_ptr_set_tag(tagged_ptr_t *dst, u3 tag)
{
        error_if_null(dst);
        *dst = (void *)(((uintptr_t) *dst & addr_mask) | tag);
        return true;
}

NG5_EXPORT(bool) tagged_ptr_get_tag(u3 *tag, const tagged_ptr_t ptr)
{
        error_if_null(tag);
        error_if_null(ptr);
        *tag = (uintptr_t) ptr & 0x03;
        return true;
}

NG5_EXPORT(bool) tagged_ptr_is_tagged(tagged_ptr_t ptr)
{
        u3 tag;
        return tagged_ptr_get_tag(&tag, ptr) ? tag != 0 : 0;
}

NG5_EXPORT(void *) tagged_ptr_get_pointer(const tagged_ptr_t ptr)
{
        error_if_null(ptr);
        return (void *) ((uintptr_t) ptr & addr_mask);
}