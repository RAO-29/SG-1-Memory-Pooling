
#include <inttypes.h>
#include "core/pack/pack.h"
#include "core/carbon/archive_query.h"
#include "core/carbon/archive_int.h"
#include "core/carbon.h"

#include "modules.h"

struct js_to_context
{
    struct strdic dictionary;
    struct doc_bulk context;
    struct doc_entries *partition;
    struct columndoc *partitionMetaModel;
    char *jsonContent;
};

static int convertJs2Model(struct js_to_context *context, FILE *file, bool optimizeForReads, const char *fileName,
                           size_t fileNum, size_t fileMax)
{

    NG5_CONSOLE_WRITELN(file, "** Process file %zu of %zu **", fileNum, fileMax);
    NG5_CONSOLE_WRITELN(file, "%s", fileName);

    NG5_CONSOLE_WRITE(file, "  - Read contents into memory%s", "");

    FILE *f = fopen(fileName, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    context->jsonContent = malloc(fsize + 1);
    size_t nread = fread(context->jsonContent, fsize, 1, f);
    ng5_unused(nread);
    fclose(f);
    context->jsonContent[fsize] = 0;

    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");


    NG5_CONSOLE_WRITE(file, "  - Setup string dictionary%s", "");

    encode_async_create(&context->dictionary, 1000, 1000, 1000, 8, NULL);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");

    NG5_CONSOLE_WRITE(file, "  - Parse JSON file%s", "");
    struct json_parser parser;
    struct json_err error_desc;
    struct json jsonAst;
    json_parser_create(&parser, &context->context);
    int status = json_parse(&jsonAst, &error_desc, &parser, context->jsonContent);
    if (!status) {
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        if (error_desc.token) {
            NG5_CONSOLE_WRITELN(file, "** ERROR ** Parsing failed: %s\nBut token %s was found in line %u column %u",
                                   error_desc.msg, error_desc.token_type_str, error_desc.token->line,
                                   error_desc.token->column);
        } else {
            NG5_CONSOLE_WRITELN(file, "** ERROR ** Parsing failed: %s", error_desc.msg);
        }
        return false;
    } else {
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
    }

    NG5_CONSOLE_WRITE(file, "  - Test document restrictions%s", "");
    struct err err;
    status = json_test(&err, &jsonAst);
    if (!status) {
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        error_print_to_stderr(&err);
        return false;
    } else {
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
    }

    NG5_CONSOLE_WRITE(file, "  - Create bulk insertion bulk%s", "");
    doc_bulk_create(&context->context, &context->dictionary);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");

    context->partition = doc_bulk_new_entries(&context->context);

    NG5_CONSOLE_WRITE(file, "  - Add file to new partition%s", "");
    doc_bulk_add_json(context->partition, &jsonAst);
    json_drop(&jsonAst);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");

    NG5_CONSOLE_WRITE(file, "  - Cleanup reserved memory%s", "");
    doc_bulk_shrink(&context->context);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");

    NG5_CONSOLE_WRITE(file, "  - Finalize partition%s", "");
    context->partitionMetaModel =
        doc_entries_columndoc(&context->context, context->partition, optimizeForReads);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");

    return true;
}

static void cleanup(FILE *file, struct js_to_context *context)
{
    NG5_CONSOLE_WRITE(file, "  - Perform cleanup operations%s", "");
    strdic_drop(&context->dictionary);
    doc_bulk_Drop(&context->context);
    doc_entries_drop(context->partition);
    columndoc_free(context->partitionMetaModel);
    free(context->jsonContent);
    free(context->partitionMetaModel);
    NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
}

static int testFileExists(FILE *file, const char *fileName, size_t fileNum, size_t fileMax, bool requireExistence)
{
    NG5_CONSOLE_WRITE(file, "Check file %zu of %zu", fileNum, fileMax);
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
    NG5_CONSOLE_WRITE_CONT(file, "[%s]", "ERROR");
    NG5_CONSOLE_WRITE_ENDL(file);
    NG5_CONSOLE_WRITELN(file, "** ERROR ** file I/O error for file '%s'", fileName);
    return false;

success:
    NG5_CONSOLE_WRITE_CONT(file, "[%s]", "OK");
    NG5_CONSOLE_WRITE_ENDL(file);
    return true;

}

bool moduleCheckJsInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(manager);

    struct js_to_context cabContext;

    for (int i = 0; i < argc; i++) {
        if (testFileExists(file, argv[i], i + 1, argc, true) != true) {
            return false;
        }
    }

    for (int i = 0; i < argc; i++) {
        if (convertJs2Model(&cabContext, file, false, argv[i], i + 1, argc) != true) {
            return false;
        }
        cleanup(file, &cabContext);
    }

    NG5_CONSOLE_WRITELN(file, "Input files passed tests%s", "");

    return true;
}

