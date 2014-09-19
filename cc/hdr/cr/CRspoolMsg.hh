#ifndef _CRSPOOLMSG_H
#define _CRSPOOLMSG_H

//
// DESCRIPTION:
//	The classes in this file are used to send a craft Output Message (OM)
//	to CSOP (Output Message Spooler Process).
//
// OWNER: 
//	Roger McKee
//
// NOTES:
//
//

//IBM JGH 20060619 add file containing MAXHOSTNAMELEN
#include <rpc/types.h>
#include <stdio.h>
#include <time.h>
#ifdef _SVR4
#	include <netdb.h>
#else
#	include <sys/param.h>
#endif
#include "cc/hdr/cr/CRmsg.H"
#include "cc/hdr/msgh/MHnames.H"
#include "cc/hdr/msgh/MHmsgBase.H"
#include "cc/hdr/msgh/MHinfoExt.H"

const int CROMDBKEYSZ =  8;

//
// this is for the OM header and footer size and should never be
// larger then 80 chars in size due to SCC requirements.
//
const int CRHDRFTRSZ =  80;
const int CRsegStrLen = 10;


struct CRsplMsgHead
{
	Bool fileFlag;	/* YES means text tool long for a message,
			 * so a file is used to send the text.
			 */
	Long timestamp;
	CROMclass type;	/* OM class */
	char senderProcNm[MHmaxNameLen+1];
	char usliName[MHmaxNameLen+1];
	U_short length; /* text length */
	char alarmLevel[3]; /* alarm level string (exactly two chars) */
	short lostMsgs; /* number of lost msgs */
	short loggedMsgs; /* number of logged msgs */
	char segStr[CRsegStrLen];	 /* string to hold the segment string */
	char senderMachine[26];
	char senderState[9];
	char omkey[8]; // omkey is 7 chars plus 1 for NULL
	CRx733 x733alarms;
};

const int CRsplMsgHdSz = sizeof(CRsplMsgHead);
const int CRsplMaxTextSz = MHmsgLimit - CRsplMsgHdSz - MHmsgBaseSz;
const int CRMAXSEGS = 10; /* maximum number of segments that will be held 
			   * temprarily before they are automatically 
			   * sent to CSOP process. This is not a limit on 
			   * the number of segments that can be generated 
			   * for large output messages
			  */
const int CRMAXLINELEN = 82;
const int CRMAXTITLELINES = 10;
const int CRMAXTITLESZ = CRMAXLINELEN * CRMAXTITLELINES;

/* This class will be used to temporarily hold the segments 
 * generated until they are sent to CSOP process 
*/
class CRspoolSeg
{
	public:
		CRspoolSeg(int seg);
		~CRspoolSeg();

		int segment;
		char segText[CRsplMaxTextSz];
};

class CRomDest;

class CRspoolMsg : public MHmsgBase
{
	friend class CRcsopMsg;
    public:
	CRspoolMsg(Bool buffering=YES);
	GLretVal send();
	void prmSpool();
	void intSpool();
	void writeln(int fd); /* call from receive end only */
	void sendToSop(CRomDest* daylogSop,
			CRomDest* sop2 =NULL,
			CRomDest* sop3 =NULL,
			CRomDest* sop4 =NULL,
			CRomDest* sop5 =NULL);
	void abort(); /* give up on current message before sending it */
#ifdef CC
	char* getFileName() { return fileName;}
#endif
	void title(const char *hdr);
	GLretVal addblanks(const char* instring);
	GLretVal addblanks(int len, const char* instring);
	void cleanup() const; /* remove message file, if any */
	const char* getAlarmLevel() const;
	CRALARMLVL alarmLevel() const;
	const char* getClass() const;
	const char* getSegStr() const;
	Bool getFileFlag() const;
	const char* getSenderName() const;
	const char* getUSLIname() const;
	short getLoggedOMcounter() const;
	void spool();
	void miscSpool(short destDev);
	void setLoggedOMcounter(short value);
	short getLostOMcounter() const;
	void setLostOMcounter(short value);
	void getOMhdr(char outbuf[],int seqnum);  /* formats the OM hdr line */
	GLretVal prepForSend(); /* only to be used by CRcsopMsg */
	void reset(); /* prepare for next message */
	void saveTime();        /* put the current time into the message */
	void sendToSop(CRomDest* soparray[], int numSOPs);
#ifdef LX
	void sendToSop(CRomDest* soparray[], int numSOPs, int seqnum, Bool sendToLeadCSOP);
#endif
	void sendToSop(CRomDest* soparray[], int numSOPs, int seqnum);
	void setAlarmLevel(CRALARMLVL);
	void setClass(CROMclass);
#ifndef __mips
	void setState();
#endif
	void setFileFlag();
	void setLength(U_short len);
	void setUSLIname(const char*);
	int textLength() const;
	void setSegInterval(const int sleepTime);
			/* The default values for sleepTime is 2 seconds.
			 * The valid range is between 2 seconds and 30 seconds 
			*/
	void avoidSegmentation();
	void genNewSeg(Bool oldTitle);
	void setOMkey(const char* OMkey);
	const char* getOMkey() const;


