/*
**
**	File ID:	@(#): <MID7010 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7010
**	Release:				29.1.1.1
**	Date:					06/02/03
**	Time:					13:52:40
**	Newest applied delta:	06/23/03
**
** DESCRIPTION:
** 	Communicate with DBIhpr ,get and storeDBIhpr info into DBIhelper array.
**
** OWNER:
**	eDB Team
**
**
** History:
**	Yeou H. Hwang
**	Sheng Zhou (06/23/03):Updated for f61286.
**	Lucy Liang (11/18/03):Updated for f61284.
** NOTES:
*/

#include <string.h>
#include "hdr/GLtypes.h"
//#include "cc/hdr/db/DBassert.hh"
//#include "cc/hdr/db/DBdebug.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"

#include "DBIhpr.hh"
#include "DBIqryStat.hh"


#include "hdr/mydebug.h"

extern MHqid DBImhqid;

// Constructor: initialize helper table
DBIhelpersStatus::DBIhelpersStatus(const char *helperName) 
{
  Short i;
  MHqid xqid = MHnullQ;
  DBIqryStat qry;
  Short rtn;

  helperNo = 0;
  currentHelper = 0;

  if (strstr(helperName, "ora") != NULL)
  {
    strcpy(helperType, "Oracle");
  }
  else
  {
    strcpy(helperType, "PlatDB");
  }

  for (i = 0; i < DBImaxHprNo; i++)
  {
    if (i < 26)
    {
      sprintf(helpers[i].name, "%s%c", helperName, 'A' + i);
    }
    else
    {
      sprintf(helpers[i].name, "%sA%c", helperName, 'A' + (i - 26));
    }
    helpers[i].srcQue = MHnullQ;
    helpers[i].errPeg = 0;
    helpers[i].state = DBIhelperNotRdy;
    // get qid from MSGH
    if (MHmsgh.getMhqid(helpers[i].name, xqid) == GLsuccess) 
    {
      helpers[i].srcQue = xqid;
      helpers[i].state = DBIhelperStarting;
      helperNo++;
    }
    else
       CRDEBUG(DBinitFlg,("DBIhelpersStatus(): getMhqid failed: %s",
                          helpers[i].name));
  }

  // send helper query status packet
  for (i=0; i < DBImaxHprNo; i++)
  {
    if (helpers[i].srcQue != MHnullQ && 
        (rtn = qry.send(helpers[i].srcQue, DBImhqid, 0)) != GLsuccess)
    {
      //DBIGENLERROR(DBhprQRYinitFail, (DBhprQRYinitFailFmt, helpers[i].name, rtn));
      printf("DBhprQRYinitFail\n");
    }
  }
}


//Find the next available helper in a round robin way.
Short 
DBIhelpersStatus::getReadyHelper()
{
  CRDEBUG(DBinout, ("Enter getReadyHelper()"));
  if(helperNo == 0)
  {
    CRERROR("getReadyHelper(): No %s helper is available.", helperType);
    return -1;
  }

  Short guard = currentHelper;
  do
  {
    currentHelper = (++currentHelper) % DBImaxHprNo;
  } while(helpers[currentHelper].state != DBIhelperRdy && 
          currentHelper != guard);

  if (helpers[currentHelper].state != DBIhelperRdy)
  {
    CRDEBUG(DBinout, ("Exit getReadyHelper(). No ready helper"));
    return -1;
  }
  else
  {
    CRDEBUG(DBinout, ("Exit getReadyHelper(). %s is available",
                      helpers[currentHelper].name));
    return currentHelper;
  }
}


Void 
DBIhelpersStatus::setHelperStatus(DBIhprMsg* msgp)
{
  Short i;
  CRDEBUG(DBinout,("Enter setHelperStatus()"));

  for (i=0; i<DBImaxHprNo; i++)
  {
    if (strcmp(msgp->name, helpers[i].name) == 0)
    {
      helpers[i].srcQue = msgp->srcQue; // don't forget this 
      if (helpers[i].state == DBIhelperNotRdy)
         helperNo++;

      helpers[i].state = msgp->state;
      break;
    }
  }

  if (i == DBImaxHprNo)
  {
    CRERROR("setHelperStatus(): No such DBI helper: %s %s", 
            msgp->name,msgp->srcQue.display());
    return;
  }

  CRDEBUG(DBinout,("Exit setHelperStatus(), %s is set to %d",
                   helpers[i].name, msgp->state));
}

Void
DBIhelpersStatus::increaseHelperErrPeg(Short helperIndex, Short rtn)
{
  CRDEBUG(DBinout, ("Enter increaseHelperErrPeg(), %s's errPeg is %d",
                    helpers[helperIndex].name, helpers[helperIndex].errPeg));

  helpers[helperIndex].errPeg ++;
  if (helpers[helperIndex].errPeg >= DBI_MAXERRPEG)
  {
    //DBIGENLERROR(DBhlprSick, (DBhlprSickFmt, helpers[helperIndex].name, rtn));
    printf("DBhlprSick\n");
    helpers[helperIndex].state = DBIhelperBusy;
  }

  CRDEBUG(DBinout, ("Exit increaseHelperErrPeg(), %s's errPeg is %d",
                    helpers[helperIndex].name, helpers[helperIndex].errPeg));
}


//
// If DBI finds a helper is working for too long (>=90s)
// then mark the helper as sick and generate a debug message.
//
Void
DBIhelpersStatus::audit()
{
  CRDEBUG(DBinout, ("Enter audit(). %d %s helper(s) started",
                    helperNo, helperType));
  for (Short i = 0; i < DBImaxHprNo; i++)
  {
    CRDEBUG(DBaudit, ("audit(): %s status: %s", helpers[i].name, 
                      DBIhelperStatus[helpers[i].state]));
    if (helpers[i].state == DBIhelperWorking)
    {
      time_t now = time(0);
      time_t workingtime = now - helpers[i].startTime;

      CRDEBUG(DBaudit, ("audit(): %s has been busy for %d seconds",
                        helpers[i].name, workingtime));

      if (workingtime > 90)
      {
        CRDEBUG_PRINT(0, ("audit(): %s has been busy for %ld seconds.",
                          helpers[i].name, workingtime));
        helpers[i].state = DBIhelperBusy;
      }
    }

    if (helpers[i].state == DBIhelperBusy)
    {
      MHqid xqid = MHnullQ;

      if (MHmsgh.getMhqid(helpers[i].name, xqid) != GLsuccess)
      {
        CRDEBUG(DBaudit, ("audit(): %s has been removed from system",
                          helpers[i].name));
        helpers[i].state = DBIhelperNotRdy;
        helperNo--;
      }
      else
      {
        DBIqryStat qry;
        Short rtn;

        if ((rtn = qry.send(helpers[i].srcQue, DBImhqid, 0)) != GLsuccess)
        {
          helpers[i].state = DBIhelperBusy;
          CRDEBUG(DBaudit,
                  ("audit(): cannot query status of %s, rtn(%d)", 
                   helpers[i].name, rtn));
        }
      } 
    }
  }
  CRDEBUG(DBinout, ("Exit audit()"));
}

