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

#ifndef NG5_H
#define NG5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "shared/common.h"
#include "core/alloc/alloc.h"
#include "std/bitmap.h"
#include "std/bloom.h"
#include "core/carbon/archive.h"
#include "core/carbon/archive_iter.h"
#include "core/carbon/archive_visitor.h"
#include "core/carbon/archive_converter.h"
#include "json/encoded_doc.h"
#include "shared/common.h"
#include "utils/convert.h"
#include "json/columndoc.h"
#include "json/doc.h"
#include "shared/error.h"
#include "std/hash_table.h"
#include "hash/hash.h"
#include "coding/coding_huffman.h"
#include "json/json.h"
#include "core/mem/block.h"
#include "core/mem/file.h"
#include "core/oid/oid.h"
#include "std/sort.h"
#include "async/parallel.h"
#include "stdx/slicelist.h"
#include "core/async/spin.h"
#include "stdx/strdic.h"
#include "stdx/strhash.h"
#include "core/carbon/archive_strid_iter.h"
#include "core/carbon/archive_sid_cache.h"
#include "utils/time.h"
#include "shared/types.h"
#include "core/carbon/archive_query.h"
#include "std/vec.h"
#include "core/alloc/trace.h"
#include "encode/encode_async.h"
#include "core/encode/encode_sync.h"
#include "core/strhash/strhash_mem.h"
#include "core/string-pred/string_pred_contains.h"
#include "core/string-pred/string_pred_equals.h"
#include "std/histogram.h"

NG5_EXPORT (bool) init(void);

#ifdef __cplusplus
}
#endif

#endif