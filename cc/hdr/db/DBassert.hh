
#ifndef __DBASSERT_H
#define __DBASSERT_H

/*
**	File ID:    @(#): <MID26040 () - 06/13/03, 27.1.1.1>
**
**	File:                        MID26040
**	Release:                     27.1.1.1
**	Date:                        07/15/03
**	Time:                        11:08:33
**	Newest applied delta:        06/13/03
**
**
** FILE DESCRIPTION:
**	This file contains the assert ids and text strings of
**	the asserts in the DB subsystem (specifically DBI,DBMON,
**	and the Transaction Manager) and its related libraries.
**
** OWNER:
**	eDB Team
**
** HISTORY:
**	Original version - C.A. Priddy
**	06/04/03 - Modified by Hon-Wing Cheng for Feature 61286
*/


#include "hdr/GLasserts.h"
#include "cc/hdr/cr/CRassert.hh"
#include "cc/hdr/cr/CRcftError.hh"

	/* The DBIGENLERROR and DBSGENLERROR
	 * macros are used to output general error information
	 * in the log prescribed for this purpose (debuglog).
	 * If in the future a new log is created with different macros, only
	 * these macros need changes to accomodate.  It also allows for new 
	 * debugging options that may be helpful for things such as to
	 * accomodate for CSOP throwing statements away (see DBCRAFASRT macro
	 * for example).
	 *
	 * The DBIGENCRFTASRT and DBSGENCRFTASRT are similar 
	 * execept that CRCFTASSERT is eventually
	 * called instead of CRERROR.
	 */
#define DBIGENLERROR( ERRORID, ERRORFMT )  CRERROR ERRORFMT;
#define DBIGENCRFTASRT( ERRORID, ERRORFMT )  CRCFTASSERT( ERRORID, ERRORFMT )

#define DBSGENLERROR( ERRORID, ERRORFMT )  CRERROR ERRORFMT;
#define DBSGENCRFTASRT( ERRORID, ERRORFMT )  CRCFTASSERT( ERRORID, ERRORFMT )

	/* # of asserts allocated to DB subsystem is 500 */

		/* range of asserts allocated to the DBI processes and
		 * Functionality
		 */
const GLassertId DB_DBI_AssertBase	= (DBASSERT + 0);
const GLassertId DB_DBI_MaxAssert	= (DB_DBI_AssertBase + 99);

		/* range of asserts allocated to the DBMON process and
		 * general Functionality
		 */
const GLassertId DB_DBMON_AssertBase	= (DBASSERT + 100);
const GLassertId DB_DBMON_MaxAssert	= (DB_DBMON_AssertBase + 99);

		/* range of asserts allocated to the Transaction Manager
		 * processes and general Functionality
		 */
const GLassertId DB_TRANMAN_AssertBase	= (DBASSERT + 200);
const GLassertId DB_TRANMAN_MaxAssert	= (DB_TRANMAN_AssertBase + 99);

		/* Range of asserts allocated to CEP and other 
		 * processes and general functionality.
		 */
const GLassertId DB_CEP_AssertBase    = (DBASSERT + 300);
const GLassertId DB_CEP_MaxAssert     = (DB_CEP_AssertBase + 99);


		/* range of asserts allocated to the DBS processes and
		 * Functionality
		 */
const GLassertId DB_DBS_AssertBase	= (DBASSERT + 400);
const GLassertId DB_DBS_MaxAssert	= (DB_DBS_AssertBase + 99);

 /**************************************************************************
  **************************************************************************
  ***************          DBI ASSERT DEFINITIONS        *******************
  **************************************************************************
  **************************************************************************
  */

const GLassertId DBmemoryOut = DB_DBI_AssertBase+0;
#define DBmemroyOutFmt \
"DBImain, Memory run out"

const GLassertId DBrcvmsgFail = DB_DBI_AssertBase+1;
#define DBrcvmsgFailFmt \
"DBIsqlMsg, MHmsgh.receive fail %d"


