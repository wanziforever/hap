/*
**      File ID:        @(#): <MID21682 () - 7/12/96, 8.1.1.1>
**
**	File:					MID21682
**	Release:				8.1.1.1
**	Date:					7/12/96
**	Time:					20:34:59
**	Newest applied delta:	7/12/96
**
** DESCRIPTION:
** 	Definition of the general USLI application process functions.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**  Larry Kollasch 10/21/05 R25SU6 Change regName(MH_DEFAULT) to MH_LOCAL
**
*/

#include <stdlib.h>
#include <stdio.h>		
#include <errno.h>
#include <libc.h>
#include "cc/cr/hdr/CRmaintApp.H"
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHresult.H"
#include "cc/hdr/eh/EHreturns.H"
#include "cc/hdr/eh/EHhandler.H"
#include "cc/hdr/init/INmtype.H"
#include "cc/hdr/su/SUexitMsg.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRdbCmdMsg.H"
#include "cc/hdr/sci/SCstchg.H"
#include "cc/hdr/misc/GLhwstate.H"
#include "cc/hdr/cr/CRctRetVal.H"
#include "cc/hdr/cr/CRloadName.H"
#include "cc/cr/hdr/CRossServiceMsg.H"

EHhandler CRevent;	/* Event handler object */

CRmaintApplication* CRapplication = NULL;
int CRinvalidCmds = 0;

void
killAnyRunningCeps() 
{
	String tmpfname = tmpnam(NULL);

/* create file to trap stderr output from /bin/sh
** If the user tries to execute a non-existant command
** /bin/sh will print a message on stderr
*/
	FILE* stderrfp = freopen(tmpfname, "w", stderr);
	if (stderrfp == NULL)
	{
		return;
	}

	String cmdstr = "/bin/ps -ef";
	FILE* fp = popen(cmdstr, "r");
	if (fp == NULL)
	{
		pclose(fp);
		return;
	}
	fclose(stderrfp);
	const int maxLineLen = 255;
	static char linebuf[maxLineLen+1];
/*
**      Read output from the cmdstr (ps -ef)
*/
	char *tmpLine = 0;
	char *tmpField = 0;
	int pid = 0;
	int ppid = 0;
	char *proc1 = 0;
	char *proc2 = 0;
	int myPid = getpid();
	int fieldCnt = 0;

	while (fgets(linebuf, sizeof(linebuf)-1, fp))
	{
		tmpLine = linebuf;
		tmpField = strtok(tmpLine, " ");
		fieldCnt = 1;
		while(tmpField)
		{
			if(fieldCnt == 2)
				pid=atoi(tmpField);
			if(fieldCnt == 3)
				ppid=atoi(tmpField);
// because ps -ef could return these two
// differant lines we have to check field
// 8 and field 9.
// root 659 318 0 11:35:52 ?  0:00 in.telnetd
// root 414 390 0  Apr 06 ?   0:00 mibiisa -p 32962
			if(fieldCnt == 8)
				proc1=tmpField;
			if(fieldCnt == 9)
				proc2=tmpField;
			++fieldCnt;
			tmpField = strtok(NULL, " ");
		}
		if(myPid == ppid)
		{
//IBM JGH 05/09/06 warning: NULL used in arithmetic
			if(strncmp(proc1,"CEP",3)==0 ||
				strncmp(proc2,"CEP",3)==0)
			{
				kill(pid,SIGKILL);
			}
		}
	}
	pclose(fp);
}


extern "C" void
CRcleanUpFunc(int sig)
{

	String name=CRapplication->getMsghName();
	MHqid mhqid;

        signal(sig, SIG_IGN);
        MHmsgh.getMhqid(name,mhqid);
        MHmsgh.rmName(mhqid, name);
        MHmsgh.detach();

        CRDEBUG(CRusli, ("signal %d received", sig));
        cleanup(0, NULL, SN_NOINIT, 0);

	killAnyRunningCeps();
        exit(0);
}

CRmaintApplication::CRmaintApplication(const char* msghName, 
				       const char* indicator,
				       key_t shrMemKey) : 
        CRasyncSCH(msghName, indicator, shrMemKey)
{
CRDEBUG(CRusli, ("Enter CRmaintApplication %s", (const char*) msghName));
	mhqid = MHnullQ;
	//
	// This guy keeps a count of NG from the MML command line
	// for SCC/LMT/RMT/subshl, if the count is greater then
	// MAX_NUMBER_OF_BAD_INPUTS then the process dies
	//
	numberOfBadInputs = 0;
}

