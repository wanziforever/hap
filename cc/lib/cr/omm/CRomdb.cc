/*
**      File ID:        @(#): <MID13757 () - 08/17/02, 29.1.1.1>
**
**	File:					MID13757
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:16:31
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the definition of the CRomdb class,
**      as well as other classes needed for the OMDB.
** OWNER: 
**	Roger McKee
**	Yash Pal Gupta
**
** NOTES:
**
*/

#include <stdlib.h>
#include <dirent.h>
#include "CRomdb.H"
#include "cc/cr/hdr/CRomClEnt.H"
#include "CRomdbEnt.H"
#include "cc/db/config/dict/CRtable.H"
#include "cc/hdr/cr/CRspoolMsg.H"
#include "CRrcvOmdb.H"
#include "CRcsopMsg.H"
#include "cc/cr/hdr/CRshtrace.H"
#include "cc/cr/hdr/CRlogFile.H"
#include "CRdestEnt.H"
#include "CRcsopOMs.H"
#include "cc/cr/hdr/CRalarmMsg.H"
#include "cc/cr/hdr/CRomDest.H"
#include "cc/hdr/cr/CRsopRegMsg.H"
#include "cc/hdr/cr/CRassertIds.H"
#ifdef CC
#include "cc/hdr/init/INusrinit.h"
#include "hdr/GLportid.h"
#include "cc/hdr/cr/CRlocalLogMsg.H"
#include "limits.h"
#endif
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <fcntl.h> 

#ifdef EES
#include "cc/hdr/cr/CRvpath.H"
#endif


#ifndef EES
#include "cc/hdr/ft/FTalarm.H"
#endif

#include "cc/hdr/sa/SAresult.H"
#include "cc/hdr/sa/SAplatFunc.H"

extern Bool CRsetUniqueAlarm;
extern Bool CRdoCentLogs;

//extern char* itoa(unsigned int);
//extern void  reverse();

Bool CRisSpaName(const Char* procName);
Bool CRisSpaProcess(Char *procName, Char *spaName);
 
CRomdb::CRomdb()
{
	setDefaults();
}

void
CRomdb::setDefaults()
{
	validFlag = NO;
	omdbTblPtr = NULL;
	classTblPtr = NULL;
	destTblPtr = NULL;
	x733omdbTblPtr = NULL;
	x733reptTblPtr = NULL;
	alarmCodeTblPtr = NULL;
}

Bool
CRomdb::isValid() const
{
	return validFlag;
}

/* 
 * Called when the system is booted or when the system goes through
 * a system reset initialization. Allocate a shared memory segment and 
 * a message queue if they do not exist. In any case, the routine will
 * free all the allocated queues, if any, and initialize the 
 * MSGH control information structure and routing table.
 * (-1) is returned when it failed.
 */
Short
CRomdb::sysinit()
{
	setDefaults();
	return 0;
}

GLretVal
CRomdb::insertDest(const char* dest, const char* logicalDev,
                   const char* channelType, int logsz, 
                   int numOfLogs, const char* daily, int logAge)
{
	return destTblPtr->insertDest(dest, logicalDev, channelType, logsz,
	                              numOfLogs, daily, logAge);
}
GLretVal
CRomdb::updateDest(const char* dest, const char* logicalDev,
                   const char* channelType, int logsz,
                   int numOfLogs, const char* daily, int logAge)
{
	return destTblPtr->updateDest(dest, logicalDev, channelType, logsz,
	                              numOfLogs, daily, logAge);
}

CRdestEntryPtr
CRomdb::getDestEntry(const char* destName)
{
	if (destTblPtr)
     return destTblPtr->getDestEntry(destName);

	return NULL;
}

void
CRomdb::regMsg(CRsopRegMsg* msg)
{
	if (destTblPtr == NULL)
     return;
	CRdestEntry* destPtr = destTblPtr->getDestEntry(msg->getSOPname());
	if (destPtr)
     destPtr->sopRegChange(msg->isRegistering());
}

void
CRomdb::loadFail(const char* tableName)
{
	CRcsopMsg om;
	om.spool(&CRloadFailOM, tableName);
}

GLretVal
CRomdb::init()
{
	validFlag = NO;

	GLretVal retval = DBsetUp("CSOPDBQ");
	if (retval != GLsuccess)
	{
		CRcsopMsg om;
		om.spool(&CRnoAccessDbOM);
	}
	else
	{
		retval = initDB();
	}
	DBclose();

	if (retval == GLsuccess)
     validFlag = YES;

#ifdef DEBUG
	dump();
#endif
	
	return retval;
}

GLretVal
CRomdb::initDB()
{
	destTblPtr = new CRdestTbl;
	if (destTblPtr == NULL)
	{
		CRERROR("new() failed");
		return GLfail;
	}

	if (destTblPtr->load() != GLsuccess)
	{
		loadFail("cr_dest");
		return GLfail;
	}

	classTblPtr = new CRomClassTbl;
	if (classTblPtr == NULL)
	{
		CRERROR("new() failed");
		return GLfail;
	}

	if (classTblPtr->load() != GLsuccess)
	{
		loadFail("cr_msgcls");
		return GLfail;
	}

	x733omdbTblPtr = new CRx733omdbTbl;
	if (x733omdbTblPtr == NULL)
	{
		CRERROR("new() failed");
		return GLfail;
	}

	if (x733omdbTblPtr->load() != GLsuccess)
	{
		loadFail("cr_x733outmsg");
		return GLfail;
	}

	x733reptTblPtr = new CRx733reptTbl;
	if (x733reptTblPtr == NULL)
	{
		CRERROR("new() failed");
		return GLfail;
	}

	if (x733reptTblPtr->load() != GLsuccess)
	{
		loadFail("cr_x733rept");
		return GLfail;
	}

	alarmCodeTblPtr = new CRalarmCodeTbl;
	if (alarmCodeTblPtr == NULL)
	{
		CRERROR("new() failed for CR_ALARMCODE table");
		//return GLfail;
	}

	if (alarmCodeTblPtr->load() != GLsuccess)
	{
		loadFail("cr_alarmcode");
		//return GLfail;
	}
#ifdef DEBUG
	else
	{
		alarmCodeTblPtr->dump();
	}
#endif

	omdbTblPtr = new CRomdbTbl;
	if (omdbTblPtr == NULL)
	{
		CRERROR("new() failed");
		return GLfail;
	}

	if (omdbTblPtr->load() != GLsuccess)
	{
		loadFail("cr_outmsg");
		return GLfail;
	}

	return GLsuccess;
}

void
CRomdb::generateISmsg()
{
	CRcsopMsg om;
	om.spool(&CRcsopIsOM);
}

