#include "pipe_common.h"
#include "log.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/ioctl.h>


int route_init(struct sockaddr_in *src_ip, char *dst_ip, char *if_name) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct rtentry route;
    struct sockaddr_in *addr;
    memset(&route, 0, sizeof(route));
    route.rt_dev = if_name;
    addr = (struct sockaddr_in *) &route.rt_gateway;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("10.0.0.1");
    addr = (struct sockaddr_in *) &route.rt_dst;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(dst_ip);
    addr = (struct sockaddr_in *) &route.rt_genmask;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_BROADCAST;
    route.rt_flags = RTF_UP | RTF_GATEWAY | RTF_HOST ;
    route.rt_metric = 101;
    if(ioctl(s, SIOCADDRT, &route) < 0) {
        L_PERROR();
        close(s);
        return -1;
    }
    close(s);
    return 0;
}
