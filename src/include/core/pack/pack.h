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

#ifndef NG5_ARCHIVE_COMPR_H
#define NG5_ARCHIVE_COMPR_H

#include "core/pack/pack_none.h"
#include "core/pack/huffman.h"
#include "coding/coding_huffman.h"
#include "shared/common.h"
#include "shared/types.h"

NG5_BEGIN_DECL

/**
 * Unique tag identifying a specific implementation for compressing/decompressing string in a CARBON archives
 * string table.
 */
enum packer_type {
        PACK_NONE, PACK_HUFFMAN
};

/**
 * Main interface for the compressor framework. A compressor is used to encode/decode strings stored in a
 * CARBON archive.
 */
struct packer {
        /** Tag identifying the implementation of this compressor */
        enum packer_type tag;

        /** Implementation-specific storage */
        void *extra;

        /**
         * Constructor for implementation-dependent initialization of the compressor at hand.
         *
         * Depending on the implementation, the compressor might allocate dynamic memory for
         * <code>extra</code> for book-keeping purposes.
         *
         * @param self A pointer to itself
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*create)(struct packer *self);

        /**
         * Destructor for implementation-dependent deinitialization of the compressor at hand.
         *
         * If the implementation acquired dynamic memory during a call to <code>create</code>,
         * a call to this function frees up this memory.
         *
         * @param self A pointer to itself
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*drop)(struct packer *self);

        /**
         * Perform a hard-copy of this compressor to dst
         *
         * @param self  A pointer to itself
         * @param dst   A pointer to the copy target
         *
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*cpy)(const struct packer *self, struct packer *dst);

        /**
         * Function to construct and serialize an implementation-specific dictionary, book-keeping data, or extra data
         * (e.g., a code table)
         *
         * Depending on the implementation, a set of book-keeping data must be managed for a compressor. For instance,
         * in case of a huffman encoded this book-keeping is the code table that maps letters to prefix codes. This
         * function is invoked before any string gets encoded, and implements a compressor-specific management and
         * serialization of that book-keeping data. After internal construction of this book-keeping data,
         * this data is serialized into the <code>dst</code> parameter.
         *
         * Reverse function of <code>read_extra</code>.
         *
         * @note single strings must not be encoded; this is is done when the framework invokes <code>encode_string</code>
         *
         * @param self A pointer to the compressor that is used; maybe accesses <code>extra</code>
         * @param dst A memory file in which the book-keeping data (<code>extra</code>) for this compressor is serialized
         * @param strings The set of all strings that should be encoded. Used for tweaking the compressor;
         *                 <b>not</b> for serialization into <code>dst</code>
         *
         * @note strings in <code>strings</code> are unique (but not sorted)
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         * */
        bool (*write_extra)(struct packer *self, struct memfile *dst,
                const struct vector ofType (const char *) *strings);

        /**
         * Function to reconstruct implementation-specific dictionary, book-keeping or extra data by deserialization (
         * e.g., a code table)
         *
         * Reverse function of <code>write_extra</code>.
         *
         * @param self A pointer to the compressor that is used; maybe accesses <code>extra</code>
         * @param src A file where the cursor is moved to the first byte of the extra field previously serialized with 'write_extra'
         * @param nbytes Number of bytes written when 'write_extra' was called. Intended to read read to restore the extra field.
         * @return The implementer must return <code>true</code> on success, and <code>false</code> otherwise.
         */
        bool (*read_extra)(struct packer *self, FILE *src, size_t nbytes);

        /**
         * Encodes an input string and writes its encoded version into a memory file.
         *
         * @param self A pointer to the compressor that is used; maybe accesses <code>extra</code>
         * @param dst A memory file in which the encoded string should be stored
         * @param err An error information
         * @param string The string that should be encoded
         *
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*encode_string)(struct packer *self, struct memfile *dst, struct err *err, const char *string);

        bool (*decode_string)(struct packer *self, char *dst, size_t strlen, FILE *src);

        /**
         * Reads implementation-specific book-keeping, meta or extra data from the input memory file and
         * prints its contents in a human-readable version to <code>file</code>
         *
         * @param self A pointer to the compressor that is used; potentially accessing <code>extra</code>
         * @param file A file to which a human-readable version of <code>extra</code> is printed (if any)
         * @param src A memory file which cursor is positioned at the begin of the serialized extra field. After
         *            a call to this function, the memory file cursor must be positioned after the serialized extra
         *            field (i.e., the entire entry must be read (if any))
         *
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*print_extra)(struct packer *self, FILE *file, struct memfile *src);

        /**
         * Reads an implementation-specific encoded string from a memory file <code>src</code>, and prints
         * the encoded string in a human-readable version to <code>file</code>
         *
         * @param self A pointer to the compressor that is used; potentially accessing <code>extra</code>
         * @param file A file to which a human-readable version of the encoded string is printed.
         * @param src A memory file which cursor is positioned at the begin of the encoded string. After a call
         *            to this function, the memory file cursor must be positioned after the encoded string (i.e.,
         *            the entire encoded string must be read)
         * @param decompressed_strlen The length of the decoded string in number of characters
         *
         * @return <b>true</b> in case of success, or <b>false</b> otherwise.
         *
         * @author Marcus Pinnecke
         * @since 0.1.00.05
         */
        bool (*print_encoded)(struct packer *self, FILE *file, struct memfile *src, u32 decompressed_strlen);

};

NG5_EXPORT(void) pack_none_create(struct packer *strategy);

NG5_EXPORT(void) pack_huffman_create(struct packer *strategy);

extern struct compressor_strategy_entry {
        enum packer_type type;

        const char *name;

        void (*create)(struct packer *strategy);

        u8 flag_bit;
} compressor_strategy_register[];

NG5_EXPORT(size_t) pack_get_num_registered_strategies();

NG5_EXPORT(bool) pack_by_type(struct err *err, struct packer *strategy, enum packer_type type);

NG5_EXPORT(u8) pack_flagbit_by_type(enum packer_type type);

NG5_EXPORT(bool) pack_by_flags(struct packer *strategy, u8 flags);

NG5_EXPORT(bool) pack_by_name(enum packer_type *type, const char *name);

NG5_EXPORT(bool) pack_cpy(struct err *err, struct packer *dst, const struct packer *src);

NG5_EXPORT(bool) pack_drop(struct err *err, struct packer *self);

NG5_EXPORT(bool) pack_write_extra(struct err *err, struct packer *self, struct memfile *dst,
        const struct vector ofType (const char *) *strings);

NG5_EXPORT(bool) pack_read_extra(struct err *err, struct packer *self, FILE *src, size_t nbytes);

NG5_EXPORT(bool) pack_encode(struct err *err, struct packer *self, struct memfile *dst, const char *string);

NG5_EXPORT(bool) pack_decode(struct err *err, struct packer *self, char *dst, size_t strlen, FILE *src);

NG5_EXPORT(bool) pack_print_extra(struct err *err, struct packer *self, FILE *file, struct memfile *src);

NG5_EXPORT(bool) pack_print_encoded(struct err *err, struct packer *self, FILE *file, struct memfile *src,
        u32 decompressed_strlen);

NG5_END_DECL

#endif