#ifdef CC
void
CRomdb::procLocalLogs()
{

/*
**	Copy /sn/log/locallogs /[process][number] to 
**	OMlog files. BUT only do log files that are less then
**	2 days old.
*/
	struct stat      fmtFileStat;
	static time_t    CurrentTime;
	struct tm        *cLocalTime;

	const time_t TWODAYS=(2 * (24 * 60 *60));

	int fd = 0;

	CurrentTime = time( (time_t *)0 );
	cLocalTime = localtime( &CurrentTime );

	DIR *fdir = NULL;
	struct dirent *dirPtr;

	CRlocalLogSop initLog;

	char fileName[50];
	char tmpath[70];
	char tmpath2[80];
	char tmpath3[80];
	char fullFileName[130];

  strcpy(tmpath, CRLCLLOGDIR);
  strcpy(tmpath2,CRDEFACTIVELOCLOGDIR);
  strcpy(tmpath3,CRDEFACTIVELOGDIR);

  fdir = opendir(tmpath);

	if (fdir == NULL)
     return;

	while (( dirPtr = readdir(fdir)) != NULL)
	{

		sprintf(fullFileName,"%s/%s",tmpath, dirPtr->d_name);
		sprintf(fileName,"%s", dirPtr->d_name);
			
		if ((fd = open(fullFileName, 0)) != -1)
		{
      if ((fstat(fd, &fmtFileStat)) == 0)
      {
        /*
         *  Determine if the current log file is too old (more than
         *  2 days).  
         */

        if((CurrentTime - fmtFileStat.st_mtime) > TWODAYS ||
           (strcmp(fileName,".")) == 0                  ||
           (strcmp(fileName,"..")) == 0 )
        {
          close(fd);
          initLog.deleteFile();
        }
        else
        {
          close(fd);
          fileName[strlen(fileName)-1] = '\0';
          processLogFile(fileName,tmpath);
        }
      }//end if fstat
      else
      {
        close(fd);
      }
    }//end open

	}// end of while loop
	closedir(fdir);

  fdir = opendir(tmpath2);

	if (fdir == NULL)
     return;

	while (( dirPtr = readdir(fdir)) != NULL)
	{

		sprintf(fullFileName,"%s/%s",tmpath2, dirPtr->d_name);
		sprintf(fileName,"%s", dirPtr->d_name);
			
		if ((fd = open(fullFileName, 0)) != -1)
		{
      if ((fstat(fd, &fmtFileStat)) == 0)
      {
        /*
         *  Determine if the current log file is too old (more than
         *  2 days).  
         */

        if((CurrentTime - fmtFileStat.st_mtime) > TWODAYS ||
           (strcmp(fileName,".")) == 0                  ||
           (strcmp(fileName,"..")) == 0 )
        {
          close(fd);
          initLog.deleteFile();
        }
        else
        {
          close(fd);
          fileName[strlen(fileName)-1] = '\0';
          processLogFile(fileName,tmpath2);
        }
      }//end if fstat
      else
      {
        close(fd);
      }
    }//end open

	}// end of while loop
	closedir(fdir);

  fdir = opendir(tmpath3);

	if (fdir == NULL)
     return;

	while (( dirPtr = readdir(fdir)) != NULL)
	{

		sprintf(fullFileName,"%s/%s",tmpath3, dirPtr->d_name);
		sprintf(fileName,"%s", dirPtr->d_name);
			
		if ((fd = open(fullFileName, 0)) != -1)
		{
      if ((fstat(fd, &fmtFileStat)) == 0)
      {
        /*
         *  Determine if the current log file is too old (more than
         *  2 days).  
         */

        if((CurrentTime - fmtFileStat.st_mtime) > TWODAYS ||
           (strcmp(fileName,".")) == 0                  ||
           (strcmp(fileName,"..")) == 0 )
        {
          close(fd);
          initLog.deleteFile();
        }
        else
        {
          if (strncmp(fileName, "OMlog",5) == 0 ||
              strncmp(fileName, "debug",5) == 0)
          {
            close(fd);
            fileName[strlen(fileName)-1] = '\0';
            processLogFile(fileName,tmpath3);
          }
          else
          {
            close(fd);
          }

        }
      }//end if fstat
      else
      {
        close(fd);
      }
    }//end open

	}// end of while loop
	closedir(fdir);

	return;

}
#endif

/* 
 * Called during process initialization.
 * (-1) is returned when it failed.
 */
Short
CRomdb::procinit(SN_LVL init_lvl)
{
	Short retval = init();

	if (retval != GLsuccess)
     return GLfail;

#ifdef CC
	if(init_lvl!=SN_LV4)
	{
		procLocalLogs();
		generateISmsg();
	}
#else
	generateISmsg();
#endif

	return GLsuccess;
}

short
CRomdb::cleanup()
{
	/* need to also make sure each CRomdbEntry gets deleted */
	if (omdbTblPtr != NULL)
	{
		delete omdbTblPtr; omdbTblPtr = NULL;
	}
	if (classTblPtr != NULL)
	{
		delete classTblPtr; classTblPtr = NULL;
	}
	if (destTblPtr != NULL)
	{
		delete destTblPtr; destTblPtr = NULL;
	}
	if (x733omdbTblPtr != NULL)
	{
		delete x733omdbTblPtr; x733omdbTblPtr = NULL;
	}
	if (x733reptTblPtr != NULL)
	{
		delete x733reptTblPtr; x733reptTblPtr = NULL;
	}
	if (alarmCodeTblPtr != NULL)
	{
		delete alarmCodeTblPtr; alarmCodeTblPtr = NULL;
	}
	return 0;
}

/*
 * Get an OMDB entry with the given OM name
 */
const CRomdbEntry*
CRomdb::getEntry(const char* name)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return NULL;
	}
	
	return omdbTblPtr->getMsgEntry(name);
}


/*
 * Delete the OMDB entry with the given OM name
 */
GLretVal
CRomdb::deleteEntry(const char* name)
{
	return omdbTblPtr->deleteEntry(name);
}

GLretVal
CRomdb::x733deleteEntry(const char* name)
{
	return x733omdbTblPtr->deleteEntry(name);
}

/*
 * Replace the an old OMDB entry with a new one
 * This is called when an OMDB entry is modified with RC/V
 */
GLretVal
CRomdb::replaceEntry(const char* name, CRomdbEntryPtr newEntry)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return GLfail;
	}
	
	return omdbTblPtr->replaceEntry(name, newEntry);
}

GLretVal
CRomdb::x733replaceEntry(const char* name, CRx733omdbEntryPtr newEntry)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return GLfail;
	}
	
	return x733omdbTblPtr->replaceEntry(name, newEntry);
}

/*
 * Need to reread the ORACLE table and compare it with in-memory data
 */
Void
CRomdb::audit()
{
}

void
CRomdb::timeChange()
{
	destTblPtr->timeChange();
}

// Print the contents of the OMDB table
Void
CRomdb::dump()
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return;
	}
	
	destTblPtr->dump();
	classTblPtr->dump();
	omdbTblPtr->dump();
}

/* 
** insert a new OMDB entry into the table.
** This is called when a new OMDB entry is created with RC/V
*/
GLretVal 
CRomdb::insertEntry(const char* name, CRomdbEntryPtr newEntry)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return GLfail;
	}
	
	return omdbTblPtr->insertEntry(name, newEntry);
}

GLretVal 
CRomdb::x733insertEntry(const char* name, CRx733omdbEntryPtr newEntry)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return GLfail;
	}
	
	return x733omdbTblPtr->insertEntry(name, newEntry);
}

CRomdbEntryPtr
CRomdb::getEntry(const char* msgName, const char* senderName,
                 const char*& format)
{
	if (!isValid())
	{
		CRERROR("OMDB not initialized");
		return NULL;
	}
	
	CRomdbEntryPtr msgEntry = omdbTblPtr->getMsgEntry(msgName);

	/* look up msgName in OMDB */
	if (msgEntry == NULL)
	{
		CRCFTASSERT(CRmissingOmId,
                (CRmissingOmFmt, msgName, senderName));
		return NULL;
	}

	/* format the msg according to the OMDB */
	format = msgEntry->getFormat();
	return msgEntry;
}

/* figure out which msg class to use,
** the one in the msg or the one in OMDB
*/
const char*
CRomdb::getClassStr(CRomdbEntryPtr msgEntry, const char* userMsgClass)
{
	static const char* CRNULLSTR = "";
	const char* retval = CRNULLSTR;

	if (userMsgClass != NULL && *userMsgClass != '\0')
     retval = userMsgClass;
	else if (msgEntry)
	{
		retval = msgEntry->getMsgClass();
		if (retval == NULL)
       return CRNULLSTR;
	}
		
	return retval;
}

/* figure out which msg class to use,
** the one in the msg or the one in OMDB
*/
CRomClassEntry*
CRomdb::getClassEntry(CRomdbEntryPtr msgEntry, const char* userMsgClass)
{
	CRomClassEntry* retval = NULL;

	if (*userMsgClass != '\0')
     retval = getClassEntry(userMsgClass);
	else if (msgEntry)
     retval = msgEntry->getMsgClassPtr();
		
	return retval;
}

