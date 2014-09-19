/*
**
**      File ID:        @(#): <MID19369 () - 08/17/02, 29.1.1.1>
**
**	File:					MID19369
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:40:14
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the definition of the CRmsgKeyBoard and 
**      CRsccMsgKeyBoard classes
**      which implement "virtual" keyboards via a MSGH interface to
**      an "lmtrproc" process which reads characters from stdin and passes
**      to the process contains a CRmsgKeyBoard (or CRsccMsgKeyBoard) object.
**      The CRsccMsgKeyBoard is derived from the CRmsgKeyBoard class and
**      differs only in the handling of some special editing characters.
**      This differences are handled by passing different parameters in the
**      constructor argument list.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <signal.h>
#include <termio.h>
#include <fcntl.h>
#include "cc/cr/hdr/CRterminal.H"
#include "cc/cr/hdr/CRredirect.H"
#include "cc/cr/hdr/CRshtrace.H"
#include "cc/cr/hdr/CRinputDev.H"
#include "cc/cr/hdr/CRrprocMsg.H"
#include "cc/cr/hdr/CRgenMHname.H"
#include "cc/hdr/cr/CRvpath.H"
#include "cc/hdr/cr/CRsysError.H"
#include "cc/hdr/misc/GLasync.H"
#include "cc/hdr/tim/TMmtype.H"
#include "cc/hdr/tim/TMtmrExp.H"
#include "cc/hdr/eh/EHreturns.H"
#include "cc/hdr/eh/EHhandler.H"
#include "cc/cr/hdr/CRtimers.H"
#include "cc/cr/hdr/CRmaintApp.H"

extern EHhandler CRevent;

const int STDIN_FD = 0;
const int STDOUT_FD = 1;
const int STDERR_FD = 2;

void
CRkillProc(int pid)
{
	if (kill(pid, SIGTERM) == -1)
	{
		CRDEBUG(CRusli, ("kill(%d, SIGTERM) failed due to errno %d",
			       pid, errno));
	}
	sleep(1);
	if (kill(pid, 0) == 0)
	{
		if (kill(pid, SIGKILL) == -1)
		{
			CRDEBUG(CRusli,
				("kill(%d, SIGKILL) failed due to errno %d",
				 errno));
			return;
		}
		sleep(1);
		if (kill(pid, 0) == 0 || errno == EPERM)
		{
			CRERROR("could not kill process (pid=%d)",
				pid);
			return;
		}
	}
}

/* CRsccMsgKeyBoard is the same as CRmsgKeyBoard,
** except the CAN character is taken as the interrupt character.
*/
const int CAN = 030;

CRsccMsgKeyBoard::CRsccMsgKeyBoard(const char* cprocMHname, MHqid cpMhqid,
				   const char* rprocMHname,
				   const char* device,
				   CRmaintApplication* app,
				   int interCharTimeout) :
				   CRmsgKeyBoard(cprocMHname, cpMhqid,
						 rprocMHname, device, app,
						 interCharTimeout, CAN)
{
}

CRsccMsgKeyBoard::~CRsccMsgKeyBoard()
{
}


CRmsgKeyBoard::CRmsgKeyBoard(const char* cprocMHname, MHqid cpMhqid,
			     const char* rprocMHname,
			     const char* device,
			     CRmaintApplication* app,
			     int interCharTimeout,
			     int intrptChar) :
			     CRinputDevice(intrptChar),
			     applicationPtr(app),
			     maxWaitTime(interCharTimeout)
{
	cprocMhqid = cpMhqid;
	strcpy(cprocMsghName, cprocMHname);
	if (rprocMHname == NULL)
		CRgenMHname("RP", (int) getpid(), rprocMsghName);
	else
		strcpy(rprocMsghName, rprocMHname);
	strcpy(deviceName, device);
	rprocPid = -1;
	tmrBlock = -1;
	if (strcmp(deviceName, "/dev/tty") == 0)
		permProcessFlag = NO;
	else
		permProcessFlag = YES;
}

GLretVal
CRmsgKeyBoard::init()
{
	if (conditionDevice(deviceName) != GLsuccess)
		return GLfail;

	spawnRproc();	/* fork/exec of rproc */

	return GLsuccess;
}

CRmsgKeyBoard::~CRmsgKeyBoard()
{
	killRproc();

	if (permProcessFlag == NO)
		restoreSettings();
}

GLretVal
CRmsgKeyBoard::processMsg(char* msgbuf, int /*msgsz*/, Bool& hwFailFlag)
{
	hwFailFlag = NO;

	switch (((MHmsgBase*) msgbuf)->msgType)
	{
	    case CRrprocMsgTyp:
	      { /* variable block */
		int senderPid = ((CRrprocMsg*) msgbuf)->getPid();
		if (senderPid != rprocPid)
		{
			CRDEBUG(CRusli, ("received characters from wrong RPROC (%d) current RPROC is %d",
				senderPid, rprocPid));
			CRkillProc(senderPid);
		}
		else
			unGetChars(((CRrprocMsg*) msgbuf)->getMsg(),
				   ((CRrprocMsg*) msgbuf)->length());
	      } /* end variable block */
		break;

	    case CRrpInitMsgTyp:
		initRproc();
		break;

	    case CRrpHwFailMsgTyp:
		hwFailFlag = YES;
		break;

	    default: /* unrecognized message */
		return GLfail;
	}
	return GLsuccess;
}

