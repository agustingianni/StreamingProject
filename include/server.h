/*
 * server.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef SERVER_H_
#define SERVER_H_

#define SERVER_STOPPED 0
#define SERVER_RUNNING 1

int server_enter_event_loop(void);
int server_leave_event_loop(void);
int server_initialize(void);
int server_shutdown(void);

#endif /* SERVER_H_ */
