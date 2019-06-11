#include "core/carbon/archive_int.h"
#include "core/carbon/archive_sid_cache.h"
#include "core/carbon.h"

#include "modules.h"

#include "cli.h"
#include "ops/ops-show-keys.h"
#include "ops/ops-count-values.h"
#include "ops/ops-show-values.h"
#include "ops/ops-get-citations.h"


static int testFileExists(FILE *file, const char *fileName, size_t fileNum, size_t fileMax, bool requireExistence)
{
    ng5_unused(fileNum);
    ng5_unused(fileMax);

    if (access( fileName, F_OK ) == 0) {
        if (requireExistence) {
            goto success;
        } else {
            goto fail;
        }
    } else {
        if (requireExistence) {
            goto fail;
        } else {
            goto success;
        }
    }

    fail:
    NG5_CONSOLE_WRITELN(file, "** ERROR ** file I/O error for file '%s'", fileName);
    return false;

    success:
    return true;

}

typedef enum {
    COMMAND_COUNT_ANY
} command_type_e;

struct capture
{
    command_type_e command_type;

    struct {

    } command_any;
};

static void
visit_root_object(struct archive *archive, object_id_t id, void *capture)
{
    ng5_unused(archive);
    ng5_unused(id);
    ng5_unused(capture);
}

static void
before_visit_starts(struct archive *archive, void *capture)
{
    ng5_unused(archive);
    ng5_unused(capture);
}

static void
after_visit_ends(struct archive *archive, void *capture)
{
    ng5_unused(archive);
    ng5_unused(capture);
}

static enum visit_policy
before_object_visit(struct archive *archive, path_stack_t path,
                                               object_id_t parent_id, object_id_t value_id,
                                               u32 object_idx, u32 num_objects, field_sid_t key,
                                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(parent_id);
    ng5_unused(value_id);
    ng5_unused(object_idx);
    ng5_unused(num_objects);
    ng5_unused(key);
    ng5_unused(capture);

    // TODO: ???   crashes??
    
    archive_visitor_print_path(stderr, archive, path);

    return VISIT_INCLUDE;
}

static void
after_object_visit(struct archive *archive, path_stack_t path, object_id_t id,
                           u32 object_idx, u32 num_objects, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(object_idx);
    ng5_unused(num_objects);
    ng5_unused(capture);

    
}

static void first_prop_type_group(struct archive *archive, path_stack_t path, object_id_t id, const field_sid_t *keys,
                              enum field_type type, bool is_array, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(type);
    ng5_unused(is_array);
    ng5_unused(num_pairs);
    ng5_unused(capture);
}

static void next_prop_type_group(struct archive *archive, path_stack_t path, object_id_t id, const field_sid_t *keys,
                             enum field_type type, bool is_array, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(type);
    ng5_unused(is_array);
    ng5_unused(num_pairs);
    ng5_unused(capture);
}

static void
visit_int8_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                              const field_sid_t *keys, const field_i8_t *values, u32 num_pairs,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_int16_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                  const field_sid_t *keys, const field_i16_t *values, u32 num_pairs,
                  void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_int32_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_i32_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    


}

static void
visit_int64_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_i64_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_uint8_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                  const field_sid_t *keys, const field_u8_t *values, u32 num_pairs,
                  void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_uint16_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_u16_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_uint32_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_u32_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_uint64_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_u64_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_number_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                   const field_sid_t *keys, const field_number_t *values, u32 num_pairs,
                   void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void
visit_string_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                    const field_sid_t *keys, const field_sid_t *values, u32 num_pairs,
                    void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);


    
}

static void
visit_boolean_pairs (struct archive *archive, path_stack_t path, object_id_t id,
                    const field_sid_t *keys, const FIELD_BOOLEANean_t *values, u32 num_pairs,
                    void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);
    ng5_unused(values);

    
}

static void visit_null_pairs (struct archive *archive, path_stack_t path, object_id_t id, const field_sid_t *keys,
                          u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);

    
}





static enum visit_policy
visit_enter_int8_array_pairs(struct archive *archive, path_stack_t path,
                             object_id_t id, const field_sid_t *keys,
                             u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(keys);
    ng5_unused(num_pairs);
    ng5_unused(capture);

    

    return VISIT_INCLUDE;
}

static void
visit_enter_int8_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                                        field_sid_t key, u32 entry_idx, u32 num_elems,
                                        void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);
    ng5_unused(capture);

     
}

static void
visit_int8_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                                   field_sid_t key, u32 entry_idx, u32 max_entries,
                                   const field_i8_t *array, u32 array_length, void *capture)
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

     
}

static void
ng5_func_unused visit_leave_int8_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                                        u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);
    ng5_unused(capture);

     
}

