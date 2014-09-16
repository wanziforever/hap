#ifndef _H_LUC_COMPAT
#define _H_LUC_COMPAT

/**********************************************************************
**
** Creation Date:
** Wed Apr 26 12:46:36 CDT 2006
**
** Description:
** Utility functions for porting Solaris to SuSE
**
** Notes:
**
**  *prototypes were copied from the manpages on Solaris
**
** Functions
**     strlcat
**     strlcat
**     cftime
**     ascftime
**
**
**********************************************************************/

#ifdef __linux

/*** includes ***/

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/types.h>
/* swerbner 05/17/06 -- add unistd.h for sysconf() */
#include <unistd.h>
#include <signal.h>

/* 20060906 IM murthya : fcntl.h needed for readelfclass */
#include <fcntl.h>

#include "ibmcm_utils.h"
#include "ibmcm_pthread_utils.h"

/*** types ****/

/* from /usr/include/sys/iso/signal_iso.h on solaris */
typedef __sighandler_t SIG_TYP;
#define SIG_PF SIG_TYP

/* see /usr/include/sys/types.h on solaris */
#if defined(__XOPEN_OR_POSIX)
typedef enum { _B_FALSE, _B_TRUE } boolean_t;
#else
typedef enum { B_FALSE, B_TRUE } boolean_t;
#endif /* defined(__XOPEN_OR_POSIX) */

/*
 * Typedefs for dev_t components.
 */
typedef unsigned int	major_t;	/* major part of device number */
typedef unsigned int	minor_t;	/* minor part of device number */

/* see /usr/include/sys/types.h on solaris */
#if defined(_IBMCM__INT64_TYPE)
typedef int64_t         pad64_t;
#else
typedef union {
        double   _d;
        int32_t  _l[2];
} pad64_t;
#endif

#if defined(_IBMCM_LONGLONG_TYPE)
typedef long long               longlong_t;
typedef unsigned long long      u_longlong_t;
#else
/* used to reserve space and generate alignment */
typedef union {
        double  _d;
        int32_t _l[2];
} longlong_t;
typedef union {
        double          _d;
        uint32_t        _l[2];
} u_longlong_t;
#endif  /* defined(_IBMCM_LONGLONG_TYPE) */

/* taken from sys/types.h on Solaris */
typedef longlong_t      offset_t;
typedef u_longlong_t    u_offset_t;


/* IBM swerbner 20060622 refer to sys/stack.h on solaris */
#ifndef STACK_ALIGN32
#define STACK_ALIGN32 (8)
#endif /* STACK_ALIGN32 */
#ifndef STACK_ALIGN64
#define STACK_ALIGN64 (16)
#endif /* STACK_ALIGN64 */

/* see /usr/include/sys/model.h on solaris */
typedef unsigned int model_t;

/* IBM swerbner 20060712 refer to sys/types32.h on solaris */
/* valid on 32-bit and 64-bit platforms */
#ifndef caddr32_t_defined
typedef unsigned int caddr32_t;
#define caddr32_t_defined
#endif

/* for cc/lib/security/SECpamLogModule.c */
#ifndef SI_HOSTNAME_defined
#define SI_HOSTNAME 2;
#define SI_HOSTNAME_defined
#endif


/* IBM jerrymck 20060523  comment */
/* Added for cc/ss7/3d/ap/base/hdr/APreturnValues.H */
#ifndef Char
typedef char Char;
#endif

/* IBM jerrymck 20060523  comment */
/* Added for ss/s7sch/os/util/event/src/Event.H */
/* ALU krallmann 20070221 No! Definition of Long must always be gotten
/* from GLtypes, not from here. having it here breaks msgutils. */
/* #ifndef Long */
/* typedef long Long; */
/* #endif */

/*** prototypes ***/


/* string.h  */
#ifdef __cplusplus
extern "C" {
#endif
extern size_t strlcat(char *dst, const char *src, size_t dstsize);
extern size_t strlcpy(char *dst, const char *src, size_t dstsize);

/* time.h */
extern int cftime(char *s, char *format, const time_t *clock);
extern int ascftime(char *s, const char *format,  const  struct  tm *timeptr);

/* /usr/include/stdlib.h */
extern char *lltostr(long long val, char *endptr);
char *ulltostr(unsigned long long val, char *endptr);
#ifdef __cplusplus
};
#endif

/* utility routines */
/* gets a record from a file via fgets() - returns 0 if fgets() returns
        nothing, otherwise returns the strlen() of the record */
int getrecord(FILE *fp, char *record);

/* trims whitespace characters from the left and right.  returns the
         strlen() of the trimmed buffer */
int trim_whitespace(char *buf);



/*** constants ***/

/* see /usr/include/values.h on solaris (swerbner -- 05/15/06) */
/*
 * Note: linux has similar but NOT identically named definitions in
 * /usr/include/limits.h (swerbner -- 05/17/06)
 */

#ifndef MAXSHORT
#define MAXSHORT (0x7fff)
#endif
#ifndef HIBITI  /* high order integer bit (32-bit integer) */
#define HIBITI (1<<31)
#endif
#ifndef MAXINT
#define MAXINT (0x7fffffff)
#endif

/* undefined SHARED MEMORY flag */
#define SHM_SHARE_MMU 0
/*
 * see /usr/include/iso/stdio_iso.h and /usr/include/stdio.h on solaris
 * swerbner 05/17/06
 */
#ifndef _NFILE
#define _NFILE (FOPEN_MAX)
#endif

/* was in ulimits.h on sun */
#ifndef UL_GMEMLIM
#define UL_GMEMLIM 3
#endif

/* see /usr/include/netdb.h on solaris wyoes - 05/11/2006 */
#ifndef AI_DEFAULT
#define    AI_DEFAULT      (AI_V4MAPPED | AI_ADDRCONFIG)
#endif

/* see /usr/include/sys/param.h on solaris - swerbner 05/17/06 */
#ifndef PAGESIZE
#define PAGESIZE (sysconf(_SC_PAGESIZE))
#endif
#ifndef PAGEOFFSET
#define PAGEOFFSET (PAGESIZE - 1)
#endif
#ifndef PAGEMASK
#define PAGEMASK (~PAGEOFFSET)
#endif

/* see /usr/include/sys/param.h on solaris - swerbner 07/06/06 */
/*
 * MAXNAMELEN is the length (including the terminating null) of
 * the longest permissible file (component) name.
 */
#ifndef MAXNAMELEN
#define MAXNAMELEN 256
#endif

/*
 * see /usr/include/sys/param.h on solaris - swerbner 05/22/06
 * This constant is defined in <bits/xopen_lim.h> (used by <limits.h> on linux
 * but is blocked from our compilations since __USE_XOPEN is NOT defined.
 */
#ifndef NZERO
#define NZERO           20	/* Default process priority.  */
#endif


/*
 * see /usr/include/sys/procfs.h on solaris - swerbner 05/23/06
 */
#ifndef PRFNSZ
#define PRFNSZ          16      /* Maximum size of execed filename */
#endif
#ifndef PRARGSZ
#define PRARGSZ         80      /* number of chars of arguments */
#endif
#ifndef PRCLSZ
#define PRCLSZ          8       /* maximum size of scheduling class name */
#endif

/* This is from /usr/include/curses.h */
#define resetterm               reset_shell_mode

/* This is from /usr/include/sys/telioctl.h */
#ifndef TEL_BINARY_IN
#define TEL_BINARY_IN   1
#endif
#ifndef TEL_BINARY_OUT
#define TEL_BINARY_OUT  2
#endif
#ifndef TELIOC
#define TELIOC                  ('n' << 8)
#endif
#ifndef TEL_IOC_ENABLE
#define TEL_IOC_ENABLE          (TELIOC|2)
#endif
#ifndef TEL_IOC_MODE
#define TEL_IOC_MODE            (TELIOC|3)
#endif
#ifndef TEL_IOC_GETBLK
#define TEL_IOC_GETBLK          (TELIOC|4)
#endif


