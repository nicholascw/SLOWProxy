#include "udp.h"
#include "pipe_common.h"
#include "log.h"

#include <sys/types.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <net/route.h>
#include <linux/if.h>
#include <sys/ioctl.h>



int slow_udp_pipe_init(char *addr, char *port, char *dst_addr, char *dst_port) {
    struct addrinfo *result, hints = {
            .ai_flags = AI_PASSIVE,
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP,
    };
    int r;
    if((r = getaddrinfo(addr, port, &hints, &result))) {
        L_WARN(gai_strerror(r));
    }

    int sockfd = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if(bind(sockfd, result->ai_addr, result->ai_addrlen)) {
        L_PERROR();
    }
    freeaddrinfo(result);
    if(dst_addr && dst_port) {
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        if((r = getaddrinfo(dst_addr, dst_port, &hints, &result))) {
            L_ERR(gai_strerror(r));
            close(sockfd);
            return -1;
        }
        if(connect(sockfd, result->ai_addr, result->ai_addrlen)) {
            L_PERROR();
            close(sockfd);
            return -1;
        }
        freeaddrinfo(result);
        struct sockaddr true_src;
        unsigned int size = sizeof(true_src);
        getsockname(sockfd, &true_src, &size);
        if(route_init((struct sockaddr_in *) &true_src, dst_addr, "enp4s0") < 0) {
            L_WARN("Error setting routing table for pipe.");
        }
    }

    return sockfd;
}

int slow_udp_pipe_reader(int fd, char *buf, int buf_size,
                             struct sockaddr *addr, unsigned int addrlen) {
    int read_in_size = addr ? recvfrom(fd, buf, buf_size, MSG_WAITALL, addr, &addrlen)
                            : read(fd, buf, buf_size);
    if(read_in_size <= 0) {
        L_PERROR();
        return -1;
    }
    return read_in_size;
}

int slow_udp_pipe_writer(int fd, char *buf, int write_size, struct sockaddr *addr,
                             unsigned int addrlen) {
    int written_size = 0, r = 0;
    do {
        r = addr ? sendto(fd, buf + written_size, write_size - written_size, 0, addr, addrlen)
                 : write(fd, buf + written_size, write_size - written_size);
        if(r < 0) {
            L_ERRF("fd=%d", fd);
            L_PERROR();
        } else {
            written_size += r;
        }

    } while(written_size < write_size);
    return written_size;
}
