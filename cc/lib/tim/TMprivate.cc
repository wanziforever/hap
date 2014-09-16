#include	"hdr/GLtypes.h"
#include	"hdr/GLreturns.h"
#include	"hdr/GLmsgs.h"

#include	"cc/hdr/tim/TMtimers.hh"
#include	"cc/hdr/tim/TMreturns.hh"
#include	"cc/lib/tim/TMlocal.hh"

/* This macro associates a tcb with a heap element and vice versa */

#define LINK2HEAP(h_p, h,t) {h_p->theap[h] = t; TMTCBA(t).thi = h;}


/* tsched scheds the timer named by the TMTCBI t to go off, by
 * inserting it into the timing queue.  The timer's state has already been
 * set to ORTIMER, CRTIMER.
 */


GLretVal
TMtimers::tsched(Long t, TMHEAP *heap_p)
{
	register Long	h;	/* Heap slot of new timer */
	register Long	ph;	/* Heap slot of its parent */
	register Long	pt;	/* Parent timer */
	TMITIME		key;	/* Go-off of new timer */
	TMITIME		pkey;	/* Go-off of parent timer */

	/* Append TMTCB to end of heap */
	h = heap_p->fftheap;
	heap_p->fftheap++;
	if ((!TMIS_THI(h)) || (heap_p->theap[h] != TMNULL)) {
		// Internal inconsistency, return
		return (TMINTERR2);
	}

	LINK2HEAP(heap_p, h, t);

	/* Now, let our new node "float" up the heap */
	key = TMTCBA(t).go_off;
	for (ph = h >> 1;   ph > 0;   h = ph, ph >>= 1) {
		pt = heap_p->theap[ph];
		if ((!TMIS_TCBI(pt)) || (TMTCBA(pt).thi != ph)) {
			// Internal heap error, return
			return(TMINTERR3);
		}
		pkey = TMTCBA(pt).go_off;
		if (LTIME(key, pkey)) {
			/* We need to float TMTCBI t up a level */
			LINK2HEAP(heap_p, h, pt);
			LINK2HEAP(heap_p, ph, t);
		} else {
			/* The heap looks OK */
			return(GLsuccess);
		}
	}
	return(GLsuccess);
}

/* tunsched unschedules a timer, named by the valid TMTCBI t, from the
 * timing queue.
 */


