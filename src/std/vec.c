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

#include <assert.h>
#include <inttypes.h>
#include <sys/mman.h>

#include "core/mem/file.h"
#include "std/vec.h"

#define DEFINE_PRINTER_FUNCTION_WCAST(type, castType, format_string)                                                   \
void vector_##type##_PrinterFunc(struct memfile *dst, void ofType(T) *values, size_t num_elems)                      \
{                                                                                                                      \
    char *data;                                                                                                        \
    type *typedValues = (type *) values;                                                                               \
                                                                                                                       \
    data = memfile_current_pos(dst, sizeof(char));                                                              \
    int nchars = sprintf(data, "[");                                                                                   \
    memfile_skip(dst, nchars);                                                                                  \
    for (size_t i = 0; i < num_elems; i++) {                                                                           \
        data = memfile_current_pos(dst, sizeof(type));                                                          \
        nchars = sprintf(data, format_string"%s", (castType) typedValues[i], i + 1 < num_elems ? ", " : "");           \
        memfile_skip(dst, nchars);                                                                              \
    }                                                                                                                  \
    data = memfile_current_pos(dst, sizeof(char));                                                              \
    nchars = sprintf(data, "]");                                                                                       \
    memfile_skip(dst, nchars);                                                                                  \
}

#define DEFINE_PRINTER_FUNCTION(type, format_string)                                                                   \
    DEFINE_PRINTER_FUNCTION_WCAST(type, type, format_string)

DEFINE_PRINTER_FUNCTION_WCAST(u_char, i8, "%d")

DEFINE_PRINTER_FUNCTION(i8, "%d")

DEFINE_PRINTER_FUNCTION(i16, "%d")

DEFINE_PRINTER_FUNCTION(i32, "%d")

DEFINE_PRINTER_FUNCTION(i64, "%"
        PRIi64)

DEFINE_PRINTER_FUNCTION(u8, "%d")

DEFINE_PRINTER_FUNCTION(u16, "%d")

DEFINE_PRINTER_FUNCTION(u32, "%d")

DEFINE_PRINTER_FUNCTION(u64, "%"
        PRIu64)

DEFINE_PRINTER_FUNCTION(size_t, "%zu")

bool vec_create(struct vector *out, const struct allocator *alloc, size_t elem_size, size_t cap_elems)
{
        error_if_null(out)
        out->allocator = malloc(sizeof(struct allocator));
        alloc_this_or_std(out->allocator, alloc);
        out->base = alloc_malloc(out->allocator, cap_elems * elem_size);
        out->num_elems = 0;
        out->cap_elems = cap_elems;
        out->elem_size = elem_size;
        out->grow_factor = 1.7f;
        error_init(&out->err);
        return true;
}

struct vector_serialize_header {
        char marker;
        u32 elem_size;
        u32 num_elems;
        u32 cap_elems;
        float grow_factor;
};

NG5_EXPORT(bool) vec_serialize(FILE *file, struct vector *vec)
{
        error_if_null(file)
        error_if_null(vec)

        struct vector_serialize_header header =
                {.marker = MARKER_SYMBOL_VECTOR_HEADER, .elem_size = vec->elem_size, .num_elems = vec
                        ->num_elems, .cap_elems = vec->cap_elems, .grow_factor = vec->grow_factor};
        int nwrite = fwrite(&header, sizeof(struct vector_serialize_header), 1, file);
        error_if(nwrite != 1, &vec->err, NG5_ERR_FWRITE_FAILED);
        nwrite = fwrite(vec->base, vec->elem_size, vec->num_elems, file);
        error_if(nwrite != (int) vec->num_elems, &vec->err, NG5_ERR_FWRITE_FAILED);

        return true;
}

