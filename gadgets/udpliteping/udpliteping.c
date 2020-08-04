#include <sys/types.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <net/route.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <string.h>

#define SOL_UDPLITE           136
#define UDPLITE_SEND_CSCOV     10
#define UDPLITE_RECV_CSCOV     11

int main(int argc, char **argv) {
    if(argc != 4) {
        fprintf(stderr, "Usage: udpliteping server [LISTEN_ADDR] [LISTEN_PORT]\n"
                        "       udpliteping client [REMOTE_ADDR] [REMOTE_PORT]\n");
        return -1;
    }
    struct addrinfo *result, hints = {
            .ai_flags = AI_PASSIVE,
            .ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDPLITE,
    };
    int r;
    if((r = getaddrinfo(argv[2], argv[3], &hints, &result))) {
        fprintf(stderr, "getaddrinfo: %s", gai_strerror(r));
    }

    int sockfd = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDPLITE);
    int checksum_len = 10;
    setsockopt(sockfd, SOL_UDPLITE, UDPLITE_SEND_CSCOV, &checksum_len, sizeof(int));
    setsockopt(sockfd, SOL_UDPLITE, UDPLITE_RECV_CSCOV, &checksum_len, sizeof(int));
    char buf[1024];
    struct sockaddr_in *addr_ptr;
    struct sockaddr addr;
    addr_ptr = (struct sockaddr_in *) &addr;
    unsigned int addrlen = sizeof(addr);
    if(!strcmp("server", argv[1])) {
        if(bind(sockfd, result->ai_addr, result->ai_addrlen)) {
            perror("bind");
        }
        while(1) {
            int count = recvfrom(sockfd, buf, sizeof(buf), MSG_WAITALL, &addr, &addrlen);
            if(count < 0) {
                perror("recvfrom");
                continue;
            }
            fprintf(stderr, "received %d bytes: \"%s\" from %s:%d\n",
                    count, buf, inet_ntoa(addr_ptr->sin_addr), ntohs(addr_ptr->sin_port));
            count = sendto(sockfd, "pong!", 6, 0, &addr, addrlen);
            if(count < 0) {
                perror("sendto");
                continue;
            }
            fprintf(stderr, "sent %d bytes: \"pong!\" to %s:%d\n",
                    count, inet_ntoa(addr_ptr->sin_addr), ntohs(addr_ptr->sin_port));
        }
    } else {
        while(1) {
            int count = sendto(sockfd, "ping!", 6, 0, result->ai_addr, result->ai_addrlen);
            if(count < 0) {
                perror("sendto");
                continue;
            }
            struct sockaddr_in *result_in_ptr = (struct sockaddr_in *) result->ai_addr;
            fprintf(stderr, "sent %d bytes: \"ping\" to %s:%d\n",
                    count, inet_ntoa(result_in_ptr->sin_addr), ntohs(result_in_ptr->sin_port));
            count = recvfrom(sockfd, buf, sizeof(buf), MSG_WAITALL, &addr, &addrlen);
            if(count < 0) {
                perror("recvfrom");
                continue;
            }
            fprintf(stderr, "received %d bytes: \"%s\" from %s:%d\n",
                    count, buf, inet_ntoa(addr_ptr->sin_addr), ntohs(addr_ptr->sin_port));
            sleep(1);
        }
    }
    freeaddrinfo(result);
    return 0;

}
