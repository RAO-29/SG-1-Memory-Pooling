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

#ifndef NG5_VECTOR_H
#define NG5_VECTOR_H

#include <sys/mman.h>

#include "shared/common.h"
#include "core/alloc/alloc.h"
#include "core/mem/file.h"

NG5_BEGIN_DECL

#define DECLARE_PRINTER_FUNC(type)                                                                                     \
    void vector_##type##_PrinterFunc(struct memfile *dst, void ofType(T) *values, size_t num_elems);

DECLARE_PRINTER_FUNC(u_char)
DECLARE_PRINTER_FUNC(i8)
DECLARE_PRINTER_FUNC(i16)
DECLARE_PRINTER_FUNC(i32)
DECLARE_PRINTER_FUNC(i64)
DECLARE_PRINTER_FUNC(u8)
DECLARE_PRINTER_FUNC(u16)
DECLARE_PRINTER_FUNC(u32)
DECLARE_PRINTER_FUNC(u64)
DECLARE_PRINTER_FUNC(size_t)

#define VECTOR_PRINT_UCHAR  vector_u_char_PrinterFunc
#define VECTOR_PRINT_UINT8  vector_u8_PrinterFunc
#define VECTOR_PRINT_UINT16 vector_u16_PrinterFunc
#define VECTOR_PRINT_UINT32 vector_u32_PrinterFunc
#define VECTOR_PRINT_UINT64 vector_u64_PrinterFunc
#define VECTOR_PRINT_INT8   vector_i8_PrinterFunc
#define VECTOR_PRINT_INT16  vector_i16_PrinterFunc
#define VECTOR_PRINT_INT32  vector_i32_PrinterFunc
#define VECTOR_PRINT_INT64  vector_i64_PrinterFunc
#define VECTOR_PRINT_SIZE_T vector_size_t_PrinterFunc

/**
 * An implementation of the concrete data type Vector, a resizeable dynamic array.
 */
struct vector {
        /**
        *  Memory allocator that is used to get memory for user data
        */
        struct allocator *allocator;

        /**
         *  Fixed number of bytes for a single element that should be stored in the vector
         */
        size_t elem_size;

        /**
         *  The number of elements currently stored in the vector
         */
        u32 num_elems;

        /**
         *  The number of elements for which currently memory is reserved
         */
        u32 cap_elems;

        /**
        * The grow factor considered for resize operations
        */
        float grow_factor;

        /**
         * A pointer to a memory address managed by 'allocator' that contains the user data
         */
        void *base;

        /**
         *  Error information
         */
        struct err err;
};

/**
 * Utility implementation of generic vector to specialize for type of 'char *'
 */
typedef struct vector ofType(const char *) string_vector_t;

/**
 * Constructs a new vector for elements of size 'elem_size', reserving memory for 'cap_elems' elements using
 * the allocator 'alloc'.
 *
 * @param out non-null vector that should be constructed
 * @param alloc an allocator
 * @param elem_size fixed-length element size
 * @param cap_elems number of elements for which memory should be reserved
 * @return STATUS_OK if success, and STATUS_NULLPTR in case of NULL pointer parameters
 */
NG5_EXPORT(bool) vec_create(struct vector *out, const struct allocator *alloc, size_t elem_size, size_t cap_elems);

NG5_EXPORT(bool) vec_serialize(FILE *file, struct vector *vec);

NG5_EXPORT(bool) vec_deserialize(struct vector *vec, struct err *err, FILE *file);

/**
 * Provides hints on the OS kernel how to deal with memory inside this vector.
 *
 * @param vec non-null vector
 * @param madviseAdvice value to give underlying <code>madvise</code> syscall and advice, see man page
 * of <code>madvise</code>
 * @return STATUS_OK if success, otherwise a value indicating the error
 */
NG5_EXPORT(bool) vec_memadvice(struct vector *vec, int madviseAdvice);

/**
 * Sets the factor for determining the reallocation size in case of a resizing operation.
 *
 * Note that <code>factor</code> must be larger than one.
 *
 * @param vec non-null vector for which the grow factor should be changed
 * @param factor a positive real number larger than 1
 * @return STATUS_OK if success, otherwise a value indicating the error
 */
NG5_EXPORT(bool) vec_set_grow_factor(struct vector *vec, float factor);

/**
 * Frees up memory requested via the allocator.
 *
 * Depending on the allocator implementation, dropping the reserved memory might not take immediately effect.
 * The pointer 'vec' itself gets not freed.
 *
 * @param vec vector to be freed
 * @return STATUS_OK if success, and STATUS_NULL_PTR in case of NULL pointer to 'vec'
 */
NG5_EXPORT(bool) vec_drop(struct vector *vec);

/**
 * Returns information on whether elements are stored in this vector or not.
 * @param vec non-null pointer to the vector
 * @return Returns <code>STATUS_TRUE</code> if <code>vec</code> is empty. Otherwise <code>STATUS_FALSE</code> unless
 *         an error occurs. In case an error is occured, the return value is neither <code>STATUS_TRUE</code> nor
 *         <code>STATUS_FALSE</code> but an value indicating that error.
 */
