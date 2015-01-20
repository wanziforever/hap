#ifndef __GLasync_H
#define __GLasync_H
/*
**      File ID:        @(#): <MID19516 () - 02/19/00, 23.1.1.1>
**
**	File:					MID19516
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:39:21
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**      This file contains the declarations of the async IO library.
**      These functions provide a more fault-tolerant version of the
**      standard "C" I/O functions to be used for I/O to asyncronous devices.
**      The standard "C" I/O functions tend to "hang" (never return) if
**      there is a hardware failure on the device being accessed.
**      The only way around the problem (currently) is to set an alarm
**      before the "C" I/O function is called.  If the function "hangs",
**      then when the alarm goes off the system call will be interrupted.
**      Each of the I/O functions (except the GLread) in this library
**      employs this little trick.  In some cases the hardware failure
**      does not cause the I/O function to "hang".  Instead, the I/O function
**      will return a failure and the global variable errno will be set
**      to some special value, such as EIO.  The functions in this library
**      also check for these types of failures.
**
**      If there is a hardware failure (either a "hang" case or otherwise),
**      the functions in library will returns a special return
**      code (GLasyncHwFailure).  If a function fails for a non-hardware
**      reason, a different value (GLasyncUNIXfailure) is returned and
**      the global variable 'errno' can be examined to determine the reason.
**
**      Functions (GLASYNCFAIL, GLASYNCHWFAIL, and GLASYNCUNIXFAIL) are 
**      provided to check for these different types of failures.
**
**      By default any of the I/O functions must complete within 5 seconds.
**      The function GLsetAsyncGuardTime() can be used to set this time
**      limit to something other than the defualt.  This is probably
**      necessary for slow devices (less than 9600 baud) or for
**      applications that output large amounts of data in a single write.
**
**      The other feature of this library is that the client of the
**      library can specify a function to be called when a hardware failure
**      occurs.
**      
** OWNER: 
**	Roger McKee
**
** NOTES:
**      Since these functions are primarily needed for the CC,
**      macros are provide a transparent means of calling the standard
**      I/O functions on the EES.   These macros should normally be used
**      instead of the corresponding "GL" functions.
*/

#include <stdio.h>

/* GLasyncErrHandler: typedef for a pointer to a function to call when a 
** hardware failure occurs
*/
typedef void (*GLasyncErrHandler)(int);

/* Error codes */
const int GLasyncHwFailure = -2;
const int GLasyncUNIXfailure = -1;

inline
int
GLASYNCFAIL(int rc)
{
	return rc < 0;
}

inline
int
GLASYNCHWFAIL(int rc)
{
	return rc == GLasyncHwFailure;
}

inline
int
GLASYNCUNIXFAIL(int rc)
{
	return rc == GLasyncUNIXfailure;
}

void GLsetAsyncErrHandler(GLasyncErrHandler);
void GLsetAsyncGuardTime(int seconds);
int GLnonblkRead(int fd, void* buf, int maxlen); /* non blocking read */

/* same as standard C I/O functions (without the "GL") */
int GLopen(const char* fname, int oflag, int mode=0666);
int GLfcntl(int tfd, int opcode, int fd);
int GLclose(int fd);
int GLioctl(int fd, int request, struct termio*);
#ifdef CC
int GLioctl(int fd, int request, struct termcb*);
#endif
int GLisatty(int fd);
int GLioctl(int fd, int request);
int GLioctl(int fd, int request, int arg);
int GLread(int fd, void* buf, int maxlen);
int GLwrite(int fd, const void* buf, int len);
int GLfputs(const char* str, FILE* fp);
int GLprintf(const char* format, ...);
int GLfflush(FILE* fp);
int GLputchar(char c);

/* CC version: call the "GL" functions */
#define GLCLOSE GLclose
#define GLOPEN GLopen
#define GLFCNTL GLfcntl
#define GLPRINTF GLprintf
#define GLFPUTS GLfputs
#define GLFFLUSH GLfflush
#define GLPUTCHAR GLputchar
#define GLIOCTL GLioctl
#define GLREAD GLread
#define GLWRITE GLwrite
#define GLISATTY GLisatty
#if 0
/* EES version: call the standard I/O functions */
#define GLCLOSE close
#define GLOPEN open
#define GLFCNTL fcntl
#define GLPRINTF printf
#define GLFPUTS fputs
#define GLFFLUSH fflush
#define GLPUTCHAR putchar
#define GLIOCTL ioctl
#define GLREAD read
#define GLWRITE write
#endif

#endif