static void
ng5_func_unused visit_leave_int8_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                                         void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_int16_array_pairs(struct archive *archive, path_stack_t path,
                              object_id_t id, const field_sid_t *keys,
                              u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_int16_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             field_sid_t key, u32 entry_idx, u32 num_elems,
                             void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_int16_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                        field_sid_t key, u32 entry_idx, u32 max_entries,
                        const field_i16_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_int16_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_int16_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_int32_array_pairs(struct archive *archive, path_stack_t path,
                              object_id_t id, const field_sid_t *keys,
                              u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_int32_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             field_sid_t key, u32 entry_idx, u32 num_elems,
                             void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_int32_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                        field_sid_t key, u32 entry_idx, u32 max_entries,
                        const field_i32_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_int32_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_int32_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_int64_array_pairs(struct archive *archive, path_stack_t path,
                              object_id_t id, const field_sid_t *keys,
                              u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_int64_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             field_sid_t key, u32 entry_idx, u32 num_elems,
                             void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     

}

static void
ng5_func_unused visit_int64_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                        field_sid_t key, u32 entry_idx, u32 max_entries,
                        const field_i64_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_int64_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     

}

static void
ng5_func_unused visit_leave_int64_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_uint8_array_pairs(struct archive *archive, path_stack_t path,
                              object_id_t id, const field_sid_t *keys,
                              u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(num_pairs);
    ng5_unused(keys);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_uint8_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             field_sid_t key, u32 entry_idx, u32 num_elems,
                             void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     

}

static void
ng5_func_unused visit_uint8_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                        field_sid_t key, u32 entry_idx, u32 max_entries,
                        const field_u8_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);

    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_uint8_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                             u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_uint8_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_uint16_array_pairs(struct archive *archive, path_stack_t path,
                               object_id_t id, const field_sid_t *keys,
                               u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_uint16_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              field_sid_t key, u32 entry_idx, u32 num_elems,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_uint16_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                         field_sid_t key, u32 entry_idx, u32 max_entries,
                         const field_u16_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     

}

static void
ng5_func_unused visit_leave_uint16_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_uint16_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_uint32_array_pairs(struct archive *archive, path_stack_t path,
                               object_id_t id, const field_sid_t *keys,
                               u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_uint32_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              field_sid_t key, u32 entry_idx, u32 num_elems,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_uint32_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                         field_sid_t key, u32 entry_idx, u32 max_entries,
                         const field_u32_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_uint32_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_uint32_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_uint64_array_pairs(struct archive *archive, path_stack_t path,
                               object_id_t id, const field_sid_t *keys,
                               u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_uint64_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              field_sid_t key, u32 entry_idx, u32 num_elems,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_uint64_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                         field_sid_t key, u32 entry_idx, u32 max_entries,
                         const field_u64_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_uint64_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_uint64_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     
}



static enum visit_policy
ng5_func_unused visit_enter_number_array_pairs(struct archive *archive, path_stack_t path,
                               object_id_t id, const field_sid_t *keys,
                               u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_number_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              field_sid_t key, u32 entry_idx, u32 num_elems,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_number_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                         field_sid_t key, u32 entry_idx, u32 max_entries,
                         const field_number_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_number_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_number_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     

}



static enum visit_policy
ng5_func_unused visit_enter_string_array_pairs(struct archive *archive, path_stack_t path,
                               object_id_t id, const field_sid_t *keys,
                               u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_string_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              field_sid_t key, u32 entry_idx, u32 num_elems,
                              void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_string_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                         field_sid_t key, u32 entry_idx, u32 max_entries,
                         const field_sid_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(key);
    ng5_unused(entry_idx);

    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     
}

static void
ng5_func_unused visit_leave_string_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                              u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     
}

static void
ng5_func_unused visit_leave_string_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     

}



static enum visit_policy
ng5_func_unused visit_enter_boolean_array_pairs(struct archive *archive, path_stack_t path,
                                object_id_t id, const field_sid_t *keys,
                                u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_enter_boolean_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                               field_sid_t key, u32 entry_idx, u32 num_elems,
                               void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     

}

static void
ng5_func_unused visit_boolean_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                          field_sid_t key, u32 entry_idx, u32 max_entries,
                          const FIELD_BOOLEANean_t *array, u32 array_length, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);

    ng5_unused(max_entries);
    ng5_unused(array);
    ng5_unused(array_length);

     

}

static void
ng5_func_unused visit_leave_boolean_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                               u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     

}

static void
ng5_func_unused visit_leave_boolean_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                                void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     

}

static enum visit_policy
ng5_func_unused visit_enter_null_array_pairs(struct archive *archive, path_stack_t path,
                                                        object_id_t id,
                                                        const field_sid_t *keys, u32 num_pairs,
                                                        void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);
    ng5_unused(keys);
    ng5_unused(num_pairs);

     

    return VISIT_INCLUDE;

}

static void
ng5_func_unused visit_enter_null_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                                    field_sid_t key, u32 entry_idx, u32 num_elems, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);
    ng5_unused(num_elems);

     
}

static void
ng5_func_unused visit_null_array_pair (struct archive *archive, path_stack_t path, object_id_t id,
                               field_sid_t key, u32 entry_idx, u32 max_entries,
                               field_u32_t num_nulls, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(key);
    ng5_unused(entry_idx);

    ng5_unused(max_entries);
    ng5_unused(num_nulls);

     
}

