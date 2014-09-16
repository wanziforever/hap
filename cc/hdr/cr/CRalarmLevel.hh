#ifndef __CRALARMLEVEL_H
#define __CRALARMLEVEL_H
/*
**      File ID:        @(#): <MID13727 () - 08/17/02, 29.1.1.1>
**
**	File:					MID13727
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:32:17
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the constants that represent the various
**      alarm levels for Output Messages (OM)
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**      The values of the constants in this file are important;
**      they are to be used to control the order in which the messages
**      are displayed.  Note all alarm levels must be greater than 0.
*/

/* priority of action fields values */
typedef unsigned char CRALARMLVL;

#define POA_CRIT     1 /* critical */
#define POA_DEFAULT  2 /* default (use value in OMDB) */
#define POA_MAJ      3 /* major */
#define POA_MIN      4 /* minor */
#define POA_CLEAR    5 /* manual */
#define POA_MAN      6 /* manual */
#define POA_ACT      7 /* automatic */
#define POA_INF      8 /* information only */
#define POA_TCR      9 /* transient critical */
#define POA_TMJ      10 /* transient major */
#define POA_TMN      11 /* transient minor */
#define POA_MAXALARM 20 /* number to see if its a PRM or incident */

/* convert POA to three character string */
const char* CRPOA_to_str(CRALARMLVL alarmIndex);

/* convert 3 character string to POA */
CRALARMLVL CRstr_to_POA(const char* alarmString);

#define CRDEFALMSTR "   "
#define CRALARMSTRMAX	3

#endif
