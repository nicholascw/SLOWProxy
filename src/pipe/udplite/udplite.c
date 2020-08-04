#include "udplite.h"

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


#define SOL_UDPLITE           136
#define UDPLITE_SEND_CSCOV     10
#define UDPLITE_RECV_CSCOV     11

int route_init(char *ip, char *if_name) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct rtentry route;
    struct sockaddr_in *addr;
    memset(&route, 0, sizeof(route));
    route.rt_dev = if_name;
    //strncpy(route.rt_dev, if_name, IFNAMSIZ);
    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr = (struct sockaddr_in *) &route.rt_dst;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr = (struct sockaddr_in *) &route.rt_genmask;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_BROADCAST;
    route.rt_flags = RTF_UP | RTF_GATEWAY | RTF_HOST;
    route.rt_metric = 51;
    if(ioctl(s, SIOCADDRT, &route) < 0) {
        L_PERROR();
        close(s);
        return -1;
    }
    close(s);
    return 0;
}


int slow_udplite_pipe_init(char *addr, char *port, char *dst_addr, char *dst_port) {
    struct addrinfo *result, hints = {
            .ai_flags = AI_PASSIVE,
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDPLITE,
    };
    int r;
    if((r = getaddrinfo(addr, port, &hints, &result))) {
        L_WARN(gai_strerror(r));
    }

    int sockfd = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDPLITE);
    if(bind(sockfd, result->ai_addr, result->ai_addrlen)) {
        L_PERROR();
    }

    int checksum_len = 10;
    setsockopt(sockfd, SOL_UDPLITE, UDPLITE_SEND_CSCOV, &checksum_len, sizeof(int));
    setsockopt(sockfd, SOL_UDPLITE, UDPLITE_RECV_CSCOV, &checksum_len, sizeof(int));
    freeaddrinfo(result);
    if(dst_addr && dst_port) {
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDPLITE;
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
        if(route_init(dst_addr, "enp4s0") < 0) {
            L_WARN("Error setting routing table for pipe.");
        }
    }

    return sockfd;
}

int slow_udplite_pipe_reader(int fd, char *buf, int buf_size,
                             struct sockaddr *addr, unsigned int addrlen) {
    int read_in_size = addr ? recvfrom(fd, buf, buf_size, MSG_WAITALL, addr, &addrlen)
                            : read(fd, buf, buf_size);
    if(read_in_size <= 0) {
        L_PERROR();
        return -1;
    }
    return read_in_size;
}

int slow_udplite_pipe_writer(int fd, char *buf, int write_size, struct sockaddr *addr,
                             unsigned int addrlen) {
    int written_size = 0, r = 0;
    do {
        r = addr ? sendto(fd, buf + written_size, write_size - written_size, 0, addr, addrlen)
                 : write(fd, buf + written_size, write_size - written_size);
        if(r < 0) {
            L_PERROR();
            L_ERRF("fd=%d", fd);
        } else {
            written_size += r;
        }

    } while(written_size < write_size);
    return written_size;
}
