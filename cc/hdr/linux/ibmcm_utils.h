#ifndef __ibmcm_utils_h
#define __ibmcm_utils_h

#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdint.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include "ibmcm_pthread_utils.h"

/* IBM swerbner 20060711 disable name mangling for all local declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* See SUN header sys/strft.h for this definition */
typedef long long hrtime_t;

/*
* struct meminfo_t - 
*    Utility struct for storing data extracted from read of
*    /proc/meminfo psuedo file.  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_meminfo(meminfo_t *m);
*
*    The meminfo_t struct can be dumped to stdout by calling:
*       void dump_proc_meminfo(meminfo_t *m);
*
*  Added by Bhorton - 5/19/06
*/
     
typedef struct
{   long memTotal;
    long memFree;
    long buffers;
    long cached;
    long swpCached;
    long active;
    long inactive;
    long highTotal;
    long highFree;
    long lowTotal;
    long lowFree;
    long swapTotal;
    long swapFree;
    long dirty;
    long writeback;
    long mapped;
    long slab;
    long committedAs;
    long pageTables;
    long VmallocTotal;
    long VmallocUsed;
    long VmallocChunk;
    long hugePagesTotal;
    long hugePagesFree;
    long hugePageSize;     /* in kBytes */
} meminfo_t;



/*
* struct proc_stat_t - 
*    Utility struct for storing data extracted from read of
*    /proc/<PID>/stat psuedo file.  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_stat(pid_t pid, proc_stat_t *m);
*
*    The proc_stat struct can be dumped to stdout by calling:
*       void dump_proc_stat(proc_stat_t *m);
*
*  Added by Bhorton - 5/19/06
*/
     

typedef struct {
  pid_t         pid;
  char          comm[256];
  char          state;
  pid_t         ppid;
  int           pgrp;
  int           session;
  int           tty_nr;
  int           tpgid;
  unsigned long flags;
  unsigned long minflt;
  unsigned long cminflt;
  unsigned long majflt;
  unsigned long cmajflt;
  unsigned long utime;
  unsigned long stime;
  long          cutime;
  long          cstime;
  long          priority;
  long          nice;
  long          SetZero;
  long          itrealvalue;
  unsigned long starttime;
  unsigned long vsize;
  long          rss;
  unsigned long rlim;
  unsigned long startcode;
  unsigned long endcode;
  unsigned long startstack;
  unsigned long kstkesp;
  unsigned long kstkeip;
  unsigned long signal;
  unsigned long blocked;
  unsigned long sigignore;
  unsigned long sigcatch;
  unsigned long wchan;
  unsigned long nswap;
  unsigned long cnswap;
  int           exit_signal;
  int           processor;
} proc_stat_t;

/*
* struct proc_uptime_t - 
*    Utility struct for storing data extracted from read of
*    /proc/uptime  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_uptime(proc_uptime_t *ptr);
*
*    The proc_uptime_t can be dumped to stdout by calling:
*       int dump_proc_uptime(proc_uptime_t *ptr);
*
*    The proc_uptime_t can be initialized by calling init_proc_uptime:
*       int init_proc_uptime(proc_uptime_t *ptr);
*
*  Added by wyoes 5/23/2006
*/
     

typedef struct {
  double uptime;
  double idletime;
  } proc_uptime_t;

/*
* struct proc_loadavg_t - 
*    Utility struct for storing data extracted from read of
*    /proc/loadavg  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_loadavg(proc_loadavg_t *ptr);
*
*    The proc_loadavg_t can be dumped to stdout by calling:
*       int dump_proc_loadavg(proc_loadavg_t *ptr);
*
*  Added by wyoes 5/23/2006
*/

typedef struct {
  double min1;
  double min5;
  double min15;
  long   running_procs;
  long   total_procs;
  pid_t  last_active_proc;
} proc_loadavg_t;

/*
* struct proc_cpuinfo_t - 
*    Utility struct for storing data extracted from read of
*    /proc/cpuinfo  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_cpuinfo(proc_cpuinfo_t *ptr);
*
*    The proc_cpuinfo_t can be dumped to stdout by calling:
*       int dump_proc_cpuinfo(proc_cpuinfo_t *ptr);
*
*    The proc_cpuinfo_t can be initialized by calling:
*       int init_proc_cpuinfo(proc_cpuinfo_t *ptr);
*
*  Added by wyoes 5/23/2006
*/

