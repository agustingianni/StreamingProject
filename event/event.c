/*
 * event.c
 *
 *  Created on: Jul 20, 2009
 *      Author: gr00vy
 */

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#include "../include/event.h"

int
event_initialize(events_t *ev)
{
	// Current fd es usado para el iterator (get next event)
	ev->currfd	= -1;

	ev->maxfds 		= FD_SETSIZE - 24;	/* dejamos algunos fd's libres par ala app. */
	ev->max_fd 		= -1;				/* socket maximo en -1 */
	ev->nevents		= 0;
	ev->handlers 	= (handler_t *) calloc(ev->maxfds, sizeof(handler_t));
	if(ev->handlers == NULL)
	{
		perror("event_initialize():");
		return -1;
	}

	event_reset(ev);

	return 0;
}

int
event_shutdown(events_t *ev)
{
	if(ev->handlers)
		free(ev->handlers);

	event_reset(ev);

	ev->currfd = -1;
	ev->max_fd = -1;
	ev->nevents = 0;

	return 0;
}

int
event_reset(events_t *ev)
{
	FD_ZERO(&(ev->set_read));
	FD_ZERO(&(ev->set_write));
	FD_ZERO(&(ev->set_error));

	ev->max_fd = -1;

	return 0;
}

int
event_del(events_t *ev, int fd)
{
	if ((fd < 0) || ((unsigned) fd >= ev->maxfds))
	{
		fprintf(stderr, "event_del(fd=%d): fd out of range\n", fd);
		return -1;
	}

	if (ev->handlers[fd] == NULL)
	{
		fprintf(stderr, "event_del(fd=%d): not monitoring that fd!\n", fd);
		return -1;
	}

	event_set_mask(ev, fd, EVENT_NONE);

	FD_CLR(fd, &(ev->set_read_copy));
	FD_CLR(fd, &(ev->set_write_copy));
	FD_CLR(fd, &(ev->set_error_copy));

	ev->handlers[fd] = NULL;

	/* Si el fd que sacamos era el MAX_FD tenemos que buscar el mayor de nuevo */
	while ((ev->max_fd >= 0) && (ev->handlers[ev->max_fd] == NULL))
		ev->max_fd--;

	return 0;
}

int
event_add(events_t *ev, int fd, handler_t handler, int event_mask)
{
	if ((fd < 0) || ((unsigned) fd >= ev->maxfds))
	{
		fprintf(stderr, "event_add(fd=%d): fd out of range\n", fd);
		return -1;
	}

	if (ev->handlers[fd])
	{
		fprintf(stderr, "event_add(fd=%d): already monitoring that fd!\n", fd);
		return -1;
	}

	/* actualizamos el maximo fd */
	if (fd > ev->max_fd)
		ev->max_fd = fd;

	ev->handlers[fd] = handler;
	event_set_mask(ev, fd, event_mask);

	return 0;
}

void
event_set_mask(events_t *ev, int fd, int event_mask)
{
	/* dependiendo el tipo de evento (lectura, escritura o exception) */
	FD_CLR(fd, &(ev->set_read));
	if (event_mask & EVENT_READ)
		FD_SET(fd, &(ev->set_read));

	FD_CLR(fd, &(ev->set_write));
	if (event_mask & EVENT_WRITE)
		FD_SET(fd, &(ev->set_write));

	FD_CLR(fd,&(ev->set_error));
	if (event_mask & EVENT_EXCEPT)
		FD_SET(fd, &(ev->set_error));
}

int
event_wait(events_t *ev, int microseconds)
{
	struct timeval *t = NULL;

	if(microseconds != -1)
	{
		t 			= &ev->tv;
		t->tv_sec 	= 0;
		t->tv_usec 	= microseconds;
	}

	ev->set_read_copy 	= ev->set_read;
	ev->set_write_copy 	= ev->set_write;
	ev->set_error_copy 	= ev->set_error;

	/* va a ser incrementado en event_next() */
	ev->currfd = -1;

	ev->nevents = select(ev->max_fd + 1, &(ev->set_read_copy),
			&(ev->set_write_copy), &(ev->set_error_copy), t);

	// Si hubo un error y no es que un sighandler interrumpio la llamada
	if(ev->nevents == -1)
	{
		if(errno == EINTR)
			return 0;

		perror("event_wait():");
		return -1;
	}

	return ev->nevents;
}

int
event_next(events_t *ev, event_t *event)
{
	assert(event != NULL);

	int ret =0;
	event->event_mask = EVENT_NONE;

	for (; ev->currfd++ < ev->max_fd;)
	{
		if (FD_ISSET(ev->currfd, &(ev->set_read_copy)))
		{
			event->event_mask |= EVENT_READ;
			ev->nevents--;
			ret++;
		}

		if (FD_ISSET(ev->currfd, &(ev->set_write_copy)))
		{
			event->event_mask |= EVENT_WRITE;
			ev->nevents--;
			ret++;
		}

		if (FD_ISSET(ev->currfd, &(ev->set_error_copy)))
		{
			event->event_mask |= EVENT_EXCEPT;
			ev->nevents--;
			ret++;
		}

		if(event->event_mask)
			break;
	}

	event->fd 		= ev->currfd;
	event->handler 	= ev->handlers[ev->currfd];

	assert(event->handler != NULL);

	return ret;
}
