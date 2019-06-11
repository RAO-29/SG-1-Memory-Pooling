#include "core/carbon/archive_visitor.h"
#include "std/hash_set.h"
#include "std/hash_table.h"
#include "core/carbon/archive_query.h"
#include "ops-show-values.h"
//
struct capture
{
    const char *path;
    u32 offset;
    u32 limit;

    u32 current_off;
    u32 current_num;

    i32 between_lower_bound;
    i32 between_upper_bound;
    const char *contains_string;

    struct vector ofType(ops_show_values_result_t) *result;

  //  struct hashtable ofMapping(field_sid_t, u32) counts;
  //  struct hashset ofType(field_sid_t) keys;
};
////
static void
visit_string_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                              const field_sid_t *keys, const field_sid_t *values, u32 num_pairs,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(values);
    ng5_unused(num_pairs);
    ng5_unused(capture);

    struct capture *params = (struct capture *) capture;

    if (params->current_num > params->limit) {
        return;
    }

    struct archive_query *query = archive_query_default(archive);
    for (size_t i = 0; i < num_pairs; i++) {
        char *keystr = query_fetch_string_by_id(query, keys[i]);
        if (strstr(params->path, keystr) != 0) {
            if (params->current_off >= params->offset) {
                ops_show_values_result_t *r = NULL;
                for (u32 k = 0; k < params->result->num_elems; k++)
                {
                    r = vec_get(params->result, k, ops_show_values_result_t);
                    if (r->key == keys[i]) {
                        break;
                    }
                }
                if (!r) {
                    r = vec_new_and_get(params->result, ops_show_values_result_t);
                    r->key = keys[i];
                    r->type = FIELD_STRING;
                    vec_create(&r->values.string_values, NULL, sizeof(field_sid_t), 1000000);
                }

                if (!params->contains_string) {
                    vec_push(&r->values.string_values, &values[i], 1);
                    params->current_num += 1;
                } else {
                    struct archive_query *q = archive_query_default(archive);
                    char *value = query_fetch_string_by_id(q, values[i]);
                    if (strstr(value, params->contains_string)) {
                        vec_push(&r->values.string_values, &values[i], 1);
                        params->current_num += 1;
                    }
                    free(value);
                }

            } else {
                params->current_off++;
            }
        }

        free(keystr);
    }

}

static enum visit_policy
before_visit_object_array(struct archive *archive, path_stack_t path,
                                                     object_id_t parent_id, field_sid_t key,
                                                     void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(capture);
    ng5_unused(key);

    enum visit_policy follow = VISIT_EXCLUDE;

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);


    struct capture *params = (struct capture *) capture;

    size_t len_current_path = strlen(buffer);
    size_t len_user_path = strlen(params->path);


    if (len_user_path >= len_current_path && strncmp(buffer, params->path, len_current_path) == 0) {
        follow = VISIT_INCLUDE;
    }



    return follow;
}


static enum visit_policy
before_visit_object_array_object_property(struct archive *archive, path_stack_t path,
                                          object_id_t parent_id,
                                          field_sid_t key,
                                          field_sid_t nested_key,
                                          enum field_type nested_value_type,
                                          void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);
    ng5_unused(nested_value_type);
    ng5_unused(capture);

    enum visit_policy follow = VISIT_EXCLUDE;

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);


    struct capture *params = (struct capture *) capture;

    size_t len_current_path = strlen(buffer);
    size_t len_user_path = strlen(params->path);


    if (len_user_path >= len_current_path && strncmp(buffer, params->path, len_current_path) == 0) {
        follow = VISIT_INCLUDE;
    }


    return follow;
}


