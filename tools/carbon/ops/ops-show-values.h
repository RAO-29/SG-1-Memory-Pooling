//
// Created by Marcus Pinnecke on 28.02.19.
//

#ifndef LIBNG5_OPS_SHOW_VALUES_H
#define LIBNG5_OPS_SHOW_VALUES_H

#include "shared/common.h"
#include "std/vec.h"
#include "core/carbon/archive.h"
#include "utils/time.h"

typedef struct
{
    field_sid_t key;
    enum field_type type;

    union {
        struct vector ofType(field_sid_t) string_values;
        u32 num_nulls;
        struct vector ofType(field_i64_t) integer_values;
    } values;

} ops_show_values_result_t;

NG5_EXPORT(bool)
ops_show_values(timestamp_t *duration, struct vector ofType(ops_show_values_result_t) *result, const char *path,
                struct archive *archive, u32 offset, u32 limit, i32 between_lower_bound,
                i32 between_upper_bound, const char *contains_string);

#endif //LIBNG5_OPS_SHOW_KEYS_H
