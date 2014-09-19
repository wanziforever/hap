/*
**      File ID:        @(#): <MID18650 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18650
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

#include <String.h>
#include <Map.h>
#include "hdr/GLreturns.h"
#include "CRctParm.H"
#include "cc/hdr/cr/CRsllist.H"

class CRctBlockList;
class CRblockList;

class CRcmdEntry 
{
    public:
	CRcmdEntry();
	~CRcmdEntry();
	void init();
	GLretVal add(CRctBlockList* variant);
	int getNumVariants() const;
	void dump() const;
	CRctBlockList* findVariant(const CRblockList* target,
				   int& maxKeyMatches);
	void printHelp(const CRblockList* target, int minKeyMatches,
		       int helpLevel = 1, const char* prefix = NULL);
	void printHelp(int helpLevel = 1, const char* prefix = NULL);
	void printSyntax(const char* cmdname, Bool verboseFlag = NO) const;
	void printManPages(const char* cmdname) const;
	void setCmdName(const char* cmdStr);

	Bool	isPositional() const;

    private:
	void printVariants(const CRblockList* target, int minKeyMatches,
			   int numMatchingVariants, int maxPrintAtOnce);
	CRctBlockList* getVariant(int indx) const;
	const CRctBlockList* findVariant(const CRctBlockList* target) const;

    private:
	CRsllist variantList;
	int numVariants;
	const char* cmdStr;
};

typedef CRcmdEntry* CRcmdEntryPtr;

class CRcmdTbl {
      public:
	CRcmdTbl();
	~CRcmdTbl();
	void insert(const String& cmdStr, CRcmdEntryPtr parmEntry);
	CRcmdEntryPtr getEntry(const char* cmdStr);
	void printHelp();
	void dump();
	void printManPages() const;
	//void delete();
		
      private:
	Map<String,CRcmdEntryPtr> map;
};



typedef CRctParm* CRparmPtr;

class CRparmTbl {
      public:
	CRparmTbl();
	~CRparmTbl();
	void insert(const String& parmStr, CRparmPtr parmEntry);
	CRparmPtr getEntry(const char* parmStr);
	void dump();
		
      private:
	Map<String,CRparmPtr> map;
};


inline CRctBlockList*
CRcmdEntry::getVariant(int indx) const
{
	return (CRctBlockList*) variantList.nth_const(indx);
}
