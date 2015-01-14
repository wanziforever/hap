#ifndef __CROMDEST_H
#define __CROMDEST_H

/*
**      File ID:        @(#): <MID18885 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18885
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:15:43
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file declares an abstract class 'CRomDest' used by CSOP 
**      to send formatted Output Messages to SOPs.
**      Two concrete classes (CRlogSop and CRmsghSop) are also declared.
**      The concrete classes implement two different methods of sending 
**      OMs to a SOP: via a logfile and via MSGH messages.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
*/

#include <String.h>
#include "cc/hdr/cr/CRlogFile.hh"
#include "hdr/GLreturns.h"
#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/cr/CRalarmLevel.hh"

class CRomDest
{
public:
	CRomDest();
	virtual ~CRomDest();
	virtual GLretVal init(const char* msghname) =0;
	virtual void send(const char* text, int length, CRALARMLVL alm) =0;
	virtual void timeChange() =0;

public:
	static CRomDest* makeOriginatorDest();
	static CRomDest* getDayLogPtr();
	static void setDayLogPtr(CRomDest*);
	static CRomDest* getROPsopPtr();
	static void setROPsopPtr(CRomDest*);

private:
	static CRomDest* daylogPtr;
	static CRomDest* ropSopPtr;

};


class CRmsghSop : public CRomDest
{
    public:
	CRmsghSop();
	~CRmsghSop();
	GLretVal init(const char* msghname);
	void send(const char* text, int length, CRALARMLVL alm);
	void timeChange();
    private:
	char mhname[MHmaxNameLen+1];
};


class CRlogSop : public CRomDest
{
    public:
	CRlogSop(int logsz =4000, const char* dirname =CRDEFLOGDIR,
		 int numfiles =CRlogFile::DEFNUMFILES, 
		 const char* daily = NONDAILY,
		 int logAge = 0);
	~CRlogSop();
	GLretVal init(const char* msghname);
	void send(const char* text, int length, CRALARMLVL alm);
	void timeChange();

    protected:
	CRlogFile logfile;
	int loghsize;
	int numfiles;
	const char* directory;
	const char* dstatus;
	int theLogAge;
};

class CRlocalLogSop : public CRlogSop
{
    public:
	CRlocalLogSop(const char* dir = CRLCLLOGDIR );
	~CRlocalLogSop();
	const char* getText();
	void deleteFile();
	GLretVal init(const char* msghname);
	void send(const char* text, int length, CRALARMLVL alm);
	void timeChange();

    private:
	static int (*logfunc)(CRALARMLVL alm, const char *text);
  std::string tmpbuf;
};

/* This class is used to log initialization messages to PRMlog file */
class CRprmLogSop : public CRlogSop
{
    public:
	CRprmLogSop();
	~CRprmLogSop();
};

/* This class is used to log initialization messages to INTlog file */
class CRintLogSop : public CRlogSop
{
    public:
	CRintLogSop();
	~CRintLogSop();
};
#endif

