/*
**      File ID:        @(#): <MID13746 () - 9/29/95, 8.1.1.5>
**
**	File:					MID13746
**	Release:				8.1.1.5
**	Date:					9/29/95
**	Time:					19:43:57
**	Newest applied delta:	9/29/95
**
** DESCRIPTION:
**	This file contains the high level functions of the 
**      Output Spooler (CSOP) process.
**
** OWNER: 
**	Roger McKee
**	Yash Pal Gupta
**
*/

#include <stdio.h>
#include <errno.h>
#include <sys/utsname.h>
#include <thread.h>
#include <signal.h>
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/tim/TMmtype.H"
#include "cc/hdr/tim/TMtmrExp.H"
#include "cc/hdr/eh/EHreturns.H"
#include "cc/hdr/eh/EHhandler.H"
#include "cc/hdr/init/INmtype.H"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/msgh/MHinfoExt.H"
#include "cc/hdr/msgh/MHmsgBase.H"
#include "cc/hdr/bill/BLmtype.H"
#include "cc/hdr/su/SUexitMsg.H"
#include "cc/hdr/su/SUmtype.H"
#include "CRcsopTrace.H"
#include "cc/hdr/cr/CRspoolMsg.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/cr/CRdbCmdMsg.H"
#include "CRrcvOmdb.H"
#include "cc/hdr/cr/CRomdbMsg.H"
#include "CRomdb.H"
#include "cc/hdr/cr/CRloadName.H"
#include "cc/hdr/cr/CRofficeNm.H"
#include "cc/hdr/db/DBintfaces.H"
#include "CRomBuffers.H"
#include "cc/cr/hdr/CRtimers.H"
#include "cc/cr/hdr/CRlogFile.H"
#include "cc/cr/hdr/CRrmOldFiles.H"
#include "cc/cr/hdr/CRlogFileSwitchUpd.H"
#include "CRcsopAlvMsg.H"
#include "CRcsopOMs.H"
#include <sys/statvfs.h>

#include "cc/lib/msgh/MHmsg.H"
#include "cc/hdr/msgh/MHgq.H"

#include "cc/hdr/init/INinitialize.H"
#include "cc/hdr/ft/FTmsgs.H"

#include "cc/hdr/cr/CRlocalLogMsg.H"
#include "cc/cr/hdr/CRgdoMemory.H"

#include "cc/hdr/db/DBselect.H" 
#include "cc/hdr/db/DBackBack.H" 
#include "cc/hdr/db/DBfomatMsg.H"
#include "cc/hdr/db/DBintfaces.H"
#include "cc/hdr/db/DBsqlMsg.H"
#include "cc/hdr/db/DBfmUpdate.H"
#include "cc/hdr/db/DBselect.H"
#include "cc/hdr/db/DBupdate.H"

#include "cc/cr/hdr/CRomClEnt.H"
#include "cc/cr/hdr/CRomDest.H"

#ifdef LX
#include "cc/hdr/cr/CRsopMsg.H"
#endif

EHhandler CRevent;	/* Event handler object */

#define STDIN_FD	0	/* standard in file descriptor */
#define STDOUT_FD	1	/* standard out file descriptor */
#define STDERR_FD       2       /* standard error file description */

Bool CRsetUniqueAlarm = FALSE;
Bool CRdoCentLogs = FALSE;

#ifdef LX
Bool isOAMLead = TRUE;
Char *theLeadCSOPptr;
Char theLeadCSOP[15];
#endif

/* Name of OMM coordinator process */
static const Char PROCESS_NAME[] = "CSOP";
int CRglobalDirectoryName = LEAD_LOG_DIRECTORY;

static MHqid CRITmhqid = MHnullQ; // mhqid associated with this process 
static MHqid mhqid = MHnullQ; // mhqid associated with this process 
static MHqid mhGqid = MHnullQ; // mhqid associated with this process 

/* Number of output message buffers in the buffer pool 
** This determines the number of output messages that can be 
** held in memory at one time.  If CSOP gets behind it will throw
** away low priority output messages by reusing the buffers
** before the contents of the buffers were processed.
*/ 
const int CRnumOMbuffers = 1000;
CRomBufferPool CRbufferPool(CRnumOMbuffers);
void createBrevShareMemory();
GLretVal checkOnPlatTable();
GLretVal getDataForPlatDB();

CRomBufQueList CRomQueues;
CRomdb omdb;
int CRstolenBufferCnt = 0;

extern "C" Void* CRstartUpLocalLogThread(void* arg);

#ifdef CC

char* 
CRgetremote()
{
	struct utsname un;
	char str[20];

	int retval = uname(&un);
	if (retval == -1)
	{
		return NULL;
	}

	int slen = strlen(un.nodename);

	if (un.nodename[slen-1] == '0')
	{
		un.nodename[slen-1]= '\0';
		sprintf(str, "%s1", un.nodename);
		return str;
	}
	else if (un.nodename[slen-1] == '1')
	{
		un.nodename[slen-1]= '\0';
		sprintf(str, "%s0", un.nodename);
		return str;
	}
	return NULL;
}
#endif

void
CRgracefulExit()
{
	char* dummyArgv[1];
	cleanup(0, dummyArgv, SN_NOINIT, 0);
	exit(0);
}

/*
** This routine is called when the system is booted or when the system goes
** through a system reset.  This process doesn't contrl any system resources
** which are shared between processes in the CC.  Therefore there is no
** SYSINIT work to be done:
*/

