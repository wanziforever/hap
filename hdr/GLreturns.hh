#ifndef	_GLRETURNS
#define	_GLRETURNS
/*
** File ID:	@(#): <MID7870 () - 08/17/02, 29.1.1.1>
**
** File:		MID7870
** Release:		29.1.1.1
** Date:		08/21/02
** Time:		21:30:56
** Newest applied delta:08/17/02
*/

/*
**	Owner:  Mavis A. Bates
*/

#if defined(c_plusplus) | defined(__cplusplus)
#	include "hdr/GLtypes.hh"
#	define GLsuccess	0
#	define GLfail		-1
#else
#	define GLSUCCESS	0
#	define GLFAIL		-1
#endif

#define HW_FAIL		-100	/* PC DEVICE DRIVER (hdr/GLhwnum.h) */
#define	OS_FAIL		-1000
#define SU_FAIL 	-1500	/* cc/hdr/su/SUreturns.H  */
#define	ISLU_FAIL	-2000	/* hdr/sm/is/ISglobals.h */
#define ASI_FAIL	-2500	/* cc/asi/hdr/ASreturns.H */
#define	CI_FAIL		-3000	/* hdr/sm/ci/CIglobals.h */
#define EIB_FAIL	-3500	/* hdr/sm/ap/eib/EBreturns.h */
#define	TSI_FAIL	-4000	/* hdr/sm/ts/TSglobals.h */
#define MISC_FAIL	-4500	/* cc/hdr/misc/GLreturns.H */
#define	MP_FAIL		-5000
#define	MD_FAIL		-6000	/* hdr/sm/md/MDglobals.h */
#define	FT_FAIL		-6500	/* cc/hdr/ft/FTreturns.H */
#define DSC_FAIL	-7000	/* hdr/sm/ds/DSglobals.h */
#define GDSU_FAIL	-8000	/* hdr/sm/cf/CFglobals.h */
#define PSU_FAIL	-9000	/* hdr/sm/ps/PSglobals.h */
#define PI_FAIL		-10000	/* hdr/sm/pi/PIglobals.h */
#define EC_FAIL		-11000	/* hdr/sm/eic/ECglobals.h */
#define MSGH_FAIL	-12000	/* cc/hdr/msgh/MHresult.H */
#define MS_FAIL		-12500	/* cc/hdr/msgh/MSresult.H */
#define SCEI_FAIL	-13000	/* cc/hdr/scei/SEreturns.H */
#define SA_FAIL		-14000	/* cc/hdr/sa/SAresult.H */
#define TT_FAIL		-15000	/* cc/hdr/tts/TTreturns.H */
#define TCPSCH_FAIL     -15500  /* cc/hdr/tcpsch/TIdefs.H */
#define SMSCH_FAIL	-16000	/* cc/hdr/smsch/SMglobals.H */
#define LIH_FAIL	-17000	/* cc/hdr/smsch/lih/LIglobals.H */
#define ISUP_FAIL       -17500  /* cc/hdr/isupfmt/ISreturns.H */
#define SP_FAIL		-18000	/* cc/spa/hdr/SPreturns.H */
#define DI_FAIL		-18200	/* cc/sce/hdr/debug/DIreturns.H */
#define TM_FAIL		-19000	/* cc/hdr/tim/TMreturns.H */
#define DBOP_FAIL	-20000   /* cc/hdr/db/DBretval.H */
#define RDB_FAIL	-20500   /* cc/hdr/rdb/RDBretval.H */
#define	PRI_FAIL	-21000	/* hdr/sm/pri/PRI_defs.h */
#define IN_FAIL		-22000	/* cc/hdr/init/INreturns.H */
#define EH_FAIL		-23000	/* cc/hdr/eh/EHreturns.H */
#define CSCF_FAIL	-24000	/* cc/hdr/cscf/CSreturns.H */
#define	TS_FAIL		-24500	/* cc/hdr/ts/TSreturns.H */
#define BILL_FAIL	-25000  /* cc/hdr/bill/BLreturns.H */
#define	NSI_FAIL	-25500	/* cc/nsi/hdr/NSreturns.H */
#define SMSI_FAIL	-26000	/* cc/hdr/smsi/SSreturns.H */
#define X25_FAIL 	-27000	/* cc/hdr/x25/x25.H	   */
#define CUSCH1_FAIL 	-28000	/* customer sch 1	   */
#define CUSCH2_FAIL 	-29000	/* customer sch 2	   */
#define CUSCH3_FAIL 	-30000	/* customer sch 3	   */
#define SCI_FAIL 	-31000	/* cc/hdr/sci/SCreturns.H  */
#define	DLTU_FAIL	-32000	/* hdr/sm/dl/DLtu_defs.h */
#define IP_FAIL		-32000  /* cc/hdr/ip/IPreturns.H */
                                /* IPSCH re-uses DLTU failure reutrns */
#define EM_FAIL		-33000  /* cc/hdr/ecam/EMreturns.H */

/*
** GLretval is a short.  The maximum value it can hold
** -32767.  To add more subsystems, contact the owner of
** the file.
*/

typedef short GLretVal;

/*
**	The follow macros test for simple success or failure.
**	If a program needs more resolution, a switch or if
**	can be made on the specific values of interest.
*/

#if defined(c_plusplus) | defined(__cplusplus)

inline Bool
GLisSuccess(GLretVal x) {
	Bool	b;
	if (x >= 0) {
		b = TRUE;
	} else {
		b = FALSE;
	}
	return b;
}

inline Bool
GLisFail(GLretVal x) {
	Bool	b;
	if (x < 0) {
		b = TRUE;
	} else {
		b = FALSE;
	}
	return b;
}

#else

#	define IS_SUCC(x)	( (x) >= 0 )	/* successes are assumed to be >= 0 */
#	define IS_FAIL(x)	( (x) < 0 )	/* failures are assumed to be < 0 */

#endif

typedef short RET_VAL;

#define _GLRETURNS
#endif
