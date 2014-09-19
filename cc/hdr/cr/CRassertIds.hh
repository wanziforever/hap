#ifndef	__CRASSERTIDS_H
#define	__CRASSERTIDS_H
//
// DESCRIPTION:
//      This file contains the assert numbers (ids) and text strings
//      of the asserts used by the USLI (CR) subsystem.
//
// OWNER: 
//	Roger McKee
//
// NOTES:
//
#include "hdr/GLasserts.h"
#include "cc/hdr/cr/CRassert.hh"

/* CRmissingParmId: a parameter considered required by a CEP was not found
** on the command line.  This is probably due to an error in the IMDB or
** the CEP was invoked incorrectly through cepexec or UNIX.
** If the error was from cepexec being invoked improperly,
** invoke cepexec with the correct values.
** If the error is in the IMDB
** use a text editor to correct the IMDB file(s).  Then use INIT:PROC
** to restart the process that detected the error.
*/
const GLassertId CRmissingParmId = CRASSERTRNG+1;
#define CRmissingParmFmt "\
COMMAND EXECUTION PROGRAM IS MISSING A REQUIRED PARAMETER\n\
CEP=%s PARENT=%s IM=%s PARAMETER=%s"

/* CRnumNotFoundId: a CEP expected an optional numeric parameter, but found
** a non-numeric value for the parameter on its command line.
** This is probably due to an error in the IMDB or the CEP was invoked 
** incorrectly through cepexec or UNIX.
** If the error was from cepexec being invoked improperly,
** invoke cepexec with the correct values.
** If the error is in the IMDB
** use a text editor to correct the IMDB file(s).  Then use INIT:PROC
** to reset the process that detected the error.
*/
const GLassertId CRnumNotFoundId = CRASSERTRNG+2;
#define CRnumNotFoundFmt "\
INPUT MESSAGE EXPECTED PARAMETER\n\
CEP=%s PARENT=%s IM=%s PARAMETER=%s"

/* CRinterNaId: an interactive CEP was invoked by a USL parser or program
** that is only allowed to invoke non-interactive CEPs.
** This may be an input message scheduled through SCHED, invoked by ALARM
** as the result of a scan point firing, or the CEP was invoked 
** incorrectly through cepexec or UNIX.
** If the CEP was invoked through SCHED, use RCV form "TIMED SCHEDULING FORM"
** (2.7) to remove or inhibit the job.
** If the CEP was invoked through ALARM, modify the SQL table CR_ARUSP
** with the UNIX tool sqlplus. Then use OP:INIT to restart the ALARM process.
*/
const GLassertId CRinterNaId = CRASSERTRNG+3;
#define CRinterNaFmt "\
INTERACTIVE INPUT MESSAGE WAS INVOKED WITHOUT A USER TERMINAL\n\
CEP=%s PARENT=%s IM=%s"

/* CRbadBaudId: the CR_DEVICE table has an invalid baud rate in it.
** Valid baud values are: 19200, 9600, 4800, 2400, 1200, 300.
** Correct the SQL table CR_DEVICE with the UNIX tool sqlplus.
** Then use OP:INIT to restart the process that detected the error.
*/
const GLassertId CRbadBaudId = CRASSERTRNG+4;
#define CRbadBaudFmt "\
TABLE: CR_DEVICE: HAS INVALID BAUD RATE\n\
KEY: DEVICE=%s BAD VALUE: BAUD=%s"

/* CRbadParityId: the CR_DEVICE table has an invalid baud rate in it.
** Valid parity values are: NONE, EVEN, and ODD.
** Correct the SQL table CR_DEVICE with the UNIX tool sqlplus.
** Then use OP:INIT to restart the process that detected the error.
*/
const GLassertId CRbadParityId = CRASSERTRNG+5;
#define CRbadParityFmt "\
TABLE: CR_DEVICE: HAS INVALID PARITY\n\
KEY: DEVICE=%s BAD VALUE: PARITY=%s"

/* CRtooManyVarsId: a format string in the CR_OUTMSG table has more 
** variables than a client output message (CRomdbMsg) expected.
** This is probably caused by an error in the format string in the CR_OUTMSG.
** Use RCV "USLI OUTPUT MESSAGE DEFINITION FORM" (2.3) to correct the FORMAT 
** field.
*/
const GLassertId CRtooManyVarsId = CRASSERTRNG+6;
#define CRtooManyVarsFmt "\
TABLE: CR_OUTMSG: HAS MORE VARIABLES THAN EXPECTED\n\
KEY: MSGNAME=%s\n\
NUMBER OF VARIABLES IN CROMDBMSG=%d"