CRmaintApplication::~CRmaintApplication()
{
}

CRmsgResult
CRmaintApplication::processMsg(GLretVal rcvRetVal, char* msg_buf, int msgsz)
{
	String name=CRapplication->getMsghName();
	CRDEBUG(CRusli+8, ("Enter processMsg(%d)", rcvRetVal));

	switch (rcvRetVal)
	{
	    case GLsuccess: /* received a message */
		switch (((MHmsgBase*) msg_buf)->msgType)
		{
		    case SUexitTyp: /* field update message */
	                /*{
				char* tmpargv[1];
				::cleanup(0, tmpargv, SN_LV0, 0);
			}*/
			exit(0);

				case SUversionTyp:	/* field update new version message */
			CRcurrentLoad.init();
			break;

		    case INmissedSanTyp:
			IN_SANPEG();
			break;

		    case INpDeathTyp: /* ignore msg */
			break;

		    case SCstchgMsg:
		      {
			SCstchg* stchgMsg = (SCstchg*) msg_buf;

			CRDEBUG(CRusli+CRmsginFlg,
				("SCstchgMsg(%s,%s) rcved",
				 GLstateName(stchgMsg->state),
				 GLreasonName(stchgMsg->reason)));

			/* Look inside msg to figure out if the port
			** is ACTIVE or OOS.
			*/
			if (stchgMsg->state == ACTIVE)
			{
				if (channelState.state() == CRdownState || channelState.state() == CRhwDown || channelState.state() == CRloopBack)

        			INITREQ(SN_LV0, GLsuccess, "ASYNC PORT CLEANUP", IN_EXPECTED);
			}
			else
			{
				isOKtoWrite(NO);
				channelState.state(CRdownState);
				CRasyncSCH::reportOOS();
			}
		      } /* end block */
			break;

		    case CRdbCmdMsgTyp:
			if (msgsz != sizeof(CRdbCmdMsg))
			{
				CRERROR("received message of size %d, not size %d",
					msgsz, sizeof(CRdbCmdMsg));
				break;
			}
			((CRdbCmdMsg*) msg_buf)->unload();
			break;

		    case CRserviceRemoveMsgTyp:
			CRERROR("received message from OSMON to exit");
			fatalExit();


		    default: /* send all other messages to asyncSCH */
			return CRasyncSCH::processMsg(rcvRetVal, msg_buf,
						      msgsz);
		}
		return CRmsgAccepted;

	    case MHnoMsg:	/* no messages */
		CRDEBUG(CRusli+CRmsginFlg, ("no msg received"));
		return CRmsgAccepted;
		
	    case MHtimeOut: /* time out (no messages) */
		CRDEBUG(CRusli+CRmsginFlg, ("time out, no msg received"));
		return CRmsgAccepted;

	    case MHintr:
		CRDEBUG(CRusli+CRmsginFlg, ("interrupt, no msg received"));
		return CRmsgAccepted;

	    case EHNOTMREXP:
#ifdef EES
		return CRmsgAccepted;
#else
		CRERROR("getEvent() failed with return code EHNOTMREXP");
		fatalExit();
#endif

	    case EHNOEVENTNB:
		CRDEBUG(CRusli+8, ("no event"));
		return CRmsgAccepted;

	    case MHidRm:
		CRDEBUG(CRusli+CRmsginFlg,
			("getEvent() failed with return code (%d)",
			 rcvRetVal));
		fatalExit();

	    case MHbadQid:
	    case MHfault:
	    case MHnoQue:
	    case MHnoShm:
		CRERROR("getEvent() failed with return code (%d)", rcvRetVal);
		fatalExit();

	    default:
		CRERROR("getEvent() fail! rtn=%d\n", rcvRetVal);
		fatalExit(); /* does not return */
	}
	return CRmsgUnknown;
}

/* the following function counts the consective invalid attempts and 
* figures out if there is a loop back connection
*/
void 
chkLoopBack(CRctRetVal ret)
{
	if (ret == CRSUCCESS)
	{
		CRinvalidCmds = 0;
	}
	else
	{
		if (CRinvalidCmds == 100)
		{
			CRinvalidCmds++;
			CRapplication->loopBack();
		}
		else
			CRinvalidCmds++;
	}
}

