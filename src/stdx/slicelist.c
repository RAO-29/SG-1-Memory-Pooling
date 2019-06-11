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
#include <math.h>

#include "stdx/slicelist.h"
#include "hash/add.h"
#include "hash/xor.h"
#include "hash/rot.h"
#include "hash/sax.h"

#define NG5_SLICE_LIST_TAG "slice-list"

#define get_hashcode(key)    NG5_HASH_ADDITIVE(strlen(key), key)

/** OPTIMIZATION: we have only one item to find. Use branch-less scan instead of branching scan */
/** OPTIMIZATION: find function as macro */
#define SLICE_SCAN(slice, needle_hash, needle_str)                                                                     \
({                                                                                                                     \
    ng5_trace(NG5_SLICE_LIST_TAG, "SLICE_SCAN for '%s' started", needle_str);                                    \
    assert(slice);                                                                                                     \
    assert(needle_str);                                                                                                \
                                                                                                                       \
    register bool continueScan, keysMatch, keyHashsNoMatch, endReached;                                                \
    register bool cacheAvailable = (slice->cacheIdx != (u32) -1);                                                 \
    register bool hashsEq = cacheAvailable && (slice->keyHashColumn[slice->cacheIdx] == needle_hash);                  \
    register bool cacheHit = hashsEq && (strcmp(slice->key_column[slice->cacheIdx], needle_str) == 0);                 \
    register uint_fast32_t i = 0;                                                                                      \
    if (!cacheHit) {                                                                                                   \
        do {                                                                                                           \
            while ((keyHashsNoMatch = (slice->keyHashColumn[i]!=needle_hash)) && i++<slice->num_elems) { ; }           \
            endReached    = ((i+1)>slice->num_elems);                                                                  \
            keysMatch      = endReached || (!keyHashsNoMatch && (strcmp(slice->key_column[i], needle_str)==0));        \
            continueScan  = !endReached && !keysMatch;                                                                 \
            i             += continueScan;                                                                             \
        }                                                                                                              \
        while (continueScan);                                                                                          \
        slice->cacheIdx = !endReached && keysMatch ? i : slice->cacheIdx;                                              \
    }                                                                                                                  \
    cacheHit ? slice->cacheIdx : (!endReached && keysMatch ? i : slice->num_elems);                                    \
})

#define SLICE_BESEARCH(slice, needle_hash, needle_str)                                                                 \
({                                                                                                                     \
    0; \
})

static void appenderNew(slice_list_t *list);
static void appenderSeal(Slice *slice);

static void lock(slice_list_t *list);
static void unlock(slice_list_t *list);

NG5_EXPORT(bool) slice_list_create(slice_list_t *list, const struct allocator *alloc, size_t sliceCapacity)
{
        error_if_null(list)
        error_if_null(sliceCapacity)

        alloc_this_or_std(&list->alloc, alloc);
        spin_init(&list->lock);
        error_init(&list->err);

        vec_create(&list->slices, &list->alloc, sizeof(Slice), sliceCapacity);
        vec_create(&list->descriptors, &list->alloc, sizeof(SliceDescriptor), sliceCapacity);
        vec_create(&list->filters, &list->alloc, sizeof(bloom_t), sliceCapacity);
        vec_create(&list->bounds, &list->alloc, sizeof(HashBounds), sliceCapacity);

        ng5_zero_memory(vec_data(&list->slices), sliceCapacity * sizeof(Slice));
        ng5_zero_memory(vec_data(&list->descriptors), sliceCapacity * sizeof(SliceDescriptor));
        ng5_zero_memory(vec_data(&list->filters), sliceCapacity * sizeof(bloom_t));
        ng5_zero_memory(vec_data(&list->bounds), sliceCapacity * sizeof(HashBounds));

        appenderNew(list);

        return true;
}

NG5_EXPORT(bool) SliceListDrop(slice_list_t *list)
{
        ng5_unused(list);
//    NOT_YET_IMPLEMENTED
        // TODO: implement
        vec_drop(&list->slices);
        vec_drop(&list->descriptors);
        vec_drop(&list->bounds);
        for (size_t i = 0; i < list->filters.num_elems; i++) {
                bloom_t *filter = vec_get(&list->filters, i, bloom_t);
                bloom_drop(filter);
        }
        vec_drop(&list->filters);
        return true;
}

NG5_EXPORT(bool) SliceListIsEmpty(const slice_list_t *list)
{
        return (vec_is_empty(&list->slices));
}

