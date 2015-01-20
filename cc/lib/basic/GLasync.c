/*
**      File ID:        @(#): <MID19495 () - 08/17/02, 29.1.1.1>
**
**	File:					MID19495
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:43:32
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the functions for the general async device
**      library.  These functions mimic many of the "standard" C input/output
**      functions.  But they deal more gracefully with tandem hardware errors.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
#include <stdlib.h>
#include <stdarg.h>
#include <sysent.h>
#include <signal.h>
#include <fcntl.h>
#include <termio.h>
#include "cc/hdr/misc/GLasync.h"
#include "cc/hdr/misc/GLvsprintf.h"
#include "cc/hdr/cr/CRdebugMsg.hh"

#include <errno.h>

static GLasyncErrHandler GLasyncPortErrHandler = NULL;
static int GLasyncGuardTime = 5; /* default is 5 seconds */

void
GLsetAsyncGuardTime(int seconds)
{
	if (seconds <= 0)
	{
		CRERROR("invalid value (%ld) for guard time",
			seconds);
	}
	else
		GLasyncGuardTime = seconds;
}

static void
GLalarmHandler(int)
{
	CRDEBUG(CRusli, ("alarm caught"));
}

inline
void
GLinitAsync()
{
#ifdef __mips
                signal(SIGALRM,GLalarmHandler);
#else
                class sigaction act;
#if defined(UNIXWARE) | defined(SOLARIS5_4)
                act.sa_handler = (void(*)())GLalarmHandler;
#else
                act.sa_handler = GLalarmHandler;
#endif
		sigemptyset( & act.sa_mask );
                act.sa_flags = 0;
                (Void) sigaction( SIGALRM, &act, NULL );
#endif
}

static
void
GLsetAlarm(int seconds)
{
	GLinitAsync();
	alarm(seconds);
}
	
void
GLsetAsyncErrHandler(GLasyncErrHandler fp)
{
	CRDEBUG(CRusli, ("GLsetAsyncErrHandler(%x)", fp));
	GLasyncPortErrHandler = fp;
}

static
void
GLcallErrHandler(int fd)
{
	if (GLasyncPortErrHandler)
		(*GLasyncPortErrHandler)(fd);
}

static
int
GLasyncFail(int fd)
{
	GLcallErrHandler(fd);
	return GLasyncHwFailure;
}

static
int
GLasyncRet(const char* caller, int kernelRetVal, unsigned alarmRetVal, int fd)
{
	if (kernelRetVal == -1)
	{
		CRDEBUG(CRusli, ("caller=%s, fd=%d, kernalRetVal=-1, errno=%d",
				 caller, fd, errno));
		switch (errno) {
		    case EINTR:
			if (alarmRetVal == 0)
				return GLasyncFail(fd);
			break;
		    case EIO:
		    case ENXIO:
			return GLasyncFail(fd);
		}
	}
	return kernelRetVal;
}	

static inline
unsigned
GLclearAlarm()
{
	return alarm(0);
}

int
GLopen(const char* fname, int oflag, int mode)
{
	GLsetAlarm(GLasyncGuardTime);
	int kret = ::open(fname, oflag, mode);
	return GLasyncRet("GLopen",kret, GLclearAlarm(), -1);
}

int
GLclose(int fd)
{
	GLsetAlarm(GLasyncGuardTime);
	int kret = ::close(fd);
	return GLasyncRet("GLclose", kret, GLclearAlarm(), fd);
}

int
GLisatty(int fd)
{
	GLsetAlarm(2);
	int kret = ::isatty(fd);
	return GLasyncRet("GLisatty", kret, GLclearAlarm(), fd);
}

int
GLioctl(int fd, int request, struct termio* termioPtr)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::ioctl(fd, request, termioPtr);
	return GLasyncRet("GLioctl(int,int,termio*)", ioctlRet,
			  GLclearAlarm(), fd);
}

#ifdef CC
int
GLioctl(int fd, int request, struct termcb* termcbPtr)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::ioctl(fd, request, termcbPtr);
	return GLasyncRet("GLioctl(int,int,termcb*)", ioctlRet,
			  GLclearAlarm(), fd);
}
#endif

int
GLioctl(int fd, int request)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::ioctl(fd, request);
	return GLasyncRet("GLioctl(int,int)", ioctlRet, GLclearAlarm(), fd);
}

int
GLioctl(int fd, int request, int arg)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::ioctl(fd, request, arg);
	return GLasyncRet("GLioctl(int,int,int)", ioctlRet,
			  GLclearAlarm(), fd);
}

int
GLioctl(int fd, int request, int* arg)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::ioctl(fd, request, arg);
	return GLasyncRet("GLioctl(int,int,int*)", ioctlRet,
			  GLclearAlarm(), fd);
}

int
GLfcntl(int fd, int request, int arg)
{
	GLsetAlarm(GLasyncGuardTime);
	int ioctlRet = ::fcntl(fd, request, arg);
	return GLasyncRet("GLfcntl", ioctlRet, GLclearAlarm(), fd);
}

