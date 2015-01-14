/*
**      File ID:        @(#): <MID18642 () - 09/09/02, 29.1.1.1>
**
**	File:					MID18642
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:23
**	Newest applied delta:	09/09/02
**
** DESCRIPTION:
**	This file defines the interface(s) to the function(s)
**	that are used to format the date and time for the
**	ULSI subsystem.
**
**	CRformatTime - returns a formatted date/time string
**			Its argument is number of seconds since 1970.
**			(The result of a time() system call).
**	CRgetCurTime - returns a formatted date/time string
**			representing the current date/time.
**	CRformatDate - returns a formatted date string.
**			Its argument is number of seconds since 1970 and
**			a date format.
**			(The result of a time() system call).
**	CRgetFormat - returns the current date format.
**	CRgetPosition - returns the position of the month, day,
**			or year field using the active date format. This
**			is for use on dates that have no characters 
**			separating fields; e.g., YYMMDD. Its argument is
**			the type of field.
**
** OWNER: 
**	Roger McKee
**
** MODIFIER:
**  Tim Richter
**
** NOTES:
**	The resulting date/time strings do not have a
**	trailing newline.
*/


#include	<time.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<locale.h>
#include	<fcntl.h>
#include	<memory.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	"hdr/GLtypes.h"
#include	"cc/hdr/cr/CRtmstamp.hh"
#include	"cc/hdr/cr/CRdebugMsg.hh"
#include	"cc/hdr/cr/CRassertIds.hh"

void
CRformatTime(long t, char timebuf[])
{
	std::string dateFmt = CRgetFormat();
	dateFmt += " %T";
	Bool retval = CRformatDate(t, (const char*) dateFmt.c_str(), timebuf);
	if (retval == NO)
	{
		struct tm *time_ptr = localtime(&t);

		sprintf(timebuf, "%04d-%02d-%02d %02d:02d:02d", 
			time_ptr->tm_year, 
			time_ptr->tm_mon + 1,			/* month (jan == 0) */
			time_ptr->tm_mday,				/* day of month */
			time_ptr->tm_hour,
			time_ptr->tm_min,
			time_ptr->tm_sec);
	}
}

void
CRgetCurTime(char timebuf[])
{
	CRformatTime(time((long*) 0), timebuf);
}


const char* defaultLctime = "psp_usa_std";

Bool
CRformatDate(long t, const char* fmt, char timebuf[])
{
	static Bool noLangAssert = NO;

	if (fmt == NULL || *fmt == '\0')
     fmt = (const char*) CRgetFormat().c_str();

	const char* lc_time = getenv("LC_TIME");
	if (lc_time == NULL || *lc_time == '\0')
	{
		lc_time = defaultLctime;

		std::string newLangEnvVar = "LC_TIME=";
		newLangEnvVar += lc_time;
		const char* coerced = (const char*) newLangEnvVar.c_str();
		putenv((char*) coerced);
#if 0
		if (noLangAssert == NO)
		{
			noLangAssert = YES;
			CRCFTASSERT(CRnoLangEnvVar, (CRnoLangEnvVarFmt, "LC_TIME"));
		}
#endif
	}

	char*	locale = setlocale(LC_TIME, (const char*) lc_time);
	struct tm *tms = localtime(&t);

#if LX
	// set LC_ALL for perl output
	const char* lc_all = getenv("LC_ALL");
	if (lc_all == NULL || *lc_all == '\0')
	{
		putenv((char*) "LC_ALL=C");
	}
#endif

	int	len = strftime(timebuf, timebufLen - 1, fmt, tms);

	if (len == 0)
	{
		timebuf[0] = '\0';
		return NO;
	}

	else
		return YES;
}