GLretVal
TMtimers::tunsched(Long t, TMHEAP *heap_p)
{
	register Long	h;	/* Heap slot of replacement timer */
	register Long	nh;	/* Heap slot of its parent or left child */
	Long		rh;	/* Heap slot of its right child */

	register Long	lastheap;	/* Last heap slot */

	register Long	replacement;	/* Replacement timer */
	register Long 	nt;		/* Parent timer or left child timer */
	register Long	rt;		/* Right child timer */

	TMITIME		key;	/* Go-off of replacement timer */
	TMITIME		nkey;	/* Go-off of parent timer or left timer */
	TMITIME		rkey;	/* Go-off of right child */

	/* TMTCB t doesn't belong in the heap anymore */
	TMTCBA(t).tstate = TLIMBO;

	/* Find t's position within the heap */
	h = TMTCBA(t).thi;
	if ((!TMIS_THI(h)) || (heap_p->theap[h] != t)) {
		// Internal error...
		return(TMINTERR4);
	}

	/* Replace t with the last timer in the heap, shrinking the heap */
	lastheap = --heap_p->fftheap;
	
	if (!(TMIS_THI(lastheap))) {
		// Internal error...
		return(TMINTERR5);
	}
	replacement = heap_p->theap[lastheap];
	if (!(TMIS_TCBI(replacement)) || (TMTCBA(replacement).thi != lastheap)) {
		// Internal error
		return(TMINTERR6);
	}
	LINK2HEAP(heap_p, h, replacement);

	/* The last heap slot is no longer occupied */
	heap_p->theap[lastheap] = TMNULL;

	/* If the deleted timer was the last timer in the heap, we are
	 * finished.  This situation can be detected in a variety of ways.
	 * The most economical is to compare the replacement timer's new
	 * location (i.e. h) with its former location (i.e. lastheap), since
	 * both of these are stored in register variables.
	 */

	if (h == lastheap) {
		/* Deleting the last timer in the heap is easy. */
		return(GLsuccess);
	}


	/* Chances are that t's replacement is now in the wrong heap
	 * slot.  Let's find out what its key is, and then try to float
	 * it up the heap (as per tsched).  Since the replacement
	 * could potentially sink down the heap, we'll try that afterwards.
	 */

	key = TMTCBA(replacement).go_off;

	/* Let replacement for t float up the heap */
	for (nh = h >> 1 ;   nh > 0;   h = nh, nh >>= 1) {
		nt = heap_p->theap[nh];
		if (!(TMIS_TCBI(nt)) ||  (TMTCBA(nt).thi != nh)) {
			// Internal error
			return(TMINTERR7);
		}
		nkey = TMTCBA(nt).go_off;
		if (LTIME(key, nkey)) {
			/* We need to propagate t back a level */
			LINK2HEAP(heap_p, h, nt);
			LINK2HEAP(heap_p, nh, replacement);
		} else {
			/* The node is high enough */
			break;
		}
	}


	/* Insure that it isn't too high (replacement is still in slot h) */
	for (  ;  ;  h = nh) {
		nh = h * 2;	/* Replacement's left child */

		/* Does heap-slot h have any children? */
		if (nh >= lastheap) {
			/* No -- it's as low as it can go -- we're done */
			return(GLsuccess);
		}

		/* Yes, there is at least a left child */
		nt = heap_p->theap[nh];
		if(!(TMIS_TCBI(nt)) || (TMTCBA(nt).thi != nh)) {
			// Internal error
			return(TMINTERR8);
		}
		nkey = TMTCBA(nt).go_off;

		/* The replacement might have a right child */
		rh = nh + 1;
		if (rh < lastheap) {
			/* Yes, it does -- maybe the right child is smaller */
			rt = heap_p->theap[rh];
			if (!(TMIS_TCBI(rt)) || (TMTCBA(rt).thi != rh)) {
				// Internal error
				return(TMINTERR9);
			}
			rkey = TMTCBA(rt).go_off;
			if (LTIME(rkey, nkey)) {
				/* It is -- let's descend toward the right */
				nh = rh;
				nt = rt;
				nkey = rkey;
			}
		}

		/* At this point the "replacement" tcb (located in heap-slot h)
		 * has a minimal child, nt (located in heap-slot nh).  To
		 * satisfy the heap constraint, the replacement's "key" must
		 * be less than the child's key (stored in nkey).
		 */

		if (LTIME(nkey, key)) {
			/* No, swap with smaller child denoted by nh/nt */
			LINK2HEAP(heap_p, h, nt);
			LINK2HEAP(heap_p, nh, replacement);
		} else {
			/* Yes, the order is fine -- we are finished */
			return(GLsuccess);
		}
	}
}

/* TMgettcb allocates a TMTCB from the free list and returns its TMTCBI. 
 * In case of error (no free TMTCB's), the negative value GLFAIL is
 * returned.
 */

Long
TMtimers::gettcb()
{
	register Long	t;
	register TMTCB	*tcbptr;
	

	/* Get first free TMTCB */
	t = TMfftcb;
	if (t == TMNULL) {
		/* extra error handling... find tcb hogs?	*/
		return((Short)TMENOTCB);
	}
	tcbptr = &TMTCBA(t);
	if (!(TMIS_TCBI(t)) || (tcbptr->llink != TMNULL)) {
		// Internal error
		return((Short)TMINTERR10);
	}
	if (tcbptr->tstate != TEMPTY) {
		// Internal error
		return((Short)TMINTERR11);
	}
	/* Remove element t from the free list */
	TMfftcb = tcbptr->rlink;
	if (TMfftcb != TMNULL) {
		if (!(TMIS_TCBI(TMfftcb)) || (TMTCBA(TMfftcb).llink != t)) {
			// Internal error
			return((Short)TMINTERR12);
		}
		TMTCBA(TMfftcb).llink = TMNULL;
	} else {
		if (TMlftcb != t) {
			// Internal error
			return((Short)TMINTERR13);
		}
		TMlftcb = TMNULL;
	}

	tcbptr->rlink = TMNULL;
	tcbptr->llink = TMNULL;

	/* The timer is in an undefined state */
	tcbptr->tstate = TLIMBO;

	/* Decrement TMidletcb which indicates the number of TCB's on the TCB
	 * free list.  This variable is also assigned and/or referenced by
	 * TMcntcb(), TMfreetcb(), and TMtiminit().
	 */
	TMidletcb--;


	return (t);
}

