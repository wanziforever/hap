#ifndef __CRLOGFILE_H
#define __CRLOGFILE_H

/*
**      File ID:        @(#): <MID6922 () - 09/06/02, 29.1.1.1>
**
**	File:					MID6922
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:38:32
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This file declares a class used by CSOP to save information
**	in a logfile.  The main intended use is to log Output Messages.
**	The constructor has several arguments that are defaulted to
**	the values used by CSOP.  The process name is required.
**
**	Public Member Functions:
**
**		constructor - construct a logfile object for a process.
**			 There should be only one logfile object per
**			 process.
**			 The directory argument gives the name of the
**			 directory in which the logfiles will be stored.
**			 The 'logical_filename' argument is
**			 the logical name of the logfile.
**			 The actual names will end with a number.
**			 Note there will be more than one
**			 file and they will be used in a rotating manner.
**			 The 'max_sub_files' argument controls how many
**			 files are used in the rotation.
**			 By default, they are stored in the current
**			 directory.
**			 The 'max_file_size' argument gives an approximate
**			 upper limit on how big each file is allowed to
**			 grow to.
**		write -  writes the specified string to the logfile
**			 adding in a timestamp (current time) and
**			 extra newlines after the string.
**			 The string must be null-terminated and should
**			 not end with a newline.
**		writeln - like write, except the length of the string
**			 is specified, so the string does not have
**			 to be null-terminated.  The user must also
**			 supply the timestamp string.
**		isValid - indicates whether the CRlogFile constructor
**			 succeeded (YES) or failed (NO).
** OWNER: 
**	Roger McKee
**
** NOTES:
**	It is assumed that all processes using this class have called
**	CRERRINIT (in cc/hdr/cr/CRdebugMsg.H) to setup for CRERROR macros.
*/

#include <stdio.h>
#include <sys/types.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/cr/CRcirIndex.hh"
#include "cc/hdr/cr/CRstring.hh"

#define CRDEFLOGDIR "/sn/log"
#define CRDEFACTIVELOGDIR "/opt/sn/tmp/log"
#define CRDEFACTIVELOCLOGDIR "/opt/sn/tmp/log/locallogs"
#define CRLCLLOGDIR "/sn/log/locallogs"
#define CRPRMLOGDIR "/sn/log"

#define CRINTLOGDIR "/opt/sn/tmp/log"

enum { 
	LEAD_LOG_DIRECTORY = 0, 
	ACTIVE_LOG_DIRECTORY = 1
};

#define CRDEFLOGFILE "OMlog"
#define CRDEFROPLOG  "ROP"
#define CRDEFLOCALLOG "OMlocallog"
#define CRDEFPRMLOG "PRMlog"
#define CRDEFINTLOG "incidentlog"
#define NONDAILY 	"N"

/* Number of physical files for each logical local log 
** For the EES file space is less of a concern, so use to halves.
*/

const int CRnumLocalLogs = 1;
const int CRnumLogFiles = 2;
const int CRnumPrmLogs = 2;

const int CRlogFnameMax = 50;
CRstrdeclare(CRlogFnameMax)
typedef CRstr(CRlogFnameMax) CRlogFname;


/* the CRlogLoc class encapsulates the knowledge of how files are named.
** It also keeps track of the current location in the file and determines
** whether the location is at the end of the file or not.
*/

class CRlogLoc
{
public:
	CRlogLoc();
	~CRlogLoc();
	void init(const char* logical_fname);
	void init(const char* logical_fname, const char* dir,
            Long bytes_offset, int numFiles =CRnumLogFiles,
            const char* daily = NONDAILY ,
            int logAge = 0 );
	Bool isValid();
	int incr(int numbytes);
	void setSubIndex(int);
	int nextSubIndex(); /* move to next index */
	void getSubFileName(CRlogFname& result) const; /* current open sub filename */
	void getSubFileName(int index, CRlogFname& result) const;
	void getLogicalName(CRlogFname& result) const;
	void moveToEndOfNewestFile();
	Bool moveToNextPrintAble(FILE*&);
	Bool findNewestFile(int& newestIndex, time_t& modTime,
                      Long& fileLen) const;
	Bool isAtEof() const;
	void seek(FILE*) const;
	void dump() const;
	time_t curTime() const;
private:
	Bool moveToNextNewerFile();
private:
	CRlogFname logFname;
	CRcirIndex curOpenIndex;
	Long byteOffset;
	time_t lastChange;
};

class String;

class CRlogFile
{
public:
	enum { DEFNUMFILES = 2, DEFFILEMAX = 300000, DEFLCLLOGMAX = 50000,
         DEFPRMLOGMAX = 1000000, DEFINTLOGMAX = 1000000 };

	CRlogFile();
	virtual ~CRlogFile();
	void init(const char* logical_filename = CRDEFLOGFILE,
            const char* directory = CRDEFLOGDIR,
            int max_sub_files = DEFNUMFILES,
            int max_file_size = DEFFILEMAX,
            const char* daily = NONDAILY,
            int logAge = 0);
	void init(const CRlogLoc& startLoc);
	void cleanup();
	int writeOM(const char* omstr, int num_bytes);
	Bool isValid();
	void getLogicalName(CRlogFname& result_string) const;
	int getOM(CRlogLoc& resulting_loc, char outbuf[], int obufsize);
	void deleteLogFile();
  int readLog(std::string& inputBuf);
	void timeChange();

private:
	int switchFiles();
	int isFull();
	Bool newDay();
	Bool dirCheck(const char* directory) const;
	void prepForOutput();

private:
	CRlogLoc curLocation;
	enum { READ =1, WRITE =2 } openMode;
	FILE* openFp;
	int maxFiles;
	int maxFileSize;
	const char* switchDailyStatus;
	int logAge;
	time_t switchTime;
};
#endif
