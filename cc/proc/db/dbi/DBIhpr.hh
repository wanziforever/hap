#ifndef __DBIHPR_H
#define __DBIHPR_H

/*
**
**	File ID:	@(#): <MID7011 () - 08/17/02, 29.1.1.1>
**
**	File:					MID7011
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:21:43
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	  Used to define  DBI helper table
**
** OWNER:
**	eDB team
**
** History:
**	Yeou H. Hwang
**	Lucyliang: Change to C++ syntax for feature 61284
**
** NOTES:
*/

#include "hdr/GLtypes.h"
#include "DBIhprMsg.hh"

//max peg count of send select to DBIhpr error
#define DBI_MAXERRPEG	6

const  Short DBImaxHprNo = 52;

// helper state
const Short DBIhelperNotRdy = 0;
const Short DBIhelperStarting = 1;
const Short DBIhelperRdy = 2;
const Short DBIhelperWorking = 3;
const Short DBIhelperBusy = 4;

const Char DBIhelperStatus[][12] = {"NOT STARTED", "STARTING", "READY",
                                    "WORKING", "BUSY"};

typedef struct
{
	Char  name[MHmaxNameLen+1];	// name 
	Short errPeg;			// err peg count
	MHqid srcQue;			// MSGH queue id  
	Short state;			// state 
	time_t startTime;		// time begin to handle a sql.
} DBIhelperTblTyp;

class DBIhelpersStatus
{
public:
  DBIhelpersStatus(const char *helperName);

  Short getReadyHelper();

  inline const Char * getHelperName(Short helperIndex) const
	   { return helpers[helperIndex].name; }

  inline MHqid getHelperQid(Short helperIndex) const
	   { return helpers[helperIndex].srcQue;};

  inline Short getHelperStatus(Short helperIndex) const
	   { return helpers[helperIndex].state; }

  inline Void setHelperStatus(Short status, Short helperIndex)
	   { helpers[helperIndex].state = status; }

  inline Void setHelperStartTime(time_t tm, Short helperIndex)
	   { helpers[helperIndex].startTime = tm; }

  Void setHelperStatus(DBIhprMsg* sqlp);

  inline Short getHelperErrPeg(Short helperIndex) const
	   { return helpers[helperIndex].errPeg; }

  inline Void resetHelperErrPeg(Short helperIndex)
	   { helpers[helperIndex].errPeg = 0; }

  Void increaseHelperErrPeg(Short helperIndex, Short rtn);

  inline Short getTotalNumber() const
	   { return helperNo;}

  Void audit();
        
private:
  Char helperType[16];
  DBIhelperTblTyp helpers[DBImaxHprNo];
  Short helperNo;
  Short currentHelper;	// The last helper a SQL message was sent
};

#endif

