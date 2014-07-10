#ifndef _GLMSGS_H
#define _GLMSGS_H

/*
** File ID:	@(#): <MID7866 () - 07/16/03, 1.1.1.79>
**
** File:		MID7866
** Release:		1.1.1.79
** Date:		07/16/03
** Time:		09:56:16
** Newest applied delta:07/16/03
*/
/*
** File:	GLmsgs.h
**
** Description:	This file contains messages which flow between the CC
**		and the SM.  Message types are currently an U_short.
**
**		Note: This file also contains message types only used
**		      on the CC.  It has become the centralized "message
**		      range file" for all messages in the CC and the SM.
**
**		Note2: If a maximum range is specified make sure that it
**             does not run into the following range by using
**             0xff rather than 0x100
*/

#define GLMSGBASE	0
#define	GLPSTOPMSG	(GLMSGBASE)		/* Process stop message */
/* local messages on SM or AM use 0x100 starting at GLLMSGBASE */
#define GLLMSGBASE	(GLMSGBASE+0xff00)

/*
** SM MD process needs 0x200 msgs
** starting at MDMSGBASE in hdr/sm/md/MDglobals.h
*/
#define MDMSGBASE	(GLMSGBASE+0x100)

/*
** CC stchg process uses 1 msg
** defined in hdr/GLhwstate.h
*/
#define GLSTCHGBASE	(GLMSGBASE+0x300)

/*
**	DSC reports take 0x100 (max!) messages 
**	starting at DSMSGBASE in hdr/sm/ds/DSglobals.h
*/
#define	DSMSGBASE	(GLMSGBASE+0x400)

/*
** OSDS takes 0x100 messages starting at OSMSGBASE,
** in sm/hdr/os/OSdefs.h
*/
#define OSMSGBASE	(GLMSGBASE+0x500)

/*
** Message Handler takes 0x100 messages starting at MH_MSGBASE,
** in cc/hdr/msgh header files.
*/
#define MHMSGBASE	(GLMSGBASE+0x600)

/*
** SCI takes 0x100 messages starting at SCMSGBASE,
** in cc/hdr/sci header files.  Every message has an ack and a failure
** response.  The ACK msg types are the request msg type + SCACKOFF
** while the fail msg types are the request msg type + SCFAILOFF
*/
#define SCMSGBASE	(GLMSGBASE+0x700)
#define SCACKOFF	(0x20)
#define SCFAILOFF	(0x40)
#define SCMSGMAX	(SCMSGBASE+0xff)

/*
** SPMAN takes 0x100 messages starting at SAMSGBASE,
** in cc/sa/lib/MHmsg.H
*/
#define SAMSGBASE	(GLMSGBASE+0x800)

/*
** SMI subsystem takes 0x100 messages starting at ECMSGBASE
*/
#define ECMSGBASE     (GLMSGBASE + 0xa00)

/*
** DBI takes 0x100 messages starting at DBMSGBASE,
** in cc/hdr/db header files.
*/
#define DBMSGBASE	(GLMSGBASE+0xb00)

/*
** CFTI subsystem takes 0x100 messages starting at CRMSGBASE,
** in cc/hdr/msgh header files.
*/
#define CRMSGBASE	(GLMSGBASE+0xc00)

/*
** LIH subsystem takes 0x100 messages starting at LIMSGBASE,
** in cc/hdr/smsch/lih header files.
*/
#define LIMSGBASE	(GLMSGBASE+0xd00)
#define LIMSGMAX	(LIMSGBASE+0xff)

/*
** SM subsystem takes 0x100 messages starting at SMMSGBASE,
** in cc/hdr/smsch/smspec header files.
*/
#define SMSPECMSGBASE	(GLMSGBASE+0xe00)
#define SMSPECMSGMAX	(SMSPECMSGBASE+0xff)

/*
** "Trace and trap" subsystem messages -- used
** in "cc/hdr/tt/TTmtype.H"
*/
#define TTMSGBASE	(GLMSGBASE+0xf00)
#define TTMSGMAX	(TTMSGBASE+0xff)

/*
*
*** The SPA subsystem range has been moved to 0x3000.  So ***
*** this range is available for use by another subsystem. ***
*
*#define SPMSGBASE	(GLMSGBASE + 0x1000)
*#define SPMSGSIZE	0x400
*/

