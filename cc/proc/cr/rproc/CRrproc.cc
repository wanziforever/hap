/*
**      File ID:        @(#): <MID17560 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17560
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:26:59
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**	This file contains the member functions for the CRrproc class.
** OWNER: 
**	Yash Gupta
**	Roger McKee
** NOTES:
**
*/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termio.h>
#include <ctype.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "CRrproc.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRomdbMsg.H"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/misc/GLasync.H"
#include "cc/hdr/cr/CRsysError.H"

CRrproc::CRrproc()
{
	text[0] = '\0';
	fd = -1;
}

CRrproc::~CRrproc()
{
}

//	Name:	CRrproc::init
//	Function:
//		This routine does the initialization of 
//		the tty port with the initialization parameters 
//		received from the CPROC process.
//
//	Called By:
//		procinit (which is called at the discretion of INIT).

Short
CRrproc::init(const char *devName)
{
	CRDEBUG( CRusli+CRinitFlg, ("enter init") ) ;

	device = devName;
	fd = 0; /* stdin fd */

	return GLsuccess;
} /* init */


/* 	Name CRrproc::hardwareFailure
**	Function:
**	This routine will handle a hardware failure
**
**	Called By:
**		CRrproc::readInput() 
*/	
void
CRrproc::hardwareFailure()
{
	CRDEBUG(CRusli+CRinoutFlg, ("hardware failure in RPROC"));

	/* send msg to CPROC to inform it of failure */
	CRrpHwFailMsg failMsg;
	failMsg.send();

	exit(cleanup(1, NULL, SN_NOINIT, 0));
}

// 	Name CRrproc::readInput
//	Function:
//	This routine will read the input from the device.
//
//	Called By:
//		CRrproc::processInput() 
//	

short
CRrproc::readInput()
{
	int numchars = GLREAD(fd, text, TEXTSZ);

#ifdef __linux
	while(numchars < 1)
	{
		numchars = GLREAD(fd, text, TEXTSZ);
	}
#endif

	if (GLASYNCFAIL(numchars))
	{
		if (GLASYNCHWFAIL(numchars))
		{
			hardwareFailure();
		}
		else
		{
			CRDEBUG(CRusli, ("error (%d) [%s] in reading from %s",
                       errno, CRsysErrText(errno),
                       (const char*) device));
			exit(cleanup(1, NULL, SN_NOINIT, 0));
		}

		return 0;
	}
	CRDEBUG(CRusli, ("numchars read(%d)", numchars));

	if (CRDEBUG_FLAGSET(CRusli))
	{
		for (int i = 0; i < numchars; i++)
		{
			if (text[i] == '\0')
			{
				CRDEBUG(CRusli, ("NULL char read"));
			}
			else
			{
				if (isprint(text[i]) && !isspace(text[i]))
				{
					CRDEBUG(CRusli, ("char=%c", text[i]));
				}
				else
				{
					CRDEBUG(CRusli,
                  ("char=(0x%x)", text[i]));
				}
			}
		}
	}

	text[numchars] = '\0';
	return numchars;
}

/*	Name::CRrproc::processInput()
 *	Function:
 *		This routine is called by the process() routine to
 *		read abd process the input received from the device.
 *
 *	Called By:	process() routine
 */

GLretVal
CRrproc::processInput()
{
	short numchars = readInput();
	if (numchars == 0)
     return GLfail;
	
	rpMsg.putMsg(text, numchars);
	
	if (rpMsg.send() != GLsuccess)
	{
		CRERROR("Unable to send message to CPROC process");
		return GLfail;
	}

	return GLsuccess;
}
