#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void gai_error(int code, char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
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

int open_clientfd(char* client_port, char* hostname, char* port)
{
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
    struct addrinfo client_hints, *client_listp, *client_p;
    int flag;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        
        flag = 0;
        memset(&client_hints, 0, sizeof(struct addrinfo));
        client_hints.ai_socktype = SOCK_STREAM;
        client_hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
        client_hints.ai_flags |= AI_NUMERICSERV;
        if ((rc = getaddrinfo(NULL, client_port, &client_hints, &client_listp)) != 0)
        {
            fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
            return -2;
        }
        for (client_p = client_listp; client_p; client_p = client_p->ai_next)
        {
            if (client_p->ai_family == p->ai_family && client_p->ai_socktype == p->ai_socktype && client_p->ai_protocol == p->ai_protocol)
            {
                if ((bind(clientfd, client_p->ai_addr, client_p->ai_addrlen)) == 0)
                {
                    flag = 1;
                    break;
                }
            }
        }
        freeaddrinfo(client_listp);

        if (flag)
        {
            if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
                break;
        }
        if (close(clientfd) < 0)
        {
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;
    else
        return clientfd;
}

int Open_clientfd(char* client_port, char* hostname, char* port)
{
    int rc;

    if ((rc = open_clientfd(client_port, hostname, port)) < 0)
        unix_error("Open_clientfd error");
    return rc;
}

void app_error(char* msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

char* Fgets(char* ptr, int n, FILE* stream)
{
    char* rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
        app_error("Fgets error");
    return rptr;
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

void Fputs(const char* ptr, FILE* stream)
{
    if (fputs(ptr, stream) == EOF)
        unix_error("Fputs error");
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

int main(int argc, char** argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    char *client_port;
    rio_t rio;

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s <port> <host> <port>\n", argv[0]);
        exit(0);
    }

    client_port = argv[1];
    host = argv[2];
    port = argv[3];
    
    clientfd = Open_clientfd(client_port, host, port);
    Rio_readinitb(&rio, clientfd);
    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}