/*
**      File ID:        @(#): <MID18677 () - 04/20/04, 28.1.1.1>
**
**	File:					MID18677
**	Release:				28.1.1.1
**	Date:					04/26/04
**	Time:					17:42:05
**	Newest applied delta:	04/20/04
**
** DESCRIPTION:
**      This file defines the member functions of the class CRuslParser
**      that are common to both an interactive and non-interactive USL parser.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#ifdef CC
#include <wait.h>
#else
#include <sys/wait.h>
#endif
#include "cc/hdr/cr/CRuslParser.H"
#include "CRcmdTbl.H"
#include "cc/cr/hdr/CRparmv.H"
#include "cc/cr/hdr/CRcepArgv.H"
#include "cc/cr/hdr/CRblockList.H"
#include "CRctBlock.H"
#include "CRctParm.H"
#include "cc/hdr/cr/CRusl.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "CRyaccUtil.H"
#include "cc/hdr/cr/CRvpath.H"
#include "CRerrMsgs.H"
#include "cc/cr/hdr/CRtokenIze.H"
#include "cc/cr/hdr/CRfunKeyStr.H"
#include "cc/cr/hdr/CRomText.H"
#include "cc/cr/hdr/CRmaintApp.H"
#include "cc/hdr/cr/CRassertIds.H"

CRuslParser* CRparser = NULL;

CRinputProcessor::~CRinputProcessor()
{
}

CRcepArgv CRargv;
static CRparmv CRcmdParmv;

CRuslParser::CRuslParser(CRmaintApplication* mtApp) : application(mtApp)
{
	init("NULL", MHnullQ, NO);
}

CRuslParser::~CRuslParser()
{
	delete cmdTbl; cmdTbl = NULL;
	delete parmTbl; parmTbl = NULL;
	killchildren();
}

void
CRuslParser::init(const char* mhName, MHqid mhQid, Bool interactiveFlag,
		  const char* OMmhName)
{
	CRparser = this;
	strcpy(msghName, mhName);
	if (OMmhName)
		strcpy(msghOMname, OMmhName);
	else
		strcpy(msghOMname, msghName);
	msghQid = mhQid;
	promptFlag = interactiveFlag;
	cmdTbl = NULL;
	parmTbl = NULL;
	curCmdPtr = NULL;
}

/*
** use wait() to get rid of children that are zombie processes
*/


void
CRuslParser::clearZombies()
{
/*
	#ifdef CC
		while (waitpid((pid_t) -1, NULL, WNOHANG|WUNTRACED) > 0)
	#else
		while (wait3(NULL, WNOHANG, NULL) > 0)
	#endif
		; 
*/
}

/* test to see if is a permanent process */
Bool
CRuslParser::isLMT() const
{
	if (strcmp(msghName, "LMT") == 0 ||
	    strcmp(msghName, "RMT") == 0 ||
	    strcmp(msghName, "SCC") == 0 ||
	    strcmp(msghName, "SMSUSLI") == 0 ||
	    strcmp(msghName, "SCHED") == 0)
		return YES;
	else
		return NO;
}

// NAME
//  int checkOnProcess()
//
// PURPOSE
//   This function will check to see is any processes (CEPs)
//   are still running. If they are, then this will return count.
//
// DESCRIPTION
//   I try every way to find out about the children that the
//   parser had fork/exec, but there is no clear cut method. 
//   I try looking at /proc, but this didn't work because I can't
//   look at processes owned by root. So, that is why a system call 
//   to ps is done.
//
// RETURNS 
//   int 
//
// FILES 
//   A temp file is created and a pipe open is done
//

