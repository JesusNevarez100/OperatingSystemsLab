#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
  char *p = sbrk(4096);
  for (int i = 0; i < 4096; i++)
    p[i] = 'A';

  int pid = fork();
  if (pid == 0) {
    p[0] = 'B';   // cause COW
    printf("child wrote, p[0]=%c\n", p[0]);
    exit(0);
  } else {
    wait(0);
    printf("parent sees p[0]=%c\n", p[0]);
    exit(0);
  }
}