Short
sysinit(int /*argc*/, Char */*argv*/[], SN_LVL init_lvl, U_char run_lvl)
{

	CRERRINIT(PROCESS_NAME);

	/* check here for standby machine */

	CRcurrentLoad.init();
	CRswitchName.init();

	CRCSOPDEBUG(CRusli, (" init_lvl=%d run_lvl=%d\n",
                       init_lvl, run_lvl));

	GLretVal retval = omdb.sysinit();
	if (retval != GLsuccess)
	{
		CRCSOPERROR("CRomdb.sysinit() failed with error code %d",
                retval);
		return GLfail;
	}

	
  return GLsuccess;
}


short CRcsopAuditTmrBlock = -1;
short CRcriticalQueueTmrBlock = -1;

void
CRstopAuditTimer()
{
	if (CRcsopAuditTmrBlock != -1)
	{
		CRevent.clrTmr(CRcsopAuditTmrBlock);
		CRcsopAuditTmrBlock = -1;
	}
}

void
CRstartAuditTimer()
{
	CRCSOPDEBUG(CRusli+CRinoutFlg, ("starting audit timer"));
	CRcsopAuditTmrBlock = CRevent.setRtmr(CRcsopAuditTime,
                                        CRcsopAuditTag, TRUE);
	if (TMINTERR(CRcsopAuditTmrBlock))
	{
		CRCSOPERROR("CRevent.setRtmr failed with error code %d",
                CRcsopAuditTmrBlock);
		CRgracefulExit();
	}
}

// Start a cyclic idle timer to get out of CRITICAL CSOP queue
void
CRcriticalQueueTimer()
{
	CRCSOPDEBUG(CRusli+CRinoutFlg, ("starting queue timer"));

	CRcriticalQueueTmrBlock = CRevent.setRtmr(CRqueueTimer,
                                            CRqueueCheckTag, TRUE, TRUE);

	if (TMINTERR(CRcriticalQueueTmrBlock))
  {
    CRERROR("CRevent.setRtmr failed with error code %d",
        		CRcriticalQueueTmrBlock);
		CRgracefulExit();
  }

}

/*
** This routine is called during process initialization.  This routine is used
** to attempt a recovery from a problem that is local to this process
*/

Short
procinit(int /*argc*/, Char */*argv*/[], SN_LVL init_lvl, U_char run_lvl)
{

	if(init_lvl==SN_LV4 )
     CRglobalDirectoryName=ACTIVE_LOG_DIRECTORY;

	CRcurrentLoad.init();
	CRswitchName.init();

	CRERRINIT(PROCESS_NAME);
	CRCSOPDEBUG(CRusli, ("init_lvl=%d run_lvl=%d\n",
                       init_lvl, run_lvl));

	GLretVal retval;

	/* attach to the MSGH subsystem */

	if ((retval = CRevent.attach()) != GLsuccess)
	{
    CRCSOPERROR("attach to MSGH failed! errn= %d\n",
                retval);
    return GLfail;
  }

  /* register process name and get a MSGH queue regName */
 	// local process name starts with a _
 	String localProcessName = "_";
  localProcessName += PROCESS_NAME;

 	// Critical process name starts with a Crit
	// Must be local, we can only have one global queue.
 	String CritProcessName = "Crit";
  CritProcessName += PROCESS_NAME;



	//HERE
  //OLDif ((retval = CRevent.regName(localProcessName, 
  //OLDmhqid, FALSE)) != GLsuccess)
  if ((retval = CRevent.regName(localProcessName, 
                                mhqid, FALSE, FALSE, MH_GLOBAL)) != GLsuccess)
	{
    CRCSOPERROR("MSGH regName() failed with name=%s retval=%d\n",
                PROCESS_NAME, retval);
    return GLfail;
  }

  if ((retval = CRevent.regName(CritProcessName, 
                                CRITmhqid, FALSE)) != GLsuccess)
	{
    CRCSOPERROR("MSGH regName() failed with name=%s retval=%d\n",
                (const char*) CritProcessName, retval);
    return GLfail;
  }
	/* initialize timer library */
	if ((retval = CRevent.tmrInit()) != GLsuccess)
     return retval;

	retval = omdb.procinit(init_lvl);

	if (retval != GLsuccess)
	{
		CRCSOPERROR("CRomdb.procinit() failed with error code %d",
                retval);
		return GLfail;
	}

	
	retval = CRbufferPool.procinit();
	if (retval != GLsuccess)
	{
		CRCSOPERROR("CRbufferPool.procinit() failed with error code %d",
                retval);
		return GLfail;
	}

	CRstartAuditTimer();

	// For Linux register CSOP like this:
	// If you have a LEAD then your in a cluster
	// and register as system global
	// else  your in a AA then register as a local global
#ifdef LX
	// hostid = getOAMLead(); = host id else -1 if 
	isOAMLead=MHmsgh.isOAMLead();
	if(isOAMLead) 
	{
    // hostid => 0
    if ((retval = CRevent.regGlobalName(PROCESS_NAME, mhqid, mhGqid, 
                                        TRUE, MH_systemGlobal)) != GLsuccess)
    {
      CRCSOPERROR("MSGH regGlobalName() failed with name=%s retval=%d\n",
                  PROCESS_NAME, retval);
      return GLfail;
    }
	}
	else
	{
    if ((retval = CRevent.regGlobalName(PROCESS_NAME, mhqid, mhGqid, 
                                        TRUE)) != GLsuccess)
    {
      CRCSOPERROR("MSGH regGlobalName() failed with name=%s retval=%d\n",
                  PROCESS_NAME, retval);
      return GLfail;
    }
	}
#else
  if ((retval = CRevent.regGlobalName(PROCESS_NAME, mhqid, mhGqid, 
                                      TRUE)) != GLsuccess)
	{
    CRCSOPERROR("MSGH regGlobalName() failed with name=%s retval=%d\n",
                PROCESS_NAME, retval);
    return GLfail;
	}
#endif

	
#ifdef CC
	if(init_lvl==SN_LV3 || init_lvl==SN_LV5)
	{
    //lets getting any more log files in locallogs
    omdb.procLocalLogs(); 
	}
#endif
	CRcriticalQueueTimer();
	
	/* Tell the other processes that CSOP is now alive */
	CRcsopAliveMsg aliveMsg;
	aliveMsg.send();

	/* set CRsetUniqueAlarm flag */
#ifndef EES
	char alarmCodeOffFile[] = "/sysdata1/CSOP/CRAlarmCodeOff";
#else
	char alarmCodeOffFile[200];
	sprintf(alarmCodeOffFile, "%s/CRAlarmCodeOff", getenv("MYNODE"));
#endif
	struct stat st;
	if (lstat(alarmCodeOffFile, &st) < 0)
	{
		CRsetUniqueAlarm=TRUE;
		//read /sn/cr/CRsubsystemList file and store it in a array
		omdb.storeSubSystemList();
		//read /sn/cr/CRuniqueAlarmIds file and store it in a array
		omdb.storeAdditionalNumbers();
	}

#ifdef LX

	/*
    I'm keeping this code because one day I may use it

    isOAMLead=MHmsgh.isOAMLead();

    Short LeadMach=MHmsgh.getOAMLead();
    if( LeadMach < 0 )
    {
    CRDEBUG(0,
    ("LeadMach = %d Fail to send center OM",LeadMach));
    }
    else
    {
    Char theMachine[20];
    MHmsgh.hostId2Name(LeadMach, theMachine);
    snprintf(theLeadCSOP,15,"%s:_CSOP",theMachine);
		theLeadCSOP[sizeof(theLeadCSOP)]='\0';
    theLeadCSOPptr=theLeadCSOP;

    CRDEBUG(0,("sending center OM to %s",theLeadCSOP));
    }

	*/

#endif

	return GLsuccess;
}

