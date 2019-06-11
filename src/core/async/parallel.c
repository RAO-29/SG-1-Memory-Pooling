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

#include "core/async/parallel.h"

NG5_EXPORT(void *)parallel_for_proxy_function(void *args)
{
        ng5_cast(struct parallel_func_proxy *, proxy_arg, args);
        proxy_arg->function(proxy_arg->start, proxy_arg->width, proxy_arg->len, proxy_arg->args, proxy_arg->tid);
        return NULL;
}

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
        enum threading_hint hint, uint_fast16_t num_threads)
{
        parallel_match(parallel_sequential_for(base, width, len, f, args),
                parallel_parallel_for(base, width, len, f, args, num_threads))
}

NG5_EXPORT(bool) parallel_map(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads)
{
        return parallel_map_exec(dst, src, src_width, len, dst_width, f, args, hint, num_threads);
}

NG5_EXPORT(bool) parallel_gather(void *dst, const void *src, size_t width, const size_t *idx, size_t dst_src_len,
        enum threading_hint hint, uint_fast16_t num_threads)
{
        parallel_match(parallel_sequential_gather(dst, src, width, idx, dst_src_len),
                parallel_parallel_gather(dst, src, width, idx, dst_src_len, num_threads))
}

NG5_EXPORT(bool) parallel_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads)
{
        parallel_match(parallel_sequential_gather_adr(dst, src, src_width, idx, num),
                parallel_parallel_gather_adr_func(dst, src, src_width, idx, num, num_threads))
}

NG5_EXPORT(bool) parallel_scatter(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        enum threading_hint hint, uint_fast16_t num_threads)
{
        parallel_match(parallel_sequential_scatter_func(dst, src, width, idx, num),
                parallel_parallel_scatter_func(dst, src, width, idx, num, num_threads))
}

NG5_EXPORT(bool) parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len, enum threading_hint hint)
{
        parallel_match(parallel_sequential_shuffle(dst, src, width, dst_idx, src_idx, idx_len),
                parallel_parallel_shuffle(dst, src, width, dst_idx, src_idx, idx_len))
}

NG5_EXPORT(bool) parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, uint_fast16_t num_threads)
{
        parallel_match(parallel_sequential_filter_early(result, result_size, src, width, len, pred, args),
                parallel_parallel_filter_early(result, result_size, src, width, len, pred, args, num_threads))
}

NG5_EXPORT(bool) parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, enum threading_hint hint, size_t num_threads)
{
        parallel_match(parallel_sequential_filter_late(pos, num_pos, src, width, len, pred, args),
                parallel_parallel_filter_late(pos, num_pos, src, width, len, pred, args, num_threads))
}

NG5_EXPORT(bool) parallel_sequential_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args)
{
        error_if_null(base)
        error_if_null(width)
        error_if_null(len)
        f(base, width, len, args, 0);
        return true;
}

NG5_EXPORT(bool) parallel_parallel_for(const void *base, size_t width, size_t len, parallel_for_body_func_t f,
        void *args, uint_fast16_t num_threads)
{
        error_if_null(base)
        error_if_null(width)

        if (len > 0) {
                uint_fast16_t num_thread = num_threads + 1; /** +1 since one is this thread */
                pthread_t threads[num_threads];
                struct parallel_func_proxy proxyArgs[num_thread];
                size_t chunk_len = len / num_thread;
                size_t chunk_len_remain = len % num_thread;
                const void *main_thread_base = base + num_threads * chunk_len * width;

                prefetch_read(f);
                prefetch_read(args);

                /** run f on NTHREADS_FOR additional threads */
                for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                        struct parallel_func_proxy *proxy_arg = proxyArgs + tid;
                        proxy_arg->start = base + tid * chunk_len * width;
                        proxy_arg->len = chunk_len;
                        proxy_arg->tid = (tid + 1);
                        proxy_arg->width = width;
                        proxy_arg->args = args;
                        proxy_arg->function = f;

                        prefetch_read(proxy_arg->start);
                        pthread_create(threads + tid, NULL, parallel_for_proxy_function, proxyArgs + tid);
                }
                /** run f on this thread */
                prefetch_read(main_thread_base);
                f(main_thread_base, width, chunk_len + chunk_len_remain, args, 0);

                for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                        pthread_join(threads[tid], NULL);
                }
        }
        return true;
}

