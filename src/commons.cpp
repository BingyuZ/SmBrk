//============================================================================
// Name        : Commons.cc
// Author      : Bingyu
// Version     :
// Created on  : June 18, 2015
// Description : Misc subroutines
//============================================================================

#include "commons.h"

#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <signal.h>
#include <fcntl.h>

#ifndef CRONY_
#include <sys/eventfd.h>
#endif


#define SHELL	"/bin/sh"

using namespace std;


void MySleep(int mSeconds)
{
	::poll(NULL, 0, mSeconds);
}

void errQuit(int ret, const char *msgs)
{
	cerr << gAppName << ": " <<	msgs << endl;
	exit(ret);
}

// As popen(cmdstring, "r")
// Redirect both stdout and stderr
// The child process can use signal safe function only for multi-thread
// The functions are described in man 7 signal,
// POSIX-1-2004 confirms close(), dup2(), _exit(), execle(), execve()
// POSIX-1-2008 appends  execl()
FILE * popen2(const char *cmd, pid_t &childpid)
{
	int pfd[2];
	pid_t pid;
	FILE *fp;

	if (pipe(pfd)<0) return NULL;
	if ((pid=fork()) < 0) return NULL;
	else if (0 == pid)
	{			// Child
		close(pfd[0]);
		if (pfd[1] != STDOUT_FILENO)
		{
			dup2(pfd[1], STDOUT_FILENO);	// Redirect stdout
			dup2(pfd[1], STDERR_FILENO);	// Redirect stderr
			close(pfd[1]);
		}

		execl(SHELL, "sh", "-c", cmd, (char *)0);
		_exit(127);
	}
	else
	{
		// Parent
		close(pfd[1]);
		if ((fp = fdopen(pfd[0], "r")) == NULL) return NULL;
	}
	childpid = pid;
	return fp;
}

int pclose2(FILE *fp, pid_t pid)
{
	int stat;

	if (fclose(fp) == EOF) return -1;

	while(waitpid(pid, &stat, 0) < 0)
		if (errno != EINTR) return -1;

	if (WIFEXITED(stat)) return(WEXITSTATUS(stat));
	else return -2;
}

int SecondsInDay()
{
	struct tm tmt;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tmt);

	return tmt.tm_hour*3600 + tmt.tm_min*60 + tmt.tm_sec;
}

void FormatDate(char *buf, const time_t *tv_sec)
{
	struct tm tm_time;

	if (tv_sec == NULL)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);

		localtime_r(&tv.tv_sec, &tm_time);
	}
	else localtime_r(tv_sec, &tm_time);

	sprintf(buf, "%4d-%02d-%02d",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday);
}

const char *gsFmtLong = "%4d-%02d-%02d %02d:%02d:%02d";
const char *gsFmtStd = "%4d%02d%02d %02d:%02d:%02d";
const char *gsFmtCompact = "%4d%02d%02d%02d%02d%02d";

void FormatZStr(char *buf, struct timeval *pTv, bool bLocal, bool withUs)
{
	struct tm tm_time;
	struct timeval tv;

    if (pTv == NULL)
    {
		gettimeofday(&tv, NULL);
        pTv = &tv;
    }
    if (bLocal) localtime_r(&pTv->tv_sec, &tm_time);
    else gmtime_r(&pTv->tv_sec, &tm_time);

    sprintf(buf, "%2d%02d%02d%02d%02d%02d",
            tm_time.tm_year - 100, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    while (*buf) buf++;
    if (withUs)
        sprintf(buf, ".%06ld", pTv->tv_usec);
}

void FormatTimeString(char *buf, const char *format,
                      const time_t *tv_sec, bool bLocal)
{
	struct tm tm_time;

	if (tv_sec == NULL)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);

		if (bLocal) localtime_r(&tv.tv_sec, &tm_time);
		else gmtime_r(&tv.tv_sec, &tm_time);
	}
	else {
        if (bLocal) localtime_r(tv_sec, &tm_time);
        else gmtime_r(tv_sec, &tm_time);
	}

	sprintf(buf, format,
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

int CreatePipes(int *pfd)
{
	int ret = pipe(pfd);
	if (ret < 0)
	{
		cerr << "Failed in eventfd" << endl;
	    abort();
	}
	int flags = fcntl(pfd[0], F_GETFL);
	fcntl(pfd[0], F_SETFL, flags | O_NONBLOCK);
	return 0;
}

#ifndef CRONY_
int CreateEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    cerr << "Failed in eventfd" << endl;
    abort();
  }
  return evtfd;
}
#endif
