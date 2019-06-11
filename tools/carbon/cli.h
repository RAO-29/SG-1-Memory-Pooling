//
// Created by Marcus Pinnecke on 25.02.19.
//

#ifndef LIBNG5_CLI_H
#define LIBNG5_CLI_H

#include "shell/opt.h"

bool moduleCliInvoke(int argc, char **argv, FILE *file, struct cmdopt_mgr *manager);

#endif //LIBNG5_CLI_H
