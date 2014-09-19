/*
**      File ID:        @(#): <MID18638 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18638
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:53
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file defines a USLI class that is used to represent an
**      Output Message Data Base (OMDB) key.  Class should only be
**      constructed by the CROMDBKEY macro (defined in CRomdbMsg.H).
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <string.h>
#include "cc/hdr/cr/CRomdbKey.H"
#include "cc/hdr/cr/CRdebugMsg.H"

CRomdbKey::CRomdbKey(const char* omdbKey, const char* filename, int linenum)
{
	if (strlen(omdbKey) > CRmaxOmdbEntryNm)
	{
		CRERROR("OMDB Key '%s' is too long (file: %s, line: %d)",
            omdbKey, filename, linenum);
	}

#ifdef EES
	/* should check to make sure the omdbKey is valid by looking
	** in the proper ORACLE script file for this key
	*/
#endif

	strncpy(keystr, omdbKey, CRmaxOmdbEntryNm);
	keystr[CRmaxOmdbEntryNm] = '\0';
}

CRomdbKey::CRomdbKey()
{
	keystr[0] = '\0';
}

const char*
CRomdbKey::getValue() const
{
	return keystr;
}
