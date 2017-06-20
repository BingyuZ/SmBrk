/*
 * commons.h
 *
 *  Created on: Dec 1, 2014
 *      Author: bingyu
 */

#ifndef COMMONS_H_
#define COMMONS_H_

// for 2.6.22-
//#define CRONY_

#include "gconf.h"

//#include "Mutex.h"
//#include "Thread.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <poll.h>

extern const char *gAppName;			// Should be defined in main stub
void errQuit(int ret, const char *msgs);

FILE *popen2(const char *cmd, pid_t &childpid);
int pclose2(FILE *fp, pid_t pid);

void MySleep(int mSeconds);
void FormatTimeString(char *buf, const time_t *tv_sec = NULL);
void FormatDate(char *buf, const time_t *tv_sec = NULL);
int SecondsInDay();

int CreateEventfd();
int CreatePipes(int *pfd);

#endif /* COMMONS_H_ */