CRomClassEntry*
CRomdb::getClassEntry(const char* msgclass)
{
	if (classTblPtr)
     return classTblPtr->getClassEntry(msgclass);
	return NULL;
}

CRomClassEntry*
CRomdb::insertMsgClass(const char* msgclass)
{
	return classTblPtr->insertMsgClass(msgclass);
}

void
CRomdb::delClassEntry(const char* msgclass)
{
	classTblPtr->delClassEntry(msgclass);
}

CRALARMLVL
CRomdb::getAlarmLvl(CRALARMLVL userAlarmLvl, CRomdbEntry* msgEntry)
{
	if (userAlarmLvl != POA_DEFAULT)
	{
		return userAlarmLvl;
	}

	if (msgEntry)
	{
		return msgEntry->getAlarmLevel();
	}

	return POA_INF;
}

const char*
CRomdb::getAlarmStr(CRALARMLVL userAlarmLvl, CRomdbEntry* msgEntry)
{
	/* figure out what the alarm string should be */
	
	static char alarmstr[CRALARMSTRMAX+1] = CRDEFALMSTR;

	if (userAlarmLvl != POA_DEFAULT)
	{
		strcpy(alarmstr, CRPOA_to_str(userAlarmLvl));
	}
	else
	{
		if (msgEntry)
		{
			strcpy(alarmstr, CRPOA_to_str(msgEntry->getAlarmLevel()));
		}
	}
	return alarmstr;
}

void
CRinformAlarm(const char* alarmStr)
{
	if (*alarmStr++ != '*')
     return;

	CRALARMLVL alarm;

	switch (*alarmStr)
	{
  case 'C':
		alarm = POA_CRIT;
		break;
  case '*':
		alarm = POA_MAJ;
		break;
  default:
		alarm = POA_MIN;
		break;
	}

	CRalarmMsg alarmIntf;
	alarmIntf.soundAlarm(alarm);

	//
	// update the ALARM on the Netra box/low cost eCS box
	//
#ifndef EES
	GLretVal FTretVal = FTalarmSet(alarm);
	if(FTretVal != GLsuccess)
	{
		CRDEBUG(CRusli,("Problem setting the ALARM, retval = %d",FTretVal));
	}
#endif
}

/* formats and routes a CRrcvOmdbMsg (result of CRomdbMsg) */
void
CRomdb::route(CRrcvOmdbMsg& msg)
{

	//change these hardcode values !!!
	char  pCauseValue[64];
	char  alarmTypeValue[30];
  char  objectName[CRMAX_OBJNAME_SZ];
	char  specProb[CRMAX_SPECPROB_SZ];
  char  addText[CRMAX_ADDTEXT_SZ];

	CRX733AlarmProbableCause pCause;
	CRX733AlarmType          alarmType;

	const char*  pCausePtr = pCauseValue;
	const char*  alarmTypePtr = alarmTypeValue;
	const char*  objectNamePtr = objectName;
	const char*  specProbPtr = specProb;
	const char*  addTextPtr = addText;

	pCauseValue[0] = 0;
	alarmTypeValue[0] = 0;
	objectName[0] = 0;
	specProb[0] = 0;
	addText[0] = 0;

	if (isValid() == NO)
	{
		/* inform ALARM process, if alarm 
		** is CRITICAL, MAJOR, or MINOR
		*/
		CRinformAlarm(msg.getAlarmStr());

		if(msg.getAlarmLevel() == POA_TCR)
       CRinformAlarm("*C");
		if(msg.getAlarmLevel() == POA_TMJ)
       CRinformAlarm("**");
		if(msg.getAlarmLevel() == POA_TMN)
       CRinformAlarm("*");

		limpRoute(msg);
		return;
	}

	/* get msgName from CRrcvOmdbMsg */
	const char* msgName = msg.getMsgName();

  CRx733omdbEntryPtr x733msgEntryPtr = x733omdbTblPtr->getMsgEntry(msgName);
  if( x733msgEntryPtr != NULL )
  {
		if( msg.getAlarmType() == -1 )
		{
			strcpy(alarmTypeValue,x733msgEntryPtr->getAlarmType());
			alarmType=setAlarmTypeEnumStr((String)x733msgEntryPtr->getAlarmType());

		}
		else
		{
			strcpy(alarmTypeValue,msg.getAlarmTypeValue());
			alarmType=msg.getAlarmType();
		}

		if( msg.getProbableCause() == -1 )
		{
			strcpy( pCauseValue,x733msgEntryPtr->getProbableCause());
			pCause=setProbableCauseEnumStr((String)x733msgEntryPtr->getProbableCause());
		}
		else
		{
			strcpy( pCauseValue,msg.getProbableCauseValue());
			pCause=msg.getProbableCause();
		}

		if( strcmp("",msg.getAlarmObjectName()) == 0 )
		{
#ifdef LX
			if( strcmp("TRAPGW", msg.getSenderName()) == 0 )
			{
				strcpy(objectName,x733msgEntryPtr->getAlarmObjectName());
			}
			else
			{
				snprintf(objectName, sizeof(objectName),"%s#%s",
                 msg.getSenderMachine(),
                 x733msgEntryPtr->getAlarmObjectName());
			}
#else
			strcpy(objectName,x733msgEntryPtr->getAlarmObjectName());
#endif
		}
		else
		{
#ifdef LX
			if( strcmp("TRAPGW", msg.getSenderName()) == 0 )
			{
				strcpy(objectName,msg.getAlarmObjectName());
			}
			else
			{
				snprintf(objectName, sizeof(objectName),"%s#%s",
                 msg.getSenderMachine(),
                 msg.getAlarmObjectName());
			}
#else
			strcpy(objectName,msg.getAlarmObjectName());
#endif
		}

		if( strcmp("",msg.getSpecificProblem()) == 0 )
       strcpy(specProb,x733msgEntryPtr->getSpecificProblem());
		else
       strcpy(specProb,msg.getSpecificProblem());

		if( strcmp("",msg.getAdditionalText()) == 0 )
       strcpy(addText,x733msgEntryPtr->getAdditionalText());
		else
       strcpy(addText,msg.getAdditionalText());

    CRDEBUG(CRusli,(" pCause = %d alarmType = %d objectName = %s specProb = %s addText = %s", pCause,alarmType,objectName,specProb,addText));

  }
  else
  {
    alarmType=msg.getAlarmType();
    pCause=msg.getProbableCause();

    strcpy(alarmTypeValue,msg.getAlarmTypeValue());
    strcpy( pCauseValue,msg.getProbableCauseValue());
#ifdef LX
    if( strcmp("TRAPGW", msg.getSenderName()) == 0 )
    {
      strcpy(objectName,msg.getAlarmObjectName());
    }
    else
    {
      snprintf(objectName, sizeof(objectName),"%s#%s",
               msg.getSenderMachine(), 
               msg.getAlarmObjectName());
    }
#else
    strcpy(objectName,msg.getAlarmObjectName());
#endif
    strcpy(specProb,msg.getSpecificProblem());
    strcpy(addText,msg.getAdditionalText());
  }

	/* look up OMDB entry */
	const char* formatstr = NULL;
	CRomdbEntryPtr msgEntryPtr = getEntry(msgName, msg.getSenderName(),
                                        formatstr);

	/* figure out what the alarm string should be */
	CRALARMLVL almLevel = getAlarmLvl(msg.getAlarmLevel(), msgEntryPtr);
	const char* alarmstr = CRPOA_to_str(almLevel);

	/* inform ALARM process, if alarm is CRITICAL, MAJOR, or MINOR */
	CRinformAlarm(alarmstr);

	if(almLevel == POA_TCR)
     CRinformAlarm("*C");
	if(almLevel == POA_TMJ)
     CRinformAlarm("**");
	if(almLevel == POA_TMN)
     CRinformAlarm("*");

	const char* msgclass = getClassStr(msgEntryPtr, msg.getMsgClass());


	if(CRsetUniqueAlarm) 
	{
		if (strncmp(alarmstr, "*", 1) == 0 ||
		    strncmp(alarmstr, "T", 1) == 0 ||
		    strncmp(alarmstr, "CL", 2) == 0)
		{
			//Fill in any %s etc.
			msg.fillInFormattedMsg();
			String theAddText;
			theAddText=addText;
			createOmdbMsgAlarmCode(msgName, msg.getFormattedTextMsg());
#ifdef LX
			int CRalarmCodeSize=17; //(PLATFORM)8 + (;)1 + (CRalarmCode)8
#else
			int CRalarmCodeSize=8;
#endif

      if ( (strcmp("TRAPGW", msg.getSenderName())) != 0 )
      {
        if(strlen(addText)<(CRMAX_ADDTEXT_SZ - CRalarmCodeSize))
        {
          theAddText=theAddText + ";" + CRalarmKey;
        }
        else
        {
          theAddText=theAddText(0, (CRMAX_ADDTEXT_SZ-CRalarmCodeSize)) + String(";") + CRalarmKey;
        }
        strcpy(addText,theAddText);
      }
		}
	}

	/*x733 stuff */
	msg.x733Fields(pCausePtr,alarmTypePtr,objectNamePtr,
                 specProbPtr,addTextPtr);

	/* format the message */
	msg.format(alarmstr, formatstr, msgclass);
	
	/* for each destination
	**     tell msg to print itself out to the file description
	*/

	const char* finalMsg = msg.getFormattedMsg();
	int msgLen = strlen(finalMsg);

	/* figure out which msg class to use,
	** the one in the msg or the one in OMDB
	*/
	CRomClassEntryPtr classEntry = getClassEntry(msgEntryPtr,
                                               msg.getMsgClass());

	if (!classEntry)
	{
		if (msgclass && *msgclass != '\0' &&
		    strcmp(msgclass, "ASRT") != 0)
		{
			CRCFTASSERT(CRmissingMsgclassId,
                  (CRmissingMsgclassFmt, msgclass,
                   msg.getSenderName(), msgName));
		}
		limpRoute(msgclass, finalMsg, msgLen, almLevel);
		return;
	}

	Bool skipOriginator = NO;

	const char* originatorName = msg.getUSLIname();
	if (originatorName && *originatorName == '\0')
     originatorName = NULL;
	else
	{ /* do not send back to originator if the msgclass is INECHO */
		if (msgclass && strcmp(msgclass, "INECHO") == 0)
       skipOriginator = YES;
	}

	int numDests = classEntry->getNumDests();
	for (int i = 0; i < numDests; i++)
	{
    //
    //      Only send ALARMED and CLEAR OMs to PLATAGT/MOFPROC
    //
		if(((strcmp(classEntry->getDestName(i),"PLATAGT"))==0) ||
       (strcmp(classEntry->getDestName(i), "MOFPROC")) == 0)
		{
			if (strncmp(alarmstr, "*", 1) == 0 ||
			    strncmp(alarmstr, "T", 1) == 0 ||
			    strncmp(alarmstr, "CL", 2) == 0)
			{
				// continue
			}
			else
			{
				continue; // get next dest
			}
		}

		const CRdestEntryPtr destEntryPtr = classEntry->getDestPtr(i);
		CRomDest* sopPtr = destEntryPtr->getOMdestPtr();
		if (sopPtr)
		{
			/* If originating process is the same as the 
			** destination about to send to,
			** then set orginatorName to NULL to indicate
			** that CSOP does not need to specially send a OM to
			** the originator.
			*/
			if (originatorName &&
			    strcmp(originatorName,
                 classEntry->getDestName(i)) == 0)
			{
				originatorName = NULL;

				/* if the originator should not see the OM
				** at all, then skip it.
				*/
				if (skipOriginator == YES)
           continue;
			}
			sopPtr->setAlarmType((CRX733AlarmType)alarmType);
			sopPtr->setProbableCause((CRX733AlarmProbableCause)pCause);
			sopPtr->setAlarmObjectName(objectName);
			sopPtr->setSpecificProblem(specProb);
			sopPtr->setAdditionalText(addText);
			sopPtr->send(finalMsg, msgLen, almLevel);

		}
	}

	/* Also send a copy of the OM to the originator,
	** if it has not already done so.
	*/
	if (originatorName && skipOriginator == NO)
	{
		CRdestEntry* origDest = getDestEntry(originatorName);
		if (origDest)
		{
			CRomDest* sopPtr2 = origDest->getOMdestPtr();
			if (sopPtr2)
			{
				sopPtr2->setAlarmType((CRX733AlarmType)alarmType);
				sopPtr2->setProbableCause((CRX733AlarmProbableCause)pCause);
				sopPtr2->setAlarmObjectName(objectName);
				sopPtr2->setSpecificProblem(specProb);
				sopPtr2->setAdditionalText(addText);
				sopPtr2->send(finalMsg, msgLen, almLevel);
			}
		}
		else
		{

			CRomDest *origdest = CRomDest::makeOriginatorDest();
			origdest->init(originatorName);
			origdest->setAlarmType((CRX733AlarmType)alarmType);
			origdest->setProbableCause((CRX733AlarmProbableCause)pCause);
			origdest->setAlarmObjectName(objectName);
			origdest->setSpecificProblem(specProb);
			origdest->setAdditionalText(addText);
			origdest->send(finalMsg, msgLen, almLevel);

			delete origdest;
		}
	}
}