static void
visit_object_array_object_property_string(struct archive *archive, path_stack_t path,
                                               object_id_t parent_id,
                                               field_sid_t key,
                                               object_id_t nested_object_id,
                                               field_sid_t nested_key,
                                               const field_sid_t *nested_values,
                                               u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_object_id);
    ng5_unused(nested_key);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);
    ng5_unused(capture);




    struct capture *params = (struct capture *) capture;

    if (params->current_num > params->limit) {
        return;
    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);

    if (strcmp(buffer, params->path) == 0) {
        if (params->current_off >= params->offset) {
            ops_show_values_result_t *r = NULL;
            for (u32 k = 0; k < params->result->num_elems; k++)
            {
                r = vec_get(params->result, k, ops_show_values_result_t);
                if (r->key == nested_key) {
                    break;
                }
            }
            if (!r) {
                r = vec_new_and_get(params->result, ops_show_values_result_t);
                r->key = nested_key;
                r->type = FIELD_STRING;
                vec_create(&r->values.string_values, NULL, sizeof(field_sid_t), 1000000);
            }



            if (!params->contains_string) {
                vec_push(&r->values.string_values, nested_values, num_nested_values);
                params->current_num += num_nested_values;
            } else {
                struct archive_query *q = archive_query_default(archive);
                for (u32 k = 0; k < num_nested_values; k++) {
                    char *value = query_fetch_string_by_id(q, nested_values[k]);
                    if (strstr(value, params->contains_string)) {
                        vec_push(&r->values.string_values, &nested_values[k], 1);
                        params->current_num += 1;
                    }
                    free(value);
                }

            }


        } else {
            params->current_off++;
        }
    }
}



static void
visit_object_array_object_property_int8(struct archive *archive, path_stack_t path,
                                          object_id_t parent_id,
                                          field_sid_t key,
                                          object_id_t nested_object_id,
                                          field_sid_t nested_key,
                                          const field_i8_t *nested_values,
                                          u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_object_id);
    ng5_unused(nested_key);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);
    ng5_unused(capture);




    struct capture *params = (struct capture *) capture;

    if (params->current_num > params->limit) {
        return;
    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);

    if (strcmp(buffer, params->path) == 0) {
        if (params->current_off >= params->offset) {
            ops_show_values_result_t *r = NULL;
            for (u32 k = 0; k < params->result->num_elems; k++)
            {
                r = vec_get(params->result, k, ops_show_values_result_t);
                if (r->key == nested_key) {
                    break;
                }
            }
            if (!r) {
                r = vec_new_and_get(params->result, ops_show_values_result_t);
                r->key = nested_key;
                r->type = FIELD_INT8;
                vec_create(&r->values.integer_values, NULL, sizeof(field_i64_t), 1000000);
            }

            for (u32 k = 0; k < num_nested_values; k++) {
                i64 val = nested_values[k];
                if (val >= params->between_lower_bound && val <= params->between_upper_bound) {
                    vec_push(&r->values.integer_values, &val, 1);
                }
            }

            params->current_num += num_nested_values;
        } else {
            params->current_off++;
        }
    }
}


static void
visit_object_array_object_property_int16(struct archive *archive, path_stack_t path,
                                        object_id_t parent_id,
                                        field_sid_t key,
                                        object_id_t nested_object_id,
                                        field_sid_t nested_key,
                                        const field_i16_t *nested_values,
                                        u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_object_id);
    ng5_unused(nested_key);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);
    ng5_unused(capture);




    struct capture *params = (struct capture *) capture;

    if (params->current_num >= params->limit) {
        return;
    }

    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);

    if (strcmp(buffer, params->path) == 0) {
        if (params->current_off >= params->offset) {
            ops_show_values_result_t *r = NULL;
            for (u32 k = 0; k < params->result->num_elems; k++)
            {
                r = vec_get(params->result, k, ops_show_values_result_t);
                if (r->key == nested_key) {
                    break;
                }
            }
            if (!r) {
                r = vec_new_and_get(params->result, ops_show_values_result_t);
                r->key = nested_key;
                r->type = FIELD_INT16;
                vec_create(&r->values.integer_values, NULL, sizeof(field_i64_t), 1000000);
            }

            for (u32 k = 0; k < num_nested_values; k++) {
                i64 val = nested_values[k];
                if (val >= params->between_lower_bound && val <= params->between_upper_bound) {
                    vec_push(&r->values.integer_values, &val, 1);
                }

            }

            params->current_num += num_nested_values;
        } else {
            params->current_off++;
        }
    }
}


