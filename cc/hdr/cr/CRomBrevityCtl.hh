#ifndef __CROMBREVITYCTL_H
#define __CROMBREVITYCTL_H

/*
**
**      File ID:        @(#): <MID6929 () - 06/20/94, 1.1.1.40>
**
**
**      File:                                   MID6929
**      Release:                                1.1.1.40
**      Date:                                   04/03/00
**      Time:                                   10:19:15
**      Newest applied delta:   06/20/94
**
**
** DESCRIPTION:
**	This  header file was created for feature 6049 brevity control.
**	The header contains the CRomBrevityCtl class. This class
**	stores information on OMs being printed out. The class also
**	has methods to obtain/add/change the data.
**
** OWNER:
**      USLI subsystem
**
** NOTES:
*/

#include "hdr/GLreturns.h"
#include "cc/hdr/cr/CRalarmLevel.hh"

const int CRMAXLISTSIZE   = 200;
const int CROMKEYSZ       = 20;
const int CRMAXTIMEPERIOD = 5;
const int CRTIMEDELAY     = 300; // five minutes

enum CRomStatus
{
        CRCLEAR,               //OM is clear to be sent
	CRBLOCK,               //OM is block from being sent
	CRPENDING              //OM is pending 
};

enum CRomType
{
        CROMCRMSG,                 //OM is using the CRmsg class
	CROMCROMDB,                //OM is using the CRomdbMsg class
	CROMCRERROR,               //OM is a CRERROR
	CROMCRASSERT,              //OM is a CRASSERT
	CROMCRCFTASSERT            //OM is a CRCFTASSERT
};

struct CRomList {
	char       omKey[CROMKEYSZ + 1];	// OM key ie. OMDBKEY 
	int        omCnt;               // present count of number OMs
	int        totalomCnt; 		// total count of discarded OMs
	Long       nextTimePeriod;      // next time frame
	int        totalTimePeriods;    // number of time periods
	CRomStatus status;              // status of OM ie. BLOCK
	CRomType   omType;              // type of OM ie. CRERROR
};


class CRomBrevityCtl
{

public:
        // constructor, destructor are done in protected section

	static CRomBrevityCtl* Instance();

	CRomStatus status(const char* omKey, CRALARMLVL alarmLevel);
	CRomStatus updateStatusForNewPeriod(int i, Long thisTimePeriod);
	CRomStatus updateStatus(int i);
	Void printBlockOM(int i);
	Long getOMtime();
	Void dump();
	Void addNewOM(const char* omKey);
	Void resetOM(int i);
	Void printDelayOM(Long thisTimePeriod);
	Void removeOM(int i);
	static GLretVal attachGDO();

private:
	int _period;
	int _high;
	int _low;
	int _omArrayCnt;
	Long _printTimer;
	struct CRomList _OMlist[CRMAXLISTSIZE];
	Bool _brevityCtlFlag;
	static Char *_brevGDOaddr;


protected:

	// Hide for singleton implementation
	CRomBrevityCtl();
	~CRomBrevityCtl();
	static CRomBrevityCtl*  _instance;

};

#endif
