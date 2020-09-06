#ifndef SLOWPROXY_PIPE_COMMON_H
#define SLOWPROXY_PIPE_COMMON_H

#include <netinet/in.h>
int route_init(struct sockaddr_in *src_ip, char *dst_ip, char *if_name);

#endif //SLOWPROXY_PIPE_COMMON_H
