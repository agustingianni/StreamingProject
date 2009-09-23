/*
 * socket.c
 *
 *  Created on: 11/07/2009
 *      Author: gr00vy
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "../include/socket.h"
#include "../include/server.h"

/* returns the last socket error */
int
socket_error(void)
{
	return errno;
}

/*
 * Determina si el error que arrojo el socket es recuperable o no.
 */
int
socket_recoverable(int error)
{
	switch (error)
	{
		case 0:
		case EAGAIN:
		case EINTR:
		case EINPROGRESS:
		#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
		#endif
		#ifdef ERESTART
		case ERESTART:
		#endif
			return 1;
		default:
			return 0;
	}
}

/*
 * Verificamos si el socket esta en estado 'stalled'
 */
int
socket_stalled(int error)
{
	switch (error)
	{
		case EAGAIN:
		case EINPROGRESS:
		case EALREADY:
		#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
		#endif
		#ifdef ERESTART
		case ERESTART:
		#endif
			return 1;
		default:
			return 0;
	}
}

/*
 * Verificamos si la conexion en el socket esta activa o no.
 */
int
socket_active(int sock)
{
	char c;
	int l;

	l = recv(sock, &c, 1, MSG_PEEK);
	if (l == 0)
		return 0;

	return 0;
}

/*
 * Seteamos un socket en modo no bloqueante
 */
int
socket_set_blocking(int sock)
{
	return fcntl(sock, F_SETFL, 0);
}

/*
 * Seteamos un socket en modo no bloqueante
 */
int
socket_set_nonblocking(int sock)
{
	return fcntl(sock, F_SETFL, O_NONBLOCK);
}

/*
 * Seteamos un socket en modo no linger es decir
 * cuando hacemos close() en el fd la conexion se cierra correctamente
 * enviando todos los datos que hayan quedado en buffers y aguarda
 * por la respuesta del peer.
 *
 * http://www.developerweb.net/forum/archive/index.php/t-2982.html
 */
int
socket_set_no_linger(int sock)
{
	struct linger lin =	{ 0, 0 };
	return setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *) &lin,
			sizeof(struct linger));
}

/*
 * Seteamos un socket en modo no delay, mandamos los datos de una.
 * Esto deshabilita el algoritmo de NAGLE. Es importante
 * para no generar delay en la transmision.
 *
 * http://articles.techrepublic.com.com/5100-10878_11-1050878.html
 */
int
socket_set_nodelay(int sock)
{
	int nodelay = 1;

	return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *) &nodelay,
			sizeof(int));
}

/*
 * Seteamos un socket en modo keep alive.
 * Nos ayuda a detectar HOSTS clientes que se cayeron y por lo tanto
 * el kernel no llego a enviarnos un FIN Flag para terminar la conexion.
 * Con esto lo que hacemos es tener cada 2 horitas un cleanup de los sockets.
 *
 * http://www.unixguide.net/network/socketfaq/4.7.shtml
 */
int
socket_set_keepalive(int sock)
{
	int keepalive = 1;
	return setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepalive,
			sizeof(int));
}

/*
 * Cerramos el socket
 */
int
socket_close(int sock)
{
	return close(sock);
}

/*
 * Escribimos 'len' bytes desde el buffer al socket.
 */
int
socket_write(int sock, const void *buff, size_t len)
{
	return send(sock, buff, len, 0);
}

int
socket_write_all(int sock, const void *buff, size_t len)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = buff;
	nleft = len;

	while (nleft > 0)
	{
		if ( (nwritten = socket_write(sock, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;   /* and call write() again */
			else
				return (-1);    /* error */
		}

		nleft -= nwritten;
		ptr += nwritten;
	}

	return len;
}


/*
 * Leemos 'len' bytes desde el socket en buffer.
 */
int
socket_read(int sock, char *buff, size_t len)
{
	if (!buff)
		return 0;

	return recv(sock, buff, len, 0);
}

/*
 * Leemos 'len' bytes desde el socket en una string (Null terminated)
 */
int
socket_read_string(int sock, char *string_buffer, size_t len)
{
	if (!string_buffer)
		return 0;

	int ret = recv(sock, string_buffer, len-1, 0);
	if(ret != -1)
		string_buffer[ret] = '\0';

	return ret;
}

/*
 * Devuelve un socket tcp servidor bindeado en el puerto
 * 'port'
 */
int
socket_tcp_server(int port)
{
	struct sockaddr_in sa;
	int opt;
	int sock;

	if (port < 0)
		return -1;

	memset(&sa, 0, sizeof(sa));

	sa.sin_addr.s_addr 	= INADDR_ANY;
	sa.sin_family 		= AF_INET;
	sa.sin_port 		= htons(port);

	if ((sock = socket(AF_INET, SOCK_STREAM,
			IPPROTO_TCP)) == -1)
		return -1;

	/* reuse it if we can */
	opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &opt, sizeof(int));

	/* bind socket to port */
	if(bind(sock, (struct sockaddr *) &sa,
			sizeof(struct sockaddr_in)) == -1)
		return -1;

	return sock;
}

/*
 * Devuelve un socket udp servidor bindeado en el puerto
 * 'port'
 */
int
socket_udp_server(int port)
{
	struct sockaddr_in sa;
	int opt;
	int sock;

	if (port < 0)
		return -1;

	memset(&sa, 0, sizeof(sa));

	sa.sin_addr.s_addr 	= INADDR_ANY;
	sa.sin_family 		= AF_INET;
	sa.sin_port 		= htons(port);

	if ((sock = socket(AF_INET, SOCK_DGRAM,
			IPPROTO_UDP)) == -1)
		return -1;

	/* reuse it if we can */
	opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &opt, sizeof(int));

	/* bind socket to port */
	if(bind(sock, (struct sockaddr *) &sa,
			sizeof(struct sockaddr_in)) == -1)
		return -1;

	return sock;
}


/*
 * Pone a la escucha un socket servidor.
 */
int
socket_listen(int serversock)
{
	return listen(serversock, SOMAXCONN);
}

/*
 * Acepta un cliente en un socket a la escucha.
 */
int
socket_accept(int serversock)
{
	int ret;
	struct sockaddr sa;
	socklen_t slen = sizeof(struct sockaddr);

	ret = accept(serversock, (struct sockaddr *) &sa, &slen);

	socket_set_no_linger(ret);
	socket_set_keepalive(ret);

	return ret;
}