/* 
*   int get_number_of_CPUs(&numCPUs)
*   counts the number of 'processor' entries in /proc/cpuinfo
*   appended by murthya 7/12/2006
*/
     
typedef struct {
    long processor;
    char vendor_id[16];
    long cpu_family;
    long model;
    char model_name[64];
    long stepping;
    float cpu_MHz;
    char cache_size[16];
    long physical_id;
    long siblings;
    long core_id;
    long cpu_cores;
    char fpu[8];
    char fpu_exception[8];
    long cpuid_level;
    char wp[8];
    char flags[512];
    float bogomips;
    long clflush_size;
    long cache_alignment;
    char address_sizes[256];
    char power_management[256];
} proc_cpuinfo_t;

/*
* struct proc_pid_status_t - 
*    Utility struct for storing data extracted from read of
*    /proc/<PID>/status  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_pid_status((pid_t pid, proc_pid_status_t *ptr);
*
*    The proc_pid_status_t can be dumped to stdout by calling:
*       int dump_proc_pid_status(proc_pid_status_t *ptr);
*
*    The proc_pid_status_t can be initialized by calling:
*       int init_proc_pid_status(proc_pid_status_t *ptr);
*
*  Added by wyoes 5/24/2006
*/
     

typedef struct {
  char Name[256];             /* command name */
  char State[256];            /* process state */
  long SleepAVG;      
  gid_t Tgid;
  pid_t Pid;                  /* Its PID  */
  pid_t PPid;                 /* Its PPID */
  pid_t TracerPid;
  uid_t Uid[4];               /* Its [ ESF ] UID */
  gid_t Gid[4];               /* Its [ ESF ] GID */
  long FDSize;
  gid_t Groups[128];
  char VmSize[32];            /* Size of VM areas */
  char VmLck[32];             /* Locked VM areas  */
  char VmRSS[32];             /* RSS size */
  char VmData[32];            /* Data segment (without stack) */
  char VmStk[32];             /* Stack segment */
  char VmExe[32];             /* Loaded program */
  char VmLib[32];             /* Loaded libraries */
  long Threads;
  char SigPnd[32];            /* Pending signals */
  char ShdPnd[32];
  char SigBlk[32];            /* Mask of blocked signals */
  char SigIgn[32];            /* Mask of ignored signals */
  char SigCgt[32];            /* Mask of signals with handling routine */
  char CapInh[32];
  char CapPrm[32];
  char CapEff[32];
} proc_pid_status_t;

/*
* struct proc_pid_statm_t -
*    Utility struct for storing data extracted from read of
*    /proc/<PID>/statm  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_pid_statm(pid_t pid, proc_pid_statm_t *ptr);
*
*    The proc_pid_statm_t can be dumped to stdout by calling:
*       int dump_proc_pid_status(proc_pid_statm_t *ptr);
*
*    The proc_pid_statm_t can be initialized by calling:
*       int init_proc_pid_status(proc_pid_statm_t *ptr);
*
*  Added by wyoes 5/26/2006
*/

typedef struct {
  long size;       /* total number of memory pages used */
  long resident;   /* number of memory pages currently in physical memory */
  long share;      /* number of memory pages the process shares with other processes */
  long trs;        /* number of text pages currently in physical memory */
  long lrs;        /* number of library pages currently in physical memory */
  long drs;        /* number of data pages including written library pages and the stack, currently in physical memory */
  long dt;         /* number of library pages that have been accessed */
} proc_pid_statm_t;

/*
* struct proc_vmstat_t -
*    Utility struct for storing data extracted from read of
*    /proc/vmstat  (see man 5 proc for details)
*
*    This struct is populated by calling the function:
*       int get_proc_vmstat(proc_vmstat_t *ptr);
*
*    The proc_vmstat_t can be dumped to stdout by calling:
*       int dump_proc_vmstat(proc_vmstat_t *ptr);
*
*    The proc_vmstat_t can be initialized by calling:
*       int init_vmstat(proc_vmstat_t *ptr);
*
*  Added by wyoes 5/31/2006
*/

