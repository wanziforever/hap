/*
**      File ID:        @(#): <MID18641 () - 09/06/02, 29.1.1.1>
**
**	File:					MID18641
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:22
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This file contains functions to send mesages to the
**	the Output Message Spooler (CSOP).
**	It also contains the member functions that deal with
**	receiving/dumping out the output messages received by CSOP.
**
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/utsname.h>
#include <ctype.h>

#include "cc/hdr/cr/CRspoolMsg.hh"
//#include "cc/cr/hdr/CRshtrace.hh"
#include "cc/hdr/cr/CRmtype.hh"
#include "cc/hdr/cr/CRomDest.hh"
#include "cc/hdr/cr/CRtmstamp.hh"
#include "cc/hdr/cr/CRloadName.hh"
#include "cc/hdr/cr/CRofficeNm.hh"
//#include "cc/hdr/cr/CRsysError.hh"
#include "cc/hdr/cr/CRdirCheck.hh"
//#include "cc/hdr/misc/GLasync.hh"
#include "cc/hdr/cr/CRomHdrFtr.hh"
//#include "cc/cr/hdr/CRomClEnt.hh"
#include "cc/hdr/cr/CRassertIds.hh"    // for CRNullPointerId, FMT

static const char blanks[] = "   ";
static const Char CSOPprocName[] = "CSOP";
static const Char CritCSOPprocName[] = ":CritCSOP"; // Will have hots tacked on
MHqid CRspoolMsg::CSOPqid = MHnullQ;
char CRspoolMsg::hostname[] = "";
short CRspoolMsg::lostMsgCount = 0;
short CRspoolMsg::loggedMsgCount = 0;
const short CRspoolMsg::maxLineLen = 80;/* three blanks which are in front of 
                                         * each line are not included in this 
                                         * count 
                                         */

const char* CRspoolMsg::directory = 
#ifdef CC
   "/sn/log/CRmsg";
#else
"./CRmsg";
#endif

const int maxSegInt = 30;
const int minSegInt = 2;


CRspoolSeg::CRspoolSeg(int seg)
{
	segment = seg;
	segText[0]='\0';

}
CRspoolSeg::~CRspoolSeg()
{
}

void
CRspoolMsg::audit()
{
}

void
CRspoolMsg::incrLostCount()
{
	lostMsgCount++;
}

void
CRspoolMsg::avoidSegmentation()
{
	if (segment <= 0)
     segmentFlag = NO;
}

void
CRspoolMsg::clearLostCount()
{
	lostMsgCount = 0;
}

void
CRspoolMsg::incrLoggedCount()
{
	loggedMsgCount++;
}

void
CRspoolMsg::clearLoggedCount()
{
	loggedMsgCount = 0;
}

void
CRspoolMsg::spool()
{
  if (lnbuf[3] != '\0')
     add(lnbuf);
  sendSegs(currentSeg,NO);
}
	

CRspoolMsg::CRspoolMsg(Bool buffering)
{
	CRSentToCsop = TRUE;
	if (buffering == YES)
     turnOnBuffering();
	else
     turnOffBuffering();

	/* point the function pointer here to the send() routine */
	sendfn = &CRspoolMsg::send;

	/* set the sender machine name and state here */
	char    theHostName[ sizeof(msghead.senderMachine) ];
	//char    theSystemHostName[ sizeof(msghead.senderMachine) ];
	char    theSystemHostName[ MHmaxNameLen+1 ];
  int     retval = 0;
 
	retval= MHmsgh.getMyHostName(theHostName);

#ifdef LX

	if (retval != GLsuccess)
	{
		strncpy(msghead.senderMachine, "UNK",
            sizeof(msghead.senderMachine));
	}
	else
	{
// The following call to logicalToSystem() always succeeds since
// Machine_Name is the returned value of getMyHostName() above.
// If no system is defined for this machine, then Machine_Name
// will retian its logical name.  Otherwise, the R-C-S value
// will be returned.
		if ((retval = MHmsgh.logicalToSystem(theHostName, theSystemHostName))
				!= GLsuccess)
		{
			strncpy(msghead.senderMachine, "UNK",
              sizeof(msghead.senderMachine));
		}
		else
		{
			strncpy(msghead.senderMachine, theSystemHostName,
              sizeof(msghead.senderMachine));
		}
	}
#else 

	if (retval != GLsuccess)
	{
		strncpy(msghead.senderMachine, "UNK",
            sizeof(msghead.senderMachine));
	}
	else
	{
		strncpy(msghead.senderMachine, theHostName,
            sizeof(msghead.senderMachine));
	}
#endif

//IBM JGH 05/04/06 warning: NULL used in arithmetic

	if(0 == strncmp(MHmsgh.myNodeState(),"ACTIVE",
                  sizeof(MHmsgh.myNodeState())))
	{
		strncpy(msghead.senderState, "ACT",
            sizeof(msghead.senderState));
	}
//IBM JGH 05/04/06 warning: NULL used in arithmetic
	else if(0 == strncmp(MHmsgh.myNodeState(),"UNKNOWN",
                       sizeof(MHmsgh.myNodeState())))
	{
		strncpy(msghead.senderState, "UNK",
            sizeof(msghead.senderState));
	}
	else
	{
		strncpy(msghead.senderState, MHmsgh.myNodeState(),
            sizeof(msghead.senderState));
  }
  msghead.senderState[strlen(MHmsgh.myNodeState())+1]='\0';

	CRSentToCsop = TRUE;
	currentSeg = 0;
	init();
	initMsgInfo();
}

