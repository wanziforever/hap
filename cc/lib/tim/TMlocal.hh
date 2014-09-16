/* This is the header file for timer management in OSDS.  It is #included by
 * any files which deal with timer management;
 *
 * A portion of this file describes the three clocks: internal,
 * relative, and absolute.  The remainder of the file describes data
 * structures associated with timers.
 */

#ifndef	_TMTIMERS
#define	_TMTIMERS

#include "cc/hdr/tim/TMtimers.hh"	/* TMHZ */

/* The TMIS_ITIME() validates an TMITIME.
 */
#define TMIS_ITIME(x) ((((x).lo_time & ~07777777777) == 0) && ((x).hi_time >= 0))

#define TMTIMINCR(x,y) \
        {x.hi_time+=(short)((x.lo_time+=y)>>30);x.lo_time&=07777777777;}

/*
 * TMTIMADD adds two TMITIME times together
 */
#define TMTIMADD(x,y) \
        {x.hi_time+=(((short)((x.lo_time+=y.lo_time)>>30))+y.hi_time); \
		x.lo_time&=07777777777;}

/*
 * TMTIMSUB subtracts two TMITIME times.
 * NOTE: x must be larger than y to guarantee non-negative numbers.
 */
#define TMTIMSUB(x,y) \
        { if (x.lo_time < y.lo_time) { \
		x.lo_time += (1<<30) - y.lo_time; \
		x.hi_time--; \
	  } \
	  else { \
		x.lo_time -= y.lo_time; \
	  } \
	  x.hi_time -= y.hi_time; \
	}

/* TMITIMDIFF() returns (a long) the difference in milliseconds
 * between two variables of type TMITIME (see TMITIME's typedef in
 * os/OStimers.h for a complete description as to how the count of
 * milliseconds is stored in a variable of this type).
 *		
 * WARNING!!! - This macro will not work if the two variables are
 *		more than ((2 * TMIMAXTIME) + 1) milliseconds apart.
 */

/* For a variable of type TMITIME, TMIMAXTIME is the maximum value
 * that the "lo_time" field can assume.
 */
#define TMIMAXTIME	0x3fffffff

#define TMITIMDIFF(start, stop)					\
	(((stop.hi_time) == (start.hi_time)) ? ((stop.lo_time) - (start.lo_time))  \
		:((stop.lo_time) - (start.lo_time) + (TMIMAXTIME + 1))) 


/* TMTIME defines time as seen by a user program; as such it is typedef'd
 * in the user header file "OStime.h".  OSDS programs see this typedef by
 * including "OSdefs.h" which eventually includes "OStime.h".
 * RTIME is defined as an TMTIME, and as such represent
 * time as a pair: days and milliseconds within a day past some epoch.
 */



/* The macro TMIS_TIME validates a TMTIME.
 */
/* #define	TMIS_TIME(x)	((x).tod >= 0 && (x).tod < TMONEDAY) */



/* TMONEDAY is the length of a day, in clock ticks.
 */
#define TMONEDAY	(3600L*24*TMHZ)


/* The remainder of this header file describes the data structures
 * associated with timer management, such as TCBs, their idle resource
 * list, and the timing queue (implemented as a heap).
 */

/* The tstate field is used to store the current timer TMTSTATE.
 *
 * Free TCBs are in the TEMPTY state, and are doubly-linked into a 
 * free list.  The rlink names the next free TCB, and the llink names
 * the previous one.  TMfftcb (first free TCB) refers to the head of
 * this list, while TMlftcb (last free TCB) refers to its end.
 *
 * TCBs in the QTIMEOUT, QTIMER, CTIMER, states hold
 * real timers; their state tells which type.  These TCBs are present in the
 * timing queue, represented as a heap, which is sorted by their go_off
 * TMITIMEs.  The thi identifies the TCB's position within the heap.
 *
 * For explicit timers, the tag field holds the tag specified as the second
 * argument to the corresponding call to the timer-creation primitive.
 *
 * For CTIMERs, the period field holds the period of the timer; this is the
 * same as the first argument to TMCTIMER().
 *
 * All timers are "owned" by the process that caused their creation.  The owner
 * field holds the PCB index of this process.  All timers owned by the same
 * process are chained together in a double-linked list using the llink and
 * and rlink (no order is implied by this chain).  The tcb0 and tcbn
 * field in the PCB refer to the beginning and the end of this chain.
 *
 * TCBs in the TLIMBO state are in transition from one state to another.  A
 * timer is placed into the TLIMBO state before calling an internal OSDS
 * routine whose action depends on the current timer state, in cases where
 * the "current" state is no longer applicable.
 *
 * The go_off field specifies the TMITIME at which the timer will go off.
 */


#define TMTCBA(x)	TMbtcb[x]		/* READ WARNING ABOVE !	*/

/* TMIS_TCBI() insures that an TCB index is >= 0 and < TMTCBNUM.
 */
#define TMIS_TCBI(x)	((unsigned long) (x) < TMTCBNUM)


/* TMIS_THI() insures that a heap index is > 0 and <= TMTCBNUM.
 */
#define TMIS_THI(x)	((unsigned long) (x) <= TMTCBNUM)

/* The following macro evaluates to true iff the TMITIME specified by the first
 * argument is strictly less than (prior in time to) the TMITIME specified by
 * the second argument.
 */
#define LTIME(a,b)((a.hi_time==b.hi_time)?(a.lo_time<b.lo_time):(a.hi_time<b.hi_time))

#define LETIME(a,b)((a.hi_time==b.hi_time)?(a.lo_time<=b.lo_time):(a.hi_time<b.hi_time))

const Short TMNULL = -1;

/*
** These constants are used when manipulating timer values in TMITIME
** structures.  They are used when determining "roll-over" from "lo_time"
** to "hi_time" in TMITIME structures as only 30 bits of the "lo_time"
** structure element are used:
*/
const Long	TMlo_bits = 30;
const Long	hzlimit = 1 << TMlo_bits; // Limit for TMHZ stored in lo_time
const Long	maxhz = hzlimit - 1; // Max. num. of TMHZ possible in lo_time

/* This constant is used when converting seconds to hertz: */
const Long	seclimit = hzlimit/TMHZ;	/* (2^30)/TMHZ	*/

#endif
