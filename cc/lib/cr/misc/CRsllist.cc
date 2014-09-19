/*
**      File ID:        @(#): <MID21683 () - 09/06/02, 28.1.1.1>
**
**	File:					MID21683
**	Release:				28.1.1.1
**	Date:					09/12/02
**	Time:					10:39:14
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**      This file defines the class CRasyncSCH which provides many of
**      functions needed to control an asynchronous port. 
**      This class assumes that stdin,stdout, and stderr will be directed
**      to a single async port
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

//IBM JGH 20060619 add include file contains memset
#include <string.h>
#include "cc/cr/hdr/CRasyncSCH.H"
#include <stdlib.h>
#include <signal.h>
#include <termio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRomdbMsg.H"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/init/INmtype.H"
#include "cc/hdr/bill/BLmtype.H"
#include "cc/hdr/tim/TMmtype.H"
#include "cc/hdr/tim/TMtmrExp.H"
#include "cc/hdr/eh/EHhandler.H"
#include "cc/hdr/eh/EHreturns.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/misc/GLasync.H"
#include "cc/cr/hdr/CRredirect.H"
#include "cc/cr/hdr/CRtimers.H"
#include "cc/cr/hdr/CRgetDevice.H"
#include "cc/hdr/cr/CRindStatus.H"
#include "cc/hdr/cr/CRsysError.H"
#include "cc/cr/hdr/CRropMsg.H"
#include "cc/hdr/init/INsharedMem.H"
#include "cc/cr/hdr/CRossServiceMsg.H"
#include <sys/procfs.h>

#include <utmpx.h>	/* Needed for OSMON handshake */

extern EHhandler CRevent;

const int STDIN_FD = 0;
const int STDOUT_FD = 1;
const int STDERR_FD = 2;

extern GLretVal CRchklineStatus(char *);

CRdevName CRasyncSCH::devName = "";

extern "C"
void
CRhupfunc(int /*sig*/)
{
	CRDEBUG(CRusli+4, ("LMT CRhupfunc()"));
	CRasyncSCH::HUPfunc();
}

void
CRhwFailFunc(int /*fd*/)
{
	CRDEBUG(CRusli+1, ("LMT CRhwFailFunc()"));
	CRasyncSCH::hwFailFunc();
}

void
CRasyncSCH::hwFailFunc()
{
	CRDEBUG(CRusli+1, ("LMT CRasyncSCH::CRhwFailFunc()"));
	if (asyncPtr)
		asyncPtr->hwFail();
	gracefulExit();
}

extern "C"
void
CRabortfunc(int sig)
{
	signal(sig, SIG_IGN);
	CRDEBUG(CRusli+1, ("LMT signal %d received", sig));
	CRasyncSCH::fatalExit();
}

extern "C"
void
CRexitHang(int /*sig*/)
{
}

CRasyncSCH* CRasyncSCH::asyncPtr = NULL;

CRasyncSCH::CRasyncSCH(const char* msghName, 
		       const char* indicator,
		       key_t shrMemKey) :
	sharedMemoryKey(shrMemKey),
	indName(indicator)
{
	CRDEBUG(CRusli+1, ("LMT CRasyncSCH "));
	asyncPtr = this;

	if (msghName == NULL)
	{
		/* subshl case */
		permProcessFlag = NO;
	}
	else
	{
		permProcessFlag = YES;


	/*
	**	For the lmt(OS RMT) process we DON'T
	**	what to make them the process group leader.
	**
	**	We want to keep it in the same process group as
	**	the /sn/cr/rmtshl shell that start this.
	*/

	if((strncmp(permProcessName,"lmt",3))!=0 )
	{
		setpgrp();
		IN_CLRIGNPDEATH();
	}

		setMsghName(msghName);
	}

	initRetryTmrBlock = -1;
#if 0
	startUpTmrBlock = -1;
#endif
	isOKtoWrite(NO);
	channelState.state(CRinitState);
	sharedMemoryPtr = (CRasyncSharedMemory*) -1;
}

void
CRasyncSCH::setMsghName(const char* msghName)
{
	CRDEBUG(CRusli+1, ("LMT setMsghName "));
	strcpy(permProcessName, msghName);
}

void
CRasyncSCH::setSignals()
{
	CRDEBUG(CRusli+1, ("LMT setsetSignals "));
	signal(SIGHUP, CRhupfunc);
	signal(SIGTERM, CRabortfunc);
}