void 
CRspoolMsg::initMsgInfo()
{
	/* The follwoing fields are only need to be initialized on 
	 * message boundary and on a segment boundary
   */
	segment = -1;
	segInterval = 2;	/* set to 2 seconds */
	omTitle[0] = '\0';
	segmentFlag = YES;
	firstLine = NO;

	strcpy(lnbuf, blanks);
	currentPos = strlen(blanks);

	msghead.usliName[0] = '\0';
	msghead.omkey[0] = '\0';
	setAlarmLevel(POA_INF);
}

void
CRspoolMsg::init()
{
	/* the following fields are need to be initialized after 
	 * a segment is sent to CSOP process 
   */

	//
	// This check is here for brevity control.
	// When a message (OM) is in brevity control the
	// seg is created(allocSeg) but was never deleted.
	// This will called deleteSegs and free up the memory.
	//
	if(CRSentToCsop != TRUE )
	{
		if( seg[currentSeg] != NULL )
		{
			deleteSegs(currentSeg);
			initMsgInfo();
		}
	}

	msgText[0] = '\0';
	MHmsgBase::msgType = CRgenOMTyp;
	
	msghead.length = 0;
	msghead.segStr[0] = '\0';
	msghead.fileFlag = NO;
	msghead.timestamp = 0;
	strcpy(msghead.senderProcNm, CRprocname);
	nextAvailableChar = 0;
	fileName[0] = '\0';
	currentPos = 0; 
	errFlag = NO;

}

CRALARMLVL
CRspoolMsg::alarmLevel() const
{
	return CRstr_to_POA(getAlarmLevel());
}

void
CRspoolMsg::setUSLIname(const char* origTerminal)
{
	if (strlen(origTerminal) > MHmaxNameLen)
	{
		strncpy(msghead.usliName, origTerminal, MHmaxNameLen);
		msghead.usliName[MHmaxNameLen] = '\0';
	}
	else
     strcpy(msghead.usliName, origTerminal);
}

void
CRspoolMsg::reset()
{
	init();
/*
	initMsgInfo();
*/
}

const char*
CRspoolMsg::getTime() const
{
	static char timebuf[30];
	CRformatTime(msghead.timestamp, timebuf);
	return timebuf;
}

void
CRspoolMsg::getOMhdr(char outbuf[], int seqnum)
{
	char*    time_buf;
	char*    msgclass;
	char*    segstr;
	const char*    omdbkey;


	time_buf = (char*)getTime();
	msgclass = (char*)getClass();
	omdbkey =  getOMkey();
	segstr =   (char*)getSegStr();

	if( omdbkey[0] == '\0' )
  {
    /*
    ** pass in omdbkey as CRDEFOMDBKEY="      " because CRmsg got
    ** no omdbkeys; omdbkey is for OMDB.
    */
		omdbkey = CRDEFOMDBKEY;
	}

	static char hdr_str[200];

	strcpy(hdr_str , CRgenOMhdr(time_buf, msgclass, (char*)omdbkey, seqnum, 
	                            segstr, msghead.senderMachine, 
	                            msghead.senderState));
	strcpy(outbuf, hdr_str);
}

const char*
CRspoolMsg::getHostName()
{
	if (hostname[0] == '\0')
	{
		struct utsname un;
		int retval = uname(&un);
		if (retval != 0)
		{
			//CRSHERROR("uname failed with error code %d",
			//	  retval);
			strcpy(hostname, "UNKNOWN");
		}
		else
		{
			/* copy and capitalize the system name */
			char *t, *h;
			for (t = un.sysname, h = hostname;
			     *t; t++, h++)
         *h = toupper(*t);
			*h = '\0';
		}
		
			
	}

	return hostname;
}

void
CRspoolMsg::setAlarmLevel(CRALARMLVL alarmIndex)
{
	MHmsgBase::priType = alarmIndex; /* MSGH message priority */

	strncpy(msghead.alarmLevel, CRPOA_to_str(alarmIndex), 3);
	msghead.alarmLevel[2] = '\0';
}

void
CRspoolMsg::initForSend()
{
 	setState(); // reset the CC state

	saveTime(); /* put the current time into the message */

	setLostOMcounter(lostMsgCount);
	setLoggedOMcounter(loggedMsgCount);
}

/* This function is needed by CRcsopMsg
** It should do all last second formatting of the message contents
*/
GLretVal
CRspoolMsg::prepForSend()
{
	if (errFlag == YES)
     return GLfail;

	initForSend();

	/* if the message is stored in a file (due to its length)
	** then copy the filename into the msg
	*/
	if (getFileFlag())
	{
		if (pageOut() != GLsuccess)
		{
			errFlag = YES;
			return GLfail;
		}

		strcpy(msgText, fileName);
	}
	else
	{
		strcpy(msgText, seg[0]->segText);
    msghead.length = strlen(msgText);
	}
	deleteSegs(0);
	initMsgInfo();

	return GLsuccess;
}

