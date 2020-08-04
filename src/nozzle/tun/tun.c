#include "log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <bits/ioctls.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <errno.h>
#include <netinet/in.h>

/**
 * Helper function for initialize the tun device. Sets route and IP.
 *
 * @param tun_name tun interface name.
 * @param ip_addr designated ip address.
 * @param set_route if true, will set as default route.
 * @return On success, return 0, return value less than zero indicates a failure.
 */

int tun_init(const char *tun_name, const char *ip_addr, bool set_route) {

//Credit: Mahdi Mohammadi
//https://stackoverflow.com/questions/6652384/how-to-set-the-ip-address-from-c-in-linux/49334944#49334944
    struct ifreq ifr;
    struct sockaddr_in sin = {.sin_family=AF_INET};

    // make ioctl() happy.
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0) {
        L_ERR("Failed to create socket for ioctl() calls.");
        return -1;
    }

    // Convert IP from numbers and dots to binary notation
    if(ip_addr) inet_aton(ip_addr, (struct in_addr *) &sin.sin_addr.s_addr);
    else inet_aton("172.23.33.33", (struct in_addr *) &sin.sin_addr.s_addr);

    // fill in interface name
    strncpy(ifr.ifr_name, tun_name, IFNAMSIZ);

    // Read interface flags
    if(ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
        L_PERROR();
        close(s);
        return -1;
    }

    // If interface is down, bring it up
    if(ifr.ifr_flags | ~(IFF_UP)) {
        ifr.ifr_flags |= IFF_UP;
        if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0) {
            L_PERROR();
            close(s);
            return -1;
        }
    }

    // Set IP
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

    if(ioctl(s, SIOCSIFADDR, &ifr) < 0) {
        L_PERROR();
        close(s);
        return -1;

    }
    struct sockaddr_in *addr = (struct sockaddr_in *) &ifr.ifr_netmask;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("255.255.255.0"); // set netmask
    if(ioctl(s, SIOCSIFNETMASK, &ifr) < 0) {
        L_PERROR();
        close(s);
        return -1;
    }

    // Set route
    if(set_route) {
        struct rtentry route;
        memset(&route, 0, sizeof(route));
        route.rt_dev = ifr.ifr_name;
        addr = (struct sockaddr_in *) &route.rt_gateway;
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = sin.sin_addr.s_addr;
        addr = (struct sockaddr_in *) &route.rt_dst;
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = INADDR_ANY;
        addr = (struct sockaddr_in *) &route.rt_genmask;
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = INADDR_ANY;
        route.rt_flags = RTF_UP | RTF_GATEWAY;
        route.rt_metric = 51;
        if(ioctl(s, SIOCADDRT, &route) < 0) {
            L_PERROR();
            close(s);
            return -1;
        }
    }
    close(s);
    return 0;
}

/**
 * Create a tun interface as drain.
 * @param tun_name name of tun interface.
 * @param ip_addr ip address of tun interface
 * @param set_route if true, will set as default route.
 * @return if success, return tun file descriptor; if failed, return -1.
 */
int slow_tun_create(const char *tun_name, const char *ip_addr, bool set_route) {
    // set tun interface
    int tun_fd;
    if((tun_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        L_PERROR();
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    if(tun_name) strncpy(ifr.ifr_name, tun_name, IFNAMSIZ);
    ifr.ifr_flags = (short) (IFF_TUN | IFF_TUN_EXCL | IFF_NO_PI | IFF_MULTI_QUEUE);
    if(ioctl(tun_fd, TUNSETIFF, (void *) &ifr) < 0) {
        L_PERROR();
        close(tun_fd);
        return -1;
    }

    L_INFOF("Created TUN interface drain: %s.", ifr.ifr_name);

    if(tun_init(ifr.ifr_name, ip_addr, set_route) < 0) {
        L_ERRF("Failed to set route and IP for TUN interface drain: %s.", ifr.ifr_name);
    } else {
        L_INFOF("Route and IP are set for TUN interface drain: %s.", ifr.ifr_name);
    }
    return tun_fd;
}