/* this function reports the loop back connection to the craft */
void 
CRmaintApplication::loopBack()
{
	CRDEBUG(CRusli+2, ("CRmaintApplication::loopBack()"));
	CRasyncSCH::rptLoopBack();
	CRDEBUG(CRusli+2, ("After CRmaintApplication::loopBack()"));
}

/*
** This routine is called when the system is booted or when the system goes
** through a system reset.  This process doesn't contrl any system resources
** which are shared between processes in the CC.  Therefore there is no
** SYSINIT work to be done:
*/

short
sysinit(int /*argc*/, Char *argv[], 
	SN_LVL init_lvl, U_char run_lvl)
{
	char result[20];

	if((strncmp(argv[0],"sccci",5))==0 )
	{
		sprintf(result, "%s%d", "sccci", (int) getpid());

		CRapplication = CRmakeApplication(result);
		CRERRINIT(CRapplication->getMsghName());
	}
	else
	{
		CRapplication = CRmakeApplication(argv[0]);
		CRERRINIT(CRapplication->getMsghName());
	}

	CRDEBUG(CRusli, ("sysinit %d,%d,", init_lvl, run_lvl));
	return CRapplication->sysinit();
}

/*
** This routine is called during process initialization.  This routine is used
** to attempt a recovery from a problem that is local to this process
*/

short
procinit(int argc, Char *argv[],
	 SN_LVL init_lvl, U_char run_lvl)
{
	CRDEBUG(CRusli+CRinitFlg, ("argv[0] = %s",argv[0]));
	if (CRapplication == NULL)
		CRapplication = CRmakeApplication(argv[0]);

	CRERRINIT(CRapplication->getMsghName());
	CRDEBUG(CRusli+CRinitFlg, ("enter procinit: init_lvl=%d, run_lvl=%d",
				     init_lvl, run_lvl));
	return CRapplication->procinit(argc, argv, init_lvl, run_lvl);
}

/*
** This routine is used to remove the MSGH queue resources (but not our MSGH
** queue name), detach from the MSGH shared memory segment
*/

Short 
cleanup(int /*argc*/, Char */*argv*/[],
	SN_LVL /*init_lvl*/, U_char /*run_lvl*/)
{

	if (CRapplication)
	{
		CRapplication->cleanup();
		delete CRapplication; CRapplication = NULL;
	}

	return GLfail; /* force INIT to kill and restart the process */
}

void 
process(int /* argc */, char* /*argv*/[], SN_LVL init_lvl, U_char run_lvl)
{
	String name=CRapplication->getMsghName();
	CRDEBUG(CRusli+CRinitFlg, ("enter process: init_lvl=%d, run_lvl=%d",
				     init_lvl, run_lvl));

	if (init_lvl != SN_NOINIT)
		CRapplication->setSignals();

	CRapplication->process(init_lvl);
}

/* this should be a virtual function */
void
CRmaintApplication::setSignals()
{
	signal(SIGCLD, SIG_IGN);

	/* ignore SIGINT. This causes some interactive CEPs to ignore the
 	** interrupt character
	*/
	signal(SIGINT, SIG_IGN);

	/* Ignore SIGQUIT. This causes some interactive CEPs to ignore the
	** quit character.
	*/
	signal(SIGQUIT, SIG_IGN);

	/*
	**	set SIGHUP for when termpory process dies
	*/
	signal(SIGHUP, CRcleanUpFunc);

}

GLretVal
CRmaintApplication::procinit(int argc, char* argv[],
			     SN_LVL init_lvl, U_char run_lvl)
{
	CRDEBUG(CRusli, ("CRmaintApplication:procinit argv[0] = %s",argv[0]));
	/* attach to the MSGH subsystem */
	GLretVal retval = CRevent.attach();
	if (retval != GLsuccess)
	{
                CRERROR("attach to MSGH failed with error code %d\n",
			  retval);
                return GLfail;
        }

	Bool isCondFlag = (isPermProcess() == YES) ? FALSE : TRUE;

        /* register process name and get a MSGH queue */
        retval = CRevent.regName(getMsghName(), mhqid, isCondFlag, FALSE, FALSE, MH_LOCAL);
        if (retval != GLsuccess)
	{
                CRERROR("regName() failed; name=%s retval=%d\n",
			getMsghName(), retval);
                return retval;
        }

	/* initialize timer library */
	if ((retval = CRevent.tmrInit()) != GLsuccess)
		return retval;

	return CRasyncSCH::procinit(argc, argv, init_lvl, run_lvl);
}