void mapProxy(const void *src, size_t src_width, size_t len, void *args, thread_id_t tid)
{
        ng5_unused(tid);
        ng5_cast(struct map_args *, mapArgs, args);
        size_t globalStart = (src - mapArgs->src) / src_width;

        prefetch_read(mapArgs->src);
        prefetch_read(mapArgs->args);
        prefetch_write(mapArgs->dst);
        mapArgs->map_func(mapArgs->dst + globalStart * mapArgs->dst_width,
                src,
                src_width,
                mapArgs->dst_width,
                len,
                mapArgs->args);
}

NG5_EXPORT(bool) parallel_map_exec(void *dst, const void *src, size_t src_width, size_t len, size_t dst_width,
        parallel_map_body_func_t f, void *args, enum threading_hint hint, uint_fast16_t num_threads)
{
        error_if_null(src)
        error_if_null(src_width)
        error_if_null(dst_width)
        error_if_null(f)

        prefetch_read(f);
        prefetch_write(dst);

        struct map_args mapArgs = {.args = args, .map_func = f, .dst = dst, .dst_width = dst_width, .src = src};

        return parallel_for((void *) src, src_width, len, &mapProxy, &mapArgs, hint, num_threads);
}

void gather_function(const void *start, size_t width, size_t len, void *args, thread_id_t tid)
{
        ng5_unused(tid);
        ng5_cast(struct gather_scatter_args *, gather_args, args);
        size_t global_index_start = (start - gather_args->dst) / width;

        prefetch_write(gather_args->dst);
        prefetch_write(gather_args->idx);
        prefetch_read((len > 0) ? gather_args->src + gather_args->idx[0] * width : NULL);

        for (register size_t i = 0, next_i = 1; i < len; next_i = ++i + 1) {
                size_t global_index_cur = global_index_start + i;
                size_t global_index_next = global_index_start + next_i;
                memcpy(gather_args->dst + global_index_cur * width,
                        gather_args->src + gather_args->idx[global_index_cur] * width,
                        width);

                bool has_next = (next_i < len);
                prefetch_read(has_next ? gather_args->idx + global_index_next : NULL);
                prefetch_read(has_next ? gather_args->src + gather_args->idx[global_index_next] * width : NULL);
                prefetch_write(has_next ? gather_args->dst + global_index_next * width : NULL);
        }
}

NG5_EXPORT(bool) parallel_sequential_gather(void *dst, const void *src, size_t width, const size_t *idx,
        size_t dst_src_len)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(width)

        prefetch_read(src);
        prefetch_read(idx);
        prefetch_write(dst);

        prefetch_read(idx);
        prefetch_write(dst);
        prefetch_read((dst_src_len > 0) ? src + idx[0] * width : NULL);

        for (register size_t i = 0, next_i = 1; i < dst_src_len; next_i = ++i + 1) {
                memcpy(dst + i * width, src + idx[i] * width, width);

                bool has_next = (next_i < dst_src_len);
                prefetch_read(has_next ? idx + next_i : NULL);
                prefetch_read(has_next ? src + idx[next_i] * width : NULL);
                prefetch_write(has_next ? dst + next_i * width : NULL);
        }

        return true;
}

NG5_EXPORT(bool) parallel_parallel_gather(void *dst, const void *src, size_t width, const size_t *idx,
        size_t dst_src_len, uint_fast16_t num_threads)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(width)

        prefetch_read(src);
        prefetch_read(idx);
        prefetch_write(dst);

        struct gather_scatter_args args = {.idx           = idx, .src           = src, .dst           = dst,};
        return parallel_parallel_for(dst, width, dst_src_len, gather_function, &args, num_threads);
}

NG5_EXPORT(bool) parallel_sequential_gather_adr(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(src_width)

        prefetch_read(src);
        prefetch_read(idx);
        prefetch_write(dst);

        prefetch_read(idx);
        prefetch_write(dst);
        prefetch_read(num > 0 ? src + idx[0] * src_width : NULL);

        for (register size_t i = 0, next_i = 1; i < num; next_i = ++i + 1) {
                const void *ptr = src + idx[i] * src_width;
                size_t adr = (size_t) ptr;
                memcpy(dst + i * sizeof(void *), &adr, sizeof(size_t));

                bool has_next = (next_i < num);
                prefetch_read(has_next ? idx + next_i : NULL);
                prefetch_read(has_next ? src + idx[next_i] * src_width : NULL);
                prefetch_write(has_next ? dst + next_i * sizeof(void *) : NULL);
        }
        return true;
}

