//#include <sys/trap.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
//#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/init/INsbt.hh"

extern	"C"	void	INsignal_abort( int );
extern	"C"	void	INsignal_exit( int );

//CRmsg			INsbt_msg( CL_DEBUG, POA_INF );

void
INsignal_abort( int sig )
{
	//CRERROR( "signal %d, aborting", sig );
  printf("signal %d, aborting", sig);
	INstackBackTrace();
	signal( sig, SIG_DFL );

	/*
	 *	We are counting on UNIX attempting to re-execute
	 *	the machine instruction that caused the core dump
	 *	to make it happen again so we generate a core file.
	 *
	 *	We do this instead of calling abort(3).  Since dbx(1)
	 *	can not handle printing a stack back trace when a
	 *	signal frame is on the stack.
	 */
	return;
} /* INsignal_abort() */

void
INsignal_exit( int sig )
{
	//CRERROR( "signal %d, exiting", sig );
  printf("signal %d, exiting", sig);
	INstackBackTrace();
	exit( 1 );
	/* NOTREACHED */
} /* INsignal_exit() */

//void
//INsbt_dump()
//{
//	register const char *	s;
//	FILE *		file;
//	char		filename[ 30 ];
//
//	register ADDRESS start_text = START_TEXT;
//	register ADDRESS end_text = END_TEXT;
//	register ADDRESS start_data = START_DATA;
//	register ADDRESS end_data = END_DATA;
//	register ADDRESS start_stack = START_STACK;
//	register ADDRESS end_stack = END_STACK;
//
//	static	int	version;
//	sprintf( filename, "stack.%d.%d", getpid(), version++ );
//	if( (file = fopen( filename, "w" )) == NULL ) {
//		return;
//	}
//
//	register unsigned long	value;
//
//	for( ADDRESS a = start_stack; a < end_stack; a += 4 ) {
//		value = * (unsigned long *) a;
//		if( value >= start_text && value <= end_text ) {
//			s = "text";
//		} else if( value >= start_data && value <= end_data ) {
//			s = "data";
//		} else if( value >= start_stack && value <= end_stack ) {
//			s = "stack";
//		} else {
//			s = 0;
//		}
//
//		fprintf( file, "%8.8x	%8.8x", a, value );
//		if( s != 0 ) {
//			fprintf( file, "\t%s", s );
//		}
//		fprintf( file, "\n" );
//	} /* for */
//
//	fclose( file );
//} /* INsbt_dump() */
//
//#define MIN_MARKER	2
//#define MAX_MARKER	60
//
///************************************************************************
// *									*
// *	INsbt.c:							*
// *									*
// *	Implementation of INstackBackTrace():				*
// *									*
// *	This code is invoked only for debugging purposes, or as the 	*
// *	result of an internal error.  It is never used during normal	*
// *	processing.							*
// *									*
// ***********************************************************************/
//
//void
//INsave_stackBackTrace( IN_StackFrameInfo * info, int limit, struct siginfo * siginfo, struct ucontext * ucontext )
//{
//	MARK(INsave_stackBackTrace)
//
//	/* not implemented */
//
//	/* flush the SPARC windows to memory on the stack */
//	asm("	t	0x3   ! ST_FLUSH_WINDOWS");
//
//	if( siginfo != 0 ) {
//		memset( (char *) siginfo, '\0', sizeof( struct siginfo ) );
//	}
//
//	if( ucontext != 0 ) {
//		memset( (char *) ucontext, '\0', sizeof( struct ucontext ) );
//	}
//
//	memset( (char *) info, '\0', sizeof(IN_StackFrameInfo) * limit );
//} /* INsave_stackBackTrace() */
//
///*
// *	INprint_stackBackTrace()
// *
// *	This should not be called from a signal handler.
// */
//
//void
//INprint_stackBackTrace( IN_StackFrameInfo * info, int limit, struct siginfo * siginfo, struct ucontext * ucontext )
//{
//	MARK(INprint_stackBackTrace)
//	int	saw_main = 0;
//
//	INsbt_msg.add( "REPT ERROR LOG %s\n", CRprocname );
//
//	/*
//	 *	SUN 4 (SPARC) stack frame:
//	 *
//	 *
//	 */
//
//	INsbt_msg.add( "Stack Backtrace:  Sorry, not available on SPARC\n\n" );
//
//	saw_main = 1;
//	INsbt_msg.spool("\n");
//
//	if( CRtraceFlags.isBitSet(574) == YES || ! saw_main ) {
//		INsbt_dump();
//	} /* if */
//
//} /* INprint_stackBackTrace() */
//
/*
 *	INstackBackTrace()
 */

#define  LIMIT 100

void
INstackBackTrace()
{
//	MARK(INstackBackTrace)
//	IN_StackFrameInfo	info[ LIMIT ];
//	struct siginfo		siginfo;
//	struct ucontext		ucontext;
//
//	INsave_stackBackTrace( info, LIMIT, & siginfo, & ucontext );
//	INprint_stackBackTrace( info, LIMIT, & siginfo, & ucontext );
} /* INstackBackTrace() */

/*
 *	INsbt__func__()
 *
 *	Find the name of the function 'levels' stack frames
 *	up the stack.
 */

const char *
INsbt__func__( int )
{
	MARK(INsbt__func__)

	return "unknown";
} /* INsbt__func__() */
