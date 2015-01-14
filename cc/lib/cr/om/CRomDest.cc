/*
**      File ID:        @(#): <MID18887 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18887
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:55
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file defines the classes used to send a formatted
**      Output Message (OM) from CSOP to a SOP.
**      The formatted OMs are sent via MSGH or a log file; depending
**      on the concrete class used (CRmsghSop, CRlogSop, or CRlocalLogSop).
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
//#include "cc/hdr/cr/CRsopMsg.hh"
#include "cc/hdr/cr/CRomDest.hh"
//#include "cc/cr/hdr/CRshtrace.hh"

#ifdef __linux
#include <dlfcn.h>
#endif

CRomDest* CRomDest::daylogPtr = NULL;
CRomDest* CRomDest::ropSopPtr = NULL;
int (*CRlocalLogSop::logfunc)(CRALARMLVL alm, const char *text) = NULL;

const int CRSOPLOGSIZE = 2000;

CRomDest*
CRomDest::makeOriginatorDest()
{
	return new CRmsghSop;
}

CRomDest*
CRomDest::getDayLogPtr()
{
	return daylogPtr;
}

void
CRomDest::setDayLogPtr(CRomDest* newValue)
{
	daylogPtr = newValue;
}

CRomDest*
CRomDest::getROPsopPtr()
{
	return ropSopPtr;
}

void
CRomDest::setROPsopPtr(CRomDest* newValue)
{
	ropSopPtr = newValue;
}


CRomDest::CRomDest()
{
}

CRomDest::~CRomDest()
{
}


CRmsghSop::CRmsghSop()
{
}

CRmsghSop::~CRmsghSop()
{
}

GLretVal
CRmsghSop::init(const char* msghname)
{
	if(strcmp(msghname,"")==0)
	{
		//CRSHERROR("NULL msghname");
		return GLfail;
	}

	if (strlen(msghname) > MHmaxNameLen)
	{
		//CRSHERROR("msgh name '%s' is too long", msghname);
		return GLfail;
	}

	strcpy(mhname, msghname);
	return GLsuccess;
}

void
CRmsghSop::send(const char* text, int length, CRALARMLVL alm)
{
	//CRsopMsg sopMsg;
  //
	//sopMsg.setProbableCause( getProbableCause() );
	//sopMsg.setAlarmType( getAlarmType() );
	//sopMsg.setAlarmObjectName( (char*) getAlarmObjectName() );
	//sopMsg.setSpecificProblem( (char*) getSpecificProblem() );
	//sopMsg.setAdditionalText( (char*) getAdditionalText() );
  //
	//sopMsg.alarmLevel(alm);
	//sopMsg.send(mhname, text, length);
}

void
CRmsghSop::timeChange()
{
}


CRlogSop::CRlogSop(int logsz, const char* dirname, int nfiles , 
                   const char* daily, int logAge )
{
	loghsize = logsz;
	numfiles = nfiles;
	directory = dirname;
	dstatus = daily;
	theLogAge = logAge;

}

CRlogSop::~CRlogSop()
{
	logfile.cleanup();
}

GLretVal
CRlogSop::init(const char* msghname)
{
	logfile.init(msghname, directory, numfiles, loghsize, dstatus,
			theLogAge);
	return logfile.isValid() ? GLsuccess : GLfail;
}

void
CRlogSop::send(const char* text, int length, CRALARMLVL /*alm*/)
{
	logfile.writeOM(text, length);
}

void
CRlogSop::timeChange()
{
		logfile.timeChange();
}

CRlocalLogSop::CRlocalLogSop(const char* dir) : CRlogSop(CRlogFile::DEFLCLLOGMAX,
					  dir, CRnumLocalLogs)
{
}

CRlocalLogSop::~CRlocalLogSop()
{
}

GLretVal
CRlocalLogSop::init(const char* msghname)
{
#ifdef __linux

	static int looked = 0;
	void *dlhandle;

	if(looked == 0)
	{
		looked = 1;
#ifdef _LP64
		dlhandle = dlopen("/usr/lib64/liblogmessage.so", RTLD_NOW);
#else
		dlhandle = dlopen("/usr/lib/liblogmessage.so", RTLD_NOW);
#endif

		if(dlhandle != NULL)
		{
			logfunc = (int (*)(unsigned char, const char*)) 
					dlsym(dlhandle, "outputMessageLog");
		}
		if(logfunc != NULL)
			return GLsuccess;
	}
#endif

	if(logfunc == NULL)
		return CRlogSop::init(msghname);
	else
		return GLsuccess;
}

void
CRlocalLogSop::send(const char* text, int length, CRALARMLVL alm)
{
	if(logfunc == NULL)
		CRlogSop::send(text, length, alm);
	else
		logfunc(alm, text);
}

void
CRlocalLogSop::timeChange()
{
	if(logfunc == NULL)
		CRlogSop::timeChange();
}
	  
const char*
CRlocalLogSop::getText()
{
	if(logfunc == NULL)
		logfile.readLog(tmpbuf);
	return tmpbuf.c_str();
}

void
CRlocalLogSop::deleteFile()
{
	if(logfunc == NULL)
		logfile.deleteLogFile();
}

CRprmLogSop::CRprmLogSop() : CRlogSop(CRlogFile::DEFPRMLOGMAX,
					  CRPRMLOGDIR, CRnumPrmLogs)
{
}

CRprmLogSop::~CRprmLogSop()
{
}

CRintLogSop::CRintLogSop() : CRlogSop(CRlogFile::DEFPRMLOGMAX,
					  CRINTLOGDIR, CRnumPrmLogs)
{
}

CRintLogSop::~CRintLogSop()
{
}

/* setting the x733 alarm stuff */
//void
//CRomDest::setAlarmObjectName(const Char* objectName)
//{
//        strcpy(_objectName,objectName);
//}
// 
//void
//CRomDest::setAlarmType(CRX733AlarmType alarmType)
//{
//        _alarmType = alarmType;
//}
// 
//void
//CRomDest::setProbableCause(CRX733AlarmProbableCause pCause)
//{
//        _pCause = pCause;
//}
// 
//void
//CRomDest::setSpecificProblem(const char* specProb)
//{
//        strcpy(_specProb,specProb);
//}
// 
//void
//CRomDest::setAdditionalText(const char* addText)
//{
//        strcpy(_addText,addText);
//}
