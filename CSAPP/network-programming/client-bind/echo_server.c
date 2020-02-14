
#include <sys/kernel_types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define LISTENQ 1024
#define MAXLINE 4096

typedef struct sockaddr SA;

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void gai_error(int code, char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
}

int open_listenfd(char* port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    for (p = listp; p; p = p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        if (close(listenfd) < 0)
        {
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;
    
    if (listen(listenfd, LISTENQ) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

int Open_listenfd(char* port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
        unix_error("Open_listenfd error");
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
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

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

#define RIO_BUFSIZE 8192

typedef struct {
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
} rio_t;

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

void Rio_readinitb(rio_t* rp, int fd)
{
    rio_readinitb(rp, fd);
}

static ssize_t rio_read(rio_t* rp, char* usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0)
    {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0)
        {
            if (errno != EINTR)
                return -1;
        }
        else if (rp->rio_cnt == 0)
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;
    }

    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readn(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        }
        else if (nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    return n - nleft;
}

ssize_t rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++)
    {
        if ((rc = rio_read(rp, &c, 1)) == 1)
        {
            *bufp++ = c;
            if (c == '\n')
            {
                n++;
                break;
            }
        }
        else if (rc == 0)
        {
            if (n == 1)
                return 0;
            else
                break;
        }
        else
            return -1;
    }
    *bufp = 0;
    return n - 1;
}

ssize_t Rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

ssize_t rio_writen(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            if (errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

void Rio_writen(int fd, void* usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

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

int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
        printf("(%s, %s) closed\n", client_hostname, client_port);
        fflush(stdout);
    }
    exit(0);
}