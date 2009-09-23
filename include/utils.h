/*
 * utils.h
 *
 *  Created on: Jul 17, 2009
 *      Author: gr00vy
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifndef TEMP_FAILURE_RETRY
#include <errno.h>
# define TEMP_FAILURE_RETRY(expression) \
	(__extension__							      \
			({ long int __result;						      \
			do __result = (long int) (expression);				      \
			while (__result == -1L && errno == EINTR);			      \
			__result; }))
#endif

#endif /* UTILS_H_ */