void
CRasyncSCH::cleanUpPort()
{
	CRDEBUG(CRusli+1, ("LMT cleanUpPort "));

        struct termio termio;
	if (GLASYNCFAIL(GLIOCTL(STDIN_FD, TCGETA, &termio)))
	{
                return;
        }
        termio.c_cflag &= ~CBAUD;
        termio.c_cflag |= B0;

	if (GLASYNCFAIL(GLIOCTL(STDIN_FD, TCSETA, &termio)))
	{
                return;
        }

        /*
              * After hang_up_line, the stream is in STRHUP state.
                 * We need to do another open to reinitialize streams
                 * then we can close one fd
        */
	int tmpfd;
#ifdef CC
	CRDEBUG(CRusli+1, ("LMT cleanUpPort seting devName = %d",
		(const char*) devName));
	CRDEBUG(CRusli+1, ("LMT to O_RDWR|NONBLOCK"));
        if ((tmpfd=open(devName, O_RDWR|O_NONBLOCK)) == -1 ) {
#else
	CRDEBUG(CRusli+1, ("LMT cleanUpPort seting devName = %d",
		(const char*) devName));
	CRDEBUG(CRusli+1, ("LMT to O_RDWR|NDELAY"));

        if ((tmpfd=open(devName, O_RDWR|O_NDELAY)) == -1 ) {
#endif
//IBM JGH 20060619 take away void
// new code
                 (void) fclose((int)STDIN_FD);
// old code
//                  (void)fclose(STDIN_FD);
// end modification
        }
	else
               	(void)close(tmpfd);
}

void
CRexitFunc(int /* exitValue */)
{
	CRDEBUG(CRusli+1, ("LMT CRexitFunc "));
	exit(0);
}

void
CRasyncSCH::gracefulExit()
{
	CRDEBUG(CRusli+1, ("LMT gracefulExit "));
	/* Ignore SIGTERM because cleanup will send it to the entire
	** process group (including this function).
	*/
	signal(SIGTERM, SIG_IGN);
	CRDEBUG(1, ("CRasyncSCH::gracefulExit()"));

	char* dummyArgv[1];
	::cleanup(0,dummyArgv, SN_NOINIT,0);
	// exit out when process dies INIT will restart
	CRexitFunc(0);
}

void
CRasyncSCH::fatalExit()
{
	CRDEBUG(CRusli+1, ("fatalExit "));
	gracefulExit();
}

void
CRasyncSCH::HUPfunc()
{
	CRDEBUG(CRusli+1, ("LMT HUPfunc "));
	signal(SIGHUP, SIG_IGN);
	cleanUpPort();
	CRDEBUG(CRusli+1, ("hangup signal received, calling gracefulExit"));

	/* Ignore SIGTERM because cleanup will send it to the entire
	** process group (including this function).
	*/
	signal(SIGTERM, SIG_IGN);

	char* dummyArgv[1];
	::cleanup(0, dummyArgv, SN_NOINIT, 0);

	/* Set an alarm signal to try to prevent the exit() from hanging.
	** The exit can hang if the user hit the SCROLL-LOCK key,
	** because the exit will try to close the open device file.
	*/
	signal(SIGALRM, CRexitHang);
	alarm(3);

	// exit out when process dies INIT will restart
	CRexitFunc(0);
}

void
CRasyncSCH::rmvIM(const CRomInfo* omi)
{
	CRDEBUG(CRusli+1, ("LMT rmvIM "));
	rmv(omi);

	if (isOnLine() == YES)
	{
		CRERROR("RMV %s failed", getMsghName());

	}
	else
	{
		CROMDBKEY(RMVMSG, "/CR001"); /* RMV succeeded */
		CRomdbMsg om;
		om.add(getMsghName());
		om.spool(RMVMSG, omi);
	}
}

void
CRasyncSCH::rstIM(const CRomInfo* omi)
{
	CRDEBUG(CRusli+1, ("LMT rstIM "));
	if (isAttached() == YES)
	{
		CRDEBUG(CRusli+1, ("setting shared memory for RST"));
		sharedMemoryPtr->isRstOp = YES;

		if (omi)
		{
			sharedMemoryPtr->omInfoFlag = YES;
			sharedMemoryPtr->omInfo = *omi;
		}
		else
		{
			sharedMemoryPtr->omInfoFlag = NO;
		}
	}
	
	rst();

	/* Only gets here if did not exit,
	** so clear the shared memory.
	*/
	if (isAttached() == YES)
		sharedMemoryPtr->init();

	CRomdbMsg om;

	if (isOnLine() == YES)
	{
		/* this should call a virtual function in CRrop */
		CROMDBKEY(RSTMSG, "/CR000"); /* RST succeeded */
		om.add(getMsghName());
		om.spool(RSTMSG, omi);
	}
	else
	{
		/* this should call a virtual function in CRrop */
		/* restore failed, due to device being OffLine */
		CROMDBKEY(RSTFAIL, "/CR008");
		om.spool(RSTFAIL, omi);
	}
}

/* processMsg -  MSGH message processing function for the async SCH
**	It returns GLsuccess if successfully processed a message or handled
**      a receive() failure.
**      Returns GLfail if could not process the message
**      Sets rcvRetVal if did not process the receive() failure
*/
CRmsgResult
CRasyncSCH::processMsg(GLretVal rcvRetVal, char* msg_buf, int msgsz)
{
	CRDEBUG(CRusli+1, ("LMT processMsg "));
	if (rcvRetVal == GLsuccess)
	{
		switch (((MHmsgBase*) msg_buf)->msgType)
		{
		    case CRindQryMsgTyp:
			reportStatus();
			return CRmsgAccepted;

		    case CRropMsgTyp:
			CRDEBUG(CRusli+CRmsginFlg, ("RMV/RST msg received"));
			if (msgsz != (int) sizeof(CRropMsg))
			{
				CRERROR("received message of size %d, not size %d",
					msgsz, (int) sizeof(CRropMsg));
			}
			else
			{
				CRropMsg *msgPtr = (CRropMsg*) msg_buf;

				/* RST case */
				if (msgPtr->getStateFlag() == YES)
				{
					rstIM(msgPtr->getOmInfo());
				}
				else
				{
					rmvIM(msgPtr->getOmInfo());
				}
			}
			return CRmsgAccepted;
		}
	}
	return CRmsgUnknown;
}

void
CRasyncSCH::hwFail()
{
	CRDEBUG(CRusli+1, ("LMT hardware fail"));
	GLsetAsyncErrHandler(NULL);
	goOffLine(YES);
	gracefulExit();
}

void
CRasyncSCH::loopBack()
{
}

void
CRasyncSCH::rptLoopBack()
{
	CRDEBUG(CRusli+1, ("LMT loop back detected"));
	isOKtoWrite(NO);
	goOffLine(NO);
	channelState.state(CRloopBack);
	printLPom();
	reportStatus();
	CRDEBUG(CRusli+1, ("after loop back detected"));
}

void
CRasyncSCH::initDeviceInfo()
{
	CRDEBUG(CRusli+1, ("LMT initDeviceInfo"));
	if (isPermProcess() == NO)
	{
		if (GLISATTY(STDIN_FD))
		{
			devName = ttyname(STDIN_FD);
			if (devName == "")
				devName = "/dev/tty";
		}
		else
			devName = "/dev/tty";
		return;
	}

        /* if no filename then means UNEQUIP */
	const char* filename = "";
	const char* termtype = "615";
	baudRate = B9600;
	parity = CRPARITY_NONE;
	devName = filename;
	terminalType = termtype;

	if( (strncmp(permProcessName,"lmt",3))==0 ||
	    (strcmp(permProcessName,"/sn/cr/sccci"))==0 ||
	    (strncmp(permProcessName,"sccci",5))==0 ||
	    (strcmp(permProcessName,"sccci"))==0 )
	{
		termtype=getenv("TERM");
 
		if(termtype=="")
			termtype = "vt100";
 
		devName = ttyname(STDIN_FD);
		terminalType = termtype;

		//
		// Need to see if OSMON will let us start
		//
		if( (strncmp(permProcessName,"lmt",3))==0)
		{
			MHqid toQid;
			MHqid fromQid;
			pid_t getPid = getpid();
			String	theHostName;
			String	theLoginName;
			String theTtyName = ttyname(STDIN_FD);
			GLretVal rtn;
			GLretVal getInfo(const String theDevName, 
			            String &loginName, String &remoteHostName );

// In EES: Perform handshake only if OSMON process exists
#ifdef EES
			MHqid osmonQid;
			if ( MHmsgh.getMhqid("OSMON",osmonQid) != GLsuccess )
			{
				return;
			}
#endif	/* #ifdef EES */

		       rtn=getInfo(theTtyName, theLoginName, theHostName );


			if(rtn == GLfail)
			{
				// A second PTY may have been set in SECspawnWatson.c
				// See if thats the case and try again.
				//
				theTtyName = getenv( "FIRST_PTY" );
				rtn = getInfo( theTtyName, theLoginName, theHostName );

				if ( rtn == GLfail )
				{
					CRERROR("LMT fail to get host name");
					exit(0);
				}
			}

			String theServiceName = (theLoginName == "rmt") ? "/SEFM_RMT" : "LAB_RMT";

			CRossServAttemptMsg ossServAttemptMsg(getPid, 
			                        theTtyName,
						theHostName, theServiceName,
						String (permProcessName));

			static class rcvMsg : public MHmsgBase
			{
			    public:
			    union
				{
				Long    l_align;
				double  d_align;
				void   *p_align;
				Char    body[MHmsgSz]; // MSGH message receiving buffer
				};
			} myMsg;
			static Char *myMsgp = (Char *)&myMsg;
			Long                msgsz = MHmsgSz;


			rtn=MHmsgh.getMhqid("OSMON",toQid);
			if(rtn == GLfail)
			{
				CRERROR("LMT fail to get OSMON qid");
				exit(0);
			}

			rtn=MHmsgh.getMhqid(permProcessName,fromQid);
			if(rtn == GLfail)
			{
				CRERROR("LMT fail to get rmt qid");
				exit(0);
			}

			rtn=ossServAttemptMsg.send(toQid,fromQid);
			if(rtn == GLfail)
			{
				CRERROR("LMT send fail to OSMON");
				exit(0);
			}

			GLretVal rtn1 = MHmsgh.receive(fromQid, myMsgp, msgsz, 0,  60000);
			if(rtn1 == GLsuccess)
			{

				switch(myMsg.msgType)
				{
					case CRserviceAcceptMsgTyp:
					{
CRDEBUG(CRusli,("CRasyncSCH : OSMON is letting us on"));
					}
					return;

					case CRserviceDeniedMsgTyp:
					{
			      CRERROR("Recieved CRserviceDeniedMsg from OSMON");
					}
					exit(0);

					default:
					CRERROR("LMT Unexpected Message type = %d",
					         myMsg.msgType);
					//linuex FIX
					//exit(0);

				}// end switch
			}//end if
			else
			{
				CRERROR("LMT Didn't recieve message from OSMON");
				//linuex FIX
				//exit(0); //cleanup
			}

		}// end if lmt

CRDEBUG(CRusli,("LMT CRasyncSCH devName = %s termtype = %s",
(const char*) devName, (const char*) terminalType));
CRDEBUG(CRusli,("LMT CRasyncSCH permProcessName = %s",
(const char*) permProcessName));

		return;

	}// end if lmt or sccci

	// read from DB
	if (CRgetDeviceInfo(permProcessName, filename, baudRate,
		    parity, termtype) != GLsuccess)
	{
		CRERROR("could not read device info");
	}
	else
	{
		devName = filename;
		terminalType = termtype;
	}
}

void
CRasyncSCH::setTermio(CRTERMIO& settings)
{
CRDEBUG(0,("LMT setTermio"));
	tty_settings = settings;
}

void
CRasyncSCH::isOKtoWrite(Bool newValue)
{
CRDEBUG(0,("LMT isOKtoWrite"));
	okToWrite = newValue;
}

Bool
CRasyncSCH::isOKtoWrite() const
{
CRDEBUG(0,("LMT isOKtoWrite 2"));
	return okToWrite;
}

GLretVal
CRasyncSCH::initDevice()
{
CRDEBUG(0,("LMT initDevice"));
	isOKtoWrite(NO);

	if (isPermProcess() == YES)
	{
		/* if unequipped (no UNIX device file name)
		** then enter the UNEQ state
		*/
		if (*deviceName() == '\0')
		{
			channelState.state(CRuneqState);
			reportStatus();
			return GLsuccess;
		}

		/* this code is here to avoid contention in issuing a 
		 * ioctl with the FT subsystem. Tandem only allows one 
		 * ioctl to be active at any one time. Issuing two ioctls 
		 * on the same device causes port to be in a weird state .
		*/
#ifndef SOCKET
CRDEBUG(0,("LMT initDevice 1"));

		if (CRchklineStatus(devName) == GLfail)
		{
			initFail();
			return GLfail;
		}	
#endif
CRDEBUG(0,("LMT initDevice 2"));

		GLretVal retval = CRsetupStdio(devName, tty_settings);
		if (retval != GLsuccess)
		{
CRDEBUG(0,("LMT initDevice 3"));
			CRDEBUG(CRusli+1, ("LMT redirection of stdin/stdout to %s failed",
					 (const char*) devName));
			initFail();
			return retval;
		}

#ifdef _SVR4
CRDEBUG(0,("LMT initDevice 4"));
#ifndef SOCKET
		CRDEBUG(CRusli,
			("LMT 1 tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x)",
			 tty_settings.c_iflag, tty_settings.c_oflag,
			 tty_settings.c_lflag));
#else
		CRDEBUG(CRusli,
			("LMT 2 tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x c_line=%x)",
			 tty_settings.c_iflag, tty_settings.c_oflag,
			 tty_settings.c_lflag, tty_settings.c_line));
#endif
#else
		CRDEBUG(CRusli,
			("LMT 3 tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x c_line=%x)",
			 tty_settings.c_iflag, tty_settings.c_oflag,
			 tty_settings.c_lflag, tty_settings.c_line));
#endif
CRDEBUG(0,("LMT initDevice 6"));

#ifdef _SVR4
#ifndef SOCKET
CRDEBUG(0,("LMT initDevice 7"));
		if (tcsetattr(STDIN_FD, TCSANOW, &tty_settings) < 0)
#else
CRDEBUG(0,("LMT initDevice 8"));
		if (GLASYNCFAIL(GLIOCTL(STDIN_FD, TCSETAF, &tty_settings)))
#endif
#else
CRDEBUG(0,("LMT initDevice 10"));
		if (GLASYNCFAIL(GLIOCTL(STDIN_FD, TCSETAF, &tty_settings)))
#endif
		{
			CRDEBUG(CRusli+1, ("LMT ioctl of MCC tty failed because of %s.",
					 CRsysErrText(errno)));
			initFail();
			return GLfail;
		}
	}

	isOKtoWrite(YES);

	GLsetAsyncErrHandler((GLasyncErrHandler) hwFailFunc);
	initDeviceObjects();

	if (channelState.state() == CRhwDown)
	{
		return GLfail;
	}

	startUp();

	if (channelState.state() == CRhwDown)
		return GLfail;

	/* restore succeeded */
	CRomInfo* omi = NULL;

	if (isAttached() == YES && sharedMemoryPtr->isRstOp == YES)
	{
		if (sharedMemoryPtr->omInfoFlag == YES)
			omi = &sharedMemoryPtr->omInfo;
			
		/* this should call a virtual function defined in CRrop */
		CROMDBKEY(RSTMSG, "/CR000");
		CRomdbMsg om;
		om.add(getMsghName());
		om.spool(RSTMSG, omi);

		sharedMemoryPtr->init();
	}
	else
	{
		printISom(omi);
	}

	channelState.state(CRidle);
	reportStatus();

	return GLsuccess;
}

void
CRasyncSCH::initFail()
{
	CRDEBUG(0,("LMT initFail"));
	if (isPermProcess() == NO)
		fatalExit();

	CRomInfo* omi = NULL;

	switch (channelState.state())
	{
	    case CRinitState:
		printOOSom(omi);

		if (isAttached() == YES && sharedMemoryPtr->isRstOp == YES)
		{
			/* should be a virtual function */
			/* restore failed, due to device being OffLine */
			CRomdbMsg om;
			CROMDBKEY(RSTFAIL, "/CR008");

			if (sharedMemoryPtr->omInfoFlag == YES)
				omi = &sharedMemoryPtr->omInfo;
			
			om.spool(RSTFAIL, omi);
		}
		
		startRetryInitTmr(CRinitRetryTime1);
		channelState.state(CRhwDown);
		reportStatus();
		break;
	    case CRreInit:
		channelState.state(CRhwDown);
		break;
	    default:
		CRERROR("bad state %d", channelState.state());
		channelState.state(CRhwDown);
		break;
	}

	if (isAttached() == YES)
		sharedMemoryPtr->init();
}

void
CRasyncSCH::reportStatus() const
{
	CRDEBUG(0,("LMT reportStatus"));
	if (indName == NULL)
		return;

	/*
	**	Don't report status for sccci OS. OSMON will
	**	report the status.
	*/
	if( (strcmp(permProcessName,"/sn/cr/sccci"))==0 ||
	    (strncmp(permProcessName,"sccci",5))==0 ||
	    (strcmp(permProcessName,"sccci"))==0 )
	{
		return;
	}

	const char* indicatorState = "OOS";

	switch (channelState.state())
	{
	    case CRuneqState:
		indicatorState = "UNEQUIP";
		break;
	    case CRdownState:
		indicatorState = "OOSMAN";
		break;
	    case CRexit:
	    case CRhwDown:
	    case CRloopBack:
		indicatorState = "OOS";
		break;
	    default:
		indicatorState = "NORMAL";
		break;
	}

	CRindStatusMsg reportMsg;
	reportMsg.send(indName, indicatorState);
}

short
CRasyncSCH::procinit(int /*argc*/, char* /*argv*/[],
		     SN_LVL init_lvl, U_char /*run_lvl*/)
{
	CRDEBUG(0,("LMT procinit"));
	initDeviceInfo();

	switch (init_lvl)
	{
	    case SN_LV3:
	    case SN_LV4:
	    case SN_LV5:
		CRDEBUG(CRusli+CRinitFlg, ("init_lvl=%d"));
		createSharedMemory();
		break;
	}

	attachToSharedMemory();
	return GLsuccess;
}

void
CRasyncSCH::cleanup()
{
	CRDEBUG(0,("LMT cleanup"));

	detachFromSharedMemory();
	closeDevice();
}

void
CRasyncSCH::closeDevice()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::closeDevice()"));
	isOKtoWrite(NO);
	GLsetAsyncErrHandler(NULL);
	GLCLOSE(STDIN_FD);
	GLCLOSE(STDOUT_FD);
	GLCLOSE(STDERR_FD);
}

