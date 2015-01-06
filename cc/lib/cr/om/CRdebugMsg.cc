/*
**
**      File ID:        @(#): <MID18632 () - 09/06/02, 29.1.1.1>
**
**      File:                                   MID18632
**      Release:                                29.1.1.1
**      Date:                                   09/13/02
**      Time:                                   12:00:10
**      Newest applied delta:   09/06/02
**
** DESCRIPTION:
**	This file contains some of the global objects needed to support
**	the CRERROR and CRDEBUG macros.
**
** OWNER:
**      Roger McKee
**
** NOTES:
**
*/

#include <stdlib.h>
#include <ctype.h>
#include <thread.h>

#include <libgen.h>

#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRmsg.hh"
#include "cc/hdr/cr/CRprmMsg.hh"
#include "cc/hdr/msgh/MHnames.hh"

//#include "cc/hdr/misc/GLvsprintf.h"

static CRmsg* CRassertOM = NULL;
static CRmsg* CRcraftErrorOM = NULL;
static CRmsg* CRcraftAssertOM = NULL;
static CRmsg* CRerrorOM = NULL;
static CRmsg* CRdebugOM = NULL;
static CRmsg* CRprmOM = NULL;
static CRmsg* CRmiscPrmOM = NULL;

/*
**	The PRM marco are used by INIT so don't do a static
**	to save memory since INIT the only one that uses
**	it. The memory is save at the beginning with
**	doing the new.
*/

static void (*CRassertFn)(short errFlag) = 0;

const char *CRasrtFmt = "REPT DFC=%d %s\n%s AT LINE %d\n";
const char *CRcftErrorFmt = "REPT MANUAL ERROR=%d\nPROC=%s, %s AT LINE %d\n";
const char *CRcftAsrtFmt = "REPT MANUAL ASSERT=%d\nPROC=%s, %s AT LINE %d\n";
const char *CRerrFmt = "REPT ERROR LOG %s, FILE %s, LINE %d\n";
const char *CRdbgFmt = "REPT DEBUG %s TR (0x%x), FILE %s, LINE %d\n";
const char *CRdbg2Fmt = "REPT DEBUG %s TR (none), FILE %s, LINE %d\n";

/* Process name, set by CRERRINIT */
char CRprocname[MHmaxNameLen+1] = "unknown";
mutex_t CRlockVarible = DEFAULTMUTEX;

CRmsg*
CRprmPtr()
{
	/* singleton pattern: at most only one instance of CRprmOM should be
	** should exist in the process.  Note: This instance is never deleted
	** This is ok since CRmsg is about 16 bytes only which is very minor.
	** and hopefully the garbage collector would clean this up upon process
	** death.  If this message size increases significantly, we need to 
	** create a destructor and not rely on the garbage collector or the
	** operating system.
	*/
	if (CRprmOM == NULL)
		CRprmOM = new CRmsg(CL_MAINT, POA_INF);

	return CRprmOM;
}

CRmsg*
CRmiscPrmPtr()
{
	/* CRmiscPrmOM is an instance of a singleton pattern.
	** see comments above about the singleton pattern
	*/
	if (CRmiscPrmOM == NULL)
		CRmiscPrmOM = new CRmsg(CL_MAINT, POA_INF);

	return CRmiscPrmOM;
}

/* this function is used by the main() routine in the INIT library 
 * to set the global function pointer CRassertFn to a function which keeps
 * track of the number of meaningful CRERROR and CRasserts generated 
 * on the platform
*/
void 
CRsetAssertFn(void (*fnPtr)(short))
{
	CRassertFn = fnPtr;
}
	

CRmsg*
CRerrorPtr()
{

	if (CRerrorOM == NULL)
                CRerrorOM = new CRmsg(CL_DEBUG, POA_INF);

	/* This code exists to peg the number of OMs generated as a 
	 * result of CRERROR . The CRassertfn is a function pointer
	 * and is initialized by the main() routine in the INIT library
	*/
	if (CRassertFn != 0)
		(*CRassertFn)(CRerrorFlag);

	return CRerrorOM;
}

CRmsg*
CRdebugPtr()
{

	if (CRdebugOM == NULL)
                CRdebugOM = new CRmsg(CL_DEBUG, POA_INF);

	return CRdebugOM;
}

CRmsg*
CRassertPtr()
{

	if (CRassertOM == NULL)
                CRassertOM = new CRmsg(CL_ASSERT, POA_ACT);

	/* This code exists to peg the number of OMs generated as a 
	 * result of CRASSERT . The CRassertfn is a function pointer
	 * and is initialized by the main() routine in the INIT library
	*/
	if (CRassertFn != 0)
		(*CRassertFn)(CRassertFlag);

	return CRassertOM;
}

CRmsg*
CRcftErrorPtr()
{
	if (CRcraftErrorOM == NULL)
	                CRcraftErrorOM = new CRmsg(CL_ASSERT, POA_ACT);
	return CRcraftErrorOM;
}

