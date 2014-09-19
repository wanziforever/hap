/*
**      File ID:        @(#): <MID19393 () - 05/14/04, 28.1.1.1>
**
**	File:					MID19393
**	Release:				28.1.1.1
**	Date:					06/11/04
**	Time:					16:15:25
**	Newest applied delta:	05/14/04
**
** DESCRIPTION:
** 	Definition of the text-only Local Maintenance Terminal (LMT) process
**      initialization functions (main routines).
**
** OWNER:
**	Roger McKee
**
** Modified By:
**	Stephen J. W. Shiou
**
** NOTES:
**  Larry Kollasch 07/30/99 R93 Added timer & check to prevent orphaned process
*/

#include <stdlib.h>
#include <stdio.h>		
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <sys/types.h>
#include "CRtextLmt.H"
#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/bill/BLmtype.H"
#include "cc/hdr/tim/TMmtype.H"
#include "cc/hdr/tim/TMtmrExp.H"
#include "cc/hdr/tim/TMreturns.H"
#include "cc/hdr/eh/EHreturns.H"
#include "cc/hdr/eh/EHhandler.H"
#include "cc/hdr/init/INusrinit.H"
#include "cc/hdr/init/INmtype.H"
#include "cc/hdr/cr/CRmtype.H"
#include "cc/hdr/cr/CRdebugMsg.H"
#include "cc/hdr/cr/CRdbCmdMsg.H"
#include "cc/hdr/cr/CRintParser.H"
#include "cc/hdr/cr/CRlineEditor.H"
#include "cc/cr/hdr/CRinputDev.H"
#include "cc/cr/hdr/CRdap.H"
#include "cc/hdr/cr/CRomdbMsg.H"
#include "cc/cr/hdr/CRgenMHname.H"
#include "cc/cr/hdr/CRsop.H"
#include "cc/hdr/cr/CRsopMsg.H"
#include "cc/hdr/cr/CRtmstamp.H"
#include "cc/cr/hdr/CRfunKeyStr.H"
#include "cc/cr/hdr/CRsysLmtMsg.H"
#include "cc/cr/hdr/CRomText.H"
#include "cc/cr/hdr/CRnewTable.H"
#include "cc/cr/hdr/CRtimers.H"
#include "cc/hdr/db/DBintfaces.H"
#include "cc/hdr/cr/CRuslParser.H"

const char* CRuslLineTerminators = "\n!;?\015";

static char CRimTables[200] = 
#ifdef CC
   "/sn/imdb/LMTimlist";
#else
"SCNlmt";
#endif

CRtextLmt::CRtextLmt() : CRmaintApplication(NULL, NULL)
{
//	setbuf(stdout, NULL);
	char mhname[MHmaxNameLen+1];
	CRgenMHname("LMT", (int) getpid(), mhname);
	CRasyncSCH::setMsghName(mhname);

	fileFlag = NO;
	SpfileFlag = NO;
	parser = NULL;
	editorPtr = NULL;
	uslWindow = NULL;
	inputDevicePtr = NULL;
	curInputProcessor = NULL;
	sopPtr = NULL;
	cmdfile = NULL;
	manPageFlag = NO;
	imlistfileFlag = NO;
	rprocCheckTmrBlock = -1;
	orphanCheckTmrBlock = -1;

	parentPid = getppid();	// Checkpoint the parent process Id

}

static const char* CRclassSecurity[] = {
	"class0", 
	"class1", 
	"class2",
	"class3" 
};