typedef struct {
  long nr_dirty;
  long nr_writeback;
  long nr_unstable;
  long nr_page_table_pages;
  long nr_mapped;
  long nr_slab;
  long pgpgin;
  long pgpgout;
  long pswpin;
  long pswpout;
  long pgalloc_high;
  long long pgalloc_normal;
  long pgalloc_dma32;
  long pgalloc_dma;
  long long pgfree;
  long pgactivate;
  long pgdeactivate;
  long long pgfault;
  long pgmajfault;
  long pgrefill_high;
  long pgrefill_normal;
  long pgrefill_dma32;
  long pgrefill_dma;
  long pgsteal_high;
  long pgsteal_normal;
  long pgsteal_dma32;
  long pgsteal_dma;
  long pgscan_kswapd_high;
  long pgscan_kswapd_normal;
  long pgscan_kswapd_dma32;
  long pgscan_kswapd_dma;
  long pgscan_direct_high;
  long pgscan_direct_normal;
  long pgscan_direct_dma32;
  long pgscan_direct_dma;
  long pginodesteal;
  long slabs_scanned;
  long kswapd_steal;
  long kswapd_inodesteal;
  long pageoutrun;
  long allocstall;
  long pgrotated;
} proc_vmstat_t;


/*
* struct proc_diskstats_t -
*    Utility struct for storing data extracted from read of
*    /proc/diskstats  (see man 5 proc for details)
* 
* Note: The values gathered for the diskstats are unsigned 32 or 64 bit values.
*       On a very busy or long-lived system they may wrap. 
*
*    This struct is populated by calling the function:
*       int get_proc_diskstats_by_name(char *devname, proc_diskstats_t *ptr);
*
*    The proc_diskstats_t can be dumped to stdout by calling:
*       int dump_proc_diskstats(proc_diskstats_t *ptr);
*
*    The proc_diskstats_t can be initialized by calling:
*       int init_diskstats(proc_diskstats_t *ptr);
*
*  Added by wyoes 6/01/2006
*/


typedef struct {
  int major;                 /* major device number */
  int minor;                 /* minor device number */
  char  devicename[128];     /* device name */
  uint64_t num_reads;        /* total number of reads completed successfully */
  uint64_t num_merged_reads; /* number of reads merged */
  uint64_t num_sectors_read; /* number of sectors read */
  uint64_t num_ms_read;      /* nbr of milliseconds spent reading */
  uint64_t num_writes_complete; /* nbr of writes completed */
  uint64_t num_merged_writes;   /* number of writes merged */
  uint64_t num_sectors_written; /* number of sectors written */
  uint64_t num_ms_written;      /* number of milliseconds spent writing */
  uint64_t num_io_current;      /* number of I/Os currently in progress */
  uint64_t num_ms_io;           /* nbr of milliseconds spent doing I/Os */
  uint64_t num_ms_io_wt;     /* weighted nbr of milliseconds spent doing I/Os */
} proc_diskstats_t;

/*
* struct proc_partitions_t -
*    Utility struct for storing data extracted from read of
*    /proc/partitions  (see man 5 proc for details)
* 
*    This struct is populated by calling the function:
*       int get_proc_partitions_by_name(char *devname, proc_partitions_t *ptr);
*
*    The proc_partitions_t can be dumped to stdout by calling:
*       int dump_proc_partitions(proc_partitions_t *ptr);
*
*    The proc_partitions_t can be initialized by calling:
*       int init_partitions(proc_partitions_t *ptr);
*
*  Added by wyoes 6/05/2006
*/

typedef struct {
  int major;                 /* major device number */
  int minor;                 /* minor device number */
  uint64_t num_of_blocks;    /* Number of blocks */
  char  devicename[128];     /* device name */
} proc_partitions_t;


/* IBM  20060712 murthya : define cpu_usage_stat */
/* murthya : The complete version of cpu_usage_stat is defined in <linux/kernel_stat.h>
   However, including it is causing many compilation issues resulting in many other includes,
   and #defines . To avoid unnecessary complexity, the structure is being re-defined here.
   The u64 of the original is replaced by 'long' here. 

   However, this issue can be revisted to get rid of this duplicate definition.

   Also added get_sys_proc_stat()
*/

typedef struct {
	hrtime_t snapshotTime;
        long user;
        long nice;
        long system;
        long idle;
        long iowait;
        long irq;
        long softirq;
	long steal_time;
} cpu_usage_stat;


/*
** This structure will hold the stats from /proc/net/dev
** 
*/

