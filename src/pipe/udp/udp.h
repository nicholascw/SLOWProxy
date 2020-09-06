#ifndef SLOWPROXY_UDP_H
#define SLOWPROXY_UDP_H
#include <sys/socket.h>

int slow_udp_pipe_init(char *addr, char *port, char *dst_addr, char *dst_port);


int slow_udp_pipe_reader(int fd, char *buf, int buf_size,
                             struct sockaddr *addr, unsigned int addrlen);

int slow_udp_pipe_writer(int fd, char *buf, int write_size,
                             struct sockaddr *addr, unsigned int addrlen);


#endif //SLOWPROXY_UDP_H
