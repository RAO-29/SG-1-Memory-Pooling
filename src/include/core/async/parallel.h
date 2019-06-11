/**
 * Copyright 2018 Marcus Pinnecke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without ion, including without limitation the
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

#ifndef NG5_PARALLEL_H
#define NG5_PARALLEL_H

#include "shared/common.h"
#include "stdlib.h"
#include "shared/error.h"

NG5_BEGIN_DECL

#define PARALLEL_MSG_UNKNOWN_HINT "Unknown threading hint"

typedef uint_fast16_t thread_id_t;

typedef void (*parallel_for_body_func_t)(const void *start, size_t width, size_t len, void *args, thread_id_t tid);

typedef void
(*parallel_map_body_func_t)(void *dst, const void *src, size_t src_width, size_t dst_width, size_t len, void *args);

typedef void(*parallel_predicate_func_t)
        (size_t *matching_positions, size_t *num_matching_positions, const void *src, size_t width, size_t len,
                void *args, size_t position_offset_to_add);

enum threading_hint {
        THREADING_HINT_SINGLE, THREADING_HINT_MULTI
};

struct parallel_func_proxy {
        parallel_for_body_func_t function;
        const void *start;
        size_t width;
        size_t len;
        thread_id_t tid;
        void *args;
};

struct filter_arg {
        size_t num_positions;
        size_t *src_positions;
        const void *start;
        size_t len;
        size_t width;
        void *args;
        parallel_predicate_func_t pred;
        size_t position_offset_to_add;
};

NG5_EXPORT(void *)parallel_for_proxy_function(void *args);

#define ng5_parallel_error(msg, retval)                                                                             \
{                                                                                                                      \
    perror(msg);                                                                                                       \
    return retval;                                                                                                     \
}

#define parallel_match(forSingle, forMulti)                                                                            \
{                                                                                                                      \
    if (likely(hint == THREADING_HINT_MULTI)) {                                             \
        return (forMulti);                                                                                             \
    } else if (hint == THREADING_HINT_SINGLE) {                                                           \
        return (forSingle);                                                                                            \
    } else ng5_parallel_error(PARALLEL_MSG_UNKNOWN_HINT, false);                                                    \
}

NG5_EXPORT(bool) parallel_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f, void *args,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_map(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_gather(void *dst, const void *src, size_t width, const size_t *idx, size_t dstSrcLen,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_scatter(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idxLen, enum threading_hint hint);

NG5_EXPORT(bool) parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, size_t num_threads);

NG5_EXPORT(bool) parallel_sequential_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args);
NG5_EXPORT(bool) parallel_parallel_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_map_exec(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_gather(void *dst, const void *src, size_t width, const size_t *idx,
        size_t dstSrcLen);
NG5_EXPORT(bool) parallel_parallel_gather(void *dst, const void *src, size_t width, const size_t *idx, size_t dstSrcLen,
        uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num);
NG5_EXPORT(bool) parallel_parallel_gather_adr_func(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_scatter_func(void *dst, const void *src, size_t width, const size_t *idx,
        size_t num);
NG5_EXPORT(bool) parallel_parallel_scatter_func(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len);

NG5_EXPORT(bool) parallel_parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len);

NG5_EXPORT(bool) parallel_sequential_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args);

NG5_EXPORT(bool) parallel_parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args);

NG5_EXPORT(bool) parallel_parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, size_t num_threads);

NG5_EXPORT(bool) parallel_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f, void *args,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_map(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_gather(void *dst, const void *src, size_t width, const size_t *idx, size_t dst_src_len,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_scatter(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len, enum threading_hint hint);

NG5_EXPORT(bool) parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, size_t num_threads);

NG5_EXPORT(bool) parallel_sequential_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args);

NG5_EXPORT(bool) parallel_parallel_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args, uint_fast16_t num_threads);

struct map_args {
        parallel_map_body_func_t map_func;
        void *dst;
        const void *src;
        size_t dst_width;
        void *args;
};

void mapProxy(const void *src, size_t src_width, size_t len, void *args, thread_id_t tid);

NG5_EXPORT(bool) parallel_map_exec(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads);

struct gather_scatter_args {
        const size_t *idx;
        const void *src;
        void *dst;
};

void gather_function(const void *start, size_t width, size_t len, void *args, thread_id_t tid);

NG5_EXPORT(bool) parallel_sequential_gather(void *dst, const void *src, size_t width, const size_t *idx,
        size_t dst_src_len);

NG5_EXPORT(bool) parallel_parallel_gather(void *dst, const void *src, size_t width, const size_t *idx,
        size_t dst_src_len, uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num);

void parallel_gather_adr_func(const void *start, size_t width, size_t len, void *args, thread_id_t tid);

NG5_EXPORT(bool) parallel_parallel_gather_adr_func(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num, uint_fast16_t num_threads);

void parallel_scatter_func(const void *start, size_t width, size_t len, void *args, thread_id_t tid);

NG5_EXPORT(bool) parallel_sequential_scatter_func(void *dst, const void *src, size_t width, const size_t *idx,
        size_t num);

NG5_EXPORT(bool) parallel_parallel_scatter_func(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        uint_fast16_t num_threads);

NG5_EXPORT(bool) parallel_sequential_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len);

NG5_EXPORT(bool) parallel_parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len);

NG5_EXPORT(bool) parallel_sequential_filter_late(size_t *positions, size_t *num_positions, const void *source,
        size_t width, size_t length, parallel_predicate_func_t predicate, void *arguments);

void *parallel_filter_proxy_func(void *args);

NG5_EXPORT(bool) parallel_parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, size_t num_threads);

NG5_EXPORT(bool) parallel_sequential_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args);

NG5_EXPORT(bool) parallel_parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args, uint_fast16_t num_threads);

NG5_END_DECL

#endif
