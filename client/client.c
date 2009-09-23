/*
 * client.c
 *
 *  Created on: 11/07/2009
 *      Author: gr00vy
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>

#include "../include/client.h"

/* Tabla de clientes indexada por FD */
static client_t **clients;
static size_t client_limit = 0;
static size_t current_clients = 0;

/*
 * Inicializamos el subsistema de clientes.
 */
int
client_initialize(size_t max_clients)
{
	clients = (client_t **) calloc(max_clients, sizeof(client_t *));
	if(clients == NULL)
	{
		perror("client_initialize():");
		return -1;
	}

	client_limit = max_clients;

	return 0;
}

/*
 * Agregamos el cliente a la tabla de clientes/descriptores.
 * Es usada para relacionar los descriptores obtenidos por
 * la libreria de eventos y obtener un handle al cliente.
 */
static int
client_add(client_t *client)
{
	int fd = client->connection_fd;
	if(clients[fd])
	{
		fprintf(stderr, "client_add(fd=%d): Error, el cliente ya existe\n", fd);
		return -1;
	}

	clients[fd] = client;

	return 0;
}

/*
 * Removemos el cliente de la tabla de descriptores/clientes
 */
static int
client_remove(client_t *client)
{
	int fd = client->connection_fd;
	if((size_t) fd > client_limit || !clients[fd])
	{
		fprintf(stderr, "client_remove(fd=%d): Error, cliente inexistente\n", fd);
		return -1;
	}

	clients[fd] = NULL;

	return 0;
}

/*
 * Obtenemos un handle al cliente cuyo descriptor es 'fd'
 * Usado para mapear un fd a una estructura cliente
 */
client_t *
client_get(int fd)
{
	return ((size_t) fd > client_limit) ? NULL : clients[fd];
}

/*
 * Creamos un cliente con su correspondiente informacion que lo
 * identifica univocamente.
 */
int
client_create(client_t **client, int connection)
{
	// Este checkeo es del subsistema de clientes, no del Streamer.
	if(current_clients == client_limit)
	{
		fprintf(stderr, "client_create(): Se llego al limite de clientes\n");
		return -1;
	}

	current_clients++;

	client_t *tmp = (client_t *) malloc(sizeof(client_t));
	if(!tmp)
	{
		perror("client_create():");
		return -1;
	}

	tmp->connection_fd 	= connection;		// Guardamos un handle al socket
	tmp->id 			= connection;		// El socket es un buen ID de cliente
	tmp->name 			= "1337 Client";	// ?
	tmp->offset 		= 0;				// Offset actual dentro del Stream de datos
	tmp->state 			= CLIENT_WAITING;	// Estado inicial del cliente

	#ifdef BANDWIDTH_THROTTLING
	timerclear(&tmp->last_write_ts);
	#endif

	*client = tmp;

	return client_add(*client);
}

/*
 * Destruimos el cliente. Esta funcion debe ser llamada
 * siempre que creemos el cliente con client_create() de otra
 * manera habran memory leaks.
 */
int
client_destroy(client_t * client)
{
	int ret = client_remove(client);

	assert(client != NULL);
	free(client);

	current_clients--;

	return ret;
}
