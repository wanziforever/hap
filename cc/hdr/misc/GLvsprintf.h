#ifndef __GLvsprintf_h
#define __GLvsprintf_h
/*
**      File ID:        @(#): <MID41908 () - 02/19/00, 9.1.1.1>
**
**	File:			MID41908
**	Release:		9.1.1.1
**	Date:			05/13/00
**	Time:			13:39:25
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**      This file contains declarations for two 'sprintf()' wrappers;
**	the two entry points are 'GLsprintf()' and 'GLvsprintf()'.
**	Instead of passing a buffer to the wrappers,
**	they return a 'malloc()'ed buffer to us.
**	The caller is responsible for 'free()'ing the return buffer.
**
**	The final routine, GLvstrlen() returns the over-estimated length
**	of the resulting formatted string. (For those routines who want
**	to use their own buffer, if they can.)
*/


#ifdef  __cplusplus
extern "C" {
#endif

char * GLsprintf (const char *format, ...);
char * GLvsprintf(const char *format, va_list va);
int    GLvstrlen(const char *format, va_list va);

#ifdef  __cplusplus
}
#endif
#endif
