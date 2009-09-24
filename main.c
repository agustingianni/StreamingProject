/*
 * main.c
 *
 *  Created on: 11/07/2009
 *      Author: gr00vy
 *  ja!
 *  je!
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/server.h"
#include "include/client.h"
#include "include/main.h"
#include "include/signal.h"


server_t server;
config_t config;

static void
main_initialize(void)
{
	// Server status Defaults
	server.name 			= "KENNY Streaming Server";

	// Estado actual del servidor
	server.status 			= STATUS_INITIALIZING;

	// Numero actual de clientes siendo servidos
	server.clients 			= 0;

	// Deberia ser dinamico, leido del mp3
	server.bitrate 			= 128;	/* Kbps  */

	// Este funciona bien
	server.buffer_size 		= 1024;	/* bytes */

	// Por ahora esto no se usa, es para hacer mas eficiente
	// el uso del ancho de banda del servidor.
	server.timeout			= -1; /* en microsegundos */
	//((server.buffer_size * 8000) / server.bitrate)

	// Config defaults

	// Maximo numero de clientes que podemos servir
	config.max_clients 		= FD_SETSIZE - 24;

	// A futuro se puede especificar abrir mas de un socket
	// a la escucha y mandarlo a un proceso diferente.
	// Con esto podriamos incrementar la capacidad que tiene
	// el servidor de servir clientes
	config.sockets_number 	= 1;

	// Archivo que vamos a servir
	config.streaming_file 	= "test.mp3";
	config.mode 			= CONSOLE_MODE;
	config.port 			= DEFAULT_PORT;
	config.interface 		= "lo";
}

int
main_daemonize(void)
{
	pid_t pid;
	int i;

	pid = fork();
	if(pid == -1)
	{
		perror("main_daemonize():");
		return -1;
	}
	else if(pid)
	{
		// Terminamos el padre
		_exit(0);
	}

	// Creamos un nuevo process leader detachado de la TTY
	if(setsid() == -1)
	{
		perror("main_daemonize():");
		return -1;
	}

	chdir("/");

	// Obtenemos el numero maximo de sockets abiertos
	int maxfd = sysconf(_SC_OPEN_MAX);
	for (i = 0; i < maxfd; i++)
		 close(i);

	// Seteamos los permisos por defecto de creacion de archivos
	// del daemon
	umask(077);

	// Los abrimos por las dudas que alguien se le ocurra
	// escribir a 0,1,2 los descriptores por defecto
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	return 0;
}

static void
show_help(char **argv)
{
	printf
	(
		"Streaming Server v 0.1\n\n"
		"  -h            Muestra esta ayuda.\n"
		"  -f <archivo>  Especifica el archivo a servir (Path absoluto).\n"
		"  -p <puerto>   Especifica el puerto para escuchar.\n"
		"  -d            Iniciar en modo DAEMON (por defecto es en modo consola).\n\n"
		"Uso:\n"
		"   %s -f streaming.wav\n",
		argv[0]
	);

	exit(0);
}

static int
parse_arguments(int argc, char **argv)
{
	int c;

	while((c = getopt(argc, argv, "hf:dp:")) != -1)
	{
		switch(c)
		{
			// Show Help
			case 'h':
				show_help(argv);
				break;
			// Set individual streaming file
			case 'f':
				config.streaming_file = strdup(optarg);
				printf("Streaming file %s\n", config.streaming_file);
				break;
			// Run in daemon mode
			case 'd':
				config.mode = DAEMON_MODE;
				printf("Setting daemon mode on!\n");
				break;
			case 'p':
				config.port = (unsigned short) atoi(optarg);
				fprintf(stdout, "Puerto de escucha : %d\n", config.port);
				break;
			case '?':
				fprintf(stderr, "Error, opcion invalida\n");
				return -1;
			default:
				break;
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	int err = 0;

	/*
	 * TODO LIST:
	 *
	 * 	- insertar puntos de profiling
	 * 		- ver cuantos datos por segundo podemos enviar					LISTO
	 * 		- cuantos clientes soportamos									LISTO
	 * 		- cuanto tardamos en servir a cada cliente						LISTO
	 *
	 * Para testear usamos:
	 *   $ mplayer -nosound -ao null http://localhost:1337/ASd
	 */
	main_initialize();

	// Obtenemos todas las opciones (si las hay) por linea de comando
	if(parse_arguments(argc, argv) == -1)
	{
		fprintf(stderr, "[!] Error parseando argumentos.\n");
		return -1;
	}

	if(config.mode == DAEMON_MODE)
	{
		if(main_daemonize() == -1)
		{
			fprintf(stderr, "[!] No se pudo inicializar el servidor como demonio\n");
			return -1;
		}
	}

	if(signal_initialize() == -1)
	{
		fprintf(stderr, "[!] Error seteando manejadores de seniales\n");
		return -1;
	}

	fprintf(stdout, "[*] Inicializando el servidor\n");

	// Con esto tendriamos un origen de datos de donde leer y enviar a los clients
	if(stream_create(&server.stream, config.streaming_file) == -1)
	{
		fprintf(stderr, "[!] Error inicializando el stream de bytes\n");
		return -1;
	}

	/* Inicializamos el subsistema de networking */
	if(server_initialize() == -1)
	{
		fprintf(stderr, "[!] No se pudo inicializar el servidor.\n");
		stream_destroy(server.stream);
		return -1;
	}

	/* Inicializamos el subsistema de clientes */
	if(client_initialize(config.max_clients) == -1)
	{
		fprintf(stderr, "[!] No se pudo inicializar el subsistema de clientes.\n");
		stream_destroy(server.stream);
		server_shutdown();
		return -1;
	}

	fprintf(stdout, "[*] Entrando en el loop de eventos\n");

	// el loop de eventos sirve clientes con contenido
	if(server_enter_event_loop() == -1)
	{
		fprintf(stderr, "[!] No se pudo entrar en el loop de eventos.\n");
		stream_destroy(server.stream);
		server_shutdown();
		return -1;
	}

	fprintf(stdout, "[*] Cerrando el servidor\n");

	stream_destroy(server.stream);

	// cerramos el servidor. Matamos los clientes y liberamos archivos.
	if(server_shutdown() == -1)
	{
		fprintf(stderr, "[!] Cierre inconcluso del servidor, verifique los logs.\n");
		return -1;
	}

	return 0;
}
