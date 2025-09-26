#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();

  printf("My pid: %d\n", pid);
  printf("Default priority: %d\n", get_priority(pid));

  printf("Setting priority to 10\n");
  if(set_priority(pid, 10) == 0)
    printf("New priority: %d\n", get_priority(pid));
  else
    printf("set_priority failed\n");

  printf("Setting priority to 60 (out of range)\n");
  if(set_priority(pid, 60) == 0)
    printf("Clamped priority: %d\n", get_priority(pid));
  else
    printf("set_priority failed\n");

  exit(0);
}