NG5_EXPORT(bool) vec_deserialize(struct vector *vec, struct err *err, FILE *file)
{
        error_if_null(file)
        error_if_null(err)
        error_if_null(vec)

        offset_t start = ftell(file);
        int err_code = NG5_ERR_NOERR;

        struct vector_serialize_header header;
        if (fread(&header, sizeof(struct vector_serialize_header), 1, file) != 1) {
                err_code = NG5_ERR_FREAD_FAILED;
                goto error_handling;
        }

        if (header.marker != MARKER_SYMBOL_VECTOR_HEADER) {
                err_code = NG5_ERR_CORRUPTED;
                goto error_handling;
        }

        vec->allocator = malloc(sizeof(struct allocator));
        alloc_this_or_std(vec->allocator, NULL);
        vec->base = alloc_malloc(vec->allocator, header.cap_elems * header.elem_size);
        vec->num_elems = header.num_elems;
        vec->cap_elems = header.cap_elems;
        vec->elem_size = header.elem_size;
        vec->grow_factor = header.grow_factor;
        error_init(&vec->err);

        if (fread(vec->base, header.elem_size, vec->num_elems, file) != vec->num_elems) {
                err_code = NG5_ERR_FREAD_FAILED;
                goto error_handling;
        }

        return true;

        error_handling:
        fseek(file, start, SEEK_SET);
        error(err, err_code);
        return false;
}

bool vec_memadvice(struct vector *vec, int madviseAdvice)
{
        error_if_null(vec);
        ng5_unused(vec);
        ng5_unused(madviseAdvice);
        madvise(vec->base, vec->cap_elems * vec->elem_size, madviseAdvice);
        return true;
}

bool vec_set_grow_factor(struct vector *vec, float factor)
{
        error_if_null(vec);
        error_print_if(factor <= 1.01f, NG5_ERR_ILLEGALARG)
        vec->grow_factor = factor;
        return true;
}

bool vec_drop(struct vector *vec)
{
        error_if_null(vec)
        alloc_free(vec->allocator, vec->base);
        free(vec->allocator);
        vec->base = NULL;
        return true;
}

bool vec_is_empty(const struct vector *vec)
{
        error_if_null(vec)
        return vec->num_elems == 0 ? true : false;
}

bool vec_push(struct vector *vec, const void *data, size_t num_elems)
{
        error_if_null(vec && data)
        size_t next_num = vec->num_elems + num_elems;
        while (next_num > vec->cap_elems) {
                size_t more = next_num - vec->cap_elems;
                vec->cap_elems = (vec->cap_elems + more) * vec->grow_factor;
                vec->base = alloc_realloc(vec->allocator, vec->base, vec->cap_elems * vec->elem_size);
        }
        memcpy(vec->base + vec->num_elems * vec->elem_size, data, num_elems * vec->elem_size);
        vec->num_elems += num_elems;
        return true;
}

const void *vec_peek(struct vector *vec)
{
        if (!vec) {
                return NULL;
        } else {
                return (vec->num_elems > 0) ? vec_at(vec, vec->num_elems - 1) : NULL;
        }
}

bool vec_repeated_push(struct vector *vec, const void *data, size_t how_often)
{
        error_if_null(vec && data)
        size_t next_num = vec->num_elems + how_often;
        while (next_num > vec->cap_elems) {
                size_t more = next_num - vec->cap_elems;
                vec->cap_elems = (vec->cap_elems + more) * vec->grow_factor;
                vec->base = alloc_realloc(vec->allocator, vec->base, vec->cap_elems * vec->elem_size);
        }
        for (size_t i = 0; i < how_often; i++) {
                memcpy(vec->base + (vec->num_elems + i) * vec->elem_size, data, vec->elem_size);
        }

        vec->num_elems += how_often;
        return true;
}

const void *vec_pop(struct vector *vec)
{
        void *result;
        if (likely((result = (vec ? (vec->num_elems > 0 ? vec->base + (vec->num_elems - 1) * vec->elem_size : NULL)
                                      : NULL)) != NULL)) {
                vec->num_elems--;
        }
        return result;
}

bool vec_clear(struct vector *vec)
{
        error_if_null(vec)
        vec->num_elems = 0;
        return true;
}

bool vec_shrink(struct vector *vec)
{
        error_if_null(vec);
        if (vec->num_elems < vec->cap_elems) {
                vec->cap_elems = ng5_max(1, vec->num_elems);
                vec->base = alloc_realloc(vec->allocator, vec->base, vec->cap_elems * vec->elem_size);
        }
        return true;
}

