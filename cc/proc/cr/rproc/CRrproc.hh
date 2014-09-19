#ifndef __CRRPROC_H
#define __CRRPROC_H
/*
**      File ID:        @(#): <MID17561 () - 02/19/00, 23.1.1.1>
**
**	File:					MID17561
**	Release:				23.1.1.1
**	Date:					05/13/00
**	Time:					13:27:00
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**	This file defines the CRrproc class used by the RPROC process 
**	for its internal use.
** OWNER: 
**	Yash Gupta
**      Roger McKee
** NOTES:
**
*/

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/cr/hdr/CRrprocMsg.H"
#include "cc/cr/hdr/CRdevName.H"

class CRrproc
{
      public:
	CRrproc();
	~CRrproc();

	short init(const char *dev);
	GLretVal processInput();
	
      private:
	short readInput(); /* returns number of chars read */
	void hardwareFailure();

      private:
	CRrprocMsg rpMsg;
	enum {TEXTSZ = 10 };
	char 	text[TEXTSZ];
	CRdevName device;
	Short	fd;
} ;

#endif

