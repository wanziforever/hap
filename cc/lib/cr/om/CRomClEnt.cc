/*
**      File ID:        @(#): <MID28523 () - 08/17/02, 26.1.1.1>
**
**	File:					MID28523
**	Release:				26.1.1.1
**	Date:					08/21/02
**	Time:					19:39:56
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the declaration of the CRomClassEntry class.
**      This class represents an Output Message class entry in the OMDB.
**      Specifically, one CRomClassEntry object corresponds to a row of
**      the ORACLE table 'cr_msgcls'.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <stdio.h>
#include <string.h>
#include "cc/cr/hdr/CRomClEnt.H"
#include "cc/cr/hdr/CRomHdrFtr.H"
#include "cc/cr/omm/CRcsopMsg.H"
#include "cc/cr/omm/CRcsopOMs.H"
#include "cc/cr/omm/CRomdb.H"
#include "cc/hdr/cr/CRdebugMsg.H"


CRomClassEntry::CRomClassEntry()
{
  numDests = 0;
  nextSeqnum = 0;
}
 
CRomClassEntry::~CRomClassEntry()
{
}
 
 
int
CRomClassEntry::genNextSeqnum()
{
 
  if (nextSeqnum < CRMAXSEQNUM)
 
  {
    return nextSeqnum++;
  }
  else
     return nextSeqnum=0;
}     
 

const char*
CRomClassEntry::getDestName(int nth) const
{
	return dest[nth];
}

const CRdestEntryPtr
CRomClassEntry::getDestPtr(int nth) const
{
	return destptr[nth];
}

int
CRomClassEntry::getNumDests() const
{
	return numDests;
}

void
CRomClassEntry::dump() const
{
	fprintf(stderr, "numDest=|%d|", getNumDests());
	for (int i = 0; i < getNumDests(); i++)
	{
		fprintf(stderr, ", %s ", (const char*) (dest[i]));
	}
	fprintf(stderr, "\n");
}

GLretVal
CRomClassEntry::addDest(const char* destName, CRdestEntryPtr destPtr,
                        char* failreason)
{
	if (numDests >= CR_MSGCLS_MAXDESTS)
	{
		strcpy(failreason, "too many destinations for message class");
		return GLfail;
	}

	if (destName)
	{
		/* Make sure destName is not already in the msgclass entry */
		for (int i = 0; i < numDests; i++)
		{
			if (strcmp(dest[i], destName) == 0)
			{
				sprintf(failreason,
                "Destination '%s' entered more than once",
                destName);
				return GLfail;
			}
		}

		dest[numDests] = destName;
		destptr[numDests] = destPtr;
		numDests++;
	}
	return GLsuccess;
}


CRomClassTbl::CRomClassTbl() : table()
{
}

Bool
CRomClassTbl::isSpecialMsgClass(const char* mc)
{
	if (strcmp(mc, "ASRT") == 0
	    || strcmp(mc, "DEBUG") == 0
	    || strcmp(mc, "INECHO") == 0
	    || strcmp(mc, "R") == 0
	    || strcmp(mc, "SPAMTCL") == 0
	    || strcmp(mc, "SPAMTCR") == 0
	    || strcmp(mc, "SPAMTCB") == 0
	    || strcmp(mc, "T") == 0
	    || strcmp(mc, "TR") == 0
    )
	{
		return YES;
	}
	else
	{
		return NO;
	}
}

GLretVal
CRomClassTbl::load()
{
	const char* tblname = "cr_msgcls";
	GLretVal retval = GLsuccess;

	if ((retval = DBloadTable(tblname)) != GLsuccess)
     return retval;

	int tupleCount = 0;

	/* DBgetTuple() returns 0 if end, < 0 for error. */
	while ((retval = DBgetTuple(CR_MSGCLSatt)) > 0)
	{
		/* Get message class name		*/
		char* msgclass = CR_MSGCLSatt[CRCL_MSGCLSCOL].value;
		CROMclass omClass = msgclass;

		delClassEntry(msgclass);
		CRomClassEntry* classEntryPtr = insertMsgClass(msgclass);

		for (int i = 0; i < CR_MSGCLS_MAXDESTS; i++)
		{
			if (!CR_MSGCLSatt[CRCL_DEST0COL+i].set)
         continue;

			/* Get Destination Id		*/
			const char* destName = CR_MSGCLSatt[CRCL_DEST0COL+i].value;
			if (*destName == '\0')
         continue;

			CRdestEntryPtr destEntryPtr = omdb.getDestEntry(destName);

			if (destEntryPtr == NULL)
			{
				CRcsopMsg om;
				om.spool(&CRbadDestOM, msgclass,
                 (const char*) destName);
			}
			else
			{
				char failreason[100];
				if (classEntryPtr->addDest(destName,
                                   destEntryPtr,
                                   failreason) == GLfail)
				{
					CRcsopMsg om;
					om.spool(&CRbadDestOM, msgclass,
                   (const char*) destName);
				}
			}
		} /* end for each dest */

		tupleCount++;
	}

	if (tupleCount == 0)
	{
		CRcsopMsg om;
		om.spool(&CRnoTuplesOM, tblname);
		return GLfail;
	}

	return retval;
}

CRomClassEntry*
CRomClassTbl::getClassEntry(const char* msgclass)
{
	if (msgclass && table.element(msgclass))
     return table[msgclass];

	return NULL;
}

CRomClassEntry*
CRomClassTbl::insertMsgClass(const char* msgclass)
{
	CRomClassEntry* retval = new CRomClassEntry;
	table[msgclass] = retval;
	return retval;
}

void
CRomClassTbl::delClassEntry(const char* msgclass)
{
	CRomClassEntry* classEntryPtr = getClassEntry(msgclass);

	if (classEntryPtr)
	{
		table[msgclass] = NULL;
		delete classEntryPtr;
	}
}

void
CRomClassTbl::dump()
{
	fprintf(stderr, "dump Message Class table\n");

	/* how to print the table? Need to iterate over the table*/
	for (Mapiter<CROMclass,CRomClassEntryPtr> ci(table);
	     ci; ++ci)
	{
		CRomClassEntryPtr classEntPtr = ci.value();
		fprintf(stderr, "name=%s ", ci.key().getCharStar());
		classEntPtr->dump();
	}
}