/* TMfreetcb frees the TMTCB named by the TMTCBI t, unchaining it from any
 * structures it may be in (i.e., the timing queue and the list of timers
 * owned by a process).
 */

GLretVal
TMtimers::freetcb(Long t)
{
	register TMTCB	*tcbptr = &TMTCBA(t);
	GLretVal	ret;

	/* Verify the TCB owner */

	switch (tcbptr->tstate) {

	case ORTIMER:
	case CRTIMER:
		/* The timer is in the timing queue, so we need to remove it */
		ret = tunsched(t, &TMrtheap);
		if (ret != GLsuccess) {
			// internal error
			return(TMINTERR15);
		}
		break;

	case OATIMER:
	case CATIMER:
		/* The timer is in the timing queue, so we need to remove it */
		ret = tunsched(t, &TMatheap);
		if (ret != GLsuccess) {
			// internal error
			return(TMINTERR27);
		}
		break;

	case TLIMBO:
		/* The timer is not in the timing queue */
		break;

	case TEMPTY:
	default:
		/* We should never see timers in other states */
		return(TMINTERR16);
	}
	tcbptr->thi = TMNULL;

	/* Set the TMTCB to an empty state */
	tcbptr->tstate = TEMPTY;

	/* Link the TMTCB onto the end of the free list */
	tcbptr->llink = TMlftcb;
	tcbptr->rlink = TMNULL;
	if (TMlftcb != TMNULL) {
		if (!(TMIS_TCBI(TMlftcb)) || (TMTCBA(TMlftcb).rlink != TMNULL)) {
			// Internal error
			return(TMINTERR17);
		}
		TMTCBA(TMlftcb).rlink = t;
	} else {
		if (TMfftcb != TMNULL) {
			// Internal error
			return(TMINTERR18);
		}
		TMfftcb = t;
	}
	TMlftcb = t;


	/* Increment TMidletcb which indicates the number of TCB's on the TCB
	 * free list.  This variable is also assigned and/or referenced by
	 * gettcb(), and TMtimInit().
	 */
	TMidletcb++;
	return(GLsuccess);
}

///* tick is called by machine-dependent timer management to signal that time
///* has gone by, in the amount of "interval" milliseconds.
// */
//Void
//TMtimers::tick(Long interval)
//{
//	/* Update the current TMITIME */
//	if (!TMIS_ITIME(TMnow)) {
//		// Internal error
//		return(TMINTERRX);
//	}
//	TMTIMINCR(TMnow, interval);
//	if (TMnow.hi_time < 0) {
//		// Internal error
//		return(TMINTERRX);
//	}
//	return(GLsuccess);
//}

/*
 * TMsectohz converts seconds to clock ticks in TMITIME format
 */
Void
TMtimers::sectohz(Long time,TMITIME *hztime)
{
	unsigned long long	ntime;
	if (time >= seclimit) {
		/*
		 * long long will avoid the possibility of overflow
		 */
		ntime = (unsigned long long)time * TMHZ;
		hztime->hi_time = (Short) (ntime >> TMlo_bits);
		hztime->lo_time = (Long) (ntime & maxhz);
	}
	else {
		/* still requires
		 * a multiply, however and should, therefore only be
		 * used when the number of TMHZ cannot be pre-computed:
		 */
		hztime->hi_time = 0;
		hztime->lo_time = time * TMHZ;
	}
}
