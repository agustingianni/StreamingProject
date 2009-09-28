/*
 * server.c
 *
 *  Created on: 11/07/2009
 *      Author: gr00vy
 */
#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include "../include/server.h"
#include "../include/main.h"
#include "../include/stream.h"
#include "../include/client.h"
#include "../include/socket.h"
#include "../include/event.h"

extern server_t server;
extern config_t config;

static int
server_running(void)
{
	// TODO: Ver si necesitamos mutex o variables atomicas
	return server.status == SERVER_RUNNING;
}

/*
 * Maneja los requests que hacen los clientes.
 */
int
connection_client_handler(events_t *ev, event_t *event)
{
	client_t *client = client_get(event->fd);

	char *data =
	"ICY 200 OK\r\n"
	"icy-notice1: NOTICE 1\r\n"
	"icy-notice2: NOTICE 2\r\n"
	"icy-name: STATION NAME\r\n"
	"icy-genre: GENRE\r\n"
	"icy-url: http://localhost\r\n"
	"content-type: audio/mpeg\r\n"
	"icy-pub: 1\r\n"
	"icy-metaint: 0\r\n"
	"icy-br: 128\r\n"
	"\r\n";

	// TODO a buffer moverlo de aca, no tiene sentido alocarlo en la stack cada ves
	// que llamamos al event handler
	char buffer[512];

	if(event->event_mask & EVENT_READ)
	{
		// Evento de lectura, puede ser que se cerro el cliente
		// o que es el inicio de la conexion y estamos por
		// intercambiar headers.
		// Otra cosa es error, no esperada.

		// Esta funcion es llamada cada ves que hay un evento de lectura
		// es decir cuando uno de nuestros clientes nos envia algo.
		// Esto solo sucede al inicio de la conexion cuando el servidor y
		// el cliente intercambian headers con informacion sobre la proxima
		// conexion.
		//
		// Vamos a aceptar los clientes nuevos aca, a nivel ICECAST no nivel socket
		// es decir que vamos a crear las estructuras necesarias para
		// mantener la informacion sobre los clientes.
		// Tambien vamos a manejar las desconexiones. Estas las vamos a detectar
		// por que el socket al leer nos va a devolver 0 bytes.


		// Si el cliente estaba en estado WAITING lo proximo que tendria
		// que enviar son headers de ICECAST para establecer conexion.
		if(client->state == CLIENT_WAITING)
		{
			#ifdef DEBUG
			printf("[cliente=%d] connection_client_handler(EVENT_READ):CLIENT_WAITING\n", event->fd);
			#endif

			if(socket_read_string(event->fd, buffer, sizeof(buffer)) == -1)
			{
				perror("connection_client_handler():");
				return -1;
			}

			// Por ahora la verificacion es simple
			if(strcasestr(buffer, "icy-metadata: 1") == NULL)
			{
				#ifdef DEBUG
				fprintf(stderr, "[cliente=%d] Headers invalidos, desconectando\n", event->fd);
				#endif

				event_del(ev, event->fd);
				socket_close(event->fd);
				client_destroy(client);
				server.clients--;

				return 0;
			}

			// Si esta todo bien seteamos como empezando la conexion
			client->state = CLIENT_STARTING;

			return 0;
		}
		else
		{
			// Manejar desconecciones
			if(socket_read_string(event->fd, buffer, sizeof(buffer)) == 0)
			{
				#ifdef DEBUG
				fprintf(stderr, "[cliente=%d] Desconectando cliente\n", event->fd);
				#endif

				event_del(ev, event->fd);
				socket_close(event->fd);
				client_destroy(client);
				server.clients--;

				return 0;
			}

			#ifdef DEBUG
			fprintf(stderr, "[cliente=%d] Invalid read\n%s\n", event->fd, buffer);
			#endif
		}
	}
	else if(event->event_mask & EVENT_WRITE)
	{
		char *buffer;
		int ret;

		switch(client->state)
		{
			case CLIENT_STARTING:
				#ifdef DEBUG
				printf("[cliente=%d] connection_client_handler(EVENT_WRITE):CLIENT_STARTING\n",
						event->fd);
				#endif

				if(socket_write_all(event->fd, data, strlen(data)) == -1)
				{
					perror("connection_client_handler():");
					return -1;
				}

				client->state = CLIENT_ACTIVE;
				break;
			case CLIENT_ACTIVE:
				// Servir con contenido al cliente.
				#ifdef DEBUG
				fprintf(stderr, "[cliente=%d] connection_client_handler(EVENT_WRITE):CLIENT_ACTIVE\n",
						event->fd);
				#endif

				ret = stream_read(server.stream, client->offset,
						server.buffer_size, &buffer);

				if(ret == -1)
				{
					perror("connection_client_handler():");
					return -1;
				}

				#ifdef BANDWIDTH_THROTTLING
				#include <sys/time.h>

				struct timeval now;
				if(gettimeofday(&now, NULL) == -1)
				{
					perror("connection_client_handler():");
					return -1;
				}

				static inline int
				difftime(struct timeval *t1, struct timeval *t2, useconds_t interval)
				{
					// Si hay diferencia de segundos devolver que si paso el
					// intervalo ya que este es medido en usecs.
					if(t2->tv_sec - t1->tv_sec)
						return 1;

					// si la diff entre usecs es mayor o igual que interval
					// podemos enviar
					return ((t2->tv_usec - t1->tv_usec) >= interval) ? 1 : 0;
				}

				if(!difftime(&client->last_write_ts, &now, interval))
				{
					// El intervalo no paso, hacemos que se sirva otro socket
					return 0;
				}
				#endif

				// Podriamos haber usado socket_write_all pero no me convence, por que
				// si fallo, es por algo y de ultima eso lo manejamos con el offset del cliente
				// y los datos se los reenviamos.
				ret = socket_write(event->fd, buffer, server.buffer_size);
				if(ret == -1)
				{
					perror("connection_client_handler():");
					return -1;
				}

				#ifdef BANDWIDTH_THROTTLING
				if(gettimeofday(&client->last_write_ts, NULL) == -1)
				{
					perror("connection_client_handler():");
					return -1;
				}
				#endif


				client->offset += ret;

				// Por ahora reiniciamos el streaming.
				if(client->offset == server.stream->size)
				{
					#ifdef DEBUG
					fprintf(stderr, "[cliente=%d] connection_client_handler(CLIENT_ACTIVE):EVENT_WRITE: Se llego al final del stream, reiniciando.\n"
							, event->fd);
					#endif

					client->offset = 0;
				}

				break;
			default:
				break;
		}
	}

	return 0;
}

