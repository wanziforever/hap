/*
**      File ID:        @(#): <MID18616 () - 09/06/02, 29.1.1.1>
**
**	File:					MID18616
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:15
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This file defines the class used by CSOP to save information
**	in a logfile.  The main intended use is to log Output Messages.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <String.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include "cc/cr/hdr/CRlogFile.H"
#include "cc/cr/hdr/CRlogFileSwitchUpd.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRtmstamp.H"
#include "cc/cr/hdr/CRshtrace.H"
#include "cc/hdr/cr/CRsysError.H"
#include "cc/cr/hdr/CRdirCheck.H"
#include "cc/hdr/cr/CRassertIds.H"
#include <pwd.h>
#include <grp.h>



CRlogFile::CRlogFile()
{
	openMode = READ;
	openFp = NULL;
	maxFiles = DEFNUMFILES;
	maxFileSize = DEFFILEMAX;
}

void
CRlogFile::init(const CRlogLoc& startLoc)
{
	curLocation = startLoc;
	openMode = READ;
	openFp = NULL;
	maxFiles = DEFNUMFILES;
	maxFileSize = DEFFILEMAX;

	CRlogFname filename;
	curLocation.getSubFileName(filename);

	openFp = fopen(filename, "r");
	if (openFp)
	{
		/* seek to proper location */
		curLocation.seek(openFp);
	}
}

void
CRlogFile::init(const char* log_filename,
		const char* directory, int max_sub_files,
		int max_fsize, const char* daily, int thelogAge)
{
        openMode = WRITE;
        curLocation.init(log_filename, directory, 0, max_sub_files);
	maxFiles = max_sub_files;
	maxFileSize = max_fsize;
	switchDailyStatus = daily;
	logAge = thelogAge;
	openFp = NULL;
	if(logAge > 0 )
	{
		time_t CurrentTime;
		CurrentTime = time( (time_t *)0 );
		switchTime=CurrentTime+(logAge * 60 *60); // number of hours

struct tm       *cLocalTime;
cLocalTime = localtime( &switchTime );

	}
	else
	{
		switchTime=0;
	}

	if (dirCheck(directory) == YES)
	{
		prepForOutput();
	}
	else
	{
		CRSHERROR("'%s' is not a writable directory", directory);
	}
}

Bool
CRlogFile::isValid()
{
	if (openMode == WRITE && openFp == NULL)
	{
		prepForOutput(); /* should set openFp */
	}
	return (openFp == NULL) ? NO : YES;
}

/* check to make sure directory exists and is of the proper permissions
*   (read/write by owner)
*/
Bool
CRlogFile::dirCheck(const char* dir_name) const
{
	return CRdirCheck(dir_name, YES, "ainet");
}

void
CRlogFile::deleteLogFile()
{
	if (openFp != NULL)
	{
		fclose(openFp);
		openFp = NULL;
	}

	CRlogFname filename;

	for (int i = 0; i < maxFiles; i++)
	{
		curLocation.getSubFileName(i, filename);
		unlink(filename);
	}
}

