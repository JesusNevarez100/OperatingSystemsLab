// cowread.c
#include "kernel/types.h"
#include "user/user.h"

int main() {
  char *p = sbrk(4096);   // get a page
  for (int i = 0; i < 4096; i++)
    p[i] = 'A';

  int pid = fork();
  if (pid == 0) {
    // child: read but don't write
    int sum = 0;
    for (int i = 0; i < 4096; i++)
      sum += p[i];
    printf("child sum=%d\n", sum);
    exit(0);
  } else {
    wait(0);
    exit(0);
  }
}