/* blocking read */
int
GLread(int fd, void* buf, int maxlen)
{
	int readRetval = ::read(fd, (char*)buf, maxlen);
	if (readRetval == 0)
	{
		CRDEBUG(CRusli, ("read() returned EOF"));
		return GLasyncFail(fd);
	}
	return GLasyncRet("GLread", readRetval, 1, fd);
}

/* non blocking read */
int
GLnonblkRead(int fd, void* buf, int maxlen)
{
	GLsetAlarm(GLasyncGuardTime);
	int kret = ::read(fd, (char*)buf, maxlen);
	return GLasyncRet("GLnonblkRead", kret, GLclearAlarm(), fd);
}

const char* GLwriteErrFmt = 
   "write() returned %d (errno=%d)\nattempted to write %d chars";

static
int
glwrite(Bool firstTry, const char* func, int fd, const void* buf, int len)
{
	if (firstTry == NO)
	{
		if (GLISATTY(fd) == 1)
		{
			/* Maybe the user has done a CTL-S, 
			** So try to restart the output.
			*/
			CRDEBUG(CRusli, ("about to try ioctl(TCXONC, 1)"));
			int ioctlRetval = GLIOCTL(fd, TCXONC, 1);
			if (ioctlRetval != GLsuccess)
			{
				CRDEBUG(CRusli, ("ioctl(TCXONC, 1) returned %d, errno=%d",
						 ioctlRetval, errno));
				return ioctlRetval;
			}
		}
	}
	GLsetAlarm(GLasyncGuardTime);
	int retval = ::write(fd, (const char*)buf, len);
	if (retval != len)
	{
		if (GLclearAlarm() == 0 && errno == EINTR && firstTry)
		{
			CRDEBUG(CRusli, ("trying %s again", func));
			const char* restartPos = (const char*) buf + retval;
			return glwrite(NO, func, fd, restartPos, len-retval);
 		}
		CRDEBUG(CRusli, (GLwriteErrFmt, retval, errno, len));
		return GLasyncFail(fd);
	}

	return GLasyncRet(func, retval, GLclearAlarm(), fd);
}

int
GLwrite(int fd, const void* buf, int len)
{
	return glwrite(YES, "GLwrite", fd, buf, len);
}

/* returns same values as GLwrite()
*/
int
GLfputs(const char* str, FILE* fp)
{
	int fd = fileno(fp);
	int len = strlen(str);

	return glwrite(YES, "GLfputs", fd, str, len);
}

int
GLprintf(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);

	int have_buf = 1;
	const char *buf = GLvsprintf(format, ap);
	va_end(ap);
	if ( buf == NULL ) 
	{
		/* it failed that time; try something different */
		buf = GLsprintf("(Unable to format '%s')", format);
		if ( buf == NULL ) {
			have_buf = 0;
			buf = "(Memory allocation error)";
		}
	}
	int fd  = fileno(stdout);
	int len = strlen(buf);
	int ret_val = glwrite(YES, "GLprintf", fd, buf, len);
	if ( have_buf ) 
	{
		free((void *)buf);
	}
	return ret_val;
}

static
int
glfflush(FILE* fp, Bool firstTry)
{
	if (!firstTry)
	{
		/* Maybe the user has done a CTL-S, 
		** So try to restart the output
		*/
		int ioctlRetval = GLIOCTL(fileno(fp), TCXONC, 1);
		if (ioctlRetval != GLsuccess)
			return ioctlRetval;
	}
	GLsetAlarm(GLasyncGuardTime);

	int retval = ::fflush(fp);
	if (retval != 0)
	{
		if (GLclearAlarm() == 0 && errno == EINTR && firstTry)
		{
			CRDEBUG(CRusli, ("trying fflush again"));
			return glfflush(fp, NO);
 		}
		CRDEBUG(CRusli, ("glfflush failed with retval=%d, errno=%d",
				 retval, errno));
		return GLasyncFail(fileno(fp));
	}

	unsigned alarmRetVal = GLclearAlarm();
	if (retval != 0 && errno == EINTR && alarmRetVal == 0 && firstTry)
	{
		CRDEBUG(CRusli, ("trying fflush again"));
		return glfflush(fp, NO);
	}
	return GLasyncRet("GLfflush", retval, alarmRetVal, fileno(fp));
}

int
GLfflush(FILE* fp)
{
	return glfflush(fp, YES);
}

int
GLputchar(char c)
{
	int fd = fileno(stdout);
	int len = 1;
	GLsetAlarm(GLasyncGuardTime);
	int retval = ::write(fd, &c, len);
	if (retval != len)
	{
		GLclearAlarm();
		CRDEBUG(CRusli, (GLwriteErrFmt, retval, errno, len));
		return GLasyncFail(fd);
	}
	return GLasyncRet("GLputchar", retval, GLclearAlarm(), fd);
}
