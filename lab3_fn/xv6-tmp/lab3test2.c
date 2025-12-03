#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int k, n, id;
    volatile long x = 0, z;

    if(argc < 2)
        n = 1;      // default value
    else
        n = atoi(argv[1]);

    if(n < 0 || n > 20)
        n = 2;

    id = 0;
    for(k = 0; k < n; k++){
        id = fork();
        if(id < 0){
            printf("%d failed in fork!\n", getpid());
        }
        else if(id > 0){ // parent
            printf("Parent %d creating child %d\n", getpid(), id);
        } else { // child
            printf("Child %d created\n", getpid());
            for(z = 0; z < 8000000000; z++)
                x = x + 1;
            printf("Child %d terminated\n", getpid());
            exit(0);   // child exits after work
        }
    }

    // parent waits for all children to finish
    for(k = 0; k < n; k++){
        wait(0);
    }

    printf("Parent %d: all children terminated\n", getpid());
    exit(0);
}
