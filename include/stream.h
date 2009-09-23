/*
 * stream.h
 *
 *  Created on: 12/07/2009
 *      Author: gr00vy
 */

#ifndef STREAM_H_
#define STREAM_H_

#include <sys/types.h>

typedef struct
{
	char *filename;
	int id;
	size_t size;
	char *bytes;

}stream_t;

int stream_create(stream_t **stream, const char *filename);
int stream_destroy(stream_t *stream);
int stream_read(stream_t *stream, off_t offset, size_t size, char **buffer);

#endif /* STREAM_H_ */