typedef struct
{
    /*
    ** Since this table will have multiple entries, the scheduled collection
    ** time, actual collection time, and duration are not stored in the
    ** structure to conserve space.  They are stored in SchTimeIPdataMeas,
    ** ActTimeIPdataMeas, and DurationIPdataMeas, respectively.
    */

    char  intfName[100];  /* the name of the IP interface */
    unsigned long long ibytes;         /* the size of incomming bytes */
    unsigned long ipackets;           /* the size of the Incomming packet */
    unsigned long ierrors;            /* the size of the Incomming packet error */
    unsigned long idrop;
    unsigned long ififo;
    unsigned long iframes ; 
    unsigned long icompressed ;
    unsigned long imulticast;

    unsigned long long obytes;         /* the size of outgoing bytes */
    unsigned long opackets;           /* the size of the Outgoing packet */
    unsigned long oerrors;            /* the size of the Outgoing packet error */
    unsigned long odrop ;
    unsigned long ofifo;
    unsigned long ocollisions;         /* the number of collisions encountered */
    unsigned long ocarrier;
    unsigned long ocompressed;
} IPDataMeas;

/*
 * process usage structure
 *
 * IBM scohoon 20060726 - this structure is a copy of the Solaris prusage
 * structure that reports process usage statistics.  It will be populated
 * by the populate_prusage() function.  Some of these measurements are not
 * available on Linux so those elements will be zero.
 *
 */

#ifndef ulong_t_defined
#define ulong_t_defined
typedef unsigned long ulong_t;
#endif

#ifndef LINUX_TIME
#define LINUX_TIME
typedef struct timestruc  { 
    long tv_sec;   // seconds
    long tv_nsec;  // nanoseconds
} timestruc_t;
#endif

typedef struct prusage {
        id_t            pr_lwpid;       /* lwp id.  0: process or defunct */
        int             pr_count;       /* number of contributing lwps */
        timestruc_t     pr_tstamp;      /* current time stamp */
        timestruc_t     pr_create;      /* process/lwp creation time stamp */
        timestruc_t     pr_term;        /* process/lwp termination time stamp */
        timestruc_t     pr_rtime;       /* total lwp real (elapsed) time */
        timestruc_t     pr_utime;       /* user level cpu time */
        timestruc_t     pr_stime;       /* system call cpu time */
        timestruc_t     pr_ttime;       /* other system trap cpu time */
        timestruc_t     pr_tftime;      /* text page fault sleep time */
        timestruc_t     pr_dftime;      /* data page fault sleep time */
        timestruc_t     pr_kftime;      /* kernel page fault sleep time */
        timestruc_t     pr_ltime;       /* user lock wait sleep time */
        timestruc_t     pr_slptime;     /* all other sleep time */
        timestruc_t     pr_wtime;       /* wait-cpu (latency) time */
        timestruc_t     pr_stoptime;    /* stopped time */
        timestruc_t     filltime[6];    /* filler for future expansion */
        ulong_t         pr_minf;        /* minor page faults */
        ulong_t         pr_majf;        /* major page faults */
        ulong_t         pr_nswap;       /* swaps */
        ulong_t         pr_inblk;       /* input blocks */
        ulong_t         pr_oublk;       /* output blocks */
        ulong_t         pr_msnd;        /* messages sent */
        ulong_t         pr_mrcv;        /* messages received */
        ulong_t         pr_sigs;        /* signals received */
        ulong_t         pr_vctx;        /* voluntary context switches */
        ulong_t         pr_ictx;        /* involuntary context switches */
        ulong_t         pr_sysc;        /* system calls */
        ulong_t         pr_ioch;        /* chars read and written */
        ulong_t         filler[10];     /* filler for future expansion */
} prusage_t;

/* IBM 20060803 murthya
   login_parms holds the variables that are set in the environment when 
   the osmond retrieves during login. More can be added as necessary.
   The method get_def_login_env_parms() populates the memebers of this structure.
   So, when new data elements are added here, we need to modify the method as well.
*/
typedef struct 
{
        char TZ[24];
	char syslog[8];
	char HZs[6];
	char Umask[5];
	char sleeptime[5];
/* It is the default, system-wide path we are talking about here. so, hopefully
   1024 (1023 actually) should be OK
*/
	char path[1024];
} login_parms;

/*
 * scohoon 20060807 - copied from Solaris and modified to fit Linux headers.
 *
 * IN6_INADDR_TO_V4MAPPED 
 *      Assign a IPv4 address address to an IPv6 address as a IPv4-mapped
 *      address.
 *      Note: These macros are NOT defined in RFC2553 or any other standard
 *      specification and are not macros that portable applications should
 *      use.
 *
 * void IN6_INADDR_TO_V4MAPPED(const struct in_addr *v4, in6_addr_t *v6);
 *
 */
