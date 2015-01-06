#include "cc/hdr/cr/CRmsgClass.hh"

/*
**      File ID:        @(#): <MID18635 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18635
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:53
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file defines the constants declared in CRmsgClass.H
**
** OWNER:
**	Roger McKee
**
** NOTES:
*/

static const char* CRmsgClassXlation[] = { 
	"SPERR", /* 0 */
	"MAINT", /* 1 */
	"MAIPR", /* 2 */
	"DEBUG", /* 3 */
	"AUDT",  /* 4 */
	"AUDL",  /* 5 */
	"RC",    /* 6 */
	"ASRT",  /* 7 */
	"SED",   /* 8 */
	"PLNT",  /* 9 */
	"TRFM",  /* 10 */
	"INECHO",  /* 11 */
	"TTLOG",  /* 12 */
};

const int CRnumClassNum = sizeof(CRmsgClassXlation) / sizeof(char*);

CROMclass::CROMclass(CROMCLASS omClassNum)
{
	if (omClassNum >= CRnumClassNum)
     omClassNum = CL_SPERR;

	strcpy(name, CRmsgClassXlation[omClassNum]);
}
