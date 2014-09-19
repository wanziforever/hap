#ifndef __DBMTYPE_H
#define __DBMTYPE_H

/*
**
**	File ID:    @(#): <MID7055 () - 06/11/03, 29.1.1.1>
**
**	File:                        MID7055
**	Release:                     29.1.1.1
**	Date:                        07/15/03
**	Time:                        11:08:35
**	Newest applied delta:        06/11/03
**
** DESCRIPTION:
**	Definition of all DBI-related messages types.
**	Allowed sql size in a message and data size in DBselectAck
**
** OWNER:
**	eDB Team
**
** HISTORY:
**      Original version - Yeou H. Hwang
**      06/04/03 - Modified by Hon-Wing Cheng for Feature 61286
**
** NOTES:
*/

#include <sys/types.h>
#include "hdr/GLmsgs.h"

/* Message to the DBI */
#define DBselectTyp  DBMSGBASE		/*  Select message */
#define DBinsertTyp  DBMSGBASE+1	/*  Insert message */
#define DBupdateTyp  DBMSGBASE+2	/*  Update message */
#define DBdeleteTyp  DBMSGBASE+3	/*  Delete message */
#define DBabortSelectTyp  DBMSGBASE+4   /*  Abort select message */

/* Message from the DBI */
#define DBselectAckTyp  DBMSGBASE+10	/*  Ack to select msg */
#define DBinsertAckTyp  DBMSGBASE+11 	/*  Ack to insert msg */
#define DBupdateAckTyp  DBMSGBASE+12 	/*  Ack to update msg */
#define DBdeleteAckTyp  DBMSGBASE+13 	/*  Ack to delete msg */
#define DBackBackTyp  DBMSGBASE+14 	/*  Ack to reqAck msg */

/* Messages from FORMS */
#define DBfmInsertTyp  DBMSGBASE+20	/*  Insert message */
#define DBfmUpdateTyp  DBMSGBASE+21	/*  Update message */
#define DBfmDeleteTyp  DBMSGBASE+22	/*  Delete message */
#define DBfmLockTyp    (DBMSGBASE+23)	/*  Lock message */
#define DBfmLockCancelTyp (DBMSGBASE+24) /*  Lock Cancel message */

/* Messages to FORMS */
#define DBfmInsertAckTyp  DBMSGBASE+30	/*  Insert message */
#define DBfmUpdateAckTyp  DBMSGBASE+31	/*  Update message */
#define DBfmDeleteAckTyp  DBMSGBASE+32	/*  Delete message */
#define DBfmLockIPTyp     (DBMSGBASE+33) /*  Lock In Progress message */
#define DBfmLockAckTyp    (DBMSGBASE+34) /*  Lock Ack message */

/* Messages used between DBI and DBIhelpers */
#define DBIsyncMsgTyp     DBMSGBASE+40 /* DBI sync Message */
#define DBIhprMsgTyp   DBMSGBASE+41 /* DBI helper ready message */
#define DBIqryStatMsgTyp     DBMSGBASE+42 /* DBI query status Message */
#define DBIhrtBeatRtnMsgTyp   DBMSGBASE+43 /* DBI heart return message */

/* Messages used between DB driver processes and the DBopSchedule processes
 *	when a transaction is NOT being used
 */
#define DBOPDRIVETyp	DBMSGBASE+50 /* Non-Transaction Driver Message */
#define DBEOTUPLES	DBMSGBASE+51 /* Last of a set of tuple message */
#define DBPRIMITIVEMSG	DBMSGBASE+52 /* DB primitive specification (portion) */
#define DBACKOPER	DBMSGBASE+53 /* DB primitive acknowledgment message */
#define DBACKCONTINUE	DBMSGBASE+54 /* send remainder of message sequence */
#define DBACKSTOP	DBMSGBASE+55 /* terminate remaining message sequence */