#define JS_2_CAB_OPTION_FORCE_OVERWRITE "--force-overwrite"
#define JS_2_CAB_OPTION_SILENT_OUTPUT "--silent"
#define JS_2_CAB_OPTION_SIZE_OPTIMIZED "--size-optimized"
#define JS_2_CAB_OPTION_READ_OPTIMIZED "--read-optimized"
#define JS_2_CAB_OPTION_DIC_TYPE "--dic-type"
#define JS_2_CAB_OPTION_DIC_NTHREADS "--dic-nthreads"
#define JS_2_CAB_OPTION_NO_STRING_ID_INDEX "--no-string-id-index"
#define JS_2_CAB_OPTION_USE_COMPRESSOR "--compressor"
#define JS_2_CAB_OPTION_USE_COMPRESSOR_HUFFMAN "huffman"

static void tracker_begin_create_from_model()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create from model started");
}

static void tracker_end_create_from_model()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create from model finished");
}

static void tracker_begin_create_from_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create from json started");
}

static void tracker_end_create_from_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create from json finished");
}

static void tracker_begin_archive_stream_from_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create stream from json started");
}

static void tracker_end_archive_stream_from_json()

{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Create stream from json finished");
}
static void tracker_begin_write_archive_file_to_disk()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Write archive to disk started");
}

static void tracker_end_write_archive_file_to_disk()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Write archive to disk finished");
}

static void tracker_begin_load_archive()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Load archive from disk started");
}

static void tracker_end_load_archive()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Load archive from disk finished");
}

static void tracker_begin_setup_string_dictionary()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Setup string dictionary started");
}

static void tracker_end_setup_string_dictionary()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Setup string dictionary finished");
}

static void tracker_begin_parse_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Parsing json started");
}

static void tracker_end_parse_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Parsing json finished");
}

static void tracker_begin_test_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Test document for restrictions started");
}

static void tracker_end_test_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Test document for restrictions finished");
}

static void tracker_begin_import_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Import document started");
}

static void tracker_end_import_json()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Import document finished");
}

static void tracker_begin_cleanup()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Cleanup started");
}

static void tracker_end_cleanup()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Cleanup finished");
}

static void tracker_begin_write_string_table()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Writing string table started");
}

static void tracker_end_write_string_table()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Writing string table finished");
}

static void tracker_begin_write_record_table()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Writing record table started");
}

static void tracker_end_write_record_table()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Writing record table finished");
}

static void tracker_skip_string_id_index_baking()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Backing indexes skipped");
}

static void tracker_begin_string_id_index_baking()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Backing string id to offset index started");
}

static void tracker_end_string_id_index_baking()
{
    NG5_CONSOLE_WRITELN(stdout, "%s", "  - Backing string id to offset index finished");
}