const GLassertId DBmsgNoExceed = DB_DBI_AssertBase+2;
#define DBmsgNoExceedFmt \
"DBmsgMsg, trouble, message number exceed"

const GLassertId DBhlprSick = DB_DBI_AssertBase+3;
#define DBhlprSickFmt \
"Failed to communicate with DB helper %s, error code %d"

const GLassertId DBsndhlprFail = DB_DBI_AssertBase+4;
#define DBsndhlprFailFmt \
"DBIsndHelper, send helper %d fail, err %d"

const GLassertId DBIsndSelAckFail = DB_DBI_AssertBase+5;
#define DBIsndSelAckFailFmt \
"DBIsndSeletAck: %s send fail, try too many times, rtn %d"

const GLassertId DBIrtnMsgFail = DB_DBI_AssertBase+6;
#define DBIrtnMsgFailFmt \
"DBIrtnMsg, send fail rtn code %d"

const GLassertId DBIrtnSelMsgFail = DB_DBI_AssertBase+7;
#define DBIrtnSelMsgFailFmt \
"DBIrtnSelMsg, send fail rtn code %d"

const GLassertId DBrgstMSGHFail = DB_DBI_AssertBase+8;
#define DBrgstMSGHFailFmt \
"DBI fail in register with MSGH"

const GLassertId DBconnORATry = DB_DBI_AssertBase+9;
#define DBconnORATryFmt \
"Warning, connect ORACLE fail, try again"

const GLassertId DBconnORAFail = DB_DBI_AssertBase+10;
#define DBconnORAFailFmt \
"connect ORACLE fail"

const GLassertId DBinitnodeFail = DB_DBI_AssertBase+11;
#define DBinitnodeFailFmt \
"DBI fail in initialization node messages"

const GLassertId DBHprSndRdyFail = DB_DBI_AssertBase+12;
#define DBHprSndRdyFailFmt \
"helper try to send ready message fail"

const GLassertId DBhprORAinitFail = DB_DBI_AssertBase+13;
#define DBhprORAinitFailFmt  \
"helper init ORACLE data structure fail"

const GLassertId DBhprMSGinitFail = DB_DBI_AssertBase+14;
#define DBhprMSGinitFailFmt  \
"helper fail in initialization message buffers"

const GLassertId DBsqldebug = DB_DBI_AssertBase+15;

const GLassertId DBImallocFail = DB_DBI_AssertBase+16;
#define DBImallocFailFmt \
"helper, DBIgetSel: malloc fail, run out of space!"

const GLassertId DBhprQRYinitFail = DB_DBI_AssertBase+17;
#define DBhprQRYinitFailFmt \
"DBInithpr, qry helper %s %d"

const GLassertId DBMONError = DB_DBI_AssertBase+18;

const GLassertId DBMONputenvHMFail = DB_DBI_AssertBase+19;
#define DBMONputenvHMFailFmt \
"Putenv ORACLE_HOME fail"

const GLassertId DBMONputenvSIDFail = DB_DBI_AssertBase+20;
#define DBMONputenvSIDFailFmt \
"Putenv ORACLE_SID fail"

const GLassertId DBMONrcvMsghFail = DB_DBI_AssertBase+21;
#define DBMONrcvMsghFailFmt \
"MHmsgh.receive message fail %d, GLsuccess %d"

const GLassertId DBopenNullFail = DB_DBI_AssertBase+22;
#define DBopenNullFailFmt \
"Open /dev/null fail, exiting"

const GLassertId DBdup0callFail = DB_DBI_AssertBase+23;
#define DBdup0callFailFmt \
"Dup2 to 0  call fail, exit"

const GLassertId DBdup1callFail = DB_DBI_AssertBase+24;
#define DBdup1callFailFmt \
"Dup2 to 1  call fail, exit"

const GLassertId DBexecOraCmdFail = DB_DBI_AssertBase+25;
#define DBexecOraCmdFailFmt \
"exec getOraCmd fail"

