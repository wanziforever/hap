/*
**	EMACS_MODES: c, !fill, tabstop=4
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
** Definition of member functions of the message classes DBselect and DBselectAck
**
** OWNER:
**	Yeou H. Hwang
**
** NOTES:
*/

#include <String.h>
#include <string.h>
#include "cc/hdr/db/DBretval.hh"
#include "cc/hdr/db/DBlimits.hh"
#include "cc/hdr/db/DBselect.hh"


DBselect::DBselect()
{
	msgType = DBselectTyp;
  rowLimit = 0;   // default=0 -> return all records retrieved.   If any
  // process wishes to receive only a limited number of 
  // records retrieved, it must overwrite this value.
}


DBselectAck::DBselectAck()
{
	priType = MHoamPtyp;
	msgType = DBselectAckTyp;
	srcQue = MHnullQ;
	ackMessage[0] = '\0';
}


GLretVal 
DBselectAck::send(MHqid toQid, MHqid sQid, Short len, Long time)
{
  if (ackCode == DBSUCCESS)
  {
    ackMessage[0] = '\0';
  }
  return (MHmsgBase::send(toQid, sQid, len, time));
}


const Char *
DBselectAck::dump(Void) const
{
  Char name[DB_COL_VALUE_LEN + 1];
  Char value[DB_COL_VALUE_LEN + 1];
  Short nameLen;
  Short valueLen;
  static std::string str;

  str = std::string("") + "ackCode(" + int_to_str(ackCode) + ") sid(" +
     int_to_str(sid) + ") npairs(" + int_to_str(npairs) + ") endFlag(" +
     int_to_str(endFlag) + ") reqAck(" + int_to_str(reqAck) +
     ")\nackMesssage(" + ackMessage + ")\nnvlist:\n";

  const Char *p = nvlist;
  for (int i = 0; i < npairs; i++)
  {
    nameLen = *p++;

    strncpy(name, p, nameLen);
    name[nameLen] = '\0';
    p += nameLen;
    valueLen = *p++;

    strncpy(value, p, valueLen);
    value[valueLen] = '\0';
    p += valueLen;
    str = str + name + "(" + value + ")\n";
  }

  return (const Char *)str.c_str();
}