/* Called when retry init timer goes off.
** Must be in the CRhwDown state.
*/
GLretVal
CRasyncSCH::retryInit()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::retryInit()"));
	if (channelState.state() != CRhwDown)
	{
		CRDEBUG(CRusli+1, ("LMT state=%d", channelState.state()));
		stopRetryInitTmr();
		return GLsuccess;
	}
		
	channelState.state(CRreInit);
 
	if (initDevice() != GLsuccess)
	{
		isOKtoWrite(NO);
		channelState.state(CRhwDown);
		return GLfail;
	}

	stopRetryInitTmr();
//	startStartUpTimer();
	return GLsuccess;
}

/* start initialization retry timer */
void
CRasyncSCH::startRetryInitTmr(Long timeValue)
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::StartretryInit()"));
	initRetryTmrBlock = CRevent.setRtmr(timeValue,
					    CRinitRetryTag, TRUE);
	if (TMINTERR(initRetryTmrBlock))
	{
		CRERROR("CRevent.setRtmr failed with error code %d",
			initRetryTmrBlock);
		CRexitFunc(1);
	}
}

void
CRasyncSCH::stopRetryInitTmr()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::StopretryInit()"));
	if (initRetryTmrBlock == -1)
		return;

	CRevent.clrTmr(initRetryTmrBlock);
	initRetryTmrBlock = -1;
}

