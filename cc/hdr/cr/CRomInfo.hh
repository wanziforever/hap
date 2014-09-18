#ifndef __CROMINFO_H
#define __CROMINFO_H

// DESCRIPTION:
//      This file contains the declaration of the CRomInfo class.
//      This class is used to interface with USLI when sending
//      Output messages to CSOP.
//
// OWNER: 
//	Roger McKee
//
// NOTES:
//      This object is hand-compacted for use within a message.
//      The size of an object of this the class should be divisible by 4.
//

#include "cc/hdr/cr/CRalarmLevel.hh"
#include "cc/hdr/msgh/MHnames.hh"

class CRomInfo 
{
    public:
	CRomInfo();
	CRomInfo(CRALARMLVL a, short msghqid);
	CRomInfo(CRALARMLVL a, const char* usliName);
	CRALARMLVL getAlarmLevel() const;
	const char* getUSLIname() const;
    private:
	void init(CRALARMLVL a =POA_DEFAULT, const char* usliName ="",
		  short msghqid =-1);
    private:
	short usliMsghqid;
	CRALARMLVL alarmLevel;
	char usliProcName[MHmaxNameLen+1];
};
#endif