/* Solaris little-endian version - 
#define IN6_INADDR_TO_V4MAPPED(v4, v6) \
        ((v6)->_S6_un._S6_u32[3] = (v4)->s_addr, \
        (v6)->_S6_un._S6_u32[2] = 0xffff0000U, \
        (v6)->_S6_un._S6_u32[1] = 0, \
        (v6)->_S6_un._S6_u32[0] = 0)
*/
#define IN6_INADDR_TO_V4MAPPED(v4, v6) \
        ((v6)->in6_u.u6_addr32[3] = (v4)->s_addr, \
        (v6)->in6_u.u6_addr32[2] = 0xffff0000U, \
        (v6)->in6_u.u6_addr32[1] = 0, \
        (v6)->in6_u.u6_addr32[0] = 0)

/* 20060901 IBM murthya : add the ps_oput structure. This holds the most general
   output from ps. 
   NOTE : The field widths in the structure, and the output format string go together. if one is changed
   (which probably is not going to be needed very often, the other MUST be changed */
   
/* The field names in the ps_oput structure are exactly the same as those described in the man page
    for ps , as of 20060831 on the linux box with uname = Linux 2.6.5-7.244-smp #1 x86_64 GNU/Linux.
    This was done to make it easier to match the fields and the output of ps. 
    Also, 'args' has been put at the end because it can have spaces

    The field widths here are
    going to be used to specify the width of columns in the formatting string for ps.
*/    
    
#define PS_OPUT_FORMAT_STRING "class,stat,tty,pid,ppid,lwp,nlwp,bsdstart,bsdtime,egid,egroup,fuid,fuser,ruid,ruser,rgid,rgroup,sgid,sgroup,eip,esp,etime,euid,euser,suid,suser,sgi_p,cputime,flags,lstart,ni,wchan:20,pgid,psr,rss,rtprio,sess,stackp,sz,tname,tpgid,vsz,cpu,%mem,blocked:64,caught:64,pending:64,ignored:64,args:100"
#if 0
typedef struct ps_oput
{
	/* First the left-justified fields */
	char class[3+1];        /* scheduling class of the process. This is the same as policy , cls */
	char stat[4+1];	      /* multi character process state */
	char tty[8+1];         /* controlling tty */

	/* Some of the following are right-justified per procps code. for the rest, (again per procps)
	   the field is left justified if numeric and right justified if text */

     	char pid[5+1];	      /* process ID number of the process */
	char ppid[5+1];        /* parent process ID */
	char lwp[5+1];         /* thread ID . same as 'tid' or 'spid' */
	char nlwp[4+1];         /* number of threads */
	char bsdstart[6+1];    /* time when the process/thread started */
	char bsdtime[6+1];     /* accumulated cpu time - user + sys */
        char egid[5+1];        /* effective group ID number */
	char egroup[8+1];      /* the textual, effective group ID */
	char fuid[5+1];        /* file system access user ID */
	char fuser[8+1];       /* filesystem access user ID (textual) */
	char ruid[5+1];        /* real user ID */
	char ruser[8+1];       /* real user iD - textual */
	char rgid[5+1];        /* rea group ID */
	char rgroup[8+1];      /* real group name */
	char sgid[5+1];         /* saved group ID */
        char sgroup[8+1];      /* saved group name */
	char eip[8+1];         /* instruction pointer */
        char esp[8+1]; 	      /* stack pointer */
        char etime[11+1];       /* elapsed time since the process was started */
        char euid[5+1];        /* effective user ID */
	char euser[8+1];	      /* effective user name */
	char suid[5+1];        /* Saved user ID */
	char suser[8+1];       /* saved user name */
	char sgi_p[1+1];	      /* processor that the process is currently executing on */
	char cputime[8+1];     /* cumulative spu time */
        char flags[1+1];	      /* process flags */
	char lstart[24+1];      /* Time when the command started */
	char ni[3+1];           /* nice value */

	/* The width of the wchan field is closely tied to the width specified in the PS_OPUT_FORMAT_STRING above. 
            If this changes, that should change too */

	char wchan[20+1];	      /*  address of the kernel function where the process is sleeping */
	char pgid[5+1];        /* process group ID */
	char psr[3+1];          /* processor that the process is currently assigned to */
	char rss[5+1];	      /* resident set size - the non-swapped physical memory that a process has used */
	char rtprio[6+1];	      /* real time priority */
	char sess[5+1];	      /* session ID, or equivalently, process ID of the session leader */
        char stackp[8+1];      /* address of the bottom (start) of the stack for the process*/
	char sz[5+1];          /* size in physical pages of the core image of the process. 
                                  This includes text, data, and stack space. Device
                                  mappings are currently excluded; this is subject to change. See vsz and rss.*/
	char tname[8+1];       /* controling terminal name */
	char tpgid[5+1];	/* ID of the foreground process group on the tty (terminal) that the process is connected to, 
                                   or -1 if the process is not connected to a tty */
	char vsz[6+1];         /* virtual memory size of the process in KiB (1024-byte units). Device mappings are currently 
					excluded; this is subject to change. */
	char cpu[4+1];          /* percentage cpu utilization of the process. */
	char mem[4+1];          /* percentage memory utilization of the process */

	/* The following are the fields on which we specify, on which we specify much more than what the procps code says */
	char blocked[64+1] ;    /* the mask can be upto 64 bit */
	char caught[64+1];      /* mask of caught sighnals */
	char pending[64+1];     /* mask of signals pending */
        char ignored[64+1];     /* mask of ignored signals */
	char args[100+1];	      /* the executable name */	
} ps_oput;
#endif

