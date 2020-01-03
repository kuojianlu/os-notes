#include "apue.h"

// gcc -o a.out fork.c

int main()
{
    // a.out
    // hellohello
    // a.out > out.txt
    // hellohello
    printf("hello");

    // a.out
    // hello
    // a.out > out.txt
    // hello
    // hello
    // printf("hello\n");


    // a.out
    // hello
    // a.out > out.txt
    // hello
    // write(STDOUT_FILENO, "hello", 6);

    // a.out
    // hello
    // a.out > out.txt
    // hello
    // write(STDOUT_FILENO, "hello\n", 7);

    if (fork() == 0)
    {
        // child
    }
    else
    {
        // parent
    }
    return 0;
}