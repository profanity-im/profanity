#ifndef COMMAND_H
#define COMMAND_H

#define AWAIT_COMMAND 1
#define START_MAIN 2
#define QUIT_PROF 3

int handle_start_command(char *inp);
int handle_command(char *inp);

#endif