/*
** This routine is used to remove the MSGH queue resources (but not our MSGH
** queue name), detach from the MSGH shared memory segment
*/

Short 
cleanup(int /*argc*/, Char */*argv*/[], SN_LVL init_lvl, U_char run_lvl)
{
	CRCSOPDEBUG(CRusli, ("init_lvl=%d run_lvl=%d\n", init_lvl, run_lvl));

	GLretVal retval;

	retval = omdb.cleanup();
	if (retval != GLsuccess)
	{
		return GLfail;
  }

	retval = CRbufferPool.cleanup();
	if (retval != GLsuccess)
	{
		return GLfail;
  }

	return GLfail; /* force INIT to kill & restart the process */
}

void
CRtmpFileAudit(Long maxAgeInSecs)
{
	/* delete all old temporary logfiles */
	CRrmOldFiles(CRLCLLOGDIR, maxAgeInSecs);

	/* delete all old files used by CRspoolMsg and CRomdbMsg */
	CRrmOldFiles(CRspoolMsg::dirname(), maxAgeInSecs);
	CRrmOldFiles(CRomdbMsg::dirname(), maxAgeInSecs);
}

void
CRcsopAudit()
{
	CRCSOPDEBUG(CRusli+CRinoutFlg, ("audits started"));

// change for 15 minutes
  CRtmpFileAudit(600);
}


const char* CRlogPartition = "/sn/log";
const int CRminorAlrm = 70;
const int CRmajorAlrm = 90;
const int CRcritAlrm = 100;


