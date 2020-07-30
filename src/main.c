#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "tun.h"

/**
 * check if running with root.
 */
void root_check() {
#ifdef NDEBUG
    if(geteuid() == 0 || getegid() == 0)
        L_WARN("You are not recommended to use this program with root privilege."
               " Use setcap if you are able to.");
#endif
}

int main(int argc, char *argv[]) {
    root_check();
    int tun_fd = slow_tun_create("slow0", NULL);
    while(1) {
        char buf[100];
        int len = read(tun_fd, buf, 98);
        if(len < 0) {
            perror("read");
            break;
        }
        write(STDOUT_FILENO, buf, len);
    }
    return 1;
}


/**

 setsockopt(sd, IPPROTO_TCP, TCP_SYNCNT, 6);
 https://blog.cloudflare.com/when-tcp-sockets-refuse-to-die/

 */
