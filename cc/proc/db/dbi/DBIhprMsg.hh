#ifndef __DBIHPRMSG_H
#define __DBIHPRMSG_H

/*
**
**	File ID:	%Z%: <%M% (%Q%) - %G%, %I%>
**
**	File:					%M%
**	Release:				%I%
**	Date:					%H%
**	Time:					%T%
**	Newest applied delta:	%G%
**
** DESCRIPTION:
**	  Used to define  DBI helper ready  msgs
**
** OWNER:
**	eDB team
**
** HISTORY
**	Yeou H. Hwang
**
** NOTES:
*/

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/db/DBmtype.hh"
 
  
class DBIhprMsg : public MHmsgBase {
public :
  DBIhprMsg();
  GLretVal send(MHqid toQid, MHqid srcQid, Long time);
  // Msg format
public :
  Short state; // helper state
  Char name[MHmaxNameLen+1]; // name of this helper
};

#endif
