#ifndef __GLRETURNS_H
#define __GLRETURNS_H

/*
**	File ID:    @(#): <MID22340 () - 02/19/00, 21.1.1.1>
**
**	File:			MID22340
**	Release:		21.1.1.1
**	Date:			05/13/00
**	Time:			13:39:22
**	Newest applied delta:	02/19/00
**
**
** DESCRIPTION:
**
** OWNER:
**	B. L. Prokopowicz
*/

#include "hdr/GLreturns.h"

/*
** Error return values should take the form:
**
** const GLretVal GLERRTYPE1 = MISC_FAIL;	// Description of ERRTYPE1
** const GLretVal GLERRTYPE2 = (MISC_FAIL-1);	// Description of ERRTYPE2
** etc.
**
** It would be helpful to indicate which functional area of code "owns" the value.
*/

/*
 * values returned by "data structure" code, e.g. GL23tree,
 *	GLidentDict, etc.
 */

		/* An attempt to allocate memory failed. */
#define GLnoMemory	(MISC_FAIL - 1)
		/* The value being looked for is not present. */
#define GLnotFound	(MISC_FAIL - 2)

#endif