/* this stuff is for the psinfo_t structure */

typedef unsigned short ushort_t;
typedef id_t taskid_t;
typedef id_t projid_t;
typedef id_t poolid_t;
typedef id_t pr_zoneid;
typedef int processorid_t;
typedef int psetid_t;
typedef id_t zoneid_t;

/* This structure is not currently populated 
   see solaris:/usr/include/sys/procfs.h */
typedef struct {
  int     pr_flag;        /* lwp flags (DEPRECATED; do not use) */
  id_t    pr_lwpid;       /* lwp id */
  uintptr_t pr_addr;      /* internal address of lwp */
  uintptr_t pr_wchan;     /* wait addr for sleeping lwp */
  char    pr_stype;       /* synchronization event type */
  char    pr_state;       /* numeric lwp state */
  char    pr_sname;       /* printable character for pr_state */
  char    pr_nice;        /* nice for cpu usage */
  short   pr_syscall;     /* system call number (if in syscall) */
  char    pr_oldpri;      /* pre-SVR4, low value is high priority */
  char    pr_cpu;         /* pre-SVR4, cpu usage for scheduling */
  int     pr_pri;         /* priority, high value is high priority */
          /* The following percent number is a 16-bit binary */
          /* fraction [0 .. 1] with the binary point to the */
          /* right of the high-order bit (1.0 == 0x8000) */
  ushort_t pr_pctcpu;     /* % of recent cpu time used by this lwp */
  ushort_t pr_pad;
  timestruc_t pr_start;   /* lwp start time, from the epoch */
  timestruc_t pr_time;    /* usr+sys cpu time for this lwp */
  char    pr_clname[PRCLSZ];      /* scheduling class name */
  char    pr_name[PRFNSZ];        /* name of system lwp */
  processorid_t pr_onpro;         /* processor which last ran this lwp */
  processorid_t pr_bindpro;       /* processor to which lwp is bound */
  psetid_t pr_bindpset;   /* processor set to which lwp is bound */
  int     pr_filler[5];   /* reserved for future use */
} lwpsinfo_t;

/* fields with a comment starting with a "1" indicate a supported field */
/* see /usr/include/sys/procfs.h */
typedef struct  {
  int     pr_flag;        /* process flags (DEPRECATED; do not use) */
  int     pr_nlwp;        /* number of active lwps in the process */
  pid_t   pr_pid;         /*1 unique process id */
  pid_t   pr_ppid;        /*1 process id of parent */
  pid_t   pr_pgid;        /* pid of process group leader */
  pid_t   pr_sid;         /*1 session id */
  uid_t   pr_uid;         /* real user id */
  uid_t   pr_euid;        /* effective user id */
  gid_t   pr_gid;         /* real group id */
  gid_t   pr_egid;        /* effective group id */
  uintptr_t pr_addr;      /*1 address of process */
  size_t  pr_size;        /*1 size of process image in Kbytes */
  size_t  pr_rssize;      /*1 resident set size in Kbytes */
  size_t  pr_pad1;
  dev_t   pr_ttydev;      /*1 controlling tty device (or PRNODEV) */
          /* The following percent numbers are 16-bit binary */
          /* fractions [0 .. 1] with the binary point to the */
          /* right of the high-order bit (1.0 == 0x8000) */
  ushort_t pr_pctcpu;     /* % of recent cpu time used by all lwps */
  ushort_t pr_pctmem;     /* % of system memory used by process */
  timestruc_t pr_start;   /* process start time, from the epoch */
  timestruc_t pr_time;    /* usr+sys cpu time for this process */
  timestruc_t pr_ctime;   /* usr+sys cpu time for reaped children */
  char    pr_fname[PRFNSZ];       /*1 name of execed file */
  char    pr_psargs[PRARGSZ];     /*1 initial characters of arg list */
  int     pr_wstat;       /* if zombie, the wait() status */
  int     pr_argc;        /*1 initial argument count */
  uintptr_t pr_argv;      /* address of initial argument vector */
  uintptr_t pr_envp;      /* address of initial environment vector */
  char    pr_dmodel;      /* data model of the process */
  char    pr_pad2[3];
  taskid_t pr_taskid;     /* task id */
  projid_t pr_projid;     /* project id */
  int     pr_nzomb;       /* number of zombie lwps in the process */
  poolid_t pr_poolid;     /* pool id */
  zoneid_t pr_zoneid;     /* zone id */
  id_t    pr_contract;    /* process contract */
  int     pr_filler[1];   /* reserved for future use */
  lwpsinfo_t pr_lwp;      /* information for representative lwp */
} psinfo_t;

/* 20060906 IBM murthya : include pstatus_t for compatibility */
/*
 * process status file.  /proc/<pid>/status
 */
typedef struct pstatus {
        int     pr_flags;       /* flags (see below) */
        int     pr_nlwp;        /* number of active lwps in the process */
        pid_t   pr_pid;         /* process id */
        pid_t   pr_ppid;        /* parent process id */
        pid_t   pr_pgid;        /* process group id */
        pid_t   pr_sid;         /* session id */
        pid_t    pr_aslwpid;     /* historical; now always zero */
        pid_t    pr_agentid;     /* lwp id of the /proc agent lwp, if any */
        sigset_t pr_sigpend;    /* set of process pending signals */
        uintptr_t pr_brkbase;   /* address of the process heap */
        size_t  pr_brksize;     /* size of the process heap, in bytes */
        uintptr_t pr_stkbase;   /* address of the process stack */
        size_t  pr_stksize;     /* size of the process stack, in bytes */
        timestruc_t pr_utime;   /* process user cpu time */
        timestruc_t pr_stime;   /* process system cpu time */
        timestruc_t pr_cutime;  /* sum of children's user times */
        timestruc_t pr_cstime;  /* sum of children's system times */
 //       sigset_t pr_sigtrace;   /* set of traced signals */
  //      fltset_t pr_flttrace;   /* set of traced faults */
   //     sysset_t pr_sysentry;   /* set of system calls traced on entry */
  //      sysset_t pr_sysexit;    /* set of system calls traced on exit */
        char    pr_dmodel;      /* data model of the process (see below) */
        char    pr_pad[3];
        taskid_t pr_taskid;     /* task id */
        projid_t pr_projid;     /* project id */
        int     pr_nzomb;       /* number of zombie lwps in the process */
        zoneid_t pr_zoneid;     /* zone id */
        int     pr_filler[15];  /* reserved for future use */
        //lwpstatus_t pr_lwp;     /* status of the representative lwp */
} pstatus_t ;

/* prints a psinfo_t structure to stdout - returns 0 */
int dump_psinfo_t(psinfo_t *psinfo);

/* populates a psinfo_t - this is only partly implemented - see psinfo_t
   definition for supported fields
   returns 0 for success, 1 for an error getting stat data,
   returns 2 for an error getting the status data,
   returns 3 for a problem opening the /proc/<PID>/cmdline file
*/
int populate_psinfo(pid_t *pid, psinfo_t *ptr);
/* end of psinfo */

/* These macros are from the generic.h file on Solaris */

// token-pasting macros; ANSI requires an extra level of indirection
#define name2(z, y)             name2_x(z, y)
#define name2_x(z, y)           z##y
#define name3(z, y, x)          name3_x(z, y, x)
#define name3_x(z, y, x)        z##y##x
#define name4(z, y, x, w)       name4_x(z, y, x, w)
#define name4_x(z, y, x, w)     z##y##x##w

// macros for declaring and implementing classes
#define declare(z, y)           name2(z, declare)(y)
#define implement(z, y)         name2(z, implement)(y)
#define declare2(z, y, x)       name2(z, declare2)(y, x)
#define implement2(z, y, x)     name2(z, implement2)(y, x)

// macros for declaring error-handling functions
extern void genericerror(int, char*);
typedef int (*GPT)(int, char*);
#define set_handler(gen, tp, z) name4(set_, tp, gen, _handler)(z)
#define errorhandler(gen, tp)   name3(tp, gen, handler)
#define callerror(gen, tp, z, y) (*errorhandler(gen, tp))(z, y)

