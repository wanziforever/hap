/*
**      File ID:        @(#): <MID17564 () - 03/01/02, 1.1.1.18>
**
**	File:					MID17564
**	Release:				1.1.1.18
**	Date:					10/06/03
**	Time:					11:27:08
**	Newest applied delta:	03/01/02
**
** DESCRIPTION:
**	This file contains the functions needed by INIT and the main routine 
**	for the RPROC process.
** OWNER: 
**	Yash Gupta
**      Roger McKee
** NOTES:
**
*/
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <libgen.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/msgh/MHnames.H"
#include "cc/hdr/msgh/MHinfoExt.H"
#include "cc/hdr/msgh/MHmsgBase.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/cr/CRdbCmdMsg.H"
#include "cc/cr/hdr/CRrpInitMsg.H"
#include "CRrproc.H"

static MHqid CRcproc_qid = MHnullQ;
static MHqid CRrproc_qid = MHnullQ;
static char CRprocessName[MHmaxNameLen+1] = "";
char CRcprocName[MHmaxNameLen+1] = "LMT";
static CRrproc CRread_proc;

/*	Called when INIT has requested the RPROC process to get its things
**	in order. There is a possible shutdown/kill in the future.
**
**	It's here to meet INIT standards.
*/
Short 
cleanup(int , Char *[], SN_LVL init_lvl, U_char run_lvl)
{
	CRDEBUG(CRusli+CRinitFlg, ("enter cleanup: init_lvl=%d, run_lvl=%d",
				    init_lvl, run_lvl));
	MHmsgh.rmName(CRrproc_qid, CRprocessName);

	return GLsuccess;
} /* cleanup */

static
void
CRgracefulExit()
{
	CRDEBUG(CRusli, ("RPROC is exiting"));
	exit(cleanup(1, NULL, SN_NOINIT, 0));
}

/*
** name: CRprocessMsgs
**
** description: process MSGH messages
**              Will loop over MSGH messages until the special initialization
**              message from CPROC is encountered.  After processing this msg
**              the function will return.  Assumes that all msgs after this
**              one will be ignored.
**
** Assumptions:
**              1. process is attached to MSGH
**              2. receive message queue exists.
**                  a. queue id stored in rproc_qid
**                  b. queue name stored in PROCESS_NAME
*/
void
CRprocessMsgs(int argc, char* argv[])
{
	static char msg_buf[MHmsgSz];

	for(;;)
	{
		/* Check for message from CPROC processes */
		short msg_sz = MHmsgSz;
		GLretVal rtn = MHmsgh.receive(CRrproc_qid, msg_buf,
					      msg_sz, 0, -1);
		switch (rtn)
		{
		    case GLsuccess: 	/* received a message */
			switch (((MHmsgBase*) msg_buf)->msgType)
			{
			    case CRrpInitMsgTyp:
				CRDEBUG(CRusli+CRmsginFlg,
					("CRrpInitmsgTyp received"));
			
				if (msg_sz != CRrpInitMsgSz)
				{
					CRERROR("received message of size %d, not size %d",
						msg_sz, sizeof(CRrpInitMsg));
					continue;
				}

			      { /* variable block */
				CRrpInitMsg *msgPtr = (CRrpInitMsg *)msg_buf;
				if (CRread_proc.init(msgPtr->getDevName()) != GLsuccess)
				{
					/* if fails to initialize
					** then exit
					*/
					CRDEBUG(CRusli, ("RPROC failed to initialize device ... exiting"));
						
					exit(cleanup(argc, argv, SN_NOINIT, 0));
				}
			      } /* end variable block */
				return; /* done waiting for messages */

			    case CRdbCmdMsgTyp:
				if (msg_sz != sizeof(CRdbCmdMsg))
				{
					CRERROR("received message of size %d, not size %d",
						msg_sz, sizeof(CRdbCmdMsg));
					continue;
				}

				((CRdbCmdMsg*) msg_buf)->unload();
				break;

			    default: 	/* ignore all other messages */
				CRDEBUG(CRusli+CRmsginFlg,
					("msg received type %x",
					 ((MHmsgBase*) msg_buf)->msgType));
				break;
			}
			break;
		    case MHnoMsg:	/* no messages */
			CRDEBUG(CRusli+CRmsginFlg, ("no msg received"));
			break;
		    case MHtimeOut: /* time out (no messages) */
			CRDEBUG(CRusli+CRmsginFlg,
				("time out, no msg received"));
			break;
		    case MHintr:
			CRDEBUG(CRusli+CRmsginFlg,
				("interrupt, no msg received"));
			break;
		    default:
			CRDEBUG(CRusli+CRmsginFlg,
				("MHmsgh.receive() fail! rtn=%d", rtn));
			CRgracefulExit();
		}
	}/* for loop */
}