/* Messages used between DB driver processes and the DBopSchedule processes
 *	when a transaction IS being used
 */
#define DBTRANOPDRIVETyp DBMSGBASE+60 /* Transaction Driver Message */
#define DBTRANCOMMIT_1	DBMSGBASE+61 /* Begin Transaction Message */
#define DBTRANCOMMIT_2	DBMSGBASE+62 /* Begin Transaction Message */
#define DBTRANSTART	DBMSGBASE+63 /* Begin Transaction Message */
#define DBTRANBACKOUT	DBMSGBASE+64 /* Begin Transaction Message */

/* Messages used between the DBopSchedule processes and application
** 	processes acting in a server functionality for a driver process
*/
#define DBTRANMSGTyp	DBMSGBASE+70     /* Transaction Manager Message */

	/* SHARED MEMORY KEY BETWEEN DBopSchedule and DBopControl processes */
#define DBOPSCHEDSHAREDMEM DBMSGBASE+71

/* Messages used between DB-related CEPs and DBmonitor.C
 */
#define	DBCEP_CLRLOG	DBMSGBASE+80	/* CEP -> DBMON:zero archives	*/

/*  Messages between DBS and SPMAN for SPAFU
 */

  /* SPMAN -> DBS : SPAFU is starting */
#define DBsuSyncStartTyp         (DBMSGBASE+81)
#define DBsuSyncStartAckTyp      (DBMSGBASE+82) /* DBS -> SPMAN: Ack to above */

  /* SPMAN -> DBS : Mapping of data done */
#define DBsuSyncMapDoneTyp       (DBMSGBASE+83)
#define DBsuSyncMapDoneAckTyp    (DBMSGBASE+84) /* DBS -> SPMAN: Ack to above */

  /* SPMAN -> DBS : Apply updates to SPAs */
#define DBsuSyncApplyTyp         (DBMSGBASE+85)
#define DBsuSyncApplyAckTyp      (DBMSGBASE+86) /* DBS -> SPMAN: Ack to above */

  /* DBS -> SPMAN : Application of updates in log files is almost done */
#define DBsuSyncAlmostDoneTyp    (DBMSGBASE+87)
#define DBsuSyncAlmostDoneAckTyp (DBMSGBASE+88) /* SPMAN -> DBS: Ack to above */

  /* SPMAN -> DBS : Old SPA has been killed */
#define DBsuSyncOldDoneTyp       (DBMSGBASE+89)

  /* DBS -> SPMAN : Operation queue for old SPA is empty */
#define DBsuSyncDoneTyp          (DBMSGBASE+90)

  /* SPMAN -> DBS or DBS -> SPMAN : Abort SPAFU */
#define DBsuSyncAbortTyp         (DBMSGBASE+91)


/*  Messages between DBS and SPMAN/CEP
 */
#define DBsaSuspendUpdateTyp    (DBMSGBASE+92) /* SPMAN -> DBS : request any updates to
                                                database be queued */
#define DBsaSuspendUpdateAckTyp (DBMSGBASE+93) /* DBS -> SPMAN : acknowledge that updates
                                                to database are being queued. */
#define DBsaResumeUpdateTyp     (DBMSGBASE+94) /* SPMAN -> DBS : resume updating the
                                                SPA database table */
#define DBsaResumeUpdateAckTyp  (DBMSGBASE+95) /* DBS -> SPMAN : acknowledge receipt of
                                                Resume Update message */
#define DBsaRushDataTyp         (DBMSGBASE+96) /* SPMAN -> DBS : request providing the SPP
                                                with latest data */
#define DBsaRushDataAckTyp      (DBMSGBASE+97) /* DBS -> SPMAN : acknowledge that SPP has
                                                been provided with latest data */

#define DBsaDelSpaTyp      	(DBMSGBASE+98) /* CEP -> DBS : inform DBS to do cleanup
						due to deletion of SPA */