void
CRlogFile::prepForOutput()
{
	time_t timestamp;
	Long file_size;
	int newest_indx;

	if (curLocation.findNewestFile(newest_indx, timestamp,
				       file_size) == YES)
	{
		curLocation.setSubIndex(newest_indx);
	}

	CRlogFname filename;
	curLocation.getSubFileName(filename);

	if (openFp != NULL)
		fclose(openFp);


	///////////////
	int retCode = -1;
	//mode=0664; - for all files under locallogs
	//mode=0666; - for PRMlogs
	mode_t mode=0640; //default mode for all log files

	if( (strncmp(filename,"/sn/log/PRMlog",14) == NULL) ||
		(strncmp(filename,"/opt/sn/tmp/log/PRMlog",22) == NULL) )
	{
		//PRMlogs
		mode=0666;
	}
#ifdef CC
	else if((strncmp(filename,CRDEFACTIVELOCLOGDIR,strlen(CRDEFACTIVELOCLOGDIR)) == NULL))
	{
		//files under locallogs i.e. /opt/sn/tmp/log/locallogs
		mode=0664;
	}
#endif
	else if((strncmp(filename,CRLCLLOGDIR,strlen(CRLCLLOGDIR)) == NULL) )
	{
		//files under locallogs i.e. /snlog/locallogs
		mode=0664;
	}
	else
	{
		//default mode for log files i.e. OMlog
		mode=0640;
#ifdef LX
		//for 5350 (R6.0) make OMlog* 644 for WebIM
		if( (strncmp(filename,"/sn/log/OMlog",13) == NULL) )
		{
			mode=0644;
		}

#endif
		if( ( strncmp(filename,"/sn/log/secure",14) == NULL) )
		{
			mode=0600;
		}
	}

	openFp = fopen(filename, "r");
	if (openFp == NULL)
	{
		if (( retCode = creat( (const char *)filename,mode )) == -1 )
		{
			// let the fopen fail and print error
		}
		else
		{
#ifdef CC
			struct passwd *pw = getpwnam("root");
			struct group    *ainetGroup = NULL;
			if( (ainetGroup = getgrnam( "ainet" )) != NULL )
			{
				if (chown(filename, pw->pw_uid, ainetGroup->gr_gid) == -1)
				{
					CRSHERROR("chown/grp of %s %d %d fail to %d",
						(const char*)filename, 
						pw->pw_uid, ainetGroup->gr_gid, errno);
				}
			}
#endif
			close(retCode);
		}
	}
	else
	{
		chmod( (const char *)filename,mode );
		fclose(openFp);
	}


	openFp = fopen(filename, "a");
	if (openFp == NULL)
	{
		CRSHERROR("could not open logfile '%s' due to error %s\n",
			  (const char*) filename,
			  CRsysErrText(errno));
	}
}


CRlogFile::~CRlogFile()
{
	cleanup();
}

void
CRlogFile::cleanup()
{
	if (openFp != NULL)
	{
		fclose(openFp);
		openFp = NULL;
	}
}

const char CRswitchLogSizeMsg[] = "\n*** end of log file half ***\n";
int CRswLogSizeMsgSz = sizeof(CRswitchLogSizeMsg) - 1;

int
CRlogFile::switchFiles()
{

        CRlogFileSwitchEvent event;
        CRlogFname oldFileName;
        curLocation.getSubFileName(oldFileName);
        event.setOldLogFileName(oldFileName);

	curLocation.nextSubIndex();

	if (openFp != NULL)
	{
		fclose(openFp);
		openFp = NULL;
	}

	/* assumes this function is only called when in WRITE mode */

	CRlogFname filename;
	curLocation.getSubFileName(filename);

	/////////
	int retCode = -1;
	mode_t mode = 0664;
	openFp = fopen(filename, "r");
	if (openFp == NULL)
	{
		if( (strncmp(filename,"/sn/log/PRMlog",14) == NULL)  ||
#ifdef CC
		  (strncmp(filename,CRDEFACTIVELOCLOGDIR,strlen(CRDEFACTIVELOGDIR)) == NULL) ||
#endif
		  (strncmp(filename,CRLCLLOGDIR,strlen(CRLCLLOGDIR)) == NULL) )
		{
		   if((strstr(filename, "PRMlog"))!=NULL) //PRMlog
		   {
			mode=0666;
		   }

		   if (( retCode = creat( (const char *)filename,mode )) == -1 )
		   {
			// let the fopen fail and print error
		   }
		   else
		   {
#ifdef CC
			struct passwd *pw = getpwnam("root");
			struct group    *ainetGroup = NULL;
		   	chmod( (const char *)filename,mode );
			if( (ainetGroup = getgrnam( "ainet" )) != NULL )
			{
				if (chown(filename, pw->pw_uid, ainetGroup->gr_gid) == -1)
				{
				   CRSHERROR("chown/grp of %s %d %d fail to %d",
					(const char*)filename, 
					pw->pw_uid, ainetGroup->gr_gid, errno);
				}
			}
#endif

			close(retCode);
		   }
		}
		else
		{
		  if (( retCode = creat( (const char *)filename,0640 )) == -1 )
		  {
			// let the fopen fail and print error
		  }
		  else
		  {
#ifdef CC
			struct passwd *pw = getpwnam("root");
			struct group    *ainetGroup = NULL;
			if( (ainetGroup = getgrnam( "ainet" )) != NULL )
			{
				if (chown(filename, pw->pw_uid, ainetGroup->gr_gid) == -1)
				{
				   CRSHERROR("chown/grp of %s %d %d fail to %d",
					(const char*)filename, 
					pw->pw_uid, ainetGroup->gr_gid, errno);
				}
			}
#endif

			close(retCode);

		  }
	        }
		//////
	}
	else
	{
		fclose(openFp);
	}


	/////////

	openFp = fopen(filename, "w");
	if (openFp == NULL)
	{
		CRSHERROR("could not open logfile '%s' due to error %s\n",
			  (const char*) filename, 
			  CRsysErrText(errno));
		return 0;
	}
        event.setNewLogFileName(filename);

        //Send Notification messages to only registerd queues.
        for(int i = 0; i < interestedQueues.size(); i++) 
            event.send(interestedQueues[i].c_str(), MHnullQ,
                       CRlogFileSwitchEventSz, -1);
	return 1;
}

