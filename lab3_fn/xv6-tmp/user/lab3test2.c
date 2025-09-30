#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int k, n, id;
    volatile long x = 0, z;

    if(argc < 2)
        n = 1;        // default value
    else
        n = atoi(argv[1]);  // from command line

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
            wait(0);  // <-- Parent waits for child to finish
        }
        else { // child
            printf("Child %d created\n", getpid());
            for(z = 0; z < 800000000; z++) x = x + 1; // burn CPU
            exit(0);
        }
    }
    exit(0);
}