GLretVal
CRtextLmt::procinit(int argc, char* argv[], SN_LVL init_lvl, U_char run_lvl)
{

	int i=1;

	GLretVal retval;

	while ( i < argc )
	{
		if (strcmp(argv[i], "-i") == 0)
		{
			// if argc < i+1 , this test will fail, so don't
			// need to check i+1 against argc
			if (argv[i+1] && *argv[i+1] != '\0')
			{
				imlistfileFlag = YES;
				strncpy (CRimTables, argv[i+1],
                 sizeof(CRimTables) - 1);
				CRimTables[sizeof(CRimTables) - 1] = '\0';

			}
      i=i+2;
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			// to preserve the mutually exclusivity of f and m,
			// while looping to make sure we get i, set flag to
			// opposite value of the excluded option.
			// E.g., if fileFlag has been set to yes, manPageFlag
			// will be set to NO, preserving exclusivity.
			fileFlag = ! manPageFlag;
			i++;
		}
		else if (strcmp(argv[i], "-F") == 0)
		{
			// This flag makes the subshl wait until CEP is done
			fileFlag = ! manPageFlag;
			SpfileFlag = YES;
			i++;
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			manPageFlag = ! fileFlag;
			i++;
		}
		else
		{
			i++;
		}
	}

	retval = CRmaintApplication::procinit(argc, argv, init_lvl, run_lvl);
	if (retval != GLsuccess)
     return retval;

	char qname[MHmaxNameLen+4];
	char* facility;
	char* activate;

	int securityFlag = NO;
	struct passwd *passwdChk = getpwnam("security");
	if (passwdChk != NULL)
	{

    //
    //      Check to see if security feature is active
    //
    sprintf(qname, "%sDBQ", argv[0]);

    if (DBsetUp(qname) == GLsuccess)
    {
      if((retval = DBloadTable("security.cr_security")) == GLsuccess)
      {
        while((retval = DBgetTuple(CR_SECURITYlu)) > 0)
        {
          facility = CR_SECURITYlu[0].value;
          activate = CR_SECURITYlu[1].value;
	
          if (strcmp("subshl", facility) == 0)
          {
            if (strcmp("ON", activate) == 0)
            {
              if(checkOnSecurity() == GLsuccess)
              {
                securityFlag = YES;
                break;
              }
              else
              {
                //print OM
                CROMDBKEY(CRfailSecurity, "/CR044");
                CRomdbMsg om;
                om.add(getenv("LOGNAME"));
                om.add("subshl");
                om.spool(CRfailSecurity);
                DBclose();
                exit(1);
              }
            }
          }
          if(securityFlag == YES)
             break;
        }
      }
      else
      {
        // Don't do anything here since not all customers 
        // have this feature
      }
    }
    else
    {
      CRERROR("DB SETUP FAILED");
    }

    DBclose();
	}

  if (fileFlag == YES)
  {
		inputDevicePtr = new CRrealKeyBoard;
		if(SpfileFlag == YES)
       CRasyncSCH::isOKtoWrite(YES);
	}
  else
  {
		/* NOTE: Due to device file permissions (/dev/tty is rw by
		** other, but /dev/ttyqX is only w by other),
    ** the CRmsgKeyBoard object is given the /dev/tty device name 
		** to use for GLopen(), etc.
		** This is only necessary for the non-LMT applications where 
		** the user has done an 'su' to a different login than the 
		** original login id for the tty.
		*/
    const char* tty = "/dev/tty";
    inputDevicePtr = new CRmsgKeyBoard(getMsghName(), mhqid,
                                       NULL, tty, this);

/*CRmsgKeyBoard::CRmsgKeyBoard(const char* cprocMHname, MHqid cpMhqid,
  const char* rprocMHname,
  const char* device,
  CRmaintApplication* app,
  int interCharTimeout,
  int intrptChar) :
  CRinputDevice(intrptChar),
  applicationPtr(app),
  maxWaitTime(interCharTimeout)*/

	}

	uslWindow = new CRfileDevice(stdout);

	editorPtr = new CRlineEditor(CRuslLineTerminators, YES,
                               inputDevicePtr);
	parser = new CRintParser(this, editorPtr, uslWindow);
	parser->init(getMsghName(), mhqid, YES);

	sopPtr = new CRsop(getMsghName(), *uslWindow, NULL, "\n");

#ifdef EES
	if (imlistfileFlag != YES)
	{
		char* argcmdfile = getenv("USLCOMMANDS");
		if (argcmdfile && *argcmdfile != '\0')
		{
			strncpy (CRimTables, argcmdfile, sizeof(CRimTables) - 1);
			CRimTables[sizeof(CRimTables) - 1] = '\0';
		}
	}
#endif	

	if (!parser->loadTables(CRimTables))
     return GLfail;

#ifdef CC
	if(MHmsgh.onLeadCC())
	{
		;;
	}
	else
	{
		fprintf(stderr,"%s can only be run on the LEAD side\n",
            argv[0]);
		exit(1);
	}
#endif

#ifdef LX
	if(MHmsgh.isOAMLead())
	{
		;;
	}
	else
	{
		fprintf(stderr,"%s can only be run on the ACTIVE pilot\n",
            argv[0]);
		exit(1);
	}
#endif


	return GLsuccess;
}