#define CROM_END_STR "\n\001\n" 
const char* end_str = CROM_END_STR; 
const int end_str_len = strlen(CROM_END_STR);

int
CRlogFile::writeOM(const char* str, int str_len)
{
	/* check to make sure there is room in the file */
	if (!isValid())
		return -1;

	if (isFull())
		if (!switchFiles())
			return -1;

	/* drop any trailing whitespace */
	int i = str_len - 1;
	while (isspace(str[i]))
	{
		i--;
		str_len--;
	}

	int retval = 0;

	if (write(fileno(openFp), str, str_len) == str_len)
	{
		retval += str_len;
		if (write(fileno(openFp), end_str, end_str_len) != end_str_len)
			retval = -1;
		else
			retval += end_str_len;
	}

	if (retval < 0)
	{
		CRlogFname filename;
		curLocation.getSubFileName(filename);
		CRSHERROR("write() to file '%s' failed due to error %s\n",
			  (const char*) filename,
			  CRsysErrText(errno));
	}
	else
	{
		if (isFull())
		{
			/* write msg to indicate end of log file half */
			write(fileno(openFp), CRswitchLogSizeMsg,
			      CRswLogSizeMsgSz);
		}
	}

	fclose(openFp); openFp = NULL;

	return retval;
}

/* determine if the current subfile is full.
*  Returns true (non-zero) if the file is full.
*/
int
CRlogFile::isFull()
{

	if (!isValid())
		return 0;

	CRlogFname filename;
	curLocation.getSubFileName(filename);
	struct stat stbuf;

	if (fstat(fileno(openFp), &stbuf) == -1)
	{
		CRSHERROR("stat of %s failed due to error %s\n",
			  (const char*) filename,
			  CRsysErrText(errno));
		return 0;
	}

	// check hourly flag and switch if time is up
	if(switchTime > 0)
	{
		time_t currentTime;
		currentTime = time( (time_t *)0 );

	// IF current time GREATER switch time change switch time
	// and return 1 (switch log) else return size
struct tm       *cLocalTime;
cLocalTime = localtime( &switchTime );
		if( currentTime > switchTime )
		{
		switchTime=currentTime+(logAge * 60 *60); // number of hours

			return 1; // switch file
		}
		else
		{
			return stbuf.st_size > maxFileSize;
		}
	}

	// check for daily flag and then check file date
	if( (strcmp(switchDailyStatus,"Y")) != NULL)
	{
	//  Don't care about checking if its a new day thus return
	//  the size of the file.
	//
		return stbuf.st_size > maxFileSize;
	}
	else
	{
	// daily flag ON check on status if new day then return 1 
	// else return size.
		if((newDay()) == YES)
		{
			return 1; // switch file
		}
		else
		{
			return stbuf.st_size > maxFileSize;
		}
	}

}


