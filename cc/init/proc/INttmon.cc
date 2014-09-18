//*********************************************************************************
// This file implements the code to start/stop & manage the TimesTen database
// services needed by applications running on the platform.  
//
//
// Key routines:
//	broadcastMessage   - Broadcasts the MessageH message
//********************************************************************************/

/* IBM wyoes 20060508 added sys/stat.h for chmod prototype */
#include <sys/stat.h>

#include	<stdlib.h>
#include  <iostream>
#include  <fstream>
#include	<memory.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<errno.h>
#include 	<sys/uio.h>
#include  <dirent.h>

#ifdef __linux
/* IBM wyoes 20060705 added luc_compat.h for wrappers */
#include "luc_compat.h"
#endif

#include	<hdr/GLtypes.h>
#include	<cc/hdr/su/SUexitMsg.H>
#include	<cc/hdr/init/INusrinit.H>
#include	<cc/hdr/init/INinitSCN.H>
#include	<cc/hdr/init/INsetRstrt.H>
#include	<cc/hdr/init/INmtype.H>
#include	<cc/hdr/cr/CRdebugMsg.H>
#include 	<cc/hdr/eh/EHhandler.H>
#include	<cc/hdr/tim/TMtmrExp.H>
#include 	<cc/hdr/cr/CRdbCmdMsg.H>
#include 	<cc/hdr/cr/CRomdbMsg.H>
#include 	<cc/hdr/cr/CRfeatureStatus.H>
#include   	<cc/hdr/msgh/MHinfoExt.H>
#include        "cc/hdr/cr/CRindStatus.H"
#include	"cc/hdr/db/DBintfaces.H"
#include 	"cc/hdr/db/DBfmUpdate.H"
#include 	"cc/hdr/db/DBfmInsert.H"
#include	"cc/hdr/db/DBfmDelete.H"
#include 	"cc/hdr/timesten/TTMsgs.h"
#include 	"cc/hdr/timesten/dbdefs.h"
#include	"cc/db/x10/utils/locktimesten.H"
#include	"cc/hdr/misc/GLvpath.h"

#include	<sys/wait.h>
#include 	<string>
#include	<set>
#include	<sstream>

#include <unistd.h>
#include <netdb.h>

#ifndef EES
#include 	<sql.h>
#include 	<timesten.h>
#include 	<sqlext.h>
#include 	<sqltypes.h>
#endif

EHhandler	INevent;
MHqid		INmhqid;		

#define         INmaxTimers             		30
#define         INtimestenDaemonCheckTag       		1L
#define         INtimestenDaemonCheckInterval		15
#define         INtimestenDatastoreCheckTag    		2L
#define         INtimestenDatastoreCheckInterval	5
#define		INwallClockTag				4L
#define		INwallClockInterval			1
#define		INttMonitorCheckTag			5L
#define		INttMonitorCheckInterval		15
#define		INtimestenNonNativeCheckTag		6L
#define 	INtimestenNonNativeCheckInterval 	15

#define         CHECK_LOG_DATE_TAG      		7L
#define         CHECK_LOG_DATE_INTVL    		300 // 5 minutes

#define         LOG_DIRECTORY           "/ttlog"
//tt41data.log0

// #define         MKLOCAL
// #include	"cc/init/proc/INorbmon.H"

// struct hostent* hostStruct;
// struct in_addr* hostNode;
#define TTNonNativeVersion "tt4538"

void startLittlettMonitor();
void startDB();
void stopDB();
void startNonNativeDB();
void stopNonNativeDB();
bool nonNativeDBStarted();
bool nonNativeDBEnabled();
void installNonNativeDB();
void broadcastDataReadyMsg();
void invalidDatastoreDetected();
void gracePeriodMechanism();
void postGracePeriodOne( std::string &pids );
void postGracePeriodTwo( std::string &pids );
void setupServerEnv(void);
bool setRamGracePeriod(const char *dsn, int seconds=60) throw();
bool setLoggingFacility(const char *facility="local1") throw();
#ifndef EES
std::string getSQLError(const SQLHENV &env, const SQLHDBC &dbc);
#endif

  // functions to handle indicators
static void clearIndicators();
static void reportIndicators();

void checkLogAge();

  // the name of INIT process
#define INIT_PROCESS "INIT"

  // TimesTen database state strings
static const char* TT_DB_IS_STR = "IS";
static const char* TT_DB_OOS_STR = "OOS";

  // TimesTen database indicators' names
static const char* DB_IN_MEMORY = "DBinMemory";
static const char* DB_DURABLE_INTV = "DBdurableIntv";  // durable interval
static const char* DB_CHKPT_INTV = "DBchkptIntv";
static const char* DB_DURABLE_INTV_VOL = "DBdurableIntvVol";  // trans no
static const char* DB_CHKPT_INTV_VOL = "DBchkptIntvVol";

static const char* DB_NA_STR = "N/A";

// This has to account for EE and server environment.  EE environment supports
// both 32 and 64 bit timesten dynamically.  X10_RELOCATABLE_BASE points
// to the correct position.  Note, no feature locking in EE

// Here are the definitions for both EE and server
#ifdef EES
static const char* TT_CHECK_DB_STATUS = "$X10_RELOCATABLE_BASE/opt/TimesTen/bin/ttStatus | /bin/grep 'Shared Memory' > /dev/null";
#else
static const char* TT_CHECK_DB_STATUS = "/vendorpackages/TimesTen/bin/ttStatus | /bin/grep 'Shared Memory' > /dev/null";
static const char* TT_SYNC_CONFIG = "/cs/sn/db/x10/bin/TTsyncConfig > /dev/null";
#endif

static const char* TT_CHECK_LITTLE_TTMONITOR_STATUS = "/usr/bin/pgrep '\\<ttMonitor\\>' > /dev/null";
static const char* TT_CHECK_DAEMON_STATUS = "/usr/bin/pgrep '\\<timestend\\>' > /dev/null";
static const char* TT_CHECK_SERVER_STATUS = "$X10_BIN/ttStatus | /bin/grep 'TimesTen server pid' > /dev/null";
#define TIMESTEN_MAIN_DAEMON_EXECUTABLE "$X10_BIN/timestend"
#define TIMESTEN_MAIN_DAEMON_TEMP_FILE  "/tmp/timestend.dec.tmp"

// These pointers are setup later
static char* TT_START_DB;
static char* TT_STOP_DB;
static char* TT_START_LITTLE_TTMONITOR;
static char* TT_LIST_ALL_STALE_APPS;
static char* TT_LIST_PARTIAL_STALE_APPS;
static char* TT_LIST_ALL_PROCESSES_USING_MSGQ;
static char* TT_CHECK_FEATURE;
static char* TT_DECRYPT_MAIN_DAEMON;
static char* TT_CHECK_ELF_DAEMON_EXECUTABLE;
static char* MV_MAIN_DAEMON_EXECUTABLE;

  // TimesTen Parameters Table
#define TIMESTEN_PARM 			"TIMESTEN_PARM"
#define DEFAULT_DURABLE_INTVL 		30
#define DEFAULT_TRANS_NO 		100
#define DEFAULT_CHECKPOINT_INTVL 	900
#define DEFAULT_TRANS_LOG_FILE_NO 	10
#define DEFAULT_D_G_P_1			5
#define DEFAULT_D_G_P_2			5
#define DEFAULT_MAX_LOG_FILE_AGE	0

  // TimesTen current database states
static TT_DB_STATE g_currentDBstate;  // TT_DB_STATE is defined in cc/hdr/timesten/TTMsgs.h
static int g_currentDurableIntvl;  // durable commit interval
static int g_currentTransNo;  // number of new transactions
static int g_currentCheckpointIntvl;  // checkpoint interval
static int g_currentTransLogFileNo;  // number of new checkpoint files
static int g_currentDGP1;  // disconnect grace period 1
static int g_currentDGP2;  // disconnect grace period 2
static int g_currentMaxLogFileAge;

static int g_wallClock;  // the wall clock
static int g_ttMonitorDeath;  // ttMonitor continuous deaths counter
static int g_ttDaemonDeath;  // timesten daemon continuous deaths counter
static int g_ttServerDeath;  // timesten server continuous deaths counter
static int g_gracePeriodStage;  // in grace period one or two? or not in grace period?
static int g_gracePeriodClock;  // how far is in the current grace period

CROMDBKEY( X10_OOS, "/X1011" );  // timesten becomes OOS because of other reasons
CROMDBKEY( OM_DAEMON_DIED, "/X1020" );  // timesten daemon died
CROMDBKEY( OM_SERVER_DIED, "/X1076" );  // timesten server died
CROMDBKEY( OM_ttMonitor_DIED, "/X1022" );  // ttMonitor died
CROMDBKEY( OM_DATASORE_READY, "/X1023" );  // datastore is ready
CROMDBKEY( OM_INVALID_DATASTORE_IN_USE, "/X1024" );  // invalid datastore is in use by
CROMDBKEY( OM_INVALID_DATASTORE_RELEASED, "/X1025" );  // invalid datastore is released
CROMDBKEY( OM_INITIALIZE_PROCS, "/X1026" );  // initialize processes which have message queue
CROMDBKEY( OM_SEND_SIGTERM, "/X1027" );  // send sigterm to unix processes
CROMDBKEY( OM_SEND_SIGKILL, "/X1028" );  // send sigkill and escalate
CROMDBKEY( OM_NONNATIVE_TT_FAILED, "/X1049" ); // non-native timesten start failed

CROMDBKEY( OM_ALARM_OLD_LOGFILE, "/X1085" );  // TT LOG is past varible date
CROMDBKEY( OM_CLEAR_OLD_LOGFILE, "/X1086" );  // TT LOG is clear

