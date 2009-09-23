/*
 * socket.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#ifdef INET6_ADDRSTRLEN
#define MAX_ADDR_LEN INET6_ADDRSTRLEN
#else
#define MAX_ADDR_LEN 46
#endif

int socket_error(void);
int socket_recoverable(int error);
int socket_stalled (int error);
int socket_active (int sock);
int socket_set_blocking(int sock);
int socket_set_nonblocking(int sock);
int socket_set_no_linger(int sock);
int socket_set_nodelay(int sock);
int socket_set_keepalive(int sock);
int socket_close(int sock);
int socket_write(int sock, const void *buff, size_t len);
int socket_write_all(int sock, const void *buff, size_t len);
int socket_read(int sock, char *buff, size_t len);
int socket_read_string(int sock, char *string_buffer, size_t len);
int socket_connected (int sock, int timeout);
int socket_tcp_server(int port);
int socket_listen(int serversock);
int socket_accept(int serversock);

#endif /* SOCKET_H_ */