void
CRlogFsAudit()
{
	static int brevCnt=0;
	//read in and create share memory
	if(brevCnt == 0)
	{
		createBrevShareMemory();
		++brevCnt;
	}

	static CRALARMLVL currentAlarm = POA_INF;
	static time_t minorStart = 0;
	static time_t majorStart = 0;
	static time_t criticalStart = 0;
	static Bool NeedToSendClear = NO;

	struct statvfs fs;
	Long timeInAlrm = 0;
	short newAlarm = 0;

	CRCSOPDEBUG(CRusli, ("entered CRlogFsAudit, alarm state is %d",
                       currentAlarm));
#ifdef EES
	short ret = statvfs(".",&fs);
#else
	short ret = statvfs(CRlogPartition,&fs);
#endif

	if (ret != 0)
	{
		/* probably partition does not exist */
		CRCSOPDEBUG(CRusli,
                ("statfs returned with %d, cannot get capacity",
                 ret));
		return;
	}

	Long totalblks, used, free, avail, reserved;

	totalblks = fs.f_blocks;
	free = fs.f_bfree;
	used = totalblks - free;
	avail = fs.f_bavail;
	reserved=free - avail;
	totalblks -= reserved;

	/*
	** this check to for float point exp.
	** nnn/0 will cause a core dump
	*/
	if(used <= 0)
     return;
	if(totalblks <= 0)
     return;

	Long CRlogCapacity = (double)used / (double)totalblks * 100.0;

	CRCSOPDEBUG(CRusli,
              ("f_blocks %d, f_bfree %d, capacity %d",
               fs.f_blocks, fs.f_bfree, CRlogCapacity));

	if (CRlogCapacity < CRminorAlrm)
	{
		/*
		** check to see if were in alarm state.
		** add to cumulative timer time 
		** from start till now.  put curr_alarm into NONE state.
		*/
		CRCSOPDEBUG(CRusli, ("capacity < minor %d\n",
                         CRminorAlrm));
		switch (currentAlarm)
		{
    case POA_INF:
			break;
    case POA_MIN:
			timeInAlrm = time(0) - minorStart;
			minorStart = 0;
			break;
    case POA_MAJ:
			timeInAlrm = time(0) - majorStart;
			majorStart = 0;
			break;
    case POA_CRIT:
			timeInAlrm = time(0) - criticalStart;
			criticalStart = 0;
			break;
		}
		currentAlarm = POA_INF;

		if( NeedToSendClear == YES )
		{
			CROMDBKEY(clearOM, "/CR029");
			CRomdbMsg om;
			om.spool(clearOM);
			NeedToSendClear=NO;
		}
	}
	else if (CRlogCapacity >= CRminorAlrm &&
           CRlogCapacity < CRmajorAlrm)
	{
		/* in minor alarm state */
		CRCSOPDEBUG(CRusli,
                ("capacity > minor %d and < major %d",
                 CRminorAlrm, CRmajorAlrm));
		switch (currentAlarm)
		{
    case POA_INF:
			newAlarm = 1;
			break;
    case POA_MIN:
			/* already in minor alarm, don't do anything */
			newAlarm = 0;
			break;
    case POA_MAJ:
			timeInAlrm = time(0) - majorStart;
			majorStart = 0;
			newAlarm = 1;
			break;
    case POA_CRIT:
			timeInAlrm = time(0) - criticalStart;
			criticalStart = 0;
			newAlarm = 1;
			break;
		}
		if (newAlarm == 1)
		{
			currentAlarm = POA_MIN;
			minorStart = time(0);
			/* send alarm to console */
			CROMDBKEY(minorOM, "/CR026");
			CRomdbMsg om;
			om.add(CRlogCapacity);
			om.spool(minorOM);
			NeedToSendClear=YES;
		}
	}
	else if (CRlogCapacity >= CRmajorAlrm &&
           CRlogCapacity < CRcritAlrm)
	{
		/* in major alarm state */
		CRCSOPDEBUG(CRusli,
                ("capacity > major %d and < critical %d",
                 CRmajorAlrm, CRcritAlrm));
		switch (currentAlarm)
		{
    case POA_INF:
			newAlarm = 1;
			break;
    case POA_MIN:
			timeInAlrm = time(0) - minorStart;
			/* store in minutes */
			//minorCumulative += timeInAlrm/3600;
			minorStart = 0;
			newAlarm = 1;
			break;
    case POA_MAJ:
			/* already in major alarm, don't do anything */
			newAlarm = 0;
			break;
    case POA_CRIT:
			timeInAlrm = time(0) - criticalStart;
			/* store in minutes */
			//criticalCumulative += timeInAlrm/3600;
			criticalStart = 0;
			newAlarm = 1;
			break;
		}
		if (newAlarm == 1)
		{
			currentAlarm = POA_MAJ;
			majorStart = time(0);
			/* send alarm to console */
			CROMDBKEY(majorOM, "/CR027");
			CRomdbMsg om;
			om.add(CRlogCapacity);
			om.spool(majorOM);
			NeedToSendClear=YES;
		}
	}
	else if (CRlogCapacity >= CRcritAlrm)
	{
		/* in critical alarm state */
		CRCSOPDEBUG(CRusli,
                ("capacity > critical %d", CRcritAlrm));
		switch (currentAlarm)
		{
    case POA_INF:
			newAlarm = 1;
			break;
    case POA_MIN:
			timeInAlrm = time(0) - minorStart;
			/* store in minutes */
			//minorCumulative += timeInAlrm/3600;
			minorStart = 0;
			newAlarm = 1;
			break;
    case POA_MAJ:
			timeInAlrm = time(0) - majorStart;
			/* store in minutes */
			//majorCumulative += timeInAlrm/3600;
			majorStart = 0;
			newAlarm = 1;
			break;
    case POA_CRIT:
			/* already in critical alarm, don't do anything */
			newAlarm = 0;
			break;
		}
		if (newAlarm == 1)
		{
			currentAlarm = POA_CRIT;
			criticalStart = time(0);
			/* send alarm to console */
			CROMDBKEY(critOM, "/CR028");
			CRomdbMsg om;
			om.add(CRlogCapacity);
			om.spool(critOM);
			NeedToSendClear=YES;

			/* Try to reclaim space with the tmp file audit
			** Get rid of any tmp files older than 5 seconds.
			*/
			CRtmpFileAudit(5);
		}
	}
	
	CRCSOPDEBUG(CRusli,
              ("alarm levels: minor %d, major %d, critical %d",
               CRminorAlrm, CRmajorAlrm, CRcritAlrm));
}


struct thread_arg{
  char    hostName[ MHmaxNameLen+1 ];
};