/*
** Read an output message from the logfile and store it in the buffer
** provided.  
**
** Returns the number of lines read.  The last line read may be partial if
** the entire line will not fit in the remainder of the provided
** output buffer.
**
** Reads until the end of the OM (first line with just one character - 
** assumed to be a lone newline).  Will terminate if at end of file or
** the output buffer is full.
*/
int
CRlogFile::getOM(CRlogLoc& externalLoc, char outbuf[], int outbufsize)
{
	if (externalLoc.isAtEof() == YES)
	{
		if (externalLoc.moveToNextPrintAble(openFp) == NO)
			return 0;
	}

	if (openFp == NULL)
	{
		CRlogFname fname;
		externalLoc.getSubFileName(fname);
		openFp = fopen(fname, "r");
		if (openFp == NULL)
		{
			CRSHERROR("could not open logfile '%s due to error %s",
				  (const char*) fname,
				  CRsysErrText(errno));
			return 0;
		}
		externalLoc.seek(openFp);
	}

	char* bufptr = outbuf;
	int roomleft = outbufsize;
	int numLinesRead = 0;

	while (roomleft > 1 && fgets(bufptr, roomleft, openFp))
	{
		numLinesRead++;
		int nchars = strlen(bufptr);

		bufptr += nchars;
		roomleft -= nchars;
		externalLoc.incr(nchars);

		if (nchars == 1)
			break;
	}

	return numLinesRead;
}

void
CRlogFile::getLogicalName(CRlogFname& result_str) const
{
	curLocation.getLogicalName(result_str);
}

/* reads the entire contents of the logfile into the String provided
** Returns the number of characters read.
*/
int
CRlogFile::readLog(String& inbuf)
{
	CRlogFname filename;

	int totalBytes = 0;

	for (int i = 0; i < maxFiles; i++)
	{
		curLocation.getSubFileName(i, filename);
		FILE *fp = fopen((const char*) filename, "r");
		if (fp)
		{
			char buf[513];
			int n = 0;
			while (n = fread(buf, 1, sizeof(buf)-1, fp))
			{
				totalBytes += n;
				buf[n] = '\0';
				inbuf += buf;
			}
			fclose(fp);
		}
	}
	return totalBytes;
}

void
CRlogFile::timeChange()
{
	time_t curtime = time(NULL);
	struct utimbuf tvals;

	/* Sets the newest file to the current time and
	** sets the each successive file to the current time -1
	** additional minute.
	*/

	time_t newtime = curtime;
	for (int i = 0; i < maxFiles; i++, newtime -= 60)
	{
		time_t timestamp;
		Long file_size;
		int newest_indx;

		if (curLocation.findNewestFile(newest_indx, timestamp,
					       file_size) == NO)
		{
			return;
		}

		if (timestamp <= curtime)
			return;

		CRlogFname filename;
		curLocation.getSubFileName(newest_indx, filename);
		
		tvals.actime = newtime;
		tvals.modtime = newtime;
		int rtn = utime(filename, &tvals); /* sets to new time */

		if (rtn == -1)
		{
			CRCFTASSERT(CRlogTmChgFailId,
				    (CRlogTmChgFailFmt,
				     (const char*) filename,
				     CRsysErrText(errno)));
			return;
		}
	}
}

Bool
CRlogFile::newDay()
{
	CRlogFname filename;
	curLocation.getSubFileName(filename);

	// Get today
	char curDay[10];
	time_t timeValue = time((time_t *)0);
	strftime(&curDay[0], 3, "%d", localtime(&timeValue));

	// Get day file was last touch
	char fileDay[10];
	struct stat st;

	if ((lstat(filename, &st)) == -1)
	{
		CRSHERROR("lstat of %s failed\n", (const char*) filename);
		return NO;
	}
	time_t tmpTimeVar = st.st_mtime;
	strftime(&fileDay[0], 3, "%d", localtime(&tmpTimeVar));

	// If same do nothing else return YES
	if((strcmp(fileDay,curDay))==NULL)
	{
		return NO;
	}
	else
	{
		return YES;
	}
}
