#ifndef __CRDEBUGMSG_H
#define __CRDEBUGMSG_H

// DESCRIPTION:
//      This file defines three USLI macros: CRERRINIT, CRDEBUG, and CRERROR.
//
//      CRERRINIT is used to setup some global variables needed for later
//      calls to the CRDEBUG and CRERROR macros.
//
//      CRDEBUG is used to generate debugging output messages that
//      are controlled by 'traceflags'.
//
//      CRERROR is used to generate a 'defensive check failure' output
//      message.
//
//      This file also defines and describes the traceflags scheme used
//      by the CRDEBUG macro.
//
// OWNER:
//      Roger McKee
//
// NOTES:


#include <stdio.h>
#include <stdarg.h>
#include <thread.h>
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/cr/CRmtype.hh"
#include "cc/hdr/cr/CRmsg.hh"

/*
** The macro CRDEBUG conditionally formats and sends a debugging
** message to the CR subsystem.
** The debugging message will be logged in the logfiles /sn/log/OMlogX
** (where X is either the digit 0 or 1) only if the traceflag condition
** is true.
** The format string works the same as the C function printf(2).
** Note that an extra set of parentheses are required around the print-like
** format string and variable parameters,
** due to macro preprocessor limitations.
**
** The CRERRINIT macro should be called before the first CRDEBUG macro
** is called.
** The process should be attached to MSGH
** before calling the CRDEBUG macro.
** If the process is not attached to MSGH (or MSGH is dead),
** then CRDEBUG will log the message in a logfile named \f(CWOM\fR in
** the current directory.
**
** The traceflag is a single value that is tested against a global
** set of trace flags set for each process.
** These flags are by default all cleared (set to false (0)).
** Each process can have a file that lists the trace flags that will be
** set to true (1) when the CRERRINIT macro is called for the process.
** The file consists of a list of decimal numbers representing the trace flags
** to be set.
** By default, all flags not listed are cleared.
** The flags can be separated by whitespace or a comma.
** This file must be located in the current directory that the process runs
** in (usually in a subdirectory of /sn/core).
** The file is normally named following the pattern procname_flags,
** where procname is the name of the process.
** The one exception to this rule is that procname of CEP is used
** for all CEPs (Command Execution Programs), since their actual process
** name is difficult to determine in advance.
**
** The settings of the trace flags for a process can also be controlled
** by the TRACE USL command.
** The TRACE command has two parameters: a process name and a symbolic
** trace flag name.
** Each symbolic trace flag name actually represents an entire map of all
** the possible individual trace flags.
** There are two special flag names: "on" and "off".
** "on" sets all of the trace flags for the specified process,
** while "off" clears all of the flags.
** All other symbolic flag names can be defined in the file, /sn/cr/traceflags.
** Blank lines are allowed and any line starting with a '#' is a comment.
** Each symbolic flag is defined on a separate line in the file.
** Each symbolic flag name must be in lowercase and it must start the line.
** A colon ':' is used to separate the name from its list of decimal
** trace flags.  The list of trace flags must be on a single line.
*/

/*
** Each subsystem may use the bits allocated to it as it chooses, defining
** its own meanings.  Suggested bit settings for some of the bits are
** given below with the type CRdbflag.  Each developer should have a bit
** or group of bits that implement each one of the suggested functions.
*/
typedef Short CRsubsys;
#define CRusli		0x0
#define CRbill		0x40
#define CReadasi	0x80
#define CRadei		0xc0
#define CRasi		0x100
#define CRmeas		0x140
#define CRdb		0x180
#define CRsched		0x1c0
#define CRinit		0x200
#define CRadeload	0x240
#define CRdegr		0x280
#define CRmsgh		0x2c0
#define CRspman		0x300
#define CRsci		0x340
#define CRsfsch		0x380
#define CRcscfsch	0x3c0
#define CRecam		0x3c0	/* eCAM is using CSCFSCH's value */
#define CRrwpsch	0x400
#define CRspa0		0x440
#define CRspa1		0x480
#define CRspa2		0x4c0
#define CRtt		0x500
#define CRs7sch		0x540
#define CRdi   		0x580
#define CRseasi		0x5c0	
#define CRsmsisch	0x600
#define CRsu		0x640
#define CRgl		0x680
#define CRcust1		0x6c0 /* reserved for customer */
#define CRcust2		0x700 /* reserved for customer */
#define CRcust3		0x740 /* reserved for customer */
#define CRcust4		0x780 /* reserved for customer */
#define CRcust5		0x7c0 /* reserved for customer */
#define CRcust6		0x800 /* reserved for customer */
#define CRcust7		0x840 /* reserved for customer */
#define CRcust8		0x880 /* reserved for customer */
#define CRcust9		0x8c0 /* reserved for customer */
#define CRcust10	0x900 /* reserved for customer */
#define CRftsch		0x940
#define CRrc		0x980
#define CRx25sch	0x9c0
#define CRipsch 	0x9c0 /*X25SCH flag is being re-used for IPSCH */
#define CRappmap	0xa00
#define CRtcap		0xa40
#define CRtcpsch	0xa80
#define CRspare7	0xb00
#define CRnuance	0xb00 /* Nuance CP uses spare7 */
#define CRsbcsch	0xb40 /* SBCsch uses spare8 */
#define CRspare9	0xb80 /* spare9 is here for backward compatibility */
#define CRmproc		0xb80 /* MPROC uses spare9 */
#define CRspare10	0xbc0
#define CRdiamsch       0xbc0