bool moduleJs2CabInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(manager);

    if (argc < 2) {
        NG5_CONSOLE_WRITE(file, "Require at least <output> and <input> parameters for <args>.%s", "");
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        NG5_CONSOLE_WRITELN(file, "Run '%s' to see an example on the usage.", "$ types convert");
        return false;
    } else {
        bool flagSizeOptimized = false;
        bool flagReadOptimized = false;
        bool flagForceOverwrite = false;
        bool flagBakeStringIdIndex = true;
        enum packer_type compressor = PACK_NONE;
        enum strdic_tag dic_type = ASYNC;
        int string_dic_async_nthreads = 8;

        int outputIdx = 0, inputIdx = 1;
        int i;

        for (i = 0; i < argc; i++) {
            char *opt = argv[i];
            if (strncmp(opt, "--", 2) == 0) {
                if (strcmp(opt, JS_2_CAB_OPTION_SIZE_OPTIMIZED) == 0) {
                    flagSizeOptimized = true;
                    compressor = PACK_HUFFMAN;
                } else if (strcmp(opt, JS_2_CAB_OPTION_READ_OPTIMIZED) == 0) {
                    flagReadOptimized = true;
                } else if (strcmp(opt, JS_2_CAB_OPTION_NO_STRING_ID_INDEX) == 0) {
                    flagBakeStringIdIndex = false;
                } else if (strcmp(opt, JS_2_CAB_OPTION_SILENT_OUTPUT) == 0) {
                    NG5_CONSOLE_OUTPUT_OFF();
                } else if (strcmp(opt, JS_2_CAB_OPTION_FORCE_OVERWRITE) == 0) {
                    flagForceOverwrite = true;
                } else if (strcmp(opt, JS_2_CAB_OPTION_USE_COMPRESSOR) == 0 && i++ < argc) {
                    const char *compressor_name = argv[i];
                    if (!pack_by_name(&compressor, compressor_name)) {
                        NG5_CONSOLE_WRITE(file, "unsupported pack requested: '%s'",
                                             compressor_name);
                        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
                        NG5_CONSOLE_WRITELN(file, "** ERROR ** unsupported operation requested: %s", opt);
                        return false;
                    }
                } else if (strcmp(opt, JS_2_CAB_OPTION_DIC_TYPE) == 0 && i++ < argc) {
                    const char *dic_type_name = argv[i];
                    if (strcmp(dic_type_name, "async") == 0) {
                        dic_type = ASYNC;
                    } else if (strcmp(dic_type_name, "sync") == 0) {
                        dic_type = SYNC;
                    } else {
                        NG5_CONSOLE_WRITE(file, "unsupported dictionary type requested: '%s'",
                                             dic_type_name);
                        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
                        NG5_CONSOLE_WRITELN(file, "** ERROR ** unsupported operation requested: %s", opt);
                        return false;
                    }
                } else if (strcmp(opt, JS_2_CAB_OPTION_DIC_NTHREADS) == 0 && i++ < argc) {
                    const char *nthreads_str = argv[i];
                    int nthreads_atoid = atoi(nthreads_str);
                    if (nthreads_atoid > 0) {
                        string_dic_async_nthreads = nthreads_atoid;
                    } else {
                        NG5_CONSOLE_WRITE(file, "not a number or zero threads assigned: '%s'",
                                             nthreads_str);
                        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
                        NG5_CONSOLE_WRITELN(file, "** ERROR ** thread setting cannot be applied: %s", opt);
                        return false;
                    }
                } else {
                    NG5_CONSOLE_WRITELN(file, "** ERROR ** unrecognized option '%s'", opt);
                    return false;
                }
            } else {
                break;
            }
        }

        if (!flagSizeOptimized && compressor != PACK_NONE) {
            NG5_CONSOLE_WRITELN(file, "** WARNING ** a pack was specified but will be ignored because size "
                "optimization is turned off. Use '--size-optimized' such that a pack has any effect%s", "");
        }

        if (i + 1 >= argc) {
            NG5_CONSOLE_WRITELN(file, "** ERROR ** require <output> and <input> parameter: %d remain", argc);
            return false;
        } else if (argc - (i + 1) != 1) {
            NG5_CONSOLE_WRITELN(file, "** ERROR ** unsupported number of arguments: %d provided beyond <input>", argc - (i + 1));
            return false;
        } else {
            outputIdx = i;
            inputIdx = i + 1;
        }

        const char *pathCarbonFileOut = argv[outputIdx];
        const char *pathJsonFileIn = argv[inputIdx];

        if (!flagForceOverwrite && (testFileExists(file, pathCarbonFileOut, 1, 1, false) != true)) {
            NG5_CONSOLE_WRITELN(file, "Output file already exists. Remove it first, or use --force-overwrite. %s", "STOP.");
            return false;
        }
        if (testFileExists(file, pathJsonFileIn, 1, 1, true) != true) {
            NG5_CONSOLE_WRITELN(file, "Input file cannot be found. %s", "STOP.");
            return false;
        }

        NG5_CONSOLE_WRITELN(file, "  - Read contents into memory%s", "");

        FILE *f = fopen(pathJsonFileIn, "rb");
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *jsonContent = malloc(fsize + 1);
        size_t nread = fread(jsonContent, fsize, 1, f);
        ng5_unused(nread);
        fclose(f);
        jsonContent[fsize] = 0;

        struct archive archive;
        struct err err;

        struct archive_callback progress_tracker = { 0 };
        progress_tracker.begin_create_from_model = tracker_begin_create_from_model;
        progress_tracker.end_create_from_model = tracker_end_create_from_model;
        progress_tracker.begin_create_from_json = tracker_begin_create_from_json;
        progress_tracker.end_create_from_json = tracker_end_create_from_json;
        progress_tracker.begin_archive_stream_from_json = tracker_begin_archive_stream_from_json;
        progress_tracker.end_archive_stream_from_json = tracker_end_archive_stream_from_json;
        progress_tracker.begin_write_archive_file_to_disk = tracker_begin_write_archive_file_to_disk;
        progress_tracker.end_write_archive_file_to_disk = tracker_end_write_archive_file_to_disk;
        progress_tracker.begin_load_archive = tracker_begin_load_archive;
        progress_tracker.end_load_archive = tracker_end_load_archive;
        progress_tracker.begin_setup_string_dictionary = tracker_begin_setup_string_dictionary;
        progress_tracker.end_setup_string_dictionary = tracker_end_setup_string_dictionary;
        progress_tracker.begin_parse_json = tracker_begin_parse_json;
        progress_tracker.end_parse_json = tracker_end_parse_json;
        progress_tracker.begin_test_json = tracker_begin_test_json;
        progress_tracker.end_test_json = tracker_end_test_json;
        progress_tracker.begin_import_json = tracker_begin_import_json;
        progress_tracker.end_import_json = tracker_end_import_json;
        progress_tracker.begin_cleanup = tracker_begin_cleanup;
        progress_tracker.end_cleanup = tracker_end_cleanup;
        progress_tracker.begin_write_string_table = tracker_begin_write_string_table;
        progress_tracker.end_write_string_table = tracker_end_write_string_table;
        progress_tracker.begin_write_record_table = tracker_begin_write_record_table;
        progress_tracker.end_write_record_table = tracker_end_write_record_table;
        progress_tracker.skip_string_id_index_baking = tracker_skip_string_id_index_baking;
        progress_tracker.begin_string_id_index_baking = tracker_begin_string_id_index_baking;
        progress_tracker.end_string_id_index_baking = tracker_end_string_id_index_baking;

        if (!archive_from_json(&archive, pathCarbonFileOut, &err, jsonContent,
                                      compressor, dic_type, string_dic_async_nthreads, flagReadOptimized,
                                      flagBakeStringIdIndex, &progress_tracker)) {
            error_print_and_abort(&err);
        } else {
            archive_close(&archive);
        }


        free(jsonContent);

//        struct memblock *carbonFile;
//        NG5_CONSOLE_WRITE(file, "  - Convert partition into in-memory CARBON file%s", "");
//        struct err err;
//        if (!archive_from_model(&carbonFile, &err, cabContext.partitionMetaModel, pack, flagBakeStringIdIndex)) {
//            error_print_and_abort(&err);
//        }
//        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
//
//        NG5_CONSOLE_WRITE(file, "  - Write in-memory CARBON file to disk%s", "");
//        FILE *outputFile = fopen(pathCarbonFileOut, "w");
//        if (!outputFile) {
//            NG5_CONSOLE_WRITE(file, "Unable to open file for writing: '%s'", pathCarbonFileOut);
//            NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
//            return false;
//        } else {
//            if (archive_write(outputFile, carbonFile) != true) {
//                NG5_CONSOLE_WRITE(file, "Unable to write to file: '%s'", pathCarbonFileOut);
//                NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
//                return false;
//            }
//            fclose(outputFile);
//        }
//        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
//
//        NG5_CONSOLE_WRITE(file, "  - Clean up in-memory CARBON file%s", "");
//        memblock_drop(carbonFile);
//        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "OK");
//
//        cleanup(file, &cabContext);

        NG5_CONSOLE_WRITELN(file, "Conversion successfull%s", "");

        return true;
    }
}

