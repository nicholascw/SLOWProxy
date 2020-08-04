
#ifndef SLOWPROXY_TUN_H
#define SLOWPROXY_TUN_H

int slow_tun_create(const char *tun_name, const char *ip_addr, bool set_route);
#endif //SLOWPROXY_TUN_H
