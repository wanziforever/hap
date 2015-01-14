
/*
**
**      File ID:        @(#): <MID30011 () - 09/06/02, 25.1.1.1>
**
**      File:                                   MID30011
**      Release:                                25.1.1.1
**      Date:                                   09/12/02
**      Time:                                   10:39:19
**      Newest applied delta:   09/06/02
**
** DESCRIPTION:
**	This contain functions for CRspoolMsg class that are used for 
**	CR_MISCPRM macro only. 
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
#include "cc/hdr/cr//CRomDest.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRprmMsg.hh"

short CRdestFile;
/* the following routines will be called only when the CR_MISCPRM macro is used */
/* CR_MISCPRM macro is used by CSCC and CSOP to log initialization
 * messages 
*/
void CRspoolMsg::miscSpool(short destDev) {
	CRdestFile = destDev;
	sendfn = &CRspoolMsg::miscSend;
	spool();
}

GLretVal CRspoolMsg::miscSend() {
	Long time_s;
	time_s = time(0);
	msghead.timestamp = time_s;
	
	char tmp_buf[CRsplMaxTextSz];
	if (textLength() == 0) {
		CRERROR("Output message has no text. Message not sent.");
		return GLfail;
	}
	(void) strcpy(tmp_buf, msgText);
	
	CRprmLogSop logerr;
	if (logerr.init(CRDEFPRMLOG) == GLsuccess)
		sendToSop(&logerr);

	switch (CRdestFile) {

	case CRprmLogOnly:
		break;
		
	case CRprmLogAndCnsl:
		{
		break;
		}
	}	
	return GLsuccess;
}