/*
** INIT subsystem takes 0x100 messages starting at INMSGBASE
**	Note that this value is also used as the key to attach
**	to the INIT shared memory segment used by permanent processes.
**	Also, INMSGMAX is used as the key for INIT's private shared
**	memory segment.
*/
#define INMSGBASE	(GLMSGBASE + 0x1400)
#define INMSGMAX	(INMSGBASE+0xff)

/*
** SLL Debugger (DI) subsystem takes 0x100 starting at DIMSGBASE, in
** the cc/sce/debugger/hdr/DIPrimMsg.H header file.
*/

#define DIMSGBASE	(GLMSGBASE + 0x1500)
#define DIMSGMAX	(DIMSGBASE+0xff)

/*
** EIB firmware takes 0x20 messages starting at EBFWMSGBASE,
** in the hdr/sm/ap/eib/EBmsgtypes.h header file
*/
#define EBFWMSGBASE     (GLMSGBASE + 0x1600)
#define EBFWMSGMAX	(EBFWMSGBASE+0x19)

/*
** TimesTen takes 0x80 messages starting at 0x1620 till 0x1699
*/
#define TTENMSGBASE  (GLMSGBASE +0x1620);

/*
** Timing library:
**	The event handler will return timer expiration events which look
**	like timer expiration messages.  Therefore, the timer library
**	needs to define message types.
*/
#define TMMSGBASE	(GLMSGBASE + 0x1700)
#define TMMSGMAX	(TMMSGBASE+0xff)


/*
** MP subsystem uses 1 msg
** defined in hdr/sm/mp/MPglobals.h
*/
#define MPMSGBASE	(GLMSGBASE+0x1800)
#define MPMSGMAX	(MPMSGBASE+0x10)

/*
** CSCF subsystem uses 0x400 messages.
** defined in hdr/cscf/CSmsgtypes.h
*/
#define CSMSGBASE	(GLMSGBASE+0x1900)
#define CSMSGMAX	(CSMSGBASE+0x3ff)


/* 
** BILL subsystem messages defined in cc/hdr/bill/BLmtype.H
*/
#define BLMSGBASE	(GLMSGBASE+0x1D00) 
#define BLMSGMAX	(BLMSGBASE+0xff)

/*
** SS7 SCH takes 0x100 messages starting at S7MSGBASE.
*/
#define S7MSGBASE	(GLMSGBASE+0x1E00)
#define S7MSGMAX	(S7MSGBASE+0xff)

/* Define extra MSGBASE for multi-process S7SCH */
#define S71MSGBASE      (S7MSGBASE+0x10)
#define S72MSGBASE      (S7MSGBASE+0x20)
#define S73MSGBASE      (S7MSGBASE+0x30)

/*
** MEAS subsystem uses 0x100 messages
** defined in cc/hdr/meas/MSmtype.H
*/

#define MSMSGBASE	(GLMSGBASE+0x1F00)
#define MSMSGMAX	(MSMSGBASE+0xff)


/*
** SMSI SCH uses 0x100 messages
** defined in cc/hdr/smsi/SSglobal.h
*/

#define SSSPECMSGBASE	(GLMSGBASE+0x2000)
#define SSSPECMSGMAX	(SSSPECMSGBASE+0xff)

/*
** HWid's use 0x100 messages
** defined in cc/hdr/misc/HWid.h
*/

#define HWSPECMSGBASE	(GLMSGBASE+0x2100)
#define HWSPECMSGMAX	(HWSPECMSGBASE+0xff)


/*
** RWP uses 0x200 messages
** defined in cc/hdr/rwp/RWPmtype.H
**
** RTDB uses a subset of this range
** defined in cc/hdr/rdb/RDBmtype.H
*/

#define RWPMSGBASE (GLMSGBASE+0x2200)
#define RWPMSGMAX (RWPMSGBASE+0x1ff)


/* The next three message ranges are for
** additional customer SCH's that can be added later */

#define CUSCH1MSGBASE (GLMSGBASE+0x2400)
#define CUSCH1MSGMAX (CUSCH1MSGBASE+0xff)


#define CUSCH2MSGBASE (GLMSGBASE+0x2500)
#define CUSCH2MSGMAX (CUSCH2MSGBASE+0xff)


#define CUSCH3MSGBASE (GLMSGBASE+0x2600)
#define CUSCH3MSGMAX (CUSCH3MSGBASE+0xff)


