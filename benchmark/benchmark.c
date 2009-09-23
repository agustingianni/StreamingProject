/*
 * benchmark.c
 *
 *  Created on: Sep 7, 2009
 *      Author: gr00vy
 */

/*
 * Programa simple para probar la performance del servidor de Streaming.
 *
 * Basicamente se ejecutan 'MAX_CLIENTS' hijos los cuales llaman a mplayer
 * u otro programa segun corresponda con el sistema que se corra.
 *
 * Se utiliza este programa para facilitar la simulacion de carga en el sistema.
 * Los resultados del benchmark seran recojidos en el servidor ya que se
 * exportan variables de benchmark si se compila con el flag DO_BENCHMARK
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HELLO 		"icy-metadata: 1"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100		// numero de clientes a spawnear
#define N 			60		// numero de request, cada uno es aprox. un segundo
#define KBPS 		128		// default bitrate

int
client(void)
{
	char buffer[BUFFER_SIZE];
	int sock;
	struct sockaddr_in addr;

	struct timeval t1;
	struct timeval t2;
	size_t received = 0;
	size_t r = 0;
	useconds_t elapsed_time	= 0;
	uint64_t total_time = 0;
	int i;

	/* Numero de veces que se paso del limite de 1 segundo */
	int playback_failure = 0;

	memset((void *) &addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1337);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock == -1)
	{
		perror("socket()");
		return -1;
	}

	if(connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("connect()");
		close(sock);
		return -1;
	}

	/* Le enviamos al servidor la cabezera minima para que acepte la conexion */
	if(send(sock, HELLO, strlen(HELLO), 0) < strlen(HELLO))
	{
		perror("send()");
		close(sock);
		return -1;
	}

	/* Recibimos la respuesta del servidor y la descartamos ya que no es de utilidad */
	if(recv(sock, &buffer, sizeof(buffer), 0) == -1)
	{
		perror("recv()");
		close(sock);
		return -1;
	}

	for(i = 0; i < N; i++)
	{
		gettimeofday(&t1, NULL);

		/* Cortamos cuando el "buffer" se llena con 128 kbits de audio */
		while(received < KBPS*1000/8)
		{
			/* La informacion se descarta ya que no es usada en el benchmark */
			if((r = recv(sock, &buffer, BUFFER_SIZE, 0)) == -1)
			{
				perror("recv()");
				close(sock);
				return -1;
			}

			received += r;
		}

		gettimeofday(&t2, NULL);

		/*
		 * Calculamos el tiempo que tardamos en llenar el buffer.
		 * Realizamos la correccion correspondiente si tomamos el segundo timestamp
		 * en un segundo diferente al primero
		 */
		elapsed_time = (t2.tv_sec - t1.tv_sec) ? ((t2.tv_usec + 1000000) - t1.tv_usec) : (t2.tv_usec- t1.tv_usec);
		total_time += elapsed_time;
		received = 0;

		/* Si paso mas de un segundo en llenar el buffer hubo un fallo de reproduccion */
		if(elapsed_time >= 1000000)
			playback_failure++;

		// Ajustamos cuando debemos dormir si es que el recv del buffer
		// recibio mas rapido las cosas
		// usar sleep es benefico ya uqe el benchmark de por si no seria real
		// ya que un proceso se usaria todo su timeslice para IO y no dejaria
		// al resto de los procesos concurrentes del benchmark realizar IO
		// El sleep este ayuda a contrarestar ese efecto introduciendo una oportunidad
		// para el scheduler de pasar a otro proceso.
		// La cantidad de tiempo a sleepear va a ser aquella que reste para completar un segundo
		// ya que las tranferencias se miden sobre kilo bits por segundo y nosotros satisfacemos
		// en el loop de recepcion el volumne de datos necesario para realizar playback durante un segundo.
		if(elapsed_time < 1000000)
			usleep(1000000 - elapsed_time);
	}

	printf("Tiempo promedio de llenado de buffer %f\n", total_time*1.0/N);

	if(playback_failure)
		printf("Numero de fallas %d\n", playback_failure);

	close(sock);

	return 0;
}

int
main(int argc, char **argv)
{
	int i;
	int status;
	int max_clients = MAX_CLIENTS;

	if (argc >= 2)
		max_clients = atoi(argv[1]);

	printf("Inicializando %d clientes\n", max_clients);

	for (i = 0; i < max_clients; i++)
	{
		if(!fork())
		{
			//return execlp("mplayer", "mplayer", "-nosound", "-ao", "null", "-really-quiet", "http://localhost:1337/Streaming", NULL);
			client();
			exit(0);
		}
	}

	for (i = 0; i < max_clients; i++)
	{
		wait(&status);
	}

	return 0;
}