/* CRbadVarTypeId: the type of a variablein a format string in the CR_OUTMSG
** table does not match the type of the corresponding variable in a
** client output message (CRomdbMsg). 
** This is probably caused by an error in the format string in the CR_OUTMSG.
** Use RCV "USLI OUTPUT MESSAGE DEFINITION FORM" (2.3) to correct the FORMAT 
** field.
*/
const GLassertId CRbadVarTypeId = CRASSERTRNG+7;
#define CRbadVarTypeFmt "\
TABLE: CR_OUTMSG: TYPE OF VARIABLE DOES NOT MATCH\n\
KEY: MSGNAME=%s: FORMAT STRING VARIABLE NUMBER=%d TYPE=%s\n\
TYPE OF VARIABLE IN MSG=%s"

/* CRmissingImdbId: the IMDB file containing the list of input
** message groups could not be read.  This is probably due to the
** file not existing or having the wrong permissions.
** Set the permissions on the file to "rw rw rw  ainet ainet"
** using the UNIX commands below:
** (where $file is the name of the file)
**     chmod 666 $file
**     chgrp ainet $file
**     chown ainet $file
*/
const GLassertId CRmissingImdbId = CRASSERTRNG+8;
#define CRmissingImdbFmt "\
USLI PROCESS COULD NOT READ IMDB\n\
PROCESS=%s IMDB FILE=%s"

/* CRimdbSummaryId: the IMDB files contained one or more errors.
** Look for other error messages that indicate the specific errors
** in the IMDB file(s).  Then use a text editor to correct the file(s).
*/
const GLassertId CRimdbSummaryId = CRASSERTRNG+9;
#define CRimdbSummaryFmt "\
USLI PROCESS FOUND %d ERROR(S) IN IMDB"

/* CRparmNameLenId: a parameter name in an IMDB file is too long
** Use a text editor to shorten the parameter name everywhere
** it occurs in the IDMB files (in directory /sn/imdb).
*/
const GLassertId CRparmNameLenId = CRASSERTRNG+10;
#define CRparmNameLenFmt "\
IMDB ERROR, PARAMETER NAME IS TOO LONG\n\
PARAMETER NAME=%s MAX LEN=%d"

/* CRbadTermTypeId: The CR_DEVICE table has an invalid TERM value.
** Valid terminal types are: vt100, vt220, and 615.
** Correct the SQL table CR_DEVICE with the UNIX tool sqlplus.
** Then use OP:INIT to reinitialize the process that detected the error.
*/
const GLassertId CRbadTermTypeId = CRASSERTRNG+11;
#define CRbadTermTypeFmt "\
TABLE: CR_DEVICE: HAS INVALID TERM TYPE\n\
KEY: DEVICE=%s: TERM TYPE=%s\n\
USING TO TERM TYPE OF VT100"

/* CRbegGroupMissingId: The CR_OUTMSG table has a format string
** that has a "end group" format token '%)' without a matching
** previous "begin group" format token '%('.
** Use RCV "USLI OUTPUT MESSAGE DEFINITION FORM" (2.3) to correct the FORMAT 
** field.
*/
const GLassertId CRbegGroupMissingId = CRASSERTRNG+12;
#define CRbegGroupMissingFmt "\
TABLE: CR_OUTMSG: FORMAT STRING HAS UNEXPECTED END GROUP '%)'\n\
KEY: MSGNAME=%s: OMFORMAT='%s'"

/* CRmissingOmId: An output message was received that referenced
** a non-existant OMDB entry.
** Use RCV "USLI OUTPUT MESSAGE DEFINITION FORM" (2.3) to
** add the missing record.
*/
const GLassertId CRmissingOmId = CRASSERTRNG+13;
#define CRmissingOmFmt "\
TABLE: CR_OUTMSG: MISSING RECORD\n\
KEY: MSGNAME=%s:\n\
REQUESTING PROCESS=%s"

/* CRmissingMsgclassId: An output message was received by CSOP that referenced
** a non-existant message class.
** MSGNAME is the name of the CR_OUTMSG table key if the process
** that requested the output message used the OMDB.
** MSGNAME will have the value "CRmsg" if the process that requested
** the output message did not use the OMDB.
** Use RCV "USLI MESSAGE CLASS DEFINITION FORM" (2.2) to
** add the missing record.
*/
const GLassertId CRmissingMsgclassId = CRASSERTRNG+14;
#define CRmissingMsgclassFmt "\
TABLE: CR_MSGCLS: MISSING MSG CLASS\n\
KEY: MSGCLS=%s:\n\
REQUESTING PROCESS=%s MSGNAME=%s"

