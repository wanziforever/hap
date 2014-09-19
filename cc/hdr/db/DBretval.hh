
#ifndef __DBRETVAL_H
#define __DBRETVAL_H

/*
**	File ID:    @(#): <MID22142 () - 06/20/03, 27.1.1.1>
**
**	File:                        MID22142
**	Release:                     27.1.1.1
**	Date:                        07/15/03
**	Time:                        11:08:38
**	Newest applied delta:        06/20/03
**
**
** FILE DESCRIPTION:
**	Contains the values of all DB primitive function return codes
**	and a short description of each.
**
** OWNER:
**	C.A. Priddy
*/

#include "hdr/GLreturns.h"

	// <0 == failure value
	// 0  == general success
	// >0 == success with addtl information

typedef enum
{
	DBPLATDBFAILURE = (DBOP_FAIL - 20), /* an error occurred during the
					     * interface with the platform RDBMS
					     * or psql class library
					     */
	DBDRIVERPROCDEATH = (DBOP_FAIL - 19),  /* during an operation or 
						* transaction the death of the
						* driver process was detected
						*/
	DBSERVERPROCDEATH = (DBOP_FAIL - 18),  /* during an operation or 
						* transaction the death of one
						* or more server process(es)
						* was detected
						*/
	DBORACLEFAILURE = (DBOP_FAIL - 17), /* an error occurred during the
					     * interface with the Oracle RDBMS
					     * and SQL++ class library
					     */
	DBINCONSISTENT = (DBOP_FAIL - 16), /* a commit or backout of a
					    * transaction was unable to
					    * guarantee the same result on
					    * all processes involved in the
					    * transaction. A return code of
					    * this indicates that the processes
					    * involved in this transaction may
					    * have inconsistent/dependent data
					    * values.
					    */
	DBOPABORTED  = (DBOP_FAIL - 15), /* An operation or transaction was 
					  * aborted 
					  */
	DBEVENTFAILURE = (DBOP_FAIL - 14), /* an error occurred in attempting to
					    * set an event (timer)
					    */
	DBBADTRANSTATE = (DBOP_FAIL - 13),  /* either the current state of a
					     * transaction is invalid or is
					     * inconsistent with the operation
					     * being executed
					     */
	DBTRANTIMEOUT = (DBOP_FAIL - 12), /* a transaction execution timed out*/
	DBOPTIMEOUT = (DBOP_FAIL - 11), /* an individual operation timed out */
	DBINVALIDINPUT = (DBOP_FAIL - 10), /* member function input parameter
					    * was provided with illegal value
				 	    */
	DBCONSTRUCTORFAILED = (DBOP_FAIL - 9), /* object's constructor failed */
	DBNOMESSAGEFOUND = (DBOP_FAIL - 8), /* a request for a message failed 
				  	     * because the message queue did
					     * not contain the requested type
					     * of messages
				  	     */
	DBNEWFAILED = (DBOP_FAIL - 7),  /* a call to new x() failed */
	DBMSGQFULL = (DBOP_FAIL - 6),  /* failed message send to a destination
					* because its MSGH queue is full.
					*/
	DBBADMSGFORMAT = (DBOP_FAIL - 5), /* A message was found to contain an
					   * invalid format 
					   */
	
	DBBADMSGSEQUENCE = (DBOP_FAIL - 4),    /* A message was presented with
						* an invalid sequence number 
						* that was unexpected
						*/
	DBMSGQERROR = (DBOP_FAIL - 3), /* An error occurred involving a message
					* Queue (eg. a Qid for a name doesnt
					* exist)
					*/
	DBRESOURCE = (DBOP_FAIL - 2),  /* Not due to code failure -- indicates
			     		* some kind of resource (eg. memory,
					* message queue filled, ..etc.) could
			     		* not be allocated or de-allocated
					* properly
					*/
	DBFAILURE = (DBOP_FAIL - 1),	/* General DB failure */
	DBSUCCESS = 0,		/* General DB Success value */

	DBSELECT_LAST_TUP = 1, // Last tuple of a set being returned from
			       //  a read/select primitive
	DBAPPLY_COMMITTED = 2,	// a non-transaction operation application was
				// applied and committed in a single step by
				// the server process
	DBDELAYACK = 3,		// a function's acknowledgement will be
				// asynchronously supplied to the DBtranMan
				// object in the future, even though it is
				// returning at this point in time
	DBMSGCOMPLETE = 4,   // the last of a sequence of messages has been
			     //   sent or received
	DBMSGBUFFERED = 5,	/* the last message could not be sent
				 * presumeabley because the destination is full.
				 * However a copy of the message was created
				 * to allow for further retry attempts
				 */
	DBPARTIALMSG = 6,     /* message is partially completed */
	DBENDOFMESSAGES = 7,     /* A termination of message set message was
				 * received
				 */
	DBNOOP = 8,	/* ignore type return type */
	DBNOMATCH = 9,	  /* A read operation against Oracle select 0 tuples 
			   * to return -- the user must decide whether this is
			   * an error or not depending on their own application
			   */
	DBCONTINUE = 10, /* continue processing */
	DBSTOP = 11,  /* stop processing */
	DBPREVBACKEDOUT = 12,     /* A transaction backout request was
				   * received for a transaction that has either
				   * not been started or has already completed
			      	   */
	DBCHECKPROCALIVE = 13,  /* test status of transaction member procIds */
        DBTOOMANYTUPFOUND = 14, /* return val for the RCV query when number
				 * of tuples found exceed maximum.
				 */
	DBLASTRETVAL		/* unused */
} DBRETVAL;

	/* macro returns TRUE==YES iff the DBRETVAL input parameter
	 * indicates a failure occurred
	 */
#define  DBERROR( DBRVAL ) (((DBRVAL) < DBSUCCESS) ? YES : NO )

	/* macro returns TRUE==YES iff the DBRETVAL input parameter
	 * indicates a type of successful return
	 */
#define  DBOK( DBRVAL ) (((DBRVAL) >= DBSUCCESS) ? YES : NO )

	/* macro returns TRUE==YES iff the DBRETVAL input parameter indicates
	 * an Oracle specific error range.
	 */
#define	 DBORACLEERROR( DBRVAL ) ((((DBRVAL) > (DBOP_FAIL)) && ((DBRVAL) < 0)) ? YES : NO )

#endif  /* DBRETVAL */
