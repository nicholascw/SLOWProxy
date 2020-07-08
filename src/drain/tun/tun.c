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

int slow_tun_create() {
    int tun_fd = open("/dev/net/tun", O_RDWR);
    if(tun_fd < 0) {
        perror(__func__);
        return -1;
    }
    struct ifreq ifr;

    // Set up the ioctl request
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "slow0");
    ifr.ifr_flags = (short) (IFF_TUN | IFF_TUN_EXCL | IFF_NO_PI | IFF_MULTI_QUEUE);
    if(ioctl(tun_fd, TUNSETIFF, (void *) &ifr) < 0) {
        L_PERROR();
        return -1;
    }
    sleep(30);
    close(tun_fd);
    return 0;
}