const GLassertId DBoraDead = DB_DBI_AssertBase+26;
#define DBoraDeadFmt \
"ORACLE process: %s is dead"

const GLassertId DBmanInsertFail = DB_DBI_AssertBase+27;
#define DBmanInsertFailFmt \
"DBman::DBman insert of table '%s' failed %d"

const GLassertId DBmanNotable = DB_DBI_AssertBase+28;
#define DBmanNotableFmt \
"DBman::selectDataWait can't find table name %s, cond = %s"

const GLassertId DBmanSelectFail = DB_DBI_AssertBase+29;
#define DBmanSelectFailFmt \
"Select of table %s where '%s' failed, retval %d, explain:\n%s"

const GLassertId DBmanSelWhereFail = DB_DBI_AssertBase+30;
#define DBmanSelWhereFailFmt \
"Select of table %s where '%s' failed, retval %d"

const GLassertId DBmanSeltblNoRgst = DB_DBI_AssertBase+31;
#define DBmanSeltblNoRgstFmt \
"Select on tbl %s not registered."

const GLassertId DBmanTwoSelAttemp = DB_DBI_AssertBase+32;
#define DBmanTwoSelAttempFmt \
"Two select attempts on one table %s"

const GLassertId DBmanNotfindDBI = DB_DBI_AssertBase+33;
#define DBmanNotfindDBIFmt \
"can't find database name `%s', retval %d"

const GLassertId DBmanWhereTooLong = DB_DBI_AssertBase+34;
#define DBmanWhereTooLongFmt \
"select * from %s where %s string too long"

const GLassertId DBmanSendDBFail = DB_DBI_AssertBase+35;
#define DBmanSendDBFailFmt \
"Can't send Select message to DB %s, ret %d"

const GLassertId DBmanWrongSelAck = DB_DBI_AssertBase+36;
#define DBmanWrongSelAckFmt \
"Select Ack message sid = %d, not recognized."

const GLassertId DBmanNVpairWrong = DB_DBI_AssertBase+37;
#define DBmanNVpairWrongFmt \
"DBman::dbSelAckMsg: nv pair not complete, aborting"

const GLassertId DBmanduplicateFld = DB_DBI_AssertBase+38;
#define DBmanduplicateFldFmt \
"DBman::dbSelAckMsg: %d contains duplicate field %s"

const GLassertId DBmanFldTooLong = DB_DBI_AssertBase+39;
#define DBmanFldTooLongFmt \
"DBman::dbSelAckMsg: %d value of field %s too long, %d > %d"

const GLassertId DBmanTablePairWrong = DB_DBI_AssertBase+40;
#define DBmanTablePairWrongFmt \
"DBman::dbFmMsg: tablename pair not complete, ignoring"

const GLassertId DBmanFirstRcdWrong = DB_DBI_AssertBase+41;
#define DBmanFirstRcdWrongFmt \
"DBman::dbFmMsg: DBfmMsg message rec'd first value not 0, %d %s"

const GLassertId DBmanUnknownTbl = DB_DBI_AssertBase+42;
#define DBmanUnknownTblFmt \
"DBman::dbFmMsg: don't know table %s"

const GLassertId DBmanNullPair = DB_DBI_AssertBase+43;
#define DBmanNullPairFmt \
"DBman::dbFmMsg: got null pair with %d pairs left in %d message!"

const GLassertId DBmanNVPairNotCmplet = DB_DBI_AssertBase+44;
#define DBmanNVPairNotCmpletFmt \
"DBman::dbFmMsg: nv pair not complete, aborting"

const GLassertId DBmanDuplField = DB_DBI_AssertBase+45;
#define DBmanDuplFieldFmt \
"DBman::dbFmMsg: %d contains duplicate field %s"

const GLassertId DBmanFldValueTooLong = DB_DBI_AssertBase+46;
#define DBmanFldValueTooLongFmt \
"DBman::dbFmMsg: %d value of field %s too long, %d > %d"