GLretVal
CRasyncSCH::timerHandler(char* msgBuf)
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::timerHandler()"));
	switch (((MHmsgBase*) msgBuf)->msgType)
	{
	    case BLtimeChngTyp:
		CRDEBUG(CRusli+CRmsginFlg, ("time change"));
		CRevent.updoffset();
		return GLsuccess;

	    case TMtmrExpTyp:
		switch (((TMtmrExp*) msgBuf)->tmrTag)
		{
#if 0
		    case CRstartUpTag:
			CRDEBUG(CRusli+CRmsginFlg, ("start up timer"));
			switch (channelState.state())
			{
			    case CRinitState:
			    case CRreInit:
				startUp();
				break;
			    default:
				break;
			}
			return GLsuccess;

		    case CRoffLineOMTag:
			CRDEBUG(CRusli+CRmsginFlg, ("offLineOM timer"));
			printOffLineOM();
			return GLsuccess;
#endif

		    case CRinitRetryTag:
			CRDEBUG(CRusli+CRmsginFlg, ("init retry timer"));
			retryInit();
			return GLsuccess;

		    default:
			break;
		}
	}
	return GLfail;
}

#if 0
void
CRasyncSCH::startStartUpTimer()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::startStartUpTimer()"));
	startUpTmrBlock = CRevent.setRtmr(CRstartUpTime, CRstartUpTag);
	if (TMINTERR(startUpTmrBlock))
	{
		CRERROR("CRevent.setRtmr failed with error code %d",
			startUpTmrBlock);
		CRasyncSCH::fatalExit();
	}
}
#endif

