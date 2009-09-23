/*
 * main.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <signal.h>

#include "stream.h"
#include "event.h"

typedef struct
{
	char *name;

	#define STATUS_RUNNING 0
	#define STATUS_STOPPED 1
	#define STATUS_INITIALIZING 2

	// Esta variable es volatile por que puede
	// ser modificada por un sighandler
	volatile sig_atomic_t status;

	unsigned int clients;

	int server_fd;

	stream_t *stream;

	events_t ev;

	// Datos que modifican la performance del server, los dejo por defecto
	// en general estan bien.
	int bitrate;					// default 128 kbps
	int buffer_size;				// 512 bytes
	int timeout;					// 32000 usecs

}server_t;

typedef struct
{
	unsigned int max_clients; 		// hard limit de conexiones de clientes
	unsigned short sockets_number; 	// numero de sockets listeners
	char *streaming_file;

	#define DEFAULT_PORT 1337
	unsigned short port;
	char *interface;

	#define DAEMON_MODE 0
	#define CONSOLE_MODE 1
	unsigned int mode;
} config_t;

#endif /* MAIN_H_ */