CRmsg*
CRcftAssertPtr()
{
	if (CRcraftAssertOM == NULL)
                CRcraftAssertOM = new CRmsg(CL_ASSERT, POA_ACT);

	return CRcraftAssertOM;
}

CRtraceTbl CRtraceFlags;

void
CRerrInit(const char* mhname)
{
	//
	// In EE subshl is call from a long PATH.
	// there was a problem were mhname was bigger then
	// CRprocname so address got hosed.
	// Had to cast mhname is a const and
	// I wanted to run basename on it.
	//


	strncpy(CRprocname, basename((char*) mhname), sizeof(CRprocname) - 1);
        CRprocname[sizeof(CRprocname) - 1] = '\0';
	CRtraceFlags.init();
}

#ifdef CC
const char CRtraceDir[] = "/sn/cr/tracedir/";
const char CRdataFile[] = "/sn/cr/tracedir/traceflags";
#else
const char CRtraceDir[] = "./";
const char CRdataFile[] = "./traceflags";
#endif



CRtraceMap::CRtraceMap()
{
	setAllBits(0);
}

void
CRtraceMap::dump() const
{
	fprintf(stderr, "dumping CRtraceMap (the following bits are set):\n");

	int firstone = -2;
	int lastone = -2;
	
	for (int byte = 0; byte < CRnumTraceBytes; byte++)
	{
		if (bitmap[byte] == 0)
			continue;

		int targetIndex = byte * 8;

		for (int bit = 0; bit < 8; bit++)
		{
			int flag = (bitmap[byte] >> bit) & 1;

			if (flag)
			{
				int idx = targetIndex + bit;
				if (idx-1 == lastone)
				{
					lastone = idx;
					continue;
				}

				if (lastone > firstone)
				{
					fprintf(stderr, "-%d", lastone);
					firstone = -2;
				}

				fprintf(stderr, " %d", idx);
				firstone = idx;
				lastone = idx;
			}
		}
	}
	if (lastone > firstone)
	{
		fprintf(stderr, "-%d", lastone);
	}
}

void
CRtraceMap::setBit(Short bitnum)
{
	int byteNum = bitnum >> 3;
	int bitOffset = bitnum & 0x7;
	unsigned char bitmask = 1 << bitOffset;

	bitmap[byteNum] |= bitmask;
}

void
CRtraceMap::clearBit(Short bitnum)
{
	int byteNum = bitnum >> 3;
	int bitOffset = bitnum & 0x7;
	unsigned char bitmask = 1 << bitOffset;

	bitmap[byteNum] &= ~bitmask;
}

Bool
CRtraceMap::setMap(const char* valueName)
{
	CRDEBUG(CRusli+CRinoutFlg, ("enter setMap(%s)", valueName));

	char lowerName[50];
	int i = 0;
	
	for (const char* tmpptr = valueName; *tmpptr; tmpptr++, i++)
	{
		if (isupper(*tmpptr))
			lowerName[i] = tolower(*tmpptr);
		else
			lowerName[i] = *tmpptr;
	}
	
	lowerName[i] = '\0';

	if (strcmp(lowerName, "on") == 0)
		setAllBits(1);
	else if (strcmp(lowerName, "off") == 0)
		setAllBits(0);
	else 
	{
		/* search file looking for the entry that matches
		** the specified name
		*/
		FILE* fp = fopen(CRdataFile, "r");
		if (fp == NULL)
		{
			CRERROR("Could not read trace flag file '%s'",
				CRdataFile);
			return NO;
		}

		return findEntry(fp, valueName);
	}
	return YES;
}

Bool
CRtraceMap::findEntry(FILE* fp, const char* flagName)
{
	const int inbufsize = 20000;
	char inbuf[inbufsize+1];
	Bool retval = NO;

	while (fgets(inbuf, inbufsize, fp))
	{
		if (inbuf[0] == '#')
			continue;

		char curFlagName[100];
		
		if (sscanf(inbuf, "%s", curFlagName) != 1)
			continue;

		if (strlen(curFlagName) >= 100)
		{
			CRERROR("trace flag file has invalid line:\n%s",
				inbuf);
			break;
		}

		char* colonPtr = strchr(curFlagName, ':');
		if (colonPtr)
		{
			*colonPtr = '\0';
		
			if (strcmp(curFlagName, flagName) == 0)
			{
				retval = processMatch(inbuf);
				break; /* get out of while loop */
			}
		}
	}
	
	fclose(fp);
	return retval;
}

Bool
CRtraceMap::processMatch(char* inbuf)
{
	const char nameDelimChar = ':';
	
	char* curptr = strchr(inbuf, nameDelimChar);
	
	if (curptr == NULL)
	{
		CRERROR("Missing delimiter '%s' after trace flag name on line:\n%s",
			nameDelimChar, inbuf);
		return NO;
	}

#ifdef OLDWAY
	/* clear the bit map */
	setAllBits(0);
#endif
	
	procDecBits(curptr+1);

	return YES;
}