static char CRmsgBuf[MHmsgLimit];

void
CRmaintApplication::waitForNextEvent()
{
        if(MHmsgh.detach() == GLsuccess)
	{
		if (CRevent.attach() != GLsuccess)
		{
			CRcleanUpFunc(0);
                	exit(0);
		}
        }

	/* wait for an event */
	short msgsz= sizeof(CRmsgBuf);
	GLretVal rcvRtn = CRevent.getEvent(mhqid, CRmsgBuf, msgsz);
	processMsg(rcvRtn, CRmsgBuf, msgsz);
}

void
CRmaintApplication::processPendingEvents(EHEVTYPE evtype)
{
	GLretVal rcvRtn;
	String name=CRapplication->getMsghName();

	do
	{
		short msgsz= sizeof(CRmsgBuf);
		rcvRtn = CRevent.getEvent(mhqid, CRmsgBuf, msgsz,
					  0, FALSE, evtype);
		processMsg(rcvRtn, CRmsgBuf, msgsz);
	} while (rcvRtn == GLsuccess);
}

void
CRmaintApplication::shellEscape()
{
	CRERROR("default shellEscape() called");
}

void
CRmaintApplication::shellReturn()
{
	CRERROR("default shellReturn() called");
}

void
CRmaintApplication::uneqState()
{
	CRasyncSCH::isOKtoWrite(NO);
	waitForNextEvent();
}

void
CRmaintApplication::downState()
{
	CRasyncSCH::isOKtoWrite(NO);
	waitForNextEvent();
}

void
CRmaintApplication::initState()
{
	waitForNextEvent();
}

void
CRmaintApplication::reInitState()
{
	waitForNextEvent();
}

void
CRmaintApplication::idleState()
{
	waitForNextEvent();
}

void
CRmaintApplication::normalPromptState()
{
	waitForNextEvent();
}

void
CRmaintApplication::outputState()
{
	waitForNextEvent();
}

void
CRmaintApplication::normalInputState()
{
	waitForNextEvent();
}

void
CRmaintApplication::interruptResponseState()
{
	waitForNextEvent();
}

void
CRmaintApplication::overridePromptState()
{
	waitForNextEvent();
}

void
CRmaintApplication::overrideInputState()
{
	waitForNextEvent();
}

void
CRmaintApplication::hwDownState()
{
	CRasyncSCH::isOKtoWrite(NO);
	waitForNextEvent();
}

void
CRmaintApplication::dapInputState()
{
	waitForNextEvent();
}

void
CRmaintApplication::process(SN_LVL init_lvl)
{
	String name=CRapplication->getMsghName();
	if (init_lvl != SN_NOINIT)
	{
		channelState.state(CRinitState);
		processPendingEvents(EHMSGONLY);
		if (channelState.state() != CRdownState)
		{
			doTTYsettings();
			if (initDevice() != GLsuccess)
			{
				CRDEBUG(CRusli, ("initDevice failed"));
			}
		}
	}
	if (process() == NO)
		fatalExit();
}

Bool
CRmaintApplication::process()
{
	String name=CRapplication->getMsghName();
	/* step 1: check for pending events */

	processPendingEvents();
  CRDEBUG(CRusli+3, ("CRmaintApp::process() channelState.state() = %d",
                      channelState.state()));
	switch (channelState.state())
	{
	    case CRuneqState:
		uneqState();
		break;
	    case CRdapInput:
		dapInputState();
		break;
	    case CRhwDown:
		hwDownState();
		break;
	    case CRloopBack:	
	    case CRdownState:
		downState();
		break;
	    case CRinitState:
		initState();
		break;
	    case CRreInit:
		reInitState();
		break;
	    case CRidle:
		idleState();
		break;
	    case CRnormalPrompt:
		normalPromptState();
		break;
	    case CRoutput:
		outputState();
		break;
	    case CRnormalInput:
		normalInputState();
		break;
	    case CRinterruptResponse:
		interruptResponseState();
		break;
	    case CRoverridePrompt:
		overridePromptState();
		break;
	    case CRoverrideInput:
		overrideInputState();
		break;
	    case CRexit:
		return FALSE;
	}
	return TRUE;
}

GLretVal
CRmaintApplication::cleanup()
{
	CRasyncSCH::cleanup();
	return GLsuccess;
}