void parallel_gather_adr_func(const void *start, size_t width, size_t len, void *args, thread_id_t tid)
{
        ng5_unused(tid);
        ng5_cast(struct gather_scatter_args *, gather_args, args);

        prefetch_read(gather_args->idx);
        prefetch_write(gather_args->dst);
        prefetch_read((len > 0) ? gather_args->src + gather_args->idx[0] * width : NULL);

        size_t global_index_start = (start - gather_args->dst) / width;
        for (register size_t i = 0, next_i = 1; i < len; next_i = ++i + 1) {
                size_t global_index_cur = global_index_start + i;
                size_t global_index_next = global_index_start + next_i;
                const void *ptr = gather_args->src + gather_args->idx[global_index_cur] * width;
                size_t adr = (size_t) ptr;
                memcpy(gather_args->dst + global_index_cur * sizeof(void *), &adr, sizeof(size_t));

                bool has_next = (next_i < len);
                prefetch_read(has_next ? gather_args->idx + global_index_next : NULL);
                prefetch_read(has_next ? gather_args->src + gather_args->idx[global_index_next] * width : NULL);
                prefetch_write(has_next ? gather_args->dst + global_index_next * sizeof(void *) : NULL);
        }
}

NG5_EXPORT(bool) parallel_parallel_gather_adr_func(void *dst, const void *src, size_t src_width, const size_t *idx,
        size_t num, uint_fast16_t num_threads)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(src_width)

        prefetch_read(src);
        prefetch_read(idx);
        prefetch_write(dst);

        struct gather_scatter_args args = {.idx = idx, .src = src, .dst = dst};
        return parallel_parallel_for(dst, src_width, num, parallel_gather_adr_func, &args, num_threads);
}

void parallel_scatter_func(const void *start, size_t width, size_t len, void *args, thread_id_t tid)
{
        ng5_unused(tid);
        ng5_cast(struct gather_scatter_args *, scatter_args, args);

        prefetch_read(scatter_args->idx);
        prefetch_read(scatter_args->src);
        prefetch_write((len > 0) ? scatter_args->dst + scatter_args->idx[0] * width : NULL);

        size_t global_index_start = (start - scatter_args->dst) / width;
        for (register size_t i = 0, next_i = 1; i < len; next_i = ++i + 1) {
                size_t global_index_cur = global_index_start + i;
                size_t global_index_next = global_index_start + next_i;

                memcpy(scatter_args->dst + scatter_args->idx[global_index_cur] * width,
                        scatter_args->src + global_index_cur * width,
                        width);

                bool has_next = (next_i < len);
                prefetch_read(has_next ? scatter_args->idx + global_index_next : NULL);
                prefetch_read(has_next ? scatter_args->src + global_index_next * width : NULL);
                prefetch_write(has_next ? scatter_args->dst + scatter_args->idx[global_index_next] * width : NULL);
        }
}

NG5_EXPORT(bool) parallel_sequential_scatter_func(void *dst, const void *src, size_t width, const size_t *idx,
        size_t num)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(width)

        prefetch_read(idx);
        prefetch_read(src);
        prefetch_write((num > 0) ? dst + idx[0] * width : NULL);

        for (register size_t i = 0, next_i = 1; i < num; next_i = ++i + 1) {
                memcpy(dst + idx[i] * width, src + i * width, width);

                bool has_next = (next_i < num);
                prefetch_read(has_next ? idx + next_i : NULL);
                prefetch_read(has_next ? src + next_i * width : NULL);
                prefetch_write(has_next ? dst + idx[next_i] * width : NULL);
        }
        return true;
}

NG5_EXPORT(bool) parallel_parallel_scatter_func(void *dst, const void *src, size_t width, const size_t *idx, size_t num,
        uint_fast16_t num_threads)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(idx)
        error_if_null(width)

        prefetch_read(src);
        prefetch_read(idx);
        prefetch_write(dst);

        struct gather_scatter_args args = {.idx = idx, .src = src, .dst = dst};
        return parallel_parallel_for(dst, width, num, parallel_scatter_func, &args, num_threads);
}