GLretVal
CRspoolMsg::send()
{
	CRSentToCsop = TRUE;
	MHqid qid;

	/*
	** put message state and host name in message
	*/
	char    theHostName[ sizeof(msghead.senderMachine) ];
	char    theSystemHostName[ MHmaxNameLen+1 ];
  int     retval = 0;
 
	retval= MHmsgh.getMyHostName(theHostName);

#ifdef LX

	if (retval != GLsuccess)
	{
		strncpy(msghead.senderMachine, "UNK",
            sizeof(msghead.senderMachine));
	}
	else
	{
// The following call to logicalToSystem() always succeeds since
// Machine_Name is the returned value of getMyHostName() above.
// If no system is defined for this machine, then Machine_Name
// will retian its logical name.  Otherwise, the R-C-S value
// will be returned.
		if ((retval = MHmsgh.logicalToSystem(theHostName, theSystemHostName))
				!= GLsuccess)
		{
			strncpy(msghead.senderMachine, "UNK",
              sizeof(msghead.senderMachine));
		}
		else
		{
			strncpy(msghead.senderMachine, theSystemHostName,
              sizeof(msghead.senderMachine));
		}
	}
#else 

	if (retval != GLsuccess)
	{
		strncpy(msghead.senderMachine, "UNK",
            sizeof(msghead.senderMachine));
	}
	else
	{
		strncpy(msghead.senderMachine, theHostName,
            sizeof(msghead.senderMachine));
	}
#endif

//IBM JGH 05/04/06 warning: NULL used in arithmetic
	if(0 == strncmp(MHmsgh.myNodeState(),"ACTIVE",
                  sizeof(MHmsgh.myNodeState())))
	{
		strncpy(msghead.senderState, "ACT",
            sizeof(msghead.senderState));
	}
//IBM JGH 05/04/06 warning: NULL used in arithmetic
	else if(strncmp(MHmsgh.myNodeState(),"UNKNOWN",
                  sizeof(MHmsgh.myNodeState())) == 0)
	{
		strncpy(msghead.senderState, "UNK",
            sizeof(msghead.senderState));
	}
	else
	{
		strncpy(msghead.senderState, MHmsgh.myNodeState(),
            sizeof(msghead.senderState));
  }
  msghead.senderState[strlen(MHmsgh.myNodeState())+1]='\0';

	initForSend();
	if (errFlag == YES)
	{
		incrLostCount();
		reset();
		return GLfail;
	}


	/*
  ** If this is the first send to CSOP must get its queue id
  **
  ** If the OM has a ALARM level of Critical or Major use
  ** CSOP critical queue else use CSOP other queue.
  */
	if (CSOPqid == MHnullQ)
	{
		if (MHmsgh.getMhqid(CSOPprocName, CSOPqid) != GLsuccess)
		{
      /* If CSOP does not have a queue id
      ** then log the message locally.
      */
			CSOPqid = MHnullQ;
			logMsgLocally();
			reset();
			return GLfail;
		}
	}

  if((priType == POA_CRIT) ||
     (priType == POA_MAJ))
  {
		// Crit queue can't be global, since we can't have
		// two.  Look up where the regular global queue is routing to
		// and direct the query to the Crit queue on that machine.
		MHqid realcsop;
		char critname[2*(MHmaxNameLen+1)];

		if (MHmsgh.global2Real(CSOPqid, realcsop) != GLsuccess)
		{
			CSOPqid = MHnullQ;
			logMsgLocally();
			reset();
			return GLfail;
		}

		if (MHmsgh.hostId2Name(MHmsgh.Qid2Host(realcsop), critname) !=
        GLsuccess)
		{
			CSOPqid = MHnullQ;
			logMsgLocally();
			reset();
			return GLfail;
		}

		strcat(critname, CritCSOPprocName);

		if (MHmsgh.getMhqid(critname, qid) != GLsuccess)
		{
			/* If CSOP does not have a queue id
			** then log the message locally.
			*/
			qid = MHnullQ;
			logMsgLocally();
			reset();
			return GLfail;
		}
  }
  else
	{
    qid = CSOPqid;
	}
	Short msgsz = MHmsgBaseSz + CRsplMsgHdSz;
	if (getFileFlag())
     msgsz += strlen(msgText) + 1;
	else
	{
		int textlen = textLength();
		if (textlen == 0)
		{
			//CRSHERROR("Output message has no text. Message not sent.");
			reset();
			return GLfail;
		}

		msgsz += textlen + 1;
	}

	Long timeParm = 0;

	GLretVal rtn = MHmsgBase::send(qid, MHnullQ, msgsz, timeParm);
	switch (rtn)
	{
  case MHagain:
		incrLostCount();
		this->cleanup(); /* removes tmp file, if it exists */
		break;

  case MHbadName:
		/* If CSOP is not registered with MSGH
		** then log the message locally, and clear CSOPqid
		*/
		CSOPqid = MHnullQ;
		logMsgLocally();
		break;

  case GLsuccess:
		clearLostCount();
		clearLoggedCount();
		break;

  default:
		/* If failed for some other reason (like a MSGH failure)
		** then just toss the message.
		*/
		CSOPqid = MHnullQ;
		this->cleanup(); /* removes tmp file, if it exists */
		incrLostCount();
		break;
	}

	reset();
	return rtn;
}

void
CRspoolMsg::logMsgLocally()
{
	// Change the name of /sn/log/locallogs/machineProcname
	// to be copy to LEAD CC when FTMON comes up on the
	// machine .
  char machProcName[MHmaxNameLen+27]; //add machine 26 plus one for NULL

  // the senderMachine should be set but if not set it to UNKNOWN

  //IBM JGH 05/04/06  warning: NULL used in arithmetic

  if((strcmp(msghead.senderMachine,"")) == 0)
  {
    sprintf(machProcName,"%s.%s.",CRprocname,"UNK");
  }
  else
  {
    sprintf(machProcName,"%s.%s.",CRprocname,msghead.senderMachine);
  }

  CRlocalLogSop logerr;
  if (logerr.init(machProcName) == GLsuccess)
  {
    incrLoggedCount();
    sendToSop(&logerr);
  }
  else
  {
    this->cleanup();
    incrLostCount();
  }
}