#define CRisupsch	0xc00
#define CRtsip		0xc40
#define CRtssch		0xc80
#define CRfm		0xcc0
#define CRs7tssch	0xd00
#define CRdhproc	0xd40
#define CRnsi		0xd80
#define CRus		0xe00
#define	CRjpp		0xe40
#define	CRldap		0xe40	/* mutually exclusive w/CRjpp, let's reuse */
#define CRtmg		0xe80
#define CRstsch         0xec0
#define CRsctpsch       0xf00
#define CRmof		0xf40
#define CRcdrsch        0xf80
// The following spares can be used for new subsystems in R26 and later
#define CRspare11        0xfc0 /* spare11 is here for backward compatibility */
#define CRsipsch	 0xfc0 /* SIPSCH uses spare11 */
#define CRspare12        0x1000
#define CRODBCsch       0x1000  /* ODBCSCH uses spare12 */
#define CRspare13        0x1040
#define CRmuxsch	 0x1040 /* MUXSCH uses spare13 */
#define CRspare14        0x1080
#define CRpf	 		 0x1080 /* PF uses spare14 */
#define CRdnssch         0x10c0	 /* Uses CRspare15 */
#define CRspare16        0x1100
#define CRospsch	 0x1100  /* OSPSCH use CRspare16 */
#define CRspare17        0x1140
#define CRspare18        0x1180
#define CRspare19        0x11c0
#define CRspare20        0x1200
#define CRspare21        0x1240
#define CRspare22        0x1280

/* WARNING: DON'T CHANGE CRmaxflag DURING ANY SU !!!!!!
   IT CAN ONLY BE CHANGED IN A NEW RELEASE !!!!!! */

#define CRmaxflag       0x1300

/* WARNING: DON'T CHANGE CRmaxflag DURING ANY SU !!!!!!
   IT CAN ONLY BE CHANGED IN A NEW RELEASE !!!!!! */

#define CRnumTraceBytes  (CRmaxflag / 8)

/* Type CRdbflag is for the seperate debugging of different kinds of
** processing within a subsystem.  These numbers will are added to the
** CRsubsys number to provide the ultimate trace bit.  These bit settings
** are only suggestions, however, each developer should at least have a group
** of bits which correspond to these functions.  The numbers can
** range up to 64.
*/

typedef Short	CRdbflag;

/* print every function entry & return + args	*/
#define CRinoutFlg		0

/* Print every message received.	*/
#define CRmsginFlg		1

/* Print every message sent.		*/
#define CRmsgoutFlg		2

/* Call processing debug messages.	*/
#define CRcallprocFlg		3

/* Audit activity messages.		*/
#define CRauditFlg		4

/* Measurements activity		*/
#define CRmeasFlg		5

/* Database interactions.		*/
#define CRdbFlg			6

/* Init interaction.			*/
#define CRinitFlg		7

/* SPMAN interaction.			*/
#define CRspmanFlg		8

/* SCH interaction.			*/
#define CRschFlg		9

/* AEX interaction.			*/
#define CRaexFlg		10

/* SPP interaction.			*/
#define CRsppFlg		11
 
/* Non-SPA interaction.			*/
#define CRnsFlg			12

/* Maximum trace flag value for each subsystem.	*/
#define CRmaxFlg		64

/* used for feature locking debug */
#define CRfeatStat 		30


class CRtraceMap {
    friend class CRtraceTbl;
      public:
	CRtraceMap();

	void dump() const;
	Bool setMap(const char* valueName);
	void setBit(Short bitnum);
	void clearBit(Short bitnum);
	unsigned char* getByteStart(Short bitnum);
	inline Bool isBitSet(Short bitnum) const;