NG5_EXPORT(bool) parallel_sequential_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len)
{
        error_if_null(dst)
        error_if_null(src)
        error_if_null(dst_idx)
        error_if_null(src_idx)
        error_if_null(width)

        bool has_first = (idx_len > 0);
        prefetch_read(src_idx);
        prefetch_read(dst_idx);
        prefetch_read(has_first ? src + src_idx[0] * width : NULL);
        prefetch_write(has_first ? dst + dst_idx[0] * width : NULL);

        for (register size_t i = 0, next_i = 1; i < idx_len; next_i = ++i + 1) {
                memcpy(dst + dst_idx[i] * width, src + src_idx[i] * width, width);

                bool has_next = (next_i < idx_len);
                prefetch_read(has_next ? src_idx + next_i : NULL);
                prefetch_read(has_next ? dst_idx + next_i : NULL);
                prefetch_read(has_next ? src + src_idx[next_i] * width : NULL);
                prefetch_write(has_next ? dst + dst_idx[next_i] * width : NULL);
        }

        return true;
}

NG5_EXPORT(bool) parallel_parallel_shuffle(void *dst, const void *src, size_t width, const size_t *dst_idx,
        const size_t *src_idx, size_t idx_len)
{
        ng5_unused(dst);
        ng5_unused(src);
        ng5_unused(width);
        ng5_unused(dst_idx);
        ng5_unused(src_idx);
        ng5_unused(idx_len);
        NG5_NOT_IMPLEMENTED
}

NG5_EXPORT(bool) parallel_sequential_filter_late(size_t *positions, size_t *num_positions, const void *source,
        size_t width, size_t length, parallel_predicate_func_t predicate, void *arguments)
{
        error_if_null(positions);
        error_if_null(num_positions);
        error_if_null(source);
        error_if_null(width);
        error_if_null(length);
        error_if_null(predicate);

        predicate(positions, num_positions, source, width, length, arguments, 0);

        return true;
}

void *parallel_filter_proxy_func(void *args)
{
        ng5_cast(struct filter_arg *, proxy_arg, args);
        proxy_arg->pred(proxy_arg->src_positions,
                &proxy_arg->num_positions,
                proxy_arg->start,
                proxy_arg->width,
                proxy_arg->len,
                proxy_arg->args,
                proxy_arg->position_offset_to_add);
        return NULL;
}

NG5_EXPORT(bool) parallel_parallel_filter_late(size_t *pos, size_t *num_pos, const void *src, size_t width, size_t len,
        parallel_predicate_func_t pred, void *args, size_t num_threads)
{
        error_if_null(pos);
        error_if_null(num_pos);
        error_if_null(src);
        error_if_null(width);
        error_if_null(pred);

        if (unlikely(len == 0)) {
                *num_pos = 0;
                return true;
        }

        uint_fast16_t num_thread = num_threads + 1; /** +1 since one is this thread */

        pthread_t threads[num_threads];
        struct filter_arg thread_args[num_thread];

        register size_t chunk_len = len / num_thread;
        size_t chunk_len_remain = len % num_thread;
        size_t main_position_offset_to_add = num_threads * chunk_len;
        const void *main_thread_base = src + main_position_offset_to_add * width;

        prefetch_read(pred);
        prefetch_read(args);

        /** run f on NTHREADS_FOR additional threads */
        if (likely(chunk_len > 0)) {
                for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                        struct filter_arg *arg = thread_args + tid;
                        arg->num_positions = 0;
                        arg->src_positions = malloc(chunk_len * sizeof(size_t));
                        arg->position_offset_to_add = tid * chunk_len;
                        arg->start = src + arg->position_offset_to_add * width;
                        arg->len = chunk_len;
                        arg->width = width;
                        arg->args = args;
                        arg->pred = pred;

                        prefetch_read(arg->start);
                        pthread_create(threads + tid, NULL, parallel_filter_proxy_func, arg);
                }
        }
        /** run f on this thread */
        prefetch_read(main_thread_base);
        size_t main_chunk_len = chunk_len + chunk_len_remain;
        size_t *main_src_positions = malloc(main_chunk_len * sizeof(size_t));
        size_t main_num_positions = 0;

        pred(main_src_positions,
                &main_num_positions,
                main_thread_base,
                width,
                main_chunk_len,
                args,
                main_position_offset_to_add);

        size_t total_num_matching_positions = 0;

        if (likely(chunk_len > 0)) {
                for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                        pthread_join(threads[tid], NULL);
                        const struct filter_arg *thread_arg = (thread_args + tid);
                        if (thread_arg->num_positions > 0) {
                                memcpy(pos + total_num_matching_positions,
                                        thread_arg->src_positions,
                                        thread_arg->num_positions * sizeof(size_t));
                                total_num_matching_positions += thread_arg->num_positions;
                        }
                        free(thread_args[tid].src_positions);
                }
        }

        if (likely(main_num_positions > 0)) {
                memcpy(pos + total_num_matching_positions, main_src_positions, main_num_positions * sizeof(size_t));
                total_num_matching_positions += main_num_positions;
        }
        free(main_src_positions);

        *num_pos = total_num_matching_positions;

        return true;
}

