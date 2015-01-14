/*
**
**      File ID:        @(#): <MID27572 () - 09/06/02, 26.1.1.1>
**
**	File:					MID27572
**	Release:				26.1.1.1
**	Date:					09/12/02
**	Time:					10:39:21
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This contain functions ifor CRspoolMsg class that are used for 
**	CR_PRM macro only. 
**
** OWNER:
**      Yash Gupta
**
** NOTES:
**
*/

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "cc/hdr/cr/CRtmstamp.hh"
#include "cc/hdr/cr/CRspoolMsg.hh"
//#include "cc/hdr/misc/GLasync.hh"
#include "cc/hdr/cr/CRomDest.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"

/* the following routines will be called only when the CR_PRM macro is used */
/* CR_PRM macro is used by many permanent processes to log initialization
 * messages 
*/
void
CRspoolMsg::prmSpool()
{
	sendfn = &CRspoolMsg::prmSend;
	spool();
}

GLretVal
CRspoolMsg::prmSend()
{
	Long time_s;
	time_s = time(0);
	msghead.timestamp = time_s;
	
	char tmp_buf[CRsplMaxTextSz];
	if (textLength() == 0)
	{
		CRERROR("Output message has no text. Message not sent.");
		return GLfail;
	}
	(void) strcpy(tmp_buf, msgText);
	
	CRprmLogSop logerr;
	if (logerr.init(CRDEFPRMLOG) == GLsuccess)
		sendToSop(&logerr);

	/* now send the message to CSOP */
	GLretVal rtn = send();

	if (rtn != GLsuccess)
	{
#ifndef __linux

		/* since the CSOP process is not up and running, write the 
		 * message to console 
		*/
		String dynamic_buf = Stringsize(4096);
		dynamic_buf = "\n";
		static char timebuf[30];
		CRformatTime(time(0), timebuf);
		dynamic_buf += timebuf;
		dynamic_buf += " ";
		dynamic_buf += tmp_buf;
		dynamic_buf += "\n";

		/* the message needs to be written to console here */
		/* set async gaurd time to 1 second for now */
		GLsetAsyncGuardTime(1);
#ifdef CC
		int console_fd = GLopen("/dev/pspcon", O_WRONLY| O_NOCTTY);
#else
		int console_fd = GLopen("/dev/pspcon", O_WRONLY);
#endif
		if (console_fd != -1)
		{	
			GLwrite(console_fd, (const char *) dynamic_buf, dynamic_buf.length());
  			GLclose(console_fd);
		}
#endif /* not __linux */
		return GLfail;
	}
	return GLsuccess;
}	

void
CRspoolMsg::intSpool()
{
	sendfn = &CRspoolMsg::intSend;
	spool();
}

GLretVal
CRspoolMsg::intSend()
{
	Long time_s;
	time_s = time(0);
	msghead.timestamp = time_s;
	
	char tmp_buf[CRsplMaxTextSz];
	if (textLength() == 0)
	{
		CRERROR("Output message has no text. Message not sent.");
		return GLfail;
	}
	(void) strcpy(tmp_buf, msgText);
	
	CRintLogSop logerrI;
	if (logerrI.init(CRDEFINTLOG) == GLsuccess)
		sendToSop(&logerrI);

	//send the OM to the PRM logs also
	CRprmLogSop logerr;
	if (logerr.init(CRDEFPRMLOG) == GLsuccess)
		sendToSop(&logerr);
	
	/* now send the message to CSOP */
	GLretVal rtn = send();

	if (rtn != GLsuccess)
	{
#ifndef __linux
		/* since the CSOP process is not up and running, write the 
		 * message to console 
		*/
		String dynamic_buf = Stringsize(4096);
		dynamic_buf = "\n";
		static char timebuf[30];
		CRformatTime(time(0), timebuf);
		dynamic_buf += timebuf;
		dynamic_buf += " ";
		dynamic_buf += tmp_buf;
		dynamic_buf += "\n";

		/* the message needs to be written to console here */
		/* set async gaurd time to 1 second for now */
		GLsetAsyncGuardTime(1);
#ifdef CC
		int console_fd = GLopen("/dev/pspcon", O_WRONLY| O_NOCTTY);
#else
		int console_fd = GLopen("/dev/pspcon", O_WRONLY);
#endif
		if (console_fd != -1)
		{	
			GLwrite(console_fd, (const char *) dynamic_buf, dynamic_buf.length());
  			GLclose(console_fd);
		}
#endif /* not __linux */
		return GLfail;
	}
	return GLsuccess;
}	