/*** prototypes ***/

typedef enum  {eUnknown = -1, PR_MODEL_ILP32 , PR_MODEL_LP64} fileClass ;

/*
        20061026 IBM murthya : The structure of a 'mapping' (each line-entry in /proc/pid/maps)
        on linux is different from the structure in the prmap_t structure (of the contents of
        /proc/pid/map) on Sun. The following structure is being defined as a part of an attempt to
        design the linux code in terms similar to that on sun. This structure is based on the contents
        of a /proc/pid/maps file . Where common attributes exist between sun and linux, the name in the
	linux version has been set to be same as sun.


*/
#if __WORDSIZE == 64
#define KLONG long long
#define KLF "L"
#else
#define KLONG long
#define KLF "l"
#endif

#define PRMAPSZ 512
typedef struct {
        unsigned KLONG pr_vaddr;     /* virtual address of mapping start */
        unsigned KLONG pr_vaddr_end;       /* virtual address of mapping end */

        unsigned KLONG pr_size; /* virtual address mapping size .
                                               Just the difference begin-end , added for convenience*/

        char pr_mflags[32];              /* The process 'permission' flags */
        unsigned long long pr_offset;    /* offset into mapped object, if any */
        unsigned dev_major;                /* The device major number*/
        unsigned dev_minor;                /* The device major number*/
        unsigned long long inode;
        char    pr_mapname[PRMAPSZ];       /* name in /proc/<pid>/object */
    } prmap_linux_t;

/*
	20061026 IBM murthya : need this for lib/procdump/symtbl.c
*/
typedef uint64_t          Elf64_Lword;
typedef uint64_t          Elf32_Lword;

/* populates a prusage struct for the calling process */
/* 10232006 IBM murthya :  As of today, populate_prusage() calls getrusage() with
   RUSAGE_SELF. However,  within misc/tools/monitor/src/command.c:do_commands() for the
 	switch-case C_PUSAGE, the monitor is expected to print the usage for a process
	with a given pid. At the point we get to this line, the second process has already
	started being 'ptrace'd and is stopped within a loop.
	
 Per the comments in the code for sys_getrusage within the linux kernel code, for
 getrusage() to yield usage of a process (other than oneself) by the use of RUSAGE_CHILDREN
 the other process should either be stopped or zombied. The processes being handled within
 misc/tools/monitor/src/command.c:do_commands() meet this requirement because they are stopped
 as a result of the ptrace. Hence it is expected that using the RUSAGE_CHILDREN parameter in this
 case will give the right results.

 Note that the man page of getrusage () is at variance with the comment in the kernel code because
 the man page says RUSAGE_CHILDREN will yield results for processes that have 'terminated and have 
 been waited for'.
*/

int populate_prusage(prusage_t* pr_u, int self_or_children);

/* prints a prusage struct that was filled by populate_prusage() */
void dump_prusage(prusage_t* pr_u);

/* prints out a list of cstrings  - returns 0 */
int dump_cstring_list(char **s);

/* prints out the hostent structure - return 0 */
int dump_hostent(struct hostent *hp);

