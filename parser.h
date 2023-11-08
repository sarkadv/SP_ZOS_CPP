#ifndef SP_ZOS_PARSER_H
#define SP_ZOS_PARSER_H

#include "directory.h"
#include "vfs.h"

#define INPUT_DELIMITER " "
#define PATH_DELIMITER "/"

int parse_input(char *input, char *command, char *param1, char *param2);
directory *parse_path(vfs *fs, char *input, bool without_last_part);
char *get_last_part_of_path(vfs *fs, char *input);

#endif
