/*
 * signal.c
 *
 *  Created on: Jul 16, 2009
 *      Author: gr00vy
 */

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>

#include "../include/signal.h"
#include "../include/main.h"
#include "../include/server.h"

extern server_t server;
extern config_t config;

// se nota que no me gusta que el compilador me tire warnings? :P
#define unused __attribute__ ((__unused__))

void
signal_handler(unused int sig)
{
	// Paramos el servidor.
	server_leave_event_loop();

	//signal(sig, &signal_handler);
}

int
signal_initialize(void)
{
	/*
	 * http://www.gnu.org/s/libc/manual/html_node/index.html#toc_Signal-Handling
	 *
	 * SIGTERM: Handler principal para la terminacion en modo DAEMON
	 * SIGINT: Handler para la terminacion en modo CONSOLA
	 */

	struct sigaction action;

	// !SA_RESTART asi sale de select() y se ejecuta el cleanup
	sigemptyset (&action.sa_mask);
	action.sa_flags = 0;

	// Handle CTRL+C
	action.sa_handler = (config.mode == CONSOLE_MODE) ? &signal_handler : SIG_IGN;
	if(sigaction (SIGINT, &action, NULL) == -1)
	{
		perror("signal_initialize():");
		return -1;
	}

	// Handle kill
	action.sa_handler = &signal_handler;
	if(sigaction (SIGTERM, &action, NULL) == -1)
	{
		perror("signal_initialize():");
		return -1;
	}

	action.sa_handler = SIG_IGN;
	if(sigaction (SIGHUP, &action, NULL) == -1)
	{
		perror("signal_initialize():");
		return -1;
	}

	if(sigaction (SIGQUIT, &action, NULL) == -1)
	{
		perror("signal_initialize():");
		return -1;
	}

	return 0;
}