//IBM JGH 20060629 remove extern "C"
// old code
// extern "C"
// end modification
static
void
CRdef_handler(int sig)
{
	signal(sig, SIG_IGN);
	CRDEBUG(CRusli, ("def_handler(%d) exiting!", sig));
	CRgracefulExit();
}

static 
void
CRset_signals()
{
	/* Set the signal settings here.
	** SIGHUP is set to the default treatment, 
	** so this process (the RPROC) will die if the parent process (LMT)
	** dies.
	*/
	signal(SIGHUP, SIG_DFL);
 	signal(SIGUSR1, CRdef_handler);
	signal(SIGTERM, CRdef_handler);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, CRdef_handler);
} 

/*	Called when the system boots or goes through a system
**	reinitialization.
**
**	Nothing to be done. It's here to meet INIT standards.
*/
GLretVal
sysinit(int , char *argv[], SN_LVL init_lvl, U_char run_lvl)
{
	CRERRINIT(basename(argv[0]));
	CRDEBUG(CRusli+CRinitFlg,
		("enter sysinit: init_lvl=%d, run_lvl=%d", init_lvl, run_lvl));
        return GLsuccess;
}

/*	Called during process initialization or when the process
**	loses sanity.
**
**	It's here to meet INIT standards.
*/
GLretVal
procinit(int argc, char *argv[], SN_LVL init_lvl, U_char run_lvl)
{
	CRERRINIT(basename(argv[0]));
	CRDEBUG(CRusli+CRinitFlg, ("enter procinit: init_lvl=%d, run_lvl=%d",
		 init_lvl, run_lvl));

	strcpy(CRprocessName, argv[0]);

	if (argc == 2)
		strcpy(CRcprocName, argv[1]);
		
	GLretVal ret_val;

	if ((ret_val = MHmsgh.attach()) != GLsuccess)
                return GLfail;

        if ((ret_val = MHmsgh.regName(CRprocessName, CRrproc_qid, FALSE)) != GLsuccess)
                return GLfail;

	CRset_signals();

	/* Send a message to CPROC process requesting device-specific 
	** information.
	*/

	CRrpInitMsg initMsg;
	if (initMsg.sendToCproc(CRcprocName) != GLsuccess)
	{
		CRERROR("Unable to send initialization message to %s process",
			CRcprocName);
		return GLfail;
	}

	return ret_val;
}

/*	Name:	process
**	Function:
**		The main routine of the RPROC process.
**	Called By:
**		INIT support routines.
*/
void 
process(int argc, Char* argv[], SN_LVL init_lvl, U_char run_lvl)
{
	CRDEBUG(CRusli+CRinitFlg,
		("enter process: init_lvl=%d, run_lvl=%d", init_lvl, run_lvl));

	CRprocessMsgs(argc, argv);

	/* remove message queue */
	MHmsgh.rmName(CRrproc_qid, CRprocessName);

	/* process input */
	while (CRread_proc.processInput() == GLsuccess)
		; /* no op */
}

int
main(int argc, char *argv[])
{
	sysinit(argc, argv, SN_NOINIT, 0);
	
	if (procinit(argc, argv, SN_NOINIT, 0) == GLsuccess)
	{
		process(argc, argv, SN_NOINIT, 0);
	}

	CRgracefulExit();
	return(0);
}