      private:
	Bool findEntry(FILE* fp, const char* flagNmae);
	Bool processMatch(char* inbuf);
	void procDecBits(char* firstDecBit);
	void setAllBits(Bool value /* 1 or 0 */);
	Bool scanForBit();

      private:
	unsigned char bitmap[CRnumTraceBytes];
};


class CRtraceTbl {
      public:
	CRtraceTbl();

	void init();
	void dump() const;
	void copy(Bool anyBitsSet, const CRtraceMap& from);
	void setTraceFlag(Bool newValue); /* temporary function */

	inline Bool isBitSet(Short bitnum) const;
	inline Bool anyBitSet() const;

	void setBit(Short bitnum);
	void clearBit(Short bitnum);

      private:
	void procDecBits(char* firstDecBit);
	void setAllBits(Bool value /* 1 or 0 */);
	Bool setMap(FILE* locaTraceFp);

      private:
	Bool tracingFlag;
	CRtraceMap tracemap;
};

//
//	This class is used for the CRERROR function.
//	In order to make the CRERROR marco "thread safe"
//	it was recommended to change the CRERROR marco into a 
//	function. This class is called from the new CRERROR "function"
//
class CRerrorObj {
	public:
#if __SUNPRO_CC != 0x420
		// Can't change the signature of this function until
		// after we port to the new compiler, cause of shared libs
		CRerrorObj(const char * ,int);
#else
		CRerrorObj(char * ,int);
#endif 
		// this spool calls the CRmsg add and spool functions
		void spoolErr(const Char * format, ...);
  
		int line; 	// the line number of the CRERROR
		const char * file; 	// the file name of the CRERROR
 };

extern CRtraceTbl CRtraceFlags;

extern Char		CRprocname[];	/* Process name, set by CRERRINIT */

/* CRERRINIT must be called before CRERROR or CRDEBUG macros are used.
** It should only be called once for each process, and the MSGH name
** should be it's argument.
*/
extern void CRerrInit(const char*);

#define	CRERRINIT(PROCNAME) CRerrInit(PROCNAME);

/* the CRERROR macro is used to send ERROR messages to log files for later
** use by the developer.  Usage: CRERROR(printf-like-arguments).
*/
extern CRmsg* CRerrorPtr();

extern const char *CRerrFmt;
extern const char *CRdbgFmt;
extern const char *CRdbg2Fmt;

extern mutex_t CRlockVarible;

#define CRERROR								\
        CRerrorObj(__FILE__,__LINE__).spoolErr

extern CRmsg* CRdebugPtr();

/* The CRDEBUG_FLAGSET macro is used to determine if a particular trace/debug
** flag is currently set.  It returns non-zero (true) if the flag is set.
*/
#define CRDEBUG_FLAGSET(TRFLAG) \
        (CRtraceFlags.anyBitSet() && CRtraceFlags.isBitSet(TRFLAG))

#define CRDEBUG_PRINT(TRFLAG, STRING) \
	{ \
	    mutex_lock(&CRlockVarible); \
	    CRdebugPtr()->add(CRdbgFmt, CRprocname, TRFLAG, __FILE__, __LINE__) \
		.spool STRING; \
	    mutex_unlock(&CRlockVarible); \
	}

#define CRDEBUG(TRFLAG, STRING) { \
        if (CRDEBUG_FLAGSET(TRFLAG)) \
        { \
	        CRDEBUG_PRINT(TRFLAG, STRING) \
	} \
	}


/* CRDEBUG2 is like CRDEBUG, except it just checks to see if any
** of the trace bits are set, instead of a specific trace bit.
** This is an obsolete macro that should not be replaced by CRDEBUG calls.
** It is kept for backwards compatibility with a previous version of CRDEBUG.
*/


#define CRDEBUG2(STRING) \
if (CRtraceFlags.anyBitSet() == YES) \
{ \
	CRdebugPtr()->add(CRdbg2Fmt, CRprocname, __FILE__, __LINE__) \
	             .spool STRING; \
}



inline Bool
CRtraceMap::isBitSet(Short bitnum) const
{
	return bitmap[bitnum >> 3] & (1 << (bitnum & 0x7)) ? YES : NO;
}

inline Bool
CRtraceTbl::isBitSet(Short bitnum) const
{
	return tracemap.isBitSet(bitnum);
}

inline Bool
CRtraceTbl::anyBitSet() const
{
	return tracingFlag;
}

inline unsigned char*
CRtraceMap::getByteStart(Short bitnum)
{
	return &bitmap[bitnum >> 3];
}

#endif
