#ifndef __CRDBCMDMSG_H
#define __CRDBCMDMSG_H

/*
**
**      File ID:        @(#): <MID12276 () - 08/17/02, 29.1.1.1>
**
**      File:                                   MID12276
**      Release:                              	29.1.1.1  
**      Date:                                   08/21/02
**      Time:                                   19:32:16
**      Newest applied delta:   08/17/02
**
** DESCRIPTION:
**      This file defines a CFTI class that will be used to send
**      a debug message that contains information that will determine
**	whether the a subsystem should set its debugging flag to on or off.
**
** OWNER:
**      Roger McKee
**
** NOTES:
**
**
*/

#include "cc/hdr/cr/CRdebugMsg.hh"

class CRdbCmdMsg: public MHmsgBase {
      public:
	CRdbCmdMsg();

	Bool setMap(const char* flagName);
	Bool setMap(short flagValue);
	GLretVal send(MHqid toQid, MHqid sQid, Long time =-1);
	GLretVal send(const Char *name, MHqid sQid, Long time =-1);

	void unload() const;
	const unsigned char* ssnStart(CRsubsys);
	
      public:
	Bool isDebugOn;

      private:
	CRtraceMap bitmap;
};
#endif