/* CRmissingCepId: A USLI processes attempted to execute an Command
** Execution Program (CEP, but failed due to the CEP not being
** executable or missing.
** Verify that the CEP exists and is executable by others.
** The CEP should be in the /sn/cep directory.
*/
const GLassertId CRmissingCepId = CRASSERTRNG+15;
#define CRmissingCepFmt "\
CEP MISSING OR NOT EXECUTABLE\n\
CEP=%s"

/* CRomTooManyVarsId: An output message was received that had more
** variables in it than the CR_OUTMSG format string.
** Use RCV "USLI OUTPUT MESSAGE DEFINITION FORM" (2.3) to correct the FORMAT 
** field.
*/
const GLassertId CRomTooManyVarsId = CRASSERTRNG+16;
#define CRomTooManyVarsFmt "\
TABLE: CR_OUTMSG: FORMAT STRING HAS FEWER VARIABLES THAN OUTPUT MSG\n\
KEY: MSGNAME=%s: OMFORMAT='%s'\n\
REQUESTING PROCESS=%s"

/* CRbadARUpriorityId: The CR_ARU or CR_ARUREL table was found to have a tuple
** with an invalid PRIORITY field. Legal values are from '1' - '16'.
** Correct the SQL table (CR_ARU or CR_ARUREL) with the UNIX tool sqlplus.
** Then use OP:INIT to restart the ALARM process.
*/
const GLassertId CRbadARUpriorityId = CRASSERTRNG+17;
#define CRbadARUpriorityFmt "\
TABLE: %s: INVALID ALARM_PRIORITY_N\n\
KEY: ALARM_PRIORITY_N=%s"

/* CRbadRelayId: The CR_ARUREL table was found to have a tuple
** with an invalid RELAY field.  Legal values are from '1' - '8'.
** Correct the SQL table with the UNIX tool sqlplus.
** Then use OP:INIT to restart the ALARM process.
*/
const GLassertId CRbadRelayId = CRASSERTRNG+18;
#define CRbadRelayFmt "\
TABLE: CR_ARUREL: INVALID RELAY\n\
KEY: ALARM_PRIORITY_N=%s BAD VALUE: RELAY=%s"

/* CRbadIntRateId: The CR_ARUREL table was found to have a tuple
** with an invalid INTERUPT_RATE field.  
** Legal values are: '120', '30', '60', 'STEADY'.
** Value of STEADY will be used by the system.
** Correct the SQL table with the UNIX tool sqlplus.
** Then use OP:INIT to restart the ALARM process.
*/
const GLassertId CRbadIntRateId = CRASSERTRNG+19;
#define CRbadIntRateFmt "\
TABLE: CR_ARUREL: INVALID INTERUPT_RATE\n\
KEY: ALARM_PRIORITY_N=%s BAD VALUE: INTERUPT_RATE=%s"

/* CRbadScanPtId: The CR_ARUSP table was found to have a tuple
** with an invalid field.
** If field NORMAL_STATE is invalid it default to LOW.
** If field LEVEL is invalid it default to MJ.
** Valid values for the SCAN_POINT_N field are: '1' - '?'.
** Valid values for the NORMAL_STATE field are: 'HIGH', 'LOW', 'ON', 'OFF'.
** Valid values for the ALARMLVL_V field are: 'CR', 'MJ', 'MN'.
** Correct the SQL table with the UNIX tool sqlplus.
** Then use OP:INIT to restart the ALARM process.
*/
const GLassertId CRbadScanPtId = CRASSERTRNG+20;
#define CRbadScanPtFmt "\
TABLE: CR_ARUSP: INVALID SCAN_POINT_N\n\
KEY: SCAN_POINT_N=%s BAD VALUE: %s=%s"

/* CRlogTmChgFailId: The timestamp on a logfile could not be changed.
** CSOP could not change the timestamp on a logfile to adjust for
** a SET:CLK input message that moved the system clock backwards.
** Action Required: verify the permissions on the file are 
**     "rw rw rw ainet ainet".  
** Correct if needed using the UNIX commands below
** (where $file is the name of the file):
**     chmod 666 $file
**     chgrp ainet $file
**     chown ainet $file
*/
const GLassertId CRlogTmChgFailId = CRASSERTRNG+21;
#define CRlogTmChgFailFmt "\
CSOP FAILED TO CHANGE THE TIME STAMP OF LOGFILE %s\n\
REASON=%s"