static const int CRMAXINBUF = 81;

void
CRspoolMsg::writeln(int fd)
{
	static char hdr_str[200];
	int seqnum=999999;

/*
** Attach the OM header here
*/
	getOMhdr(hdr_str, seqnum);
	::write(fd, (const char*) hdr_str, strlen(hdr_str));
	::write(fd, getAlarmLevel(), 2);
	::write(fd, " ", 1);

	if (getFileFlag() == NO)
     ::write(fd, msgText, textLength());
	else
	{
		FILE *fptr = fopen(msgText, "r");
		if (!fptr) {
			CRERROR("could not open file '%s' for read", msgText);
			return;
		}
		char inbuf[CRMAXINBUF];
		int bytes_read;
		while (bytes_read = fread(inbuf, sizeof(char),
                              CRMAXINBUF, fptr)) {
			::write(fd, inbuf, bytes_read);
		}
		fclose(fptr);
	}

/*
** Attach the OM footer here
*/
	static char ftr_str[200];

	strcpy(ftr_str , CRgenOMftr(seqnum));
	::write(fd, ftr_str, strlen(ftr_str));
	::write(fd, "\n\n", 2);
}

void
CRspoolMsg::sendToSop(CRomDest* sop1, CRomDest* sop2,
                      CRomDest* sop3, CRomDest* sop4, CRomDest* sop5)
{
	CRomDest* soparray[5];
	int numsops = 0;

	if (sop1)
	{
		soparray[numsops++] = sop1;

		/* print to other SOPs */
		if (sop2)
       soparray[numsops++] = sop2;

		if (sop3)
       soparray[numsops++] = sop3;

		if (sop4)
       soparray[numsops++] = sop4;

		if (sop5)
       soparray[numsops++] = sop5;
	}

	sendToSop(soparray, numsops);
}

void
CRspoolMsg::sendToSop(CRomDest* soparray[], int numSOPs)
{
	sendToSop(soparray, numSOPs, 999999);
}

void
CRspoolMsg::sendToSop(CRomDest* soparray[], int numSOPs, int seqnum)
{
	if (numSOPs <= 0)
	{
		logMsgLocally();
		return;
	}
	
	char hdr_str[CRHDRFTRSZ];
/*
** Attach the OM header here
*/
	getOMhdr(hdr_str, seqnum);


	// start to determine buffer size needed for one OM
	char* dynamicBufPtr;

	int sizeOfBuffer = (2 * sizeof(hdr_str)) +
     strlen(getAlarmLevel()) +
     sizeof(" ");
	int dynamicBufLen = 0;

	if (getFileFlag() == NO) {

		sizeOfBuffer = sizeOfBuffer + sizeof(msgText);
		dynamicBufPtr = (char*)calloc(1, sizeOfBuffer);

    /*
    **	put OM in
    */
    snprintf(dynamicBufPtr, sizeOfBuffer - 1,
             "%s%s %s%s", hdr_str, getAlarmLevel(), msgText,
             CRgenOMftr(seqnum));

		// remove extra \n in dynamicBufPtr
		dynamicBufPtr[sizeOfBuffer-1] = '\0';
		dynamicBufLen = strlen(dynamicBufPtr) - 1;
	} else {
		int fd;
		if ((fd = open(msgText, O_RDONLY)) < 0 ) {
			CRERROR("could not open file '%s' for read", msgText);
			unlink(msgText); 
			return;
		}

		struct stat st;
		if(fstat(fd,&st)<0) {
			close(fd);
			CRERROR("could not stat file '%s' for size", msgText);
			unlink(msgText); 
			return;
		}

		int sizeOfFile = st.st_size;
		sizeOfBuffer = sizeOfBuffer + sizeOfFile;
		dynamicBufPtr = (char*)calloc(1, sizeOfBuffer);
		char *fileBufPtr;
		fileBufPtr = new char[sizeOfFile];

		//
		//	Get OM body
		//
		int bytesRead;
		if ((bytesRead = read(fd, (void*) fileBufPtr, sizeOfFile)) < 0 )
		{
			close(fd);
			delete [] fileBufPtr;
			CRERROR("could not read file '%s'", msgText);
			unlink(msgText); 
			return;
		}

    /*
    **	put OM in
    */
    snprintf(dynamicBufPtr, sizeOfBuffer,
             "%s%s %s%s", hdr_str, getAlarmLevel(), fileBufPtr,
             CRgenOMftr(seqnum));

    dynamicBufLen = strlen(dynamicBufPtr) - 1;

		delete [] fileBufPtr;
		close(fd);
		unlink(msgText); 
	}

	CRALARMLVL almLevel = alarmLevel();
	for (int i = 0; i < numSOPs; i++) {
		if (soparray[i] == NULL) {
			//CRSHERROR("soparray[%d] is NULL", i);
			continue;
		}
		soparray[i]->send((const char*) dynamicBufPtr,
                      dynamicBufLen, almLevel);
	}
	free(dynamicBufPtr);
}