/* end generic.h entries */

/* IBM 20060718 murthya : typedefs needed across files in MSmt 

  uint_t, uchar_t reproduced from /usr/include/sys/types.h on solaris
  kid_t from sys/kstat.h

*/

typedef unsigned int uint_t;
typedef unsigned char uchar_t;
typedef int kid_t ;


/* IBM 20060714 murthya - duplicate NANOSEC from sys/time.h on solaris */
#ifndef    NANOSEC
#define    NANOSEC         1000000000
#endif
#ifndef    MILLISEC
#define    MILLISEC        1000
#endif
#ifndef    MICROSEC
#define    MICROSEC        1000000
#endif

/* IBM 20060718 murthya : Givev below are relevant structures copied over from
   sysinfo.h on Solaris
*/
#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _SYS_SYSINFO_FROMSOL_H
#define _SYS_SYSINFO_FROMSOL_H

/*   murthya : BEGIN : Contents from sysinfo.h */
/*
 *	System Information.
 */
#define	CPU_IDLE	0
#define	CPU_USER	1
#define	CPU_KERNEL	2
#define	CPU_WAIT	3
#define	CPU_STATES	4

#define	W_IO		0
#define	W_SWAP		1
#define	W_PIO		2
#define	W_STATES	3

typedef struct cpu_sysinfo {
	uint_t	cpu[CPU_STATES]; /* CPU utilization			*/
	uint_t	wait[W_STATES];	/* CPU wait time breakdown		*/
	uint_t	bread;		/* physical block reads			*/
	uint_t	bwrite;		/* physical block writes (sync+async)	*/
	uint_t	lread;		/* logical block reads			*/
	uint_t	lwrite;		/* logical block writes			*/
	uint_t	phread;		/* raw I/O reads			*/
	uint_t	phwrite;	/* raw I/O writes			*/
	uint_t	pswitch;	/* context switches			*/
	uint_t	trap;		/* traps				*/
	uint_t	intr;		/* device interrupts			*/
	uint_t	syscall;	/* system calls				*/
	uint_t	sysread;	/* read() + readv() system calls	*/
	uint_t	syswrite;	/* write() + writev() system calls	*/
	uint_t	sysfork;	/* forks				*/
	uint_t	sysvfork;	/* vforks				*/
	uint_t	sysexec;	/* execs				*/
	uint_t	readch;		/* bytes read by rdwr()			*/
	uint_t	writech;	/* bytes written by rdwr()		*/
	uint_t	rcvint;		/* XXX: UNUSED				*/
	uint_t	xmtint;		/* XXX: UNUSED				*/
	uint_t	mdmint;		/* XXX: UNUSED				*/
	uint_t	rawch;		/* terminal input characters		*/
	uint_t	canch;		/* chars handled in canonical mode	*/
	uint_t	outch;		/* terminal output characters		*/
	uint_t	msg;		/* msg count (msgrcv()+msgsnd() calls)	*/
	uint_t	sema;		/* semaphore ops count (semop() calls)	*/
	uint_t	namei;		/* pathname lookups			*/
	uint_t	ufsiget;	/* ufs_iget() calls			*/
	uint_t	ufsdirblk;	/* directory blocks read		*/
	uint_t	ufsipage;	/* inodes taken with attached pages	*/
	uint_t	ufsinopage;	/* inodes taked with no attached pages	*/
	uint_t	inodeovf;	/* inode table overflows		*/
	uint_t	fileovf;	/* file table overflows			*/
	uint_t	procovf;	/* proc table overflows			*/
	uint_t	intrthread;	/* interrupts as threads (below clock)	*/
	uint_t	intrblk;	/* intrs blkd/prempted/released (swtch)	*/
	uint_t	idlethread;	/* times idle thread scheduled		*/
	uint_t	inv_swtch;	/* involuntary context switches		*/
	uint_t	nthreads;	/* thread_create()s			*/
	uint_t	cpumigrate;	/* cpu migrations by threads 		*/
	uint_t	xcalls;		/* xcalls to other cpus 		*/
	uint_t	mutex_adenters;	/* failed mutex enters (adaptive)	*/
	uint_t	rw_rdfails;	/* rw reader failures			*/
	uint_t	rw_wrfails;	/* rw writer failures			*/
	uint_t	modload;	/* times loadable module loaded		*/
	uint_t	modunload;	/* times loadable module unloaded 	*/
	uint_t	bawrite;	/* physical block writes (async)	*/
/* Following are gathered only under #ifdef STATISTICS in source 	*/
	uint_t	rw_enters;	/* tries to acquire rw lock		*/
	uint_t	win_uo_cnt;	/* reg window user overflows		*/
	uint_t	win_uu_cnt;	/* reg window user underflows		*/
	uint_t	win_so_cnt;	/* reg window system overflows		*/
	uint_t	win_su_cnt;	/* reg window system underflows		*/
	uint_t	win_suo_cnt;	/* reg window system user overflows	*/
} cpu_sysinfo_t;

/* IBM 20060714 murthya : definition of sysinfo_t removed from below*/

typedef struct cpu_syswait {
	int	iowait;		/* procs waiting for block I/O		*/
	int	swap;		/* XXX: UNUSED				*/
	int	physio;		/* XXX: UNUSED 				*/
} cpu_syswait_t;

typedef struct cpu_vminfo {
	uint_t	pgrec;		/* page reclaims (includes pageout)	*/
	uint_t	pgfrec;		/* page reclaims from free list		*/
	uint_t	pgin;		/* pageins				*/
	uint_t	pgpgin;		/* pages paged in			*/
	uint_t	pgout;		/* pageouts				*/
	uint_t	pgpgout;	/* pages paged out			*/
	uint_t	swapin;		/* swapins				*/
	uint_t	pgswapin;	/* pages swapped in			*/
	uint_t	swapout;	/* swapouts				*/
	uint_t	pgswapout;	/* pages swapped out			*/
	uint_t	zfod;		/* pages zero filled on demand		*/
	uint_t	dfree;		/* pages freed by daemon or auto	*/
	uint_t	scan;		/* pages examined by pageout daemon	*/
	uint_t	rev;		/* revolutions of the page daemon hand	*/
	uint_t	hat_fault;	/* minor page faults via hat_fault()	*/
	uint_t	as_fault;	/* minor page faults via as_fault()	*/
	uint_t	maj_fault;	/* major page faults			*/
	uint_t	cow_fault;	/* copy-on-write faults			*/
	uint_t	prot_fault;	/* protection faults			*/
	uint_t	softlock;	/* faults due to software locking req	*/
	uint_t	kernel_asflt;	/* as_fault()s in kernel addr space	*/
	uint_t	pgrrun;		/* times pager scheduled		*/
	uint_t  execpgin;	/* executable pages paged in		*/
	uint_t  execpgout;	/* executable pages paged out		*/
	uint_t  execfree;	/* executable pages freed		*/
	uint_t  anonpgin;	/* anon pages paged in			*/
	uint_t  anonpgout;	/* anon pages paged out			*/
	uint_t  anonfree;	/* anon pages freed			*/
	uint_t  fspgin;		/* fs pages paged in			*/
	uint_t  fspgout;	/* fs pages paged out			*/
	uint_t  fsfree;		/* fs pages free			*/
} cpu_vminfo_t;

typedef struct vminfo {		/* (update freq) update action		*/
	uint64_t freemem; 	/* (1 sec) += freemem in pages		*/
	uint64_t swap_resv;	/* (1 sec) += reserved swap in pages	*/
	uint64_t swap_alloc;	/* (1 sec) += allocated swap in pages	*/
	uint64_t swap_avail;	/* (1 sec) += unreserved swap in pages	*/
	uint64_t swap_free;	/* (1 sec) += unallocated swap in pages	*/
} vminfo_t;