/* CRpermFailId: The device file for a USLI process could not be
** opened due to the file having the wrong permissions.
** It is also possible that one of the directories in the path
** have the wrong permissions.
** Action Required: use /bin/ls to verify that the permissions on the file 
** are:  "-rw --- --- ainet ainet".
** Correct if needed using the UNIX commands below
** (where $file is the name of the file):
**     chmod 600 $file
**     chgrp ainet $file
**     chown ainet $file
*/
const GLassertId CRpermFailId = CRASSERTRNG+22;
#define CRpermFailFmt "\
DEVICE FILE %s HAS WRONG PERMISSIONS"

/* CRindErrId: An error was detected in an indicator or display page file.
** Action Required: use a text editor to correct the error,
** then use INIT:PROC to reinitialize the process that detected the error.
*/
const GLassertId CRindErrId = CRASSERTRNG+23;
#define CRindErrFmt "\
ERROR IN FILE %s, LINE %d\n\
%s"

/* CRterseErrId: An error was detected in a USL parser, but it was in
** mode where it was only allowed to print "NG" to the current window.
** This assert prints the full text of the error message.
** The error is probably in a graphical display page definition file
** under /sn/display/sys/pages or in an IMDB file under /sn/imdb.
** Action Required: use a text editor to correct the error,
** then if the error was in an IMDB file use INIT:PROC to 
**      reinitialize the process that detected the error.
** else if the error was in a display page file, just enter the page number
**      again on the terminal that is displaying the page.
*/
const GLassertId CRterseErrId = CRASSERTRNG+24;
#define CRterseErrFmt "\
USL PARSER ERROR=%s"

/* CRvpathNotSetId: The VPATH UNIX environment variable was not set.
** This assert can only happen in the Execution Environment.
** Action Required: set the VPATH variable and restart the process
** that reported the error.
*/
const GLassertId CRvpathNotSetId = CRASSERTRNG+25;
#define CRvpathNotSetFmt "\
VPATH ENVIRONMENT VARIABLE NOT SET"

/* CRvpathTooLongId: The VPATH UNIX environment variable is too long.
** This assert can only happen in the Execution Environment.
** Action Required: set the VPATH variable and restart the process
** that reported the error.
*/
const GLassertId CRvpathTooLongId = CRASSERTRNG+26;
#define CRvpathTooLongFmt "\
VPATH ENVIRONMENT VARIABLE IS TOO LONG"

/* CRnoDateFormatFile: The date format file cannot be opened for reading.
** Action Required: use a text editor to establish the file with the
** proper date format and use INIT:PROC to reinitialize the process that
** detected the error.
*/
const GLassertId CRnoDateFormatFile = CRASSERTRNG+27;
#define CRnoDateFormatFileFmt "\
DATE FORMAT FILE %s NOT FOUND."

/* CRinvalidDateFormat: The date format is invalid. No formats in the
** date format file match any valid format.
** Action Required: use a text editor to change the date format to a valid
** format.
*/
const GLassertId CRinvalidDateFormat = CRASSERTRNG+28;
#define CRinvalidDateFormatFmt "\
%s IS AN INVALID DATE FORMAT."

/* CRnoLangEnvVar: The language (LC_TIME on CC, LC_TIME on EE) environment 
** variable is not set.
** Action Required: set language environment variable. E.g., "LC_TIME=psp_US;
** export LC_TIME" on EE, "LC_TIME=psp_usa_std;export LC_TIME" on CC.
*/
const GLassertId CRnoLangEnvVar = CRASSERTRNG+29;
#define CRnoLangEnvVarFmt "\
%s ENVIRONMENT VARIABLE IS NOT SET."

/* SYSTAT message queue is full.
** Action Required: init:proc=SYSTAT,level=0 to correct problem
**
*/
const GLassertId CRmsgQueueFull = CRASSERTRNG+30;
#define CRmsgQueueFullFmt "\
SYSTAT MESSAGE QUEUE IS FULL. %s UNABLE TO REPORT INDICATORS."


/* BACKUP setuid(0) failed.
** Action Required: Check permissions for backup executable 4700
*/
const GLassertId CRbkupNoRoot = CRASSERTRNG+31;
#define CRbkupNoRootFmt "\
BACKUP SETUID FAILED. RETURN CODE = %d."


