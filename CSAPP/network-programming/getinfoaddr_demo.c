#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 8192

void gai_error(int code, char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
}

void Getnameinfo(const struct sockaddr* sa, socklen_t salen, char*host,
                 size_t hostlen, char* serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)) != 0)
    {
        gai_error(rc, "Getnameinfo error");
    }
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

int main(int argc, char** argv)
{
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    char buf1[MAXLINE];
    int rc, flags;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <domain name> <service name>\n", argv[0]);
        exit(0);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((rc = getaddrinfo(argv[1], argv[2], &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }

    flags = NI_NUMERICHOST;
    for (p = listp; p; p = p->ai_next)
    {
        Getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, buf1, MAXLINE, flags);
        printf("%s %s\n", buf, buf1);
    }

    Freeaddrinfo(listp);

    exit(0);
}