const GLassertId DBmanRecordErr = DB_DBI_AssertBase+47;
#define DBmanRecordErrFmt \
"Error'ed record, type = %d:"

const GLassertId DBprimRegNameFail = DB_DBI_AssertBase+48;
#define DBprimRegNAmeFailFmt \
"regName(%s) failed, retval %d"

const GLassertId DBprimNoDBIName = DB_DBI_AssertBase+49;
#define DBprimNoDBINameFmt \
"Cannot find DBM process name `%s' in MSGH table, retval %d\n"

const GLassertId DBprimRmNameFail = DB_DBI_AssertBase+50;
#define DBprimRmNameFailFmt \
"rmName(%s, %s) failed, retval %d"

const GLassertId DBprimSendDBFail = DB_DBI_AssertBase+51;
#define DBprimSendDBFailFmt \
"Can't send Select message to DB %s, ret %d\n"

const GLassertId DBprimSelAckFail = DB_DBI_AssertBase+52;
#define DBprimSelAckFailFmt \
"select ACK rcv failed %d\n"

const GLassertId DBprimInvalidMsg = DB_DBI_AssertBase+53;
#define DBprimInvalidMsgFmt \
"Invalid message received, type %d from %d\n"

const GLassertId DBprimSelAckTooShort = DB_DBI_AssertBase+54;
#define DBprimSelAckTooShortFmt \
"Select ACK message too short, %d < %d, ignoring\n"

const GLassertId DBprimSelAckInvalid = DB_DBI_AssertBase+55;
#define DBprimSelAckInvalidFmt \
"Select ACK invalid sid, %d != %d, ignoring\n"

const GLassertId DBprimSelFail = DB_DBI_AssertBase+56;
#define DBprimSelFailFmt \
"Select failed %d\n"

const GLassertId DBprimNVpairBAD = DB_DBI_AssertBase+57;
#define DBprimNVpairBADFmt \
"Select name-value pair not complete, ignoring\n"

const GLassertId DBprimSelAckDuplicate = DB_DBI_AssertBase+58;
#define DBprimSelAckDuplicateFmt \
"Select ACK contains duplicate field %s\n"

const GLassertId DBprimValueTooLong = DB_DBI_AssertBase+59;
#define DBprimValueTooLongFmt \
"Select ACK value of field %s too long, %d > %d\n"

const GLassertId DBconnPlatDbTry = DB_DBI_AssertBase+60;
#define DBconnPlatDbTryFmt \
"Warning: Connect to platform database failed. Try again"

const GLassertId DBconnPlatDbFail = DB_DBI_AssertBase+61;
#define DBconnPlatDbFailFmt \
"Connect to platform database failed"

const GLassertId DBplatDbDown = DB_DBI_AssertBase+62;
#define DBplatDbDownFmt \
"Platform database is down"

const GLassertId DBconnDbTry = DB_DBI_AssertBase+63;
#define DBconnDbTryFmt \
"Warning: Connect to database failed. Try again"

const GLassertId DBconnDbFail = DB_DBI_AssertBase+64;
#define DBconnDbFailFmt \
"Connect to database failed"

const GLassertId DBdbDown = DB_DBI_AssertBase+65;
#define DBdbDownFmt \
"Database is down"

 /**************************************************************************
  **************************************************************************
  ***************          DBMON ASSERT DEFINITIONS        *****************
  **************************************************************************
  **************************************************************************
  */


 /**************************************************************************
  **************************************************************************
  ***************       TRANMAN ASSERT DEFINITIONS        ******************
  **************************************************************************
  **************************************************************************
  */

	/* The Transaction Manager communicates via a set of private MSGH
	 * Queues.  This assert indicates that an attempt to create one of
	 * these queues failed because the process could not attach to
	 * the MSGH subsystem via the attach() MSGH library member function.
	 * This error indicates a global system initialization synchronization
	 * failure. 
	 * Actions To Take :
	 * 	Check if the MSGH process is alive and if not then execute
	 *	the INIT:PROC command.
	 */