void
CRasyncSCH::goOffLine(Bool /*hwfailFlag*/)
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::goOffLine()"));
	if (isAttached() == YES)
		sharedMemoryPtr->init();
}

void
CRasyncSCH::rst()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::rst()"));
	if (isOnLine() == YES)
		return;

	switch (channelState.state())
	{
	    case CRloopBack:
	    case CRuneqState:
	    case CRdownState:
	    case CRhwDown:
		gracefulExit(); /* cause process to restart */

	    default:
		CRERROR("LMT bad state %d", channelState.state());
		break;
	}
}

void
CRasyncSCH::rmv(const CRomInfo* /*ominfo*/)
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::rmv()"));
	goOffLine(NO);
	channelState.state(CRdownState);
	reportStatus();
}

Bool
CRasyncSCH::isOnLine() const
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() "));
	switch (channelState.state())
	{
	    case CRloopBack:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRloopBack "));
		return FALSE;

	    case CRuneqState:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRuneqState "));
		return FALSE;

	    case CRhwDown:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRhwDown "));
		return FALSE;

	    case CRdownState:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRdownState "));
		return FALSE;

	    case CRinitState:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRinitState "));
		return FALSE;

	    case CRreInit:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRreInit "));
		return FALSE;

	    case CRexit:
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::isOnLine() CRexit "));
		return FALSE;

	    default:
		return TRUE;
	}
}


