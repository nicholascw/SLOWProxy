//
// Created by nicholas on 2020/8/2.
//

#ifndef SLOWPROXY_UDP_LITE_H
#define SLOWPROXY_UDP_LITE_H

#include <sys/socket.h>

int slow_udplite_pipe_init(char *addr, char *port, char *dst_addr, char *dst_port);


int slow_udplite_pipe_reader(int fd, char *buf, int buf_size,
                             struct sockaddr *addr, unsigned int addrlen);

int slow_udplite_pipe_writer(int fd, char *buf, int write_size,
                             struct sockaddr *addr, unsigned int addrlen);


#endif //SLOWPROXY_UDP_LITE_H
