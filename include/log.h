/*
 * log.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef LOG_H_
#define LOG_H_

int log_initialize(void);
int log_fatal(char *msg);
int log_warning(char *msg);
int log_info(char *msg);
int log_console(char *msg);

#endif /* LOG_H_ */
