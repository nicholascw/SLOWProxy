/*
default dev vpn0 proto static scope link metric 50
10.0.0.1 dev enp4s0 proto static scope link metric 100
128.174.81.154 via 10.0.0.1 dev enp4s0 proto static metric 100

    int sfd, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[500];

int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
struct addrinfo a_info={
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP,
        .ai_flags = AI_PASSIVE,
        .ai_addr = NULL,
        .ai_next = NULL,
        .ai_canonname = NULL
};
struct addrinfo *result;
getaddrinfo("10.0.0.184", "1800", &a_info, &result);
// bind(sock_fd, result->ai_addr, result->ai_addrlen);
freeaddrinfo(result);
struct sockaddr_in dest={
        .sin_family=AF_INET,
        .sin_addr= {inet_addr("10.0.0.1")},
        .sin_port= htons(1801)
};
sendto(sock_fd, "hello", 6, 0, (const struct sockaddr *) &dest, sizeof(dest));
for (;;) {
peer_addr_len = sizeof(struct sockaddr_storage);
nread = recvfrom(sock_fd, buf, 500, 0,
                 (struct sockaddr *) &peer_addr, &peer_addr_len);
if (nread == -1)
continue;               // Ignore failed request

char host[NI_MAXHOST], service[NI_MAXSERV];

s = getnameinfo((struct sockaddr *) &peer_addr,
                peer_addr_len, host, NI_MAXHOST,
                service, NI_MAXSERV, NI_NUMERICSERV);
if (s == 0)
printf("Received %zd bytes from %s:%s\n",
nread, host, service);
else
fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

if (sendto(sock_fd, buf, nread, 0,
(struct sockaddr *) &peer_addr,
peer_addr_len) != nread)
fprintf(stderr, "Error sending response\n");
}
*/