bool moduleViewCabInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(argc);
    ng5_unused(argv);
    ng5_unused(file);
    ng5_unused(manager);

    NG5_CONSOLE_OUTPUT_OFF()

    if (argc != 1) {
        NG5_CONSOLE_WRITE(file, "Require exactly one argument <types-file> that is a path to a types file.%s", "");
    } else {
        const char *carbonFilePath = argv[0];
        if (testFileExists(file, carbonFilePath, 1, 1, true) != true) {
            NG5_CONSOLE_OUTPUT_ON()
            NG5_CONSOLE_WRITELN(file, "Input file '%s' cannot be found. STOP", carbonFilePath);
            return false;
        }
        struct err err;
        FILE *inputFile = fopen(carbonFilePath, "r");
        struct memblock *stream;
        archive_load(&stream, inputFile);
        if (!archive_print(stdout, &err, stream)) {
            error_print_to_stderr(&err);
            error_drop(&err);
        }
        memblock_drop(stream);
        fclose(inputFile);

    }

    return true;
}

bool moduleInspectInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(manager);

    if (argc < 1) {
        NG5_CONSOLE_WRITE(file, "Require input file <input> as parameter for <args>.%s", "");
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        NG5_CONSOLE_WRITELN(file, "Run '%s' to see an example on the usage.", "$ types inspect");
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

        struct archive archive;
        struct archive_info info;
        if ((archive_open(&archive, pathCarbonFileIn)) != true) {
            NG5_CONSOLE_WRITE(file, "Cannot open requested CARBON file: %s", pathCarbonFileIn);
            return false;
        } else {
            archive_get_info(&info, &archive);
            FILE *f = fopen(pathCarbonFileIn, "r");
            fseek(f, 0, SEEK_END);
            offset_t fileLength = ftell(f);
            fclose(f);
            printf("file:\t\t\t'%s'\n", pathCarbonFileIn);
            printf("file-size:\t\t%" PRIu64 " B\n", fileLength);
            printf("string-table-size:\t%zu B\n", info.string_table_size);
            printf("record-table-size:\t%zu B\n", info.record_table_size);
            printf("index-size:\t\t%zu B\n", info.string_id_index_size);
            printf("#-embedded-strings:\t%" PRIu32 "\n", info.num_embeddded_strings);
        }
    }

    return true;
}

