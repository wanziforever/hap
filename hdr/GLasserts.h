#ifndef	__GLASSERTS_H
#define	__GLASSERTS_H
/*
**	File ID: 	@(#): <MID22291 () - 08/17/02, 28.1.1.1>
**
**	File:			MID22291
**	Release:		28.1.1.1
**	Date:			08/21/02
**	Time:			21:30:53
**	Newest applied delta:	08/17/02 04:15:00
**
** DESCRIPTION:
**      This file contains the ranges of assert numbers for each
**      subsystem.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**      The suggested name of the hdr file to contain the
**      individual assert numbers is shown as a comment on each line.
**      If a different file name is used, the comment should be udpated.
*/
#define INASSERT	1000	/* cc/init/hdr/INasserts.H */
#define MHASSERT	2000	/* cc/msgh/hdr/MHasserts.H */
#define CRASSERTRNG	3000	/* cc/hdr/cr/CRassertIds.H */
#define FTASSERT        3500    /* cc/hdr/ft/FTasserts.H */
#define DBASSERT        4000    /* cc/hdr/db/DBassert.H */
#define RCASSERT	4500	/* cc/hdr/rc/RCassertIds.H */
#define SAASSERT	5000	/* cc/sa/hdr/SAasserts.H */
#define SPASSERT	6000	/* cc/spa/hdr/SPasserts.H */
#define TMASSERT	7000	/* cc/tim/hdr/TMasserts.H */
#define EHASSERT	8000	/* cc/eh/hdr/EHasserts.H */
#define BLASSERT	9000    /* cc/bill/hdr/BLasserts.H */
#define MSASSERT        10000   /* cc/meas/hdr/MSasserts.H */
#define SHASSERT        11000   /* cc/hdr/sched/SHasserts.H */
#define SUASSERT        12000   /* cc/hdr/su/SUasserts.H */
#define MISCASSERT      13000   /* cc/misc/hdr/GLasserts.H */
#define CSCFASSERT	14000	/* cc/cscf/hdr/CSasserts.H */
#define SMSIASSERT	15000	/* cc/smsi/hdr/SSasserts.H */
#define SFASSERT	16000	/* hdr/sm/SMasserts.h */
#define SCIASSERT 	17000	/* cc/hdr/sci/SCasserts.H  */
#define DIASSERT        18000   /* cc/sce/debugger/hdr/DIasserts.H */
#define ASIASSERT	19000	/* cc/hdr/asi/ASasserts.H */
#define SCEIASSERT	20000	/* cc/scei/hdr/SEasserts.H */
#define RWPASSERT       21000   /* cc/rwp/hdr/RWPasserts.H */
#define SEASIASSERT     22000   /* cc/seasi/hdr/SSasserts.H */
#define S7ASSERT        23000   /* cc/hdr/s7sch/S7assertIds.H */
#define X25ASSERT 	24000	/* cc/hdr/x25/X25asserts.H */
#define TTASSERT	25000	/* cc/hdr/tt/TTasserts.H */
#define SLASSERT	26000	/* cc/cep/sl/SLasserts.H */
#define TCASSERT	27000	/* cc/hdr/tcap/TCasserts.H */
#define APASSERT	28000	/* cc/hdr/ap/APasserts.H */
#define CUSCH1ASSERT 	30000	/* customer sch 1	   */
#define CUSCH2ASSERT 	31000	/* customer sch 2	   */
#define CUSCH3ASSERT 	32000	/* customer sch 3	   */
#define TSASSERT 	33000	/* cc/hdr/ts/TSasserts.H   */
#define STASSERT 	34000	/* cc/hdr/s7ts/STasserts.H   */
#define	NSIASSERT	35000	/* cc/nsi/hdr/NSasserts.H */
#define	ISASSERT	36000	/* cc/hdr/isup/ISasserts.H */
#define	USASSERT	37000	/* cc/us/hdr/USasserts.H */
#define SSASSERT        38000   /* SignalSoft assert base */
                                /* 150 errors 100 alarms  */
#define TIASSERT	39000   /* cc/hdr/tcpsch/TIassertIds.H */
#define LDAPASSERT      40000   /* cc/hdr/ldap/LDAPasserts.H */
				/* LDAP needs fewer than 100 */

#define IPASSERT        41000   /* cc/hdr/ts/IPasserts.H   */
#define SGASSERT        42000   /* cc/hdr/stsch/SGasserts.H   */
#define CDRASSERT       43000   /* cc/hdr/cdrsch/CDRasserts.H */
#define PFASSERT 		44000	/* cc/pf/src/infra/PFasserts.h */
#define MXASSERT        45000   /* cc/hdr/mux/MXasserts.H */




typedef long GLassertId;
#endif
