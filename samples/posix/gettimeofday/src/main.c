/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

void convert_time(struct timeval tv);

void main(void)
{
	struct timeval tv;

	while (1) {
		int res = gettimeofday(&tv, NULL);

		if (res < 0) {
			printf("Error in gettimeofday(): %d\n", errno);
			return 1;
		}

		printf("gettimeofday(): HI(tv_sec)=%d, LO(tv_sec)=%d, "
		       "tv_usec=%d\n", (unsigned int)(tv.tv_sec >> 32),
		       (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
                convert_time(tv);
		sleep(1);
	}
}

// time_t current_time;
// char* c_time_string;

/* Obtain current time. */
void convert_time(struct timeval tv)
{
    char* c_time_string;

/* Convert to local time format. */
    c_time_string = ctime(&tv);

    if (c_time_string == NULL)
    {
        (void) fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    }

/* Print to stdout. ctime() has already added a terminating newline character. */
    (void) printf("Current time is %s", c_time_string);
}