static CROMclass CRdebugMclass(CL_DEBUG);
static CROMclass CRassertMclass(CL_ASSERT);

void
CRomdb::limpRoute(const char* msgclass, const char* text, int textlen,
                  CRALARMLVL almLevel)
{
	CRomDest* daylogPtr = CRomDest::getDayLogPtr();
	if (daylogPtr)
     daylogPtr->send(text, textlen, almLevel);

	if (CRdebugMclass != msgclass && CRassertMclass != msgclass)
	{
		CRomDest* ropSopPtr = CRomDest::getROPsopPtr();
		if (ropSopPtr)
       ropSopPtr->send(text, textlen, almLevel);
	}
}

void
CRomdb::limpRoute(CRrcvOmdbMsg& msg)
{
	const char* msgclass = msg.getMsgClass();

	msg.format(msg.getAlarmStr(), NULL, msgclass);
	const char* finalMsg = msg.getFormattedMsg();
	int len = strlen(finalMsg);

	limpRoute(msgclass, finalMsg, len,
            getAlarmLvl(msg.getAlarmLevel(), NULL));
}

void
CRomdb::limpRoute(CRspoolMsg* msg)
{
	const char* msgclass = msg->getClass();

	if (CRdebugMclass != msgclass && CRassertMclass != msgclass)
	{
		msg->sendToSop(CRomDest::getDayLogPtr(),
                   CRomDest::getROPsopPtr());
    /*
    **	If msgclass is NULL send a ASSERT with the message
    */
		if((strcmp(msgclass,"")) == 0 || msgclass == '\0')
    {
			CRCFTASSERT(CRMsgclassIdNull,(CRMsgclassIdNullFmt));
		}
	}
	else
     msg->sendToSop(CRomDest::getDayLogPtr());
}