void
CRprocActLocalLogs(CRlocalLogMsg* msg_ptr)
{

/*
**	This is called when FTMON starts on a ACTIVE(NON LEAD)
**	processer. 
**
**	This will rcp any files in /sn/log/locallogs from the 
**      machine and put it on the LEAD CC. Then it will remove the
**	file from the ACTIVE processer. Call procLocallogs
**	to put it in the OMlog files.
*/

  Short   hostid, myHostid;
  char    lHost[ MHmaxNameLen+1 ];
  char    mylHost[ MHmaxNameLen+1 ];
  int	rc;
  struct thread_arg *tt;

/*
**	Get the machine that sent us here
*/
  if ((MHmsgh.getMyHostName(mylHost)) != GLsuccess)
  {
    // if this fails set my host to a bogus value
    sprintf(mylHost,"X");
  }

  hostid = MHmsgh.Qid2Host( msg_ptr->srcQue );

  if ( MHmsgh.hostId2Name( hostid, lHost ) != GLsuccess )
  {
    /* IBM swerbner 200608 access id member via accessor */
    CRERROR( "COULD NOT FIND HOST NAME FOR QID = %d",
             msg_ptr->srcQue.getid() );
    return;
  }

  if((strcmp(mylHost, lHost)) == 0 )
  {
		// don't want to do lead locallogs again
		return;
	}

  // allocate memory to hold host name
  if((tt=(struct thread_arg *) malloc(sizeof(struct thread_arg))) == NULL)
  {
    CRERROR( "FAILED TO MALLOC HOST NAME, ERRNO %d", errno );
    return;
  }

  if ( MHmsgh.getRealHostname( lHost, tt->hostName ) != GLsuccess )
  {
    CRERROR( "COULD NOT FIND REAL HOST NAME FOR = %s", lHost );
    return;
  }

  /*
  **	Start a thread that will only rcp and rsh rm
  **	the locallog files. We'll doing this
  **	because if the processer goes down CSOP could
  **	be stuck doing a rcp and stay that way until
  **	INIT kills CSOP and restarts it.
  **
  **	stack base = 0
  **	stack size = 1MEG
  **	start routine = startUpLocalLogThread
  **	arg = hostName (the processer that sent the FT message)
  **	THR_DETACHED flag is set so I don't have to worry about it
  **	thread = NULL
  */
  if ((rc = thr_create((void *) 0, 1000000,
                       CRstartUpLocalLogThread,
                       (void *) tt, THR_DETACHED,
                       NULL)) != 0) 
	{
		CRERROR("Can't create locallog thread");
		return;
	}
	return;
}

Void *
CRstartUpLocalLogThread(void* arg)
{
	
	/*
	 * Now rcp the files over
	 */
	char str[100];
	const char* log_dir = "/sn/log/locallogs";

	thread_arg* tt = (thread_arg*) arg;


	sprintf(str, "/sn/lcr/rmLocalLogs %s %s",
          tt->hostName, log_dir);
	system(str);

	/* RE DO THE WHOLE THING TO PICK UP ANY LATE GUYS */
	sleep(3);

	sprintf(str, "/sn/lcr/rmLocalLogs %s %s",
          tt->hostName, log_dir);
	system(str);

	free(tt);

	/*
	**	Send a message to CSOP to get local log files
	*/
	CRgetLocalLogMsg   getLocalLogMsg;

	getLocalLogMsg.send();

	return(NULL);

}