NG5_EXPORT(bool) slice_list_insert(slice_list_t *list, char **strings, field_sid_t *ids, size_t num_pairs)
{
        lock(list);

        while (num_pairs--) {
                const char *key = *strings++;
                field_sid_t value = *ids++;
                hash32_t keyHash = get_hashcode(key);
                slice_handle_t handle;
                int status;

                assert (key);

                /** check whether the keys-values pair is already contained in one slice */
                status = slice_list_lookup(&handle, list, key);

                if (status == true) {
                        /** pair was found, do not insert it twice */
                        assert (value == handle.value);
                        continue;
                } else {
                        /** pair is not found; append it */
                        HashBounds *restrict bounds = vec_all(&list->bounds, HashBounds);
                        bloom_t *restrict filters = vec_all(&list->filters, bloom_t);
                        Slice *restrict slices = vec_all(&list->slices, Slice);

                        if (list->appender_idx != 0) { ; // TODO: remove
                        }

                        Slice *restrict appender = slices + list->appender_idx;
                        bloom_t *restrict appenderFilter = filters + list->appender_idx;
                        HashBounds *restrict appenderBounds = bounds + list->appender_idx;

                        ng5_debug(NG5_SLICE_LIST_TAG,
                                "appender # of elems: %zu, limit: %zu",
                                appender->num_elems,
                                SLICE_KEY_COLUMN_MAX_ELEMS);
                        assert(appender->num_elems < SLICE_KEY_COLUMN_MAX_ELEMS);
                        appender->key_column[appender->num_elems] = key;
                        appender->keyHashColumn[appender->num_elems] = keyHash;
                        appender->string_id_tColumn[appender->num_elems] = value;
                        appenderBounds->minHash = appenderBounds->minHash < keyHash ? appenderBounds->minHash : keyHash;
                        appenderBounds->maxHash = appenderBounds->maxHash > keyHash ? appenderBounds->maxHash : keyHash;
                        NG5_BLOOM_SET(appenderFilter, &keyHash, sizeof(hash32_t));
                        appender->num_elems++;
                        if (unlikely(appender->num_elems == SLICE_KEY_COLUMN_MAX_ELEMS)) {
                                appenderSeal(appender);
                                appenderNew(list);
                        }
                }
        }

        unlock(list);
        return true;
}

NG5_EXPORT(bool) slice_list_lookup(slice_handle_t *handle, slice_list_t *list, const char *needle)
{
        ng5_unused(list);
        ng5_unused(handle);
        ng5_unused(needle);

        hash32_t keyHash = get_hashcode(needle);
        u32 numSlices = vec_length(&list->slices);

        /** check whether the keys-values pair is already contained in one slice */
        HashBounds *restrict bounds = vec_all(&list->bounds, HashBounds);
        bloom_t *restrict filters = vec_all(&list->filters, bloom_t);
        Slice *restrict slices = vec_all(&list->slices, Slice);
        SliceDescriptor *restrict descs = vec_all(&list->descriptors, SliceDescriptor);

        for (register u32 i = 0; i < numSlices; i++) {
                SliceDescriptor *restrict desc = descs + i;
                HashBounds *restrict bound = bounds + i;
                Slice *restrict slice = slices + i;

                desc->numReadsAll++;

                if (slice->num_elems > 0) {
                        bool keyHashIn = keyHash >= bound->minHash && keyHash <= bound->maxHash;
                        if (keyHashIn) {
                                bloom_t *restrict filter = filters + i;
                                bool maybeContained = NG5_BLOOM_TEST(filter, &keyHash, sizeof(hash32_t));
                                if (maybeContained) {
                                        ng5_debug(NG5_SLICE_LIST_TAG,
                                                "NG5_slice_list_lookup_by_key keys(%s) -> ?",
                                                needle);
                                        u32 pairPosition;

                                        switch (slice->strat) {
                                        case SLICE_LOOKUP_SCAN:
                                                pairPosition = SLICE_SCAN(slice, keyHash, needle);
                                                break;
                                        case SLICE_LOOKUP_BESEARCH:
                                                pairPosition = SLICE_BESEARCH(slice, keyHash, needle);
                                                break;
                                        default: error(&list->err, NG5_ERR_UNSUPFINDSTRAT)
                                                return false;
                                        }

                                        ng5_debug(NG5_SLICE_LIST_TAG,
                                                "NG5_slice_list_lookup_by_key keys(%s) -> pos(%zu in slice #%zu)",
                                                needle,
                                                pairPosition,
                                                i);
                                        if (pairPosition < slice->num_elems) {
                                                /** pair is contained */
                                                desc->numReadsHit++;
                                                handle->is_contained = true;
                                                handle->value = slice->string_id_tColumn[pairPosition];
                                                handle->key = needle;
                                                handle->container = slice;

                                                desc->numReadsHit++;
                                                return true;
                                        }
                                } else {
                                        /** bloom_t is sure that pair is not contained */
                                        continue;
                                }
                        } else {
                                /** keys hash is not inside bounds of hashes in slice */
                                continue;
                        }
                }
        }

        handle->is_contained = false;

        return false;
}