/* formats and routes a CRspoolMsg (result of CRmsg)
** This should be expanded to use the actual destinations lists
** and somehow include the CRomInfo stuff with the originating USLI
** process name also.
*/
void
CRomdb::route(CRspoolMsg* msg)
{
	String theOMkey = msg->getOMkey();
	//Bool sendToLeadCSOP=FALSE;

	msg->setOMkey(" ");

	CRinformAlarm(msg->getAlarmLevel());
	const char* tmpAlarmstr = msg->getAlarmLevel();

	if((strncmp(tmpAlarmstr,"TC",2)) == 0) 
     CRinformAlarm("*C");

	if((strncmp(tmpAlarmstr,"TJ",2)) == 0)
     CRinformAlarm("**");

	if((strncmp(tmpAlarmstr,"TN",2)) == 0)
     CRinformAlarm("*");

	if (isValid() == NO)
	{
		limpRoute(msg);
		return;
	}

	const char* msgclass = msg->getClass();

	/* figure out which msg class to use,
	** the one in the msg or the one in OMDB
	*/
	CRomClassEntryPtr classEntry = getClassEntry(msgclass);

	if (!classEntry)
	{
		if (msgclass && *msgclass != '\0' &&
		    strcmp(msg->getSenderName(), "CSOP") == 0 &&
		    strcmp(msgclass, "ASRT") != 0)
		{
			CRCFTASSERT(CRmissingMsgclassId,
                  (CRmissingMsgclassFmt, msgclass,
                   msg->getSenderName(), "CRmsg"));
		}
		limpRoute(msg);
		return;
	}

	Bool skipOriginator = NO;

	const char* originatorName = msg->getUSLIname();
	if (originatorName && *originatorName == '\0')
     originatorName = NULL;
	else
	{ /* do not send back to originator if the msgclass is INECHO */
		if (msgclass && strcmp(msgclass, "INECHO") == 0)
       skipOriginator = YES;
	}

	const int MAXSOPS = 10;
	CRomDest* soparray[MAXSOPS];
	for (int x = 0; x < MAXSOPS; x++)
     soparray[x] = NULL;

	CRomDest* MOFsoparray[2];
	for (int x = 0; x < 2; x++)
     MOFsoparray[x] = NULL;

	if (CRsetUniqueAlarm &&
	    (strncmp(tmpAlarmstr, "*", 1) == 0 ||
	     strncmp(tmpAlarmstr, "T", 1) == 0 ||
	     strncmp(tmpAlarmstr, "CL", 2) == 0))
	{
		String alarmCode;
		char addText[CRMAX_ADDTEXT_SZ];
		char sender[100];
		char omkey[8];

		strncpy(addText, msg->getAdditionalText(), sizeof(addText) - 1);
		addText[sizeof(addText)-1]='\0';
		strncpy(sender, msg->getSenderName(), sizeof(sender) - 1);
		sender[sizeof(sender)-1]='\0';

		// Handle code added to Additional Text field by INIT subsystem.
		// The end should be ";nnn" where nnn is a 3-digit number.
		Bool isFromINIT = FALSE;
		char* pos = strchr(addText, ';');
		if (pos != 0 &&
		    isdigit(pos[1]) && isdigit(pos[2]) && isdigit(pos[3]))
		{
			strcpy(omkey, subsystemCode("IN"));
			strncat(omkey, pos + 1, 3);
			omkey[5] = '\0';
			isFromINIT = TRUE;

			// delete ';' and the 3-digit number from
			// AdditionalText
			*pos = '\0';
		}
		else
		{
			strncpy(omkey, theOMkey, sizeof(omkey));
			omkey[sizeof(omkey) - 1] = '\0';
		}

		createSpoolMsgAlarmCode(omkey, sender, alarmCode, isFromINIT);

#ifdef LX
		if(strcmp("TRAPGW", msg->getSenderName()) != 0) // no match to TRAPGW
		{
      // truncate additional text if it is longer than CRMAX_ADDTEXT_SZ - 17
      if (strlen(addText) >= (CRMAX_ADDTEXT_SZ - 17))
      {
        addText[CRMAX_ADDTEXT_SZ - 17] = '\0';
      }

      // add semi-colon and alarm code at the end
      strcat(addText, ";");
      strcat(addText, alarmCode);
		}
#else
		// truncate additional text if it is longer than CRMAX_ADDTEXT_SZ - 8
		if (strlen(addText) >= (CRMAX_ADDTEXT_SZ - 8))
		{
			addText[CRMAX_ADDTEXT_SZ - 8] = '\0';
		}

		// add semi-colon and alarm code at the end
		strcat(addText, ";");
		strcat(addText, alarmCode);
#endif
		msg->setAdditionalText(addText);
	}

	int numSOPs = 0;
	int numDests = classEntry->getNumDests();
	for (int i = 0; i < numDests; i++)
	{
    //
    //      Only send ALARMED/CLEAR OMs to PLATAGT/MOFPROC
    //
		if(((strcmp(classEntry->getDestName(i),"PLATAGT"))==0) ||
		   ((strcmp(classEntry->getDestName(i),"MOFPROC"))==0))
		{
			const char* alarmstr = msg->getAlarmLevel();

			if (strncmp(alarmstr, "*", 1) == 0 ||
			    strncmp(alarmstr, "T", 1) == 0 ||
			    strncmp(alarmstr, "CL", 2) == 0)
			{
				const CRdestEntryPtr destEntryPtr = classEntry->getDestPtr(i);
				CRomDest* MOFsopPtr = destEntryPtr->getOMdestPtr();
				if (MOFsopPtr)
				{
          /* If originating process is the same as the 
          ** destination about to send to,
          ** then set orginatorName to NULL to indicate
          ** that CSOP does not need to specially 
          ** send a OM to the originator.
          */
          if (originatorName &&
              strcmp(originatorName,
                     classEntry->getDestName(i)) == 0)
          {
            originatorName = NULL;

            /* if the originator should not 
            ** see the OM at all, then skip it.
            */
            if (skipOriginator == YES)
               continue;
          }

          MOFsoparray[0] = MOFsopPtr;
				}//end if MOFsopPtr
				
				//send to MOFPROC
				msg->sendToSop(MOFsoparray, 1, 0);

				// Skip MOFPROC sent already
				continue;
        // end if its ALARMED or CLEAR
			}
			else
			{
				continue; // get next dest
        // Skip MOFPROC
			}
		}

		const CRdestEntryPtr destEntryPtr = classEntry->getDestPtr(i);
		CRomDest* sopPtr = destEntryPtr->getOMdestPtr();

		if (sopPtr)
		{
			if (numSOPs == MAXSOPS)
			{
				CRERROR("too many destinations (max=%d)",
                MAXSOPS);
			}
			else
			{
				/* If originating process is the same as the 
				** destination about to send to,
				** then set orginatorName to NULL to indicate
				** that CSOP does not need to specially 
				** send a OM to the originator.
				*/
				if (originatorName &&
				    strcmp(originatorName,
                   classEntry->getDestName(i)) == 0)
				{
					originatorName = NULL;

					/* if the originator should not 
					** see the OM at all, then skip it.
					*/
					if (skipOriginator == YES)
             continue;
				}

				soparray[numSOPs++] = sopPtr;
			}
		}
	}

	//set x733 to OFF
	if(x733reptTblPtr->getX733Report() == FALSE)
	{
		msg->setAlarmType((CRX733AlarmType)alarmTypeUndefined);
	}

	/* Also send a copy of the OM to the originator,
	** if it has not already done so.
	*/
	CRomDest* origSopPtr = NULL;

	if (originatorName && skipOriginator == NO)
	{
		CRomDest* sopPtr2 = NULL;

		CRdestEntry* origDest = getDestEntry(originatorName);

		if (origDest)
		{
			sopPtr2 = origDest->getOMdestPtr();
		}
		else
		{
			origSopPtr = CRomDest::makeOriginatorDest();
			origSopPtr->init(originatorName);
			sopPtr2 = origSopPtr;
		}
		if (sopPtr2)
		{
			if (numSOPs == MAXSOPS)
			{
				CRERROR("too many destinations (max=%d)",
                MAXSOPS);
			}
			else
         soparray[numSOPs++] = sopPtr2;
		}
	}

	int seqnum = classEntry->genNextSeqnum();
	if (numSOPs > 0)
	{
//#ifdef LX
		//msg->sendToSop(soparray, numSOPs, seqnum, sendToLeadCSOP);
//#else
		msg->sendToSop(soparray, numSOPs, seqnum);
//#endif
			
	}

	delete origSopPtr;
}