void
CRmsgKeyBoard::stopTimer()
{
	if (tmrBlock != -1)
	{
		CRevent.clrTmr(tmrBlock);
		tmrBlock = -1;
	}
}

/* set one-shot inter character timer */
void
CRmsgKeyBoard::startTimer()
{
	tmrBlock = CRevent.setRtmr(maxWaitTime, CRinterCharTag);
	if (TMINTERR(tmrBlock))
	{
		CRERROR("CRevent.setRtmr failed with error code %d",
			tmrBlock);
		exit(1);
	}
}

/* set one-shot inter character timer */
void
CRmsgKeyBoard::startQuickTimer()
{
	tmrBlock = CRevent.setRtmr(CRquickInterCharTime, CRinterCharTag);
	if (TMINTERR(tmrBlock))
	{
		CRERROR("CRevent.setRtmr failed with error code %d",
			tmrBlock);
		exit(1);
	}
}

int
CRmsgKeyBoard::waitForCharMsg()
{
	const int DEL = 0177;

	/*
         * IBM swerbner 20061201
         * Eliminate extraneous declaration of message buffer pointer.
         * Lucent had intended it to be replaced by a member in an
         * aligned union -- refer to the MHlocBuffer macro call below.
         */
	// Char *msg_buf;

	int origCharLen = storedChars_V.length();

	do
	{
		MHlocBuffer(msg_buf);

		/* wait for message from RPROC */

		Short msgsz = MHmsgSz;

		GLretVal rtn = CRevent.getEvent(cprocMhqid, msg_buf, msgsz);

		if (rtn == GLsuccess)
		{
			if (((MHmsgBase*) msg_buf)->msgType == TMtmrExpTyp &&
			    ((TMtmrExp*) msg_buf)->tmrTag == CRinterCharTag)
			{
				stopTimer(); /* just to be sure */
				return CRtimeoutChar;
			}
		}
		else
		{
			if (interruptPending())
			{
				CRDEBUG(CRusli, ("input interrupt while getting chars"));
				stopTimer();
				unGetChar(CAN);
				return CAN;
			}
		}

		switch (applicationPtr->processMsg(rtn, msg_buf, msgsz))
		{
		    case CRmsgAcceptAbort:
			CRDEBUG(CRusli, ("aborted due to critical OM"));
			stopTimer();
			return CRabortChar;

		    case CRmsgAccepted:
		    case CRmsgAcceptContinue:
			break;
		    case CRmsgUnknown:
			if (rtn == GLsuccess)
			{
				CRDEBUG(CRusli,
					("unrecognized msgtype %d",
					 ((MHmsgBase*) msg_buf)->msgType));
			}
			else
			{
				CRERROR("unexepected failed %d from receive()",
					rtn);
			}
			break;
		    case CRmsgInterrupt:
			CRDEBUG(CRusli, ("input interrupt"));
			stopTimer();
			unGetChar(DEL);
			return DEL; /* simulates user hitting DELETE */
		}
	} while (storedChars_V.length() <= origCharLen);

	stopTimer();
	return 0;
}

int
CRmsgKeyBoard::getNewChar()
{
	/* set a timer */
	/* wait for msg with characters */
	/* return the first character, store the rest */
	/* if timer expires, return special CRtimeoutChar value */

	startTimer();

	switch (waitForCharMsg())
	{
	    case CRtimeoutChar:
		return CRtimeoutChar;
	    case CRabortChar:
		return CRabortChar;
	}

	return getChar();
}

int
CRmsgKeyBoard::loadNchars(int /*n*/)
{
	/* set a timer */
	/* wait for msg with characters */
	/* returns number of chars read */

	startQuickTimer();
	waitForCharMsg();
	return storedChars_V.length();
}

void
CRmsgKeyBoard::killRproc()
{
	if (rprocPid > 0)
	{
		CRkillProc(rprocPid);
		rprocPid = -1;
	}
	else
	{
		CRDEBUG(CRusli, ("no RPROC to kill (pid = %d)", rprocPid));
	}
}

/* this function (haltRproc()) is no longer used
** The RPROC is just killed whenever it is temporarily not needed.
** When it is needed again it is respawned.
*/
void
CRmsgKeyBoard::haltRproc()
{
}

void
CRmsgKeyBoard::suspendInput()
{
	killRproc();
	restoreSettings();
}