/* 
** SCHED also uses 0x100 messages
**	defined in cc/sched/SHmsgs.H
*/

#define SHMSGBASE	(GLMSGBASE+0x2700)
#define SHMSGMAX	(SHMSGBASE+0xff)


#define FUPDMSGBASE (GLMSGBASE+0x2800)
#define FUPDMSGMAX (FUPDMSGBASE+0xff)


#define SFSIMSGBASE (GLMSGBASE+0x2900)
#define SFSIMSGMAX  (SFSIMSGBASE+0xff)


/* Messages for the ASI subsystem */

#define ASIMSGBASE (GLMSGBASE+0x2a00)
#define ASIMSGMAX (ASIMSGBASE+0xc5)

/*
** for some historical reasons, when TimesTen subsystem
** was introduced into PSP code, it used 10951( 0x2ac7 ) and 
** thereafter as its own message code without noticing that 
** this range of code has already been reserved for ASI
** subsystem.  Since now there are many applications already
** hard coded 10951 in there program, we have to squeeze
** ASI message code range to 0x2ac5
*/

/* Messages for the TimesTen subsystem
** detailed message codes are defined in
** cc/hdr/timesten/msgtype.h
** see also TIMESTENHIGHMSGBASE
*/
#define TIMESTENLOWMSGBASE ( GLMSGBASE + 0x2ac6 ) 
#define TIMESTENLOWMSGMAX  ( GLMSGBASE + 0x2aff )

/* Messages for the FTSCH subsystem */

#define FTMSGBASE (GLMSGBASE+0x2b00)
#define FTMSGMAX (FTMSGBASE+0xff)

/* Messages for the IOSCH subsystem */

#define IOMSGBASE (GLMSGBASE+0x2c00)
#define IOMSGMAX (IOMSGBASE+0xff)

/* Messages for the X25SCH subsystem */
#define X25MSGBASE (GLMSGBASE+0x2d00)
#define X25MSGMAX (X25MSGBASE+0xff)


/* Messages for the RC subsystem */
#define RCMSGBASE (GLMSGBASE+0x2e00)
#define RCMSGMAX (RCMSGBASE+0xff)

/*
** Reserve 0x100 message range starting at ISUPMSGBASE for ISUP system process.
** All ISUP messages are defined in cc/hdr/ismsch/isup/ISupglobals.H
*/
#define ISUPMSGBASE     (GLMSGBASE+0x2f00)
#define ISUPMSGMAX      (ISUPMSGBASE+0xff)

/*
** SPA subsystem takes 0x800 messages starting at SPMSGBASE,
** in the cc/spa/hdr/SPmsgs.H header file.
*/
#define SPMSGBASE	(GLMSGBASE + 0x3000)
#define SPMSGSIZE	0x800


/*              
** TCPIPSCH subsystem takes 0x200 messages starting at TCPSCHMSGBASE,
** in the cc/hdr/tcpsch/TIdefs.H header file.
*/
#define TCPSCHMSGBASE       (GLMSGBASE + 0x3800) 
#define TCPSCHMSGMAX	    (TCPSCHMSGBASE+0x1ff)

/* Define extra MSGBASE for the seperated MATESCH */
#define MATESCHMSGBASE       (TCPSCHMSGBASE + 0x50) 

/* Define extended EXTMSGBASE for the seperated out shm objects */
#define TCPSCHEXTMSGBASE    (TCPSCHMSGBASE + 0x100)

/*              
** Message numbers for Telcom Server subsystem.
** The defines for the messages can be found in
** cc/ts/hdr/TSmsgs.H
*/
#define TSMSGBASE       (GLMSGBASE + 0x3a00) 
#define TSMSGMAX	(TSMSGBASE+0x1ff)


/*              
** Message numbers for File Manager 
** The defines for the messages can be found in
** cc/hdr/fm/FMmsgs.H
*/
#define FMMSGBASE       (GLMSGBASE + 0x3c00) 
#define FMMSGMAX	(FMMSGBASE + 0xff)

/*
** S7TSSCH takes 0x100 messages starting at STMSGBASE.
*/
#define STMSGBASE       (GLMSGBASE+0x3E00)
#define STMSGMAX        (STMSGBASE+0xff)

/* Define extra MSGBASE for multi-process S7TSSCH */ 
#define ST1MSGBASE      (STMSGBASE+0x10)
#define ST2MSGBASE      (STMSGBASE+0x20)
#define ST3MSGBASE      (STMSGBASE+0x30)