typedef struct cpu_stat {
	uint_t		__cpu_stat_lock[2];	/* 32-bit kstat compat. */
	cpu_sysinfo_t	cpu_sysinfo;
	cpu_syswait_t	cpu_syswait;
	cpu_vminfo_t	cpu_vminfo;
} cpu_stat_t;
/* IBM 20060714 murthya : definition of cpu_sys_stats removed from below */

typedef struct cpu_vm_stats {
	uint64_t pgrec;			/* page reclaims (includes pageout) */
	uint64_t pgfrec;		/* page reclaims from free list */
	uint64_t pgin;			/* pageins */
	uint64_t pgpgin;		/* pages paged in */
	uint64_t pgout;			/* pageouts */
	uint64_t pgpgout;		/* pages paged out */
	uint64_t swapin;		/* swapins */
	uint64_t pgswapin;		/* pages swapped in */
	uint64_t swapout;		/* swapouts */
	uint64_t pgswapout;		/* pages swapped out */
	uint64_t zfod;			/* pages zero filled on demand */
	uint64_t dfree;			/* pages freed by daemon or auto */
	uint64_t scan;			/* pages examined by pageout daemon */
	uint64_t rev;			/* revolutions of page daemon hand */
	uint64_t hat_fault;		/* minor page faults via hat_fault() */
	uint64_t as_fault;		/* minor page faults via as_fault() */
	uint64_t maj_fault;		/* major page faults */
	uint64_t cow_fault;		/* copy-on-write faults */
	uint64_t prot_fault;		/* protection faults */
	uint64_t softlock;		/* faults due to software locking req */
	uint64_t kernel_asflt;		/* as_fault()s in kernel addr space */
	uint64_t pgrrun;		/* times pager scheduled */
	uint64_t execpgin;		/* executable pages paged in */
	uint64_t execpgout;		/* executable pages paged out */
	uint64_t execfree;		/* executable pages freed */
	uint64_t anonpgin;		/* anon pages paged in */
	uint64_t anonpgout;		/* anon pages paged out */
	uint64_t anonfree;		/* anon pages freed */
	uint64_t fspgin;		/* fs pages paged in */
	uint64_t fspgout;		/* fs pages paged out */
	uint64_t fsfree;		/* fs pages free */
} cpu_vm_stats_t;

/* IBM 20060731 murthya : add ipaddr_t - borrowed from inet/ip.h on solaris*/
#ifndef _IPADDR_T
#define _IPADDR_T
typedef uint32_t ipaddr_t;
#endif


/* 20060906 IBM murthya : BEGIN :  #defines needed to read a binary file's 'class' */

/* magic number */
#define EI_MAG0         0               /* e_ident[] indexes */
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4

/* elf class */
#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

#define ELFMAG0         0x7f            /* EI_MAG */
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'
 
#define O_BINARY 0

#define       OLFMAG1         'O'

/* 20061013 IBM murthya : define POLLIN, POLLOUT and POLLREMOVE for use by
   epoll-related code
*/
#define POLLIN 0x0001
#define POLLOUT 0x0004
#define POLLREMOVE 0x0800

/* 20060925 IBM murthya : number of signals in a sigset. copied over from /usr/include/signal.h
   on sun
*/
#define SIG2STR_MAX     32

/* 20060906 IBM murthya : END : #defines needed to read a binary file's 'class' */

#define IN6_V4MAPPED_TO_INADDR( v6, v4 ) \
        ((v4)->s_addr = ((__const uint32_t *) (v6))[3])


/* murthya : The following endif is for #define _SYS_SYSINFO_FROMSOL_H */
#endif 
/*   murthya : END : Contents from sysinfo.h */

/* IBM 20060718 murthya : Givev below are relevant structures copied over from
   sys/kstat.h on Solaris. These need to precede the contents from kstat.h, that are
   included later in the file */

/* murthya : BEGIN : contents from sys/kstat.h */
#ifndef _SYS_KSTAT_FROMSOL_H
#define _SYS_KSTAT_FROMSOL_H

/* Almost the entire contents being copied here on 20060718, to checkin the file in time for a build.
   Irrelvant contents will be cleaned up */
   
/* IBM 20060714 murthya :  QQ: typedef u_longlong_sol_t for linux 
	On Solaris, this file included (as can be seen above), sys/types.h that
   defined u_longlong_t as 'typedef unsigned long long u_longlong_t'. However,
   That type is already being defined by luc_compat.h with _IBMCM_LONGLONG_TYPE
   UNdefined. This is causing conflicts on the name u_longlong_t. since this file
   was borrowed from solaris, just add a typedef specific to solaris

  Revisit the issue later to just continue to use the def in luc_compat.h
*/

typedef unsigned long long u_longlong_sol_t ;


/*
 * Kernel statistics driver (/dev/kstat) ioctls
 */

#define	KSTAT_IOC_BASE		('K' << 8)

#define	KSTAT_IOC_CHAIN_ID	KSTAT_IOC_BASE | 0x01
#define	KSTAT_IOC_READ		KSTAT_IOC_BASE | 0x02
#define	KSTAT_IOC_WRITE		KSTAT_IOC_BASE | 0x03

/*
 * /dev/kstat ioctl usage (kd denotes /dev/kstat descriptor):
 *
 *	kcid = ioctl(kd, KSTAT_IOC_CHAIN_ID, NULL);
 *	kcid = ioctl(kd, KSTAT_IOC_READ, kstat_t *);
 *	kcid = ioctl(kd, KSTAT_IOC_WRITE, kstat_t *);
 */

#define	KSTAT_STRLEN	31	/* 30 chars + NULL; must be 16 * n - 1 */

/*
 * The generic kstat header
 */

typedef struct kstat {
	/*
	 * Fields relevant to both kernel and user
	 */
	hrtime_t	ks_crtime;	/* creation time (from gethrtime()) */
	struct kstat	*ks_next;	/* kstat chain linkage */
	kid_t		ks_kid;		/* unique kstat ID */
	char		ks_module[KSTAT_STRLEN]; /* provider module name */
	uchar_t		ks_resv;	/* reserved, currently just padding */
	int		ks_instance;	/* provider module's instance */
	char		ks_name[KSTAT_STRLEN]; /* kstat name */
	uchar_t		ks_type;	/* kstat data type */
	char		ks_class[KSTAT_STRLEN]; /* kstat class */
	uchar_t		ks_flags;	/* kstat flags */
	void		*ks_data;	/* kstat type-specific data */
	uint_t		ks_ndata;	/* # of type-specific data records */
	size_t		ks_data_size;	/* total size of kstat data section */
	hrtime_t	ks_snaptime;	/* time of last data shapshot */
	/*
	 * Fields relevant to kernel only
	 */
	int		(*ks_update)(struct kstat *, int); /* dynamic update */
	void		*ks_private;	/* arbitrary provider-private data */
	int		(*ks_snapshot)(struct kstat *, void *, int);
	void		*ks_lock;	/* protects this kstat's data */
} kstat_t;

#ifdef _SYSCALL32

typedef int32_t kid32_t;

typedef struct kstat32 {
	/*
	 * Fields relevant to both kernel and user
	 */
	hrtime_t	ks_crtime;
	caddr32_t	ks_next;		/* struct kstat pointer */
	kid32_t		ks_kid;
	char		ks_module[KSTAT_STRLEN];
	uint8_t		ks_resv;
	int32_t		ks_instance;
	char		ks_name[KSTAT_STRLEN];
	uint8_t		ks_type;
	char		ks_class[KSTAT_STRLEN];
	uint8_t		ks_flags;
	caddr32_t	ks_data;		/* type-specific data */
	uint32_t	ks_ndata;
	size32_t	ks_data_size;
	hrtime_t	ks_snaptime;
	/*
	 * Fields relevant to kernel only (only needed here for padding)
	 */
	int32_t		_ks_update;
	caddr32_t	_ks_private;
	int32_t		_ks_snapshot;
	caddr32_t	_ks_lock;
} kstat32_t;

#endif	/* _SYSCALL32 */

