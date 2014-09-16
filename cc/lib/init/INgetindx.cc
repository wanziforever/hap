
// DESCRIPTION:
//	This file contains an auxiliary routine used to
//	map a process id into the associated process index.
//
// FUNCTIONS:
//	_ingetindx()	- get process index into INIT shared memory
//			  given a process tag
//
// NOTES:
//
#include "cc/lib/init/INlibinit.hh"
#include <string.h>

/*
 *	Name:	
 *		_ingetindx()
 *
 *	Description:
 *		This routine searches for the process tag passed as an
 *		argument in the IN_LDPTAB[] table.  The index position
 *		is returned if the search is successful, otherwise
 *		a failure indicator is returned (GLfail).  
 *
 *	Inputs:
 *		tag - process msgh tag
 *
 *		Shared Memory:
 *			IN_LDPTAB[] table
 *
 *	Returns:
 *		Index into CONTROL table if successful, otherwise
 *		GLfail.
 *
 *	Calls:
 *		none
 *
 *	Called By:
 *		Permanent processes via the INIT() macro.
 *
 *	Side Effects:
 *			none
 *      WARNING:
 *	This function is executed before global constructors are done
 *	and therefore it cannot use any objects that have global constructors.
 */

Short _ingetindx(char * tag) {
	U_short indx;
	/*
	 *	Loop over process control table looking for matching
	 *	process tag.  Return indx position of process in table
	 *	if match is made.
	 */
	for (indx = 0; indx < IN_SNPRCMX; indx++) {
		if (IN_VALIDPROC(indx) && 
			strncmp(IN_LDPTAB[indx].proctag,tag,IN_NAMEMX) == 0) {
			return(indx);
		}
	}

	/*
	 *	No match - process not found.
	 */
	return(GLfail);
}
