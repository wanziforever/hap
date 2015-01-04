/*	@(#): <MID4678 () - 1/11/91, 2.1.1.4>
**
**	File:					MID4678
**	Release:				2.1.1.4
**	Date:					1/17/91
**	Time:					19:14:22
**	Newest applied delta:	06/23/03
**
** DESCRIPTION:
**		register with MSGH, get DBImhqid.
**
** OWNER:
**	eDB Team
**
** History:
**	Yeou H. Hwang
**	Sheng Zhou(06/23/03):Modified for f61286
** NOTES:
**		This routine is called by DBI main and DBI helper processes
*/


#include <stdio.h>
#include <String.h>
#include "cc/hdr/eh/EHhandler.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/db/DBretval.hh"
#include "DBI.hh"

#include "hdr/mydebug.h"

MHqid DBImhqid = MHnullQ;	// mhqid associated with process DBI or DBIhelper
MHqid DBIglbMhqid = MHnullQ;     // MH global ID with DBI or DBIhelper
EHhandler DBIeventHandler;

Short DBIregister(const Char * Pname, MHregisterTyp regTyp) {
  GLretVal rtn;		// MSGH function return value

  // attach to the MSGH subsystem
  if ((rtn = DBIeventHandler.attach()) != GLsuccess) {
    CRERROR("Unable to attach to MSGH, rtn(%d)", rtn);
    return DBFAILURE;
  }

  // register process name Pname and get a MSGH queue
  if ((rtn = DBIeventHandler.regName(Pname, DBImhqid, FALSE,
                                     FALSE, FALSE, regTyp)) != GLsuccess) {
    CRERROR("Unable to register for name %s", Pname);
    return DBFAILURE;
  }

  return DBSUCCESS;
}

Short DBIglobalReg(const Char *Pname) {
	if (DBIeventHandler.regGlobalName(Pname,  DBImhqid, DBIglbMhqid,
#ifdef LX
                                    TRUE, MH_systemGlobal) != GLsuccess)
#else
     TRUE) != GLsuccess)
#endif
        {
          return DBFAILURE;
        }
return DBSUCCESS;
}

Short DBIunRegister(const Char *Pname)
{
  if (Pname == 0)
  {
    return(GLfail);
  }

  std::string tmp_name = "_";

  tmp_name += Pname;
  // remove name process name Pname and detach MSGH queue
  if (DBIeventHandler.rmName(DBImhqid, tmp_name.c_str(), FALSE) != GLsuccess)
  {
    return(GLfail);
  }
    
  if (DBIeventHandler.detach() != GLsuccess)
  {
    return(GLfail);
  }

  return(DBSUCCESS);
}

Void DBItimerInit()
{
  DBIeventHandler.tmrInit(FALSE, DBItmrMax);

  // set cyclic timer for sanity pegging 
  DBIeventHandler.setlRtmr(DBIsanityTime, DBIsanityTmrTyp, TRUE);
}