/*
 * kstat structure and locking strategy
 *
 * Each kstat consists of a header section (a kstat_t) and a data section.
 * The system maintains a set of kstats, protected by kstat_chain_lock.
 * kstat_chain_lock protects all additions to/deletions from this set,
 * as well as all changes to kstat headers.  kstat data sections are
 * *optionally* protected by the per-kstat ks_lock.  If ks_lock is non-NULL,
 * kstat clients (e.g. /dev/kstat) will acquire this lock for all of their
 * operations on that kstat.  It is up to the kstat provider to decide whether
 * guaranteeing consistent data to kstat clients is sufficiently important
 * to justify the locking cost.  Note, however, that most statistic updates
 * already occur under one of the provider's mutexes, so if the provider sets
 * ks_lock to point to that mutex, then kstat data locking is free.
 *
 * NOTE: variable-size kstats MUST employ kstat data locking, to prevent
 * data-size races with kstat clients.
 *
 * NOTE: ks_lock is really of type (kmutex_t *); it is declared as (void *)
 * in the kstat header so that users don't have to be exposed to all of the
 * kernel's lock-related data structures.
 */

#if	defined(_KERNEL)

#define	KSTAT_ENTER(k)	\
	{ kmutex_t *lp = (k)->ks_lock; if (lp) mutex_enter(lp); }

#define	KSTAT_EXIT(k)	\
	{ kmutex_t *lp = (k)->ks_lock; if (lp) mutex_exit(lp); }

#define	KSTAT_UPDATE(k, rw)		(*(k)->ks_update)((k), (rw))

#define	KSTAT_SNAPSHOT(k, buf, rw)	(*(k)->ks_snapshot)((k), (buf), (rw))

#endif	/* defined(_KERNEL) */

/*
 * kstat time
 *
 * All times associated with kstats (e.g. creation time, snapshot time,
 * kstat_timer_t and kstat_io_t timestamps, etc.) are 64-bit nanosecond values,
 * as returned by gethrtime().  The accuracy of these timestamps is machine
 * dependent, but the precision (units) is the same across all platforms.
 */

/*
 * kstat identity (KID)
 *
 * Each kstat is assigned a unique KID (kstat ID) when it is added to the
 * global kstat chain.  The KID is used as a cookie by /dev/kstat to
 * request information about the corresponding kstat.  There is also
 * an identity associated with the entire kstat chain, kstat_chain_id,
 * which is bumped each time a kstat is added or deleted.  /dev/kstat uses
 * the chain ID to detect changes in the kstat chain (e.g., a new disk
 * coming online) between ioctl()s.
 */

/*
 * kstat module, kstat instance
 *
 * ks_module and ks_instance contain the name and instance of the module
 * that created the kstat.  In cases where there can only be one instance,
 * ks_instance is 0.  The kernel proper (/kernel/unix) uses "unix" as its
 * module name.
 */

/*
 * kstat name
 *
 * ks_name gives a meaningful name to a kstat.  The full kstat namespace
 * is module.instance.name, so the name only need be unique within a
 * module.  kstat_create() will fail if you try to create a kstat with
 * an already-used (ks_module, ks_instance, ks_name) triplet.  Spaces are
 * allowed in kstat names, but strongly discouraged, since they hinder
 * awk-style processing at user level.
 */

/*
 * kstat type
 *
 * The kstat mechanism provides several flavors of kstat data, defined
 * below.  The "raw" kstat type is just treated as an array of bytes; you
 * can use this to export any kind of data you want.
 *
 * Some kstat types allow multiple data structures per kstat, e.g.
 * KSTAT_TYPE_NAMED; others do not.  This is part of the spec for each
 * kstat data type.
 *
 * User-level tools should *not* rely on the #define KSTAT_NUM_TYPES.  To
 * get this information, read out the standard system kstat "kstat_types".
 */

#define	KSTAT_TYPE_RAW		0	/* can be anything */
					/* ks_ndata >= 1 */
#define	KSTAT_TYPE_NAMED	1	/* name/value pair */
					/* ks_ndata >= 1 */
#define	KSTAT_TYPE_INTR		2	/* interrupt statistics */
					/* ks_ndata == 1 */
#define	KSTAT_TYPE_IO		3	/* I/O statistics */
					/* ks_ndata == 1 */
#define	KSTAT_TYPE_TIMER	4	/* event timer */
					/* ks_ndata >= 1 */

#define	KSTAT_NUM_TYPES		5

/*
 * kstat class
 *
 * Each kstat can be characterized as belonging to some broad class
 * of statistics, e.g. disk, tape, net, vm, streams, etc.  This field
 * can be used as a filter to extract related kstats.  The following
 * values are currently in use: disk, tape, net, controller, vm, kvm,
 * hat, streams, kstat, and misc.  (The kstat class encompasses things
 * like kstat_types.)
 */

/*
 * kstat flags
 *
 * Any of the following flags may be passed to kstat_create().  They are
 * all zero by default.
 *
 *	KSTAT_FLAG_VIRTUAL:
 *
 *		Tells kstat_create() not to allocate memory for the
 *		kstat data section; instead, you will set the ks_data
 *		field to point to the data you wish to export.  This
 *		provides a convenient way to export existing data
 *		structures.
 *
 *	KSTAT_FLAG_VAR_SIZE:
 *
 *		The size of the kstat you are creating will vary over time.
 *		For example, you may want to use the kstat mechanism to
 *		export a linked list.  NOTE: The kstat framework does not
 *		manage the data section, so all variable-size kstats must be
 *		virtual kstats.  Moreover, variable-size kstats MUST employ
 *		kstat data locking to prevent data-size races with kstat
 *		clients.  See the section on "kstat snapshot" for details.
 *
 *	KSTAT_FLAG_WRITABLE:
 *
 *		Makes the kstat's data section writable by root.
 *		The ks_snapshot routine (see below) does not need to check for
 *		this; permission checking is handled in the kstat driver.
 *
 *	KSTAT_FLAG_PERSISTENT:
 *
 *		Indicates that this kstat is to be persistent over time.
 *		For persistent kstats, kstat_delete() simply marks the
 *		kstat as dormant; a subsequent kstat_create() reactivates
 *		the kstat.  This feature is provided so that statistics
 *		are not lost across driver close/open (e.g., raw disk I/O
 *		on a disk with no mounted partitions.)
 *		NOTE: Persistent kstats cannot be virtual, since ks_data
 *		points to garbage as soon as the driver goes away.
 *
 * The following flags are maintained by the kstat framework:
 *
 *	KSTAT_FLAG_DORMANT:
 *
 *		For persistent kstats, indicates that the kstat is in the
 *		dormant state (e.g., the corresponding device is closed).
 *
 *	KSTAT_FLAG_INVALID:
 *
 *		This flag is set when a kstat is in a transitional state,
 *		e.g. between kstat_create() and kstat_install().
 *		kstat clients must not attempt to access the kstat's data
 *		if this flag is set.
 */

#define	KSTAT_FLAG_VIRTUAL		0x01
#define	KSTAT_FLAG_VAR_SIZE		0x02
#define	KSTAT_FLAG_WRITABLE		0x04
#define	KSTAT_FLAG_PERSISTENT		0x08
#define	KSTAT_FLAG_DORMANT		0x10
#define	KSTAT_FLAG_INVALID		0x20

