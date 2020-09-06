#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <zstd.h>

#include "log.h"
#include "tun.h"
#include "udp.h"


static size_t pkt_count_rx = 0, pkt_count_tx = 0;


void h_sigint(int s) {
    L_DEBUGF("rx=%zu, tx=%zu", pkt_count_rx, pkt_count_tx);
    exit(0);
}

void *server1(void *fd) {
    int *fds = ((int **) fd)[0];
    struct sockaddr *c_addr = ((void **) fd)[1];
    char *buf = malloc(8192);
    while(1) {
        int c = read(fds[0], buf, 4096);
        if(c <= 0)continue;
        pkt_count_rx++;
        size_t cc = ZSTD_compress(buf + 4096, 4096, buf, c, 3);
        int r = slow_udp_pipe_writer(fds[1], buf + 4096, cc, c_addr, sizeof(struct sockaddr));
        L_DEBUGF("tun => udp: %d bytes", r);
    }
    return NULL;
}

void *server2(void *fd) {
    int *fds = ((int **) fd)[0];
    char *buf = malloc(8192);
    struct sockaddr *c_addr = ((void **) fd)[1];
    while(1) {
        int c = slow_udp_pipe_reader(fds[1], buf, 4096, c_addr, sizeof(struct sockaddr));
        if(c <= 0)continue;
        size_t dc = ZSTD_decompress(buf + 4096, 4096, buf, c);
        struct sockaddr_in *dbgin = (struct sockaddr_in *) c_addr;
        L_DEBUGF("%s:%d", inet_ntoa(dbgin->sin_addr), ntohs(dbgin->sin_port));
        if(c < 8192) {
            int r = write(fds[0], buf + 4096, dc);
            pkt_count_tx++;
            L_DEBUGF("udp => tun: %d bytes", r);
        }
    }
    return NULL;
}

void *client1(void *fd) {
    int *fds = ((int **) fd)[0];
    struct sockaddr *srv = ((void **) fd)[1];
    char *buf = malloc(8192);

    while(1) {
        int c = read(fds[0], buf, 4096);
        pkt_count_rx++;
        if(c <= 0)continue;
        struct iphdr *hdr = (struct iphdr *) buf;
        inet_ntop(AF_INET, &hdr->saddr, buf + 8000, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &hdr->daddr, buf + 8032, INET_ADDRSTRLEN);
        L_DEBUGF("src: %s; dst: %s", buf + 8000, buf + 8032);
        size_t cc = ZSTD_compress(buf + 4096, 4096, buf, c, 3);
        int r = slow_udp_pipe_writer(fds[1], buf + 4096, cc, srv, sizeof(struct sockaddr));
        L_DEBUGF("tun => udp: %d bytes", r);
    }
}

void *client2(void *fd) {
    int *fds = ((int **) fd)[0];
    struct sockaddr *srv = ((void **) fd)[1];
    char *buf = malloc(8192);
    while(1) {
        int c = slow_udp_pipe_reader(fds[1], buf, 4096, srv, sizeof(struct sockaddr));
        if(c <= 0)continue;
        size_t dc = ZSTD_decompress(buf + 4096, 4096, buf, c);
        struct iphdr *hdr = (struct iphdr *) (buf + 4096);
        inet_ntop(AF_INET, &hdr->saddr, buf + 8000, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &hdr->daddr, buf + 8032, INET_ADDRSTRLEN);
        L_DEBUGF("src: %s; dst: %s", buf + 8000, buf + 8032);
        int r = write(fds[0], buf + 4096, dc);
        pkt_count_tx++;
        L_DEBUGF("udp => tun: %d bytes", r);
    }
}

int main(int argc, char *argv[]) {
#ifdef NDEBUG
    if(geteuid() == 0 || getegid() == 0)
        L_WARN("You are not recommended to use this program with root privilege."
               " Use setcap if you are able to.")
#endif
    signal(SIGINT, h_sigint);
    pthread_t pool[2];
    if(argc > 1 && argv[1][0] == 's') {
        struct sockaddr c_addr;
        // server
        int fds[2];
        void *args[2];
        args[0] = fds;
        args[1] = &c_addr;
        fds[0] = slow_tun_create(NULL, "172.23.33.32", false);
        fds[1] = slow_udp_pipe_init("0.0.0.0", "8192", NULL, NULL);
        pthread_create(pool, 0, server1, &args);
        pthread_create(pool + 1, 0, server2, &args);
        pthread_join(pool[0], NULL);
        pthread_join(pool[1], NULL);
    } else {
        //  client
        int fds[2];
        fds[1] = slow_udp_pipe_init("0.0.0.0", "0", "1.1.1.1", "8192");
        fds[0] = slow_tun_create(NULL, NULL, true);
        struct sockaddr_in srv = {
                .sin_family = AF_INET,
                .sin_addr.s_addr = inet_addr("1.1.1.1"),
                .sin_port = htons(8192),
        };
        void *args[2];
        args[0] = fds;
        args[1] = &srv;
        pthread_create(pool, 0, client1, &args);
        pthread_create(pool + 1, 0, client2, &args);
        pthread_join(pool[0], NULL);
        pthread_join(pool[1], NULL);
    }
    return 0;
}


/**

 setsockopt(sd, IPPROTO_TCP, TCP_SYNCNT, 6);
 https://blog.cloudflare.com/when-tcp-sockets-refuse-to-die/

 */
