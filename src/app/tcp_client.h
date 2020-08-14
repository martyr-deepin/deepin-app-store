#pragma once
#include <sys/types.h>
#include <stdint.h>

class tcp_session
{
public:
        tcp_session(const char *ip, uint16_t port);
        ~tcp_session();

        int connect();
        int close();
        ssize_t recv(char *buff, size_t buff_len, bool &is_timeout);
        ssize_t send(const char *buff, size_t buff_len);
        int get_sockfd() {return sock;}

private:
        ssize_t inline Recv(void *buffer, size_t length, bool &is_timeout);

private:
        int sock;
        char dst_ip[128];
        uint16_t dst_port;
};
