#ifndef TS
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

//#include "cc/hdr/cr/CRdebugMsg.H"
//#include "cc/hdr/cr/CRmsg.H"
#include "cc/hdr/init/INsbt.hh"

extern "C" typedef void	(* IN_SIGNAL_HANDLER )( int );

	// These functions are defined in another file
	// so we can lie to C++ about their arguments.
extern	"C"	void	INsignal_abort( int );
extern	"C"	void	INsignal_exit( int );

/*
 *	Only include those signals that UNIX will retry.
 *	On the MIPS and SUN4, the debuggers do not handle
 *	having a signal on the frame.  So when we want to
 *	cause a core dump (instead of calling abort(3)) we
 *	return from the signal handler which should cause
 *	UNIX to retry the instruction that caused the signal.
 */

struct INsignal_handlers {
	int			signal;
	IN_SIGNAL_HANDLER	signal_handler;
} INsignalHandlers[] = {
	{ SIGILL, INsignal_abort },
	{ SIGBUS, INsignal_abort },
	{ SIGSEGV, INsignal_abort },
	{ SIGSYS, INsignal_abort },

	{ SIGTERM, INsignal_exit },

	{ 0, 0 }
};

void
INinit_signal()
{
	register struct INsignal_handlers * p;
	
	/* CRinit + 60 is the flag bit that turns off the traceback */
	//if( CRtraceFlags.isBitSet(CRinit +60) == YES ) {
	//	CRERROR( "Signals not being trapped." );
	//	return;
	//} /* if */

	for( p = INsignalHandlers; p->signal != 0; p++ ) {
		signal( p->signal, p->signal_handler );
	}


} /* INinit_signal() */
#endif
