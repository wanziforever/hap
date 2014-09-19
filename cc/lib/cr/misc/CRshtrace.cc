/*
**      File ID:        @(#): <MID18619 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18619
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:45
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file contains functions to print error and debugging messages
**	for the USLI subsystem.  It directs the output to stderr,
**	which should be /dev/console.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include "cc/cr/hdr/CRshtrace.H"
#include "cc/hdr/cr/CRdebugMsg.H"

char CRshprocname[25];
CRshCrMsg CRsherror;
CRshCrMsg CRshdebug;

Bool CRshCrMsg::isDebugOn = NO;

CRshCrMsg::CRshCrMsg()
{
}

CRshCrMsg::~CRshCrMsg()
{
}

void
CRshCrMsg::add_va_list(const Char *strng, va_list ap)
{
	if (CRtraceFlags.isBitSet(CRusli) == YES)
		vfprintf(stderr, strng, ap);
}
			
void
CRshCrMsg::add(const Char* format, ...)
{
	va_list ap;
	va_start(ap, format);

	add_va_list(format, ap);
	va_end(ap);
}

void
CRshCrMsg::spool(const Char *format,...)
{
	va_list ap;
	va_start(ap, format);

	add_va_list(format, ap);
	va_end(ap);

	spool();
}

void
CRshCrMsg::spool()
{
	if (CRtraceFlags.isBitSet(CRusli) == YES)
		fprintf(stderr, "\n\n");
}
