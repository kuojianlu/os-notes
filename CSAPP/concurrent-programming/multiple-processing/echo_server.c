#include "csapp.h"

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

handler_t* mysignal(int signum, handler_t* handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("mysignal error");
    return (old_action.sa_handler);
}

void sigchld_handler(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0);
    return;
}

pid_t myfork()
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("myfork error");
    return pid;
}

int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    char client_hostname[BUFSIZ];
    char client_port[BUFSIZ];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    mysignal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        if (myfork() == 0)
        {
            Close(listenfd);
            Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("pid %d: connected to %s:%s\n", getpid(), client_hostname, client_port);
            echo(connfd);
            Close(connfd);
            exit(0);
        }
        Close(connfd);
        // NOTE: this demo do not show the expected behavior in OSX,
        // the connection between client and the forked server will be closed immediately
        // due to Close(connfd) in the parent process
    }
}