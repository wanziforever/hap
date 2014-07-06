#ifndef _INLIB_H
#define _INLIB_H

// DESCRPTION:
//  This file contains INIT subsystem interface definitions that
//  are available to any process in the system. not necessarily
//  only one running under INIT control

#include "hdr/GLtypes.hh"  // SN-wide typedefs
#include "hdr/GLreturns.hh" // SN-wide return values

#if defined(c_plusplus) | defined(__cplusplus)
extern "C" Long INvmemsize();
extern "C" GLretVal INattach();
extern "C" void INdetach();
extern "C" GLretVal INprio_change(int);
#else
extern Long INvmemsize();
extern RET_VAL INattach();
extern void INdetach();
extern RET_VAL INprio_change();
#endif

// This macro returns current value of available virtual memory
// in K bytes. If a problem is encountered a negavie value
// is returned.
#define INVMEMSIZE() INvmemsize()

// This macro allows processes to change their priority. Processes
// can always lower their priority, and they can increase it up to
// the maximum priority allowed for their processes. The priority
// ranges accepted are relative values 0-39 (0 is highest)
// using the priority definition provided in initlist.
// If this function is called with priority value of 0, process
// priority will be reset to the maximum original process priority.
// If priority value is > IN_MAXPRIO. process priority will be set to
// lowest priority.
// This macro returnes GLfail if priocntl() system calls where unsuccessfull
// and GLsuccess otherwise.
#define INPRIO_CHANGE(priority) INprio_change(priority)

#endif


