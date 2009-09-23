/*
 * event.h
 *
 *  Created on: Jul 20, 2009
 *      Author: gr00vy
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct events;
struct event;
typedef int (*handler_t)(struct events *ev, struct event *event);

typedef struct event
{
	int 		fd;
	handler_t	handler;
	int 		event_mask;
} event_t;

typedef struct events
{
	handler_t *	handlers;
	size_t 		maxfds;
	int 		currfd;

	fd_set 		set_read_copy;
	fd_set 		set_write_copy;
	fd_set 		set_error_copy;

	fd_set 		set_read;
	fd_set 		set_write;
	fd_set 		set_error;

	int 		max_fd;
	int			nevents;

	int 		has_timeout;
	struct timeval tv;
} events_t;

#define EVENT_READ 		1
#define EVENT_WRITE 	2
#define EVENT_EXCEPT 	4
#define EVENT_NONE		0

int			event_initialize		(events_t *ev);
int			event_shutdown			(events_t *ev);
void 		event_free				(events_t *ev);
int			event_reset				(events_t *ev);
int 		event_add				(events_t *ev, int fd, handler_t handler, int event_mask);
int 		event_del				(events_t *ev, int fd);
int			event_next				(events_t *ev, event_t *event);
int 		event_wait				(events_t *ev, int microseconds);
void		event_set_mask			(events_t *ev, int fd, int event_mask);

#endif /* EVENT_H_ */

