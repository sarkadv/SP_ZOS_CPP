/*
 * Prikazy, ktere muze uzivatel zadavat pro praci s file systemem.
 */

#ifndef SP_ZOS_CPP_COMMANDS_H
#define SP_ZOS_CPP_COMMANDS_H

#include "vfs.h"

// nazvy prikazu
#define COMMAND_HELP "help"
#define COMMAND_END "end"
#define COMMAND_COPY "cp"
#define COMMAND_MOVE "mv"
#define COMMAND_REMOVE "rm"
#define COMMAND_MAKE_DIR "mkdir"
#define COMMAND_REMOVE_DIR "rmdir"
#define COMMAND_LIST "ls"
#define COMMAND_CONCATENATE "cat"
#define COMMAND_CHANGE_DIR "cd"
#define COMMAND_PRINT_WORK_DIR "pwd"
#define COMMAND_INFO "info"
#define COMMAND_IN_COPY "incp"
#define COMMAND_OUT_COPY "outcp"
#define COMMAND_LOAD "load"
#define COMMAND_FORMAT "format"
#define COMMAND_SYMB_LINK "slink"

int execute_command(char *command, char *param1, char *param2, vfs *fs);
int read_vfs_from_file(vfs *fs);
int command_end(vfs *fs);

#endif //SP_ZOS_CPP_COMMANDS_H