GLretVal
CRomdb::deleteDest(const char* destName)
{
	return destTblPtr->deleteDest(destName);
}

GLretVal
CRomdb::modifyDest(DBoperation op, const char* destName,
                   const char* logicalDev,
                   const char* channelType, int log_hsize,
                   int numOfLogs, const char* daily, int logAge)
{
	if (destTblPtr == NULL)
	{
		CRERROR("internal destination table not initialized");
		return GLfail;
	}

	CRdestEntry* destEntry = NULL;

	switch (op)
	{
  case DBload:
  case DBinsertOp:
		return insertDest(destName, logicalDev, channelType, 
		                  log_hsize, numOfLogs, daily, logAge);

  case DBupdateOp:
		return updateDest(destName,logicalDev, channelType, log_hsize,
		                  numOfLogs, daily, logAge);

  case DBdeleteOp:

// PUT LEVEL 3 CROSS CHECK HERE WHEN THERE'S A BOOTABLE SU R13

		return deleteDest(destName);

  default:
		CRERROR("invalid operation '%d'", op);
		return GLfail;
	}
}

const char* CRmsgclsDelNotAllowed = 
   "Delete of special Message Class '%s' is not allowed";

const char* CRmsgDestNotAllowed = 
   "UPDATE FAILED: DEST%d invalid. Must be defined in DEST form.";

const char* CRmsgClsNotDelete = 
   "DELETE FAILED. Cannot delete when the MESSAGE CLASS is defined.";

GLretVal
CRomdb::modifyMsgCls(DBoperation op, const char* msgclass,
                     const char* destlist[], char*& failreason)
{
	GLretVal retval = GLsuccess;
	//
	// Check to make sure new/modified dest is OK
	// this was done in ORACLE forms but has been moved.
	//
	for (int destNum = 0; destNum < CR_MSGCLS_MAXDESTS; destNum++)
	{
		if (destlist[destNum] == NULL)
       break;

		if(destTblPtr->getDestEntry(destlist[destNum])==NULL)
		{
			sprintf(failreason, CRmsgDestNotAllowed, destNum);
			return GLfail;
		}
	}

	if (classTblPtr == NULL)
	{
		CRERROR("internal Message Class table not initialized");
		return GLfail;
	}

	CRdestEntry* destEntryPtr = NULL;
	CRomClassEntry* classEntryPtr = NULL;

	switch (op)
	{
  case DBload:
  case DBinsertOp:
		classEntryPtr = getClassEntry(msgclass);
		if (classEntryPtr)
       delete classEntryPtr;

		classEntryPtr = insertMsgClass(msgclass);

    { /* variable block */
      for (int i = 0; i < CR_MSGCLS_MAXDESTS; i++)
      {
        if (destlist[i] == NULL)
           continue;

        destEntryPtr = getDestEntry(destlist[i]);
        if (destEntryPtr == NULL)
        {
          CRDEBUG(CRusli, ("invalid dest '%s' for msgclass '%s'",
                           (const char*) destlist[i],
                           msgclass));
          return GLfail;
        }
			
        if (classEntryPtr->addDest(destlist[i], destEntryPtr,
                                   failreason) != GLsuccess)
        {
          return GLfail;
        }
      } /* end for */
    } /* end variable block */
		return GLsuccess;

  case DBupdateOp:
		delClassEntry(msgclass);
		return modifyMsgCls(DBinsertOp, msgclass, destlist, failreason);

  case DBdeleteOp:
		if (classTblPtr->isSpecialMsgClass(msgclass) == YES)
		{
			sprintf(failreason, CRmsgclsDelNotAllowed, 
              msgclass);
			return GLfail;
		}
		// This is for level 3 cross check
		retval = omdbTblPtr->checkMsgClass(msgclass);
		if (retval == GLsuccess) // don't remove a vaid msgclass
		{
			sprintf(failreason, CRmsgClsNotDelete);
			return GLfail;
		}

		delClassEntry(msgclass);
		return GLsuccess;

  default:
		CRERROR("invalid operation '%d'", op);
		return GLfail;
	}
}

// FIX THIS IN BOOTABLE SU R13
//const char* CRbadMsgClass = 
//     "INSERT FAILED. MSGCLS must be defined on MSGCLS form.";

GLretVal
CRomdb::modifyOM(DBoperation op, const char* msgname, const char* msgclass,
                 const char* format, CRALARMLVL alarmlvl)
{
	if (omdbTblPtr == NULL)
	{
		CRERROR("internal output message table not initialized");
		return GLfail;
	}

	switch (op)
	{
  case DBload:
  case DBinsertOp:
     {
       CRomClassEntryPtr classPtr = getClassEntry(msgclass);

       if (classPtr == NULL)
       {
         CRERROR("invalid msgclass '%s' for msgname '%s'",
                 msgclass, msgname);
         return GLfail;
       }

       return replaceEntry(msgname, new CRomdbEntry(msgclass,
                                                    classPtr,
                                                    alarmlvl,
                                                    format));
     }
  case DBupdateOp:
     {
       CRomClassEntryPtr classPtr = getClassEntry(msgclass);

       if (classPtr == NULL)
       {
// FIX THIS AND PUT IT IN THE REASON AREA IN BOOTABLE SU R13
         CRERROR("invalid msgclass '%s' for msgname '%s'",
                 msgclass, msgname);
         return GLfail;
       }
       modifyOM(DBdeleteOp, msgname, msgclass, format, alarmlvl);
       return modifyOM(DBinsertOp, msgname, msgclass, format, alarmlvl);
     }
  case DBdeleteOp:
		return deleteEntry(msgname);

  default:
		CRERROR("invalid operation '%d'", op);
		return GLfail;
	}
}

GLretVal
CRomdb::modifyX733OM(DBoperation op, const char* msgname, 
                     const char* pcause ,
                     const char* alarmType ,
                     const char* objName ,
                     const char* specificProblem ,
                     const char* addText)
{
	if (x733omdbTblPtr == NULL)
	{
		CRERROR("internal x733 output message table not initialized");
		return GLfail;
	}

	switch (op)
	{
  case DBload:
  case DBinsertOp:
     {
       return x733replaceEntry(msgname, new CRx733omdbEntry( pcause,
                                                             alarmType ,
                                                             objName ,
                                                             specificProblem ,
                                                             addText));
     }
  case DBupdateOp:
     {
       modifyX733OM(DBdeleteOp, msgname, 
                    pcause,
                    alarmType ,
                    objName ,
                    specificProblem ,
                    addText);

       return modifyX733OM(DBinsertOp, msgname, 
                           pcause,
                           alarmType ,
                           objName ,
                           specificProblem ,
                           addText);
     }
  case DBdeleteOp:
		return x733deleteEntry(msgname);

  default:
		CRERROR("invalid operation '%d'", op);
		return GLfail;
	}
}

GLretVal
CRomdb::modifyAlarmCode(DBoperation op, const char* alrmSrc, char char_1,
                        Char *&retres)
{
	if (alarmCodeTblPtr == 0)
	{
		sprintf(retres, "internal alarm code table not initialized");
		CRERROR("%s", retres);
		return GLfail;
	}

	switch (op)
	{
  case DBload:
  case DBinsertOp:
     {
       // for 5350 spa this part is being comment out
#ifndef LX
       if (strcmp(alrmSrc, "PLATFORM") != 0 && !CRisSpaName(alrmSrc))
       {
         sprintf(retres,
                 "ALARM SOURCE %s is not 'PLATFORM' or SPA name",
                 alrmSrc);
         CRERROR("%s", retres);
         return GLfail;
       }
#endif
       return alarmCodeTblPtr->
          insertEntry(alrmSrc, new CRalarmCodeEntry(char_1));
     }

  case DBupdateOp:
		if (alarmCodeTblPtr->getEntry(alrmSrc) == 0)
		{
			sprintf(retres, "Invalid ALARM SOURCE '%s'", alrmSrc);
			CRERROR("%s", retres);
			return GLfail;
		}
		modifyAlarmCode(DBdeleteOp, alrmSrc, char_1, retres);
		return modifyAlarmCode(DBinsertOp, alrmSrc, char_1, retres);

  case DBdeleteOp:
		if (!alarmCodeTblPtr->table.element(alrmSrc))
		{
			sprintf(retres, "ALARM SOURCE '%s' does not exists",
			        alrmSrc);
			CRERROR("%s", retres);
			return GLfail;
		}
		return alarmCodeTblPtr->deleteEntry(alrmSrc);

  default:
		sprintf(retres, "Invalid operation '%d'", op);
		CRERROR("%s", retres);
		return GLfail;
	}
}


