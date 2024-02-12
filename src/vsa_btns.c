#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vsalib.h"


char* get_nth_button_action(int n, char *line)
{
  FILE *file = fopen(PATH_BUTTON_ACTIONS_CFG, "r");
  if (!file)
  {
    fprintf(stderr, "Error opening file %s\n", PATH_BUTTON_ACTIONS_CFG);
    return NULL;
  }
  size_t len = 0;
  ssize_t read;
  int line_number = 0;
  
  while ((read = getline(&line, &len, file)) != -1)
  {
    line_number++;
    if (line_number == n)
    {
      fclose(file);
      size_t pos = strcspn(line, "\n");
      line[pos] = '\0';
      return line;
    }
  }
  fclose(file);
  return NULL;
}


void do_button_action(char * line)
{
  if (line != NULL)
  {
    char *type = strtok(line, ":");
    if (type != NULL)
    {
      if (strcmp(type, "non") != 0)
      {
        char *value = strtok(NULL, ":");
        if (value != NULL)
        {
          char cmd[1024];
          if (strcmp(type, "key") == 0)
          {
            sprintf(cmd, "xdotool key %s", value);
						printf("Run command: %s\n", cmd);
            run_command(cmd);
          }
          else if (strcmp(type, "cmd") == 0)
          {
            sprintf(cmd, "%s &", value);
						printf("Run command: %s\n", cmd);
            run_command(cmd);
          }
        }
      }
    }
  }
}


void key_event_handler(char * output)
{
  char line[1024];
  if (strstr(output, "KEY_PROG1), value 0") != NULL)
  {
    printf("Key PROG1 (right) is clicked\n");
    do_button_action(get_nth_button_action(RIGHT_BUTTON_ACTION_LINE, line));
  }
  else if (strstr(output, "KEY_PROG2), value 0") != NULL)
  {
    printf("Key PROG2 (center) is clicked\n");
    do_button_action(get_nth_button_action(CENTER_BUTTON_ACTION_LINE, line));
  }
  else if (strstr(output, "KEY_PROG3), value 0") != NULL)
  {
    printf("Key PROG3 (left) is clicked\n");
    do_button_action(get_nth_button_action(LEFT_BUTTON_ACTION_LINE, line));
  }
}


int main() {

	setbuf(stdout, NULL);

	printf("vsa_btns is running...\n");

  // get the touch buttons device
  run_command("cat /proc/bus/input/devices | grep -A 5 'goodix-ts' | grep Handlers | awk '{print $3}'");
  
  // monitor the touch buttons
  char cmd_buf[128];
  sprintf(cmd_buf, "sudo evtest /dev/input/%s", output_buffer);
  handle_command_output(cmd_buf, key_event_handler);

  return 0;
}