// This class encapsulates the logic for all paths that need to be set for
// EE environment to work.
static const int maxCommandSize = 256;
static const int maxMsghCmdSize = 512;
class EEenvironment {
public:
	EEenvironment();
	bool getState() const {return theState;}
private:
	bool pathHelper(char *relativePath, size_t length);
	bool theState;
	char theStartDbCmd[maxCommandSize];
	char theStopDbCmd[maxCommandSize];
	char theStart_ttMonitorCmd[maxCommandSize];
	char theTTlistStaleAppsCmd[maxCommandSize];
	char theListProcessesUsingMsghCmd[maxMsghCmdSize];
	char theTTcheckFeatureCmd[maxMsghCmdSize];
	char theOpensslCmd[maxMsghCmdSize];
	char theElfDaemonCmd[maxMsghCmdSize];
	char theMainDaemonCmd[maxMsghCmdSize];
};

// If this object is in false state, some path failed.  You may want to exit.
EEenvironment::EEenvironment() : theState(false) {

	// set up each path that is needed
	char pathBuffer[128];

	sprintf(pathBuffer, "cc/db/x10/tools/ttbit");
	if (pathHelper(pathBuffer, sizeof(pathBuffer) ) != true) {
  		CRERROR("EEenvironment::EEenvironment: cannot get ttbit path");
		return;
	}
	sprintf(theStartDbCmd, "%s start", pathBuffer);
	::TT_START_DB = theStartDbCmd;
	sprintf(theStopDbCmd, "%s stop", pathBuffer);
	::TT_STOP_DB = theStopDbCmd;
  	CRDEBUG( CRinit, ( "EEenvironment(): after theStartDbCmd; theStartDbCmd=%s", theStartDbCmd) );
  	CRDEBUG( CRinit, ( "EEenvironment(): after theStopDbCmd; theStopDbCmd=%s", theStopDbCmd) );
	
	sprintf(theStart_ttMonitorCmd, "cc/db/x10/ttMonitor/EE/EEttMonitor");
	if (pathHelper(theStart_ttMonitorCmd, sizeof(theStart_ttMonitorCmd) ) != true) {
  		CRERROR("EEenvironment::EEenvironment: cannot get ttMonitor path");
		return;
	}
	::TT_START_LITTLE_TTMONITOR = theStart_ttMonitorCmd;
  	CRDEBUG( CRinit, ( "EEenvironment(): after theStart_ttMonitorCmd; theStart_ttMonitorCmd=%s", theStart_ttMonitorCmd) );

	sprintf(pathBuffer, "cc/db/x10/tools/TTlistStaleApps");
	if (pathHelper(pathBuffer, sizeof(pathBuffer)) != true) {
  		CRERROR("EEenvironment::EEenvironment: cannot get TTlistStaleApps path");
		return;
	}
	sprintf(theTTlistStaleAppsCmd, "%s -e", pathBuffer);
  	CRDEBUG( CRinit, ( "EEenvironment(): after theTTlistStaleAppsCmd; theTTlistStaleAppsCmd=%s", theTTlistStaleAppsCmd) );

	// both are same
	::TT_LIST_ALL_STALE_APPS = theTTlistStaleAppsCmd;
	::TT_LIST_PARTIAL_STALE_APPS = theTTlistStaleAppsCmd;

	sprintf(pathBuffer, "cc/utest/msgh/EE/MHrtCk");
	if (pathHelper(pathBuffer, sizeof(pathBuffer)) != true) {
  		CRERROR("EEenvironment::EEenvironment: cannot get MHrtCk path");
		return;
	}
	strcpy(theListProcessesUsingMsghCmd, pathBuffer);
  	CRDEBUG( CRinit, ( "EEenvironment(): before theListProcessesUsingMsghCmd; theListProcessesUsingMsghCmd=%s", theListProcessesUsingMsghCmd) );
	strcat( theListProcessesUsingMsghCmd, " | /bin/grep i= | /usr/bin/awk '{ printf \"%s %d\\n\", $5, $9 }'");
  	CRDEBUG( CRinit, ( "EEenvironment(): after theListProcessesUsingMsghCmd; theListProcessesUsingMsghCmd=%s", theListProcessesUsingMsghCmd) );

	sprintf(pathBuffer, "cc/db/x10/tools/TTcheckfeature");
	if (pathHelper(pathBuffer, sizeof(pathBuffer)) != true) {
                CRERROR("EEenvironment::EEenvironment: cannot get TTcheckfeature path");
                return;
        }
	sprintf(theTTcheckFeatureCmd, "%s", pathBuffer);
        CRDEBUG( CRinit, ( "EEenvironment(): after theTTcheckFeatureCmd; theTTcheckFeatureCmd=%s", theTTcheckFeatureCmd) );
        ::TT_CHECK_FEATURE = theTTcheckFeatureCmd;


/* IBM wyoes 20061111 script fix cc/vendor to cc/vendor_linux */
#ifdef __linux
	sprintf(pathBuffer, "cc/vendor_linux/openssl/LE/bin/openssl.sh");
#else
	sprintf(pathBuffer, "cc/vendor/openssl/EE/bin/openssl.sh");
#endif

        if (pathHelper(pathBuffer, sizeof(pathBuffer)) != true) {
                CRERROR("EEenvironment::EEenvironment: cannot get openssl.sh path");
                return;
        }
	sprintf(theOpensslCmd, "%s enc -d -des -in \""TIMESTEN_MAIN_DAEMON_EXECUTABLE"\" -out \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" -k \""TIMESTEN_FEATURE_LOCK_KEY"\" 2>&1", pathBuffer);
        CRDEBUG( CRinit, ( "EEenvironment(): after theOpensslCmd; theOpensslCmd=%s", theOpensslCmd) );
	::TT_DECRYPT_MAIN_DAEMON = theOpensslCmd;

	sprintf(theElfDaemonCmd, "/usr/bin/file \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" | /bin/grep ELF");
        CRDEBUG( CRinit, ( "EEenvironment(): after theElfDaemonCmd; theElfDaemonCmd=%s", theElfDaemonCmd) );
	::TT_CHECK_ELF_DAEMON_EXECUTABLE = theElfDaemonCmd;

	sprintf(theMainDaemonCmd, "/bin/mv \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" \""TIMESTEN_MAIN_DAEMON_EXECUTABLE"\"");
        CRDEBUG( CRinit, ( "EEenvironment(): after theMainDaemonCmd; theMainDaemonCmd=%s", theMainDaemonCmd) );
	::MV_MAIN_DAEMON_EXECUTABLE = theMainDaemonCmd;


	theState = true;
}

// Calls a utility library function
bool 
EEenvironment::pathHelper(char *relativePath, size_t length)
{
	GLretVal ret = GLfullpath(relativePath, length);
	switch (ret) {
		case GLsuccess:
			return true;
		
		default:
		break;
	}
return false;
}

#ifdef	EES
static EEenvironment *eeEnv;
#endif


// needed to use DBman
class TimesTenParmTbl 
{
  public:
    enum TimesTenParmField
    {
      DURABLE_INTVL,
      TRANS_NO,
      CHECKPOINT_INTVL,
      TRANS_LOG_FILE_NO,
      D_G_P_1,
      D_G_P_2, 
      MAX_LOG_FILE_AGE,
      end_null 
    };
    
    static DBattribute rbuf[];
    static GLretVal triggerFunc( DBoperation typ, DBattribute *record, char *&retResult );
    static GLretVal checkFields( DBattribute *record, char *&retResult );
};

// Populate the rbuf for this TimesTen Parm table object.  This rbuf structure
// is passed in to the DBman constructor.  The rbuf contains a list of
// attributes within the table.
DBattribute TimesTenParmTbl::rbuf[] =
{
        { "DURABLE_INTERVAL",    4,     FALSE,  "" },
        { "TRANS_NO",    	 5,     FALSE,  "" },
        { "CHECKPOINT_INTERVAL", 5,     FALSE,  "" },
        { "TRANS_LOG_FILE_NO",   4,     FALSE,  "" },
        { "D_G_P_1",   		 3,     FALSE,  "" },
        { "D_G_P_2",   		 3,     FALSE,  "" },
        { "MAX_LOG_FILE_AGE",	 2,     FALSE,  "" },
        { "",            	 0,     FALSE,  "" }
};

// check the data fields passed in
GLretVal TimesTenParmTbl::checkFields( DBattribute *record, char *&retResult )
{
  if ( !record[ DURABLE_INTVL ].set)
  {
    retResult = (Char *)"DURABLE INTERVAL is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ TRANS_NO ].set)
  {
    retResult = (Char *)"NUMBER OF TRANSACTIONS is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ CHECKPOINT_INTVL ].set)
  {
    retResult = (Char *)"CHECKPOINT INTERVAL is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ TRANS_LOG_FILE_NO ].set)
  {
    retResult = (Char *)"NUMBER OF TRANSACTION LOG FILES is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ D_G_P_1 ].set)
  {
    retResult = (Char *)"DISCONNECT GRACE PERIOD 1 is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ D_G_P_2 ].set)
  {
    retResult = (Char *)"DISCONNECT GRACE PERIOD 2 is absent";
    CRERROR(retResult);
    return GLfail;
  }

  if ( !record[ MAX_LOG_FILE_AGE ].set)
  {
    retResult = (Char *)"MAX LOG FILE AGE is absent";
    CRERROR(retResult);
    return GLfail;
  }

  int durableIntvl = atoi( record[ DURABLE_INTVL ].value );
  int transNo = atoi( record[ TRANS_NO ].value );
  int checkpointIntvl = atoi( record[ CHECKPOINT_INTVL ].value );
  int transLogFileNo = atoi( record[ TRANS_LOG_FILE_NO ].value );
  int dgp1 = atoi( record[ D_G_P_1 ].value );
  int dgp2 = atoi( record[ D_G_P_2 ].value );
  int maxLogFileAge = atoi( record[ MAX_LOG_FILE_AGE ].value );
  static Char errmsg[100];
  errmsg[ 0 ] = '\0';

  CRDEBUG( CRinit, ( "Checking fields: durableIntvl=%d    transNo=%d    checkpointIntvl=%d    transLogFileNo=%d    d_g_p_1=%d    d_g_p_2=%d", durableIntvl, transNo, checkpointIntvl, transLogFileNo, dgp1, dgp2 ) );

  if ( durableIntvl < TT_DURABLE_INTERVAL_MIN || durableIntvl > TT_DURABLE_INTERVAL_MAX )
  {
    sprintf( errmsg, "DURABLE INTERVAL must be between %d and %d",
             TT_DURABLE_INTERVAL_MIN, TT_DURABLE_INTERVAL_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    
    return GLfail;
  }

  if ( checkpointIntvl < TT_CHECKPOINT_INTERVAL_MIN || checkpointIntvl > TT_CHECKPOINT_INTERVAL_MAX )
  {
    sprintf( errmsg, "CHECKPOINT INTERVAL must be between %d and %d",
             TT_CHECKPOINT_INTERVAL_MIN, TT_CHECKPOINT_INTERVAL_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

  if ( ( transNo < TT_TRANS_NO_MIN || transNo > TT_TRANS_NO_MAX ) && transNo != TT_TRANS_NO_OFF )
  {
    sprintf( errmsg,
             "NUMBER OF NON-DURABLE TRANSACTIONS must be between %d and %d, or 0",
             TT_TRANS_NO_MIN, TT_TRANS_NO_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

  if ( ( transLogFileNo < TT_TRANS_LOG_FILE_NO_MIN || transLogFileNo > TT_TRANS_LOG_FILE_NO_MAX ) 
       && transLogFileNo != TT_TRANS_LOG_FILE_NO_OFF )
  {
    sprintf( errmsg,
             "NUMBER OF TRANSACTION LOG FILES must be between %d and %d, or 0",
             TT_TRANS_LOG_FILE_NO_MIN, TT_TRANS_LOG_FILE_NO_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

  if ( dgp1 < TT_D_G_P_1_MIN || dgp1 > TT_D_G_P_1_MAX )
  {
    sprintf( errmsg, "DISCONNECT GRACE PERIOD 1 must be between %d and %d",
             TT_TRANS_LOG_FILE_NO_MIN, TT_TRANS_LOG_FILE_NO_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

  if ( dgp2 < TT_D_G_P_2_MIN || dgp2 > TT_D_G_P_2_MAX )
  {
    sprintf( errmsg, "DISCONNECT GRACE PERIOD 2 must be between %d and %d",
             TT_D_G_P_2_MIN, TT_D_G_P_2_MAX );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

  if ( maxLogFileAge < 0 || maxLogFileAge > 60 )
  {
    sprintf( errmsg, "MAX LOG FILE AGE must be between %d and %d",
             0, 60 );
    retResult = errmsg;
    CRERROR(retResult);
    return GLfail;
  }

    // checking passed, assign them to global variables
  g_currentDurableIntvl = durableIntvl;
  g_currentTransNo = transNo;
  g_currentCheckpointIntvl = checkpointIntvl;
  g_currentTransLogFileNo = transLogFileNo;
  g_currentDGP1 = dgp1;
  g_currentDGP2 = dgp2;
  g_currentMaxLogFileAge = maxLogFileAge;

  return GLsuccess;
}


//      Any operations which needs to be performed when the data in the table
//      is modified can be included in this trigger function.
GLretVal TimesTenParmTbl::triggerFunc( DBoperation typ, DBattribute *record, char *&retResult )
{

  CRDEBUG( CRinit,( "TimesTenParmTbl::triggerFunc(): Entry Point. typ = %d", typ ) );

  switch (typ)
  {
    case DBselDone:  // Finished retrieving data
    {
      CRDEBUG( CRinit, ( "TimesTenParmTbl::triggerFunc(): SELECT IS DONE" ) );
      break;
    }
    case DBload:    // Initial loading of data
    case DBselAck:  // Subsequent DBselAck response
    {
      CRDEBUG( CRinit, ( "TimesTenParmTbl::triggerFunc(): SELECT " ) );

      if ( checkFields( record, retResult ) != GLsuccess )
        return GLfail;

      return GLsuccess;
    } 

    case DBinsertOp:  // Recent Change, insert
    {
      CRDEBUG( CRinit, ( "TimesTenParmTbl::triggerFunc(): INSERT " ) );

      retResult = (Char *)"INSERT for "TIMESTEN_PARM" is not allowed";
      CRERROR( retResult );
      return GLfail;
    }

    case DBupdateOp:  // Recent Change, update
    {
      CRDEBUG( CRinit, ( "TimesTenParmTbl::triggerFunc(): UPDATE " ) );

      if ( checkFields( record, retResult ) != GLsuccess )
        return GLfail;

      startLittlettMonitor();  // start/re-start ttMonitor
      reportIndicators();

      return GLsuccess;

    }

    case DBdeleteOp:  // Recent Change, delete
    {
      CRDEBUG( CRinit, ( "TimesTenParmTbl::triggerFunc(): DELETE " ) );

      retResult = (Char *)"DELETE for "TIMESTEN_PARM" is not allowed";
      CRERROR( retResult );
      return GLfail;
    }

    default:        // Ignore other cases
      break;

  } // end switch(typ)

  return GLsuccess;
}

  // register with DBman
DBman TimesTenParmMan( (char *)TIMESTEN_PARM, (U_short) 1,
                TimesTenParmTbl::rbuf, TimesTenParmTbl::triggerFunc);


Short
sysinit( int, char *[], SN_LVL, U_char )
{
	return( GLsuccess );
}

Short
procinit( int, char *argv[], SN_LVL sn_lvl, U_char )
{
    // initialize global variables
  g_currentDurableIntvl = DEFAULT_DURABLE_INTVL;
  g_currentTransNo = DEFAULT_TRANS_NO;
  g_currentCheckpointIntvl = DEFAULT_CHECKPOINT_INTVL;
  g_currentTransLogFileNo = DEFAULT_TRANS_LOG_FILE_NO;
  g_currentDGP1 = DEFAULT_D_G_P_1;
  g_currentDGP2 = DEFAULT_D_G_P_2;
  g_currentMaxLogFileAge = DEFAULT_MAX_LOG_FILE_AGE;
  g_wallClock = 0;
  g_ttMonitorDeath = 0;
  g_ttDaemonDeath = 0;
  g_ttServerDeath = 0;
  g_gracePeriodStage = 0;
  g_gracePeriodClock = 0;

  GLretVal retval;

  CRERRINIT(argv[0]);

  if ( ( retval = INevent.attach() ) != GLsuccess ) 
  {
    INITREQ( SN_LV0, retval, "FAILED TO ATTACH TO MSGH", IN_EXIT );
  }

  if ( ( retval = INevent.regName( argv[0], INmhqid, FALSE, FALSE, FALSE, MH_LOCAL, TRUE ) ) 
       != GLsuccess ) 
  {
    INITREQ( SN_LV0, GLfail, "FAILED TO OBTAIN MSGH QID", IN_EXIT );
  }

	// initialize environment for EE and server
#ifdef EES
	eeEnv = new EEenvironment();
#else
	setupServerEnv();
#endif
 

    // check whether the TimesTen feature is locked or not
    // if it is locked, TTMONITOR should quit
  CRfeatureStatus featureStatus;
  int lockStatus = featureStatus.lockStatus( TIMESTEN_FEATURE_NAME );
  if ( lockStatus == CRUNLOCKED ) 
  {
      // feature is unlocked
    if ( system( TT_CHECK_FEATURE ) != 0 ) 
    {
        // TimesTen main daemon is encrypted
        // now we need to decrypt it before we can proceed
      std::string msg;
#ifdef EES
      FILE *fp = popen( TT_DECRYPT_MAIN_DAEMON, "r" );
#else
      FILE* fp = popen( TT_DECRYPT_MAIN_DAEMON, "r" );
#endif
      if ( fp == NULL ) 
      {
        msg = "Failed to create the decryption process. Error message: ";
        msg += strerror( errno );
        CRERROR( msg.c_str() );
        INITREQ( SN_LV0, 1, msg.c_str(), IN_EXIT );
      }
      char outputBuf[ 1024 ];
      outputBuf[ 0 ] = 0;
      fgets( outputBuf, sizeof( outputBuf ), fp );
      if ( pclose( fp ) )
      {
        msg = "Failed to decrypt the TimesTen main daemon. Error message: ";
        msg += outputBuf;
        CRERROR( msg.c_str() );
        INITREQ( SN_LV0, 1,  msg.c_str(), IN_EXIT );
      }

      if ( system( TT_CHECK_ELF_DAEMON_EXECUTABLE ) ) 
      {
        msg = "Failed to determine the decrypted TimesTen main daemon executable type or it is not in ELF format";
        CRERROR( msg.c_str() );
        INITREQ( SN_LV0, 1,  msg.c_str(), IN_EXIT );
      }

        // now set the permission
      if ( chmod( TIMESTEN_MAIN_DAEMON_TEMP_FILE, 0755 ) )
      {
        msg = "Failed to set file permission. Error message:";
        msg += strerror( errno );
        CRERROR( msg.c_str() );
        INITREQ( SN_LV0, 1,  msg.c_str(), IN_EXIT );
      }

        // now to move the decrypted one back to its normal position
      
      if ( system( MV_MAIN_DAEMON_EXECUTABLE ) )
      {
        unlink( TIMESTEN_MAIN_DAEMON_TEMP_FILE );  // remove the temporary file and don't care the result
        msg = "Failed to put the decrypted TimesTen main daemon in the right place. Error message: ";
        msg += strerror( errno );
        CRERROR( msg.c_str() );
        INITREQ( SN_LV0, 1,  msg.c_str(), IN_EXIT );
      }
      CRDEBUG( CRinit, ( "Successfully decrypted TimesTen main daemon" ) );
      TTfeatureUnlockedMsg unlockMsg;
      if ( INevent.broadcast( INmhqid, ( char* )&unlockMsg, sizeof( TTfeatureUnlockedMsg ), 0, FALSE ) 
             != GLsuccess ) 
      {
         CRERROR( "Failed to broadcast TTfeatureUnlockedMsg" );
      }
    }

    // if TimesTen feature is unlock and non-native TimesTen enable flag is true.
    if ( nonNativeDBEnabled() )
	installNonNativeDB();
  }
  else
  {
    if ( lockStatus == CRLOCKED ) 
    {
      IN_PROGRESS( "FEATURE "TIMESTEN_FEATURE_NAME" IS LOCKED, PROCESS EXITING" );
    }
    else
    {
      IN_PROGRESS( "FEATURE "TIMESTEN_FEATURE_NAME" IS INCORRECTLY CONFIGURED, PROCESS EXITING" );
    }

      // ask INIT not to restart TTMONITOR if TTMONITOR quits
    INsetRstrt restartMsg( argv[ 0 ], TRUE, TRUE );
    restartMsg.send( INmhqid, 0 );  
      // quit
    INinitSCN initMsg;
    initMsg.sn_lvl = SN_LV0;
    initMsg.ucl = YES;
    strlcpy( initMsg.msgh_name, argv[ 0 ], IN_NAMEMX );
    if ( initMsg.send( INIT_PROCESS, INmhqid, 0 ) != GLsuccess )
    {
      CRERROR( "Failed to send INinitSCN msg to INIT to terminate myself, now hard quit" );
      exit(1);
    }
    sleep( 100 );  // give INIT enough time to end this process
  }

  CRDEBUG(CRinit, ( " Before selectDataWait from table "TIMESTEN_PARM ) );

    // Select 1 row only from TimesTen parameters tbl
  if ( DBman::selectDataWait( TIMESTEN_PARM, "", 1 ) != GLsuccess )
  {
    CRERROR( " Failed to execute DBman::selectDataWait, use default values" );
  }
  CRDEBUG(CRinit, ( " After selectDataWait from table "TIMESTEN_PARM ) );
    

  clearIndicators();

  // Initialize timer library
  if ( ( retval = INevent.tmrInit( FALSE, INmaxTimers ) ) != GLsuccess )
  {
    INITREQ( SN_LV0, retval, "FAILED TO INITIALIZE TIMERS", IN_EXIT );
  }

  CRDEBUG( CRinit, ( "procinit entry with SN_LVL=%d ",sn_lvl ) );

  CRDEBUG( CRinit, ( "procinit - trying to start timestenmon" ) );

  startDB();  // Start TimesTen database
    
  CRDEBUG( CRinit, ( "outside the while loop trying to ping x10" ) );

  int retry = 0;
  int tempClock = 0;
  while ( true ) 
  {
    tempClock++;
    if ( system( TT_CHECK_DB_STATUS ) != 0 )  // if data store has been fully loaded?
    {
      // not fully loaded yet

      int retCode;
      if ( ( retCode = system( TT_CHECK_DAEMON_STATUS ) ) != 0 )  // if timesten daemon is running?
      {
        // not running

        retry++;
        CRERROR( "Failed to start TimesTen daemon process in procinit()" );
        if ( retry >= 3 )
        {
          CRERROR( "FAILED TO START TIMESTEN DAEMON IN procinit() FOR 3 TIMES, ESCALATING" );
          INITREQ( SN_LV5, retCode, "FAILED TO START TIMESTEN DAEMON IN procinit() FOR 3 TIMES, SN_LV5 ESCALATING", IN_EXIT );
        }
        else
        {
          CRDEBUG( CRinit, ( "Re-try to start TimesTen" ) );
          startDB();
        }

      }

      if ( tempClock % 10 == 0 )
      { 
        // generate message once every 10 seconds to conserve system resources
        IN_PROGRESS( "TimesTen is loading data into memory...." );
        CRDEBUG( CRinit, ( "TimesTen is loading data into memory...." ) );
      }

      sleep( 1 );
    }
    else
      break;  // TimesTen data store has been fully loaded
  }
  CRDEBUG( CRinit, ( "TimesTen datastore has been fully loaded" ) );

  // Set TimesTen tag
  if ( ( retval = INevent.setlRtmr( INtimestenDaemonCheckInterval, INtimestenDaemonCheckTag, 
                                    TRUE, FALSE ) ) < 0 )
  {
    INITREQ( SN_LV0, retval, "FAILED TO SET TIMER", IN_EXIT );
  }

  if ( ( retval = INevent.setlRtmr( INttMonitorCheckInterval, INttMonitorCheckTag, TRUE, FALSE ) ) 
       < 0 ) 
  {
    INITREQ( SN_LV0, retval, "FAILED TO SET TIMER", IN_EXIT );
  }

  if ( ( retval = INevent.setlRtmr( INtimestenDatastoreCheckInterval, INtimestenDatastoreCheckTag,
                                    TRUE, FALSE ) ) < 0 ) 
  {
    INITREQ( SN_LV0, retval, "FAILED TO SET TIMER", IN_EXIT );
  }

  if ( ( retval = INevent.setlRtmr( INwallClockInterval, INwallClockTag, TRUE, FALSE ) ) < 0 )
  {
    INITREQ( SN_LV0, retval, "FAILED TO SET TIMER", IN_EXIT );
  }

  if ( ( retval = INevent.setlRtmr( INtimestenNonNativeCheckInterval, INtimestenNonNativeCheckTag,
				    TRUE, FALSE ) ) < 0 )
  {
    INITREQ( SN_LV0, retval, "FAILED TO SET TIMER", IN_EXIT );
  }

	if ( ( retval = INevent.setlRtmr( CHECK_LOG_DATE_INTVL,
                                          CHECK_LOG_DATE_TAG,
                                          TRUE, FALSE ) ) < 0 )
        {
                INITREQ( SN_LV0, retval,
                     "FAILED TO SET TIMER FOR CHECK_LOG_DATE_INTVL", IN_EXIT );
        }

  broadcastDataReadyMsg();

#ifndef EES
	char *env = getenv("X10_RAM_GRACE_PERIOD");
	int gracePeriod = (env == 0) ? 60: atoi(env);
	CRDEBUG( CRinit,("TimesTen datastore ram grace period %d", gracePeriod ) );
	if (!setRamGracePeriod("tt41data", gracePeriod) ) {
    	INITREQ(SN_LV0, retval, "FAILED TO SET RAM GRACE PERIOD", IN_EXIT );
	}
#endif

  CROMDBKEY(X10_OOS_CL,"/X1053");
  CRomdbMsg clearOm;
  clearOm.spool(X10_OOS_CL);

  return(GLsuccess);
}

#ifndef EES
//
// Return value:
//   0: ok
//   1: SQLAlloc Failure
//   2: SQLAllocConnect Error
//   3: SQLDriverConnect Error 
//   4: pszConnStrOut is null or unErrBufLen less than 1
//
int 
checkClientConnection(char *pErrBufOut, unsigned int unErrBufLen)
{
    SQLRETURN rc = SQL_SUCCESS;	
    SQLHENV   henv;
    SQLHDBC   hdbc;
    
    if (pErrBufOut == 0 || unErrBufLen < 1)
    {
        return 4;
    }
    
    /* Allocate environment */
    if ((rc = SQLAllocEnv(&henv)) != SQL_SUCCESS)
    {
        CRERROR("Unable to allocate an environment handle errorcode %d",rc);
        sprintf(pErrBufOut, "Unable to alloc env handle errorcode=%d", 	rc);
        return 1;
    }

    /* Use the environment to allocate an HDBC */
    if ((rc = SQLAllocConnect(henv, &hdbc)) != SQL_SUCCESS)
    {
        CRERROR("Unable to allocate connection errorcode %d", rc);
        sprintf(pErrBufOut,"Unable to allocate connection errorcode =%d", rc);
        SQLFreeEnv( henv );
        return 2;
    }
    
    SQLCHAR     szConnStrOut1[1000] = {0};
    SQLSMALLINT cbConnStrOut;

    int attempt = 1;
    std::string sqlErr;
    while ((rc = SQLDriverConnect(hdbc,
                                  NULL,
                                  (SQLCHAR*)(TT_DB_CONN_STR_1 ";ttc_timeout=5"),
                                  SQL_NTS,
                                  (SQLCHAR*) szConnStrOut1,
                                  sizeof(szConnStrOut1),
                                  &cbConnStrOut,
                                  SQL_DRIVER_NOPROMPT)) == SQL_ERROR)
    {
        sqlErr = getSQLError(henv, hdbc);
        CRERROR("sql driver connection errorcode %d, attempt %d, error(%s)",
                rc, attempt, sqlErr.c_str());
        if (attempt++ == 10)
        {
            SQLFreeConnect(hdbc);
            SQLFreeEnv(henv);
            return 3;
        }
        sleep(2);
    }

    if (attempt > 1)
    {
        CRDEBUG_PRINT(1, ("sql driver connection successful on attempt %d",
                          attempt));
    }

    // close the handle
    SQLDisconnect( hdbc );
    SQLFreeConnect( hdbc );
    SQLFreeEnv( henv );
    
    return 0;
}
#endif

Short
cleanup(int, char *[], SN_LVL, U_char)
{
    clearIndicators();
    return(GLsuccess);
}

Void
process(int, Char *argv[], SN_LVL sn_lvl, U_char)
{
  GLretVal retval;
  MHmsgBase *msg_p;

  union
  {
    LongLong align;
    Char     msgbuf[ MHmsgLimit ];
  };

  Long msglen;
  Long serverCheckTmr = -1;
  char pErrBufOut[1024] = {0};

  // flag if configuration files on local disks need to be synchronized
  // to the Active CC if running on Leaqd CC of an A/A configuration
  static bool configToSync = true;

  FILE* p;

  CRDEBUG( CRinit, ( "process entry with SN_LVL=%d", sn_lvl ) );

  while( true ) 
  {
    char * ptr;

    IN_SANPEG();
    msglen = sizeof(msgbuf);
    retval = INevent.getEvent( INmhqid, msgbuf, msglen );
    if ( retval == GLsuccess ) 
    {
      msg_p = ( MHmsgBase * )msgbuf;
      switch( msg_p->msgType ) 
      {
        case TMtmrExpTyp:
          switch( ( ( TMtmrExp* )msg_p )->tmrTag )
          {
	    case CHECK_LOG_DATE_TAG:
                CRDEBUG( CRinit, ( "Time to check the transaction log date" ) );
                checkLogAge();
                break;

            case INtimestenDaemonCheckTag:
              if ( system( TT_CHECK_DAEMON_STATUS ) != 0 )
              {
                CRERROR( "The death of timesten daemon is detected" );
                CRomdbMsg om;
                om.spool( OM_DAEMON_DIED );
                if ( g_currentDBstate == TT_DB_IS ) 
                {
                  g_currentDBstate = TT_DB_OOS;
                  reportIndicators();
                }

                g_ttDaemonDeath++;
                if ( g_ttDaemonDeath >= 3 )
                {
                  CRERROR( "Detected 3 continuous timesten daemon deaths, escalating SN_LV5" );
                  INITREQ( SN_LV5, -1, "DETECTED 3 CONTINUOUS ttMonitor DEATHS, SN_LV5 ESCALATING",
                           IN_EXIT);
                }
                else
                {
                  startDB();
                }
              }
              else
              {
                CRDEBUG( CRinit, ( "timesten daemon is running, good" ) );
		if (g_ttDaemonDeath > 0)
                {
			CROMDBKEY(OM_DAEMON_DIED_CL,"/X1055");
			CRomdbMsg clearOm;
			clearOm.spool( OM_DAEMON_DIED_CL );
                	g_ttDaemonDeath = 0;  // clear the counter
		}
                  // DB state turns back to IS only after the datastore is fully loaded into memory
                  // don't set it to IS here!

#ifndef EES
                // If on Active/Active machine and need to sync configuration,
                // then do it.
                if (configToSync)
                {
                  CRDEBUG( CRinit, ( "Executing command %s", TT_SYNC_CONFIG ) );
                  int ret = system( TT_SYNC_CONFIG );
                  if ( ret == 0 )
                  {
                    configToSync = false;
                  }
                  else
                  {
                    CRDEBUG( CRinit, ( "Command %s failed, ret(%d)",
                             TT_SYNC_CONFIG, ret ) );
                  }
                }
#endif
              }

#ifndef EES
	      // Check ttcserver
	      if ( system( TT_CHECK_SERVER_STATUS ) != 0 ) // 0 ok, non-zero failed
              {
                CRERROR( "Death of TimesTen server is detected" );
                sprintf(pErrBufOut, "Death of TimesTen server is detected");
                g_ttServerDeath++;    	       
              }
              else
              {
                if ( checkClientConnection( (char *)pErrBufOut,
                                            sizeof(pErrBufOut) ) == 0 )
                {
              	  CRDEBUG( CRinit, ( "TimesTen C/S connection is good." ) );
              	  if ( g_ttServerDeath > 0 )
                  {
	            CROMDBKEY( OM_SERVER_DIED_CL, "/X1077" );
                    CRomdbMsg clearOm;
                    clearOm.spool( OM_SERVER_DIED_CL );
                    g_ttServerDeath = 0;  // clear the counter
              	  }
                }
                else
                {
                  CRERROR( "TimesTen client failed to connect to server." );
                  g_ttServerDeath++;
                }
              }

              // Issue alarm periodically without attempting any recovery
              // action.  The full recovery action can only be done in a
              // feature later.  (hwcheng 01/27/07)
              if ( g_ttServerDeath % 40 == 1 )
              {
                CRomdbMsg om;
                om.add( pErrBufOut );
                om.spool( OM_SERVER_DIED );
              }
#endif

              break;  // INtimestenDaemonCheckTag

            case INtimestenDatastoreCheckTag:
              if ( g_gracePeriodStage != 0 )
                break;  // don't check db status when in grace period

              if ( system( TT_CHECK_DB_STATUS ) != 0 )
              {
                if ( g_currentDBstate == TT_DB_IS )
                {
                  // invalid datastore detected
                  CRDEBUG( CRinit, ( "Invalid datastore detected by TTMONITOR" ) );
                  invalidDatastoreDetected();  // will also set DB state to OOS
                }
              }
              else
              {
                if ( system( TT_LIST_PARTIAL_STALE_APPS ) == 0 )
                {
                    // found processes still connecting to invalid data store
                    // there are two images in memory!
                    // this could happen in this scenario:
                    // TrapMgr failed to send datastore invalid message to TTMON
                    // The database store is fairly small and can be loaded within
                    // less than 5 seconds
                  CRDEBUG( CRinit, ( "Invalid datastore detected by TTMONITOR, 2 more images possible!" ) );
                  invalidDatastoreDetected();  // will also set DB state to OOS
                }
                else if ( g_currentDBstate != TT_DB_IS )
                {
                  broadcastDataReadyMsg();  // will also set DB status to IS
                }
                else
                {
                  CRDEBUG( CRinit, ( "Datastore is good" ) );
                }
              }
              break;

            case INwallClockTag:
              g_wallClock++;
              if ( g_gracePeriodStage != 0 )
                gracePeriodMechanism();
              break;

            case INttMonitorCheckTag:
            {
              if ( system( TT_CHECK_LITTLE_TTMONITOR_STATUS ) != 0 )
              {
                CRERROR( "The death of ttMonitor is detected" );
	        CRomdbMsg om;
		om.spool( OM_ttMonitor_DIED ); 
                g_ttMonitorDeath++;
                if ( g_ttMonitorDeath >= 3 )
                {
                  CRERROR( "Detected 3 continuous ttMonitor deaths, escalating SN_LV5" );
                  INITREQ( SN_LV5, -1, "DETECTED 3 CONTINUOUS ttMonitor DEATHS, SN_LV5 ESCALATING", 
                           IN_EXIT);
                }
                else
                  startLittlettMonitor();  // restart ttMonitor
              }
              else
              {
                CRDEBUG( CRinit, ( "ttMonitor is running, good" ) );
		if (g_ttMonitorDeath > 0){
			CROMDBKEY(OM_ttMonitor_DIED_CL,"/X1056");
			CRomdbMsg clearOm;
			clearOm.spool(OM_ttMonitor_DIED_CL);
                	g_ttMonitorDeath = 0;  // clear the counter
		}
              }
              break;
            }

     	    case INtimestenNonNativeCheckTag:
     	    {
     	      if ( system( TT_CHECK_DAEMON_STATUS ) == 0 && ( nonNativeDBEnabled() ) )
     	      {
     		// if non-native TimesTen enable flag is true, and it is not start up.
     		if ( ! nonNativeDBStarted() )
     		{
     			startNonNativeDB();
     		}
     		else
     		{
     			CRDEBUG( CRinit, ( "non-native timesten is running, good" ) );
     		}
     	      }
     	    }
     	    break; // INtimestenNonNativeCheckTag

            default:
              CRERROR( "Invalid Timer Tag received" );
          }
          break;

        case SUexitTyp:
          if ( msglen != SUexitSiz )
          {
            CRERROR( "SUexitTyp bad msg length=%d", msglen );
          }
          else
          {
            if ( g_currentDBstate == TT_DB_IS )
            {
              CRomdbMsg om;
              om.spool( X10_OOS );
	      clearIndicators();
	    }
	    stopNonNativeDB();
            stopDB();
            exit(0);
          }
          break;

        case CRdbCmdMsgTyp:         // a debug on/off control msg
          ( ( CRdbCmdMsg* ) msg_p )->unload();
          break;

        case CRindQryMsgTyp:  // SYSTAT asks for the indicators
          reportIndicators();
          break;

	case DBselectAckTyp:
        {
            // message length of DBfm meesages are not checkable
	  DBselectAck *dbSelAckMsg = (DBselectAck *)msg_p ;
          DBman::dbSelAckMsg( dbSelAckMsg, msglen, INmhqid );
          break;
        }

        case DBfmInsertTyp:     // Insert indication from a form
        {
            // message length of DBfm meesages are not checkable
          DBfmInsert *fmMsgp = (DBfmInsert *)msg_p;
          DBman::dbFmMsg (fmMsgp, msglen, DBinsertOp, DBfmInsertAckTyp, fmMsgp -> srcQue);
          break;
        }

        case DBfmUpdateTyp:     // Update indication from a form
        {
            // message length of DBfm meesages are not checkable
          DBfmUpdate *fmMsgp = ( DBfmUpdate *)msg_p;
          DBman::dbFmMsg (fmMsgp, msglen, DBupdateOp, DBfmUpdateAckTyp, fmMsgp -> srcQue);
          break;
        }

        case DBfmDeleteTyp:     // Delete indication from a form
        {
            // message length of DBfm meesages are not checkable
          DBfmDelete *fmMsgp = ( DBfmDelete *)msg_p;
          DBman::dbFmMsg (fmMsgp, msglen, DBdeleteOp, DBfmDeleteAckTyp, fmMsgp -> srcQue);
          break;
        }

        case TimesTenDataInvalidTyp:
          if ( g_currentDBstate == TT_DB_IS )
          {
            CRDEBUG( CRinit, ( "Invalid datastore msg received from other process, possibly TrapMgr" ) );
            invalidDatastoreDetected();
          }
          break;

        case TimesTenDBstatusQryTyp:
          if ( msglen != sizeof( TTDBstatusQueryMsg ) )
          {
            CRERROR( "TimesTenDBstatusQryTyp bad msg length=%d", msglen );
          }
          else
          {
            TTDBstatusQueryMsg *qryMsg = ( TTDBstatusQueryMsg * )msg_p;
/*
            CRDEBUG( CRinit, ( "Received TimesTen DB status query message from mhqid: %d", 
              qryMsg->srcQue << 0 ) );
*/
            CRDEBUG( CRinit, ( "Received TimesTen DB status query message from mhqid: %s", 
              qryMsg->srcQue.display() ) );
            TTDBstatusQueryAckMsg qryAckMsg( g_currentDBstate );
            retval = qryAckMsg.send( qryMsg->srcQue, INmhqid, sizeof( qryAckMsg ), 0 );
            if ( retval != GLsuccess )
            {
/*
              CRERROR( "Failed to send TTDBstatusQueryAckMsg to mhqid: %d, return code: %d", 
                qryMsg->srcQue << 0 , retval );
*/
              CRERROR( "Failed to send TTDBstatusQueryAckMsg to mhqid: %s, return code: %d", 
                qryMsg->srcQue.display() , retval );
            }
            else
            {
/*
              CRDEBUG( CRinit, ( "Successfully sent TTDBstatusQueryAckMsg to mhqid: %d",
                qryMsg->srcQue << 0 ) );
*/
              CRDEBUG( CRinit, ( "Successfully sent TTDBstatusQueryAckMsg to mhqid: %s",
                qryMsg->srcQue.display() ) );
            }
          }
          break;

        case INinitSCNAckTyp:    // don't care about these messages at this time
        case INinitSCNFailTyp:
          break;

        default:
          CRDEBUG(CRinit, ( "Unknown msgtype=%d received", msg_p->msgType ) );
          break;
      }
    }
  }
}

// To check non-native TimesTen whether enabled
// if file /cs/sn/db/x10/config/nonnativex10 exist, non-native TimesTen is enable
// if file /cs/sn/db/x10/config/nonnativex10 doesnot exist, non-native TimesTen is not enable
bool nonNativeDBEnabled()
{
   if (access( "/cs/sn/db/x10/config/nonnativex10", F_OK) != 0 )
	return false;	// not enabled
   else
	return true;	// enabled
}

bool nonNativeDBStarted()
{
   std::string cs_check = "ps -ef | grep ttcserver | grep -v grep | grep " + \
                          std::string(TTNonNativeVersion) + \
          		  " > /dev/null";
   if (system( cs_check.c_str()) != 0 )
	return false;	// Stopped
   else
	return true;	// Running
}

void stopDB()
{
  // little ttMonitor is stopped by TT_STOP_DB script
  CRDEBUG( CRinit, ( "Stopping TimesTen subsystem" ) );
  if ( system( TT_STOP_DB ) != 0)
  {
    CRDEBUG( CRinit, ( "TimesTen daemon failed to stop" ) );
  }
}

void stopNonNativeDB()
{
  CRDEBUG( CRinit, ( "Stopping non-native TimesTen" ) );
  if ( nonNativeDBStarted() )
  {
     std::string cs_check = "/SU/opt/" +
       		            std::string(TTNonNativeVersion) + \
                            "/bin/ttbit stop > /dev/null";
     if ( system( cs_check.c_str() ) != 0)
     {
    	CRDEBUG( CRinit, ( "non-native TimesTen failed to stop" ) );
     }
  }
}

void startDB()
{
  // CRDEBUG( CRinit, ( "Setting up syslog facility (local1) for TimesTen subsystem" ) );
  //
  // setLoggingFacility(); // TimesTen 7.0.2 no longer needed
  //

  CRDEBUG( CRinit, ( "Starting TimesTen subsystem" ) );
  if ( system( TT_START_DB ) != 0 )
  {
    CRDEBUG( CRinit, ( "TimesTen daemon failed to start" ) );
  }
    // little ttMonitor should be started after you have launched the TimesTen daemon
  startLittlettMonitor();  // script NR/LXttMonitor kills old instance and launch a new instance
}

void startNonNativeDB()
{
  std::string cs_check = "/SU/opt/" +
                         std::string(TTNonNativeVersion) + \
                         "/bin/ttbit start > /dev/null";
  static int firstStart=true;

  CRDEBUG( CRinit, ( "Start non-native TimesTen subsystem" ) );

  if (nonNativeDBStarted() )
  {
	CRDEBUG( CRinit, ("The non-native TimesTen has started") );
	return;
  }

  //  non-native TimesTen is stopped
  if (!firstStart)
  {
	CRDEBUG( CRinit, ("The death of non-native timesten is detected") );
  }

  if ( system( cs_check.c_str() ) != 0)
  {
      CRDEBUG( CRinit, ( "non-native TimesTen failed to start" ) );
  }
  if ( !nonNativeDBStarted() )
  {
   	CRomdbMsg om;
	om.spool( OM_NONNATIVE_TT_FAILED );
  }
}

void startLittlettMonitor()
{
  char command[ 8192 ];
  sprintf( command, "%s %d %d %d %d", TT_START_LITTLE_TTMONITOR, g_currentDurableIntvl,
           g_currentCheckpointIntvl, g_currentTransNo, g_currentTransLogFileNo );
  CRDEBUG( CRinit, ( "Executing command: %s", command ) );
  if ( system( command ) != 0 )
  {
    CRERROR( "Failed to start ttMonitor" );
  }
}

void
broadcastDataReadyMsg()
{
  GLretVal ret;
  CRDEBUG( CRinit, ( "Broadcast TimesTenDataReadyTyp message" ) );
  CRomdbMsg om;
  om.spool( OM_DATASORE_READY );
  TTdataReadyMsg msg;
  g_currentDBstate = TT_DB_IS;
  reportIndicators();
  INevent.broadcast( INmhqid, ( char* )&msg, sizeof( TTdataReadyMsg ), 0, FALSE );
} 

void invalidDatastoreDetected()
{
  if ( g_gracePeriodStage != 0 )
    return;  // already in grace period

  g_currentDBstate = TT_DB_OOS;
  reportIndicators();

  CRDEBUG( CRinit, ( "g_currentDGP1=%d\tg_currentDGP2=%d", g_currentDGP1, g_currentDGP2 ) );
  CRDEBUG( CRinit, ( "Entering disconnect grace period 1" ) );
  g_gracePeriodStage = 1;
  g_gracePeriodClock = g_wallClock + g_currentDGP1;
  
  gracePeriodMechanism();
}

void gracePeriodMechanism()
{
  std::string processes;
  std::string pids;
  char buf[ 8192 ];
  FILE* fp;
  static Bool invalidDatastoreAlarmed = false;
  if ( g_gracePeriodStage == 1 ) 
  {
      // in grace period 1, we will need to list all stale apps
    fp = popen( TT_LIST_ALL_STALE_APPS, "r" );
  }
  else 
  {
      // in grace period 2, we exclude timestend, timestensubd and timestenrepd 
      // to prevent system escalation
    fp = popen( TT_LIST_PARTIAL_STALE_APPS, "r" );
  }
  if ( fp == 0 )
  {
    CRERROR( "Failed to execute TTlistStaleApps" );
  }
  else
  {
    while ( true )
    {
      if ( fgets( buf, 8192, fp ) == 0 )
      {
        if ( !feof( fp ) )
          CRERROR( "Error occurred while reading the output of TTlistStaleApps" );
        break;
      }
      pids += buf;  // Note: no newline char is expected from TTlistStaleApps
    }
    int retCode = pclose( fp );

    if ( retCode != 0 || pids.size() == 0 ) 
    {
        // not found stale apps, quit grace period mechanism
      CRDEBUG( CRinit, ( "Not found stale apps, quit disconnect grace period mechanism" ) );
      if ( invalidDatastoreAlarmed){
      	CRomdbMsg om;
      	om.spool( OM_INVALID_DATASTORE_RELEASED );
      	invalidDatastoreAlarmed = false;
      }
      g_gracePeriodStage = 0;
      g_gracePeriodClock = 0;
      return;
    }
  }

  switch( g_gracePeriodStage )
  {
    case 1:
      if ( g_wallClock >= g_gracePeriodClock )
      {
        if ( g_currentDGP2 == 0 )
        {
          CRDEBUG( CRinit, ( "Because disconnect grace period 2 is set to 0, skip to period 2 handling immediately" ) );
          postGracePeriodTwo( pids );
        }
        else
        {
          postGracePeriodOne( pids );
        }
	if (!invalidDatastoreAlarmed) invalidDatastoreAlarmed = true;
      }
      break;
    case 2:
      if ( g_wallClock >= g_gracePeriodClock )
      {
        postGracePeriodTwo( pids );
	if (!invalidDatastoreAlarmed) invalidDatastoreAlarmed = true;
      }
      break;
    default:
      CRERROR( "Program logic error, unexpected g_gracePeriodStage" );
  }
}

// record stale apps to OMlogs
void recordStaleApps( const char* msg, std::string &pids )
{

  std::string tempStr;
  char buf[ 8192 ];
  std::string cmd = "/bin/ps -f -p '";
  cmd += pids + "'";
  FILE *fp = popen( cmd.c_str(), "r" );
  if ( fp == 0 )
  {
    CRERROR( "Failed to use ps to list stale app info" );
  }
  else
  {
    while ( true )
    {
      if ( fgets( buf, 8192, fp ) == 0 )
      {
        if ( !feof( fp ) )
          CRERROR( "Error occurred while reading the output of ps" );
        break;
      }
      tempStr += buf; 
    }
    pclose( fp );
    tempStr += msg;
    CRomdbMsg om;
    om.add( tempStr.c_str() );
    om.spool( OM_INVALID_DATASTORE_IN_USE );
  }
  
}

  // defined to help using std::set
struct ltint
{
  bool operator()( int i1, int i2 ) const
  {
    return i1 < i2;
  }
};

void postGracePeriodOne( std::string &pids )
{
  recordStaleApps( " PROCESSES ABOVE ARE STILL CONNECTING TO INVALID DATASTORE AT THE END OF DISCONNECT GRACE PERIOD 1",
                   pids );
  g_gracePeriodStage = 2;  // preparing to enter grace period 2
  g_gracePeriodClock = g_wallClock + g_currentDGP2;

  std::set< int, ltint >staleProcs;
  char *buf = new char[ pids.size() + 1 ];
  strcpy( buf, pids.c_str() );
  char *tok = strtok( buf, " \n\r\t" );

  std::string tempStr;

  // extract pids and put them into the set
  while ( tok != 0 )
  {
    int pid = atoi( tok );
    if ( pid <= 0 )
    {
      CRDEBUG( CRinit, ( "Wrong pid, token[%s]", tok ) );
    }
    else 
    {
      staleProcs.insert( pid );
      tempStr += tok;
      tempStr += " ";
    }  
    tok = strtok( 0, " \n\r\t" );
  }
  delete[] buf;

  if ( tempStr.size() != 0 )
  {
    CRDEBUG( CRinit, ( "pids %s are inserted into the set", tempStr.c_str() ) );
  }

std::string cmd;
#ifdef EES
  char eepath[ 256 ];
// THIS HAS TO BE FIXED LATER
//  if ( prependVpath( "cc/utest/msgh/EE/MHrtCk", eepath ) != GLsuccess )
  {
    CRERROR( "Cannot locate file cc/utest/msgh/EEMHrtCk" );
    eepath[ 0 ] = 0;
  }
  cmd = eepath;
  cmd += "| /bin/grep i= | /usr/bin/awk '{ printf \"%s %d\\n\", $5, $9 }'";
#else
  cmd = TT_LIST_ALL_PROCESSES_USING_MSGQ;
#endif
  FILE* fp = popen( cmd.c_str(), "r" );

  if ( fp == 0 )
  {
    CRERROR( "Failed to execute MHrtCk" );
  }

  char qname[ 8192 ];
  int pid;

  tempStr.erase();
  while ( !feof( fp ) )
  {
    if ( fscanf( fp, "%s %d", qname, &pid ) != 0 )
    {
      if ( pid > 0 )
      {
        if ( staleProcs.find( pid ) != staleProcs.end() )
        {
            // remove the underscore(_) sign that may proceding the real local message queue name
            // this is very important
          if ( qname[ 0 ] == '_' )
          {
            char tempQue[ 8192 ];
            strcpy( tempQue, qname + 1 );  // remove the first char, which is the underscore sign
            strcpy( qname, tempQue );
          }
          CRDEBUG( CRinit, ( "qname[%s] pid[%d] is a stale process, send INinitSCN msg to INIT", qname, pid ) );
          INinitSCN initMsg;
          initMsg.sn_lvl = SN_LV1;
          initMsg.ucl = YES;
          strncpy( initMsg.msgh_name, qname, IN_NAMEMX );
          tempStr += "qname[ ";
          tempStr += qname;
          tempStr += " ]  pid[ ";
          char pidStr[ 80 ];
          sprintf( pidStr, "%d", pid );
          tempStr += pidStr;
          tempStr += " ]\n";
          if ( initMsg.send( "INIT", INmhqid, 0 ) != GLsuccess )
          {
            CRDEBUG( CRinit, ( "Failed to send INinitSCN msg to INIT", qname ) );
          }
          else
          {
            CRDEBUG( CRinit, ( "Successfully sent INinitSCN msg to INIT", qname ) );
          }
          staleProcs.erase( pid );
        }
      }
    }
    fgets( qname, 8192, fp );  // skip to next line
    if ( ferror( fp ) )
    {
       CRERROR( "Read from %s pipe error",  cmd.c_str() );
    }
  }
  if ( tempStr.size() > 0 )
  {
    CRomdbMsg om;
    om.add( tempStr.c_str() );
    om.spool( OM_INITIALIZE_PROCS );
  }

  if ( pclose( fp ) != 0 )
  {
    CRERROR( "Failed to execute MHrtCk" );
  }

  // still have non-message queue stale apps to handle?
  if ( staleProcs.size() > 0 )
  {
    tempStr.erase();
    char pidStr[ 80 ];
    std::set< int, ltint >::iterator it = staleProcs.begin();
    while ( it != staleProcs.end() )
    {
      sprintf( pidStr, " %d", *it );
      tempStr  += pidStr;
      it++;
    }
    CRDEBUG( CRinit, ( "Send SIGTERM to these processes: %s", tempStr.c_str() ) );
    CRomdbMsg om;
    om.add( tempStr.c_str() );
    om.spool( OM_SEND_SIGTERM );
    tempStr = "kill " + tempStr;
    system( tempStr.c_str() );
  }
  CRDEBUG( CRinit, ( "postGracePeriodOne(): Entering disconnect grace period 2" ) );
}

void postGracePeriodTwo( std::string &pids )
{
  CRDEBUG( CRinit, ( "postGracePeriodTwo(): kill -9 all stale applications and escalate SN_LV5 pids: %s", 
                     pids.c_str() ) );
  recordStaleApps( " PROCESSES ABOVE ARE STILL CONNECTING TO INVALID DATASTORE AT THE END OF DISCONNECT GRACE PERIOD 2",
                   pids );
  g_gracePeriodStage = 0;
  g_gracePeriodClock = 0;
  CRomdbMsg om;
  om.add( pids.c_str() );
  om.spool( OM_SEND_SIGKILL );
  std::string cmd = "kill -9 ";
  cmd += pids;
  system( cmd.c_str() );
  INITREQ( SN_LV5, -1, "AFTER DISCONNECT GRACE PERIOD ESCLATING, SN_LV5 ESCALATING", IN_EXIT ); 
}


  // clear all indicators
static void clearIndicators()
{
  g_currentDBstate = TT_DB_OOS;
  reportIndicators();
}

  // report all indicators
static void reportIndicators()
{
  CRindStatusMsg statusMsg;
  if ( g_currentDBstate == TT_DB_IS )
    statusMsg.add( DB_IN_MEMORY, TT_DB_IS_STR );
  else
    statusMsg.add( DB_IN_MEMORY, TT_DB_OOS_STR );

  char temp[ 80 ];
  sprintf( temp, "%d", g_currentDurableIntvl );
  if ( g_currentDBstate == TT_DB_OOS )
    statusMsg.add( DB_DURABLE_INTV, DB_NA_STR );
  else
    statusMsg.add( DB_DURABLE_INTV, temp );

  sprintf( temp, "%d", g_currentCheckpointIntvl );
  if ( g_currentDBstate == TT_DB_OOS )
    statusMsg.add( DB_CHKPT_INTV, DB_NA_STR );
  else
    statusMsg.add( DB_CHKPT_INTV, temp );

  sprintf( temp, "%d", g_currentTransNo );
  if ( g_currentDBstate == TT_DB_OOS )
    statusMsg.add( DB_DURABLE_INTV_VOL, DB_NA_STR );
  else
    statusMsg.add( DB_DURABLE_INTV_VOL, temp );

  sprintf( temp, "%d", g_currentTransLogFileNo );
  if ( g_currentDBstate == TT_DB_OOS )
    statusMsg.add( DB_CHKPT_INTV_VOL, DB_NA_STR );
  else
    statusMsg.add( DB_CHKPT_INTV_VOL, temp );

  statusMsg.send();  // send the indicators to SYSTAT
}

void setupServerEnv(void) {
	TT_START_DB = (Char *)"$X10_OAM_BIN/ttbit start";
	TT_STOP_DB = (Char *)"$X10_OAM_BIN/ttbit stop";

#ifdef __linux
/* ALU slgan 20070115 added LXttMonitor */
	TT_START_LITTLE_TTMONITOR = (Char *)"$X10_OAM_BIN/LXttMonitor";
#else
	TT_START_LITTLE_TTMONITOR = (Char *)"$X10_OAM_BIN/NRttMonitor";
#endif

	// excluding timesten daemons
	TT_LIST_ALL_STALE_APPS = (Char *)"$X10_OAM_BIN/TTlistStaleApps -e";
	TT_LIST_PARTIAL_STALE_APPS = (Char *)"$X10_OAM_BIN/TTlistStaleApps -e";
	TT_LIST_ALL_PROCESSES_USING_MSGQ = (Char *)"/cs/sn/utest/MHrtCk | /bin/grep i= | /usr/bin/awk '{ printf \"%s %d\\n\", $5, $9 }'";
	TT_CHECK_FEATURE = (Char *)"$X10_OAM_BIN/TTcheckfeature";
	TT_DECRYPT_MAIN_DAEMON = (Char *)". $X10_OAM_HOME/config/x10-setenv; LD_LIBRARY_PATH=/vendorpackages/openssl/lib:/vendorpackages/openssl/lib/64:$LD_LIBRARY_PATH;export LD_LIBRARY_PATH; /vendorpackages/openssl/bin/openssl enc -d -des -in \""TIMESTEN_MAIN_DAEMON_EXECUTABLE"\" -out \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" -k \""TIMESTEN_FEATURE_LOCK_KEY"\" 2>&1";
	TT_CHECK_ELF_DAEMON_EXECUTABLE = (Char *)". $X10_OAM_HOME/config/x10-setenv; /usr/bin/file \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" | /bin/grep ELF";
	MV_MAIN_DAEMON_EXECUTABLE = (Char *)". $X10_OAM_HOME/config/x10-setenv; /bin/mv \""TIMESTEN_MAIN_DAEMON_TEMP_FILE"\" \""TIMESTEN_MAIN_DAEMON_EXECUTABLE"\"";
	return;
}

// Set ram grace period, gets executed when TTMONITOR runs, but idempotent
bool setRamGracePeriod(const char *dsn, int seconds) throw()
{
	char ttAdminCmd[256];
	char buffer[128];
	std::stringstream output;
	sprintf(ttAdminCmd, "$X10_BIN/ttAdmin -ramPolicy inUse -ramGrace %d %s", seconds, dsn);
	FILE	*fp = popen(ttAdminCmd, "r");
	if (fp == 0 ) {
  		CRERROR("ttAdmin command failed %s", strerror(errno) );
		return false;
	}
	while(true) {
		if (fgets(buffer, sizeof(buffer), fp ) == 0) {
			// Anything other than eof is abnormal
			if (!feof(fp) ) {
  				CRERROR("Unexpected failure %s", strerror(errno) );
				return false;
			}
			break;
		}
		output << buffer;
	}
	CRDEBUG(CRinit, ("%s", output.str().c_str() ) );
	return true;
}
// Set logging facility for TimesTen.  This has to be done before
// TimesTen is started.  This code belongs in install_timesten.sh whe TimesTen
// revision changes. Hardcoded facility
bool setLoggingFacility(const char *facility) throw()
{
	char *setFacilityCmd = (Char *)"$X10_OAM_BIN/setFacility.sh 2>&1";
	char buffer[128];
	std::stringstream output;
	FILE	*fp = popen(setFacilityCmd, "r");
	if (fp == 0 ) {
  		CRERROR("setFacilityCmd command failed %s", strerror(errno) );
		return false;
	}
	while(true) {
		if (fgets(buffer, sizeof(buffer), fp ) == 0) {
			// Anything other than eof is abnormal
			if (!feof(fp) ) {
  				CRERROR("Unexpected failure %s", strerror(errno) );
				return false;
			}
			break;
		}
		output << buffer;
	}
	CRDEBUG( CRinit, ("Setting TimesTen logging facility %s", 
		output.str().c_str() ) );
	return true;
}

// install TimesTen
void installNonNativeDB()
{
  char buf[10];
  std::string cmd = std::string("ls /SU/opt/") + \
		    std::string(TTNonNativeVersion)   + \
		    std::string(" > /dev/null");
  if ( system(cmd.c_str()) == 0 )
  {
     CRDEBUG( CRinit, ( "non-native TimesTen has installed") );
     return;
  }

  cmd = "/opt/config/bin/ChangeConfigDb -d | grep timesten | awk '{print $2}'";
  FILE *fp = popen( cmd.c_str(), "r" );
  if ( fp == 0 )
  {
    CRERROR( "Failed to get timesten installation information" );
  }
  else
  {
    if ( fgets( buf, 10, fp ) == 0 )
    {
       CRERROR( "Error to read the output of TimesTen installation information" );
       return;
    }
  }
  pclose( fp );
  buf[5]='\0';

  if ((strncmp(buf,"32bit",5) != 0) && (strncmp(buf,"64bit",5) != 0))
  {
	CRERROR( "Error to get TimesTen installation information");
	return;
  }

  cmd = std::string("cd /SU &&") + \
	std::string("tar -xvf /cs/sn/Packages/Addon/TTPackages.tar ") + \
	std::string("ChangeTTInstall.sh ") + \
	std::string(TTNonNativeVersion)+std::string(".")+std::string(buf)+std::string(".tar.gz");
  if (system(cmd.c_str()) != 0 )
  {
	CRERROR( "Error to untar TTPackages.tar");
	return;
  }

  cmd = std::string("cd /SU &&") + \
	std::string("gzcat ") + \
	std::string(TTNonNativeVersion)+std::string(".")+std::string(buf)+std::string(".tar.gz")+ \
	std::string(" | tar -xvf -");
  if ( system( cmd.c_str()) != 0 )
  {
	CRERROR( "Error to untar non-native TimesTen package");
	return;
  }

  cmd = std::string("cd /SU &&") + \
	std::string("/SU/ChangeTTInstall.sh /SU");
  if ( system( cmd.c_str()) != 0 )
  {
	CRERROR( "Error to modify TimesTen installation configure");
	return;
  }

  cmd = std::string("cd /SU &&") + \
	std::string("rm -rf ChangeTTInstall.sh ") + \
	std::string(TTNonNativeVersion)+std::string(".")+std::string(buf)+std::string(".tar.gz");
  if ( system( cmd.c_str()) != 0 )
  {
	CRERROR( "Error to remove temp file");
	return;
  }
}

void
checkLogAge()
{
	//check value of global varible if zero return
	if( g_currentMaxLogFileAge == 0 )
	{
		return;
	}

	int fd;
	int CLEAR = 0, ALARM = 1;
	static int sentFlag = CLEAR;

	struct stat fmtFileStat;
	static time_t CurrentTime;
	struct tm       *cLocalTime;

	const time_t DATEVALUE=(g_currentMaxLogFileAge * (24 * 60 * 60));

	CurrentTime = time( (time_t *)0 );
	cLocalTime = localtime( &CurrentTime );

	//loop and get all ttlog files
	DIR *fdir = NULL;
	struct dirent *dirPtr;

	char fileName[50];
	char fullFileName[150];
	char specProblem[150];
	fdir = opendir(LOG_DIRECTORY);
	String listOfOldFiles;
	listOfOldFiles.make_empty();

	if (fdir == NULL)
	{
  		CRDEBUG( CRinit, ( "COULD NOT OPEN %s", LOG_DIRECTORY ) );
		return;
	}

	while (( dirPtr = readdir(fdir)) != NULL)
	{
                fileName[0] = '\0';
 		sprintf(fileName,"%s", dirPtr->d_name);
                fileName[strlen(fileName)] = '\0';
 		sprintf(fullFileName,"%s/%s", LOG_DIRECTORY,dirPtr->d_name);

  		CRDEBUG( CRinit, ("TESTING FILE %s", fileName ) );

		if ((strncmp(fileName, "tt41data.log", 12)) == 0)
		{
		    if ((fd = open(fullFileName, 0)) != -1)
		    {
  			CRDEBUG(CRinit, ("OPENING FILE %s", fileName));

		     	if ((fstat(fd, &fmtFileStat)) == 0)
		  	{
			    if ((CurrentTime - fmtFileStat.st_mtime) > DATEVALUE) 
                            {
				//add to list
				if (!listOfOldFiles.is_empty())
				{
					listOfOldFiles = listOfOldFiles + ",";
				}
				listOfOldFiles = listOfOldFiles + fileName;
                            }
                     	}
                        close(fd);
                    }
		}
        }// end of while loop
        closedir(fdir);

	if(listOfOldFiles.is_empty())
	{
		if( sentFlag == ALARM )
		{
			//send clear
              		CRomdbMsg om;
              		om.add( g_currentMaxLogFileAge );
			sprintf(specProblem,
			"TimesTen Log Files are older than %d",
				g_currentMaxLogFileAge);
			om.setSpecificProblem(specProblem);

			om.setProbableCause(fileError);
			om.setAlarmType(processingErrorAlarm);
			om.setAlarmObjectName("TimesTen Log Files");
			om.setAdditionalText("");

              		om.spool( OM_CLEAR_OLD_LOGFILE );
			//set flag to clear
			sentFlag=CLEAR;
		}
	}
	else
	{
		//send alarm OM
              	CRomdbMsg om;
              	om.add( g_currentMaxLogFileAge );
              	om.add( listOfOldFiles );
		sprintf(specProblem,
		"TimesTen Log Files are older than %d",g_currentMaxLogFileAge);
		om.setSpecificProblem(specProblem);

		om.setProbableCause(fileError);
		om.setAlarmType(processingErrorAlarm);
		om.setAlarmObjectName("TimesTen Log Files");
		om.setAdditionalText("");

              	om.spool( OM_ALARM_OLD_LOGFILE );
		sentFlag=ALARM;
	}
}

#ifndef EES
#define MSG_LEN 2047

std::string getSQLError(const SQLHENV &env, const SQLHDBC &dbc)
{
  std::string str;
  UCHAR state[ MSG_LEN + 1 ];
  UCHAR errMsg[ MSG_LEN + 1 ];
  SDWORD nativeError;
  SWORD errMsgLen;
  errMsg[ MSG_LEN ] = 0;
  RETCODE rc = SQLError( env, dbc, SQL_NULL_HSTMT, state, &nativeError, errMsg,
                         MSG_LEN, &errMsgLen );
  if ( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
  {
    str = "SQLError failed, rc ";
    str += int_to_str(rc);
  }
  else
  {
    str = std::string(" State[") + (const char *)state;
    str += "] Message[";
    str += (const char *)errMsg;
    str += "] Vendor code [";
    str += int_to_str(nativeError);
    str += "]";
  }

  return str;
}
#endif