NG5_EXPORT(bool) parallel_sequential_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args)
{
        error_if_null(result);
        error_if_null(result_size);
        error_if_null(src);
        error_if_null(width);
        error_if_null(len);
        error_if_null(pred);

        size_t num_matching_positions;
        size_t *matching_positions = malloc(len * sizeof(size_t));

        pred(matching_positions, &num_matching_positions, src, width, len, args, 0);

        parallel_gather(result, src, width, matching_positions, num_matching_positions, THREADING_HINT_SINGLE, 0);
        *result_size = num_matching_positions;

        free(matching_positions);

        return true;
}

NG5_EXPORT(bool) parallel_parallel_filter_early(void *result, size_t *result_size, const void *src, size_t width,
        size_t len, parallel_predicate_func_t pred, void *args, uint_fast16_t num_threads)
{
        error_if_null(result);
        error_if_null(result_size);
        error_if_null(src);
        error_if_null(width);
        error_if_null(len);
        error_if_null(pred);

        uint_fast16_t num_thread = num_threads + 1; /** +1 since one is this thread */

        pthread_t threads[num_threads];
        struct filter_arg thread_args[num_thread];

        register size_t chunk_len = len / num_thread;
        size_t chunk_len_remain = len % num_thread;
        size_t main_position_offset_to_add = num_threads * chunk_len;
        const void *main_thread_base = src + main_position_offset_to_add * width;

        prefetch_read(pred);
        prefetch_read(args);

        /** run f on NTHREADS_FOR additional threads */
        for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                struct filter_arg *arg = thread_args + tid;
                arg->num_positions = 0;
                arg->src_positions = malloc(chunk_len * sizeof(size_t));
                arg->position_offset_to_add = tid * chunk_len;
                arg->start = src + arg->position_offset_to_add * width;
                arg->len = chunk_len;
                arg->width = width;
                arg->args = args;
                arg->pred = pred;

                prefetch_read(arg->start);
                pthread_create(threads + tid, NULL, parallel_filter_proxy_func, arg);
        }
        /** run f on this thread */
        prefetch_read(main_thread_base);
        size_t main_chunk_len = chunk_len + chunk_len_remain;
        size_t *main_src_positions = malloc(main_chunk_len * sizeof(size_t));
        size_t main_num_positions = 0;

        pred(main_src_positions,
                &main_num_positions,
                main_thread_base,
                width,
                main_chunk_len,
                args,
                main_position_offset_to_add);

        size_t total_num_matching_positions = main_num_positions;
        size_t partial_num_matching_positions = 0;

        for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                pthread_join(threads[tid], NULL);
                const struct filter_arg *thread_arg = (thread_args + tid);
                total_num_matching_positions += thread_arg->num_positions;
                prefetch_read(thread_arg->src_positions);
        }

        for (register uint_fast16_t tid = 0; tid < num_threads; tid++) {
                const struct filter_arg *thread_arg = (thread_args + tid);

                if (likely(thread_arg->num_positions > 0)) {
                        parallel_gather(result + partial_num_matching_positions * width,
                                src,
                                width,
                                thread_arg->src_positions,
                                thread_arg->num_positions,
                                THREADING_HINT_MULTI,
                                num_threads);
                }

                partial_num_matching_positions += thread_arg->num_positions;
                free(thread_arg->src_positions);
        }

        if (likely(main_num_positions > 0)) {
                parallel_gather(result + partial_num_matching_positions * width,
                        src,
                        width,
                        main_src_positions,
                        main_num_positions,
                        THREADING_HINT_MULTI,
                        num_threads);
        }
        free(main_src_positions);

        *result_size = total_num_matching_positions;

        return true;
}