#ifndef	__INTIMERS_H
#define __INTIMERS_H
//
// DESCRIPTION:
// 	This header file includes timer information used by the INIT
//	process. 
//
// NOTES:
//

#include "hdr/GLtypes.h"
#include "cc/hdr/eh/EHhandler.hh"	/* Event handler */

/*
 * The following typedef defines information associated with timers in
 * the timing library which are set and cleared by INIT
 */
typedef struct {
	U_short	ttag;	/* Timer tag */
	Short	tindx;	/* Timer index */
	Bool	c_flag;	/* set to "TRUE" if this is a circular timer */
} INTMR;

/*
 * The following typedef defines the structure used to manage INIT
 * client processes' timers -- a process can have both a sanity timer
 * and a restart timer active at once.
 */
typedef struct {
	INTMR	sync_tmr;	/* Process sanity timer */
	INTMR	rstrt_tmr;	/* Process restart timer */
	INTMR	gq_tmr;		/* Global queue	timer */
} INPTMRS;

/*
 * Timer tags are created by "or"ing two constants together, one indicates
 * the timer type -- INinittmr, INarutmr, or INproctmr.  The second
 * constant describes the activity being timed.  Additionally, for
 * INproctmrs, the lower 9 bits of the timer tag are set to the
 * process table index corresponding to the specific process associated
 * with the timer
 */

/* Timer types */
const U_short	INTYPEMASK =	0xf000;	/* Mask to extract tmr type from tag */
const U_short	INPTAGMASK =	 0xc00;	/* Mask for proc. tmr. tag type */
const U_short	INPINDXMASK =	 0x3ff;	/* Reserve 10 bits for proc. indices */
const U_short	INWDSEQMASK = 	0x7f;	/* 7 bits for wd messages seq numbers	*/

const U_short	INITTAG =	0x8000;
const U_short	INPROCTAG =	0x4000;
const U_short	INAUDTAG =	0x2000;
const U_short	INGENTAG = 	0x1000;	/* Indicates general purpose timer tag	*/
const U_short	INARUTAG =	0x1001;
const U_short	INCHECKLEADTAG = 0x1002;
const U_short	INSOFTCHKTAG=  	0x1003;
const U_short	INVMEMTAG = 	0x1004;
const U_short	INMATEBOOTTAG =	0x1005;
const U_short	INSANITYTAG =	0x1006;
const U_short	INSETLEADTAG =	0x1007;
const U_short	INOAMLEADTAG =	0x1008;
const U_short	INOAMREADYTAG =	0x1009;
const U_short	INSETACTIVEVHOSTTAG =	0x100a;
const U_short	INVHOSTREADYTAG =	0x100b;


/*
 * INIT subsytem-wide timer tags used in conjunction with INITTAG:
 */
const U_short	INSEQTAG =	0x800;	/* System-wide initialization timer*/
const U_short	INPOLLTAG =	0x400;	/* Main loop "polling" tag	*/

/*
 * Process timer tags used in conjunction with the INPROCTAG timer type
 * -- note that the lower nine bits must be reserved for the process
 *    index in process timer tags
 */
const U_short	INSYNCTAG =	 0x800;	/* Process synchronization tmr */
const U_short	INRSTRTAG =	 0x400;	/* Process restart timer */
const U_short	INGQTAG =	 0xc00;	/* Process restart timer */

/*
 * Timer values (seconds) for INinittmr:
 */
const Long	INCREATETMR = 180;	/* Time for processes to be created */
const Long	INCLEANUPTMR = 45;	/* Time for procs. to finish CLEANUP */
//IBM JGH 20061016 add the defind
const Long	INITINTVL = 600;	/* boot interval...this */
					/* is started AFTER the system */
					/* completes a system reset  */

/* Timer values ( seconds ) for Softchk inhibit */
const Long      INSOFTCHKTMR = 1200;     