static void
ng5_func_unused visit_leave_null_array_pair(struct archive *archive, path_stack_t path, object_id_t id,
                                    u32 pair_idx, u32 num_pairs, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

    ng5_unused(pair_idx);
    ng5_unused(num_pairs);

     


}

static void
ng5_func_unused visit_leave_null_array_pairs(struct archive *archive, path_stack_t path, object_id_t id,
                                     void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(id);
    ng5_unused(capture);

     

}

static enum visit_policy
ng5_func_unused before_visit_object_array(struct archive *archive, path_stack_t path,
                                                     object_id_t parent_id, field_sid_t key,
                                                     void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);

    archive_visitor_print_path(stderr, archive, path);
     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused before_visit_object_array_objects(bool *skip_group_object_ids,
                                          struct archive *archive, path_stack_t path,
                                          object_id_t parent_id,
                                          field_sid_t key,
                                          const object_id_t *group_object_ids,
                                          u32 num_group_object_ids, void *capture)
{
    ng5_unused(skip_group_object_ids);
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(group_object_ids);
    ng5_unused(num_group_object_ids);

     

}

static enum visit_policy
ng5_func_unused before_visit_object_array_object_property(struct archive *archive, path_stack_t path,
                                                                     object_id_t parent_id,
                                                                     field_sid_t key,
                                                                     field_sid_t nested_key,
                                                                     enum field_type nested_value_type,
                                                                     void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);
    ng5_unused(nested_value_type);

     

    return VISIT_INCLUDE;
}

static void
ng5_func_unused visit_object_array_object_property_int8(struct archive *archive, path_stack_t path,
                                               object_id_t parent_id,
                                               field_sid_t key,
                                               object_id_t nested_object_id,
                                               field_sid_t nested_key,
                                               const field_i8_t *nested_values,
                                               u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}

static void
ng5_func_unused visit_object_array_object_property_int16(struct archive *archive, path_stack_t path,
                                        object_id_t parent_id,
                                        field_sid_t key,
                                        object_id_t nested_object_id,
                                        field_sid_t nested_key,
                                        const field_i16_t *nested_values,
                                        u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}