/* BACKUP system() call failed.
** Action Required: Check if /Create_sn/bin/Backup* exists and has executable
** permissions.
*/
const GLassertId CRbkupSystem = CRASSERTRNG+32;
#define CRbkupSystemFmt "\
BACKUP SYSTEM CALL FAILED. RETURN CODE = %d."


/*
 * CRmsCallFailId: MS (messaging) library call failed.
 */
const GLassertId CRmsCallFailId = CRASSERTRNG + 33;
#define CRmsCallFailFmt "\
MS MESSAGE LIB CALL FAILURE, CODE %d (%d)\n\
%s"

/*
 * CRtmrSetFailId: Failed to set cyclic timer.
 */
const GLassertId CRtmrSetFailId = CRASSERTRNG + 34;
#define CRtmrSetFailFmt "\
FAILED TO SET CYCLIC TIMER FOR %s, CODE %d"


/*
 * CRthrUnexpDeathId: Unexpected death of thread
 */
const GLassertId CRthrUnexpDeathId = CRASSERTRNG + 35;
#define CRthrUnexpDeathFmt "\
UNEXPECTED DEATH OF THREAD"

/*
** CRMsgclassIdNull: An output message was received by CSOP that referenced
** a NULL message class.
*/
const GLassertId CRMsgclassIdNull = CRASSERTRNG+36;
#define CRMsgclassIdNullFmt "\
THE MSG CLASS FIELD IS NULL FOR THE ABOVE OR BELOW MESSAGE\n"

/*
** CRChkRawDiskId: Valotile disk partition has to be a block device, not a raw disk device 
*/
const GLassertId CRChkRawDiskId = CRASSERTRNG+37;
#define CRChkRawDiskIdFMT "\
VOLATILE TABLE DISK PARTITION HAS TO BE A BLOCK DEVICE, NOT A RAW DISK DEVICE(%s)"

/*
** CRChkOwnershipId: Ownership must be 'rwp' for disk partition
*/
const GLassertId CRChkOwnershipFailId = CRASSERTRNG+38;
#define CRChkOwnershipFailFMT "\
OWNERSHIP MUST BE 'rwp' FOR DEVICE (%s)"

/*
** CRChkDiskFailId: Disk partition must larger than specified in configuration
*/
const GLassertId CRChkDiskFailId = CRASSERTRNG+39;
#define  CRChkDiskFailFMT "\
RTDB DISK (%s) VERIFY FAIL, PARTITION SIZE(%ld) IS SMALLER THEN FILESIZE(%ld) "

/*
** CRNoCepFailId: No Cep Command at Active Side before Failover to LEAD
*/
const GLassertId CRNoCepFailId = CRASSERTRNG+40;
#define CRNoCepFailFMT "\
NO CEP COMMAND (%s) ALLOWED AT ACTIVE SIDE"

/*
** CRBadBrevityParmsId: Bad Brevity parms in /sn/init/initlist
** this is thus a place holder so that this ASSERT NUMBER is not
** used again. Please look at the CRomBrevityCtl.C module.
*/
const GLassertId CRBadBrevityParmsId = CRASSERTRNG+41;
#define CRBadBrevityParmsFMT "\
LOW BREVITY THRESHOLD (%d) IS EQUAL OR GREATER THEN\n HIGH BREVITY THRESHOLD (%d) FOR PROCESS (%s)"

/*
** Generic Null pointer format
**
*/
const GLassertId CRNullPointerId = CRASSERTRNG+42;
#define CRNullPointerFMT "\
SEGV: ATTEMPTING TO ACCESS NULL POINTER [%s]\n\
OTHER VARIABLE VALUES [%s]"

/*
** Make sure format is %s in form 2.3 for non-CRomdbMsg OMs
**
*/
const GLassertId CRbadFormatId = CRASSERTRNG+43;
#define CRbadFormat "\
OM KEY : %s FORMAT CAN ONLY BE %%s FOR THIS TYPE OF MESSAGE " 

/*
** A feature has x amount of time before its expire
**
*/
const GLassertId CRexpireFeatureId = CRASSERTRNG+44;
#define CRexpireFeature "\
Feature %s HAS ONLY %d DAYS TILL EXPIRATION  " 

/*
** Feature 65968 (R26 SU6) -
** Subsystem string in OMDB key is not found in /cs/sn/cr/CRsubsystemList.
** CSOP failed to map it to a 2-digit code when generating alarm code
*/
const GLassertId CRsubsystemMapFailureId = CRASSERTRNG+45;
#define CRsubsystemMapFailure "\
FAILED TO MAP SUBSYSTEM %s TO ITS CODE, USING DEFAULT '99' "

#endif
