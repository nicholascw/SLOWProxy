#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <sys/epoll.h>


#include "log.h"
#include "tun.h"
#include "udplite.h"

int main(int argc, char *argv[]) {
#ifdef NDEBUG
    if(geteuid() == 0 || getegid() == 0)
        L_WARN("You are not recommended to use this program with root privilege."
               " Use setcap if you are able to.")
#endif
    char *buf = malloc(80000);
    int epfd = epoll_create(2);
    if(epfd < 0) {
        L_PERROR();
        return -1;
    }

    struct epoll_event epresult[10], epev = {
            .events = EPOLLIN
    };

    if(argc > 1 && argv[1][0] == 's') {
        // server
        int nfd = slow_tun_create(NULL, "172.23.33.32", false);
        int pfd = slow_udplite_pipe_init("0.0.0.0", "8192", NULL, NULL);
        epev.data.fd = nfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, nfd, &epev);
        epev.data.fd = pfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, pfd, &epev);
        struct sockaddr c_addr;
        slow_udplite_pipe_reader(pfd, buf, 5, &c_addr, sizeof(c_addr));
        buf[5] = 0;
        L_ERR(buf);
        while(1) {
            int count = epoll_wait(epfd, epresult, 10, -1);
            if(count > 0) {
                for(int i = 0; i < count; i++) {
                    if(epresult[i].data.fd == nfd) {
                        int c = read(nfd, buf, 80000);
                        int r = slow_udplite_pipe_writer(pfd, buf, c, &c_addr, sizeof(c_addr));
                        L_DEBUGF("Wrote %d bytes to udplite pipe.", r);
                    } else if(epresult[i].data.fd == pfd) {
                        int c = slow_udplite_pipe_reader(pfd, buf, 80000, &c_addr, sizeof(c_addr));
                        dbg(c_addr.sa_family);
                        struct sockaddr_in *dbgin = (struct sockaddr_in *) &c_addr;
                        L_DEBUGF("%s:%d", inet_ntoa(dbgin->sin_addr), ntohs(dbgin->sin_port));
                        int r = write(nfd, buf, c);
                        L_DEBUGF("Wrote %d bytes to tun nozzle.", r);
                    }
                }
            }
        }
    } else {
        //  client
        int pfd = slow_udplite_pipe_init("0.0.0.0", "0", "107.174.42.145", "8192");
        int nfd = slow_tun_create(NULL, NULL, true);
        epev.data.fd = nfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, nfd, &epev);
        epev.data.fd = pfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, pfd, &epev);
        struct sockaddr_in srv = {
                .sin_family = AF_INET,
                .sin_addr.s_addr = inet_addr("107.174.42.145"),
                .sin_port = htons(8192),
        };
        L_DEBUGF("nfd=%d, pfd=%d", nfd, pfd);

        L_ERRF("%d bytes wrote",
               slow_udplite_pipe_writer(pfd, "hello", 5, (struct sockaddr *) &srv, sizeof(srv))
        );
        while(1) {
            int count = epoll_wait(epfd, epresult, 10, -1);
            if(count > 0) {
                for(int i = 0; i < count; i++) {
                    if(epresult[i].data.fd == nfd) {
                        L_DEBUG("nfd");
                        int c = read(nfd, buf, 80000);
                        struct iphdr *hdr = (struct iphdr *) buf;
                        inet_ntop(AF_INET, &hdr->saddr, buf + 3000, INET_ADDRSTRLEN);
                        inet_ntop(AF_INET, &hdr->daddr, buf + 4000, INET_ADDRSTRLEN);
                        L_DEBUGF("src: %s; dst: %s", buf + 3000, buf + 4000);
                        int r = slow_udplite_pipe_writer(pfd, buf, c, (struct sockaddr *) &srv,
                                                         sizeof(srv));
                        L_DEBUGF("Wrote %d bytes to udplite pipe.", r);
                    } else if(epresult[i].data.fd == pfd) {
                        L_DEBUG("pfd");
                        int c = slow_udplite_pipe_reader(pfd, buf, 80000, (struct sockaddr *) &srv,
                                                         sizeof(srv));
                        int r = write(nfd, buf, c);
                        L_DEBUGF("Wrote %d bytes to tun nozzle.", r);
                    }
                }
            }
        }
    }
    return 0;
}


/**

 setsockopt(sd, IPPROTO_TCP, TCP_SYNCNT, 6);
 https://blog.cloudflare.com/when-tcp-sockets-refuse-to-die/

 */