/*
 * Timer values (seconds) for INproctmr[].sync_tmr (with INSYNCTAG type):
 */
const Long INPCREATETMR = INCREATETMR/3;/* Time for a single proc. creation */
const Long INPCLEANUPTMR = INCLEANUPTMR/2;/* Time for one proc to CLEANUP */

/*
 * Timer values (seconds) for INpolltmr:
 */
const Long	INITPOLL = (100*TMHZ)/1000;/* Short polling interval during 	*/
					/* system initializations	 	*/
const Long	INPROCPOLL = (40*TMHZ)/1000;/* Polling interval during single 	*/
					/* process restart/re-init and during	*/
					/* software updated.  This timer is 	*/
					/* in system clock ticks calculated to  */
					/* result in 50 ms. timer		*/
/*
 * Timer value used for INaudtmr:
 */
const Long	INAUDTMR = 8;		/* Interval between audits 	*/
const Long	INCHECKLEADTMR = 5;	/* Interval to check for lead 	*/
const Long	INSETLEADTMR = 6;	/* Interval to enforce the lead */
const Long	INWDHBTMR = 3;		/* Frequency of WD heartbeats  	*/
const Long	INSANITYTMR = 2;	/* Frequency of INIT sanity peg	*/
/* Timer value for periodic VMEM warning message */
const Long 	INVMEMINT = 1800;	
const Long	INOAMLEADTMR = 4;	/* Interval to synchronize OAM lead 	*/
const Long	INOAMREADYTMR = 150;	/* Interval to oam lead ready 		*/
const Long	INVHOSTREADYTMR = 150;	/* Interval to vhost ready 		*/


extern mutex_t INtmrLock;
/*
 * The following macro clears timers when passed a pointer to a INTMR
 * structure containing the index of the timer to be cleared.
 */
#define INCLRTMR(tmr) {						\
	mutex_lock(&INtmrLock);					\
	(Void)INevent.clrTmr(tmr.tindx);			\
	mutex_unlock(&INtmrLock);				\
	tmr.tindx = -1;						\
	tmr.ttag = 0;						\
}

/*
 * This macro calls INsettmr() function with a hz_flag of false 
 * resulting in a timer being set in seconds. c_flag
 * determines whether the timer is circular or not.  
 * INsettmr() automatically clears any outstanding timers and
 * also escalates if a timer cannot be allocated.
 */
#define	INSETTMR(tmr, tdelay, tag, flag) INsettmr(tmr, tdelay, tag, flag, FALSE)

// the IN_SNPRCMX is defined in INinit.hh, but it is not included in the header
// include other header files which has INinit.hh before including this file
extern INPTMRS	INproctmr[IN_SNPRCMX];	/* Per-proc. timers */
extern INTMR	INinittmr;		/* INIT proc sequencing timer */
extern INTMR	INpolltmr;		/* INIT "polling" timer */
extern INTMR	INarutmr;		/* ARU "keep-alive" timer */
extern INTMR	INaudtmr;		/* Schedule audits timer */
extern INTMR	INvmemtmr;		/* Schedule periodic REPT VMEM OM */
extern INTMR	INcheckleadtmr;		/* Periodically check for lead availability */
extern INTMR    INsoftchktmr;           /* softchk inhibit timer     */ 
extern INTMR    INsanitytmr;            /* main thread sanity timer	*/ 
extern INTMR    INsetLeadTmr;           /* Lead enforcement timer	*/ 
extern INTMR    INoamLeadTmr;           /* OAM lead enforcement timer	*/ 
extern INTMR    INoamLeadTmr;           /* OAM lead enforcement timer	*/ 
extern INTMR    INoamReadyTmr;          /* OAM ready timer		*/ 
extern INTMR    INvhostReadyTmr;        /* vhost ready timer		*/ 
extern INTMR    INsetActiveVhostTmr;    /* Time to set Active Vhost	*/ 

extern class EHhandler	INevent;	/* Event handler declaration */

#endif
