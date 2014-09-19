/*
**      File ID:        @(#): <MID18648 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18648
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:59
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

#include "CRcmdTbl.H"
#include "CRctBlock.H"
#include "cc/hdr/cr/CRuslParser.H"
#include "CRerrMsgs.H"


CRcmdEntry::CRcmdEntry()
{
	init();
}

CRcmdEntry::~CRcmdEntry()
{
	/*
	** Due to core dumps on switch overs
	** This code has been comment out
	*/
	for (int i = 0; i < numVariants; i++)
	{
		CRctBlockList* bptr = getVariant(i);
		delete bptr;
	}
}

void
CRcmdEntry::init()
{
	numVariants = 0;
}

GLretVal
CRcmdEntry::add(CRctBlockList* variant)
{
	if (findVariant(variant))
	{
		return GLfail;
	}

	variant->setCmdName(cmdStr);

	numVariants++;

	/* Need to find the proper place in the list to add this 
	** variant.  The goal is to keep it in alphabetic order.
	*/
	for (int i = 0; i < numVariants-1; i++)
	{
		if (variant->lessThan(getVariant(i)))
		{
			variantList.insert((void*) variant, i);
			return GLsuccess;
		}
	}
	
	variantList.append((void*) variant);
	return GLsuccess;
}

const CRctBlockList*
CRcmdEntry::findVariant(const CRctBlockList* target) const
{
	for (int i = 0; i < numVariants; i++)
	{
		if (getVariant(i)->keysMatch(target))
			return getVariant(i);
	}
	return NULL;
}

int
CRcmdEntry::getNumVariants() const
{
	return numVariants;
}

void
CRcmdEntry::setCmdName(const char* cmdname)
{
	cmdStr = cmdname;
}

void
CRcmdEntry::printSyntax(const char* cmdname, Bool verboseFlag) const
{
	for (int i = 0; i < numVariants; i++)
	{
		CRparser->printOMstart(cmdname);
		getVariant(i)->printSyntax(verboseFlag);
		CRparser->printOM("");
	}
}

void
CRcmdEntry::printManPages(const char* cmdname) const
{
	for (int i = 0; i < numVariants; i++)
	{
		CRctBlockList* variant = getVariant(i);
		CRparser->printf("\n/*%s */\n", variant->getHelpStr());
		CRparser->printf(cmdname);
		variant->printSyntax(NO);
		CRparser->printf("\n\n");
		variant->printSyntax(YES);
		CRparser->printf("\n");
	}
}

void
CRcmdEntry::dump() const
{
	for (int i = 0; i < numVariants; i++)
	{
		CRctBlockList* variant = getVariant(i);
		fprintf(stderr, "\t");
		variant->dump();
		fprintf(stderr, "\n");
	}
}

void
CRcmdEntry::printVariants(const CRblockList* target, int minKeyMatches,
			  int numMatchingVariants, int maxPrintAtOnce)
{
	int numToPrint = maxPrintAtOnce;
	int numSuppressed = 0;
	int numPrinted = 0;

	for (int i = 0; i < numVariants; i++)
	{
		int numMatches = 0;
		CRctBlockList* variant = getVariant(i);
		variant->matches(target, numMatches);

		if (numMatches >= minKeyMatches)
		{
			if (numToPrint == 0 &&
			    numMatchingVariants - numPrinted > 1)
			{
				/* ask user if should continue */
				char mbuf[80];
				sprintf(mbuf, CRerrMoreVariants,
					numMatchingVariants - numPrinted);
				if (CRparser->yesNoResponse(mbuf) == NO)
					break;

				/* Make sure next thing printed is at the
				** start of a line.
				*/
				CRparser->printOMstart(NULL);

				numToPrint = maxPrintAtOnce;
			}
			variant->printHelp(1, cmdStr, YES);
			numToPrint--;
			numPrinted++;
		}
	}
}

