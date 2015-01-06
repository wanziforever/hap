#ifndef __CRMSGINT_H
#define __CRMSGINT_H
/*
**
**      File ID:        @(#): <MID15902 () - 08/17/02, 29.1.1.1>
**
**      File:                                   MID15902
**      Release:                                29.1.1.1
**      Date:                                   08/21/02
**      Time:                                   19:15:40
**      Newest applied delta:   08/17/02
**
** DESCRIPTION:
**	This defines constants and functions important to the internals
**      of the CRmsg and CRcsopMsg classes.
**
** OWNER:
**      Roger McKee
**
** NOTES:
**
*/

class String;

extern void CRaddblanks(const char* instring, String& outbuf);

extern const char* CRcorrupt_mem;
extern const char* CRbad_fmt;

const int CRVBUFSIZE = 8000;
#endif