/*
 * Manejador de nuevas conexiones.
 */
int
connection_accept_handler(events_t *ev, event_t *event)
{
	int client_fd;
	client_t *c;

	/* Aceptamos un nuevo cliente */
	if((client_fd = socket_accept(event->fd)) == -1)
	{
		perror("connection_accept_handler():");
		return -1;
	}

	if(server.clients == config.max_clients)
	{
		fprintf(stdout, "connection_accept_handler(): Guarda! Se llego al limite de clientes\n");
		socket_close(client_fd);
		return 0;
	}

	/* Creamos una struct cliente para mantener los datos */
	if(client_create(&c, client_fd) == -1)
	{
		fprintf(stderr, "connection_accept_handler(): Error creando cliente\n");
		socket_close(client_fd);
		return -1;
	}

	/* Seteamos el manejador de eventos de lectura para obtener requests del cliente */
	if(event_add(ev, client_fd, &connection_client_handler,
			EVENT_READ | EVENT_WRITE) == -1)
	{
		fprintf(stderr, "connection_accept_handler(): Error seteando el event handler\n");

		client_destroy(c);
		socket_close(client_fd);
		server.clients--;
		return -1;
	}

	server.clients++;

	#ifdef DEBUG
	printf("connection_accept_handler(): Aceptada nueva conexion: %d\n", client_fd);
	#endif

	return 0;
}

/*
 * Aca vamos a estar escuchando por clientes
 * cuando recibimos creamos todas las structuras
 * necesarias para mantener un control de cada uno
 * de los clientes.
 */
int
server_enter_event_loop(void)
{
	int 		nevents;
	event_t 	event;

	server.status = SERVER_RUNNING;

	/* inicializamos el monitor de eventos */
	if(event_initialize(&server.ev) == -1)
	{
		fprintf(stderr, "server_enter_event_loop(): Error inicializando el monitor de eventos\n");
		return -1;
	}

	/* agregamos un handler para las conexiones nuevas */
	if(event_add(&server.ev, server.server_fd,
			&connection_accept_handler, EVENT_READ) == -1)
	{
		fprintf(stderr, "server_enter_event_loop(): Error seteando el event handler\n");
		event_shutdown(&server.ev);
		return -1;
	}

	while(server_running())
	{
	    nevents = event_wait(&server.ev, server.timeout);

	    if(nevents == -1)
	    {
	    	fprintf(stderr, "server_enter_event_loop(): Error esperando por eventos\n");
	    	break;
	    }

	    while(nevents > 0)
	    {
	    	nevents -= event_next(&server.ev, &event);

			#ifdef BENCHMARK
	    	struct timeval t1, t2;
	    	uint64_t elapsed_time = 0;
	    	uint32_t events_served = 0;

	    	gettimeofday(&t1, NULL);
			#endif

	    	event.handler(&server.ev, &event);

	    	#ifdef BENCHMARK
	    	gettimeofday(&t2, NULL);
	    	events_served++;
	    	elapsed_time += (t2.tv_sec - t1.tv_sec) ? ((t2.tv_usec + 1000000) - t1.tv_usec) : (t2.tv_usec- t1.tv_usec);
			#endif
	    }

	    fprintf(stdout, "\r[*] Numero de clientes = %d", server.clients);
	}

	printf("\n");

	#ifdef BENCHMARK
	printf("Tiempo promedio de servicio por evento %f\n", 1.0*elapsed_time/events_served);
	#endif

	event_shutdown(&server.ev);

	return 0;
}

int
server_leave_event_loop(void)
{
	// Hace que salgamos del loop de eventos
	server.status = SERVER_STOPPED;

	return 0;
}

int
server_initialize(void)
{
	int server_fd;

	// Inicializamos el socket servidor y lo ponemos a la escucha de clientes
	server_fd = socket_tcp_server(config.port);
	if(server_fd == -1)
	{
		perror("server_initialize():");
		return -1;
	}

	// NON_BLOCKING + select() = FULL OF WIN; Los sockets acceptados de
	// este van a ser NON_BLOCKING tambien
	if(socket_set_nonblocking(server_fd))
	{
		perror("server_initialize():");
	}

	if(socket_listen(server_fd) == -1)
	{
		perror("server_initialize():");
		socket_close(server_fd);
		return -1;
	}

	server.server_fd = server_fd;

	return 0;
}

int
server_shutdown(void)
{
	socket_close(server.server_fd);

	server.server_fd = -1;

	return 0;
}