bool vec_grow(size_t *numNewSlots, struct vector *vec)
{
        error_if_null(vec)
        size_t freeSlotsBefore = vec->cap_elems - vec->num_elems;

        vec->cap_elems = (vec->cap_elems * vec->grow_factor) + 1;
        vec->base = alloc_realloc(vec->allocator, vec->base, vec->cap_elems * vec->elem_size);
        size_t freeSlotsAfter = vec->cap_elems - vec->num_elems;
        if (likely(numNewSlots != NULL)) {
                *numNewSlots = freeSlotsAfter - freeSlotsBefore;
        }
        return true;
}

NG5_EXPORT(bool) vec_grow_to(struct vector *vec, size_t capacity)
{
        error_if_null(vec);
        vec->cap_elems = ng5_max(vec->cap_elems, capacity);
        vec->base = alloc_realloc(vec->allocator, vec->base, vec->cap_elems * vec->elem_size);
        return true;
}

size_t vec_length(const struct vector *vec)
{
        error_if_null(vec)
        return vec->num_elems;
}

const void *vec_at(const struct vector *vec, size_t pos)
{
        return (vec && pos < vec->num_elems) ? vec->base + pos * vec->elem_size : NULL;
}

size_t vec_capacity(const struct vector *vec)
{
        error_if_null(vec)
        return vec->cap_elems;
}

bool vec_enlarge_size_to_capacity(struct vector *vec)
{
        error_if_null(vec);
        vec->num_elems = vec->cap_elems;
        return true;
}

NG5_EXPORT(bool) vec_zero_memory(struct vector *vec)
{
        error_if_null(vec);
        ng5_zero_memory(vec->base, vec->elem_size * vec->num_elems);
        return true;
}

NG5_EXPORT(bool) vec_zero_memory_in_range(struct vector *vec, size_t from, size_t to)
{
        error_if_null(vec);
        assert(from < to);
        assert(to <= vec->cap_elems);
        ng5_zero_memory(vec->base + from * vec->elem_size, vec->elem_size * (to - from));
        return true;
}

bool vec_set(struct vector *vec, size_t pos, const void *data)
{
        error_if_null(vec)
        assert(pos < vec->num_elems);
        memcpy(vec->base + pos * vec->elem_size, data, vec->elem_size);
        return true;
}

bool vec_cpy(struct vector *dst, const struct vector *src)
{
        ng5_check_success(vec_create(dst, NULL, src->elem_size, src->num_elems));
        dst->num_elems = src->num_elems;
        if (dst->num_elems > 0) {
                memcpy(dst->base, src->base, src->elem_size * src->num_elems);
        }
        return true;
}

NG5_EXPORT(bool) vec_cpy_to(struct vector *dst, struct vector *src)
{
        error_if_null(dst)
        error_if_null(src)
        void *handle = realloc(dst->base, src->cap_elems * src->elem_size);
        if (handle) {
                dst->elem_size = src->elem_size;
                dst->num_elems = src->num_elems;
                dst->cap_elems = src->cap_elems;
                dst->grow_factor = src->grow_factor;
                dst->base = handle;
                memcpy(dst->base, src->base, src->cap_elems * src->elem_size);
                error_cpy(&dst->err, &src->err);
                return true;
        } else {
                error(&src->err, NG5_ERR_HARDCOPYFAILED)
                return false;
        }
}

const void *vec_data(const struct vector *vec)
{
        return vec ? vec->base : NULL;
}

char *vector_string(const struct vector ofType(T) *vec,
        void (*printerFunc)(struct memfile *dst, void ofType(T) *values, size_t num_elems))
{
        struct memblock *block;
        struct memfile file;
        memblock_create(&block, vec->num_elems * vec->elem_size);
        memfile_open(&file, block, READ_WRITE);
        printerFunc(&file, vec->base, vec->num_elems);
        return memblock_move_contents_and_drop(block);
}
