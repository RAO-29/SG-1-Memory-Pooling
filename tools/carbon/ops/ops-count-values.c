#include "core/carbon/archive_visitor.h"
#include "std/hash_set.h"
#include "std/hash_table.h"
#include "core/carbon/archive_query.h"
#include "ops-count-values.h"

struct capture
{
    const char *path;
    struct hashtable ofMapping(field_sid_t, u32) counts;
    struct hashset ofType(field_sid_t) keys;
};
//
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



        struct archive_query *query = archive_query_default(archive);
        for (u32 i = 0; i < num_pairs; i++) {

            char *keystr = query_fetch_string_by_id(query, keys[i]);
            if (strlen(params->path) > strlen(keystr) && strcmp(params->path + strlen(params->path) - strlen(keystr), keystr) == 0) {
//                char *valuestr = query_fetch_string_by_id(query, values[i]);
//                printf("visit_string_pairs -- KEY %s, VALUE %s\n", keystr, valuestr);
//                free(valuestr);

                const u32 *count_ptr = hashtable_get_value(&params->counts, &keys[i]);
                u32 count_val = 0;
                if (!count_ptr) {
                    hashset_insert_or_update(&params->keys, &keys[i], 1);
                } else {
                    count_val = *count_ptr;
                }
                count_val++;
                hashtable_insert_or_update(&params->counts, &keys[i], &count_val, 1);
            }

            free(keystr);

        }

//        const u32 *count_ptr = hashtable_get_value(&params->counts, &key);
//        u32 count_val = 0;
//        if (!count_ptr) {
//            hashset_insert_or_update(&params->keys, &key, 1);
//        } else {
//            count_val = *count_ptr;
//        }
//        count_val += count;
//        hashtable_insert_or_update(&params->counts, &key, &count_val, 1);



}
//
static void
visit_string_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                                   field_sid_t key, u32 entry_idx, u32 max_entries,
                                   const field_sid_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);
    ng5_unused(capture);

    archive_visitor_print_path(stdout, archive, path);
}
//
//static void
//visit_object_array_object_property_string(struct archive *archive, path_stack_t path,
//                                               object_id_t parent_id,
//                                               field_sid_t key,
//                                               object_id_t nested_object_id,
//                                               field_sid_t nested_key,
//                                               const field_sid_t *nested_values,
//                                               u32 num_nested_values, void *capture)
//{
//    ng5_unused(archive);
//    ng5_unused(path);
//    ng5_unused(parent_id);
//    ng5_unused(key);
//    ng5_unused(nested_object_id);
//    ng5_unused(nested_key);
//    ng5_unused(nested_values);
//    ng5_unused(num_nested_values);
//    ng5_unused(capture);
//
//
//
//  //
//
//}

static bool
get_column_entry_count(struct archive *archive, path_stack_t path, field_sid_t key, enum field_type type, u32 count, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(key);
    ng5_unused(type);
    ng5_unused(count);
    struct capture *params = (struct capture *) capture;
    char buffer[2048];
    memset(buffer, 0, sizeof(buffer));
    archive_visitor_path_to_string(buffer, archive, path);

    if (strcmp(buffer, params->path) == 0) {
        const u32 *count_ptr = hashtable_get_value(&params->counts, &key);
        u32 count_val = 0;
        if (!count_ptr) {
            hashset_insert_or_update(&params->keys, &key, 1);
        } else {
            count_val = *count_ptr;
        }
        count_val += count;
        hashtable_insert_or_update(&params->counts, &key, &count_val, 1);

    }
    return true;

}

NG5_EXPORT(bool)
ops_count_values(timestamp_t *duration, struct vector ofType(ops_count_values_result_t) *result, const char *path, struct archive *archive)
{
    ng5_unused(result);
    ng5_unused(path);
    ng5_unused(archive);

    struct archive_visitor visitor = { 0 };
    struct archive_visitor_desc desc = { .visit_mask = NG5_ARCHIVE_ITER_MASK_ANY };

    struct capture capture = {
        .path = path
    };
    hashtable_create(&capture.counts, &archive->err, sizeof(field_sid_t), sizeof(u32), 50);
    hashset_create(&capture.keys, &archive->err, sizeof(field_sid_t), 50);

    visitor.visit_string_pairs = visit_string_pairs;
    visitor.visit_string_array_pair = visit_string_array_pair;
 //   visitor.visit_object_array_object_property_strings = visit_object_array_object_property_string;
    visitor.get_column_entry_count = get_column_entry_count;

    timestamp_t begin = time_now_wallclock();
    archive_visit_archive(archive, &desc, &visitor, &capture);
    timestamp_t end = time_now_wallclock();
    *duration = (end - begin);

    struct vector ofType(field_sid_t) *keys = hashset_keys(&capture.keys);
//    vec_push(result, pairs->base, pairs->num_elems);
    for (u32 i = 0; i < keys->num_elems; i++) {
        field_sid_t id = *vec_get(keys, i, field_sid_t);
        u32 count = *(u32 *) hashtable_get_value(&capture.counts, &id);
        ops_count_values_result_t r = {
            .key = id,
            .count = count
        };
        vec_push(result, &r, 1);
    }
    vec_drop(keys);


    hashtable_drop(&capture.counts);
    hashset_drop(&capture.keys);

    return true;
}