/* Returns NO if buffer can be freed
** Returns YES if buffer is still in use
*/
Bool
CRprocessMsg(GLretVal rcvRtn, CRomBuffer* omBuf, short msgsz)
{	
	if (rcvRtn == GLsuccess)
	{
		MHmsgBase* msg_ptr = (MHmsgBase*) omBuf->getMsgBuf();
		switch (msg_ptr->msgType)
		{
      //global queue message
      //send a ack back
    case MHgqRcvTyp:
       {
         MHgqRcv *p = (MHgqRcv *)msg_ptr;
         MHgqRcvAck      ack(p->gqid);
         if ( ( ack.send(p->srcQue) ) != GLsuccess) 
         {
           CRERROR("MHgqRcvAck did not return GLsuccess");
         }

         break;
       }

    case TMtmrExpTyp:
			/* assumes that is the audit timer */
       {
         switch(((TMtmrExp *)msg_ptr)->tmrTag)
         {
           /* audit timer */
         case CRcsopAuditTag:
           CRcsopAudit();
           break;

         default:
           // do nothing 
           // assume its the Critical timer
           break;
         }
       }

    case INmissedSanTyp:
			IN_SANPEG();
			break;
    case INpDeathTyp: /* ignore msg */
			break;

    case SUexitTyp: /* field update message */
       {
         char* tmpargv[1];
         cleanup(0, tmpargv, SN_LV0, 0);
       }
       exit(0);

    case SUversionTyp: /* field update version message */
			CRcurrentLoad.init();
			break;

    case BLtimeChngTyp:
			CRCSOPDEBUG(CRusli+CRmsginFlg, ("time change"));
			CRstopAuditTimer();
			CRevent.updoffset();
			omdb.timeChange();
			CRstartAuditTimer();
			return GLsuccess;

    case CRdbCmdMsgTyp:
			if (msgsz != sizeof(CRdbCmdMsg))
			{
				CRCSOPERROR("received message of size %d, not size %d",
                    msgsz, sizeof(CRdbCmdMsg));
			}
			else
			{
				CRdbCmdMsg *traceMsgPtr = (CRdbCmdMsg*) msg_ptr;
				traceMsgPtr->unload();
			}
			break;
    case DBfmInsertTyp:
			DBman::dbFmMsg((DBfmMsg *) msg_ptr, msgsz,
                     DBinsertOp, DBfmInsertAckTyp, mhqid);
			break;
    case DBfmUpdateTyp:
			DBman::dbFmMsg((DBfmMsg *) msg_ptr, msgsz,
                     DBupdateOp, DBfmUpdateAckTyp, mhqid);
			break;
    case DBfmDeleteTyp:
			DBman::dbFmMsg((DBfmMsg *) msg_ptr, msgsz,
                     DBdeleteOp, DBfmDeleteAckTyp, mhqid);
			break;


    case CRlocalLogMsgTyp:
			CRprocActLocalLogs((CRlocalLogMsg*) msg_ptr);
			return YES;

    case CRgetLocalLogMsgTyp:
#ifdef CC
			omdb.procLocalLogs();
#endif
			return YES;

#ifdef CC
    case INinitializeTyp:
       {
         // switch over just finished,
         // process becomes Lead
         INinitializeAck ack;
         ack.send(mhGqid); 
       }
       return YES;

#ifdef LX
    case INoamInitializeTyp:
       {
         // BLADE switch over just finished,
         // process becomes Lead
         // check to see if we need to tail alarm.log
         // for 5350. We should know the timer interval
         // and if 5350 logging is turn on
         CRCSOPDEBUG(CRusli+9, (" got a OAM lead change"));
       }
       return YES;
#endif

    case FTfileSysReadyTyp:
       {
         // Change the log directory
         // read the OMlog and the debuglog
         // to pickup any missing OM's

         CRglobalDirectoryName=LEAD_LOG_DIRECTORY;
         omdb.setDirectory();
         omdb.procLocalLogs();
       }
       return YES;

#endif
#ifdef CC
    case CRsgenOMTyp:
#endif
    case CRgenOMTyp:
       {
         return CRomQueues.add((CRspoolMsg*) msg_ptr);
       }
#ifdef CC
    case CRsomdbTyp:
#endif
    case CRomdbTyp:
       {
#ifdef DEBUG
         ((CRomdbMsg*) msg_ptr)->dump();
#endif
         return CRomQueues.add((CRomdbMsg*) msg_ptr);

       }
    case CRsopRegMsgTyp:
			omdb.regMsg((CRsopRegMsg*) msg_ptr);
			break;

    case CRlogFileSwitchUpdTyp:
       {
         CRlogFileSwitchUpd *upd = (CRlogFileSwitchUpd *)msg_ptr;
         std::vector<std::string>::iterator pos;
         pos = find(interestedQueues.begin(),
                    interestedQueues.end(),
                    upd->getQueueName()
           );

         switch(upd->getType()) {
         case REGISTER:

           //insert only if element does not exist already
           if(pos == interestedQueues.end()) 
              interestedQueues.insert(
                interestedQueues.end(),
                upd->getQueueName()
                );
           break;

         case DEREGISTER:
                               
           //delete only if element  exists already
           if(pos == interestedQueues.end()) 
              interestedQueues.erase(pos);
           break;
         }
       }
       break;
#ifdef LX

		   //global alarms to send to MOFPROC/PLATAGT
    case CRsopMsgTyp:
       {
         CRsopMsg *msg = (CRsopMsg*) msg_ptr;
         msg->send( "MOFPROC", msg->getOMstart(), 
                    strlen(msg->getOMstart()));

		   }
		   break;

#endif
    default:
			break;
		}
	}
	else
	{
		switch (rcvRtn)
		{
    case MHnoMsg:	/* no messages */
			CRCSOPDEBUG(CRusli+CRmsginFlg, ("no msg received"));
			break;
    case MHtimeOut: /* time out (no messages) */
			CRCSOPDEBUG(CRusli+CRmsginFlg, ("time out, no msg received"));
			break;
    case MHintr:
			CRCSOPDEBUG(CRusli+CRmsginFlg, ("interrupt, no msg received"));
			break;
    case EHNOTMREXP:
#ifdef EES
			break;
#else
			CRCSOPERROR("getEvent() failed with return code EHNOTMREXP");
			CRgracefulExit();
#endif
    case EHNOEVENTNB:
			CRCSOPDEBUG(CRusli+CRmsginFlg, ("no event"));
			break;
    case MHbadQid:
    case MHfault:
    case MHidRm:
    case MHnoQue:
    case MHnoShm:
			CRCSOPERROR("getEvent() failed with return code (%d)",
                  rcvRtn);
			CRgracefulExit();
    default:
			CRCSOPERROR("getEvent() fail! rtn=%d\n", rcvRtn);
			CRgracefulExit(); /* does not return */
		}
	}
	return NO;
}

/* Get a buffer to receive a message in.
** If none available,
** then steal one from lowest priority queue.
**
*/
static
CRomBuffer*
CRgetBuf()
{
	CRomBuffer* omBuf = CRbufferPool.get();
	if (omBuf == NULL)
	{
		CRCSOPDEBUG(CRusli, ("discarding OM due to lack of buffers"));

		CRstolenBufferCnt++;

		/* need to steal buffer */
		omBuf = CRomQueues.stealBuffer();
	}
	return omBuf;
}

