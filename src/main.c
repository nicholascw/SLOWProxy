#include "main.h"
#include "tun.h"
#include "log.h"

int main(int argc, char *argv[]) {
    slow_tun_create();
}


/**

 setsockopt(sd, IPPROTO_TCP, TCP_SYNCNT, 6);
 https://blog.cloudflare.com/when-tcp-sockets-refuse-to-die/

 */