void
CRcmdEntry::printHelp(const CRblockList* target, int minKeyMatches,
		      int /* helpLevel */, const char* prefix)
{
	if (prefix)
		CRparser->printOMstart(prefix);

	int numMatchingVariants = 0;

	CRctBlockList* variant = NULL;
	int i;

	for (i = 0; i < numVariants; i++)
	{
		int numMatches = 0;
		variant = getVariant(i);
		variant->matches(target, numMatches);

		if (numMatches >= minKeyMatches)
			numMatchingVariants++;
	}

	const char* headerStr = CRerrUseTheFmt;
	if (numMatchingVariants > 1)
		headerStr = CRerrUseAFmt;
	if (prefix)
		CRparser->printf(headerStr);
	else
		CRparser->printOMstart(headerStr);

	CRparser->printf(":\n");

	const int CRmaxPrintVariants = 17;
	printVariants(target, minKeyMatches,
		      numMatchingVariants, CRmaxPrintVariants);
}

void
CRcmdEntry::printHelp(int /* helpLevel */, const char* prefix)
{
	const char* headerStr = CRerrUseTheFmt;

	if (numVariants > 1)
		headerStr = CRerrUseAFmt;

	CRparser->printOMstart("/*\n");

	if (prefix)
		CRparser->printf(prefix);

	CRparser->printf("%s:\n", headerStr);

	for (int i = 0; i < numVariants; i++)
	{
		getVariant(i)->printHelp(1, cmdStr, YES);
	}
	CRparser->printOM("*/");
}


/* sets numParmMatches to the maximum number of parameters that matched */
CRctBlockList*
CRcmdEntry::findVariant(const CRblockList* target, int& maxParmMatches)
{
	int numOfFoundVariants = 0;


	if (numVariants == 1)
		return getVariant(0);

	CRctBlockList* retval = NULL;
	maxParmMatches = 0; /* number of matching parms even if does not
		            ** "match" all the keys.
			    */
	int maxAllMatchingKeys = 0;

	int totalFoundVariants[1000];

	for (int i = 0; i < numVariants; i++)
	{
		CRctBlockList* variant = getVariant(i);

		int numMatchingKeys = 0;
		if (variant->matches(target, numMatchingKeys))
		{
			if (numMatchingKeys > maxAllMatchingKeys)
			{
				maxAllMatchingKeys = numMatchingKeys;
				retval = variant;
				numOfFoundVariants = 0;
			}

			if (maxAllMatchingKeys > 0)
			{
				if (numMatchingKeys == maxAllMatchingKeys)
				{
				   if(numOfFoundVariants < 1000)
				     totalFoundVariants[numOfFoundVariants] = i;

				   ++numOfFoundVariants;
				}
	
			}
		}

		if (numMatchingKeys > maxParmMatches)
		{
			maxParmMatches = numMatchingKeys;
		}
	}

	int ii = 0;
	int maxMatchingNonKeys = 0;
        if(numOfFoundVariants < 1000)
	{
	  for(ii=0; ii < numOfFoundVariants; ++ii)
	  {

		CRctBlockList* newVariant = getVariant(totalFoundVariants[ii]);

		int numMatchingNonKeys = 0;
		if (newVariant->nonKeyMatches(target, numMatchingNonKeys))
		{
			if (numMatchingNonKeys > maxMatchingNonKeys)
			{
				maxMatchingNonKeys = numMatchingNonKeys;
				retval = newVariant;
			}

		}
	  }
	}

	return retval;
}

/* Check all blocks of all variants of the command for any positional
** blocks. Return YES if one or more found; otherwise, return NO.
** In reality, there should be only one variant for a command that has
** any positional blocks.
*/

Bool
CRcmdEntry::isPositional() const
{
	int		numBlocks;

	CRctBlockList*	blockList = NULL;
	CRctBlock*	block = NULL;

	for (int i = 0; i < numVariants; i++)
	{
		blockList = getVariant(i);
		numBlocks = blockList->length();

		for (int j = 0; j < numBlocks; j++)
		{
			block = (CRctBlock*) blockList->getBlock(j);
			if (block->isPositional())
				return YES;
		}
	}

	return NO;
}