void
CRprocessPendingEvents()
{
	GLretVal rcvRtn;
	GLretVal critRcvRtn;
	short msgsz= MHmsgLimit;
	MHmsgBase* msg_ptr = NULL;
#ifdef LX
	int numOfnonOMs = 0;

#endif

	CRCSOPDEBUG(CRusli+CRmsginFlg, ("START loop "));
	do
	{
		CRomBuffer* omBuf = CRgetBuf();
		if (omBuf == NULL)
		{
			CRCSOPERROR("could not get a buffer");
			return;
		}
		char* msgBuf = omBuf->getMsgBuf();
		short msgsz= MHmsgLimit;

		msg_ptr = (MHmsgBase*) msgBuf;

		// block for 100 mil seconds
		if(CRomQueues.isOMpending() == TRUE) 
		{
      rcvRtn = CRevent.getEvent(mhqid, msgBuf, msgsz,
                                0, FALSE, EHBOTH);
		}
    else 
		{
      rcvRtn = CRevent.getEvent(mhqid, msgBuf, msgsz,
                                0, TRUE, EHBOTH);
    }

#ifdef LX
		++numOfnonOMs;
		if(numOfnonOMs == 100)
		{
			IN_SANPEG();
			numOfnonOMs=0;
		}
#endif
		if (CRprocessMsg(rcvRtn, omBuf, msgsz) == NO)
       CRbufferPool.put(omBuf); /* free the buffer */

		CRCSOPDEBUG(CRusli+CRmsginFlg, ("loop %d",rcvRtn));

		do
		{
      CRomBuffer* critOmBuf = CRgetBuf();
      if (critOmBuf == NULL)
      {
				CRCSOPERROR("could not get a buffer");
				return;
      }
      char* critMsgBuf = critOmBuf->getMsgBuf();
      short critMsgsz= MHmsgLimit;
	
      // non blocking event
      critRcvRtn = CRevent.getEvent(CRITmhqid, critMsgBuf, 
                                    critMsgsz, 0, 
                                    FALSE, EHMSGONLY);
	
      CRCSOPDEBUG(CRusli+CRmsginFlg, ("inner loop %d",
                                      critRcvRtn));
      if (CRprocessMsg(critRcvRtn, critOmBuf, critMsgsz) == NO)
         CRbufferPool.put(critOmBuf); /* free the buffer */
	
		} while (critRcvRtn == GLsuccess);
	} while ((rcvRtn == GLsuccess) && (msg_ptr->msgType != TMtmrExpTyp));
}

short msgsz= MHmsgLimit;
void 
process(int /* argc */, Char* /*argv*/[], SN_LVL, U_char)
{

	/*
	** Process all waiting events/msgs.
	**
	** Process highest priority queued up msg. CRprocessCritPendingEvents
	** return msg buffer to available list.
	**
	** If no more msgs queued up, do nothing. INIT runs process()
	** then wait for next event.
	*/

#ifdef LX
	IN_SANPEG();
#endif
	CRCSOPDEBUG(CRusli, ("In Process Loop"));
		
	CRprocessPendingEvents();

	CRlogFsAudit(); /* check to see how much file space is available */

	if (CRstolenBufferCnt > 0)
	{
		CRcsopMsg om;
		om.spool(&CRstolenBufsOM, CRstolenBufferCnt);
		CRstolenBufferCnt = 0;
	}

	for(int i =0;i < 10;++i)
	{
		CRomBuffer* buffer = CRomQueues.processOM();

		if (buffer)
		{
			CRbufferPool.put(buffer);
		}
		else
		{
			break;
		}
	}
		
}

static Char *_brevGDOaddr = NULL;

void
createBrevShareMemory()
{
	// if share memory is NULL
	// check to see if cr_brevityCntl is non empty
	// if empty return
	// else 
	// create it will take data from cr_brevityCntl table
	//	and put in share memory
	// return

	GLretVal retval;
	Bool isNew = FALSE;
	if( _brevGDOaddr == NULL )
	{
		retval = CRgdo.attach( CRgdoOMtableName, /* name of GDO */
                           FALSE,           /* TRUE create GDO */
                           isNew,
                           CRbrevPerm,      /* permissions */
                           /* size of gdo */
                           (Long)sizeof(CRbrevSharedMemory), 
                           _brevGDOaddr,            /* pointer to GDO */
                           /* set to default values */
                           (Long)(3* sizeof(CRbrevSharedMemory)), 
                           (void*) NULL,

                           MHGD_ALL,   /* put the gdo on all nodes */
                           (Long)sizeof(CRbrevSharedMemory));

		CRshData = (CRbrevSharedMemory *) _brevGDOaddr;

		if( CRshData->noOfRecords == 0 ) /*  empty GDO */
		{
			retval= checkOnPlatTable();
			if(retval == GLfail)
         CRCSOPDEBUG(CRusli,("GDO empty"));
		}
		/*else
      {
			retval= checkOnPlatTable();
			if(retval == GLfail)
      CRCSOPDEBUG(CRusli,("GDO empty"));
      }*/
	}
}


MHqid DBqid = MHnullQ;          // DB Qid

Void sendSQL(const Char *whereclause, const Char *columnname, 
             const Char *tablename);

GLretVal dbSelAck(DBselectAck* SelAck );

GLretVal
checkOnPlatTable()
{
	//Char sql[100];
	GLretVal rtn;
  DBselect dbSelect;

	if ((rtn = MHmsgh.getMhqid("DBI", DBqid)) != GLsuccess)
	{
		CRERROR("Get DBI mhqid failed!  rtn = %d", rtn );
		return GLfail;
	}
	sprintf(dbSelect.sql, "SELECT * FROM CR_BREVITYCNTL");
	//dbSelect.sql
	dbSelect.sqlSz = strlen(dbSelect.sql);
	dbSelect.tblOffset = strlen("SELECT * FROM ");
	dbSelect.whereOffset = 0;
	dbSelect.endFlag = TRUE;

	rtn = GLfail;
  int whileCounter = 0;
  do
  {
    rtn = dbSelect.send(DBqid, mhqid, 0);
    ++whileCounter;
    if(whileCounter > 22)
    {
      break;
    }
  } while ((rtn == MHintr) || (rtn == MHagain));


	if (rtn != GLsuccess)
	{
		CRERROR("send failed; returned %d", rtn);
	}

	return getDataForPlatDB();
}