/*
 * Dynamic update support
 *
 * The kstat mechanism allows for an optional ks_update function to update
 * kstat data.  This is useful for drivers where the underlying device
 * keeps cheap hardware stats, but extraction is expensive.  Instead of
 * constantly keeping the kstat data section up to date, you can supply a
 * ks_update function which updates the kstat's data section on demand.
 * To take advantage of this feature, simply set the ks_update field before
 * calling kstat_install().
 *
 * The ks_update function, if supplied, must have the following structure:
 *
 *	int
 *	foo_kstat_update(kstat_t *ksp, int rw)
 *	{
 *		if (rw == KSTAT_WRITE) {
 *			... update the native stats from ksp->ks_data;
 *				return EACCES if you don't support this
 *		} else {
 *			... update ksp->ks_data from the native stats
 *		}
 *	}
 *
 * The ks_update return codes are: 0 for success, EACCES if you don't allow
 * KSTAT_WRITE, and EIO for any other type of error.
 *
 * In general, the ks_update function may need to refer to provider-private
 * data; for example, it may need a pointer to the provider's raw statistics.
 * The ks_private field is available for this purpose.  Its use is entirely
 * at the provider's discretion.
 *
 * All variable-size kstats MUST supply a ks_update routine, which computes
 * and sets ks_data_size (and ks_ndata if that is meaningful), since these
 * are needed to perform kstat snapshots (see below).
 *
 * No kstat locking should be done inside the ks_update routine.  The caller
 * will already be holding the kstat's ks_lock (to ensure consistent data).
 */

#define	KSTAT_READ	0
#define	KSTAT_WRITE	1

/*
 * Kstat snapshot
 *
 * In order to get a consistent view of a kstat's data, clients must obey
 * the kstat's locking strategy.  However, these clients may need to perform
 * operations on the data which could cause a fault (e.g. copyout()), or
 * operations which are simply expensive.  Doing so could cause deadlock
 * (e.g. if you're holding a disk's kstat lock which is ultimately required
 * to resolve a copyout() fault), performance degradation (since the providers'
 * activity is serialized at the kstat lock), device timing problems, etc.
 *
 * To avoid these problems, kstat data is provided via snapshots.  Taking
 * a snapshot is a simple process: allocate a wired-down kernel buffer,
 * acquire the kstat's data lock, copy the data into the buffer ("take the
 * snapshot"), and release the lock.  This ensures that the kstat's data lock
 * will be held as briefly as possible, and that no faults will occur while
 * the lock is held.
 *
 * Normally, the snapshot is taken by default_kstat_snapshot(), which
 * timestamps the data (sets ks_snaptime), copies it, and does a little
 * massaging to deal with incomplete transactions on i/o kstats.  However,
 * this routine only works for kstats with contiguous data (the typical case).
 * If you create a kstat whose data is, say, a linked list, you must provide
 * your own ks_snapshot routine.  The routine you supply must have the
 * following prototype (replace "foo" with something appropriate):
 *
 *	int foo_kstat_snapshot(kstat_t *ksp, void *buf, int rw);
 *
 * The minimal snapshot routine -- one which copies contiguous data that
 * doesn't need any massaging -- would be this:
 *
 *	ksp->ks_snaptime = gethrtime();
 *	if (rw == KSTAT_WRITE)
 *		bcopy(buf, ksp->ks_data, ksp->ks_data_size);
 *	else
 *		bcopy(ksp->ks_data, buf, ksp->ks_data_size);
 *	return (0);
 *
 * A more illuminating example is taking a snapshot of a linked list:
 *
 *	ksp->ks_snaptime = gethrtime();
 *	if (rw == KSTAT_WRITE)
 *		return (EACCES);		... See below ...
 *	for (foo = first_foo; foo; foo = foo->next) {
 *		bcopy((char *) foo, (char *) buf, sizeof (struct foo));
 *		buf = ((struct foo *) buf) + 1;
 *	}
 *	return (0);
 *
 * In the example above, we have decided that we don't want to allow
 * KSTAT_WRITE access, so we return EACCES if this is attempted.
 *
 * The key points are:
 *
 *	(1) ks_snaptime must be set (via gethrtime()) to timestamp the data.
 *	(2) Data gets copied from the kstat to the buffer on KSTAT_READ,
 *		and from the buffer to the kstat on KSTAT_WRITE.
 *	(3) ks_snapshot return values are: 0 for success, EACCES if you
 *		don't allow KSTAT_WRITE, and EIO for any other type of error.
 *
 * Named kstats (see section on "Named statistics" below) containing long
 * strings (KSTAT_DATA_STRING) need special handling.  The kstat driver
 * assumes that all strings are copied into the buffer after the array of
 * named kstats, and the pointers (KSTAT_NAMED_STR_PTR()) are updated to point
 * into the copy within the buffer. The default snapshot routine does this,
 * but overriding routines should contain at least the following:
 *
 * if (rw == KSTAT_READ) {
 * 	kstat_named_t *knp = buf;
 * 	char *end = knp + ksp->ks_ndata;
 * 	uint_t i;
 *
 * 	... Do the regular copy ...
 * 	bcopy(ksp->ks_data, buf, sizeof (kstat_named_t) * ksp->ks_ndata);
 *
 * 	for (i = 0; i < ksp->ks_ndata; i++, knp++) {
 *		if (knp[i].data_type == KSTAT_DATA_STRING &&
 *		    KSTAT_NAMED_STR_PTR(knp) != NULL) {
 *			bcopy(KSTAT_NAMED_STR_PTR(knp), end,
 *			    KSTAT_NAMED_STR_BUFLEN(knp));
 *			KSTAT_NAMED_STR_PTR(knp) = end;
 *			end += KSTAT_NAMED_STR_BUFLEN(knp);
 *		}
 *	}
 */

/*
 * Named statistics.
 *
 * List of arbitrary name=value statistics.
 */

typedef struct kstat_named {
	char	name[KSTAT_STRLEN];	/* name of counter */
	uchar_t	data_type;		/* data type */
	union {
		char		c[16];	/* enough for 128-bit ints */
		int32_t		i32;
		uint32_t	ui32;
		struct {
			union {
				char 		*ptr;	/* NULL-term string */
#if defined(_KERNEL) && defined(_MULTI_DATAMODEL)
				caddr32_t	ptr32;
#endif
				char 		__pad[8]; /* 64-bit padding */
			} addr;
			uint32_t	len;	/* # bytes for strlen + '\0' */
		} string;
/*
 * The int64_t and uint64_t types are not valid for a maximally conformant
 * 32-bit compilation environment (cc -Xc) using compilers prior to the
 * introduction of C99 conforming compiler (reference ISO/IEC 9899:1990).
 * In these cases, the visibility of i64 and ui64 is only permitted for
 * 64-bit compilation environments or 32-bit non-maximally conformant
 * C89 or C90 ANSI C compilation environments (cc -Xt and cc -Xa). In the
 * C99 ANSI C compilation environment, the long long type is supported.
 * The _INT64_TYPE is defined by the implementation (see sys/int_types.h).
 */
#if defined(_INT64_TYPE)
		int64_t		i64;
		uint64_t	ui64;
#endif
		long		l;
		ulong_t		ul;

		/* These structure members are obsolete */

		longlong_t	ll;
		u_longlong_sol_t	ull;
		float		f;
		double		d;
	} value;			/* value of counter */
} kstat_named_t;

#define	KSTAT_DATA_CHAR		0
#define	KSTAT_DATA_INT32	1
#define	KSTAT_DATA_UINT32	2
#define	KSTAT_DATA_INT64	3
#define	KSTAT_DATA_UINT64	4

#if !defined(_LP64)
#define	KSTAT_DATA_LONG		KSTAT_DATA_INT32
#define	KSTAT_DATA_ULONG	KSTAT_DATA_UINT32
#else
#if !defined(_KERNEL)
#define	KSTAT_DATA_LONG		KSTAT_DATA_INT64
#define	KSTAT_DATA_ULONG	KSTAT_DATA_UINT64
#else
#define	KSTAT_DATA_LONG		7	/* only visible to the kernel */
#define	KSTAT_DATA_ULONG	8	/* only visible to the kernel */
#endif	/* !_KERNEL */
#endif	/* !_LP64 */

/*
 * Statistics exporting named kstats with long strings (KSTAT_DATA_STRING)
 * may not make the assumption that ks_data_size is equal to (ks_ndata * sizeof
 * (kstat_named_t)).  ks_data_size in these cases is equal to the sum of the
 * amount of space required to store the strings (ie, the sum of
 * KSTAT_NAMED_STR_BUFLEN() for all KSTAT_DATA_STRING statistics) plus the
 * space required to store the kstat_named_t's.
 *
 * The default update routine will update ks_data_size automatically for
 * variable-length kstats containing long strings (using the default update
 * routine only makes sense if the string is the only thing that is changing
 * in size, and ks_ndata is constant).  Fixed-length kstats containing long
 * strings must explicitly change ks_data_size (after creation but before
 * initialization) to reflect the correct amount of space required for the
 * long strings and the kstat_named_t's.
 */