void
CRmsgKeyBoard::resumeInput()
{
#ifdef _SVR4
#ifndef SOCKET
	if (tcsetattr(STDIN_FD, TCSANOW, &tty_settings) < 0)
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &tty_settings)))
#endif
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &tty_settings)))
#endif
	{
		CRSHERROR("ioctl of MCC tty failed because of %s.",
			  CRsysErrText(errno));
	}
	spawnRproc();
}

void
CRmsgKeyBoard::initRproc()
{
	CRrpInitMsg msg;
	msg.sendToRproc(rprocMsghName, deviceName);
}

void
CRmsgKeyBoard::restoreSettings()
{
	if (!GLISATTY(STDIN_FD))
		return;

#ifdef _SVR4
#ifndef SOCKET
	if (tcsetattr(STDIN_FD, TCSANOW, &orig_settings) < 0)
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &orig_settings)))
#endif
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &orig_settings)))
#endif
	{
		CRERROR("ioctl() to restore terminal failed");
	}
}


GLretVal
CRmsgKeyBoard::conditionDevice(const char* /*device*/)
{
	if (!GLISATTY(STDIN_FD))
		return GLsuccess;

	initTermios();

#ifdef _SVR4
#ifndef SOCKET
	CRDEBUG(CRusli,
		("tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x)",
		 tty_settings.c_iflag, tty_settings.c_oflag,
		 tty_settings.c_lflag));
#else
	CRDEBUG(CRusli,
		("tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x c_line=%x)",
		 tty_settings.c_iflag, tty_settings.c_oflag,
		 tty_settings.c_lflag, tty_settings.c_line));
#endif
#else
	CRDEBUG(CRusli,
		("tty_settings(c_iflag=%x, c_oflag=%x, c_lflag=%x c_line=%x)",
		 tty_settings.c_iflag, tty_settings.c_oflag,
		 tty_settings.c_lflag, tty_settings.c_line));
#endif

#ifdef _SVR4
#ifndef SOCKET
	if (tcsetattr(STDIN_FD, TCSANOW, &tty_settings) < 0)
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &tty_settings)))
#endif
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCSETAF, &tty_settings)))
#endif
	{
		CRDEBUG(CRusli, ("ioctl of MCC tty failed because of %s.",
				 CRsysErrText(errno)));
		return GLfail;
	}

	return GLsuccess;
}

void
CRmsgKeyBoard::prepForInput()
{
	/* check to see if RPROC is alive or not */
	if (rprocPid < 0)
	{
		CRDEBUG(CRusli, ("prepForInput even though RPROC is dead"));
		return;
	}

	if (kill(rprocPid, 0) == 0 || errno == EPERM)
	{
		CRDEBUG(CRusli, ("RPROC (pid=%d) is alive", rprocPid));
		return;
	}

	/* if dead, then restart it */
	CRDEBUG(CRusli, ("RPROC (%d) is dead", rprocPid));
	spawnRproc();
}

void
CRmsgKeyBoard::spawnRproc()
{
	String rprocCmdPath = "/sn/cr/lmtrproc";

#ifdef EES
#ifdef __linux
	const char* rprocCmd = "cc/cr/rproc/LE/lmtrproc";
#else
	const char* rprocCmd = "cc/cr/rproc/EE/lmtrproc";
#endif
	FILE* fp = CRfopen_vpath(rprocCmd, "r", rprocCmdPath);
	if (!fp)
	{
		CRERROR("could not find RPROC for USL PARSER");
		return;
	}
	fclose(fp);
#endif

	char* rprocArgv[3];
	rprocArgv[0] = rprocMsghName;
	rprocArgv[1] = cprocMsghName;
	rprocArgv[2] = 0;

	rprocPid = (int) fork();
	if (rprocPid == 0) /* child */
	{
		execv(rprocCmdPath, rprocArgv);
	}
	else if (rprocPid == -1)
	{
		CRERROR("fork() failed");
	}
}

void
CRmsgKeyBoard::initTermios()
{
	if (!GLISATTY(STDIN_FD))
		return;


#ifdef _SVR4
#ifndef SOCKET
	if (tcgetattr(STDIN_FD, &orig_settings) < 0)
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCGETA, &orig_settings)))
#endif
#else
	if (GLASYNCFAIL(GLioctl(STDIN_FD, TCGETA, &orig_settings)))
#endif
	{
		CRERROR("ioctl(TCGETA) of tty failed because of %s",
			strerror(errno));
		exit(0);
	}

	tty_settings = orig_settings;

	/* set for RAW input mode and no flow control */
	tty_settings.c_iflag &= ~(INLCR|ICRNL|IUCLC/*|ISTRIP*//*|BRKINT*/);
	tty_settings.c_lflag &= ~(ICANON|ISIG|ECHO);
	tty_settings.c_cc[VERASE] = 0x08; /* CTRL-H */
	tty_settings.c_cc[4] = 5; /* MIN */
	tty_settings.c_cc[5] = 2; /* TIME */
#ifdef _SVR4
#ifndef SOCKET
	tty_settings.c_cc[VSUSP] = 0; /* SUSPEND */
#endif			 
#endif
}