#ifdef LX
void
CRspoolMsg::sendToSop(CRomDest* soparray[], int numSOPs, int seqnum, Bool sendToLeadCSOP)
{
	if (numSOPs <= 0)
	{
		logMsgLocally();
		return;
	}
	
	char hdr_str[CRHDRFTRSZ];
/*
** Attach the OM header here
*/
	getOMhdr(hdr_str, seqnum);


	// start to determine buffer size needed for one OM
	char* dynamicBufPtr;
	char x733Buf [200];

	int sizeOfBuffer = (2 * sizeof(hdr_str)) +
     strlen(getAlarmLevel()) +
     sizeof(x733Buf) +
     sizeof(" ");
	int dynamicBufLen = 0;

	if (getFileFlag() == NO)
	{

		sizeOfBuffer = sizeOfBuffer + sizeof(msgText);
		dynamicBufPtr = (char*)calloc(1, sizeOfBuffer);

	
    //IBM JGH 05/04/06  warning: NULL used in arithmetic

		if(0 != (strcmp("alarmTypeUndefined",
                    (const char*) msghead.x733alarms.getAlarmTypeValue())))
		{
      /*
      **	put OM in with x733 stuff
      */
			snprintf(x733Buf, sizeof(x733Buf) - 1,
               "::%s::%s::%s::%s::%s::",
               (const char*) msghead.x733alarms.getAlarmTypeValue(),
               (const char*) msghead.x733alarms.getProbableCauseValue(),
               (const char*) msghead.x733alarms.getAlarmObjectName(),
               (const char*) msghead.x733alarms.getSpecificProblem(),
               (const char*) msghead.x733alarms.getAdditionalText());
			snprintf(dynamicBufPtr, sizeOfBuffer - 1,
               "%s%s %s\n%s%s",hdr_str,getAlarmLevel(), msgText,
               x733Buf,
               CRgenOMftr(seqnum));
		}
		else
		{
      /*
      **	put OM in
      */
			snprintf(dynamicBufPtr, sizeOfBuffer - 1,
               "%s%s %s%s",hdr_str,getAlarmLevel(), msgText,
               CRgenOMftr(seqnum));
		}

		// remove extra \n in dynamicBufPtr
		dynamicBufPtr[sizeOfBuffer-1] = '\0';
		dynamicBufLen=strlen(dynamicBufPtr)-1;
	}
	else
	{
		int fd;
		if((fd = open(msgText,O_RDONLY)) < 0 )
    {
			CRERROR("could not open file '%s' for read",
              msgText);
			unlink(msgText); 
			return;
		}

		struct stat st;
		if(fstat(fd,&st)<0)
		{
			close(fd);
			CRERROR("could not stat file '%s' for size",
              msgText);
			unlink(msgText); 
			return;
		}

		int sizeOfFile = st.st_size;
		sizeOfBuffer = sizeOfBuffer + sizeOfFile + sizeof(x733Buf);
		dynamicBufPtr = (char*)calloc(1, sizeOfBuffer);
		char *fileBufPtr;
		fileBufPtr = new char[sizeOfFile];

		//
		//	Get OM body
		//
		int bytesRead;
		if ((bytesRead = read(fd, (void*) fileBufPtr, sizeOfFile)) < 0 )
		{
			close(fd);
			delete [] fileBufPtr;
			CRERROR("could not read file '%s'",
              msgText);
			unlink(msgText); 
			return;
		}

//IBM JGH 05/04/06  warning: NULL used in arithmetic
		if(0 != (strcmp("alarmTypeUndefined",
                    (const char*) msghead.x733alarms.getAlarmTypeValue())))
		{
      /*
      **	put OM in with x733 stuff
      */
			snprintf(x733Buf, sizeof(x733Buf) - 1,
               "::%s::%s::%s::%s::%s::",
               (const char*) msghead.x733alarms.getAlarmTypeValue(),
               (const char*) msghead.x733alarms.getProbableCauseValue(),
               (const char*) msghead.x733alarms.getAlarmObjectName(),
               (const char*) msghead.x733alarms.getSpecificProblem(),
               (const char*) msghead.x733alarms.getAdditionalText());
			snprintf(dynamicBufPtr, sizeOfBuffer,
               "%s%s %s\n%s%s",hdr_str,getAlarmLevel(), fileBufPtr,
               x733Buf,
               CRgenOMftr(seqnum));
		}
		else
		{
      /*
      **	put OM in
      */
			snprintf(dynamicBufPtr, sizeOfBuffer,
               "%s%s %s%s",hdr_str,getAlarmLevel(), fileBufPtr,
               CRgenOMftr(seqnum));
		}

		/*snprintf(dynamicBufPtr, sizeOfBuffer,
			"%s%s %s%s",hdr_str, getAlarmLevel(), fileBufPtr,
			CRgenOMftr(seqnum));*/
		dynamicBufLen=strlen(dynamicBufPtr)-1;

		delete [] fileBufPtr;
		close(fd);
		unlink(msgText); 
	}

	CRALARMLVL almLevel = alarmLevel();
	for (int i = 0; i < numSOPs; i++)
	{
		if (soparray[i] == NULL)
		{
			//CRSHERROR("soparray[%d] is NULL", i);
			continue;
		}
    soparray[i]->setAlarmType( msghead.x733alarms.getAlarmType());
    soparray[i]->setProbableCause( msghead.x733alarms.getProbableCause());
    soparray[i]->setAlarmObjectName((const char*) msghead.x733alarms.getAlarmObjectName());
    soparray[i]->setSpecificProblem((const char*) msghead.x733alarms.getSpecificProblem());
    soparray[i]->setAdditionalText((const char*) msghead.x733alarms.getAdditionalText());

		soparray[i]->send((const char*) dynamicBufPtr,
                      dynamicBufLen, almLevel);

	}

	if(sendToLeadCSOP)
	{
    int  rtn;
		char finalMsg[5000];
		memset((char *) &finalMsg, '\0', sizeof(finalMsg));
		snprintf(finalMsg, sizeof(finalMsg) - 1,"%s",(const char*) dynamicBufPtr);

    CRcentLogMsg centLogMsg(finalMsg, strlen(finalMsg));

    //  send message
    if ((rtn = centLogMsg.send()) != GLsuccess)
    {
      switch (rtn)
      {
      case MHbadName:
        CRERROR("send() failed MHbadName");
        break;
      default:
        CRERROR("send() failed with error code %d", rtn);
        break;
      }//end of switch
    }//end of if send
	}

	free(dynamicBufPtr);
}
#endif