#ifdef CC
void
CRomdb::processLogFile(char *fileName, char *dir)
{
	const char* msgsPtr;
	CRlocalLogSop localLog(dir);

	localLog.init(fileName);

	msgsPtr=localLog.getText();

	if (msgsPtr != NULL && msgsPtr[0] != '\0')
	{
		/* Get text of log file, if any and print to ROP
		** and main logfile.
		*/
    limpRoute("", msgsPtr, (strlen(msgsPtr)-2), POA_CRIT);
	}

	localLog.deleteFile();

	return;
}
#endif

void
CRomdb::setDirectory()
{
  destTblPtr->setDirectory();
}


GLretVal
CRomdb::upDateX733Rept(const char* report)
{
	if( strcmp(report,"ON") == 0 )
	{
		x733reptTblPtr->setX733Report(TRUE);
	}
	else
	{
		x733reptTblPtr->setX733Report(FALSE);
	}
	return GLsuccess;
}

Void
CRomdb::createOmdbMsgAlarmCode(String theOMkey, const char* textMsg)
{

	CRDEBUG(CRusli,("theOMkey = %s ", (const char*) theOMkey));

	const CRalarmCodeEntry *theAlarmCodeEntry;
	String theSub;
	String theNumber;
	String thePlatformCode;
	String theAlarmCode = "99";
	String subsystem;
	int startOfOMnumber=3;
	int sizeOfOMnumber=3;
	int sizeOfSubSystem=2;

	// Fill in leading character
	theAlarmCodeEntry = alarmCodeTblPtr->getEntry("PLATFORM");
	thePlatformCode = theAlarmCodeEntry ? theAlarmCodeEntry->getChar_1() : '9';


	char tmpChar[10];
	strlcpy(tmpChar, theOMkey, theOMkey.length() + 1);

	// check if the subsystem mapping has been stored
	if ((strcmp(theCode[0], "99")) != 0)
	{
		const char* subStart = tmpChar + 1;
		if (strncmp(subStart, "BKUP", 4) == 0 ||
		    strncmp(subStart, "DBCN", 4) == 0 ||
		    strncmp(subStart, "SLEE", 4) == 0)
		{
			sizeOfSubSystem = 4;
		}
		else if (strncmp(subStart, "AAA", 3) == 0 ||
		         strncmp(subStart, "ASI", 3) == 0 ||
		         strncmp(subStart, "ASR", 3) == 0 ||
		         strncmp(subStart, "DIA", 3) == 0 ||
		         strncmp(subStart, "EIB", 3) == 0 ||
		         strncmp(subStart, "HWM", 3) == 0 ||
		         strncmp(subStart, "JPP", 3) == 0 ||
		         strncmp(subStart, "LAN", 3) == 0 ||
		         strncmp(subStart, "MOF", 3) == 0 ||
		         strncmp(subStart, "MPR", 3) == 0 ||
		         strncmp(subStart, "RDB", 3) == 0 ||
		         strncmp(subStart, "RES", 3) == 0 ||
		         strncmp(subStart, "RWP", 3) == 0 ||
		         strncmp(subStart, "SCI", 3) == 0 ||
		         strncmp(subStart, "SCN", 3) == 0 ||
		         strncmp(subStart, "SEC", 3) == 0 ||
		         strncmp(subStart, "SFA", 3) == 0 ||
		         strncmp(subStart, "SIP", 3) == 0 ||
		         strncmp(subStart, "SMI", 3) == 0 ||
		         strncmp(subStart, "SND", 3) == 0 ||
		         strncmp(subStart, "SPA", 3) == 0 ||
		         strncmp(subStart, "TBW", 3) == 0 ||
		         strncmp(subStart, "TMG", 3) == 0 ||
		         strncmp(subStart, "XMC", 3) == 0 ||
		         strncmp(subStart, "X10", 3) == 0)
		{
			sizeOfSubSystem = 3;
		}

		int numLen = theOMkey.length() - sizeOfSubSystem - 1;
		if (numLen < 3)
		{
			theNumber = "";
			for (int i = numLen; i < 3; ++i)
			{
				theNumber += "0";
			}
			theNumber += theOMkey(sizeOfSubSystem + 1, numLen);
		}
		else
		{
			theNumber = theOMkey(theOMkey.length() - 3, 3);
		}
	}

	theSub = theOMkey(1, sizeOfSubSystem); 

#ifdef LX
	CRalarmKey = thePlatformCode + subsystemCode(theSub) + \
     addedValue(theOMkey,textMsg,theNumber) + ";PLATFORM";
#else
	CRalarmKey = thePlatformCode + subsystemCode(theSub) + \
     addedValue(theOMkey,textMsg,theNumber);

#endif

}

Void
CRomdb::createSpoolMsgAlarmCode(String theOMkey, const char* theProcess,
                                String& alarmCode, Bool isFromINIT)
{
	const CRalarmCodeEntry *theAlarmCodeEntry;
	char spaName[MHmaxNameLen + 1];
	char theProcessName[MHmaxNameLen + 1];
	strlcpy(theProcessName,theProcess,strlen(theProcess)+1);

	// An alarm is considered a platform alarm if it was generated by
	// the the INIT library, but sent by a SPA process.
	//
	// A SPA may have more than one process.  The names should exist
	// under /sn/sps/<spaName>/bin directory.
	if (!isFromINIT && CRisSpaProcess(theProcessName, spaName))
	{
		theAlarmCodeEntry = alarmCodeTblPtr->getEntry(spaName);
		if (theAlarmCodeEntry)
		{
			alarmCode = theAlarmCodeEntry->getChar_1();
		}
		else
		{
			alarmCode = "";
		}

#ifdef LX
		int diff = 17 - alarmCode.length() - theOMkey.length();
#else
		int diff = 6 - alarmCode.length() - theOMkey.length();
#endif
		if (diff > 0)
		{
			for (int i = 0; i < diff; i++)
			{
				alarmCode += "0";
			}
#ifdef LX
			if( spaName[0] == '\0' )
			{
				alarmCode += theOMkey + ";PLATFORM";
			}
			else
			{
				alarmCode += theOMkey + ";" + spaName;
			}
#else
			alarmCode += theOMkey;
#endif
		}
		else
		{
#ifdef LX
			if( spaName[0] == '\0' )
			{
				alarmCode += theOMkey.chunk(-diff) + ";PLATFORM";
			}
			else
			{
				alarmCode += theOMkey.chunk(-diff) + ";" + spaName;
			}
#else
			alarmCode += theOMkey.chunk(-diff);
#endif
		}
	}
	else
	{
		theAlarmCodeEntry = alarmCodeTblPtr->getEntry("PLATFORM");
		if (theAlarmCodeEntry)
		{
			alarmCode = theAlarmCodeEntry->getChar_1();
		}
		else
		{
			alarmCode = "9";
		}

		// special handling for platform alarms without OMDB key
		if (strlen(theOMkey) == 5)
		{
#ifdef LX
			alarmCode += theOMkey + ";PLATFORM";
#else
			alarmCode += theOMkey;
#endif
		}
		else
		{
#ifdef LX
			alarmCode += "99999;PLATFORM";
#else
			alarmCode += "99999";
#endif
		}
	}
}