/* initialize objects that deal with the terminal */
void
CRtextLmt::initDeviceObjects()
{
	inputDevicePtr->init();
	uslWindow->init();
	sopPtr->init();
	curInputProcessor = parser;
}

void
CRtextLmt::shellEscape()
{
	char* OssName = getenv("OSS");
	const char *ntm = "ntm";

	stopTimers();
	inputDevicePtr->suspendInput();
	parser->shellEscape();
	sopPtr->shellEscape();

	/*
	**	For NTM CEP's running with the forground
	**	flag set to YES. This will let the CEPs
	**	print out to the terminal.
	*/
	if (OssName != NULL && strstr(OssName, ntm) == OssName)
	{
		CRasyncSCH::isOKtoWrite(YES);
	}
	else
	{
		CRasyncSCH::isOKtoWrite(NO);
	}

}

void
CRtextLmt::shellReturn()
{
	CRasyncSCH::isOKtoWrite(YES);

	inputDevicePtr->resumeInput();
	sopPtr->shellReturn();
	parser->shellReturn();
	startTimers();
}

GLretVal
CRtextLmt::cleanup()
{
	delete inputDevicePtr; inputDevicePtr = NULL; /* kills rproc */
	delete parser; parser = NULL; /* kills CEPs */

	delete uslWindow; uslWindow = NULL;
	delete sopPtr; sopPtr = NULL;
	curInputProcessor = NULL;

	/* Delete the MSGH queue since this process is not a
	** permanent process.
	*/
	MHmsgh.rmName(mhqid, getMsghName());
/*
**	This is doing the MHmsgh.rmName function again
**	So that is why its comment out
**	CRevent.cleanup(mhqid, getMsghName());
*/

	return GLfail;
}

void
CRtextLmt::printISom(const CRomInfo* argominfo)
{
	if(SpfileFlag != YES)
	{

    CROMDBKEY(CRstartOM, "/CR019");
    CRomdbMsg om;
    om.add(deviceName()).add(getenv("LOGNAME"));
    om.spool(CRstartOM, argominfo);

	}
}

void
CRtextLmt::printOOSom(const CRomInfo* /*ominfo*/)
{
}

void
CRtextLmt::startUp()
{
	if(SpfileFlag != YES)
	{
		uslWindow->addstr(CRcftshStartMsg);
	}
	startTimers();
}

// Private member function
// Stops timers for shell escape
//
void
CRtextLmt::stopTimers()
{
	// Stop the RPROC check
	//
	if ( rprocCheckTmrBlock < 0 )
	{
		CRevent.clrTmr( rprocCheckTmrBlock );
		rprocCheckTmrBlock = -1;
	}

// Continue to monitor for orphan process even when CEP is executing.
// This is how we recognize the problem and exit if RCV:MENU,SPA
// was left running by the user, for example.
//
// 	// Stop the orphan check
//	if ( orphanCheckTmrBlock < 0 )
//	{
//		CRevent.clrTmr( orphanCheckTmrBlock );
//		orphanCheckTmrBlock = -1;
//	}

}

// Private member function
// Starts timers upon initialization and after shell escape
//
void
CRtextLmt::startTimers()
{

	if ( rprocCheckTmrBlock < 0 )
	{
    // Start cyclic timer to guard against RPROC process failure
    //
    rprocCheckTmrBlock = CRevent.setRtmr( CRrprocCheckTime,
                                          CRrprocCheckTag, TRUE);
    if ( TMINTERR( rprocCheckTmrBlock ) )
    {
      CRERROR( "CRevent.setRtmr failed with error code %d",
               rprocCheckTmrBlock );
      fatalExit();
    }
	}

	if ( orphanCheckTmrBlock < 0 )
	{
    // Start cyclic timer to guard against orphaned processes
    //
    orphanCheckTmrBlock = CRevent.setRtmr( CRorphanCheckTime,
                                           CRorphanCheckTag, TRUE);
    if ( TMINTERR( orphanCheckTmrBlock ) )
    {
      CRERROR( "CRevent.setRtmr failed with error code %d",
               orphanCheckTmrBlock );
      fatalExit();
    }
	}
}

