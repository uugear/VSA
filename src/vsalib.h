#ifndef VSALIB_H
#define VSALIB_H

#define OUTPUT_BUFFER_SIZE  1024

#define LEFT_BUTTON_ACTION_LINE     1
#define CENTER_BUTTON_ACTION_LINE   2
#define RIGHT_BUTTON_ACTION_LINE    3

#define PATH_KNOWN_KEYS_CFG  			"/etc/vsa/known_keys.cfg"
#define PATH_BUTTON_ACTIONS_CFG  	"/etc/vsa/button_actions.cfg"

typedef void (*OutputHandler)(char *);

extern char output_buffer[OUTPUT_BUFFER_SIZE];

int run_command(const char * cmd);

int handle_command_output(const char * cmd, OutputHandler output_handler);

#endif
