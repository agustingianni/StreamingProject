/*
 * client.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef CLIENT_H_
#define CLIENT_H_

typedef struct
{
	int 			connection_fd;		// socket con el cliente
	char *			name;				// alguna descripcion del cliente
	unsigned int 	id;					// identificador unico
	unsigned int 	offset;				// offset dentro del archivo de streaming

	#define CLIENT_WAITING 	0	// Esperando a ser servido
	#define CLIENT_ACTIVE 	1	// Quiere streammmmmm :P
	#define CLIENT_STARTING 2	// esta negociando headers

	int 			state;

	#ifdef BANDWIDTH_THROTTLING
	#include <sys/time.h>
	struct timeval last_write_ts;
	#endif
} client_t;

int 		client_create(client_t **client, int connection);
int 		client_destroy(client_t * client);
client_t *	client_get(int fd);
int			client_initialize(size_t max_clients);

#endif /* CLIENT_H_ */