NG5_EXPORT(bool) SliceListRemove(slice_list_t *list, slice_handle_t *handle)
{
        ng5_unused(list);
        ng5_unused(handle);
        NG5_NOT_IMPLEMENTED
}

static void appenderNew(slice_list_t *list)
{
        /** ANTI-OPTIMIZATION: madvising sequential access to columns in slice decrease performance */

        /** the slice itself */
        Slice slice = {.strat     = SLICE_LOOKUP_SCAN, .num_elems = 0, .cacheIdx = (u32) -1};

        u32 numSlices = vec_length(&list->slices);
        vec_push(&list->slices, &slice, 1);

        assert(SLICE_KEY_COLUMN_MAX_ELEMS > 0);

        /** the descriptor */
        SliceDescriptor desc = {.numReadsHit  = 0, .numReadsAll  = 0,};

        vec_push(&list->descriptors, &desc, 1);

        /** the lookup guards */
        assert(sizeof(bloom_t) <= NG5_SLICE_LIST_BLOOMFILTER_TARGET_MEMORY_SIZE_IN_BYTE);
        bloom_t filter;

        /** NOTE: the size of each bloom_t lead to a false positive probability of 100%, i.e., number of items in the
         * slice is around 32644 depending on the CPU cache size, the number of actual bits in the filter (Cache line size
         * in bits minus the header for the bloom_t) along with the number of used hash functions (4), lead to that
         * probability. However, the reason a bloom_t is used is to skip slices whch definitively do NOT contain the
         * keys-values pair - and that still works ;) */
        bloom_create(&filter, (NG5_SLICE_LIST_BLOOMFILTER_TARGET_MEMORY_SIZE_IN_BYTE - sizeof(bloom_t)) * 8);
        vec_push(&list->filters, &filter, 1);
        HashBounds bounds = {.minHash        = (hash32_t) -1, .maxHash        = (hash32_t) 0};
        vec_push(&list->bounds, &bounds, 1);

        ng5_info(NG5_SLICE_LIST_TAG,
                "created new appender in slice list %p\n\t"
                        "# of slices (incl. appender) in total...............: %zu\n\t"
                        "Slice target memory size............................: %zuB (%s)\n\t"
                        "bloom_t target memory size......................: %zuB (%s)\n\t"
                        "Max # of (keys, hash, string) in appender/slice......: %zu\n\t"
                        "Bits used in per-slice bloom_t..................: %zu\n\t"
                        "Prob. of bloom_t to produce false-positives.....: %f\n\t"
                        "Single slice type size..............................: %zuB\n\t"
                        "Total slice-list size...............................: %f MiB",
                list,
                list->slices.num_elems,
                (size_t) NG5_SLICE_LIST_TARGET_MEMORY_SIZE_IN_BYTE,
                NG5_SLICE_LIST_TARGET_MEMORY_NAME,
                (size_t) NG5_SLICE_LIST_BLOOMFILTER_TARGET_MEMORY_SIZE_IN_BYTE,
                NG5_SLICE_LIST_BLOOMFILTER_TARGET_MEMORY_NAME,
                (size_t) SLICE_KEY_COLUMN_MAX_ELEMS,
                bitmap_nbits(&filter),
                (pow(1 - exp(-(double) bloom_nhashs()
                                / ((double) bitmap_nbits(&filter) / (double) SLICE_KEY_COLUMN_MAX_ELEMS)),
                        bitmap_nbits(&filter))),
                sizeof(Slice),
                (sizeof(slice_list_t) + list->slices.num_elems
                        * (sizeof(Slice) + sizeof(SliceDescriptor) + (sizeof(u32) * list->descriptors.num_elems)
                                + sizeof(bloom_t) + bitmap_nbits(&filter) / 8 + sizeof(HashBounds))) / 1024.0 / 1024.0);

        /** register new slice as the current appender */
        list->appender_idx = numSlices;
}

static void appenderSeal(Slice *slice)
{
        ng5_unused(slice);
        //  slice->cacheIdx = 0;
        //  slice_sort(slice);
        //  slice->strat = SLICE_LOOKUP_BESEARCH;

        // TODO: sealing means sort and then replace 'find' with bsearch or something. Not yet implemented: sealed slices are also search in a linear fashion
}

static void lock(slice_list_t *list)
{
        spin_acquire(&list->lock);
}

static void unlock(slice_list_t *list)
{
        spin_release(&list->lock);
}