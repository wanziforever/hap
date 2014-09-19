/*
**      File ID:        @(#): <MID18615 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18615
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:44
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <stdio.h>
#include <string.h>
#include "cc/cr/hdr/CRgenMHname.H"

/* raise x to n-th power; asumes n > 0 */
static int
CRpower(int x, int n)
{
	int p = 1;
	for (int i = 1; i <= n; i++)
		p = p * x;
	return p;
}

/* Generate a unique MSGH name by taking a prefix string and a unique
** number.  These two items are combined to form a MSGH name.
** The as many of the least significant digits of the uniqueNum are
** used as there is space for.  
** A good way to insure a unique number is to use the UNIX process id
** of the process associated with the MSGH name.
** The resulting MSGH name is stored in the 'result' parameter.
*/
void
CRgenMHname(const char* prefix, int uniqueNum,
	      char result[])
{
	int maxnum = CRpower(10, MHmaxNameLen - strlen(prefix));
	sprintf(result, "%s%d", prefix, uniqueNum % maxnum);
}