const GLassertId DBmsghAttachFailed = DB_TRANMAN_AssertBase + 0;
#define DBmsghAttachFailedFmt \
		" Failed to attach to msgh when creating DBM message queue error=%d\n"

	/*
	 * MSGH could not find a message queue name or queue Id of the
	 * Transaction Manager DBopSchedule process (queue name AINETDB).
	 * Actions To Take :
	 *	Verify the AINETDB process is still alive (using the ps UNIX
	 *	command).  If not, execute the INIT:PROC MML input command to
	 *	re-initialize this process.
	 */
const GLassertId DBbadAINETDBqueue = DB_TRANMAN_AssertBase + 1;
#define DBbadAINETDBqueueFmt \
	" Couldn't find AIN DBM Scheduler Queue Id for Queue Name=%s - error=%d\n"

	/* 
	 * A process attempted send an operation to a remote DBopServer 
	 * process (with the indicated output queue name).  This attempt
	 * failed with the indicated error return code.
	 * Actions To Take :
	 *	Determine whether the indicated process should exist and
	 *	if it should but is not currently alive, re-initialize the
	 *	process (using INIT:PROC if it is INIT subsystem controlled).
	 */
const GLassertId DBbadServerQueue = DB_TRANMAN_AssertBase + 2;
#define DBbadServerQueueFmt \
		" Failed to Find Server Queue name=%s - error=%d\n"

	/*
	 * An error occurred during the second phase of a transaction commit.
	 * This error indicates that data inconsistencies or data corruption
	 * MAY exist in the system.
	 * Actions To Take :
	 *	If the error occurred during a Recent Change,
	 *		then execute an update recent change for the same
	 *		form with the same data in hopes that all the processes
	 *		involved will be made consistent.
	 *	If the error did not happen through Recent Change, retry
	 *		whatever command initiated the error in hopes that
	 *		the same data updates will occur a second time.
	 *
	 *	If the above do not work, then use the error and debug log files
	 *		to try and determine which processes were involved in
	 *		the transaction.  These processes may need to be 
	 *		reinitialized or the SNdbEdit tool used to hand modify
	 *		the data in whatever process is inconsistent.
	 *		Before attempting either of these actions, seek 
	 *		immediate technical help, otherwise unneccessary
	 *		downtime may be caused or extended in the involved
	 *		processes.
	 */
const GLassertId DB2ndPhaseCommitFail = DB_TRANMAN_AssertBase + 3;
#define DB2ndPhaseCommitFailFmt \
	" Failure occurred during the 2nd Phase of a Transaction Commit\n\
	Error Return=[%d]\n\
	Error String=[%s]\n"

	/*
	 * An error occurred during a transaction's backout.
	 * This assert MAY indicate data inconsistencies and/or data corruption
	 * may exist in the system.
	 * Actions To Take :
	 *	If the error occurred during a Recent Change,
	 *		then execute an update recent change for the same
	 *		form with the same data in hopes that all the processes
	 *		involved will be made consistent.
	 *	If the error did not happen through Recent Change, retry
	 *		whatever command initiated the error in hopes that
	 *		the same data updates will occur a second time.
	 *
	 *	If the above do not work, then use the error and debug log files
	 *		to try and determine which processes were involved in
	 *		the transaction.  These processes may need to be 
	 *		reinitialized or the SNdbEdit tool used to hand modify
	 *		the data in whatever process is inconsistent.
	 *		Before attempting either of these actions, seek 
	 *		immediate technical help, otherwise unneccessary
	 *		downtime may be caused or extended in the involved
	 *		processes.
	 */
const GLassertId DBbackoutFailure = DB_TRANMAN_AssertBase + 4;
#define DBbackoutFailureFmt \
	" A failure occurred during the backout of a transaction\n\
	Error Return Value=[%d] \n\
	Error String=[%s] \n"

	/* The Transaction Manager DBopControl process was unable to establish
	 * 	a connection to the Oracle Database Manager process(es) for
	 *	the indicated reasons.
	 *	This usually indicates that Oracle is shutdown or insane.
	 * Actions To Take :
	 *	Contact the database administrator and appropriate technical
	 *	support to analyze the condition of Oracle and take
	 *	any appropriate steps to recover the Oracle database.
	 */