int
CRuslParser::checkOnProcess() const
{
	String tmpfname = tmpnam(NULL);
	CRDEBUG(CRusli,("tmpfname = %s", (const char*) tmpfname));

	/* create file to trap stderr output from /bin/sh
	** If the user tries to execute a non-existant command
	** /bin/sh will print a message on stderr
	*/

	FILE* stderrfp = freopen(tmpfname, "w", stderr);
	if (stderrfp == NULL)
	{
		CRERROR("redirection of stderr to '%s' failed (errno=%d)",
			(const char*) tmpfname, errno);
		return 0;
	}

	String cmdstr = "/bin/ps -ef";
	
	CRDEBUG(CRusli,("running command %s", (const char*) cmdstr));
	
	FILE* fp = popen(cmdstr, "r");
	if (fp == NULL)
	{
		CRERROR("popen of '%s' failed", (const char*) cmdstr);
		fclose(stderrfp);
		pclose(fp);
		return 0;
	}
	fclose(stderrfp);
/*##########
 *#  IBM   10/6/2006   Code Modification  DonBeyer
 *##########*/
	unlink(tmpfname);

	const int maxLineLen = 255;
	static char linebuf[maxLineLen+1];

	/*
	**	Read output from the cmdstr (ps -ef)
	*/
	char *tmpLine = 0;
	char *tmpField = 0;

	int pid = 0;
	int ppid = 0;
	char *proc1 = 0;
	char *proc2 = 0;
	int cntOfCeps = 0;
	int myPid = getpid();
	int fieldCnt = 0;

	while (fgets(linebuf, sizeof(linebuf)-1, fp))
	{
		CRDEBUG(CRusli,("%s",linebuf));

		tmpLine = linebuf;
		tmpField = strtok(tmpLine, " ");
		fieldCnt = 1;
		while(tmpField)
		{
			if(fieldCnt == 2)
				pid=atoi(tmpField);

			if(fieldCnt == 3)
				ppid=atoi(tmpField);

			// because ps -ef could return these two
			// differant lines we have to check field
			// 8 and field 9.
			// root 659 318 0 11:35:52 ?  0:00 in.telnetd
			// root 414 390 0  Apr 06 ?   0:00 mibiisa -p 32962

			if(fieldCnt == 8)
				proc1=tmpField;

			if(fieldCnt == 9)
				proc2=tmpField;

			++fieldCnt;
			tmpField = strtok(NULL, " ");
		}

		if(myPid == ppid)
		{
			CRDEBUG(CRusli,("myPid %d %d %d %s %s\n",
			                      myPid, pid,ppid,proc1,proc2));
/* IBM wyoes 20060504 fixed NULL being used as an int with a conditional */
			if(strncmp(proc1,"CEP",3)== 0 ||
			   strncmp(proc2,"CEP",3)== 0 )

			{
				CRDEBUG(CRusli,("found a CEP still running"));
				++cntOfCeps;
			}
		}
	}

	pclose(fp);

	return cntOfCeps;
}

CRctRetVal
CRuslParser::getLine(char [], int)
{
	CRERROR("CRuslParser::getLine() called");
	return CRFAIL;
}

void
CRuslParser::printManPages() const
{
	CRERROR("CRuslParser::printManPages() called");
}

Bool
CRuslParser::doConfirmation(const char* confirmStr)
{
	char confirmBuf[500];

	sprintf(confirmBuf, CRconfirmPromptFmt, confirmStr);
	if (strlen(confirmBuf) >= sizeof(confirmBuf))
	{
		CRERROR("format buffer overflow");
		confirmBuf[sizeof(confirmBuf)-1] = '\0';
	}
	Bool retval = this->yesNoResponse(confirmBuf);
	if (retval == NO)
	{
		//usr type in something other then Y
		this->printOM("cancelled");
	}
	return retval;
}

Bool
CRuslParser::yesNoResponse(const char* str)
{
	CRERROR("CRuslParser::doConfirmation(%s) called", str);
	return FALSE;
}

CRinProcRetVal
CRuslParser::processInput()
{
	CRERROR("CRuslParser::processInput() called");
	return CRinProcDone;
}

void
CRuslParser::prepForInput()
{
	CRERROR("CRuslParser::prepForInput() called");
}

void
CRuslParser::killchildren()
{
	kill(0, SIGTERM);
}

extern "C" { 
	int yywrap();
}

