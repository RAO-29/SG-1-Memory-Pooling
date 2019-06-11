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

#ifndef NG5_STRDIC_H
#define NG5_STRDIC_H

#include <assert.h>

#include "shared/common.h"
#include "core/alloc/alloc.h"
#include "shared/types.h"
#include "hash/hash.h"
#include "std/vec.h"

NG5_BEGIN_DECL

NG5_FORWARD_STRUCT_DECL(StringDictionary)
NG5_FORWARD_STRUCT_DECL(Vector)

struct strhash_counters;

enum strdic_tag {
        SYNC, ASYNC
};

/**
 * Thread-safe string pool implementation
 */
struct strdic {
        /**
         * Implementation-specific fields
         */
        void *extra;

        /**
         * Tag determining the current implementation
         */
        enum strdic_tag tag;

        /**
         * Memory allocator that is used to get memory for user data
         */
        struct allocator alloc;

        /**
         * Frees up implementation-specific resources.
         *
         * Note: Implementation must ensure thread-safeness
         */
        bool (*drop)(struct strdic *self);

        /**
         * Inserts a particular number of strings into this dictionary and returns associated string identifiers.
         *
         * Note: Implementation must ensure thread-safeness
        */
        bool (*insert)(struct strdic *self, field_sid_t **out, char *const *strings, size_t nstrings, size_t nthreads);

        /**
         * Removes a particular number of strings from this dictionary by their ids. The caller must ensure that
         * all string identifiers in <code>strings</code> are valid.
         *
         * Note: Implementation must ensure thread-safeness
         */
        bool (*remove)(struct strdic *self, field_sid_t *strings, size_t nstrings);

        /**
         * Get the string ids associated with <code>keys</code> in this parallel_map_exec (if any).
         *
         * Note: Implementation must ensure thread-safeness
         */
        bool (*locate_safe)(struct strdic *self, field_sid_t **out, bool **found_mask, size_t *num_not_found,
                char *const *keys, size_t num_keys);

        /**
         * Get the string ids associated with <code>keys</code> in this dic. All keys <u>must</u> exist.
         *
         * Note: Implementation must ensure thread-safeness
        */
        bool (*locate_fast)(struct strdic *self, field_sid_t **out, char *const *keys, size_t num_keys);

        /**
         * Extracts strings given their string identifier. All <code>ids</code> must be known.
         *
         * Note: Implementation must ensure thread-safeness
         */
        char **(*extract)(struct strdic *self, const field_sid_t *ids, size_t num_ids);

        /**
         * Frees up memory allocated inside a function call via the allocator given in the constructor
         *
         * Note: Implementation must ensure thread-safeness
         */
        bool (*free)(struct strdic *self, void *ptr);

        /**
         * Reset internal statistic counters
         */
        bool (*resetCounters)(struct strdic *self);

        /**
         * Get internal statistic counters
         */
        bool (*counters)(struct strdic *self, struct strhash_counters *counters);

        /**
         * Returns number of distinct strings stored in the dictionary
         */
        bool (*num_distinct)(struct strdic *self, size_t *num);

        /**
         * Returns all contained (unique) strings and their mapped (unique) ids
         */
        bool (*get_contents)(struct strdic *self, struct vector ofType (char *) *strings,
                struct vector ofType(field_sid_t) *string_ids);
};

/**
 *
 * @param dic
 * @return
 */
ng5_func_unused
static bool strdic_drop(struct strdic *dic)
{
        error_if_null(dic);
        assert(dic->drop);
        return dic->drop(dic);
}

ng5_func_unused
static bool strdic_insert(struct strdic *dic, field_sid_t **out, char *const *strings, size_t nstrings, size_t nthreads)
{
        error_if_null(dic);
        error_if_null(strings);
        assert(dic->insert);
        return dic->insert(dic, out, strings, nstrings, nthreads);
}

ng5_func_unused
static bool strdic_reset_counters(struct strdic *dic)
{
        error_if_null(dic);
        assert(dic->resetCounters);
        return dic->resetCounters(dic);
}

ng5_func_unused
static bool strdic_get_counters(struct strhash_counters *counters, struct strdic *dic)
{
        error_if_null(dic);
        assert(dic->counters);
        return dic->counters(dic, counters);
}

ng5_func_unused
static bool strdic_remove(struct strdic *dic, field_sid_t *strings, size_t num_strings)
{
        error_if_null(dic);
        error_if_null(strings);
        assert(dic->remove);
        return dic->remove(dic, strings, num_strings);
}

ng5_func_unused
static bool strdic_locate_safe(field_sid_t **out, bool **found_mask, size_t *num_not_found, struct strdic *dic,
        char *const *keys, size_t num_keys)
{
        error_if_null(out);
        error_if_null(found_mask);
        error_if_null(num_not_found);
        error_if_null(dic);
        error_if_null(keys);
        assert(dic->locate_safe);
        return dic->locate_safe(dic, out, found_mask, num_not_found, keys, num_keys);
}

ng5_func_unused
static bool strdic_locate_fast(field_sid_t **out, struct strdic *dic, char *const *keys, size_t nkeys)
{
        error_if_null(out);
        error_if_null(dic);
        error_if_null(keys);
        assert(dic->locate_fast);
        return dic->locate_fast(dic, out, keys, nkeys);
}

ng5_func_unused
static char **strdic_extract(struct strdic *dic, const field_sid_t *ids, size_t nids)
{
        return dic->extract(dic, ids, nids);
}

ng5_func_unused
static bool strdic_free(struct strdic *dic, void *ptr)
{
        error_if_null(dic);
        if (ptr) {
                assert(dic->free);
                return dic->free(dic, ptr);
        } else {
                return true;
        }
}

ng5_func_unused
static bool strdic_num_distinct(size_t *num, struct strdic *dic)
{
        error_if_null(num);
        error_if_null(dic);
        assert(dic->num_distinct);
        return dic->num_distinct(dic, num);
}

ng5_func_unused
static bool strdic_get_contents(struct vector ofType (char *) *strings, struct vector ofType(field_sid_t) *string_ids,
        struct strdic *dic)
{
        error_if_null(strings)
        error_if_null(string_ids)
        error_if_null(dic);
        assert(dic->get_contents);
        return dic->get_contents(dic, strings, string_ids);
}

NG5_END_DECL

#endif
