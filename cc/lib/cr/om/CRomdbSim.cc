#/*
 **      File ID:        @(#): <MID18640 () - 08/17/02, 29.1.1.1>
 **
 **	File:					MID18640
 **	Release:				29.1.1.1
 **	Date:					08/21/02
 **	Time:					19:39:54
 **	Newest applied delta:	08/17/02
 **
 ** DESCRIPTION:
 **      This file defines a USLI class that is used to simulate
 **      part of the Output Message Data Base (OMDB).  In fact,
 **      it allows definition of an output message - its alarmlevel,
 **      msgclass, and printf-format string (like the OMDB).
 **      Using this class to define OMs will reduce the maintenance costs
 **      for OMs that are not defined in the OMDB.
 **
 ** OWNER: 
 **	Roger McKee
 **
 ** NOTES:
 **
 */
#include "cc/hdr/cr/CRomdbSim.H"
#include "cc/hdr/cr/CRdebugMsg.H"


CRomdbSim::CRomdbSim(CRALARMLVL alarmDefault, CROMclass msgclass,
                     const char* format0, const char* format1) :
alarmLevel(alarmDefault), msgClass(msgclass),
formatStr0(format0), formatStr1(format1)
{
}

const char*
CRomdbSim::getFormat(int nth) const
{
	switch (nth) {
  case 0:
		return formatStr0;
  case 1:
		return formatStr1;
  default:
		CRERROR("index (%d) out of range", nth);
		return "";
	}
}

CROMclass
CRomdbSim::getMsgClass() const
{
	return msgClass;
}

CRALARMLVL
CRomdbSim::getAlarmLevel() const
{
	return alarmLevel;
}