int
CRuslParser::loadTables(const char* cmdfileList)
{
	if(cmdTbl == NULL)
	{
		cmdTbl = new CRcmdTbl;
	}
	else
	{
		delete cmdTbl;
		cmdTbl = new CRcmdTbl;
	}

	if(parmTbl == NULL )
	{
		parmTbl = new CRparmTbl;
	}
	else
	{
		delete parmTbl;
		parmTbl = new CRparmTbl;
	}

	/* loop thru list of files reading each one
	** building the cmdTbl tables appropriately
	*/

#ifdef EES
	String realfname;
	String fname = "cc/cr/imdb/";
	fname += cmdfileList;
	FILE * fp = CRfopen_vpath(fname, "r", realfname);
	const char* masterFile = (const char*) realfname;
#else
	FILE * fp = fopen(cmdfileList, "r");
	const char* masterFile = cmdfileList;
#endif
	if (fp == NULL)
	{
#ifdef EES
		CRCFTASSERT(CRmissingImdbId,
			    (CRmissingImdbFmt, CRprocname,
			     (const char*) fname));
#else
		CRCFTASSERT(CRmissingImdbId,
			    (CRmissingImdbFmt, CRprocname,
		             cmdfileList));
#endif
		return 0;
	}

	const int CRMAXFNAME = 80;
	char linebuf[CRMAXFNAME+1];
	int linenum = 0;

	while (fgets(linebuf, CRMAXFNAME, fp))
	{
		linenum++;
		if (linebuf[0] == '#')
			continue;

		char fname[CRMAXFNAME];
		if (sscanf(linebuf, "%s", fname) == 1)
		{
			if (CRlexYaccReset(fname, masterFile, linenum))
			{
				yyparse();
				yywrap(); /* closes the file */
			}
		}
	}

	fclose(fp);

	if (CRctNumErrors() == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "dumping cmdTbl:\n");
		cmdTbl->dump();
#endif
#undef DEBUG
		return 1;
	}

        CRCFTASSERT(CRimdbSummaryId,
	            (CRimdbSummaryFmt, CRctNumErrors()));
	return 0;
}

CRctParm*
CRuslParser::getParmEntry(const char* parmName)
{
	return parmTbl->getEntry(parmName);
}

void
CRuslParser::insertParm(const char* parmName, CRctParm* parmPtr)
{
	parmTbl->insert(parmName, parmPtr);
}

CRcmdEntry*
CRuslParser::getCmdEntry(const char* cmdName)
{
	return cmdTbl->getEntry(cmdName);
}

void
CRuslParser::insertCmd(const char* cmdName, CRcmdEntry* cmdPtr)
{
	cmdTbl->insert(cmdName, cmdPtr);
}

