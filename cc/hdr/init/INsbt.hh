#ifndef __INsbt_H
#define __INsbt_H

/*
**	File ID: 	@(#): <MID19743 () - 08/17/02, 29.1.1.1>
**
**	File:			MID19743
**	Release:		29.1.1.1
**	Date:			08/21/02
**	Time:			19:32:55
**	Newest applied delta:	08/17/02 04:33:11
**
**	Description:
**		This file contains the declarations for the
**		stack backtrace code.
*/

#include <signal.h>

extern	void	INstackBackTrace();
extern	void	INinit_signal();
extern	void	INsbt_dump();

typedef unsigned long	ADDRESS;

extern	"C" char * *	environ;
extern	"C" int etext;

struct IN_StackFrameInfo {
	const char *	function_name;
	long		start_address;
	long		address;
};

#include <ucontext.h>

#include <bits/siginfo.h>

extern 	void	INsave_stackBackTrace( IN_StackFrameInfo *, int, struct siginfo *, struct ucontext * );
extern 	void	INprint_stackBackTrace( IN_StackFrameInfo *, int, struct siginfo *, struct ucontext * );

#ifdef mips

extern	const char *	INsbt__func__( int );

#define MARK( string )
#define __FUNC__	INsbt__func__( 1 )

extern	_ftext;
extern	_fdata;

#define START_TEXT	((ADDRESS) & _ftext)
#define START_DATA	((ADDRESS) & _fdata)
#define START_STACK	((ADDRESS) & start_stack)
#define END_TEXT	((ADDRESS) & etext)
#define END_DATA	((ADDRESS) sbrk(0))
#define END_STACK	((ADDRESS) * environ)

#else

#define MARK( string )	const char *	__mark = "%:" #string;
#define __FUNC__	(&__mark[2])

#define START_TEXT	((ADDRESS) 0x2000)
#define START_DATA	((ADDRESS) & etext)
#define START_STACK	((ADDRESS) & start_stack)
#define END_TEXT	((ADDRESS) & etext)
#define END_DATA	((ADDRESS) sbrk(0))
#define END_STACK	((ADDRESS) * environ)

#endif

#endif /* __INsbt_H */