/*  Message sent by CLR:ORACLE,PROC=x CEP to disconnect and then reconnect
 *  from Oracle.
 *
 *  Keep this message around for the time being to avoid build errors.
 */
#define DBDISCORACLETYP   (DBMSGBASE+100)

/*  Message sent by CLR:DBLOCKS,PROC=x CEP to disconnect and then reconnect
 *  from database.
 */
#define DBreconnectTyp    (DBMSGBASE+100)

/*  Messages between DBS and SPA
 */
#define DBsyncNVpairTyp   (DBMSGBASE+101)  /* SPA -> DBS : data message */

#define DBsyncNVackTyp    (DBMSGBASE+102)  /* DBS -> SPA : data message acknowledgement */

/*
 * Messages between DBS and a debug tool
 */
#define DBSqueueMgrDebugTyp    (DBMSGBASE+110)  /* tool -> DBS query */

#define DBSqueueDebugTyp       (DBMSGBASE+111)

#define DBSdebugRespTyp        (DBMSGBASE+112)  /* DBS -> tool response */

#define DBSdebugActionTyp      (DBMSGBASE+113)  /* tool -> DBS action */

#define DBSspafuDebugTyp       (DBMSGBASE+114)

/*
 * Messages from DBStool to DBSYNC: used for retrofit
 */ 
#define DBSretroHoldTyp             (DBMSGBASE+115)
#define DBSretroBkoutTyp            (DBMSGBASE+116)
#define DBSretroDumpTyp             (DBMSGBASE+117)

/*
 * Messages from DBSYNC to DBStool: used for retrofit
 */ 
#define DBSretroHoldAckTyp          (DBMSGBASE+119)
#define DBSretroBkoutAckTyp         (DBMSGBASE+120)
#define DBSretroDumpAckTyp          (DBMSGBASE+121)

/*
 * Messages to DBMON from DBRECOV CEPs
 */
#define DBinhRecoverTyp             (DBMSGBASE+122)
#define DBalwRecoverTyp             (DBMSGBASE+123)
 
/*
 * Messages between DBMON and SYSTAT, also DBMON and OP:DBSTATUS CEP
 */
#define DBrecoverQryTyp             (DBMSGBASE+124)
#define DBrecoverAnsTyp             (DBMSGBASE+125)

/*
 * Message between DBCN and subscribers for subscription/query database 
 * change notification and the acknowledgement.
 */ 
#define DBsubscribeChnotTyp	     (DBMSGBASE+126)
#define DBsubscribeChnotAckTyp       (DBMSGBASE+127)
#define DBsubscriptionQryTyp         (DBMSGBASE+128)
#define DBsubscriptionQryAckTyp      (DBMSGBASE+129)
#define DBqryNotifMsgIdTyp           (DBMSGBASE+130)
#define DBqryNotifMsgIdAckTyp        (DBMSGBASE+131)
#define DBCNNotifyTyp                (DBMSGBASE+132)
#define DBCNTerminateSubTyp          (DBMSGBASE+133)

/*
** Message type between DBCN and CEPs
*/
#define DBCNcepQryTyp                 (DBMSGBASE+140)
#define DBCNalwChnotTyp               (DBMSGBASE+141)
#define DBCNinhChnotTyp               (DBMSGBASE+142)
#define DBCNclrBufTyp                 (DBMSGBASE+143)
#define DBCNdelSubscriptionTyp        (DBMSGBASE+144)
#define DBCNcepAckTyp                 (DBMSGBASE+145)
#define DBCNbufQryAckTyp              (DBMSGBASE+146)
#define DBCNsubNoQryAckTyp            (DBMSGBASE+147)
#define DBCNcepQryAckBackTyp          (DBMSGBASE+148)

/*
 * Messages to DBMON from AUD:PLATDB CEP
 */
#define DBauditMsgTyp                 (DBMSGBASE+150)
#define DBauditAckTyp                 (DBMSGBASE+151)

#endif