void
CRspoolMsg::abort ()
{
	if (getFileFlag() == YES)
	{
		if (unlink(fileName) == -1)
		{
/*
  CRSHERROR("could not delete file '%s'", fileName);
*/
		}
	}
}

void
CRspoolMsg::cleanup() const
{
	if (getFileFlag() == YES)
	{
		if (unlink(msgText) == -1 && errno != ENOENT)
		{
/*			CRSHERROR("could not delete file '%s' due to errno %d",
        msgText, errno);
*/
		}
	}
}

void
CRtempnam(const char* dir, char result[])
{
	/* Maximum number of temp files at one time for each process */
	const int CRmaxTmpFiles = 10;

	//
	//      This is to make the static seed thread safe
	//
	static mutex_t CRstaticLock;

	mutex_lock(&CRstaticLock);
	static int seed = 0;

	sprintf(result, "%s/%s.tmp%d", dir, CRprocname, seed);
	seed = (seed + 1) % CRmaxTmpFiles;
	mutex_unlock(&CRstaticLock);
}

GLretVal
CRspoolMsg::pageOut()
{
	int bufLen = nextAvailableChar;
	nextAvailableChar = 0;

	int fd = -1;
	int openMode = 0;

	if (getFileFlag() == NO)
	{
		firstLine = YES;

		if (CRdirCheck(dirname(), YES, "ainet") == NO)
		{
			//CRSHERROR("could not create directory %s",
      //          (const char*) dirname());
			return GLfail;
		}

		CRtempnam(dirname(), fileName);

		/* if file already exists,
		** then abort this OM.
		**      This is a throttling mechanism.
		*/
		if (access(fileName, F_OK) == 0)
		{
			//CRSHERROR("temp file %s already exists",
      //          fileName);
			return GLfail;
		}

		openMode = O_WRONLY | O_CREAT | O_TRUNC;
	}
	else
     openMode = O_WRONLY | O_CREAT | O_APPEND;

	Bool retry = NO;

  tryOpenAgain:
	fd = open(fileName, openMode, 0666);
	if (fd == -1)
	{
		if (errno == ENOENT && retry == NO)
		{
			if (mkdir(dirname(), 0777) == -1)
			{
				//CRSHERROR("could not create directory '%s' due to errno %d",
        //          dirname(), errno);
			}
			else
			{
				retry = YES;
				goto tryOpenAgain;
			}
		}
		else
		{
			//CRSHERROR("open of file '%s' failed due to '%s'",
      //          fileName, CRsysErrText(errno));
		}
		return GLfail;
	}
	if (firstLine == YES)
	{
		firstLine = NO;
		int noChars = bufLen - strlen(blanks);
		if (write(fd, seg[currentSeg]->segText+strlen(blanks), noChars) != noChars)
		{
			//CRSHERROR("disk write to '%s' failed due to %s",
      //          msgText, CRsysErrText(errno));
			close(fd);
			return GLfail;
		}
	}
	else
	{	

		if (write(fd, seg[currentSeg]->segText, bufLen) != bufLen)
		{
			//CRSHERROR("disk write to '%s' failed due to %s",
      //          msgText, CRsysErrText(errno));
			close(fd);
			return GLfail;
		}
	}
	close(fd);
	if (getFileFlag() == NO)
	{
		setFileFlag();
		if (chmod(fileName, 0666) == -1)
		{
			//CRSHERROR("chmod of '%s' failed", msgText);
			return GLfail;
		}
	}
	return GLsuccess;
}

GLretVal
CRspoolMsg::add(char c)
{
	/* if no room, then save in file, reset, and add to buffer */
	if (nextAvailableChar >= CRsplMaxTextSz - 1)
	{
		if (pageOut() != GLsuccess)
		{
			errFlag = YES;
			return GLfail;
		}
	}
	msghead.length++;
	msgText[nextAvailableChar++] = c;
	return GLsuccess;
}

GLretVal
CRspoolMsg::add(const char* s, int /*len*/)
{
	add(s);
	return GLsuccess;
}

/* this routine will allocate memory to temporarily hold the data for a 
 * segment 
 */
void
CRspoolMsg::allocSeg()
{
  currentSeg = segment%maxSegs;

  seg[currentSeg] = new CRspoolSeg(segment);
  CRSentToCsop = FALSE;


  if ((omTitle[0] != '\0') && (segment > 0))
     add(omTitle);
}	
		
/* This routine will copy the data from temporary segments to msgText 
 * in a loop and will call the appropriate routine to send the data to CSOP 
 * process 
 *
 */
