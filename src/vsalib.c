#include <stdio.h>
#include <string.h>

#include "vsalib.h"


char output_buffer[OUTPUT_BUFFER_SIZE];


int run_command(const char * cmd)
{
  FILE *fp;
  if ((fp = popen(cmd, "r")) == NULL)
  {
      return -1;
  }
  int length = 0;
  if (fgets(output_buffer, OUTPUT_BUFFER_SIZE, fp) != NULL)
  {
      length = strlen(output_buffer);
  }
  if (pclose(fp))
  {
      return -2;
  }
  return length;
}


int handle_command_output(const char * cmd, OutputHandler output_handler)
{
  FILE *pipe = popen(cmd, "r");
  if (!pipe) {
    perror("popen failed");
    return 1;
  }

  while (fgets(output_buffer, sizeof(output_buffer), pipe) != NULL)
  {
    output_handler(output_buffer);
  }

  pclose(pipe);
  return 0;
}
