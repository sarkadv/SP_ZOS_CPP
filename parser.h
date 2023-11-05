#ifndef SP_ZOS_PARSER_H
#define SP_ZOS_PARSER_H

#include "directory.h"
#include "vfs.h"

int parse_input(char *input, char *command, char *param1, char *param2);
directory *parse_path(vfs *fs, char *input);

#endif