void 
CRspoolMsg::sendSegs(const int lastIndex, Bool more)
{
	if (segmentFlag == NO)
	{
		if (getFileFlag())
		{
			if (pageOut() != GLsuccess)
			{
				errFlag = YES;
			}

			/* msgText is an array of size CRsplMaxTextSz
			** fileName is an array of size 128
			** 128 is smaller so use sizeof(fileName)
			** Null terminate the string for added safety.
			*/
			strncpy(msgText, fileName, sizeof(fileName) - 1);
			msgText[sizeof(fileName) - 1]=0;

			/* appropriate routine pointed by function pointer
       * is called here
       */
			if (sendfn != 0)
         (this->*sendfn)();

			if (more == NO)
			{
				initMsgInfo();
			}
			deleteSegs(lastIndex);
			return;
		}
		/* let it fall as there is only one segment and we can send it 
		 * it in the MSGH message itself 
     */
	}
		
	for (int i=0; i<=lastIndex; i++)
	{
		/* leave the first three leading blanks in the first line 
		 * as CSOP adds those blanks while inserting alarm level 
     */

		/* check if seg[i] is NULL
		** This could be NULL if there is some logic problem
		**  a) in the for loop: i=0, i<= lastIndex
		**      could there possibly be a case when i should not
		**      start at 0 or should be < lastIndex  not <= ??
		**  
		*/
		if (seg[i] == 0)
		{
			/* we should quickly draw attention to this
			** problem with atleast the following information:
			**  i, lastIndex 
			** cannot use CRCFTASSERT because we might not be
			** attached to MSGH yet
			*/
			CRmsg om;
			om.setMsgClass("ASRT");
			om.setAlarmLevel(POA_ACT);

			om.add(CRasrtFmt, CRNullPointerId,
             CRprocname, __FILE__, __LINE__);

			char pointerName[100];
			sprintf(pointerName, "seg[%d]", i);

			char otherVariableValues[100];
			sprintf(otherVariableValues, "lastIndex=%d", lastIndex);

			om.add(CRNullPointerFMT, pointerName,
             otherVariableValues);
			om.spool();
			return;
		}

		/* Both msgText and segText are arrays of size CRsplMaxTextSz
		** so copy upto the string terminator or CRsplMaxTextSz
		** - 1 - sizeof the the blanks.
		** Null terminate the string for added safety.
		*/
		int blankLength = strlen(blanks);
		strncpy(msgText, seg[i]->segText+blankLength,
            CRsplMaxTextSz - 1 - blankLength);
		msgText[CRsplMaxTextSz - 1 - blankLength]=0;

		msghead.length = strlen(msgText);
		if (segment > 0)
		{
			/* sleep here, otherwise CSOP will get flooded 
			 * with messages 
       */
			sleep(segInterval);
			/* the first character in the segment string has to be 
			 * a blank character to separate this from the 
			 * previous field in the header
       */
			sprintf(msghead.segStr, " SEG#%d",seg[i]->segment);
		}
		/* appropriate routine pointed by function pointer
		 * is called here
     */
		if (sendfn != 0)
       (this->*sendfn)();
	}
	if (more == NO)
	{
		initMsgInfo();
	}
	deleteSegs(lastIndex);
}
		
/* this routine will release the memory used for segments */
void 
CRspoolMsg::deleteSegs(const int lastIndex)
{
	for (int i=0; i<=lastIndex; i++)
	{
		if(seg[i] != NULL)
		{
			delete seg[i];
			seg[i] = NULL;
		}
	}
}

void
CRspoolMsg::newSeg()
{
	if (segment < 0)
     return;
	if (segmentFlag == NO)
	{
		if (pageOut() != GLsuccess)
		{
			errFlag = YES;
		}
		nextAvailableChar = 0;
		seg[currentSeg]->segText[0] = '\0';

		return;
	}
	else
	{
		nextAvailableChar = 0;

		segment++;
		if (currentSeg == (maxSegs-1))
		{
			sendSegs(currentSeg, YES);
			/* delete all the segments */

		}
		/* allocate memory for new segment */
		allocSeg();
	}
}
/* this routine will put the string passed as an argument to the 
 * current segment 
 */
void
CRspoolMsg::add(const char *str)
{
	
	int len;
	len = strlen(str);
	if (segment < 0)
	{
		segment = 0;
		allocSeg();
	}
  else
	{

		if ((strlen(seg[currentSeg]->segText) + len) >= (CRsplMaxTextSz - 1))
		{
			newSeg();
		}
	}
	
 	strcpy((seg[currentSeg]->segText)+nextAvailableChar, str);
 	nextAvailableChar += len;
 	seg[currentSeg]->segText[nextAvailableChar] = '\0';
}

/* This function is called to change the default sleep time between 
 * segments before they are sent to CSOP process.
 * The default is 2 seconds. The valid range is 2 - 30 seconds.
 */
void 
CRspoolMsg::setSegInterval(const int sleepTime)
{
	if (sleepTime < minSegInt || sleepTime > maxSegInt)
	{
		/* generate an assert */
	}

	if (sleepTime < minSegInt)
	{
		/* mininmum time interval between segments before they are 
		 * sent to CSOP process 
     */
		segInterval = minSegInt;
	}
	else if (sleepTime <= maxSegInt)
     segInterval = sleepTime;
	else
     /* maximum time interval between segments before they are 
      * sent to CSOP process 
      */
     segInterval = maxSegInt;
}

void
CRspoolMsg::turnOffBuffering()
{
	maxSegs = 1;
}

void
CRspoolMsg::turnOnBuffering()
{
	maxSegs = CRMAXSEGS;
}

/* this routine will hold the title in omTitle which will be added to 
 * the segment at the current location and also will be used 
 * as a title for subsequent segments 
 * ????How big the title should be???????????????? it should be smaller 
 * than CRsplMaxTextSz as it appears on each subsequent segment
 */