Void
CRomdb::storeSubSystemList()
{
	FILE *fp;
	char subsystem[5], code[4];
	int i =0;

#ifndef EES
	char subsystemList[] = "/sn/cr/CRsubsystemList";
	fp = fopen(subsystemList, "r");
#else

	char subsystemList[] = "CRsubsystemList";

  String realfname;
  String fname = "cc/cr/omm/CRsubsystemList";

	fp = CRfopen_vpath(fname, "r", realfname);
#endif

	if (fp != 0)
	{
		while((fscanf(fp,"%s %s",subsystem,code))!=EOF)
		{
			strlcpy(theSubsystem[i],subsystem,strlen(subsystem)+1);
			strlcpy(theCode[i],code,3);

			++i;
		}
	}
	else
	{
		CRERROR("CAN NOT READ %s FILE", subsystemList);
		strlcpy(theCode[i],"99",2);
	}

	fclose(fp);
}

String
CRomdb::subsystemCode(const char* subsystem) const
{
	String theSubsystemCode = "99";

	if ((strcmp(theCode[0], "99")) != 0)
	{
		for (int i = 0; i < 100; ++i)
		{
			if (strncmp(theSubsystem[i], subsystem,
                  strlen(subsystem)) == 0)
			{
				theSubsystemCode = theCode[i];
				break;
			}
		}
	}

	if (theSubsystemCode == "99")
	{
		CRCFTASSERT(CRsubsystemMapFailureId,
		            (CRsubsystemMapFailure, subsystem));
	}

	return theSubsystemCode;
}

Bool
CRisSpaName(const Char* procName)
{
	Bool isSpaName = FALSE;
	DIR *dirp;
	struct dirent *dirEnt;
	char spdir[200] = "/sn/sps";
	const char* mynode = getenv("MYNODE");

	if (mynode != 0)
	{
		sprintf(spdir, "%s/sn/sps", mynode);
	}

	if ((dirp = opendir(spdir)) != 0)
	{
		while ((dirEnt = readdir(dirp)) != 0)
		{
			if (strcmp(dirEnt->d_name, procName) == 0)
			{
				isSpaName = TRUE;
				break;
			}
		}
		closedir(dirp);
	}

	return isSpaName;
}

//
// This method returns TRUE and populates the array pointed to by
// "spaName" with the SPA's name if the passed procName is determined to be a SPA.
// To determine if procName is a SPA, this method will:
// 1) If the procName contains an underscore "_", it will be truncated to
//    end before the "_".  This is to correctly handle SLL SPA client
//    processes with procName's like EPAY274_10.  Only EPAY274 will be
//    considered the procName.
// 2) Return true if the directory /sn/sps/<procName> is found.
// 3) Return true if the file /sn/sps/*/bin/<procName> is found.
//    This could be a helper process of a C++ SPA.
//

Bool
CRisSpaProcess(Char *procName, Char *spaName)
{

	char *client_p = 0;
	if ( ( client_p = strchr( procName, '_' ) ) != 0 )
	{
		*client_p='\0';
	}

	DIR *dirp, *dirp1;
	struct dirent *dirEnt, *dirEnt1;
	char spdir[200] = "/sn/sps";
	char binDir[300];
	const char* mynode = getenv("MYNODE");
	Bool isSpaProcess = FALSE;

	spaName[0] = '\0';

	if (mynode != 0)
	{
		sprintf(spdir, "%s/sn/sps", mynode);
	}

	if ((dirp = opendir(spdir)) == 0)
	{
		return FALSE;
	}

	while (!isSpaProcess && (dirEnt = readdir(dirp)) != 0)
	{
		// Skip the known non-SPA directories
		if (strcmp(dirEnt->d_name, "."         ) == 0 ||
        strcmp(dirEnt->d_name, ".."        ) == 0 ||
        strcmp(dirEnt->d_name, "SU"        ) == 0 ||
        strcmp(dirEnt->d_name, "asr.images") == 0 ||
        strcmp(dirEnt->d_name, "domains"   ) == 0 ||
		    strcmp(dirEnt->d_name, "jain"      ) == 0)
		{
			continue;
		}

		if (strcmp(dirEnt->d_name, procName) == 0)
		{
			// It is the main process of the SPA.
			strcpy(spaName, dirEnt->d_name);
			isSpaProcess = TRUE;
			break;
		}

		sprintf(binDir, "%s/%s/bin", spdir, dirEnt->d_name);
		if ((dirp1 = opendir(binDir)) == 0)
		{
			// Not a SPA directory.
			continue;
		}

		while ((dirEnt1 = readdir(dirp1)) != 0)
		{
			// Check for helper processes of the SPA.
			if (strcmp(dirEnt1->d_name, procName) == 0)
			{
				strcpy(spaName, dirEnt->d_name);
				isSpaProcess = TRUE;
				break;
			}
		}
		closedir(dirp1);
	}

	closedir(dirp);
	return isSpaProcess;
}

Void
CRomdb::storeAdditionalNumbers()
{
	FILE *fp;
	char omdbkey[7], text[100], OMtext[100];

/*
  File looks like
  #only lines starting with / are read in
  #comments start with # first charaters
  #fields are OM key number OM text
  /CR001 21 REPT CCDISK SOMETHING 
  +++ N440DA18 2009-09-24 11:18:16 MAINT /CR020 #000021 as01 LEAD >
  *  REPT ALM UNIT OOS
  ::equipmentAlarm::outOfService::ARU::OOS::OOS;906::
  END OF REPORT #000021++-
*/
	int num, i =0;

#ifndef EES
	char addedValueFile[] = "/sn/cr/CRuniqueAlarmIds";
	fp = fopen(addedValueFile, "r");
#else

	char addedValueFile[] = "CRuniqueAlarmIds";

  String realfname;
  String fname = "cc/cr/omm/CRuniqueAlarmIds";

	fp = CRfopen_vpath(fname, "r", realfname);
#endif

	if (fp == 0)
	{
		CRDEBUG(CRusli,("%s is missing", addedValueFile));
	}
	else
	{
		while((fgets(text, sizeof(text), fp)) != NULL)
		{
			if(text[0] == '/')
			{
			
				/* remove return char in file */
				if ( text[strlen(text)-1] == '\n' )
           text[strlen(text)-1]='\0';

				memset((char*) &omdbkey, 0, sizeof(omdbkey));
				memset((char*) &OMtext, 0, sizeof(OMtext));
				sscanf(text,"%s %d %100c",omdbkey, &num, OMtext);

				strlcpy(theOMdbKey[i],omdbkey,strlen(omdbkey)+1);
				strlcpy(theOMtext[i],OMtext,strlen(OMtext)+1);
				theAddedValue[i]=num;

        CRDEBUG(CRusli,("%d) theOMdbKey = %s theAddedValue = %d theOMtext = >%s<", 
                        i,theOMdbKey[i],theAddedValue[i],theOMtext[i]));

				++i;

				if( i == 100 )
				{
					CRDEBUG_PRINT(CRusli,("%s is too large first 100 entries are read", addedValueFile));
					break;
				}
			}//end of if /
		}//end of while
		fclose(fp);
	}
}

String
CRomdb::addedValue(const char* OMkey, const char* OMtext, const char* number) const
{
	CRDEBUG(CRusli,("OMkey = >%s< OMtext = >%s< number = %s",
                  (const char*) OMkey, (const char*) OMtext, (const char*) number));

	int newNumber = 0;
	String newNumberStr;

	for (int i = 0; i < 100; ++i)
	{

		if(theOMdbKey[i][0] == 0) 
       break;

		//match on /FT
		if (strncmp(OMkey,theOMdbKey[i], strlen(theOMdbKey[i])) == 0)
		{
			if (strncmp(OMtext,theOMtext[i], strlen(theOMtext[i])) == 0)
			{
				newNumber =  atoi(number) + theAddedValue[i];
				newNumberStr = int_to_str(newNumber);
				return newNumberStr;
			}
		}
	}

	return number; //not found
}
