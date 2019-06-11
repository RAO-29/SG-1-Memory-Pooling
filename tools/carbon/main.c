#include "shared/error.h"
#include "shell/opt.h"

#include "modules.h"
#include "cli.h"

#define DESC_CHECK_JS "Test if JSON files are suitable for CARBON."
#define DESC_CHECK_JS_USAGE "Test if input files given via <args> parameter are suitable for CARBON conversion.\n" \
                            "\nEXAMPLE\n\t$ carbon checkjs myjson1.json myjson2.json"

#define DESC_JS2CAB "Convert single JSON file into CARBON format"
#define DESC_JS2CAB_USAGE "The parameter <args> is split into two parts <output> and <input>.\n" \
                          "This command converts a single JSON file <input> into CARBON formatted\nfile <output>.\n\n" \
                          "Optionally, the following options are available that must be defined\nbefore <output>:\n\n" \
                          "   --size-optimized           Compress the embedded string dictionary using a\n" \
                          "                              particular compressor\n" \
                          "   --compressor <compressor>  Use <compressor> as compression technique for\n" \
                          "                              size optimization. Run `list compressors` in\n" \
                          "                              carbon-tool to see available compressors\n" \
                          "   --no-string-id-index       Turn-off pre-computation of string id to offset\n" \
                          "                              index\n" \
                          "   --read-optimized           Sort keys and values during pre-processing for\n" \
                          "                              efficient reads (experimental)\n" \
                          "   --force-overwrite          Overwrite the output file if this file already\n" \
                          "                              exists\n" \
                          "   --silent                   Suppress all outputs to stdout\n" \
                          "   --dic-type <type>          Use <type> as string dictionary implementation to\n" \
                          "                              be used. Types are 'sync' (single-threaded), and\n" \
                          "                              'async' (multi-threaded). Default type is 'async'.\n" \
                          "                              If 'async', see `--dic-nthreads` for options\n" \
                          "   --dic-nthreads <num>       Use number <num> of threads being spawn for\n" \
                          "                              string dictionary encoding. Ignored unless\n" \
                          "                              parameter `--dic-type` is set to `async`.\n" \
                          "                              By default, 8 threads are spawned\n" \
                          "\nEXAMPLE\n" \
                          "   $ carbon-tool convert out.carbon in.json\n" \
                          "   $ carbon-tool convert --size-optimized --read-optimized out.carbon in.json" \

#define DESC_CAB2JS_INFO  "Convert single CARBON file into JSON and print it to stdout"
#define DESC_CAB2JS_USAGE "The parameter <args> is a path to a CARBON file that is converted JSON and printed on stdout.\n" \
                          "\nEXAMPLE\n" \
                          "   $ carbon to_json inthewoods.carbon\n" \

#define DESC_CAB_VIEW "Print CARBON file in human readable form to stdout"
#define DESC_CAB_INFO "Display information about a CARBON file to stdout"

#define DESC_CLI "Runs the (experimental) command line interface"
#define DESC_CLI_INFO "Executes interactively commands on CARBON file <args>.\n" \
                      "\nEXAMPLE\n" \
                      "   $ carbon cli myfile.carbon\n" \

#define DESC_LIST       "List properties and configurations for carbon-tool to stdout"
#define DESC_LIST_USAGE "The parameter <args> is one of the following constants:\n\n"                                  \
                        "   compressors               Shows available compressors used by `convert` module"

#define DEFINE_MODULE(module_name, moduleCommand, desc, invokeFunc)                                              \
static int module##module_name##Entry(int argc, char **argv, FILE *file)                                         \
{                                                                                                               \
    struct cmdopt_mgr manager;                                                                        \
    opt_mgr_create(&manager, moduleCommand, desc, NG5_MOD_ARG_REQUIRED, invokeFunc);            \
    int status = opt_mgr_process(&manager, argc, argv, file);                                       \
    opt_mgr_drop(&manager);                                                                         \
    return status;                                                                                              \
}

DEFINE_MODULE(CheckJs, "checkjs", DESC_CHECK_JS_USAGE, moduleCheckJsInvoke);
DEFINE_MODULE(Js2Cab, "convert", DESC_JS2CAB_USAGE, moduleJs2CabInvoke);

DEFINE_MODULE(ViewCab, "view", DESC_CAB_VIEW, moduleViewCabInvoke);
DEFINE_MODULE(Cli, "cli", DESC_CLI_INFO, moduleCliInvoke);
DEFINE_MODULE(Inspect, "inspect", DESC_CAB_INFO, moduleInspectInvoke);
DEFINE_MODULE(Cab2Js, "to_json", DESC_CAB2JS_USAGE, moduleCab2JsInvoke);

DEFINE_MODULE(List, "list", DESC_LIST_USAGE, moduleListInvoke);


static bool showHelp(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager);

int main (int argc, char **argv)
{
    NG5_CONSOLE_OUTPUT_ON();

    struct cmdopt_mgr manager;
    struct cmdopt_group *group;

    opt_mgr_create(&manager, "types-tool", "A tool to work with CARBON files.\n"
                                 "Copyright (c) 2018-2019 Marcus Pinnecke (pinnecke@ovgu.de)", NG5_MOD_ARG_MAYBE_REQUIRED,
                             showHelp);

    opt_mgr_create_group(&group, "work with JSON files", &manager);
    opt_group_add_cmd(group,
                                "checkjs", DESC_CHECK_JS,
                                "manpages/types/checkjs",
                                moduleCheckJsEntry);
    opt_group_add_cmd(group,
                                "convert", DESC_JS2CAB,
                                "manpages/types/convert",
                                moduleJs2CabEntry);

    opt_mgr_create_group(&group, "work with CARBON files", &manager);
    opt_group_add_cmd(group,
                                "cli", DESC_CLI,
                                "manpages/types/cli",
                                moduleCliEntry);
    opt_group_add_cmd(group,
                                "view", DESC_CAB_VIEW,
                                "manpages/types/view",
                                moduleViewCabEntry);
    opt_group_add_cmd(group,
                                "inspect", DESC_CAB_INFO,
                                "manpages/types/inspect",
                                moduleInspectEntry);
    opt_group_add_cmd(group,
                                "to_json", DESC_CAB2JS_INFO,
                                "manpages/types/to_json",
                                moduleCab2JsEntry);

    opt_mgr_create_group(&group, "misc and orientation", &manager);
    opt_group_add_cmd(group,
                                "list", DESC_LIST,
                                "manpages/types/list",
                                moduleListEntry);

    int status = opt_mgr_process(&manager, argc - 1, argv + 1, stdout);
    opt_mgr_drop(&manager);
    
    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}

static bool showHelp(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager)
{
    ng5_unused(argc);
    ng5_unused(argv);
    opt_mgr_show_help(file, manager);
    return true;
}
