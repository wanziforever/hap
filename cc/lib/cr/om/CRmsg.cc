/*
**
**      File ID:        @(#): <MID18634 () - 09/06/02, 29.1.1.1>
**
**      File:                                   MID18634
**      Release:                                29.1.1.1
**      Date:                                   09/12/02
**      Time:                                   10:39:20
**      Newest applied delta:   09/06/02
**
**
** DESCRIPTION:
**	This contains functions that
**	build output messages and send them to CSOP.
**
** OWNER:
**      Roger McKee
**
**
**
** NOTES:
**
*/

#include <String.h>
#include <stdlib.h>
#include "cc/hdr/cr/CRmsg.hh"
#include "cc/hdr/cr/CRmsgInt.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRomInfo.hh"
#include "cc/hdr/cr/CRspoolMsg.hh"
//#include "cc/cr/hdr/CRshtrace.hh"

//#include "cc/hdr/misc/GLvsprintf.h"

#include "cc/hdr/cr/CRomHdrFtr.hh"

CRmsg::CRmsg(const Char* initStr, Bool buffering) : 
        OMclass(CL_DEBUG), alarmLevel(POA_DEFAULT)
{
	/* second parameter is to indicate whether to buffer upto 10 segments 
	 * before sending it to CSOP process or send the segment to CSOP 
	 * as soon as the segment is filled
	*/

	msg = new CRspoolMsg(buffering);
	msg->setClass(OMclass);
	msg->setAlarmLevel(alarmLevel);
	msg->addblanks(initStr);
	msg->setOMkey(CRDEFOMDBKEY);

	omBrevityCtl = CRomBrevityCtl::Instance();
	theStoredKey[0] = '\0';
}

CRmsg::CRmsg(CROMCLASS cl, CRALARMLVL al, Bool buffering) : 
	OMclass(cl), alarmLevel(al)
{
	/* third parameter is to indicate whether to buffer upto 10 segments 
	 * before sending it to CSOP process or send the segment to CSOP 
	 * as soon as the segment is filled
	*/

	msg = new CRspoolMsg(buffering);
	msg->setClass(OMclass);
	msg->setAlarmLevel(alarmLevel);
	msg->setOMkey(CRDEFOMDBKEY);

	omBrevityCtl = CRomBrevityCtl::Instance();
	theStoredKey[0] = '\0';
}

CRmsg::~CRmsg()
{
	delete msg;
}


/* This function will set the interval between segments before they 
 * are sent to CSOP process. The valid range is between 2 seconds and 
 * 30 seconds. If this function is not called, the default value of 
 * 2 seconds is used.
*/
void
CRmsg::setSegInterval(const int sleepTime)
{
	msg->setSegInterval(sleepTime);
}	

void
CRmsg::avoidSegmentation()
{
	msg->avoidSegmentation();
}

void
CRmsg::genNewSeg(Bool oldTitle)
{
	msg->genNewSeg(oldTitle);
}

void
CRmsg::title(const char *format,...)
{
	va_list ap;
	va_start(ap, format);
	char hdr[CRMAXTITLESZ];


	int num_bytes_printed = vsnprintf(hdr, (sizeof(hdr)-1), format, ap);
	hdr[sizeof(hdr)-1]='\0';

	if (num_bytes_printed < 0)
	{
		msg->abort();
		//CRSHERROR(CRbad_fmt, format);
	}
	else if (num_bytes_printed >= 1000)
	{
		msg->abort();
		//CRSHERROR(CRcorrupt_mem, num_bytes_printed+1);
	}
	else
		msg->title(hdr);
	va_end(ap);

}	


static char buf[CRVBUFSIZE];    /* buffer for call to vsprintf */

extern const char* CRcorrupt_mem = 
"Memory corrupted! Formatting buffer needs to be at least %d bytes";

extern const char* CRbad_fmt = "Invalid message format '%s'";

/* copies format string into output buffer
** while adding extra blanks after each \n
*/
//
//	This is only used by CRcsopMsg which is only
//	used fro the CRCSOP marcos. The reason for thsi
// 	comment is that the String class is not thread save.
//
void
CRaddblanks(const char* instring, std::string& outbuf)
{
	for (const char* inptr = instring; *inptr; inptr++)
	{
		outbuf += *inptr;
		if (*inptr == '\n' && *(inptr+1) != '\n')
			outbuf += "   ";
	}
}

