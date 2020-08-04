#include "worker.h"

/*
 * design:
 *
 *              >> sctp_pipe_out
 *              >> udplite_pipe_out
 *              >> tcp_pipe_out
 *              >> udp_pipe_out
 *              >> unix_pipe_out
 *
 *
 * tun_in       >> tun_out
 *              >> *_pipe_out
 *
 * proxy_v2_in  >> proxy_v2_out
 *              >> *_pipe_out
 *
 * socks5_in    >> socks5_out
 *              >> *_pipe_out
 *
 * ...
 *
 */