const GLassertId DBtranOracleConnect = DB_TRANMAN_AssertBase + 5;
#define DBtranOracleConnectFmt \
	" The Transaction Manager Failed to establish a connection to Oracle\n\
	Error Return=[%d] \n\
	Error String=[%s] \n"


	/*
	 * During the second phase commit of a transaction, the commit of
	 * of the changes (if any) made to Oracle failed.
	 * This assert indicates that data inconsistencies MAY exist on other
	 * processes involved in the same transaction.
	 * Actions To Take :
	 *	If the error occurred during a Recent Change,
	 *		then execute an update recent change for the same
	 *		form with the same data in hopes that all the processes
	 *		involved will be made consistent.
	 *	If the error did not happen through Recent Change, retry
	 *		whatever command initiated the error in hopes that
	 *		the same data updates will occur a second time.
	 *
	 *	If the above do not work, then use the error and debug log files
	 *		to try and determine which processes were involved in
	 *		the transaction.  These processes may need to be 
	 *		reinitialized or the SNdbEdit tool used to hand modify
	 *		the data in whatever process is inconsistent.
	 *		Before attempting either of these actions, seek 
	 *		immediate technical help, otherwise unneccessary
	 *		downtime may be caused or extended in the involved
	 *		processes.
	 */
const GLassertId DBdbCommitFailed = DB_TRANMAN_AssertBase + 6;
#define DBdbCommitFailedFmt \
	" Phase 2 Commit of database data failed - error=%d \n"


	/*
	 * This assert indicates that data was unable to be committed to
	 * the Oracle RDBMS because the connection to Oracle is no longer
	 * valid.
	 * Actions To Take :
	 *	Contact appropriate technical support to determine if
	 *	the Oracle DBMS is operating correctly and take any 
	 *	appropriate actions necessary.
	 *	After Oracle is running properly, re-execute the operation
	 *	once again.
	 */
const GLassertId DBoracleConnectBad = DB_TRANMAN_AssertBase + 7;
#define DBoracleConnectBadFmt \
	" Attempted to commit changes when Oracle connection does not exist\n"

	/* The transaction manager scheduler process (AINETDB) could not find
	 * the executeable file for a DBopControl process.
	 * Actions To Take :
	 *	Recover the indicated file from a previous system backup.
	 */
const GLassertId DBcontrolFileMissing = DB_TRANMAN_AssertBase + 8;
#define DBcontrolFileMissingFmt \
	" Could not find DBopControl file %s\n    child process is exiting"

	/* A backout operation failed in attempting to forward messages to
	 * a server process.
	 * Actions To Take :
	 *	Determine if the process' MSGH queue temporarily overflowed
	 *	and was unable to accept a message.  This can be determined
	 *	by looking at the error returned by MSGH.  If this is the
	 *	case then a data inconsistency may exist between that process
	 *	and the other processes involved in the same transaction.
	 *	If a data inconsistency is suspected, then once the cause
	 *	of the queue overflow has been removed, retry the transaction
	 *	or initialize the process off of the Oracle backup database.
	 */
const GLassertId DBbkoutServerFailed = DB_TRANMAN_AssertBase + 9;
#define DBbkoutServerFailedFmt \
		" Failed to send backout message to Server Message Queue ID=%s error=%d\n"




 /**************************************************************************
  **************************************************************************
  ***************       CEP ASSERT DEFINITIONS            ******************
  **************************************************************************
  **************************************************************************
  */

	/*
	 *	This assert is issued by the CEP BACKUP_ORACLE
	 *	in the event that the system() call to start the
	 *	bkupordbcron script fails.
	 *	Actions to take:
	 *		Check the permissions and ownership of
	 *		the bkupordbcron script.  It must be owned
	 *		by root and must be marked as executable.
	 *		If this is not true, the System Administrator
	 *		must change the erroneous setting.  If this
	 *		is true, check the Operating System documentation
	 *		to determine the meaning of the return code that
	 *		is displayed.
	 */
		