#define AEXMSGBASE       (GLMSGBASE+0x3F00)
#define AEXMSGMAX        (AEXMSGBASE+0xff)

/*
** NSI subsystem reserves 0x100 messages starting at NSIMSGBASE. 
** The defines can be found in cc/hdr/nsi/NSmsgTypes.H.
*/
#define NSIMSGBASE       (GLMSGBASE+0x4000)
#define NSIMSGMAX        (NSIMSGBASE+0xff)

/*
** Unified Studies (US) subsystem message range definition
** for 0x100 messages starting at USMSGBASE. The individual message
** definitions within this range can be found in cc/hdr/us/USmtype.H
*/
#define USMSGBASE	(GLMSGBASE+0x4100)
#define USMSGMAX	(USMSGBASE+0xff)


/*
** SEASI uses 0x100 messages
** defined in cc/hdr/seasi/hdr/SSmtype.H
*/

#define SSMSGBASE (GLMSGBASE+0x4200)
#define SSMSGMAX (SSMSGBASE+0xff)

/*
** SignalSoft application message
** defined inside SignalSoft WAF header file WAF/core/inc/lehmsg.hxx
*/
 
#define SignalSoftMSGBASE (GLMSGBASE+0x4300)
#define SignalSoftMSGMAX (SignalSoftMSGBASE+0xff)
 

/*
** Java Proxy Process uses a range of 0x100 messages
*/

#define	JPPMSGBASE	(GLMSGBASE+0x4400)
#define	JPPMSGBASEMAX	(JPPMSGBASE+0xff)

/*
** The JPP process communicates with LDAPSCH, AEX and other
** processes.  The range of messages that JPP uses is from JPPMSGBASE
** to JPPMSGBASEMAX.  The messages in this range that JPP uses to
** communicate with AEX are a subset as defined below:
*/

#define LDAMSGBASE (JPPMSGBASE + 0x10)
#define LDAMSGMAX  (LDAMSGBASE + 0x60)
  
/* The messages in the above range that are actually used are defined in 
** cc/hdr/ldap/LDAmsgType.H. Please make sure that if the JPP process 
** adds new message types they do not fall in the above range. 
** If new message types are added in cc/hdr/ldap/LDAmsgType.H see if the
** upper bound of the range needs to be increased.
*/

/* Allow 0x1800 messages for SPA actions and events.
** Notes:
** - There are places in the SLL compiler that assume
** the ranges for actions and events are continguous.
** - The sum of messageActionAllocation and messageEventAllocation
** in the builtin file must not be greater than LLMSGSIZE.
*/
#define LLMSGBASE (GLMSGBASE + 0x4500)
#define LLMSGSIZE 0x1800


/*
** The UCMDPROC process reserves 0x100 msgs. The messages are defined
** in cc/hdr/ucmd/UCMDmsgType.H.
*/
 
#define UCMDMSGBASE     (GLMSGBASE+0x5d00)
#define UCMDMSGBASEMAX  (UCMDMSGBASE+0xff)
 

/*
** The TIDMGR process reserves 0x100 msgs. The messages are defined
** in cc/hdr/tmg
*/
 
#define TIDMGRMSGBASE     (GLMSGBASE+0x5e00)
#define TIDMGRMSGBASEMAX  (TIDMGRMSGBASE+0xff)

/*
** The eCAM software reserves 0x200 msgs.  The messages are
** defined in cc/hdr/ecam
*/

#define EMMSGBASEMSG	(GLMSGBASE+0x5f00)
#define EMMSGBASEMAX	(EMMSGBASEMSG+0x1ff)


/*
** The SS7 overload software reserves 0x10 msgs.  The messages are
** defined in cc/hdr/s7proc/ovld_control.H
*/
#define OVLDMSGBASEMSG   (GLMSGBASE+0x6100)
#define OVLDMSGBASEMAX   (OVLDMSGBASEMSG+0xf)

/*
 * The ALARMS module and related functionality reserves 0x10 mesg
 * types. the values are defined in /vobs/ahe/cslee/include/PINAlarmMsgTypes.h
 */
#define ALARMSGBASEMSG   (GLMSGBASE+0x6110)
#define ALARMMSGBASEMAX   (ALARMSGBASEMSG+0xf)