void
CRspoolMsg::title(const char* instring)
{
	int len = strlen(instring);
	if (len == 0)
	{
		return;
	}
	if (len > CRMAXTITLESZ)
	{
		len = CRMAXTITLESZ;
	}
	strcpy(omTitle, blanks);
	int pos = strlen(blanks);
	for (int j=0; j<len; j++)
	{
		omTitle[pos++] = *(instring+j);

		if (*(instring+j) == '\n')
		{
			if (j != (len-1))
			{
				strcpy(omTitle+pos, blanks);
				pos += strlen(blanks);
			}
		}
	}
	if (*(instring+len-1) != '\n')
	{
		omTitle[pos++]= '\n';
		omTitle[pos] = '\0';
	}
	else
     omTitle[pos] = '\0';

	if (segment >=0)
	{
		if ((strlen(seg[currentSeg]->segText) + strlen(omTitle) + maxLineLen) >= (CRsplMaxTextSz - 1))
       newSeg();
		else
       add(omTitle);
	}
	else
	{
		add(omTitle);
		
	}
}
	
void
CRspoolMsg::genNewSeg(Bool oldTitle)
{
	if (oldTitle == NO)
     omTitle[0]='\0';
	newSeg();
}
		
/* copies string into output buffer
** while adding extra blanks after each \n
*/
GLretVal
CRspoolMsg::addblanks(const char* instring)
{
	return addblanks(strlen(instring), instring);
}

/* Copies string into output buffer
** while adding extra blanks after each \n.
** Also adds newlines (and blanks) so lines do not wrap around.
*/
GLretVal
CRspoolMsg::addblanks(int len, const char* instring)
{
	if (currentPos == 0)
	{
		for (int j=0; j<sizeof(blanks)-1; j++)
       lnbuf[currentPos++] = *(blanks+j);
		lnbuf[currentPos] = '\0';
	}

	for (int i=0; i<len; i++)
	{
		if (*(instring+i) == '\n')
		{
			lnbuf[currentPos++]= *(instring+i);
			lnbuf[currentPos] = '\0';
      add(lnbuf);
			currentPos = 0;	
			for (int j=0; j<sizeof(blanks)-1; j++)
         lnbuf[currentPos++] = *(blanks+j);
			lnbuf[currentPos] = '\0';
		}
		else
		{
			if (currentPos >= maxLineLen)
			{
				lnbuf[currentPos++] = '\n';
				lnbuf[currentPos] = '\0';
			
				add(lnbuf);
				currentPos = 0;	
				for (int j=0; j<sizeof(blanks)-1; j++)
           lnbuf[currentPos++] = *(blanks+j);
				lnbuf[currentPos] = '\0';
			}
			lnbuf[currentPos++] = *(instring+i);
		}
	}
	lnbuf[currentPos] = '\0';
	return GLsuccess;
}

void
CRspoolMsg::setState()
{

  //IBM JGH 05/04/06 warning: NULL used in arithmetic
	if(0 == strncmp(MHmsgh.myNodeState(),"ACTIVE",
                  sizeof(MHmsgh.myNodeState())))
	{
		strncpy(msghead.senderState, "ACT",
            sizeof(msghead.senderState));
	}
  //IBM JGH 05/04/06 warning: NULL used in arithmetic
	else if(0 == strncmp(MHmsgh.myNodeState(),"UNKNOWN",
                       sizeof(MHmsgh.myNodeState())))
	{
		strncpy(msghead.senderState, "UNK",
            sizeof(msghead.senderState));
	}
	else
	{
		strncpy(msghead.senderState, MHmsgh.myNodeState(),
            sizeof(msghead.senderState));
  }
  msghead.senderState[strlen(MHmsgh.myNodeState())+1]='\0';
}


const char*
CRspoolMsg::getOMkey() const
{
 	return msghead.omkey;
}

/* setting the x733 alarm stuff */
//void
//CRspoolMsg::clearX733()
//{
//	msghead.x733alarms.init( );
//}
//
//void
//CRspoolMsg::setAlarmObjectName(const Char* objectName)
//{
//#ifdef LX
//	char theObjectName[CRMAX_OBJNAME_SZ + 1];
//	if( strcmp("TRAPGW", getSenderName()) == NULL )
//	{
//		msghead.x733alarms.setAlarmObjectName( objectName );
//	}
//	else
//	{
//		//need to but NULL here?
//		snprintf(theObjectName,sizeof(theObjectName),"%s#%s",
//             msghead.senderMachine,
//             objectName);
//		const Char* theObjectNamePtr = theObjectName;
//
//		msghead.x733alarms.setAlarmObjectName( theObjectNamePtr );
//	}
//#else
//	msghead.x733alarms.setAlarmObjectName( objectName );
//#endif
//}
//
//void
//CRspoolMsg::setAlarmType(CRX733AlarmType alarmType)
//{
//	msghead.x733alarms.setAlarmType( alarmType );
//}
//
//void
//CRspoolMsg::setProbableCause(CRX733AlarmProbableCause probableCause)
//{
//	msghead.x733alarms.setProbableCause( probableCause );
//}
//
//void
//CRspoolMsg::setSpecificProblem(const Char* specificProblem)
//{
//	msghead.x733alarms.setSpecificProblem( specificProblem );
//}
//
//void
//CRspoolMsg::setAdditionalText(const Char* additionalText)
//{
//	msghead.x733alarms.setAdditionalText( additionalText );
//
//}