std::string
CRgetFormat()
{
	static Bool noLangAssert = NO;
	static std::string* retval = 0;

	if ( retval != 0 )
		return *retval;

	retval = new std::string("%Y-%m-%d");
	const char* lc_time = getenv("LC_TIME");
	if (lc_time == NULL || *lc_time == '\0')
	{
		lc_time = defaultLctime;
		std::string newLangEnvVar = "LC_TIME=";
		newLangEnvVar += lc_time;
		const char* coerced = (const char*) newLangEnvVar.c_str();
		putenv((char*) coerced);

#if 0
		if (noLangAssert == NO)
		{
			CRCFTASSERT(CRnoLangEnvVar, (CRnoLangEnvVarFmt, "LC_TIME"));
			noLangAssert = YES;
		}
#endif
	}


#ifdef __sun
	const char*	fmtDir = "/usr/lib/locale";
	std::string fmtFile = fmtDir;
	fmtFile += "/";
	fmtFile += lc_time;
	fmtFile += "/LC_TIME/time";
#elif LX
	const char*	fmtDir = "/usr/lib/locale";
	std::string fmtFile = fmtDir;
	fmtFile += "/";
	fmtFile += lc_time;
	fmtFile += "/time";
#else
	const char*	fmtDir = "/usr/lib/locale";
	std::string fmtFile = fmtDir;
	fmtFile += "/";
	fmtFile += lc_time;
	fmtFile += "/";
	fmtFile += "LC_TIME";
#endif

	int fd;
	if ((fd = open(fmtFile.c_str(), O_RDONLY)) == -1)
	{
#if 0
		CRCFTASSERT(CRnoDateFormatFile, (CRnoDateFormatFileFmt, 
								(const char*) fmtFile));
#endif

		return *retval;
	}

	struct stat fmtFileStat;
	if ((fstat(fd, &fmtFileStat)) != 0)
	{
#if 0
		CRCFTASSERT(CRnoDateFormatFile, (CRnoDateFormatFileFmt, 
								(const char*) fmtFile));
#endif

		close(fd);
		return *retval;
	}

	char* fmtFileBuf = new char[fmtFileStat.st_size + 2];

	if (read(fd, (void *) fmtFileBuf, 
		 (unsigned) fmtFileStat.st_size) != fmtFileStat.st_size)
	{
#if 0
		CRCFTASSERT(CRnoDateFormatFile, (CRnoDateFormatFileFmt, 
								(const char*) fmtFile));
#endif

		delete [] fmtFileBuf;
		close(fd);
		return *retval;
	}

	close(fd);

	/* Set 2nd to last character of fmtFileBuf to '\n' */
	/* Set last character of fmtFileBuf to '\0' */
	fmtFileBuf[fmtFileStat.st_size] = '\n';
	fmtFileBuf[fmtFileStat.st_size + 1] = '\0';

	char* bufPtr = fmtFileBuf;
	const int	linesToFmt = 39;
	for (int i = 0; i < linesToFmt && *bufPtr != '\0'; i++)
	{
		bufPtr = (char*) memchr(bufPtr, '\n', (int) fmtFileStat.st_size + 2);
		bufPtr++;
	}

	char* pptr = bufPtr;
	pptr = (char*) memchr(bufPtr, '\n', (int) fmtFileStat.st_size + 2);
	*pptr = '\0';

	delete retval;
	retval = new std::string(bufPtr);
	delete [] fmtFileBuf;
	return *retval;
}


int
CRgetPosition(fields f)
{
	int	monField;
	int dayField;
	int yearField;
	int field = 0;

	std::string fmtStr = CRgetFormat();
	if (fmtStr.empty())
	{
		fmtStr = CRgetFormat();
		if (fmtStr.empty() == YES)
			return 0;
	}


	int len = fmtStr.length();

	char c;
	for (int i = 0; i < len; i++)
	{
		c = fmtStr[i];
		if (c == '%')
		{
			c = fmtStr[++i];
			switch (c)
			{
				case '%':
					break;
				case 'm':
					monField = field;
					field += 2;
					break;
				case 'd':
				case 'e':
					dayField = field;
					field += 2;
					break;
				case 'y':
					yearField = field;
					field += 2;
					break;
				case 'Y':
					yearField = field;
					field += 4;
					break;
				case 'D':
					monField = 0;
					dayField = 2;
					yearField = 4;
				default:
					break;
			}
		}
	}

	switch (f)
	{
		case MONTH:
			return monField;
		case DAY:
			return dayField;
		case YEAR2:
		case YEAR4:
			return yearField;
		case NUMFIELDS:
		default:
			return -1;
	}
}