NG5_EXPORT(bool) vec_is_empty(const struct vector *vec);

/**
 * Appends 'num_elems' elements stored in 'data' into the vector by copying num_elems * vec->elem_size into the
 * vectors memory block.
 *
 * In case the capacity is not sufficient, the vector gets automatically resized.
 *
 * @param vec the vector in which the data should be pushed
 * @param data non-null pointer to data that should be appended. Must be at least size of 'num_elems' * vec->elem_size.
 * @param num_elems number of elements stored in data
 * @return STATUS_OK if success, and STATUS_NULLPTR in case of NULL pointer parameters
 */
NG5_EXPORT(bool) vec_push(struct vector *vec, const void *data, size_t num_elems);

NG5_EXPORT(const void *)vec_peek(struct vector *vec);

#define VECTOR_PEEK(vec, type) (type *)(vec_peek(vec))

/**
 * Appends 'how_many' elements of the same source stored in 'data' into the vector by copying how_many * vec->elem_size
 * into the vectors memory block.
 *
 * In case the capacity is not sufficient, the vector gets automatically resized.
 *
 * @param vec the vector in which the data should be pushed
 * @param data non-null pointer to data that should be appended. Must be at least size of one vec->elem_size.
 * @param num_elems number of elements stored in data
 * @return STATUS_OK if success, and STATUS_NULLPTR in case of NULL pointer parameters
 */
NG5_EXPORT(bool) vec_repeated_push(struct vector *vec, const void *data, size_t how_often);

/**
 * Returns a pointer to the last element in this vector, or <code>NULL</code> is the vector is already empty.
 * The number of elements contained in that vector is decreased, too.
 *
 * @param vec non-null pointer to the vector
 * @return Pointer to last element, or <code>NULL</code> if vector is empty
 */
NG5_EXPORT(const void *)vec_pop(struct vector *vec);

NG5_EXPORT(bool) vec_clear(struct vector *vec);

/**
 * Shinks the vector's internal data block to fits its real size, i.e., remove reserved memory
 *
 * @param vec
 * @return
 */
NG5_EXPORT(bool) vec_shrink(struct vector *vec);

/**
 * Increases the capacity of that vector according the internal grow factor
 * @param numNewSlots a pointer to a value that will store the number of newly created slots in that vector if
 *                      <code>num_new_slots</code> is non-null. If this parameter is <code>NULL</code>, it is ignored.
 * @param vec non-null pointer to the vector that should be grown
 * @return STATUS_OK in case of success, and another value indicating an error otherwise.
 */
NG5_EXPORT(bool) vec_grow(size_t *numNewSlots, struct vector *vec);

NG5_EXPORT(bool) vec_grow_to(struct vector *vec, size_t capacity);

/**
 * Returns the number of elements currently stored in the vector
 *
 * @param vec the vector for which the operation is started
 * @return 0 in case of NULL pointer to 'vec', or the number of elements otherwise.
 */
NG5_EXPORT(size_t) vec_length(const struct vector *vec);

#define vec_get(vec, pos, type) (type *) vec_at(vec, pos)

#define vec_new_and_get(vec, type)                                                                                     \
({                                                                                                                     \
    type template;                                                                                                     \
    size_t vectorLength = vec_length(vec);                                                                             \
    vec_push(vec, &template, 1);                                                                                       \
    vec_get(vec, vectorLength, type);                                                                                  \
})

NG5_EXPORT(const void *) vec_at(const struct vector *vec, size_t pos);

/**
 * Returns the number of elements for which memory is currently reserved in the vector
 *
 * @param vec the vector for which the operation is started
 * @return 0 in case of NULL pointer to 'vec', or the number of elements otherwise.
 */
NG5_EXPORT(size_t) vec_capacity(const struct vector *vec);

/**
 * Set the internal size of <code>vec</code> to its capacity.
 */
NG5_EXPORT(bool) vec_enlarge_size_to_capacity(struct vector *vec);

NG5_EXPORT(bool) vec_zero_memory(struct vector *vec);

NG5_EXPORT(bool) vec_zero_memory_in_range(struct vector *vec, size_t from, size_t to);

NG5_EXPORT(bool) vec_set(struct vector *vec, size_t pos, const void *data);

NG5_EXPORT(bool) vec_cpy(struct vector *dst, const struct vector *src);

NG5_EXPORT(bool) vec_cpy_to(struct vector *dst, struct vector *src);

/**
 * Gives raw data access to data stored in the vector; do not manipulate this data since otherwise the vector
 * might get corrupted.
 *
 * @param vec the vector for which the operation is started
 * @return pointer to user-data managed by this vector
 */
NG5_EXPORT(const void *)vec_data(const struct vector *vec);

NG5_EXPORT(char *) vector_string(const struct vector ofType(T) *vec,
        void (*printerFunc)(struct memfile *dst, void ofType(T) *values, size_t num_elems));

#define vec_all(vec, type) (type *) vec_data(vec)

NG5_END_DECL

#endif