#define	KSTAT_DATA_STRING	9

/* These types are obsolete */

#define	KSTAT_DATA_LONGLONG	KSTAT_DATA_INT64
#define	KSTAT_DATA_ULONGLONG	KSTAT_DATA_UINT64
#define	KSTAT_DATA_FLOAT	5
#define	KSTAT_DATA_DOUBLE	6

#define	KSTAT_NAMED_PTR(kptr)	((kstat_named_t *)(kptr)->ks_data)

/*
 * Retrieve the pointer of the string contained in the given named kstat.
 */
#define	KSTAT_NAMED_STR_PTR(knptr) ((knptr)->value.string.addr.ptr)

/*
 * Retrieve the length of the buffer required to store the string in the given
 * named kstat.
 */
#define	KSTAT_NAMED_STR_BUFLEN(knptr) ((knptr)->value.string.len)

/*
 * Interrupt statistics.
 *
 * An interrupt is a hard interrupt (sourced from the hardware device
 * itself), a soft interrupt (induced by the system via the use of
 * some system interrupt source), a watchdog interrupt (induced by
 * a periodic timer call), spurious (an interrupt entry point was
 * entered but there was no interrupt condition to service),
 * or multiple service (an interrupt condition was detected and
 * serviced just prior to returning from any of the other types).
 *
 * Measurement of the spurious class of interrupts is useful for
 * autovectored devices in order to pinpoint any interrupt latency
 * problems in a particular system configuration.
 *
 * Devices that have more than one interrupt of the same
 * type should use multiple structures.
 */

#define	KSTAT_INTR_HARD			0
#define	KSTAT_INTR_SOFT			1
#define	KSTAT_INTR_WATCHDOG		2
#define	KSTAT_INTR_SPURIOUS		3
#define	KSTAT_INTR_MULTSVC		4

#define	KSTAT_NUM_INTRS			5

typedef struct kstat_intr {
	uint_t	intrs[KSTAT_NUM_INTRS];	/* interrupt counters */
} kstat_intr_t;

#define	KSTAT_INTR_PTR(kptr)	((kstat_intr_t *)(kptr)->ks_data)

/*
 * I/O statistics.
 */

typedef struct kstat_io {

	/*
	 * Basic counters.
	 *
	 * The counters should be updated at the end of service
	 * (e.g., just prior to calling biodone()).
	 */

	u_longlong_sol_t	nread;		/* number of bytes read */
	u_longlong_sol_t	nwritten;	/* number of bytes written */
	uint_t		reads;		/* number of read operations */
	uint_t		writes;		/* number of write operations */

	/*
	 * Accumulated time and queue length statistics.
	 *
	 * Accumulated time statistics are kept as a running sum
	 * of "active" time.  Queue length statistics are kept as a
	 * running sum of the product of queue length and elapsed time
	 * at that length -- i.e., a Riemann sum for queue length
	 * integrated against time.  (You can also think of the active time
	 * as a Riemann sum, for the boolean function (queue_length > 0)
	 * integrated against time, or you can think of it as the
	 * Lebesgue measure of the set on which queue_length > 0.)
	 *
	 *		^
	 *		|			_________
	 *		8			| i4	|
	 *		|			|	|
	 *	Queue	6			|	|
	 *	Length	|	_________	|	|
	 *		4	| i2	|_______|	|
	 *		|	|	    i3		|
	 *		2_______|			|
	 *		|    i1				|
	 *		|_______________________________|
	 *		Time->	t1	t2	t3	t4
	 *
	 * At each change of state (entry or exit from the queue),
	 * we add the elapsed time (since the previous state change)
	 * to the active time if the queue length was non-zero during
	 * that interval; and we add the product of the elapsed time
	 * times the queue length to the running length*time sum.
	 *
	 * This method is generalizable to measuring residency
	 * in any defined system: instead of queue lengths, think
	 * of "outstanding RPC calls to server X".
	 *
	 * A large number of I/O subsystems have at least two basic
	 * "lists" of transactions they manage: one for transactions
	 * that have been accepted for processing but for which processing
	 * has yet to begin, and one for transactions which are actively
	 * being processed (but not done). For this reason, two cumulative
	 * time statistics are defined here: wait (pre-service) time,
	 * and run (service) time.
	 *
	 * All times are 64-bit nanoseconds (hrtime_t), as returned by
	 * gethrtime().
	 *
	 * The units of cumulative busy time are accumulated nanoseconds.
	 * The units of cumulative length*time products are elapsed time
	 * times queue length.
	 *
	 * Updates to the fields below are performed implicitly by calls to
	 * these five functions:
	 *
	 *	kstat_waitq_enter()
	 *	kstat_waitq_exit()
	 *	kstat_runq_enter()
	 *	kstat_runq_exit()
	 *
	 *	kstat_waitq_to_runq()		(see below)
	 *	kstat_runq_back_to_waitq()	(see below)
	 *
	 * Since kstat_waitq_exit() is typically followed immediately
	 * by kstat_runq_enter(), there is a single kstat_waitq_to_runq()
	 * function which performs both operations.  This is a performance
	 * win since only one timestamp is required.
	 *
	 * In some instances, it may be necessary to move a request from
	 * the run queue back to the wait queue, e.g. for write throttling.
	 * For these situations, call kstat_runq_back_to_waitq().
	 *
	 * These fields should never be updated by any other means.
	 */

	hrtime_t wtime;		/* cumulative wait (pre-service) time */
	hrtime_t wlentime;	/* cumulative wait length*time product */
	hrtime_t wlastupdate;	/* last time wait queue changed */
	hrtime_t rtime;		/* cumulative run (service) time */
	hrtime_t rlentime;	/* cumulative run length*time product */
	hrtime_t rlastupdate;	/* last time run queue changed */

	uint_t	wcnt;		/* count of elements in wait state */
	uint_t	rcnt;		/* count of elements in run state */

} kstat_io_t;

#define	KSTAT_IO_PTR(kptr)	((kstat_io_t *)(kptr)->ks_data)

/*
 * Event timer statistics - cumulative elapsed time and number of events.
 *
 * Updates to these fields are performed implicitly by calls to
 * kstat_timer_start() and kstat_timer_stop().
 */

typedef struct kstat_timer {
	char		name[KSTAT_STRLEN];	/* event name */
	uchar_t		resv;			/* reserved */
	u_longlong_sol_t	num_events;		/* number of events */
	hrtime_t	elapsed_time;		/* cumulative elapsed time */
	hrtime_t	min_time;		/* shortest event duration */
	hrtime_t	max_time;		/* longest event duration */
	hrtime_t	start_time;		/* previous event start time */
	hrtime_t	stop_time;		/* previous event stop time */
} kstat_timer_t;

#define	KSTAT_TIMER_PTR(kptr)	((kstat_timer_t *)(kptr)->ks_data)

#if	defined(_KERNEL)

#include <sys/t_lock.h>

extern kid_t	kstat_chain_id;		/* bumped at each state change */

/* IBM 20060714 murthya : eliminating references to solaris-sprecific functions .
   The following #ifndef will make sure we will never get to the code contained
   because this file is included only in linux
*/
#ifndef __linux
extern void	kstat_init(void);	/* initialize kstat framework */