// Private member function
// Handles timer events from eventLoop()
//
GLretVal
CRtextLmt::timerHandler(char* msgBuf)
{
	if (((MHmsgBase*) msgBuf)->msgType == TMtmrExpTyp)
	{
    switch (((TMtmrExp*) msgBuf)->tmrTag )
    {
    case CRrprocCheckTag:
      rprocCheck();
      return GLsuccess;

    case CRorphanCheckTag:
      CRDEBUG(CRusli+CRmsginFlg, ("orphan check timer"));
      if ( getppid() != parentPid )
      {
        // We have become an orphaned process.
        // Make a graceful exit.
        //
        CROMDBKEY(CRendOM, "/CR015");
        CRomdbMsg om;
        om.add(deviceName()).add(getenv("LOGNAME"));
        om.spool(CRendOM);
        gracefulExit();
      }
      return GLsuccess;

    default:
      break;
    }
	}

	return CRasyncSCH::timerHandler( msgBuf );	// Check base class
}

// Private member function
// Checks RPROC process and revives it as necessary
// via CRmsgKeyBd::prepareForInput()
//
void
CRtextLmt::rprocCheck()
{
	inputDevicePtr->prepForInput();
}


static char CRmsgBuf[MHmsgLimit];

Bool
CRtextLmt::eventLoop()
{
	static int endCnt = 0;
	for (;;)
	{
		if (fileFlag == NO)
		{
			if (inputDevicePtr->charsReady())
         processPendingEvents(EHMSGONLY);
			else
         waitForNextEvent();
		}

		if (fileFlag == YES || inputDevicePtr->charsReady())
		{
			CRinProcRetVal inpRetVal = curInputProcessor->processInput();

			/*
			** Check to see if errorflag is set then
			** check to see if you recieve X amount of 
			** NG, from the parser. NG being No Good, ie.
			** bad command or esc char. If we recieved x
			** amount with out recieving a good MML command ,
			** then send a OM and MINOR alarm and kill the
			** process.
			*/

			if(parser->errorFlag == YES)
			{
				if(SpfileFlag != YES)
           ++numberOfBadInputs;
			}
			else
         numberOfBadInputs = 0;

			if(numberOfBadInputs > MAX_NUMBER_OF_BAD_INPUTS)
			{
				CROMDBKEY(CRtooManyBadInputs, "/CR060");
				CRomdbMsg om;
				om.add("subshl");
				om.add(numberOfBadInputs);
				om.spool(CRtooManyBadInputs);
				gracefulExit();
      }

			switch (inpRetVal)
			{
      case CRinProcDone:
				if( SpfileFlag == YES )
				{
					//HERE FIX sleep(5);
					/* wait for CEPs to finish */
					if ( parser->checkOnProcess() > 0 )
					{
						sopPtr->flush();
						sleep(3);
						break;
					}
					else
					{
            /*CROMDBKEY(CRendOM, "/CR015");
              CRomdbMsg om;
              om.add(deviceName()).add(getenv("LOGNAME"));
              om.spool(CRendOM);*/
						sopPtr->flush();
            //HERE FIX if(endCnt == 0) 
            if(endCnt < 2) 
						{
              ++endCnt;
							sleep(3);
              break; 
						}
            else     
						{
              MHmsgh.rmName(mhqid, 
                            getMsghName());
              exit(0);
						}
					}
				}
				else
				{
					CROMDBKEY(CRendOM, "/CR015");
					CRomdbMsg om;
					om.add(deviceName()).add(getenv("LOGNAME"));
					om.spool(CRendOM);
					gracefulExit();
				}

      case CRinProcRestore:
				uslWindow->addstr(CRbadEscChar);
				break;

      case CRinProcTimeout:
      case CRinProcAbortCmd:
				break;
      case CRinProcSuccess:
				break;

      default:
				CRERROR("invalid return value (%d) from processInput()",
                inpRetVal);
				fatalExit();
			}

			parser->clearZombies();

			break; /* break out of while loop */
		}
		else
		{
			/* check to make sure RPROC is alive */
			rprocCheck();

			if (sopPtr->flushNeeded())
			{
				break; /* break out of while loop */
			}
		}
	}

	/* Allow some time for any
	** straggler OMs to arrive
	*/
	sleep(1);

	return TRUE;
}

