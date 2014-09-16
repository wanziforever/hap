#ifndef SYSENT_H
#define SYSENT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

///* IBM - SK - Don't include Solaris system parameters.
#include <sys/param.h>
// */

/* Added so that we could have strcpy & such, not included, no idea why */
#include <string.h>

/* IBM - SK - Added real definition of strlcpy */

//extern "C"
//{
//#endif

/* Copy no more than N characters of SRC to DEST. But insure output
   is NULL terminated */
//char *strlcpy (char *__restrict __dest,
//		      __const char *__restrict __src, size_t __n);
//#ifdef __cplusplus
//}


#ifndef PAGESIZE
#define PAGESIZE getpagesize()
#endif
#ifndef PAGEOFFSET
#define PAGEOFFSET (PAGESIZE - 1)
#endif
#ifndef PAGEMASK
#define PAGEMASK (ULONG_MAX ^ (PAGEOFFSET))
#endif


/* We can do the following since all uses of memcntl are MC_SYNC.
#define memcntl(addr, len, op, subop, z1, z2) msync(addr, len, (int)subop)
*/

/* The proper thing to do would be to change these to sigactions, but... */
/*#define sigset signal */
/*#define sigrelse(i) 0 */ /* Must do something about */

/* Ok, cc/lib/malloc and INstack back trace use asm!
#define asm(a1)
*/

/* We lock memory with SHM_SHARE_MMU in Solaris, not an option in Linux
#define SHM_SHARE_MMU 0
*/

/* MHinfoExt.C uses msgbuf type; seems to be in here */
/* This should work, but GCC has a problem with it
** typedef struct msgbuf msgbuf;
*/

/* Many people use ascftime, convert to strftime
#define ascftime(tbuf, fmt, tms) strftime(tbuf, 1000, fmt, tms)
*/

/*
#define RLIMIT_VMEM RLIMIT_RSS

#define SIG_PF __sighandler_t
#define SIG_TYP SIG_PF
*/
#endif