/* gets a record from a file via fgets() - returns 0 if fgets() returns 
        nothing, otherwise returns the strlen() of the record */
int getrecord(FILE *fp, char *record); 

/*
	20061026 IBM murthya :

	When dealing with files like /proc/pid/maps, where there is a need to
	to know the number of lines in the file. The following is just a general
	purpose routine for that	.

	parms : 
		fd = the file descriptor of the file in which to read the number of records
		number_of_records : The resulting number of records
		return 0 as of today
	
*/
	int get_number_of_records(int fd, int * number_of_records);
/* trims whitespace characters from the left and right.  returns the 
         strlen() of the trimmed buffer */
int trim_whitespace(char *buf);

/* populates a proc_stat_t for process pid; read from /proc/<PID>/stat 
   returns 0 for success, -2 for an error opening the file, -3 for 
   incorrect number of values scanned from file, -1 check the errno global */ 
int get_proc_stat(pid_t pid, proc_stat_t *p);

/* prints a proc_stat_t structure to stdout */
void dump_proc_stat(proc_stat_t *p);

/* populates a meminfo_t structure -  read from /proc/meminfo
   returns 0 for success, -2 for an error opening the file, -3 for
   incorrect number of values scanned from file, -1 check the errno global */
int get_proc_meminfo(meminfo_t *m);

/* prints a proc_stat_t structure to stdout */
void dump_proc_meminfo(meminfo_t *m);

/* populates a proc_uptime_t structure -  read from /proc/uptime
   returns 0 for success, -2 for an error opening the file, -3 for
   incorrect number of values scanned from file, -1 check the errno global */
int get_proc_uptime(proc_uptime_t *ptr);

/* prints a proc_uptime_t structure on stdout - returns 0 */
int dump_proc_uptime(proc_uptime_t *ptr);

/* inits a proc_uptime_t structure - returns 0 */
int init_proc_uptime(proc_uptime_t *ptr);

/* prints a proc_loadavg_t struct on stdout  - returns 0 */
int dump_proc_loadavg(proc_loadavg_t *ptr);

/* populates a proc_loadavg_t structure -  read from /proc/loadavg
   returns 0 for success, -2 for an error opening the file, -3 for
   incorrect number of values scanned from file, -1 check the errno global */
int get_proc_loadavg(proc_loadavg_t *ptr);

/* populates a proc_loadavg_t structure -  read from /proc/cpuinfo
   returns 0 for success, -2 for an error opening the file, -1 check the 
   errno global */
int get_proc_cpuinfo(proc_cpuinfo_t *ptr);

/* prints a proc_cpuinfo_t to stdout - returns 0 */
int dump_proc_cpuinfo(proc_cpuinfo_t *ptr);

/* inits a proc_cpuinfo_t structure - returns 0 */
int init_proc_cpuinfo(proc_cpuinfo_t *ptr);

/* puts the number of processor entries in /proc/cpuinfo  in the parm. returns 0 */
int get_number_of_cpus(short int *numCPUs);

/* inits a proc_pid_status_t - returns 0 */
int init_proc_pid_status(proc_pid_status_t *ptr);

/* populates a proc_pid_status_t from the /proc/<PID>/status file.
   On success returns 0, returns -2 if it can not opent the file. */
int get_proc_pid_status(pid_t pid, proc_pid_status_t *ptr);

/* prints a proc_pid_status_t structure to stdout - returns 0 */
int dump_proc_pid_status(proc_pid_status_t *ptr);

/* inits a proc_pid_statm_t structure - returns 0 */
int init_proc_pid_statm(proc_pid_statm_t *ptr);

/* populates a proc_pid_statm_t structure -  read from /proc/<PID>/statm
   returns 0 for success, -2 for an error opening the file, -1 check the
   errno global, -3 for incorrect number of args scanned from file */
int get_proc_pid_statm(pid_t pid, proc_pid_statm_t *ptr);

/* prints a proc_pid_statm_t structure to stdout - returns 0 */
int dump_proc_pid_statm(proc_pid_statm_t *ptr);

/* inits a proc_vmstat_t structure  - returns 0 */
int init_proc_vmstat(proc_vmstat_t *ptr);

/* populates a proc_vmstat_t structure from /proc/vmstat */
int get_proc_vmstat(proc_vmstat_t *ptr);

/* prints a proc_vmstat_t structure to stdout - returns 0 */
int dump_proc_vmstat(proc_vmstat_t *ptr);