void
CRtraceMap::procDecBits(char* curptr)
{
	/* loop thru each value, setting the corresponding bit */
	const char* bitDelimChars = " ,\t\n";
	
	for (char* tokptr = strtok(curptr, bitDelimChars);
	     tokptr;
	     tokptr = strtok(NULL, bitDelimChars))
	{
		setBit(atoi(tokptr));
	}
}

void
CRtraceMap::setAllBits(Bool binaryValue)
{
	int byteValue = 0;
	
	if (binaryValue == YES)
		byteValue = 0xff;

	for (int i = 0; i < CRnumTraceBytes; i++)
		bitmap[i] = byteValue;
}

Bool
CRtraceMap::scanForBit()
{
	for (int i = 0; i < CRnumTraceBytes; i++) {
		if (bitmap[i] != 0) {
			return YES;
		}
	}
	return NO;
}


CRtraceTbl::CRtraceTbl()
{
	setAllBits(NO);
}

void
CRtraceTbl::init()
{
	setAllBits(NO);

	if (CRprocname[0] != '\0')
	{
		char filename[sizeof(CRtraceDir) + MHmaxNameLen + 6];
		strcpy(filename, CRtraceDir);
		if (strncmp(CRprocname, "CEP", 3) == 0)
			strcat(filename, "CEP");
		else
			strcat(filename, CRprocname);
		strcat(filename, "_flags");
		setMap(fopen(filename, "r"));
	}
}

void
CRtraceTbl::dump() const
{
	fprintf(stderr, "dumping CRtraceTbl: tracingFlag=%d",
		tracingFlag);
	for (int i = 0; i < CRmaxflag; i++)
	{
		if (i % 10 == 0)
			fprintf(stderr, "\n");
		fprintf(stderr, "%d ", isBitSet(i) ? 1 : 0);
	}
}

void
CRtraceTbl::setBit(Short bitnum)
{
	tracingFlag = YES;
	tracemap.setBit(bitnum);
}

void
CRtraceTbl::clearBit(Short bitnum)
{
	tracemap.clearBit(bitnum);
	tracingFlag = tracemap.scanForBit();
}

/* set all of bits specified in the file fp.
** It is assumed that all bits are specified by decimal numbers,
** separated by whitespace and/or commas.
** There is no restriction on how many numbers can appear on a line,
** but the maximum line length is inbufsize below
*/

Bool
CRtraceTbl::setMap(FILE* fp)
{
	if (fp == NULL)
		return NO;
	
	const int inbufsize = 300;
	char inbuf[inbufsize+1];

	while (fgets(inbuf, inbufsize, fp))
	{
		if (inbuf[0] == '#')
			continue;

		procDecBits(inbuf);
	}
	
	fclose(fp);
	return YES;
}
void
CRtraceTbl::procDecBits(char* curptr)
{
	/* loop thru each value, setting the corresponding bit */
	const char* bitDelimChars = " ,\t\n";
	
	for (char* tokptr = strtok(curptr, bitDelimChars);
	     tokptr;
	     tokptr = strtok(NULL, bitDelimChars))
	{
		if (isdigit(*tokptr))
			setBit(atoi(tokptr));
		else
			tracemap.setMap(tokptr);
	}
}

void
CRtraceTbl::setAllBits(Bool binaryValue)
{
	tracemap.setAllBits(binaryValue);
	setTraceFlag(binaryValue);
}

void
CRtraceTbl::copy(Bool anyBitsSet, const CRtraceMap& from)
{
	setTraceFlag(anyBitsSet);

	if (tracingFlag == NO)
	{
		setAllBits(0);
	}
	else
	{
		for (int i = 0; i < CRnumTraceBytes; i++)
		{
			tracemap.bitmap[i] |= from.bitmap[i];
		}
	}
}

void
CRtraceTbl::setTraceFlag(Bool newValue)
{
	tracingFlag = newValue;
}

#if __SUNPRO_CC != 0x420
CRerrorObj::CRerrorObj(const char * somefile, int someline)
#else
CRerrorObj::CRerrorObj(char * somefile, int someline)
#endif
{
	line = someline; // line number CRERROR is on
	file = somefile; // file that CRERROR is in
}

void
CRerrorObj::spoolErr(const Char * format,... )
{
	va_list ap;
	va_start(ap, format);

	mutex_lock(&CRlockVarible);
	CRmsg *msgPtr;
	msgPtr = CRerrorPtr();

	// fix this 
	char omKey[CROMKEYSZ]; 
	snprintf(omKey,CROMKEYSZ,"%s!%d",basename((char *)file),line);
	msgPtr->storeKey(omKey, CROMCRERROR);

	//const Char* wholeError = "something";
	static char wholeError[8000];

	//replace GLvsprintf with vsnprintf
	//const Char* wholeError = GLvsprintf(format, ap);
	int num_bytes_printed = vsnprintf(wholeError, 
				sizeof(wholeError) -1, format, ap);
        wholeError[sizeof(wholeError)-1]='\0';


	va_end(ap);

	msgPtr->add(CRerrFmt, CRprocname, file, line).spool(wholeError);

	//free((void *) wholeError);

	mutex_unlock(&CRlockVarible);

}