const GLassertId DB_BKUP_SysCallFailed = DB_CEP_AssertBase + 0;
#define DB_BKUP_SysCallFailedFmt \
"BACKUP_ORACLE: Call to system() failed with return value = %d,\n%s"

	/*
	 *	This assert is issued by the CEP DELETE_ORACLE
	 *	in the event that the system() call to start the
	 *	bkupordbcron script fails.
	 *	Actions to take:
	 *		Check the permissions and ownership of
	 *		the bkupordbcron script.  It must be owned
	 *		by root and must be marked as executable.
	 *		If this is not true, the System Administrator
	 *		must change the erroneous setting.  If this
	 *		is true, check the Operating System documentation
	 *		to determine the meaning of the return code that
	 *		is displayed.
	 */
		
const GLassertId DB_DEL_SysCallFailed = DB_CEP_AssertBase + 1;
#define DB_DEL_SysCallFailedFmt \
"DELETE_ORACLE: Call to system() failed with return value = %d"


	/*
	 *	This assert is issued by the CEP BACKUP_TAPE
	 *	in the event that the system() call to start the
	 *	Backuptape script fails.
	 *	Actions to take:
	 *		Check the permissions and ownership of
	 *		the /opt/config/bin/Backuptape script.
	 *              It must be owned by root and must be executable.
	 *		If this is not true, the System Administrator
	 *		must change the erroneous setting.  If this
	 *		is true, check the Operating System documentation
	 *		to determine the meaning of the return code that
	 *		is displayed.
	 */
		
const GLassertId DB_BKUPTAPE_SysCallFailed = DB_CEP_AssertBase + 2;
#define DB_BKUPTAPE_SysCallFailedFmt \
"BACKUP_TAPE: Call to system() failed with return value = %d,\n%s"

 /**************************************************************************
  **************************************************************************
  ***************          DBS ASSERT DEFINITIONS        *******************
  **************************************************************************
  **************************************************************************
  */
const GLassertId DBSsndSelAckFail = DB_DBS_AssertBase+5;
#define DBSsndSelAckFailFmt \
"DBSsndSeletAck: %s send fail, try too many times, rtn %d"

const GLassertId DBSrtnMsgFail = DB_DBS_AssertBase+6;
#define DBSrtnMsgFailFmt \
"DBSrtnMsg: send fail rtn code %d"

const GLassertId DBSrtnSelMsgFail = DB_DBS_AssertBase+7;
#define DBSrtnSelMsgFailFmt \
"DBSrtnSelMsg: send fail rtn code %d"

const GLassertId DBSqueremovefail = DB_DBS_AssertBase+8;
#define DBSqueremovefailFmt \
"DBScleanup: DBS is trying to detach from global data object for large queue rtn code %d"

const GLassertId DBSqueremovemhq = DB_DBS_AssertBase+9;
#define DBSqueremovemhqFmt \
"DBScleanup: DBS is trying to detach from message H queue rtn code %d"

const GLassertId DBSmsghRegNameFail = DB_DBS_AssertBase+10;
#define DBSmsghRegNameFailFmt \
"DBSprocinit: failed to register name(%s) with MSGH, error=%d"

const GLassertId DBSgdoAttachFail = DB_DBS_AssertBase+11;
#define DBSgdoAttachFailFmt \
"DBSprocinit: failed to create or attach to Global data object for DBS Queue, error=%d"

const GLassertId DBSloginEnvFail = DB_DBS_AssertBase+12;
#define DBSloginEnvFailFmt \
"DBSprocinit: failed to obtain any Environment for Oracle, error=%d"

#endif  /* DBASSERT */
