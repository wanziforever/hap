#ifndef _CROMDB_H
#define _CROMDB_H
//
// DESCRIPTION:
//      This file contains the declaration of the CRomdb class,
//      as well as other classes needed for the OMDB.
// OWNER:
//	Roger McKee
//	Yash Pal Gupta
//
// NOTES:
//
//

#include "hdr/GLtypes.h"
#include "cc/hdr/cr/CRalarmLevel.H"
#include "cc/hdr/db/DBintfaces.H"
#ifdef CC
#include "cc/cr/omm/CRomBuffers.H"
#include "cc/hdr/cr/CRlocalLogMsg.H"
#endif
#include "cc/hdr/init/INinit.h"
#include <String.h>

class CRsopRegMsg;
class CRspoolMsg;
class CRrcvOmdbMsg;
class CRomdbEntry;
class CRx733omdbEntry;
class CRx733reptEntry;
class CRdestEntry;
class CRomClassEntry;
class CRomdbTbl;
class CRx733omdbTbl;
class CRx733reptTbl;
class CRalarmCodeTbl;
class CRomClassTbl;
class CRdestTbl;

const Short CRauditTime = 30;		// audit period = 30 seconds

// Data structure required to support the MSGH's internal part.
class CRomdb {
  public:
	CRomdb();
//	CRomdb(const CRomdb&);	memberwise initialization is ok
//	inline ~CRomdb();		nothing to destruct
//	CRomdb& operator=(const CRomdb&); memberwise assignment is ok

	GLretVal init();
	short sysinit();	/* called during system initialization */
	short procinit();	
	short procinit(SN_LVL init_lvl);	
	short cleanup();
	const CRomdbEntry* getEntry(const char* name);
	CRomdbEntry* getEntry(const char* name, const char* senderName,
			      const char*& formatstr);
	GLretVal deleteEntry(const char*);
	GLretVal x733deleteEntry(const char*);
	GLretVal insertEntry(const char*, CRomdbEntry *); // register a name
	GLretVal x733insertEntry(const char*, CRx733omdbEntry *); // register a name
	GLretVal replaceEntry(const char*, CRomdbEntry *); // register a name
	GLretVal x733replaceEntry(const char*, CRx733omdbEntry *); // register a name
	void audit();			// audit the OMDB
	void timeChange();
	void setDirectory();
	void dump();		// print the OMDB
	void regMsg(CRsopRegMsg*);
	void route(CRspoolMsg*);
	void route(CRrcvOmdbMsg&);
#ifdef CC
	void route(CRomBuffer*);
	void procLocalLogs();
	void processLogFile(char *filename, char *dir);

#endif
	GLretVal modifyDest(DBoperation, const char* dest,
			    const char* logicalDev,
			    const char* channelType, int log_hsize,
			    int numOfLogs, const char* daily, int logAge);
	CRdestEntry* getDestEntry(const char* destName);
	GLretVal modifyMsgCls(DBoperation, const char* msgclass,
			      const char* destlist[], char*& failreason);
	CRomClassEntry* getClassEntry(const char* msgclass);
	GLretVal modifyOM(DBoperation, const char* msgname,
			  const char* msgclass,
			  const char* format, CRALARMLVL alarmlvl);

	GLretVal modifyX733OM(DBoperation, const char* msgname,
			const char * pcause ,
			const char * alarmType ,
			const char * objName ,
			const char * specificProblem ,
			const char * addText);

	GLretVal modifyAlarmCode(DBoperation, const char* alrmSrc,
			char char_1, Char *&retres);

	GLretVal upDateX733Rept(const char* report);
	CRx733reptTbl *x733reptTblPtr;	// point to X733 REPORT table
	Void storeSubSystemList();
	Void storeAdditionalNumbers();

    private:
	GLretVal initDB();
	void generateISmsg();
	void loadFail(const char* tableName);
	void limpRoute(const char* msgclass, const char* text, int textlen,
		       CRALARMLVL alm);
	void limpRoute(CRspoolMsg*);
	void limpRoute(CRrcvOmdbMsg&);
#ifdef CC
	void limpRoute();
	void limpRoute(CRomBuffer*);
#endif
	Bool isValid() const;
	void setDefaults();
	const char* getClassStr(CRomdbEntry* omdbEntry,
				const char* userMsgClass);
	CRomClassEntry* getClassEntry(CRomdbEntry* omdbEntry,
				      const char* userMsgClass);
	void delClassEntry(const char* msgclass);
	CRomClassEntry* insertMsgClass(const char* msgclass);
	CRALARMLVL getAlarmLvl(CRALARMLVL userAlarmLvl, CRomdbEntry* entry);
	const char* getAlarmStr(CRALARMLVL userAlarmLvl, CRomdbEntry* entry);
	GLretVal insertDest(const char* destName, const char* logicalDev,
			    const char* channelType, int logsz,
			    int numOfLogs, const char* daily, int logAge);
	GLretVal updateDest(const char* destName, const char* logicalDev,
			    const char* channelType, int logsz,
			    int numOfLogs, const char* daily, int logAge);
	GLretVal deleteDest(const char* destName);
	//Void createOmdbMsgAlarmCode(String theOMkey, String & theAddText, const char* OMtext );
	Void createOmdbMsgAlarmCode(String theOMkey, const char* OMtext );
	Void createSpoolMsgAlarmCode(String theOMkey, const char* theProcess,
	                             String& alarmCode, Bool isFromINIT);
	String subsystemCode(const char* subsystem) const;
	String addedValue(const char* omkey, const char* omtext, const char* number) const;

    private:
	Bool validFlag;
	CRomdbTbl *omdbTblPtr;	// point to OMDB table
	CRx733omdbTbl *x733omdbTblPtr;	// point to X733 OMDB table
	CRalarmCodeTbl *alarmCodeTblPtr;// point to CR_ALARMCODE table
	CRomClassTbl *classTblPtr;   // point to OM class table
	CRdestTbl *destTblPtr;     // point to destination table
	String CRalarmKey;
	char theSubsystem[100][5];
	char theCode[100][3];

	// these get file from the file CRuniqueAlarmIds
	char theOMdbKey[100][7];
	char theOMtext[100][100];
	int theAddedValue[100];
};

extern CRomdb omdb;
#ifdef CC
extern char* CRgetremote();
#endif
#endif