CRmsg&
CRmsg::add(int len, const char *str)
{
	if (len < 0)
	{
		msg->abort();
		//CRSHERROR("invalid length (%d)", len);
	}
	else
	{
		msg->addblanks(len, str);
	}
	return *this;
}
			

CRmsg&
CRmsg::add(const Char *format,...)
{
	/* this is dangerous!  Assumes that resulting string will never
	*  be more than VBUFSIZE characters!
	*/
	va_list ap;
	va_start(ap, format);

	/* Note: there is a difference between vsprintf on the Tandem
	*  and the Sun's.  On Tandem vsprintf returns the number of
	*  of bytes "printed".  On Sun's it does not.
	*/

	//
	//      This is to make the buffer "buf" thread safe
	//
	static mutex_t CRbufferLock;
	mutex_lock(&CRbufferLock);

	int num_bytes_printed = vsnprintf(buf, sizeof(buf) -1, format, ap);
        buf[sizeof(buf)-1]='\0';

	if (num_bytes_printed < 0)
	{
		msg->abort();
		//CRSHERROR(CRbad_fmt, format);
	}
	else
	{
		msg->addblanks(buf);
	}

	mutex_unlock(&CRbufferLock);

	return *this;
}
			
void
CRmsg::add_va_list(const Char *format, va_list ap)
{
	/* Note: there is a difference between vsprintf on the Tandem
	*  and the Sun's.  On Tandem vsprintf returns the number of
	*  of bytes "printed".  On Sun's it does not.
	*/

	//
	//      This is to make the buffer "buf" thread safe
	//
	static mutex_t CRbufferLock;
 	mutex_lock(&CRbufferLock);

 	int num_bytes_printed = vsnprintf(buf, sizeof(buf) -1, format, ap);
        buf[sizeof(buf)-1]='\0';

	if (num_bytes_printed < 0)
	{
		msg->abort();
		//CRSHERROR(CRbad_fmt, format);
	}
	else
	{
		msg->addblanks(buf);
	}

	mutex_unlock(&CRbufferLock);
}

void 
CRmsg::setOMinfo(const CRomInfo* cepOMinfo)
{
	if (cepOMinfo != NULL)
	{
		if (alarmLevel == POA_DEFAULT)
			msg->setAlarmLevel(cepOMinfo->getAlarmLevel());

		msg->setUSLIname(cepOMinfo->getUSLIname());
	}
}

void
CRmsg::spool(const CRomInfo* cepOMinfo)
{
	setOMinfo(cepOMinfo);

	if(theStoredKey[0] != 0)
	{
		//
		// update get status of the OM
		//
		if((omBrevityCtl->status(theStoredKey, alarmLevel)) == CRCLEAR)
        	{
			msg->spool();
		}
		//else don't print message
		else
		{
			msg->cleanup(); /* removes tmp file, if it exists */
			msg->reset();
		}
	}
	else
	{
		msg->spool();
	}

	msg->setAlarmLevel(alarmLevel);
	msg->setClass(OMclass);
	msg->title("");
}

void
CRmsg::spool(const Char *format,...)
{
	va_list ap;
	va_start(ap, format);

	add_va_list(format, ap);
	va_end(ap);

	spool();
}

void
CRmsg::setAlarmLevel(CRALARMLVL a)
{
	alarmLevel = a;
	msg->setAlarmLevel(a);
}

void
CRmsg::setMsgClass(CROMclass c)
{
	OMclass = c;
	msg->setClass(c);
}

Void
CRmsg::storeKey(const char* OMkey,int omType)
{

	if (alarmLevel == POA_MAN) // don't block manual messages
	{
		theStoredKey[0]='\0';
	}
	else
	{
		snprintf(theStoredKey,CROMKEYSZ,"%d%s",omType,OMkey);
		if(omType == 0)
			msg->setOMkey(OMkey);
	}
}

Void
CRmsg::storeKey(int OMkey, int omType)
{
	if (alarmLevel == POA_MAN) // don't block manual messages
	{
		theStoredKey[0]='\0';
	}
	else
	{
		snprintf(theStoredKey,CROMKEYSZ,"%d%d",omType,OMkey);
		if(omType == 0)
		{
			char _theKey[CROMKEYSZ];
			snprintf(_theKey,CROMKEYSZ,"%d",OMkey);
			msg->setOMkey(_theKey);
		}
	}
}

void
CRmsg::setOMkey(const Char * theOMkey)
{
	// reset the message class and alarm level to defaults
	msg->setClass("");
	msg->setAlarmLevel(POA_DEFAULT);
        msg->setOMkey(theOMkey);
}