static void
ng5_func_unused visit_object_array_object_property_int32(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_i32_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_int64(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_i64_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}



static void
ng5_func_unused visit_object_array_object_property_uint8(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_u8_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_uint16(struct archive *archive, path_stack_t path,
                                        object_id_t parent_id,
                                        field_sid_t key,
                                        object_id_t nested_object_id,
                                        field_sid_t nested_key,
                                        const field_u16_t *nested_values,
                                        u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_uint32(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_u32_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);
    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_uint64(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_u64_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_numbers(struct archive *archive, path_stack_t path,
                                         object_id_t parent_id,
                                         field_sid_t key,
                                         object_id_t nested_object_id,
                                         field_sid_t nested_key,
                                         const field_number_t *nested_values,
                                         u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_strings(struct archive *archive, path_stack_t path,
                                          object_id_t parent_id,
                                          field_sid_t key,
                                          object_id_t nested_object_id,
                                          field_sid_t nested_key,
                                          const field_sid_t *nested_values,
                                          u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}


static void
ng5_func_unused visit_object_array_object_property_booleans(struct archive *archive, path_stack_t path,
                                          object_id_t parent_id,
                                          field_sid_t key,
                                          object_id_t nested_object_id,
                                          field_sid_t nested_key,
                                          const FIELD_BOOLEANean_t *nested_values,
                                          u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}

static void
ng5_func_unused visit_object_array_object_property_nulls(struct archive *archive, path_stack_t path,
                                           object_id_t parent_id,
                                           field_sid_t key,
                                           object_id_t nested_object_id,
                                           field_sid_t nested_key,
                                           const field_u32_t *nested_values,
                                           u32 num_nested_values, void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_values);
    ng5_unused(num_nested_values);

     
}

static enum visit_policy
ng5_func_unused before_object_array_object_property_object(struct archive *archive, path_stack_t path,
                                                                      object_id_t parent_id,
                                                                      field_sid_t key,
                                                                      object_id_t nested_object_id,
                                                                      field_sid_t nested_key,
                                                                      u32 nested_value_object_id,
                                                                      void *capture)
{
    ng5_unused(archive);
    ng5_unused(path);
    ng5_unused(capture);
    ng5_unused(parent_id);
    ng5_unused(key);
    ng5_unused(nested_key);

    ng5_unused(nested_object_id);
    ng5_unused(nested_value_object_id);

     archive_visitor_print_path(stdout, archive, path);

    return VISIT_INCLUDE;
}

static void
ng5_func_unused run_magic_visitor(int mask, struct archive *archive)
{
    struct archive_visitor visitor = { 0 };
    struct archive_visitor_desc desc = { .visit_mask = mask };
    struct capture capture = {

    };

    visitor.visit_root_object   = visit_root_object;
    visitor.before_object_visit = before_object_visit;
    visitor.visit_int8_pairs    = visit_int8_pairs;
    visitor.visit_int16_pairs   = visit_int16_pairs;
    visitor.visit_int32_pairs   = visit_int32_pairs;
    visitor.visit_int64_pairs   = visit_int64_pairs;
    visitor.visit_uint8_pairs   = visit_uint8_pairs;
    visitor.visit_uint16_pairs  = visit_uint16_pairs;
    visitor.visit_uint32_pairs  = visit_uint32_pairs;
    visitor.visit_uint64_pairs  = visit_uint64_pairs;
    visitor.visit_number_pairs  = visit_number_pairs;
    visitor.visit_string_pairs  = visit_string_pairs;
    visitor.visit_boolean_pairs = visit_boolean_pairs;
    visitor.visit_null_pairs    = visit_null_pairs;

    visitor.visit_enter_int8_array_pairs = visit_enter_int8_array_pairs;
    visitor.visit_int8_array_pair = visit_int8_array_pair;
    visitor.visit_enter_int16_array_pairs = visit_enter_int16_array_pairs;
    visitor.visit_int16_array_pair = visit_int16_array_pair;
    visitor.visit_enter_int32_array_pairs = visit_enter_int32_array_pairs;
    visitor.visit_int32_array_pair = visit_int32_array_pair;
    visitor.visit_enter_int64_array_pairs = visit_enter_int64_array_pairs;
    visitor.visit_int64_array_pair = visit_int64_array_pair;
    visitor.visit_enter_uint8_array_pairs = visit_enter_uint8_array_pairs;
    visitor.visit_uint8_array_pair = visit_uint8_array_pair;
    visitor.visit_enter_uint16_array_pairs = visit_enter_uint16_array_pairs;
    visitor.visit_uint16_array_pair = visit_uint16_array_pair;
    visitor.visit_enter_uint32_array_pairs = visit_enter_uint32_array_pairs;
    visitor.visit_uint32_array_pair = visit_uint32_array_pair;
    visitor.visit_enter_uint64_array_pairs = visit_enter_uint64_array_pairs;
    visitor.visit_uint64_array_pair = visit_uint64_array_pair;
    visitor.visit_enter_boolean_array_pairs = visit_enter_boolean_array_pairs;
    visitor.visit_boolean_array_pair = visit_boolean_array_pair;
    visitor.visit_enter_number_array_pairs = visit_enter_number_array_pairs;
    visitor.visit_number_array_pair = visit_number_array_pair;
    visitor.visit_enter_null_array_pairs = visit_enter_null_array_pairs;
    visitor.visit_null_array_pair = visit_null_array_pair;
    visitor.visit_enter_string_array_pairs = visit_enter_string_array_pairs;
    visitor.visit_string_array_pair = visit_string_array_pair;

    visitor.before_visit_object_array_objects = before_visit_object_array_objects;

    visitor.visit_object_array_object_property_int8s = visit_object_array_object_property_int8;
    visitor.visit_object_array_object_property_int16s = visit_object_array_object_property_int16;
    visitor.visit_object_array_object_property_int32s = visit_object_array_object_property_int32;
    visitor.visit_object_array_object_property_int64s = visit_object_array_object_property_int64;
    visitor.visit_object_array_object_property_uint8s = visit_object_array_object_property_uint8;
    visitor.visit_object_array_object_property_uint16s = visit_object_array_object_property_uint16;
    visitor.visit_object_array_object_property_uint32s = visit_object_array_object_property_uint32;
    visitor.visit_object_array_object_property_uint64s = visit_object_array_object_property_uint64;
    visitor.visit_object_array_object_property_numbers = visit_object_array_object_property_numbers;
    visitor.visit_object_array_object_property_strings = visit_object_array_object_property_strings;
    visitor.visit_object_array_object_property_booleans = visit_object_array_object_property_booleans;
    visitor.visit_object_array_object_property_nulls = visit_object_array_object_property_nulls;

    visitor.before_visit_starts = before_visit_starts;
    visitor.after_visit_ends = after_visit_ends;
    visitor.after_object_visit = after_object_visit;
    visitor.first_prop_type_group = first_prop_type_group;
    visitor.next_prop_type_group = next_prop_type_group;
    visitor.visit_enter_int8_array_pair = visit_enter_int8_array_pair;
    visitor.visit_leave_int8_array_pair = visit_leave_int8_array_pair;
    visitor.visit_leave_int8_array_pairs = visit_leave_int8_array_pairs;
    visitor.visit_enter_int16_array_pair = visit_enter_int16_array_pair;
    visitor.visit_leave_int16_array_pair = visit_leave_int16_array_pair;
    visitor.visit_leave_int16_array_pairs = visit_leave_int16_array_pairs;
    visitor.visit_enter_int32_array_pair = visit_enter_int32_array_pair;
    visitor.visit_leave_int32_array_pair = visit_leave_int32_array_pair;
    visitor.visit_leave_int32_array_pairs = visit_leave_int32_array_pairs;
    visitor.visit_enter_int64_array_pair = visit_enter_int64_array_pair;
    visitor.visit_leave_int64_array_pair = visit_leave_int64_array_pair;
    visitor.visit_leave_int64_array_pairs = visit_leave_int64_array_pairs;
    visitor.visit_enter_uint8_array_pair = visit_enter_uint8_array_pair;
    visitor.visit_leave_uint8_array_pair = visit_leave_uint8_array_pair;
    visitor.visit_leave_uint8_array_pairs = visit_leave_uint8_array_pairs;
    visitor.visit_enter_uint16_array_pair = visit_enter_uint16_array_pair;
    visitor.visit_leave_uint16_array_pair = visit_leave_uint16_array_pair;
    visitor.visit_leave_uint16_array_pairs = visit_leave_uint16_array_pairs;
    visitor.visit_enter_uint32_array_pair = visit_enter_uint32_array_pair;
    visitor.visit_leave_uint32_array_pair = visit_leave_uint32_array_pair;
    visitor.visit_leave_uint32_array_pairs = visit_leave_uint32_array_pairs;
    visitor.visit_enter_uint64_array_pair = visit_enter_uint64_array_pair;
    visitor.visit_leave_uint64_array_pair = visit_leave_uint64_array_pair;
    visitor.visit_leave_uint64_array_pairs = visit_leave_uint64_array_pairs;
    visitor.visit_enter_number_array_pair = visit_enter_number_array_pair;
    visitor.visit_leave_number_array_pair = visit_leave_number_array_pair;
    visitor.visit_leave_number_array_pairs = visit_leave_number_array_pairs;
    visitor.visit_enter_string_array_pair = visit_enter_string_array_pair;
    visitor.visit_leave_string_array_pair = visit_leave_string_array_pair;
    visitor.visit_leave_string_array_pairs = visit_leave_string_array_pairs;
    visitor.visit_enter_boolean_array_pair = visit_enter_boolean_array_pair;
    visitor.visit_leave_boolean_array_pair = visit_leave_boolean_array_pair;
    visitor.visit_leave_boolean_array_pairs = visit_leave_boolean_array_pairs;
    visitor.visit_enter_null_array_pair = visit_enter_null_array_pair;
    visitor.visit_leave_null_array_pair = visit_leave_null_array_pair;
    visitor.visit_leave_null_array_pairs = visit_leave_null_array_pairs;
    visitor.before_visit_object_array = before_visit_object_array;
    visitor.before_visit_object_array_object_property = before_visit_object_array_object_property;
    visitor.before_object_array_object_property_object = before_object_array_object_property_object;

    archive_visit_archive(archive, &desc, &visitor, &capture);
}

static bool
run_show_keys( timestamp_t *duration, struct encoded_doc_list *result, const char *path, struct archive *archive)
{
    struct vector ofType(ops_show_keys_key_type_pair_t) prop_keys;
    vec_create(&prop_keys, NULL, sizeof(ops_show_keys_key_type_pair_t), 100);
    object_id_t result_oid;
    object_id_create(&result_oid);

    ops_show_keys(duration, &prop_keys, path, archive);

    encoded_doc_collection_create(result, &archive->err, archive);

    struct encoded_doc *result_doc = encoded_doc_collection_get_or_append(result, result_oid);
    encoded_doc_add_prop_array_object_decoded(result_doc, "result");

    for (u32 i = 0; i < prop_keys.num_elems; i++) {
        ops_show_keys_key_type_pair_t *pair = vec_get(&prop_keys, i, ops_show_keys_key_type_pair_t);
        object_id_t tmp_obj_id;
        object_id_create(&tmp_obj_id);
        encoded_doc_array_push_object_decoded(result_doc, "result", tmp_obj_id);
        struct encoded_doc *pair_doc = encoded_doc_collection_get_or_append(result, tmp_obj_id);
        encoded_doc_add_prop_string_decoded(pair_doc, "name", pair->key);
        encoded_doc_add_prop_string_decoded_string_value_decoded(pair_doc, "type", basic_type_to_json_type_str(pair->type));
        encoded_doc_add_prop_string_decoded_string_value_decoded(pair_doc, "sub-type", basic_type_to_system_type_str(pair->type));
    }


    vec_drop(&prop_keys);
    return true;
}

static bool
run_count_values( timestamp_t *duration, struct encoded_doc_list *result, const char *path, struct archive *archive)
{
    struct vector ofType(ops_count_values_result_t) prop_keys;
    vec_create(&prop_keys, NULL, sizeof(ops_count_values_result_t), 100);
    object_id_t result_oid;
    object_id_create(&result_oid);

    ops_count_values(duration, &prop_keys, path, archive);

    encoded_doc_collection_create(result, &archive->err, archive);

    struct encoded_doc *result_doc = encoded_doc_collection_get_or_append(result, result_oid);
    encoded_doc_add_prop_array_object_decoded(result_doc, "result");

    for (u32 i = 0; i < prop_keys.num_elems; i++) {
        ops_count_values_result_t *entry = vec_get(&prop_keys, i, ops_count_values_result_t);
        object_id_t tmp_obj_id;
        object_id_create(&tmp_obj_id);
        encoded_doc_array_push_object_decoded(result_doc, "result", tmp_obj_id);
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(result, tmp_obj_id);
        encoded_doc_add_prop_string_decoded(doc, "key", entry->key);
        encoded_doc_add_prop_uint32_decoded(doc, "count", entry->count);
    }


    vec_drop(&prop_keys);
    return true;
}

static bool
run_show_values( timestamp_t *duration, struct encoded_doc_list *result, const char *path, struct archive *archive, u32 offset, u32 limit, i32 between_lower_bound,
                 i32 between_upper_bound, const char *contains_string)
{
    struct vector ofType(ops_show_values_result_t) prop_keys;
    vec_create(&prop_keys, NULL, sizeof(ops_show_values_result_t), 100);
    object_id_t result_oid;
    object_id_create(&result_oid);

    ops_show_values(duration, &prop_keys, path, archive, offset, limit, between_lower_bound, between_upper_bound, contains_string);

    encoded_doc_collection_create(result, &archive->err, archive);

    struct encoded_doc *result_doc = encoded_doc_collection_get_or_append(result, result_oid);
    encoded_doc_add_prop_array_object_decoded(result_doc, "result");

    for (u32 i = 0; i < prop_keys.num_elems; i++) {
        ops_show_values_result_t *entry = vec_get(&prop_keys, i, ops_show_values_result_t);
        object_id_t tmp_obj_id;
        object_id_create(&tmp_obj_id);
        encoded_doc_array_push_object_decoded(result_doc, "result", tmp_obj_id);
        struct encoded_doc *doc = encoded_doc_collection_get_or_append(result, tmp_obj_id);
        encoded_doc_add_prop_string_decoded(doc, "key", entry->key);
        encoded_doc_add_prop_string_decoded_string_value_decoded(doc, "type", basic_type_to_json_type_str(entry->type));


        if (entry->type == FIELD_STRING) {
            encoded_doc_add_prop_array_string_decoded(doc, "values");
            encoded_doc_array_push_string_decoded(doc, "values",vec_all(&entry->values.string_values, field_sid_t), entry->values.string_values.num_elems);
            vec_drop(&entry->values.string_values);
        } else if (entry->type == FIELD_INT8) {
            encoded_doc_add_prop_array_int64_decoded(doc, "values");
            encoded_doc_array_push_int64_decoded(doc, "values",vec_all(&entry->values.integer_values, field_i64_t), entry->values.integer_values.num_elems);
            vec_drop(&entry->values.string_values);
        } else if (entry->type == FIELD_INT16) {
            encoded_doc_add_prop_array_int64_decoded(doc, "values");
            encoded_doc_array_push_int64_decoded(doc, "values",vec_all(&entry->values.integer_values, field_i64_t), entry->values.integer_values.num_elems);
            vec_drop(&entry->values.string_values);
        }
    }


    vec_drop(&prop_keys);
    return true;
}


static bool
run_get_citations( timestamp_t *duration, struct encoded_doc_list *result, const char *paper_name, struct archive *archive)
{
    struct vector ofType(ops_get_citations_result_t) prop_keys;
    vec_create(&prop_keys, NULL, sizeof(ops_get_citations_result_entry_t), 100);
    object_id_t result_oid;
    object_id_create(&result_oid);

    ops_get_citations(duration, &prop_keys, paper_name, archive);

    encoded_doc_collection_create(result, &archive->err, archive);

    struct encoded_doc *result_doc = encoded_doc_collection_get_or_append(result, result_oid);
    encoded_doc_add_prop_array_object_decoded(result_doc, "result");

    for (u32 i = 0; i < prop_keys.num_elems; i++) {
        ops_get_citations_result_t *result_entry = vec_get(&prop_keys, i, ops_get_citations_result_t);
        for (u32 x = 0; x < result_entry->papers.num_elems; x++) {
            ops_get_citations_result_entry_t *entry = vec_get(&result_entry->papers, x, ops_get_citations_result_entry_t);
            object_id_t tmp_obj_id;
            object_id_create(&tmp_obj_id);
            encoded_doc_array_push_object_decoded(result_doc, "result", tmp_obj_id);
            struct encoded_doc *doc = encoded_doc_collection_get_or_append(result, tmp_obj_id);
            encoded_doc_add_prop_string_decoded(doc, "title", entry->paper_title);
            encoded_doc_add_prop_string_decoded(doc, "id", entry->paper_id);
            encoded_doc_add_prop_array_string_decoded(doc, "authors");

            for (u32 k = 0; k < entry->authors.num_elems; k++) {
                field_sid_t author = *vec_get(&entry->authors, k, field_sid_t);
                encoded_doc_array_push_string_decoded(doc, "authors", &author, 1);
            }
            vec_drop(&entry->authors);
        }
        vec_drop(&result_entry->papers);
    }

    vec_drop(&prop_keys);
    return true;
}

static void
process_from(struct archive *archive, const char *line)
{
    timestamp_t duration = 0;
    char *select = strstr(line, "select");
    char *show_keys = strstr(line, "show keys");
    char *linecpy = strdup(line);
    char *cites = strstr(line, "use /references) use /title, /id/, /authors");
    if (cites) {
        char *name = strstr(line, "\"") + 1;
        *strstr(name, "\"") = '\0';

        struct encoded_doc_list result;
        timestamp_t duration;
        run_get_citations(&duration, &result, name, archive);
        encoded_doc_collection_print(stdout, &result);
        encoded_doc_collection_drop(&result);
        printf("\n");
        printf("execution time: %" PRIu64"ms\n", duration);

    } else if (select)
    {
        char *path = strdup(line + 1);
        char *contains_string = NULL;
        char *lower_str, *upper_str;
        i32 between_lower_bound = INT32_MIN;
        i32 between_upper_bound = INT32_MAX;

        if (strstr(path, "contains ")) {
            path[strstr(path, " ") - path] = '\0';
            contains_string = strdup(path + strlen(path) + 1 + strlen("contains ") + 1);
            contains_string[strlen(contains_string) - strlen("select") - 4] = '\0';
        } else  if (strstr(path, "between ") && strstr(path, " and ")) {
            path[strstr(path, " ") - path] = '\0';
            lower_str = strdup(path + strlen(path) + 1 + strlen("between "));
            upper_str = strstr(lower_str, " and ") + strlen(" and ");
            *strstr(lower_str, " and ") = '\0';
            *strstr(upper_str, " select") = '\0';
            between_lower_bound = atoi(lower_str);
            between_upper_bound = atoi(upper_str);
        } else {
            path[select - line - 2] = '\0';
        }

        char *command = strstr(line, "select") + strlen("select") + 1;




        if (strcmp(command, "count(*)") == 0) {
            struct encoded_doc_list result;
            timestamp_t duration;
            run_count_values(&duration, &result, path, archive);
            encoded_doc_collection_print(stdout, &result);
            encoded_doc_collection_drop(&result);
            printf("\n");
            printf("execution time: %" PRIu64"ms\n", duration);
        } else if (strstr(command, "*") != 0) {
            int offset_count = 0;
            int limit_count = INT32_MAX;
            if (strstr(command, "* offset ") != 0 || strstr(command, "* limit ") != 0) {
                if (strstr(command, "* offset ")) {
                    char *offset = strstr(command, "offset ") + strlen("offset ");
                    char *blank = NULL;
                    blank = strstr(offset, " ");
                    if (!blank) {
                        blank = offset + strlen(offset);
                    }
                    if (blank) {
                        offset[blank - offset] = '\0';
                        offset_count = atoi(offset);
                    }
                    else {
                        fprintf(stderr, "parsing error for <offset M>: expected <limit N> afterwards");
                        goto leave;
                    }
                }


                if (strstr(linecpy, "limit ") != 0) {
                    char *limit = strstr(linecpy, "limit ") + strlen("limit ");
                    limit_count = atoi(limit);
                }

            }

            struct encoded_doc_list result;

            run_show_values(&duration, &result, path, archive, (u32) offset_count, (u32) limit_count, between_lower_bound, between_upper_bound, contains_string);
            encoded_doc_collection_print(stdout, &result);
            encoded_doc_collection_drop(&result);
leave:
            printf("\n");
            printf("execution time: %" PRIu64"ms\n", duration);


        }



       //free(path);
        // free(command);

    } else if (show_keys)
    {
        char *path = strdup(line + 1);
        path[show_keys - line - 2] = '\0';

        struct encoded_doc_list result;
        timestamp_t duration;
        run_show_keys(&duration, &result, path, archive);
        encoded_doc_collection_print(stdout, &result);
        encoded_doc_collection_drop(&result);
        printf("\n");
        printf("execution time: %" PRIu64"ms\n", duration);

        free(path);
    } else {
        printf("unexpected token found\n");
    }


}

static bool
process_command(struct archive *archive)
{
    ng5_unused(archive);

    fprintf(stdout, "> ");

    char *line = NULL;
    size_t size;
    if (getline(&line, &size, stdin) == -1) {
        fprintf(stderr, "no line\n");
    } else {
        line[strlen(line) - 1] = '\0';

        if (strncmp(line, "from", strlen("from")) == 0) {
            process_from(archive, line + strlen("from"));
        } else if (strcmp(line, ".help") == 0) {
            printf("\nUse one of the following statements:\n"
                       "\tfrom /<path> show keys\t\t\t\t\t\t\t\tto show keys of object(s) behind <path>\n"
                       "\tfrom /<path>/<key> select count(*)\t\t\t\t\tto count values for objects in <path> having key <key>\n"
                       "\tfrom /<path>/<key> [between <a> and <b> | contains <substring>] select * [offset <m>] [limit <n>]\tto get values for objects in <path> having key <key>");
            printf("\n\n");
            printf("Type .examples for examples and .exit to leave this shell. Use .drop-cache to remove the string cache, .cache-size to get its size, and .create-cache <size>.");
            printf("\n\n");
        } else if (strcmp(line, ".examples") == 0) {
            printf("from / show keys\n"
                   "from /authors show keys\n"
                   "from /title select count(*)\n"
                   "from /authors/name select count(*)\n"
                   "from /n_citation select *\n"
                   "from /title select * offset 5 limit 10\n"
                   "from /authors/org select * limit 50\n"
                   "from /n_citation between 60 and 100 select *\n"
                   "from /title contains \"attack\" select *\n"
               //    "from /ids in (from /title equals \"<name>\" use /references) use /title, /id/, /authors\n"
               //        "XXXXXX\n\n"
                    );

        } else if (strcmp(line, ".exit") == 0) {
            fprintf(stdout, "%s", "bye");
            return false;
        } else if (strcmp(line, ".drop-cache") == 0) {
            struct string_cache *cache = archive_get_query_string_id_cache(archive);
            if (cache) {
                archive_drop_query_string_id_cache(archive);
                printf("cache dropped.\n");
            } else {
                fprintf(stderr, "no cache installed.\n");
            }

        } else if (strstr(line, ".create-cache ") != 0) {
            int new_cache_size = atoi(line + strlen(".create-cache "));
            struct string_cache *cache = archive_get_query_string_id_cache(archive);
            if (!cache) {
                string_id_cache_create_LRU_ex(&archive->string_id_cache, archive, new_cache_size);
                printf("cache created.\n");
            } else {
                fprintf(stderr, "cache already installed, drop it first.\n");
            }

        } else if (strcmp(line, ".cache-size") == 0) {
            struct string_cache *cache = archive_get_query_string_id_cache(archive);
            if (cache) {
                size_t cache_size;
                string_id_cache_get_size(&cache_size, cache);
                printf("%zu\n", cache_size);
            } else {
                printf("0\n");
            }
        } else {
            fprintf(stdout, "no such command: %s\n", line);
        }





    }
    return true;
}

static void*
cache_monitor(void * data)
{
    struct archive *archive = data;
    struct sid_cache_stats statistics;

    FILE *stats_file = fopen("types-temp-statistics--cache-monitor.csv", "w");


    if (stats_file) {
        fprintf(stats_file, "timestamp;type;value\n");

        float last_hit = 0;
        float last_miss = 0;
        float last_evict = 0;

        while (true) {
            struct string_cache *cache = archive_get_query_string_id_cache(archive);


            timestamp_t now = time_now_wallclock();

            fprintf(stats_file,
                    "%zu;\"hits\";%.4f\n",
                    (size_t) now,
                    last_hit);
            fprintf(stats_file,
                    "%zu;\"misses\";%.4f\n",
                    (size_t) now,
                    last_miss);
            fprintf(stats_file,
                    "%zu;\"evicted\";%.4f\n",
                    (size_t) now,
                    last_evict);
            fflush(stats_file);

            if (cache) {

                string_id_cache_get_statistics(&statistics, cache);


                size_t total = statistics.num_hits + statistics.num_misses;


                last_hit = total == 0 ? last_hit : statistics.num_hits / (float) total * 100;
                last_miss =  total == 0 ? last_miss : statistics.num_misses / (float) total * 100;
                last_evict = total == 0 ? last_evict: statistics.num_evicted / (float) total * 100;

                string_id_cache_reset_statistics(cache);


            } else {
                last_hit = 0;
                last_miss = 0;
                last_evict = 0;
            }

            sleep(1);


        }
    }

    return NULL;
}

bool moduleCliInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(manager);
    ng5_unused(argv);
    ng5_unused(argc);
    ng5_unused(file);

    if (argc != 1) {
        NG5_CONSOLE_WRITELN(file, "Run '%s' to see usage.", "$ types-tool cli");
        return false;
    } else {
        const int filePathArgIdx = 0;
        const char *pathCarbonFileIn = argv[filePathArgIdx];

        NG5_CONSOLE_OUTPUT_OFF()
        if (testFileExists(file, pathCarbonFileIn, 1, 1, true) != true) {
            NG5_CONSOLE_OUTPUT_ON()
            NG5_CONSOLE_WRITELN(file, "Input file cannot be found. %s", "STOP.");
            return false;
        }
        NG5_CONSOLE_OUTPUT_ON()

        FILE *f = fopen(pathCarbonFileIn, "r");
        fseek(f, 0, SEEK_END);
        size_t file_size = ftell(f);
        fclose(f);

        struct archive archive;
        int status;
        if ((status = archive_open(&archive, pathCarbonFileIn))) {

            struct archive_info info;
            archive_get_info(&info, &archive);
            printf("CARBON file successfully loaded: '%s' (%.2f GiB) \n%.2f MiB record data, %.2f MiB string table (%" PRIu32 " strings), %.2f MiB index data\n",
                    pathCarbonFileIn, file_size / 1024.0 / 1024.0 / 1024.0,
                    info.record_table_size / 1024.0 / 1024.0, info.string_table_size / 1024.0 / 1024.0, info.num_embeddded_strings,
                    info.string_id_index_size / 1024.0 / 1024.0);



            printf("Type '.help' for usage instructions.\n\n");

            pthread_t cache_strat_worker;
            pthread_create(&cache_strat_worker, NULL, cache_monitor, &archive);

            while (process_command(&archive))
            { };

            archive_close(&archive);
        } else {
            error_print(archive.err.code);
        }



        return true;
    }
}
