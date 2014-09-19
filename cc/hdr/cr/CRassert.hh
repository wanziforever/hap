#ifndef	__CRASSERT_H
#define	__CRASSERT_H
/*
**      File ID:        @(#): <MID22292 () - 08/17/02, 28.1.1.1>
**
**	File:					MID22292
**	Release:				28.1.1.1
**	Date:					08/21/02
**	Time:					19:32:25
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the definition of the CRCFTASSERT macro
**      which is used to generate a CRAFT ASSERT.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
*/
#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/cr/CRmsg.hh"

/*
** The macro CRCFTASSERT formats and sends a CRAFT ASSERT output
** message to the CR subsystem. The message will be formatted and output 
** for the maintenance personnel to see.
**
** Arguments:
**    1. assert number (should be defined in subsystem-specific hdr file.
**                      The assert number ranges are defined 
**                      in hdr/GLasserts.h).
**    2. explanatory format string and variables
**
** The format string works the same as the C function printf(2).
**
** Note that an extra set of parentheses are required around the print-like
** format string and variable parameters, due to macro preprocessor
** limitations.
**
** Example:
** in cc/hdr/cr/CRassertIds.H
**     const int CRbadAlarmId = CRASSERTRNG+1;
**
**     #define CRbadAlarmMsg \
**               "CR_OUTMSG: INVALID ALARM_LVL VALUE\nKEY: MSGNAME=%s";
**
** in *.C file (note: 'omdbKey' is a character string)
**
**     CRCFTASSERT(CRbadAlarmId, (CRbadAlarmFmt, omdbKey));
**
** Preconditions:
**  1. The CRERRINIT macro must be called (just once) before the first 
**     CRCFTASSERT macro is called. 
**  2. The process should be attached to MSGH.
*/
extern CRmsg* CRcftAssertPtr();

extern mutex_t CRlockVarible;

extern const char *CRcftAsrtFmt;
#define CRCFTASSERT(assertId, assertFmt) { \
 	mutex_lock(&CRlockVarible); \
	CRcftAssertPtr()->storeKey(assertId, CROMCRCFTASSERT); \
        CRcftAssertPtr()->add(CRcftAsrtFmt, \
			 assertId, CRprocname, __FILE__, __LINE__) \
                        .spool assertFmt; \
        mutex_unlock(&CRlockVarible); \
     }


/*
** The macro CRASSERT formats and sends a regular ASSERT output
** message to the CR subsystem. The message will be formatted and output 
** for the maintenance personnel to see.  Regular asserts are meant to
** be used to report defensive check failures.
**
** Arguments:
**    1. assert number (should be defined in subsystem-specific hdr file.
**                      The assert number ranges are defined 
**                      in hdr/GLasserts.h).
**    2. explanatory format string and variables
**
** The format string works the same as the C function printf(2).
**
** Note that an extra set of parentheses are required around the print-like
** format string and variable parameters, due to macro preprocessor
** limitations.
**
** Example:
** in cc/hdr/cr/CRassertIds.H
**     const int CRbadSwitchId = CRASSERTRNG+25;
**
**     #define CRbadSwitchMsg \
**               "INVALID VALUE (%d) IN SWITCH STATEMENT"
**
** in *.C file (note: 'val' is a number)
**
**     CRASSERT(CRbadSwitchId, (CRbadSwitchFmt, val));
**
** Preconditions:
**  1. The CRERRINIT macro must be called (just once) before the first 
**     CRASSERT macro is called. 
**  2. The process should be attached to MSGH.
*/
extern CRmsg* CRassertPtr();

extern const char *CRasrtFmt;



#define CRASSERT(assertId, assertFmt) { \
	mutex_lock(&CRlockVarible); \
	CRmsg *_CR__msgPtr; \
	_CR__msgPtr = CRassertPtr(); \
	_CR__msgPtr->storeKey(assertId, CROMCRASSERT); \
	_CR__msgPtr->add(CRasrtFmt, \
	            assertId, CRprocname, __FILE__, __LINE__) \
	           .spool assertFmt; \
        mutex_unlock(&CRlockVarible); \
      }



#endif