#if 0
const Long CRoffLineOMTime = 60*60; /* 1 hour */
const Long CRinitRetryTime = 20; /* 20 seconds */
const U_short CRoffLineOMTag = 10;
Short CRoffLineOMTmrBlock = -1;
#endif

#if 0
void
CRrop::startOffLineOMTmr()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::startOfflineOMtmr()"));
	CRoffLineOMTmrBlock = CRevent.setRtmr(CRoffLineOMTime,
					      CRoffLineOMTag, TRUE);
	if (TMINTERR(CRoffLineOMTmrBlock))
	{
		CRERROR("CRevent.setRtmr failed with error code %d",
			CRoffLineOMTmrBlock);
		exit(1);
	}
}

void
CRrop::stopOffLineOMTmr()
{
	CRDEBUG(CRusli+4, ("LMT CRasyncSCH::stoptOfflineOMtmr()"));
	if (CRoffLineOMTmrBlock == -1)
		return;

	CRevent.clrTmr(CRoffLineOMTmrBlock);
	CRoffLineOMTmrBlock = -1;
}
#endif

void
CRasyncSCH::doTTYsettings()
{
}

short
CRasyncSCH::sysinit()
{
	return GLsuccess;
}

short
CRasyncSCH::createSharedMemory()
{
	CRDEBUG(CRusli, ("LMT enter createSharedMemory"));
	if (sharedMemoryKey == 0)
	{
		CRDEBUG(CRusli, ("invalid sharedMemoryKey"));
		return GLfail;
	}

	if (isAttached() == YES)
	{
		detachFromSharedMemory();
		deleteSharedMemory();
	}

	Bool noExist;
	register int shmid = INshmem.allocSeg(sharedMemoryKey,
					      (U_long) sizeof(CRasyncSharedMemory),
					      0600, noExist);
	if (shmid < 0)
	{
		CRERROR("could not create shared memory (retval = %d)", shmid);
		return GLfail;
	}

	/* Map shared memory into process's address space */
	sharedMemoryPtr = (CRasyncSharedMemory *) shmat(shmid, (char *)0,
							~SHM_RDONLY);
	if (isAttached() == NO)
	{
		CRERROR("could not attach to shared memory (errno = %d)",
			errno);
		return GLfail;
	}

	sharedMemoryPtr->init();
	detachFromSharedMemory();
	return GLsuccess;
}

