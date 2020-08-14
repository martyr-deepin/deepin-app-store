#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <memory>
#include <random>
#include "tcp_client.h"

#define UPDATE_TIMEOUT 80
#define TCP_IDLE_TIME 60
#define TCP_INTVL_TIME 30
#define TCP_CONNECT_CNT 3

/* socket属性设置 */
static int set_sockopt(int sockfd, int flag)
{
    int tcp_nodely = 1;
    int keep_alive = 1; // 开启keepalive属性
    int keep_idle = TCP_IDLE_TIME; //TCP_IDLE_TIME秒内没有任何数据往来,则进行探测
    int keep_interval = TCP_INTVL_TIME; //TCP_INTVL_TIME 秒
    int keep_count = TCP_CONNECT_CNT; //TCP_CONNECT_CNT次的不再发
    struct timeval timeout;

    timeout.tv_sec = UPDATE_TIMEOUT;
    timeout.tv_usec = 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
                         &timeout, sizeof(timeout));
    if (ret < 0) {
        perror("setsockopt err");
        return -1;
    }

    timeout.tv_sec = UPDATE_TIMEOUT;
    timeout.tv_usec = 0;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                     &timeout, sizeof(timeout));
    if (ret < 0) {
        perror("setsockopt err");
        return -1;
    }

    if (flag)
        return 0;

    if (setsockopt(sockfd, SOL_TCP, TCP_NODELAY,
                   &tcp_nodely, sizeof(tcp_nodely)) < 0) {
        perror("set TCP_NODELAY err");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
                   &keep_alive, sizeof(keep_alive)) < 0) {
        perror("set SO_KEEPALIVE err");
        return -1;
    }

    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE,
                   &keep_idle, sizeof(keep_idle)) < 0) {
        perror("set TCP_KEEPIDLE err");
        return -1;
    }

    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL,
                   &keep_interval, sizeof(keep_interval)) < 0) {
        perror("set TCP_KEEPINTVL err");
        return -1;
    }

    if (setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT,
                   &keep_count, sizeof(keep_count)) < 0) {
        perror("set TCP_KEEPCNT err");
        return -1;
    }

    return 0;
}

static int get_addr_info(const char *ip, const char *port,
                         struct addrinfo **result, int ai_socktype)
{
    struct addrinfo hints;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = ai_socktype; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;

    s = getaddrinfo(ip, port, &hints, result);
    if (s != 0)
        return -1;

    return 0;
}

int connect_server(const char *ip, uint16_t port, int ai_socktype)
{
    struct addrinfo *result, *rp;
    char dst_port[12] = {0};

    sprintf(dst_port, "%hu", port);
    int sfd = get_addr_info(ip, dst_port, &result, ai_socktype);
    int flag = ai_socktype==SOCK_STREAM ? 0 : 1;

    if (sfd < 0)
        return -1;

    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
            rp->ai_protocol);
        if (sfd == -1)
            continue;

        set_sockopt(sfd, flag);
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    freeaddrinfo(result);           /* No longer needed */
    if (rp == nullptr) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        return -1;
    }

    return sfd;
}

tcp_session::tcp_session(const char *ip, uint16_t port) :
    sock(-1),
    dst_port(port)
{
    memset(dst_ip, 0, sizeof dst_ip);
    strcpy(dst_ip, ip);
}

tcp_session::~tcp_session()
{
    close();
}

int tcp_session::connect()
{
    if (sock > 0)
        return 0;

    sock = connect_server(dst_ip, dst_port, SOCK_STREAM);
    if (sock < 0)
        return -1;
    return 0;
}

int tcp_session::close()
{
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        ::close(sock);
        sock = -1;
    }
    return 0;
}

ssize_t tcp_session::recv(char *buff, size_t buff_len, bool &is_timeout)
{
    is_timeout = false;
    ssize_t len = ::recv(sock, buff, buff_len, 0);
    if (len > 0)
        return len;

    int err = errno;
    if (err == EINTR)
        return this->recv(buff, buff_len, is_timeout);
    else {
        if (err == EAGAIN)
            is_timeout = true;
        return -1;
    }
}


ssize_t tcp_session::send(const char *buff, size_t buff_len)
{
    if (sock > 0)
        return ::send(sock, buff, buff_len, MSG_NOSIGNAL);
    return -1;
}


inline ssize_t tcp_session::Recv(void *buffer, size_t length, bool &is_timeout)
{
    is_timeout = false;
    ssize_t len = ::recv(sock, buffer, length, MSG_WAITALL);
    if (len > 0)
        return len;
    int err = errno;
    if (err == EINTR)
        return Recv(buffer, length, is_timeout);
    else {
        if (err == EAGAIN)
            is_timeout = true;
        return -1;
    }
}