bool moduleCab2JsInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(argc);
    ng5_unused(argv);
    ng5_unused(file);
    ng5_unused(manager);

    if (argc != 1) {
        NG5_CONSOLE_WRITE(file, "Require exactly one <input> parameter for <args>.%s", "");
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        NG5_CONSOLE_WRITELN(file, "Run '%s' to see an example on the usage.", "$ types-tool to_json");
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

        struct archive archive;
        int status;
        if ((status = archive_open(&archive, pathCarbonFileIn))) {
            struct encoded_doc_list collection;
            archive_converter(&collection, &archive);
            encoded_doc_collection_print(stdout, &collection);
            printf("\n");
            encoded_doc_collection_drop(&collection);
        } else {
            error_print(archive.err.code);
        }

        archive_close(&archive);

        return true;
    }
}

bool moduleListInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(manager);

    if (argc != 1) {
        NG5_CONSOLE_WRITE(file, "Require one constant for <args> parameter.%s", "");
        NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
        NG5_CONSOLE_WRITELN(file, "Run '%s' to see an example on the usage.", "$ types list");
        return false;
    } else {
        const char *constant = argv[0];
        if (strcmp(constant, "compressors") == 0) {
            for (size_t i = 0; i < pack_get_num_registered_strategies(); i++) {
                NG5_CONSOLE_WRITELN(file, "%s", compressor_strategy_register[i].name);
            }
        } else {
            NG5_CONSOLE_WRITE_CONT(file, "[%s]\n", "ERROR");
            NG5_CONSOLE_WRITELN(file, "Constant '%s' is not known.", constant);
            return false;
        }

        return true;
    }
}