/* The Resource Manager RESMGR reserves 0x20 msgs. The messages
** are defined in cc/resmgr/ResMgrDefs.H
*/

#define RESMGRMSGBASEMSG (GLMSGBASE+0x6120)
#define RESMGRMSGBASEMAX (RESMGRMSGBASEMSG+0x1f)

/* Messages for the TimesTen subsystem
** detailed message codes are defined in
** cc/hdr/timesten/msgtype.h
** see also TIMESTENLOWMSGBASE
*/
#define TIMESTENHIGHMSGBASE ( GLMSGBASE + 0x6140 ) 
#define TIMESTENHIGHMSGMAX  ( TIMESTENHIGHMSGBASE + 0x3f )

/*
 * The System Maintenance subsystem uses 0x10 messages.
 * The values are defined in cc/hdr/sm/MOFmsgs.H.
 */
#define SYSMTCEMSGBASE		(GLMSGBASE+0x6180)
#define SYSMTCEMSGBASEMAX	(SYSMTCEMSGBASE+0xf)

/*
** DNSSCH message type range
** detailed message codes are defined in
** cc/hdr/dnssch/DNSmsg.H
*/
#define DNSMSGBASE      (GLMSGBASE+0x6190)
#define DNSMSGMAX       (DNSMSGBASE+0x3f)

/*
**  FREE RANGE:  0x61c0 through 0x61ff
*/


/*
** SIP message type range
*/
#define PIMSGBASE       (GLMSGBASE+0x6200)
#define PIMSGMAX        (PIMSGBASE+0xff)

/*
** IPSCH message type range
*/
#define IPMSGBASE       (GLMSGBASE + 0x6300)
#define IPMSGMAX        (IPMSGBASE+0xff)

/*
** UNUSED?
*/
#define PTMSGBASE	(GLMSGBASE+0x6400)
#define PTMSGMAX	(PTMSGBASE+0xff)

/*
** STSCH takes 0x100 messages starting at SGMSGBASE.
*/

#define SGMSGBASE       (GLMSGBASE+0x6500)
#define SGMSGMAX        (SGMSGBASE+0xff)

/* Define extra MSGBASE for multi-process STSCH */  
#define SG1MSGBASE      (SGMSGBASE+0x10)
#define SG2MSGBASE      (SGMSGBASE+0x20)
#define SG3MSGBASE      (SGMSGBASE+0x30)

/*
** DIAMSCH takes 0x100 messages
*/

#define DIAMSCHBASE     (GLMSGBASE+0x6600)
#define DIAMSCHMAX      (DIAMSCHBASE+0xff)

/*
** SCTPSCH takes the range from 0x6700 to 0x677f
*/
#define SCTPSCHMSGBASE  (GLMSGBASE+0x6700)
#define SCTPSCHMSGMAX   (SCTPSCHMSGBASE+0x7f)

/* Define extra MSGBASE for multi-process SCTPSCH */  
#define SCTPSCH1MSGBASE (SCTPSCHMSGBASE+0x10)
#define SCTPSCH2MSGBASE (SCTPSCHMSGBASE+0x20)
#define SCTPSCH3MSGBASE (SCTPSCHMSGBASE+0x30)

/*
** MUXSCH takes the range from 0x6780 to 0x67ff
*/ 
#define MUXSCHMSGBASE  (GLMSGBASE+0x6780)
#define MUXSCHMSGMAX   (MUXSCHMSGBASE+0x7f)

/*
** CDRSCH takes the range from 0x6800 to 0x68ff
*/
#define CDRSCHMSGBASE  (GLMSGBASE+0x6800)
#define CDRSCHMSGMAX   (CDRSCHMSGBASE+0xff)

/*
** SIPSCH takes the range from 0x6900 to 0x69ff
*/
#define SIPSCHMSGBASE (GLMSGBASE+0x6900)
#define SIPSCHMSGMAX  (SIPSCHMSGBASE+0xff)


/*
** ODBCSCH takes 0x100 messages
*/

#define ODBCSCHBASE     (GLMSGBASE+0x6a00)
#define ODBCSCHMAX      (ODBCSCHBASE+0xff)

/*
** PF takes 0x100 messages
*/

#define PFBASE     (GLMSGBASE+0x6b00)
#define PFMAX      (ODBCSCHBASE+0xff)


/* next available message type is 0x6c00 */

#endif