Bool
CRtextLmt::process()
{
	return CRmaintApplication::process();
	parser->clearZombies();
}

/* return FALSE when the program should be terminated
** returns TRUE otherwise
*/
void
CRtextLmt::process(SN_LVL init_lvl)
{
	if (init_lvl != SN_NOINIT)
	{
		if (manPageFlag == YES)
		{
			parser->printManPages();
			gracefulExit();
		}

		if (initDevice() != GLsuccess)
		{
			CRERROR("failed to initialize device");
			fatalExit();
		}
	}

	/* this function will be entered everytime a prompt for a new command
	** is needed.
	** Step 1: process any waiting events (may include OMs to print)
	** Step 2: print the USL command prompt
	** Step 3: WAIT for an event
	*/

	/* Check for messages from client processes */

	/* step 1: check for pending messages */

	processPendingEvents();

	/* if not at start of a line, then should print a newline */
	if (sopPtr->flushNeeded())
	{
		if (uslWindow->cursorPos().x() != 0)
       uslWindow->addch('\n');

		sopPtr->flush();
	}

	/* step 2: print the prompt */

	if(SpfileFlag != YES)
	{
		curInputProcessor->prepForInput(); /* print prompt */
	}

	/* step 3: wait for and process the next event */

	eventLoop();
}

/* processMsg - processes a single message
**	Returns GLsuccess if successfully processed a message.
**      Returns GLfail if could not process the message
*/
CRmsgResult
CRtextLmt::processMsg(GLretVal rcvRtn, char* msgBuf, int msgsz)
{
	if (rcvRtn == GLsuccess)
	{
		if (timerHandler(msgBuf) == GLsuccess)
       return CRmsgAccepted;

		Bool hwFailFlag;
		if (inputDevicePtr->processMsg(msgBuf, msgsz,
                                   hwFailFlag) == GLsuccess)
       return CRmsgAccepted;

		Bool discardOMs = isOKtoWrite() ? NO : YES;
		if (sopPtr->processMsg(msgBuf, discardOMs) == GLsuccess)
       return CRmsgAccepted;
	}
	return CRmaintApplication::processMsg(rcvRtn, msgBuf, msgsz);
}

CRmaintApplication*
CRmakeApplication(const char* /*msghName*/)
{
	return new CRtextLmt;
}

GLretVal
CRtextLmt::checkOnSecurity()
{
	//
	//      Check on group id for classes 
	//
	//      Get my passwd entry
	struct passwd *passwdEnt = getpwuid(getuid());
	char *user;
	if (!passwdEnt)
	{
		CRERROR("ERROR: Could not find my passwd entry.");
		return GLfail;
	}
	user=passwdEnt->pw_name;

	//
	//      Get group entries
	//
	struct group *classGroup;
	int numClasses = sizeof(CRclassSecurity) / sizeof(const char*);

	char **grpUser;

	for (int ii = 0; ii < numClasses; ii++)
	{
		classGroup = getgrnam(CRclassSecurity[ii]);
		grpUser = classGroup->gr_mem;
		while (*grpUser != (char *)NULL)
		{
			if(! (strcmp(*grpUser, user)) )
			{
        if( ! (strcmp(CRclassSecurity[ii],"class3")) )
        {
          // leave imlist file alone, this is a super user
          return GLsuccess;
        }
        else if(! (strcmp(CRclassSecurity[ii],"class0")))
        {
          sprintf(CRimTables,
                  "/sn/imdb/SECURITYimlist");
          return GLsuccess;
        }
        else
        {
          sprintf(CRimTables,
                  "/sn/imdb/usr/LMTimlist.%s",
                  CRclassSecurity[ii]);
          return GLsuccess;
        }
			}
			CRDEBUG(CRusli,("user  = %s",*grpUser));
			grpUser++;
		}
	}
	return GLfail;
}