    public:
	static const char* dirname() { return directory; }
	static void audit();

// X733 Stuff
        void setAlarmObjectName( const Char* );
        void setAlarmType( CRX733AlarmType );
        void setProbableCause( CRX733AlarmProbableCause );
        void setSpecificProblem( const Char* );
        void setAdditionalText( const Char* );
        const char* getAdditionalText() const;
        void clearX733();

    private:
	void initForSend();
	void newSeg();
	void turnOffBuffering();
	void turnOnBuffering();
	GLretVal prmSend();
	GLretVal intSend();
	GLretVal miscSend();
	GLretVal add(char);
	GLretVal add(const char*, int);
	const char* getTime() const; /* formats a time string */
	void init();
	void logMsgLocally();
	GLretVal pageOut();

    private:
	void add(const char *);
	void sendSegs(const int lastIndex, Bool more);
	void allocSeg();
	void deleteSegs(const int lastIndex);
	void initMsgInfo();

    private:
	static const char* getHostName();
	static void clearLostCount();
	static void incrLostCount();
	static void clearLoggedCount();
	static void incrLoggedCount();
	
    private:
	static char hostname[MAXHOSTNAMELEN+1];
	static MHqid CSOPqid;
	static short lostMsgCount;
	static short loggedMsgCount;
	static const char* directory;			      
	static const short maxLineLen;

    private:
	CRsplMsgHead msghead;
	char msgText[CRsplMaxTextSz];

	/* following fields are NOT included in the MSGH message */

    private:

	int nextAvailableChar;
	char fileName[128];
	int currentPos;
	Bool errFlag;
	char omTitle[CRMAXTITLESZ];
	char lnbuf[CRMAXLINELEN];
	int segment;
	int currentSeg;
	int maxSegs;
	int segInterval;
	CRspoolSeg *seg[CRMAXSEGS];
	Bool segmentFlag;
	Bool firstLine;

	/* this is function pointer and will point to prmSend routine 
	* in case of CR_PRM macro and will point to send routine otherwise.
	*/
	GLretVal (CRspoolMsg::*sendfn)();
	Bool CRSentToCsop;

};

const int CRsplMsgSz = sizeof(CRspoolMsg);

extern const char* CRunknownProc;


inline int
CRspoolMsg::textLength() const
{
	return msghead.length;
}

inline const char*
CRspoolMsg::getClass() const
{
	return msghead.type.getCharStar();
}

inline const char*
CRspoolMsg::getSegStr() const
{
	return msghead.segStr;
}

inline void
CRspoolMsg::setClass(CROMclass newClass)
{
	msghead.type = newClass;
}

inline const char*
CRspoolMsg::getAlarmLevel() const
{
	return msghead.alarmLevel;
}

inline void
CRspoolMsg::setFileFlag()
{
	msghead.fileFlag = YES;
}

inline Bool
CRspoolMsg::getFileFlag() const
{
	return msghead.fileFlag;
}

inline void
CRspoolMsg::setLength(U_short len)
{
	msghead.length = len;
}

inline const char*
CRspoolMsg::getSenderName() const
{
	return msghead.senderProcNm;
}

inline const char*
CRspoolMsg::getUSLIname() const
{
	return msghead.usliName;
}

/* saves the current time in the object */
inline void
CRspoolMsg::saveTime()
{
	msghead.timestamp = time((long*) 0);
}

inline
short
CRspoolMsg::getLostOMcounter() const
{
	return msghead.lostMsgs;
}

inline
void
CRspoolMsg::setLostOMcounter(short value)
{
	msghead.lostMsgs = value;
}

inline
short
CRspoolMsg::getLoggedOMcounter() const
{
	return msghead.loggedMsgs;
}

inline
void
CRspoolMsg::setLoggedOMcounter(short value)
{
	msghead.loggedMsgs = value;
}

inline
void
CRspoolMsg::setOMkey(const char* theOMkey )
{
 	strncpy(msghead.omkey,theOMkey,sizeof(msghead.omkey) - 1);
 	msghead.omkey[(sizeof(msghead.omkey) - 1)]='\0';
}

inline
const char*
CRspoolMsg::getAdditionalText() const
{
	return msghead.x733alarms.getAdditionalText();
}

#endif
