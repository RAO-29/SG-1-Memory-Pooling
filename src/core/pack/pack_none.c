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

#include <inttypes.h>
#include <assert.h>
#include "core/pack/pack.h"
#include "core/pack/pack_none.h"

NG5_EXPORT(bool) pack_none_init(struct packer *self)
{
        ng5_unused(self);
        /* nothing to do for uncompressed dictionaries */
        return true;
}

NG5_EXPORT(bool) pack_none_cpy(const struct packer *self, struct packer *dst)
{
        ng5_check_tag(self->tag, PACK_NONE);

        /* nothing to hard copy but the function pointers */
        *dst = *self;
        return true;
}

NG5_EXPORT(bool) pack_none_drop(struct packer *self)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);
        /* nothing to do for uncompressed dictionaries */
        return true;
}

NG5_EXPORT(bool) pack_none_write_extra(struct packer *self, struct memfile *dst,
        const struct vector ofType (const char *) *strings)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);
        ng5_unused(dst);
        ng5_unused(strings);
        /* nothing to do for uncompressed dictionaries */
        return true;
}

NG5_EXPORT(bool) pack_none_read_extra(struct packer *self, FILE *src, size_t nbytes)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);
        ng5_unused(src);
        ng5_unused(nbytes);
        /* nothing to do for uncompressed dictionaries */
        return true;
}

bool pack_none_print_extra(struct packer *self, FILE *file, struct memfile *src)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);
        ng5_unused(file);
        ng5_unused(src);
        /* nothing to do for uncompressed dictionaries */
        return true;
}

NG5_EXPORT(bool) pack_none_print_encoded_string(struct packer *self, FILE *file, struct memfile *src,
        u32 decompressed_strlen)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);

        const char *string = NG5_MEMFILE_READ(src, decompressed_strlen);

        char *printableString = malloc(decompressed_strlen + 1);
        memcpy(printableString, string, decompressed_strlen);
        printableString[decompressed_strlen] = '\0';

        fprintf(file, "[string: %s]", printableString);

        free(printableString);

        return true;
}

NG5_EXPORT(bool) pack_none_encode_string(struct packer *self, struct memfile *dst, struct err *err,
        const char *string)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);

        u32 string_length = strlen(string);

        ng5_success_or_jump(memfile_write(dst, string, string_length), error_handling)

        return true;

        error_handling:
        error(err, NG5_ERR_IO)
        return false;
}

NG5_EXPORT(bool) pack_none_decode_string(struct packer *self, char *dst, size_t strlen, FILE *src)
{
        ng5_check_tag(self->tag, PACK_NONE);

        ng5_unused(self);

        size_t num_read = fread(dst, sizeof(char), strlen, src);
        return (num_read == strlen);
}