/*
 * Adding and deleting kstats.
 *
 * The typical sequence to add a kstat is:
 *
 *	ksp = kstat_create(module, instance, name, class, type, ndata, flags);
 *	if (ksp) {
 *		... provider initialization, if necessary
 *		kstat_install(ksp);
 *	}
 *
 * There are three logically distinct steps here:
 *
 * Step 1: System Initialization (kstat_create)
 *
 * kstat_create() performs system initialization.  kstat_create()
 * allocates memory for the entire kstat (header plus data), initializes
 * all header fields, initializes the data section to all zeroes, assigns
 * a unique KID, and puts the kstat onto the system's kstat chain.
 * The returned kstat is marked invalid (KSTAT_FLAG_INVALID is set),
 * because the provider (caller) has not yet had a chance to initialize
 * the data section.
 *
 * By default, kstats are exported to all zones on the system.  A kstat may be
 * created via kstat_create_zone() to specify a zone to which the statistics
 * should be exported.  kstat_zone_add() may be used to specify additional
 * zones to which the statistics are to be exported.
 *
 * Step 2: Provider Initialization
 *
 * The provider performs any necessary initialization of the data section,
 * e.g. setting the name fields in a KSTAT_TYPE_NAMED.  Virtual kstats set
 * the ks_data field at this time.  The provider may also set the ks_update,
 * ks_snapshot, ks_private, and ks_lock fields if necessary.
 *
 * Step 3: Installation (kstat_install)
 *
 * Once the kstat is completely initialized, kstat_install() clears the
 * INVALID flag, thus making the kstat accessible to the outside world.
 * kstat_install() also clears the DORMANT flag for persistent kstats.
 *
 * Removing a kstat from the system
 *
 * kstat_delete(ksp) removes ksp from the kstat chain and frees all
 * associated system resources.  NOTE: When you call kstat_delete(),
 * you must NOT be holding that kstat's ks_lock.  Otherwise, you may
 * deadlock with a kstat reader.
 *
 * Persistent kstats
 *
 * From the provider's point of view, persistence is transparent.  The only
 * difference between ephemeral (normal) kstats and persistent kstats
 * is that you pass KSTAT_FLAG_PERSISTENT to kstat_create().  Magically,
 * this has the effect of making your data visible even when you're
 * not home.  Persistence is important to tools like iostat, which want
 * to get a meaningful picture of disk activity.  Without persistence,
 * raw disk i/o statistics could never accumulate: they would come and
 * go with each open/close of the raw device.
 *
 * The magic of persistence works by slightly altering the behavior of
 * kstat_create() and kstat_delete().  The first call to kstat_create()
 * creates a new kstat, as usual.  However, kstat_delete() does not
 * actually delete the kstat: it performs one final update of the data
 * (i.e., calls the ks_update routine), marks the kstat as dormant, and
 * sets the ks_lock, ks_update, ks_private, and ks_snapshot fields back
 * to their default values (since they might otherwise point to garbage,
 * e.g. if the provider is going away).  kstat clients can still access
 * the dormant kstat just like a live kstat; they just continue to see
 * the final data values as long as the kstat remains dormant.
 * All subsequent kstat_create() calls simply find the already-existing,
 * dormant kstat and return a pointer to it, without altering any fields.
 * The provider then performs its usual initialization sequence, and
 * calls kstat_install().  kstat_install() uses the old data values to
 * initialize the native data (i.e., ks_update is called with KSTAT_WRITE),
 * thus making it seem like you were never gone.
 */

extern kstat_t *kstat_create(char *, int, char *, char *, uchar_t,
    uint_t, uchar_t);
extern kstat_t *kstat_create_zone(char *, int, char *, char *, uchar_t,
    uint_t, uchar_t, zoneid_t);
extern void kstat_install(kstat_t *);
extern void kstat_delete(kstat_t *);
extern void kstat_named_setstr(kstat_named_t *knp, const char *src);
extern void kstat_set_string(char *, char *);
extern void kstat_delete_byname(char *, int, char *);
extern void kstat_delete_byname_zone(char *, int, char *, zoneid_t);
extern void kstat_named_init(kstat_named_t *, char *, uchar_t);
extern void kstat_timer_init(kstat_timer_t *, char *);
extern void kstat_waitq_enter(kstat_io_t *);
extern void kstat_waitq_exit(kstat_io_t *);
extern void kstat_runq_enter(kstat_io_t *);
extern void kstat_runq_exit(kstat_io_t *);
extern void kstat_waitq_to_runq(kstat_io_t *);
extern void kstat_runq_back_to_waitq(kstat_io_t *);
extern void kstat_timer_start(kstat_timer_t *);
extern void kstat_timer_stop(kstat_timer_t *);

extern void kstat_zone_add(kstat_t *, zoneid_t);
extern void kstat_zone_remove(kstat_t *, zoneid_t);
extern int kstat_zone_find(kstat_t *, zoneid_t);

extern kstat_t *kstat_hold_bykid(kid_t kid, zoneid_t);
extern kstat_t *kstat_hold_byname(char *, int, char *, zoneid_t);
extern void kstat_rele(kstat_t *);

/* The following #endif is for #ifndef __linux */
#endif
#endif	/* defined(_KERNEL) */

/* The following endif is for #define  _SYS_KSTAT_FROMSOL_H */
#endif 

/* murthya : END : contents from sys/kstat.h */

/* IBM 20060718 murthya : Givev below are relevant structures copied over from
   kstat.h on Solaris. These need to precede the contents from kstat.h, that are
   included later in the file */

/* murthya : BEGIN : contents from kstat.h */

#ifndef _KSTAT_FROMSOL_H
#define _KSTAT_FROMSOL_H

/* Almost the entire contents being copied here on 20060718, to checkin the file in time for a build.
   Irrelvant contents will be cleaned up */
   
/* IBM 20060714 murthya : QQ: This file is the /usr/include/kstat.h copied over from solaris to 
   facilitate the use of some structures in the meas/MSmt directory. Eventually, the structures 
   that are not used will be removed from this file. and the surviving ones might be modified 
   to retain just the data members that are used in the Lucent code.  This file might even be renamed 
   to something else. 

   This note is being added just to make it clear the contents are taken from solaris.
*/

/* IBM 20060714 murthya : eliminating includes irrelevant to linux and adding relevant ones

#include <sys/types.h>
#include <sys/kstat.h>
*/

/*
 * kstat_open() returns a pointer to a kstat_ctl_t.
 * This is used for subsequent libkstat operations.
 */
typedef struct kstat_ctl {
	kid_t	kc_chain_id;	/* current kstat chain ID	*/
	kstat_t	*kc_chain;	/* pointer to kstat chain	*/
	int	kc_kd;		/* /dev/kstat descriptor	*/
} kstat_ctl_t;

/* query: != 0 iff flag is turned on in set */
#define prismember(sp, flag) \
        (((unsigned)((flag)-1) < 32*sizeof (*(sp))/sizeof (uint32_t)) && \
            (((uint32_t *)(sp))[((flag)-1)/32] & (1U<<(((flag)-1)%32))))

/* IBM 20060717 murthya : eliminating function declarations not relevant to linux */
#ifndef __linux
#ifdef	__STDC__
extern	kstat_ctl_t	*kstat_open(void);
extern	int		kstat_close(kstat_ctl_t *);
extern	kid_t		kstat_read(kstat_ctl_t *, kstat_t *, void *);
extern	kid_t		kstat_write(kstat_ctl_t *, kstat_t *, void *);
extern	kid_t		kstat_chain_update(kstat_ctl_t *);
extern	kstat_t		*kstat_lookup(kstat_ctl_t *, char *, int, char *);
extern	void		*kstat_data_lookup(kstat_t *, char *);
#else
extern	kstat_ctl_t	*kstat_open();
extern	int		kstat_close();
extern	kid_t		kstat_read();
extern	kid_t		kstat_write();
extern	kid_t		kstat_chain_update();
extern	kstat_t		*kstat_lookup();
extern	void		*kstat_data_lookup();
#endif
/* The following endif is for #ifndef __linux */
#endif
/* The following endif is for #define _KSTAT_FROMSOL_H */
#endif 
/* murthya : END : contents from kstat.h */
#ifdef  __cplusplus
}
#endif


#endif    	/* __LINUX  */
#endif		/* _H_LUC_COMPAT  */