//
//static bool
//get_column_entry_count(struct archive *archive, path_stack_t path, field_sid_t key, enum field_type type, u32 count, void *capture)
//{
//    ng5_unused(archive);
//    ng5_unused(path);
//    ng5_unused(key);
//    ng5_unused(type);
//    ng5_unused(count);
//    struct capture *params = (struct capture *) capture;
//    char buffer[2048];
//    memset(buffer, 0, sizeof(buffer));
//    archive_visitor_path_to_string(buffer, archive, path);
//
//    if (strcmp(buffer, params->path) == 0) {
//        const u32 *count_ptr = hashtable_get_value(&params->counts, &key);
//        u32 count_val = 0;
//        if (!count_ptr) {
//            hashset_insert_or_update(&params->keys, &key, 1);
//        } else {
//            count_val = *count_ptr;
//        }
//        count_val += count;
//        hashtable_insert_or_update(&params->counts, &key, &count_val, 1);
//
//    }
//    return true;
//
//}

NG5_EXPORT(bool)
ops_show_values(timestamp_t *duration, struct vector ofType(ops_show_values_result_t) *result, const char *path,
                struct archive *archive, u32 offset, u32 limit, i32 between_lower_bound,
                i32 between_upper_bound, const char *contains_string)
{
    ng5_unused(result);
    ng5_unused(path);
    ng5_unused(archive);

    struct archive_visitor visitor = { 0 };
    struct archive_visitor_desc desc = { .visit_mask = NG5_ARCHIVE_ITER_MASK_ANY };

    vec_create(result, NULL, sizeof(ops_show_values_result_t), 10);

    struct capture capture = {
        .path = path,
        .limit = limit,
        .offset = offset,
        .current_num = 0,
        .current_off = 0,
        .result = result,
        .between_lower_bound = between_lower_bound,
        .between_upper_bound = between_upper_bound,
        .contains_string = contains_string
    };



//    hashtable_create(&capture.counts, &archive->err, sizeof(field_sid_t), sizeof(u32), 50);
//    hashset_create(&capture.keys, &archive->err, sizeof(field_sid_t), 50);

  //  visitor.visit_string_pairs = visit_string_pairs;
 //   visitor.visit_string_array_pair = visit_string_array_pair;
 //   visitor.visit_object_array_object_property_strings = visit_object_array_object_property_string;
   // visitor.get_column_entry_count = get_column_entry_count;
    visitor.visit_object_array_object_property_strings = visit_object_array_object_property_string;
    visitor.before_visit_object_array = before_visit_object_array;
    visitor.before_visit_object_array_object_property = before_visit_object_array_object_property;
    visitor.visit_object_array_object_property_int8s = visit_object_array_object_property_int8;
    visitor.visit_object_array_object_property_int16s = visit_object_array_object_property_int16;
    visitor.visit_string_pairs = visit_string_pairs;

    timestamp_t begin = time_now_wallclock();
    archive_visit_archive(archive, &desc, &visitor, &capture);
    timestamp_t end = time_now_wallclock();
    *duration = (end - begin);


//    struct vector ofType(field_sid_t) *keys = hashset_keys(&capture.keys);
//
//    for (u32 i = 0; i < keys->num_elems; i++) {
//        field_sid_t id = *vec_get(keys, i, field_sid_t);
//        u32 count = *(u32 *) hashtable_get_value(&capture.counts, &id);
//        ops_count_values_result_t r = {
//            .key = id,
//            .count = count
//        };
//        vec_push(result, &r, 1);
//    }
//    vec_drop(keys);
//
//
//    hashtable_drop(&capture.counts);
//    hashset_drop(&capture.keys);


    return true;
}