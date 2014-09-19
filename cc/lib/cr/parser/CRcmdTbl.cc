/*
**      File ID:        @(#): <MID18649 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18649
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:59
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file implements the classes CRcmdTbl, CRparmTbl
**      which are used to represent the USL parse table in the USL parser.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
#include "CRcmdTbl.H"
#include "cc/hdr/cr/CRuslParser.H"

CRcmdTbl::CRcmdTbl()
{
}

CRcmdTbl::~CRcmdTbl()
{
	/* need to loop thru each map entry and delete CRcmdEntryPtr */
	/*
	** Due to core dumps during swithc overs
	** this code has been comment out
	*/
	Mapiter<String,CRcmdEntryPtr> mi(map);
	while (++mi)
	{
		CRcmdEntryPtr entry = mi.value();
		delete entry;
	}
}

void
CRcmdTbl::insert(const String& cmdStr, CRcmdEntryPtr parmEntry)
{
	parmEntry->setCmdName(cmdStr);

	/* insert entry into table */
	map[cmdStr] = parmEntry;
}

CRcmdEntryPtr
CRcmdTbl::getEntry(const char* cmdStr)
{
	Mapiter<String,CRcmdEntryPtr> mi = map.element(cmdStr);
	if (mi)
		return mi.value();
	else
		return NULL;
}

void
CRcmdTbl::printHelp()
{
	Mapiter<String,CRcmdEntryPtr> mi(map);
	CRparser->printf("\n/* Legal commands are:");
	int firstOne = 1;

	while (++mi)
	{
		if (firstOne)
			firstOne = 0;
		else
			CRparser->printf(",");

		CRparser->printf(" %s", (const char*) mi.key());
	}
	CRparser->printf(" */");
}	

void
CRcmdTbl::dump()
{
	Mapiter<String,CRcmdEntryPtr> mi(map);
	while (++mi)
	{
		CRcmdEntryPtr entry = mi.value();
#ifdef OLDWAY
		fprintf(stderr, "%s", (const char*) mi.key());
		entry->dump();
#endif
		entry->printSyntax((const char*) mi.key());
	}
}	

void
CRcmdTbl::printManPages() const
{
	Mapiter<String,CRcmdEntryPtr> mi(map);
	while (++mi)
	{
		CRcmdEntryPtr entry = mi.value();
		entry->printManPages((const char*) mi.key());
	}
}	



CRparmTbl::CRparmTbl()
{
}

CRparmTbl::~CRparmTbl()
{
	/* need to loop thru each map entry and delete CRparmPtr */
	/*
	**	Due to core dumps on switch over
	**	this code has been comment out
	*/
	Mapiter<String,CRparmPtr> mi(map);
	while (++mi)
	{
		CRparmPtr entry = mi.value();
		delete entry;
	}
}

void
CRparmTbl::insert(const String& parmStr, CRparmPtr parmEntry)
{
	/* insert entry into table */
	map[parmStr] = parmEntry;
}

CRparmPtr
CRparmTbl::getEntry(const char* parmStr)
{
	Mapiter<String,CRparmPtr> mi = map.element(parmStr);
	if (mi)
		return mi.value();
	else
		return NULL;
}

void
CRparmTbl::dump()
{
	Mapiter<String,CRparmPtr> mi(map);
	while (++mi)
	{
		CRparmPtr entry = mi.value();
		fprintf(stderr, "%s", (const char*) mi.key());
		entry->dump();
	}
}	

/*
CRcmdTbl::delete()
{
	 need to loop thru each map entry and delete CRcmdEntryPtr 
	Mapiter<String,CRcmdEntryPtr> mi(map);
	while (++mi)
	{
		CRcmdEntryPtr entry = mi.value();
		delete entry;
	}
	
}*/