short
CRasyncSCH::attachToSharedMemory()
{
	CRDEBUG(CRusli, ("LMT enter attachToSharedMemory"));
	if (sharedMemoryKey == 0)
	{
		CRDEBUG(CRusli, ("invalid sharedMemoryKey"));
		return GLfail;
	}

	if (isAttached() == YES)
	{
		CRDEBUG(CRusli, ("already attached"));
		return GLfail;
	}

        int shmid = shmget(sharedMemoryKey, (int) sizeof(CRasyncSharedMemory), 0);
	if (shmid < 0)
        {
		CRDEBUG(CRusli, ("shmget failed with errno %d", errno));
                return GLfail; /* shared memory does not exist! */
        }

        /* Map shared memory into calling process's address space */
	sharedMemoryPtr = (CRasyncSharedMemory *) shmat(shmid, (char *)0,
							~SHM_RDONLY);
        if (sharedMemoryPtr == (CRasyncSharedMemory*) -1)
        {
		CRDEBUG(CRusli, ("shmat failed with errno %d", errno));
                return GLfail; /* data space is not big enough */
        }

	CRDEBUG(CRusli, ("successfully attached to shared memory\nrstOp=%d",
			 sharedMemoryPtr->isRstOp));
	return GLsuccess;
}

void
CRasyncSCH::deleteSharedMemory()
{
}