static class rcvMsg : public MHmsgBase {
public:
  union {
    Long    l_align;
    double  d_align;
    void   *p_align;
    Char body[MHmsgLimit - sizeof(MHmsgBase)];
  };
} dbMsg;

static Char *msgp = (Char *)&dbMsg;


GLretVal
getDataForPlatDB()
{
	GLretVal rtn;
	Short msgsz;

	while (1)
	{
		/*
		** 10 seconds maximum wait; this is an arbitrary time to wait
		*/
		msgsz = sizeof(dbMsg);;
		if ((rtn = MHmsgh.receive(mhqid,msgp,msgsz,0,10000)) != 
        GLsuccess)
		{
			if (rtn == MHintr) //happens frequently in PI in the EE!
         continue;
			if ((rtn != MHnoMsg) && (rtn != MHtimeOut))
			{
				CRERROR("MHmsgh.receive() fail! rtn=%d errn=%d",
                rtn,errno);
				return GLfail;
			}
			if (rtn == MHtimeOut)
			{
        CRERROR("Did not receive all needed DBselectAcks");
        return GLfail; 
			}

			continue; // try again to get a message
		}
		
		switch(dbMsg.msgType) 
		{
      IN_SANPEG();
			
    case DBselectAckTyp:
       {
         DBselectAck *selAckPtr = (DBselectAck *) msgp;
         dbSelAck((DBselectAck *) msgp);

         // send ACK back to DBI
         if (selAckPtr->reqAck == 1)
         {
           CRCSOPDEBUG(CRusli,("SENDING AN ACK BACK TO DBI"));
           DBackBack ackBack;
           ackBack.sid = selAckPtr->sid;
           if ((rtn = ackBack.send(DBqid, 
                                   mhqid, 0)) != GLsuccess) 
           {
             CRERROR("Fail DBackBack to DBI rtn=%d", rtn);
           }
         }

         if(selAckPtr->endFlag == TRUE)
            return GLsuccess;
         else
         {
           break;
         }
		   }

    default:
       {
         CRCSOPDEBUG(CRusli, 
                     ("Unexpected Message type %0Xh",dbMsg.msgType));
         continue;
       }
		}// end of switch
	}// end of while true

	/*
	**	Should never reach this point
	*/
	return GLfail;
}

GLretVal
dbSelAck(DBselectAck* SelAck )
{
	U_char *pc = (U_char *) SelAck->nvlist;// pointer for parsing Ack message

	Char fieldName[30]; 
	Char fieldValue[256]; 
	int i = 0;
	int fieldCnt = 0;
	Short nameLen, dataLen;
	int recNum=0;

	//CRshData->noOfRecords=1;
	for(i = 1;i <= SelAck->npairs; ++i)
	{
		if( recNum >= CRnoOfBrevTuples )
		{
      CRDEBUG(CRusli, ("Too many OMs under brevity control"));
			break;
		}

		nameLen = *pc++;
		strncpy(fieldName,(const char*) pc,nameLen);
		fieldName[nameLen]='\0';

		pc += nameLen;
		dataLen = *pc++;
		strncpy(fieldValue,(const char*) pc,dataLen);
		fieldValue[dataLen]='\0';

		pc += dataLen;

		if ( (strcmp(fieldName, "PROCESSNAME")) == 0 )
    {
      strncpy(CRshData->records0[recNum].process,fieldValue,
              sizeof(CRshData->records0[recNum].process));
      CRshData->records0[recNum].process[strlen(fieldValue)] = '\0';
      fieldCnt = 0;
    }

		if ( (strcmp(fieldName, "OMKEY")) == 0 )
    {
			strncpy(CRshData->records0[recNum].omkey,fieldValue,
              sizeof(CRshData->records0[recNum].omkey));
      CRshData->records0[recNum].omkey[strlen(fieldValue)] = '\0';
      fieldCnt = 1;
    }

		if ( (strcmp(fieldName, "PERIOD")) == 0 )
    {
 			CRshData->records0[recNum].period=atoi(fieldValue);
      fieldCnt = 2;
    }

		if ( (strcmp(fieldName, "UPPER")) == 0 )
    {
      CRshData->records0[recNum].upper =atoi(fieldValue);
      fieldCnt = 3;
    }

		if ( (strcmp(fieldName, "LOWER")) == 0 )
    {
      CRshData->records0[recNum].lower =atoi(fieldValue);
      fieldCnt = 4;
		}

		if ((nameLen != 0) || (dataLen != 0))
    {
      if(fieldCnt == 4)
      {

        ++CRshData->noOfRecords;
				CRgdo.invalidate( &CRshData->noOfRecords, sizeof(int) );
				CRgdo.invalidate( &CRshData->records0[recNum], 
                          sizeof(CRbrevRecord) );
        fieldCnt=0;
				++recNum;
			}
		}
	}

	if(recNum > 0)
     return GLsuccess;
	else
     return GLfail;
}
