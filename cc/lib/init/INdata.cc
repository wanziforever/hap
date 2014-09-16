
// DESCRIPTION:
//	This file includes the definition of the IN_ldata structure used
//	by the INIT library included by each permanent process.
//
// NOTES:
//

#include "cc/lib/init/INlibinit.hh"

IN_SDATA	*IN_sdata;
IN_LDATA	INlocal_data = {-1};
IN_LDATA	*IN_ldata = &INlocal_data;

/* Provide local process space for INIT shared memory proxy to be
** used in LAB mode.
** Reserve space for shared data sufficient for common info and one
** process table entry.
** Use arrays of long to insure alignment and add 1 to size to compensate for roundoff.
*/
Long	INldata[((sizeof(IN_PROCDATA) - (sizeof(IN_PROCESS_DATA) * (IN_SNPRCMX - 1)))/sizeof(Long)) + 1];
Long	INsdata[((sizeof(IN_SDATA) - (sizeof(IN_PTAB) * (IN_SNPRCMX - 1)))/sizeof(Long)) + 1];

/*
 *      This array contains the text strings used when printing out the
 *      process state as part of the OP:INIT report.
 */
const char *IN_procstnm[] = {
  "IN_INVSTATE",
  "IN_NOEXIST",
  "IN_CREATING",
  "IN_HALTED",
  "IN_RUNNING",
  "IN_DEAD",
};

/*
 *      This array contains the text strings used when printing out the
 *      process sequence step as part of the OP:INIT;PS report.
 */
const char *IN_sqstepnm[] = {
  "INV_STEP",
  "IN_BGQ",
  "IN_GQ",
  "IN_EGQ",
  "IN_BSU",
  "IN_SU",
  "IN_ESU",
  "IN_READY",
  "IN_BHALT",
  "IN_HALT",
  "IN_EHALT",
  "IN_BSTARTUP",
  "IN_STARTUP",
  "IN_ESTARTUP",
  "IN_BSYSINIT",
  "IN_SYSINIT",
  "IN_ESYSINIT",
  "IN_BPROCINIT",
  "IN_PROCINIT",
  "IN_EPROCINIT",
  "IN_BPROCESS",
  "IN_PROCESS",
  "IN_EPROCESS",
  "IN_CPREADY",
  "IN_ECPREADY",
  "IN_BSTEADY",
  "IN_STEADY",
  "IN_BCLEANUP",
  "IN_CLEANUP",
  "IN_ECLEANUP",
};

/*
 *      This array contains text strings representing the current
 *      SN recovery level.
 */
const char *IN_snlvlnm[] = {
  "SN_INV",       /* Uninitialized                                */
  "SN_NOINIT",    /* No SN Initialization                         */
  "SN_LV0",       /* Single Process - don't reload provisioning info */
  "SN_LV1",       /* Single Process - reload provisioning info    */
  "SN_LV2",       /* System Reset - don't reload provisioning info */
  "SN_LV3",       /* System Reset - reload provisioning info      */
  "SN_LV4",       /* Bootstrap - re-initialize all data           */
  "SN_LV5",       /* UNIX boot or switchover			*/
};