CRctRetVal
CRuslParser::executeCmd(const char* cmdStr,
			const char*& CEPmhName, int& CEPpid)
{
	curCmdPtr = cmdStr;

	CRargv.clear_all();

	enterNormalMode();

#ifdef DEBUG
	fprintf(stderr, "cmdStr=|%s|\n", cmdStr);
#endif

	switch (CRtokenIze(cmdStr, CRcmdParmv))
	{
	    case CRtokSuccess:
		break;
	    case CRtokMissingQuote:
		CRparser->printerr(CRerrMissingQuote,
				   msghName);
		curCmdPtr = NULL;
		return CRFAIL;
	    default:
		curCmdPtr = NULL;
		return CRFAIL;
	}

#ifdef DEBUG
	CRcmdParmv.dump();
#endif

	String curCmdStr = CRcmdParmv.getPointer(0);

	if (curCmdStr.is_empty())
	{
		/* user entered nothing or just a semi-colon */
		CRparser->printerr(CRerrNoIM);
		curCmdPtr = NULL;
		return CRFAIL;
	}

	CRargv.arg_fill(curCmdStr.upper(), CRcepArgv::Colon);

	/* get the command entry pointer from the command table */
	CRcmdEntryPtr curCmdEntPtr = cmdTbl->getEntry(curCmdStr.upper());

	/* if not found, then error */
	if (curCmdEntPtr == NULL)
	{
		if (curCmdStr == "?")
		{
			curCmdPtr = NULL;
			return CRHELPNEEDED;
		}
		else
		{
			/* The QUIT command is NOT allowed on any
			** permanent process.
			*/ 
			if (isLMT() == NO)
			{
				if (curCmdStr.upper() == "QUIT")
				{
				  curCmdPtr = NULL;
				  if(checkOnProcess() > 0) //CEP process running
				  {
				    if (CRparser->
				        yesNoResponse(CRquitWithChildren) == NO)
				    {
				        return CRFAIL;
				    }
				    else
				    {
				        return CREOF;
				    }
				  }
				  return CREOF;
				}
			}
			printerr(CRerrBadCommand,
				 (const char*) curCmdStr);
		}
		curCmdPtr = NULL;
		return CRFAIL;
	}


	/* check the rest of the input line by creating objects
	 * that correspond to the input line.
	 */
	CRblockList blockList(CRcmdParmv.numParms(), CRcmdParmv.getArgv(), 1);

	if (blockList.isValid() == FALSE)
	{
		const char* errstr = "";

		switch (CRcmdParmv.numColons())
		{
		    case 0:
			errstr = CRerrNoColon;
			break;
		    case 1:
		    case 2:
			errstr = CRerrBadBlock;
			break;
		    default:
			errstr = CRerrTooManyColons;
			break;
		}

		CRparser->printerr(errstr);
		curCmdPtr = NULL;
		return CRFAIL;
	}


	/* try to match up the input line with the allowed entries */
	int numParmMatches = 0;
	CRctBlockList* closestMatch = curCmdEntPtr->findVariant(&blockList,
								numParmMatches);
	if (closestMatch == NULL)
	{
		if (blockList.length() != 0 && !blockList.isHelpChar())
		{
			CRparser->printerr(CRerrBadCombo);
		}

		if (CRparser->getPromptFlag() == YES)
			curCmdEntPtr->printHelp(&blockList, numParmMatches);

		curCmdPtr = NULL;
		return CRFAIL;
	}


	/* clear old user's parameters from the 'closestMatch' */

	closestMatch->clearUserValues();

	/* load the user's parameters into the 'closestMatch' */
	/* load fails if there are extra parameters */

	CRctRetVal ret;

	ret = closestMatch->loadUserValues(blockList);
	if (ret != CRSUCCESS)
	{
		curCmdPtr = NULL;
		return CRFAIL;
	}

	/* do error checking on parameters entered by user */

	ret = closestMatch->errorCheck();
	switch (ret)
	{
	    case CRSUCCESS:
		break;

	    case CRHELPNEEDED:
		if (CRparser->getPromptFlag() == YES)
			curCmdEntPtr->printHelp();
		curCmdPtr = NULL;
		return ret;

	    case CRDELETEPARM:
		CRERROR("bad return value (%d) from errorCheck", ret);
		curCmdPtr = NULL;
		return ret;

	    default:
		curCmdPtr = NULL;
		return ret;
	}
	
	/* 
	** prompt for all missing REQUIRED parameters. 
	*/
	ret = closestMatch->promptForMissingValues(YES);
	if (ret != CRSUCCESS)
	{
		curCmdPtr = NULL;
		return ret;
	}

	/*
	** prompt for all missing OPTIONAL parameters.
	*/
	if (helpMode == YES)
	{
		ret = closestMatch->promptForMissingValues(NO);
		if (ret != CRSUCCESS)
		{
			curCmdPtr = NULL;
			return ret;
		}
	}

	/* check for confirmation */
	const char* confirmStr = closestMatch->getConfirmation();
	if (confirmStr)
	{
		const CRctParmRef* uclParm = closestMatch->getParm("UCL");
		if (uclParm == NULL || uclParm->hasUserValue() == NO)
		{
			if (doConfirmation(confirmStr) == FALSE)
			{
				curCmdPtr = NULL;
				return CRFAIL;
			}
		}
	}

	/* if everything is OK, then generate a CEP command line */
	
	CRargv.setCepName(closestMatch->getCep());

	closestMatch->fillArgv();

	/*
	** This is put here so that if the permant process
	** SMSUSLI is running this CEP that send the 
	** output(OM) stuff back to it. If this was not
	** here SMSULSI would not get the sendAck and OM from the
	** CEP.
	*/

	if(CEPmhName == NULL)
	{
		CRargv.cmd_process(getMsghName(), getOMmhName());
	}
	else
	{
		if((strcmp(CEPmhName,"SMSUSLI"))==0)
		{
			CRargv.cmd_process("SMSUSLI", "SMSUSLI");
		}
		else
		{
			CRargv.cmd_process(getMsghName(), getOMmhName());
		}
	}

	ret = CRargv.invoke_cmd(CEPmhName, CEPpid);

#ifdef DEBUG
	CRargv.dump();
#endif
#undef DEBUG

	/* if everything is OK, then fork/exec the proper CEP */
#ifdef DEBUG
	fprintf(stderr, "fork/exec '%s'\n", closestMatch->getCep());
#endif

	curCmdPtr = NULL;
	return ret;
}