void
CRasyncSCH::detachFromSharedMemory()
{
	CRDEBUG(CRusli, ("LMT enter detachFromSharedMemory"));
	if (isAttached() == YES)
	{
		if (shmdt((char*) sharedMemoryPtr) != 0)
		{
			CRDEBUG(CRusli, ("shared memory detach failed"));
		}
	}

	sharedMemoryPtr = (CRasyncSharedMemory *) -1;
}

void
CRasyncSCH::reportOOS()
{
	CRDEBUG(CRusli, ("LMT enter reportOSS"));
	reportStatus();
}

void
CRasyncSharedMemory::init()
{
	CRDEBUG(CRusli, ("LMT enter CRasyncSharedMemory::init()"));
	isRstOp = NO;
	omInfoFlag = NO;
}

// getInfo() is not needed on FT since OSMON handshake is not supported.
// Determine Login Name and Remote Host Name from controlling TTY
// by searching the /var/adm/utmpx file for a login entry to match my TTY
//
GLretVal
getInfo(const String theDevName, String &loginName, String &remoteHostName )
{
	CRDEBUG(CRusli, ("LMT enter getInfo()"));

	struct utmpx *utmpxEntry = NULL;	// Return value from getutxline()
	struct utmpx utmpxLine;			// Argument to getutxline()

	// Set default return values
	loginName = "unknown";
	remoteHostName = "unknown";

	// Do not accept "/dev/tty" as device name or a short name
	if ( theDevName == "/dev/tty" || theDevName.length() < 6)
	{
	CRDEBUG(CRusli, ("LMT enter getInfo() fail 1"));
	    return GLfail;
	}

	// Device name must begin with "/dev/"
	if ( theDevName.index("/dev/") != 0 )
	{
	CRDEBUG(CRusli, ("LMT enter getInfo() fail 2"));
	    return GLfail;
	}

	// Stip off "/dev/" from the tty name
	String deviceName = theDevName.chunk(sizeof("/dev"));
	deviceName.dump(utmpxLine.ut_line);


	// Reset the utmpx file seek position
	//non - linux FIX if ( utmpxname("/var/adm/utmpx") < 0 )
	//linux fix if ( utmpxname("/var/log/wtmp") < 0 )
#ifdef __linux
	if ( utmpxname("/var/log/wtmp") < 0 )
#else
	if ( utmpxname("/var/adm/utmpx") < 0 )
#endif
	{
	    CRERROR("Cannot reset seek position in utmpx file, errno=%d", errno);
	CRDEBUG(CRusli, ("LMT enter getInfo() fail 3"));
	    return GLfail;
	}

	if ( (utmpxEntry = getutxline(&utmpxLine)) != NULL)
	{
	    loginName = utmpxEntry->ut_user;		// Login name
	    if (utmpxEntry->ut_host)
	    	remoteHostName = utmpxEntry->ut_host;	// Remote host
	    return GLsuccess;
	}
	CRDEBUG(CRusli, ("LMT enter getInfo() fail 4"));
	return GLfail;
}