/* inits a proc_diskstats_t structure  - returns 0 */
int init_proc_diskstats(proc_diskstats_t *ptr);

/* populates a proc_diskstats_t structure from /proc/diskstats 
  returns 0 on success */
int get_proc_diskstats_by_name(char *devname, proc_diskstats_t *ptr);

/* prints a proc_diskstats_t structure to stdout - returns 0 */
int dump_proc_diskstats(proc_diskstats_t *ptr);

/* inits a proc_partitions_t structure */
int init_proc_partitions(proc_partitions_t *ptr);

/* populates a proc_partitions_t structure based on a given device name */
int get_proc_partitions_by_name(char *devicename, proc_partitions_t *ptr);

/* prints a proc_partitions_t structure on stdout */
int dump_proc_partitions(proc_partitions_t *ptr);

/* initialize cpu-specific data */
int init_sys_proc_stat(cpu_usage_stat *ptr, int numCPUs) ;

/* retrieves cpu-specific data from the 'cpuN..' entry of /proc/stat */
int get_sys_proc_stat(cpu_usage_stat *ptr, int numCPUs) ;

/* Retrieve the default environment variables to set during login */
int get_def_login_env_parms(login_parms * parms);

/* Initialize IPdata meas structrure, retrieve number of interfaces, etc */
int init_proc_net_dev (IPDataMeas *ptr,int numIntfs);
int get_number_of_interfaces(int * numIntfs);
int get_per_intf_flow_det(IPDataMeas *ptr, int numIntfs);

/* 20060823 IBM murthya : add decl for get_pid_max */
int get_pid_max(int *pidmax);

/* 20060906 IBM murthya : add decl for retrieve_process_details() */

// the 'pid' is the input to this method. proc_det is an array of ps_oput
// structures, the first of which *(proc_det[0]) contains the details of 
// the main process. if 'numThreads' ends up not being 0, 
// proc_det[1],proc_det[2]....proc_det[numThreads] contain details of 
// the threads. If proc_det is NULL, it means the process with 'pid' does
// not exist.
#if 0
int retrieve_process_details(int pid, int * numThreads , ps_oput *** proc_det);
#endif
/* 20060906 IBM murthya : add decl for readelffile */
/* This function reads the elf header at the top of the binaryFile and populates
   the file_class parameter */

int readelfFile (char * binaryFile, fileClass * file_class);

/* 20060921 IBM murthya : add decl for getExecFileNameFromPid */
/* The 'charBuf' parameter is a pointer to the buffer into which the name of the
    executable file will be written. lenOfCharBuf is the length of this buffer.
    This is needed as an input to the 'readlink()' function that is called by
	getExecFileNameFromPid()
*/
int getExecFileNameFromPid(int pid, char * charBuf, int lenOfCharBuf);

/*
	20061026 IBM murthya : methods to retrieve and dump the mappings from /proc/pid/maps
*/
/*
	read the /proc/pid/maps file and collect the details for all the records
	in the memory pointed to by 'mappings'. The memory must be large enough for
	'num_of_mappings' prmap_linux_t objects	
*/
int get_proc_pid_maps(int fd, prmap_linux_t *mappings, int num_of_mappings);
/*
	Given a prmap_linux_t object, dump its contents
*/
void dump_proc_pid_map_record(prmap_linux_t *mapping);

/*
	20061205 IBM murthya : read the contents of /proc/pid/environ for a given pid
	Returns 0 on success. -1 on failure.
	
	The function is called from within cc/misc/tools/lib/procdump/psinfo.c . So the signature
	is configured, per the requirements there.

	parms : 
		pid = the ID of the process fo which we need to read the environment
		description  = A char string that will be printed before the environment is printed
		separator = a character that will be printed between entries in the file
*/
	int dump_proc_env(int pid, const char* description, char separator);	

/*
	20061205 IBM murthya : read the contents of /proc/pid/cmdline for a given pid
	Returns 0 on success. -1 on failure.
	
	The function is called from within cc/misc/tools/lib/procdump/psinfo.c . So the signature
	is configured, per the requirements there.

	parms : 
		pid = the ID of the process fo which we need to read the environment
		description  = A char string that will be printed before the environment is printed
		separator = a character that will be printed between entries in the file
*/
	int dump_proc_cmdline(int pid, const char* description, char separator);	
#ifdef __cplusplus

};
#endif

#endif /* __ibmcm_utils_h */
