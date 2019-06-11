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

#include <assert.h>
#include <inttypes.h>

#include "std/bitmap.h"
#include "core/pack/pack.h"
#include "coding/coding_huffman.h"

#define  MARKER_SYMBOL_HUFFMAN_DIC_ENTRY   'd'

NG5_EXPORT(bool) pack_huffman_init(struct packer *self)
{
        self->extra = malloc(sizeof(struct coding_huffman));
        if (self->extra != NULL) {
                struct coding_huffman *encoder = (struct coding_huffman *) self->extra;
                coding_huffman_create(encoder);
                return true;
        } else {
                return false;
        }
}

NG5_EXPORT(bool) pack_coding_huffman_cpy(const struct packer *self, struct packer *dst)
{
        ng5_check_tag(self->tag, PACK_HUFFMAN);

        *dst = *self;
        dst->extra = malloc(sizeof(struct coding_huffman));
        if (dst->extra != NULL) {
                struct coding_huffman *self_encoder = (struct coding_huffman *) self->extra;
                struct coding_huffman *dst_encoder = (struct coding_huffman *) dst->extra;
                return coding_huffman_cpy(dst_encoder, self_encoder);
        } else {
                return false;
        }
}

NG5_EXPORT(bool) pack_coding_huffman_drop(struct packer *self)
{
        ng5_check_tag(self->tag, PACK_HUFFMAN);

        struct coding_huffman *encoder = (struct coding_huffman *) self->extra;
        coding_huffman_drop(encoder);

        return true;
}

bool huffman_dump_dictionary(FILE *file, struct memfile *memfile)
{
        struct pack_huffman_info entry_info;
        offset_t offset;

        while ((*NG5_MEMFILE_PEEK(memfile, char)) == MARKER_SYMBOL_HUFFMAN_DIC_ENTRY) {
                memfile_get_offset(&offset, memfile);
                coding_huffman_read_entry(&entry_info, memfile, MARKER_SYMBOL_HUFFMAN_DIC_ENTRY);

                fprintf(file, "0x%04x ", (unsigned) offset);
                fprintf(file,
                        "[marker: %c] [letter: '%c'] [nbytes_prefix: %d] [code: ",
                        MARKER_SYMBOL_HUFFMAN_DIC_ENTRY,
                        entry_info.letter,
                        entry_info.nbytes_prefix);

                if (entry_info.nbytes_prefix > 0) {
                        for (u16 i = 0; i < entry_info.nbytes_prefix; i++) {
                                bitmap_print_bits_in_char(file, entry_info.prefix_code[i]);
                                fprintf(file, "%s", i + 1 < entry_info.nbytes_prefix ? ", " : "");
                        }
                } else {
                        fprintf(file, "0b00000000");
                }

                fprintf(file, "]\n");
        }
        return true;
}

bool huffman_dump_string_table_entry(FILE *file, struct memfile *memfile)
{
        ng5_unused(file);
        ng5_unused(memfile);

        struct pack_huffman_str_info info;

        coding_huffman_read_string(&info, memfile);

        fprintf(file, "[[nbytes_encoded: %d] [bytes: ", info.nbytes_encoded);
        for (size_t i = 0; i < info.nbytes_encoded; i++) {
                char byte = info.encoded_bytes[i];
                bitmap_print_bits_in_char(file, byte);
                fprintf(file, "%s", i + 1 < info.nbytes_encoded ? "," : "");
        }
        fprintf(file, "]\n");

        return true;
}

NG5_EXPORT(bool) pack_huffman_write_extra(struct packer *self, struct memfile *dst,
        const struct vector ofType (const char *) *strings)
{
        ng5_check_tag(self->tag, PACK_HUFFMAN);

        struct coding_huffman *encoder = (struct coding_huffman *) self->extra;

        coding_huffman_build(encoder, strings);
        coding_huffman_serialize(dst, encoder, MARKER_SYMBOL_HUFFMAN_DIC_ENTRY);

        return true;
}

NG5_EXPORT(bool) pack_huffman_read_extra(struct packer *self, FILE *src, size_t nbytes)
{
        ng5_check_tag(self->tag, PACK_HUFFMAN);

        ng5_unused(self);
        ng5_unused(src);
        ng5_unused(nbytes);

        abort(); /* not implemented */
        return false;
}

NG5_EXPORT(bool) pack_huffman_print_extra(struct packer *self, FILE *file, struct memfile *src)
{
        ng5_unused(self);

        huffman_dump_dictionary(file, src);

        return true;
}

NG5_EXPORT(bool) pack_huffman_print_encoded(struct packer *self, FILE *file, struct memfile *src,
        u32 decompressed_strlen)
{
        ng5_unused(self);
        ng5_unused(file);
        ng5_unused(src);
        ng5_unused(decompressed_strlen);

        huffman_dump_string_table_entry(file, src);

        return true;
}

bool pack_huffman_encode_string(struct packer *self, struct memfile *dst, struct err *err, const char *string)
{
        ng5_check_tag(self->tag, PACK_HUFFMAN);

        struct coding_huffman *encoder = (struct coding_huffman *) self->extra;
        bool status = coding_huffman_encode(dst, encoder, string);
        error_cpy(err, &encoder->err);

        return status;
}

NG5_EXPORT(bool) pack_huffman_decode_string(struct packer *self, char *dst, size_t strlen, FILE *src)
{
        ng5_unused(self);
        ng5_unused(dst);
        ng5_unused(strlen);
        ng5_unused(src);
        abort(); /* not implemented */
        return false;
}