#ifndef __CRDIRCHECK_H
#define __CRDIRCHECK_H
/*
**      File ID:        @(#): <MID21312 () - 08/17/02, 29.1.1.1>
**
**	File:					MID21312
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:15:49
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file declares two utility functions: CRdirCheck which
**      checks that a directory exists and is accessible by the calling
**      process.  CRcreateDir will attempt to create a directory that is
**      accessible by the calling process and is owned by a specified login.
** OWNER: 
**	Roger McKee
**
** NOTES:
**	It is assumed that all processes using this class have called
**	CRERRINIT (in cc/hdr/cr/CRdebugMsg.H) to setup for CRERROR macros.
*/
#include "hdr/GLtypes.h"

extern Bool CRdirCheck(const char* directory, Bool createFlag =NO,
		       const char* userId =NULL);
extern Bool CRcreateDir(const char* dirname, const char* userId =NULL);

#endif

