/*
**      File ID:        @(#): <MID18638 () - 09/12/91, 1.1.1.2>
**
**
**	File:					MID18638
**	Release:				1.1.1.2
**	Date:					04/05/00
**	Time:					16:06:16
**	Newest applied delta:	09/12/91
**
** DESCRIPTION:
**      This file defines a USLI class that is used to control
**      the flow of OM's.
**
** OWNER: 
**	USLI subsystem
**
**
**
** NOTES:
**
*/

#include	<sys/times.h>
#include        <sys/param.h>

#include	<string.h>

#include "cc/hdr/cr/CRomBrevityCtl.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"
//#include "cc/hdr/cr/CRomdbMsg.hh"
//#include "cc/cr/hdr/CRgdoMemory.hh"
#include "cc/hdr/init/INusrinit.hh"

// Initialize the static variables
CRomBrevityCtl* CRomBrevityCtl:: _instance = 0; // this is used so we don't
// create more then 1 instance

// this is done to avoid system 
// call for CLK_TCK, got this from MSGH
#ifdef __sun
Long    CRCLK_TCK = CLK_TCK;
#else
// pre linux Long        CRCLK_TCK = CLOCKS_PER_SEC;
Long    CRCLK_TCK = sysconf(_SC_CLK_TCK);
#endif

MHgd                CRgdo;

Char * CRomBrevityCtl::_brevGDOaddr = NULL;

int CRminorAlarmCnt = 0;
int CRassertCnt = 0;
int CRcrcftassertCnt = 0;

// NAME
//  CRomBrevityCtl
//
// PURPOSE
//   The constructor for the CRomBrevityCtl class.
//
// INCLUDES 
//   cc/hdr/cr/CRomBrevityCtl.H
//
// DESCRIPTION
//   This will default the OM array to null and all the counters to 0.
//

CRomBrevityCtl::CRomBrevityCtl()
{
  FILE *fp;
  int deflag=0;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "CRomBrevityCtl\n");
    fclose(fp);
  }
	if ( _instance == 0)


     // Build and NULL out main struct
     for(int i = 0;i < CRMAXLISTSIZE;++i)
     {
       _OMlist[i].omKey[0]         = '\0';
       _OMlist[i].omCnt            = 0;
       _OMlist[i].totalomCnt       = 0;
       _OMlist[i].totalTimePeriods = 0;
       _OMlist[i].nextTimePeriod   = 0;
       _OMlist[i].status           = CRCLEAR;
     }

	_omArrayCnt = 0;
	_printTimer = 0;

}


// NAME
//  ~CRomBrevityCtl
//
// PURPOSE
//   The destructor for the CRomBrevityCtl class.
//
// PARAMETERS 
//   NONE
//
// INCLUDES 
//   cc/hdr/cr/CRomBrevityCtl.H
//   cc/hdr/cr/CRdebugMsg.H
//
// DESCRIPTION
//   This will dump out the main OM array when DEBUG_FLAG is set.
//

CRomBrevityCtl::~CRomBrevityCtl()
{

	if (CRDEBUG_FLAGSET(CRusli))
	{
		dump();
	}
}


// NAME
//  CRomStatus status
//
// PURPOSE
//   To return the status of the OM.
//
// PARAMETERS 
//   The OM key, ie: /CR001 <- for OMDB OM
//
// INCLUDES 
//   cc/hdr/cr/CRomBrevityCtl.H
//
// DESCRIPTION
//   This function calls other functions to add/modifily entries 
//   in the OMlist array. 
//
//	NOTE : am omkey consists of :
//            1/BL001 where the first element is the type
//            of OM (CROMDBMSG, CRERROR, CRMSG, CRASSERT, CRCFTASSERT)
//            the rest is the key except for CRERRORs.
//           for CRERROR its 1BLopClk.C!100 which means
//	     the 1 is for CRERROR the next uptoill the ! is the file name
//           the CERROR came form then the line number.
//
// DETAILS 
//   NONE
//
// RETURNS 
//   CRomStatus an enum of CRCLEAR, CRBLOCK, CRPENDING
//

CRomStatus
CRomBrevityCtl::status(const char* msgKey, CRALARMLVL alarmLevel)
{

  int flag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    flag=1;
  }

  if(flag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "++++++++++++ S T A T U S +++++++++++++++++++++++++++++++\n");
    fprintf(fp, "Checking Process = %s  msgKey = %s alarmLevel = %d\n",
            CRprocname, (const char*) msgKey, alarmLevel);
    fclose(fp);
  }

	static int initValuesFlag = 0;
	static Char* addr = NULL;

	Bool minorAlarmFlag = FALSE;
	Bool assertFlag = FALSE;
	Bool crcftassertFlag = FALSE;

	if(initValuesFlag == 0)
	{
// Get brevity control values from INIT
// get 3 values. If INIT gives me 0 then brevity control is off
		IN_BREV_PARMS(&_low,&_high,&_period);


		if((_period != 0 ) && (_low >= _high))
		{
			// I can't use CRCFTASSERT MARCO
			// so I'm building my own CRCFTASSERT OM
			// I using CRbadBrevityParmsFMT all really 
			// in CRassertIDs
			//CRCFTASSERT(CRBadBrevityParmsId,
      //(CRBadBrevityParmsFMT, _low,_high,CRprocname));
      //
      // +++ PSPQ148 2000-07-17 15:44:22 ASRT        
      // A  REPT MANUAL ASSERT=9503
      //    PROC=CEP1746, ../BLopClk.C AT LINE 143
      //    THIS IS CRAFT ASSERT 0
      //END OF REPORT #000660++-

			CRmsg om;
			om.setMsgClass("ASRT");
			om.setAlarmLevel(POA_ACT);
			om.add("REPT MANUAL ASSERT=3041\n");
			om.add("PROC=%s, CRomBrevityCtl.C AT LINE %d\n",CRprocname, __LINE__);
			om.add("brevity_low(%d) is greater then or equal to brevity_high(%d)",_low,_high);
			om.spool();
			initValuesFlag=1;
			_period=0; // turn off brevity control
		}
		else if((_period != 0 ) && (_low < 1 || _low > 254 ))
		{
			CRmsg om;
			om.setMsgClass("ASRT");
			om.setAlarmLevel(POA_ACT);
			om.add("REPT MANUAL ASSERT=3041\n");
			om.add("PROC=%s, CRomBrevityCtl.C AT LINE %d\n",CRprocname, __LINE__);
			om.add("brevity_low(%d) can only be 1 - 254",_low);
			om.spool();
			initValuesFlag=1;
			_period=0; // turn off brevity control
		}
		else if((_period != 0 ) && (_high < 2 || _high > 255 ))
		{
			CRmsg om;
			om.setMsgClass("ASRT");
			om.setAlarmLevel(POA_ACT);
			om.add("REPT MANUAL ASSERT=3041\n");
			om.add("PROC=%s, CRomBrevityCtl.C AT LINE %d\n",CRprocname, __LINE__);
			om.add("brevity_high(%d) can only be 2 - 255",_high);
			om.spool();
			initValuesFlag=1;
			_period=0; // turn off brevity control
		}
		else if((_period != 0 ) && (_period > 60))
		{
			CRmsg om;
			om.setMsgClass("ASRT");
			om.setAlarmLevel(POA_ACT);
			om.add("REPT MANUAL ASSERT=3041\n");
			om.add("PROC=%s, CRomBrevityCtl.C AT LINE %d\n",CRprocname, __LINE__);
			om.add("brevity_peroid(%d) can only be 0, 1 - 60",_period);
			om.spool();
			initValuesFlag=1;
			_period=0; // turn off brevity control
		}
		else
		{
			++initValuesFlag;
		}
	}

  CRDEBUG(CRusli, 
          ("Checking brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",
           CRprocname, (const char*) msgKey, _period, _high, _low));


	GLretVal retval;

  char keyArray[CROMKEYSZ]; 
	strncpy(keyArray,msgKey,CROMKEYSZ);
	keyArray[strlen(msgKey)] = '\0';

  if(flag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "Checking Process = %s  msgKey = %s alarmLevel = %d keyArray = %s\n",
            CRprocname, (const char*) msgKey, alarmLevel, keyArray);
    fclose(fp);
  }

	if( _brevGDOaddr != NULL )
	{
		CRshData = (CRbrevSharedMemory *) _brevGDOaddr;

		if( CRshData->noOfRecords != 0 ) /*  non-empty GDO */
		{
			int noOfRecords = CRshData->noOfRecords;
			int i = 0;

      /* CRERROR 2file!493 */ 
      /* CROMDB 1/CR012    */ 
      /* CRASSERT 43007    */ 
      /* ALL_OM            */ 
			/* added 3 more sev keys */
			/* MINOR             */
			/* ASSERT            */
			/* CFTASSERT         */
			/* ALL               */
      char *key = &keyArray[1];

      if(flag){
        fp=fopen("/tmp/brev.out","a");
        fprintf(fp, "IN SHARE MEMORY VALUES for %s %s\n",
                CRprocname,(const char*) msgKey);
        fclose(fp);
      }

			if( CRshData->whichRecordSet == 0 )
			{
        if(flag){
          fp=fopen("/tmp/brev.out","a");
          fprintf(fp, "record set 0\n");
          fclose(fp);
        }
        for(i=0;i<noOfRecords;++i)
        {
          if( (strcmp(CRprocname, 
                      CRshData->records0[i].process)) == 0 )
          {
            if(flag){
              fp=fopen("/tmp/brev.out","a");
              fprintf(fp, "MATCH process\n");
              fclose(fp);
            }
            /* this is for sevity */
            if(((strcmp("MINOR", 
                        CRshData->records0[i].omkey )) == 0 ) ||
               ((strcmp("ALL", 
                        CRshData->records0[i].omkey )) == 0 ))
            {
              if(flag){
                fp=fopen("/tmp/brev.out","a");
                fprintf(fp, "MATCH MINOR alarm\n");
                fclose(fp);
              }
				      if(alarmLevel == POA_MIN)
				      {
                if(flag){
                  fp=fopen("/tmp/brev.out","a");
                  fprintf(fp, "alarm lvl = minor\n");
                  fclose(fp);
                }
                // set values
				        if((strcmp("ALL", 
                           CRshData->records0[i].omkey )) == 0 )
                {
                  minorAlarmFlag=TRUE;
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,"MINOR",CROMKEYSZ);
                  keyArray[strlen("MINOR")] = '\0';
                }
                initValuesFlag=0;
                _period = CRshData->records0[i].period;
                _high = CRshData->records0[i].upper;
                _low = CRshData->records0[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
				      }
            }// end of MINOR or ALL case

            // 3 at beginning of omkey means CRASSERT OM
            if(((strcmp("CRASSERT", 
                        CRshData->records0[i].omkey )) == 0 )||
				       ((strcmp("ALL", 
                        CRshData->records0[i].omkey )) == 0 ))
            {
              if((alarmLevel == POA_ACT) && ((strncmp(msgKey,"3",1)==0)))
              {
				        if((strcmp("ALL", 
                           CRshData->records0[i].omkey )) == 0 )
                {
                  assertFlag=TRUE;
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,"CRASSERT",CROMKEYSZ);
                  keyArray[strlen("CRASSERT")] = '\0';
                }
                // set values
                initValuesFlag=0;
                _period = CRshData->records0[i].period;
                _high = CRshData->records0[i].upper;
                _low = CRshData->records0[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
              }
            }// end of CRASSERT or ALL case

            // 4 at beginning of omkey means CRCFTASSERT OM
            if(((strcmp("CRCFTASSERT", 
                        CRshData->records0[i].omkey )) == 0 )||
				       ((strcmp("ALL", 
                        CRshData->records0[i].omkey )) == 0 ))
            {
              if((alarmLevel == POA_ACT) && ((strncmp(msgKey,"4",1)==0)))
              {
				        if((strcmp("ALL", 
                           CRshData->records0[i].omkey )) == 0 )
                {
                  crcftassertFlag=TRUE;
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,
                          "CRCFTASSERT",CROMKEYSZ);
                  keyArray[strlen("CRCFTASSERT")] = '\0';
                }
                // set values
                initValuesFlag=0;
                _period = CRshData->records0[i].period;
                _high = CRshData->records0[i].upper;
                _low = CRshData->records0[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
              }
            }// end of CRCFTASSERT or ALL case

            ///////// End Set brevity control for alm value

            if( (strcmp(key, CRshData->records0[i].omkey )) == 0 )
            {
              // set values
              initValuesFlag=0;
              _period = CRshData->records0[i].period;
              _high = CRshData->records0[i].upper;
              _low = CRshData->records0[i].lower;
              CRDEBUG(CRusli, ("MATCH 1 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
              break;
            }
            if( (strcmp("ALL_OMs", CRshData->records0[i].omkey )) == 0 )
            {
              // set values
              initValuesFlag=0;
              _period = CRshData->records0[i].period;
              _high = CRshData->records0[i].upper;
              _low = CRshData->records0[i].lower;
              CRDEBUG(CRusli, ("MATCH 2 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
              continue; // see if a match with omkey
            }
          }// match process name
        }//for loop
			}//record set
			else
			{
        if(flag){
          fp=fopen("/tmp/brev.out","a");
          fprintf(fp, "second set of records\n");
          fclose(fp);
        }
        for(i=0;i<noOfRecords;++i)
        {
          if( (strcmp(CRprocname, CRshData->records1[i].process)) == 0 )
          {
            if(flag){
              fp=fopen("/tmp/brev.out","a");
              fprintf(fp, "PROC match\n");
              fclose(fp);
            }

            /* this is for sevity */
            if(((strcmp("MINOR", 
                        CRshData->records1[i].omkey )) == 0 )||
				       ((strcmp("ALL", 
                        CRshData->records1[i].omkey )) == 0 ))
            {
              if(flag){
                fp=fopen("/tmp/brev.out","a");
                fprintf(fp, "MINOR alarm\n");
                fclose(fp);
              }
				      if(alarmLevel == POA_MIN)
				      {
                if(flag){
                  fp=fopen("/tmp/brev.out","a");
                  fprintf(fp, "alarm lvl match MINOR\n");
                  fclose(fp);
                }
                // set values
				        if((strcmp("ALL", 
                           CRshData->records1[i].omkey )) == 0 )
                {
                  minorAlarmFlag=TRUE;
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,"MINOR",CROMKEYSZ);
                  keyArray[strlen("MINOR")] = '\0';
                }
                initValuesFlag=0;
                _period = CRshData->records1[i].period;
                _high = CRshData->records1[i].upper;
                _low = CRshData->records1[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
				      }
            }// end of MINOR or ALL case

            // 3 at beginning of omkey means CRASSERT OM
            if(((strcmp("CRASSERT", 
                        CRshData->records1[i].omkey )) == 0 )||
				       ((strcmp("ALL", 
                        CRshData->records1[i].omkey )) == 0 ))
            {
              if((alarmLevel == POA_ACT) && ((strncmp(msgKey,"3",1)==0)))
              {
                // set values
                assertFlag=TRUE;
				        if((strcmp("ALL", 
                           CRshData->records1[i].omkey )) == 0 )
                {
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,"CRASSERT",CROMKEYSZ);
                  keyArray[strlen("CRASSERT")] = '\0';
                }
                initValuesFlag=0;
                _period = CRshData->records1[i].period;
                _high = CRshData->records1[i].upper;
                _low = CRshData->records1[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
              }
            }// end of CRASSERRT or ALL case

            // 4 at beginning of omkey means CRCFTASSERT OM
            if(((strcmp("CRCFTASSERT", 
                        CRshData->records1[i].omkey )) == 0 )||
				       ((strcmp("ALL", 
                        CRshData->records1[i].omkey )) == 0 ))
            {
              if((alarmLevel == POA_ACT) && ((strncmp(msgKey,"4",1)==0)))
              {
                // set values
				        if((strcmp("ALL", 
                           CRshData->records1[i].omkey )) == 0 )
                {
                  crcftassertFlag=TRUE;
                  strncpy(keyArray,"ALL",CROMKEYSZ);
                  keyArray[strlen("ALL")] = '\0';
                }
                else
                {
                  strncpy(keyArray,
                          "CRCFTASSERT",CROMKEYSZ);
                  keyArray[strlen("CRCFTASSERT")] = '\0';
                }
                initValuesFlag=0;
                _period = CRshData->records1[i].period;
                _high = CRshData->records1[i].upper;
                _low = CRshData->records1[i].lower;
                CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
                break;
              }
            }// end of CRCFTASSERT or ALL case

            /////////End  Set brevity control for alm value

            if((strcmp(key, CRshData->records1[i].omkey )) == 0 )
            {
              // set values
              initValuesFlag=0;
              _period = CRshData->records1[i].period;
              _high = CRshData->records1[i].upper;
              _low = CRshData->records1[i].lower;
              CRDEBUG(CRusli, ("MATCH 3 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
              break;
            }
            if((strcmp("ALL_OMs", CRshData->records1[i].omkey )) == 0 )
            {
              // set values
              initValuesFlag=0;
              _period = CRshData->records1[i].period;
              _high = CRshData->records1[i].upper;
              _low = CRshData->records1[i].lower;
              CRDEBUG(CRusli, ("MATCH 4 NEW VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname,(const char*) msgKey,  _period, _high, _low));
              continue; // see if match on key
            }
          }// if match process
        }// for loop
			}//else
		}// end CRshData.noOfRecords > 0
	}// end of CRshData != NULL

  if(flag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d\n",CRprocname, keyArray,  _period, _high, _low);
    fclose(fp);
  }

  CRDEBUG(CRusli, ("VALUES brev for %s %s values are :\nperiod = %d\nhigh = %d\nlow = %d",CRprocname, keyArray,  _period, _high, _low));

	if( _period == 0 ) // don't do brevity control
	{
    CRDEBUG(CRusli, ("BREV TURN OFF for %s %s", CRprocname, (const char*) msgKey));
		return CRCLEAR;
	}

	if((strcmp(msgKey,"1/CR065")) == 0)  // don't block my block message
     return CRCLEAR;

	int arrayCnt  = 0;
	Long thisTime = getOMtime(); // call getOMtime once per status call

	// Must be FIRST OM so thus add it to the array
	if(_omArrayCnt == 0)
	{
		addNewOM(keyArray);

		_printTimer = thisTime + CRTIMEDELAY; // five minutes
		return CRCLEAR;
	}
	else
	{

		// Check to see if its time do do some cleanup
		// and print any block OM
		if(_printTimer < thisTime)
		{
			printDelayOM(thisTime);
		}

		// go through the array until a match if no match
		// then assume its a new OM and added it in
		for(arrayCnt = 0;arrayCnt < _omArrayCnt;++arrayCnt)
		{
			// Check to see if the OM is already in the ARRAY
			if((strcmp(_OMlist[arrayCnt].omKey,keyArray)) == 0) 
			{
        // Check to see key = ALL for all sevitys
        // Check to see if in BLOCKED or PENDING
        // next add to count for MINOR/ASSERT/CRCFTASSERT
        if((strcmp("ALL", keyArray)) == 0 )
        {
          if((_OMlist[arrayCnt].status == CRBLOCK) ||
	   			   (_OMlist[arrayCnt].status == CRPENDING))
          {
            if(minorAlarmFlag)
            {
              ++CRminorAlarmCnt;
            }
            if(assertFlag)
            {
              ++CRassertCnt;
            }
            if(crcftassertFlag)
            {
              ++CRcrcftassertCnt;
            }
          }
        }

        // Check to see if its a new time peroid
        if(thisTime > _OMlist[arrayCnt].nextTimePeriod)
        {
          //
          // new time period
          //
          // have to check to see what to do
          ++_OMlist[arrayCnt].totalTimePeriods;
          return updateStatusForNewPeriod(arrayCnt, 
                                          thisTime);
        }// end of if
        else
        {
          return updateStatus(arrayCnt);
        }// end else
      }// end of if
		}//end of for loop

		// new OM so add it to array

		addNewOM(keyArray);
		return CRCLEAR;
	}
	return _OMlist[arrayCnt].status; // equal last omArrayCnt
}


// NAME
//  Void dump
//
// PURPOSE
//   This function will dump out the contains of the array
//
// PARAMETERS 
//   NONE
//
// INCLUDES 
//   cc/hdr/cr/CRomBrevityCtl.H
//
// DESCRIPTION
//   This will dump out the main OM array.
//

Void
CRomBrevityCtl::dump()
{
	int i;
  FILE *fp;

  fp=fopen("/tmp/brev.out","a");
  fprintf(fp,"================ D U M P ================================\n");
	for(i = 0;i < _omArrayCnt;++i)
	{
		if(_OMlist[i].omKey[0] == 0)
		{
			break;
		}
    CRDEBUG_PRINT(0,("omKey       = %s\nomCnt       = %d\ntotalomCnt	= %d\nstatus      = %d\n_omArrayCnt  = %d\nnextTimePeriod = %ld\ni           = %d\n",
                     (const char*)_OMlist[i].omKey, _OMlist[i].omCnt, _OMlist[i].totalomCnt,
                     _OMlist[i].status,_omArrayCnt, _OMlist[i].nextTimePeriod, i));
    fprintf(fp,"omKey       = %s\nomCnt       = %d\ntotalomCnt	= %d\nstatus      = %d\n_omArrayCnt  = %d\nnextTimePeriod = %ld\ni           = %d\n",
            (const char*)_OMlist[i].omKey, _OMlist[i].omCnt, _OMlist[i].totalomCnt,
            _OMlist[i].status,_omArrayCnt, _OMlist[i].nextTimePeriod, i);

	}// end of for
  fprintf(fp,"================ E N D  D U M P =========================\n");
  fclose(fp);

}


// NAME
//  getOMtime()
//
// PURPOSE
//   get the seconds from the system.
//
// PARAMETERS 
//   NONE
//
// INCLUDES 
//   cc/hdr/cr/CRomBrevityCtl.H
//
// DESCRIPTION
//   This will used times to get the seconds and return a long.
//
// DETAILS 
//   NONE
//
// RETURNS 
//   Long
//

Long
CRomBrevityCtl::getOMtime()
{

	struct tms	dummyTimeVarible;

	Long OMtimekeep = times(&dummyTimeVarible);

	// to get seconds
	Long OMseconds = (unsigned) OMtimekeep/CRCLK_TCK;

	return  OMseconds;
}


// NAME
//  Void updateStatusForNewPeriod(int i, Long thisTimePeriod)
//
// PURPOSE
//   This function updates the status, and counts in the OM array
//
// PARAMETERS 
//      int i - the  index into the array for the output message that we are
//              updating
//	Long thisTimePeriod - this time period
//
// INCLUDES 
//   none
//
// DESCRIPTION
//   This only happens when there is a new time period.
//
// RETURNS 
//   CRomStatus
//

CRomStatus
CRomBrevityCtl::updateStatusForNewPeriod(int omElement, Long thisTimePeriod)
{

  int flag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    flag=1;
  }

  if(flag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In updateStatusForNewPeriod for %d\n",omElement);
    fclose(fp);
  }

	switch(_OMlist[omElement].status)
	{
  case CRBLOCK:
     {
       // There's never been a message after the status
       // changed from CLEAR to BLOCK and its been
       // past more then 1 time period
       if ((_OMlist[omElement].nextTimePeriod + _period)< thisTimePeriod)
       {
         printBlockOM(omElement);
         resetOM(omElement);
         //print clear here by setting totalomCnt to -1
         _OMlist[omElement].totalomCnt = -1;
         printBlockOM(omElement);
       }
       else
       {
         _OMlist[omElement].nextTimePeriod = thisTimePeriod + _period;
         _OMlist[omElement].omCnt = 1;
         ++_OMlist[omElement].totalomCnt;
         _OMlist[omElement].status = CRPENDING;
       }
       return _OMlist[omElement].status;
     }//end case BLOCK

  case CRPENDING:
     {
       if(_OMlist[omElement].omCnt <= _low) // can reset to CLEAR
       {
         printBlockOM(omElement);
         resetOM(omElement);
         //print clear here by setting totalomCnt to -1
         _OMlist[omElement].totalomCnt = -1;
         printBlockOM(omElement);
       }
       else
       {
         _OMlist[omElement].nextTimePeriod = thisTimePeriod + _period;
         _OMlist[omElement].omCnt = 1;
         ++_OMlist[omElement].totalomCnt;
         if(_OMlist[omElement].totalTimePeriods >= CRMAXTIMEPERIOD)
         {
           printBlockOM(omElement);
         }
       }
       return _OMlist[omElement].status;
     }//end case PENDING

  case CRCLEAR:
     {
       resetOM(omElement);
       return CRCLEAR; // let the OM print
     }//end case CLEAR

  default:
     {
       return CRCLEAR; // let the OM print
     }
	}// end of switch

	return CRCLEAR; // SHOULD NEVER REACH THIS
} // end of updateStatusForNewPeriod


// NAME
//  Void updateStatus(int i)
//
// PURPOSE
//   This function updates the status, and counts in the OM array
//
// PARAMETERS 
//      int i - the  index into the array for the output message that we are
//              updating
//
// INCLUDES 
//   none
//
// DESCRIPTION
//   This only happens between time periods.
//
// RETURNS 
//   CRomStatus
//
// SEE ALSO 
//  none
//
// FILES 
//   none
//
// CAVEATS
//   none
//
// EXAMPLES 
//    blank
//

CRomStatus
CRomBrevityCtl::updateStatus(int i)
{
  int flag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    flag=1;
  }

  if(flag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In updateStatus for %d\n",i);
    fclose(fp);
  }

	++_OMlist[i].omCnt;

	if((_OMlist[i].omCnt > _high) && (_OMlist[i].status == CRCLEAR))
	{
		_OMlist[i].status = CRBLOCK;
		printBlockOM(i); 
	}

	if((_OMlist[i].status == CRBLOCK) ||
	   (_OMlist[i].status == CRPENDING))
	{
		//HERE
		++_OMlist[i].totalomCnt;
	}

	return _OMlist[i].status;

}// end of updateStatus


// NAME
//  Void printBlockOM(int i)
//
// PURPOSE
//   This function prints an OM explaining  that its being BLOCK
//
// PARAMETERS 
//      int i - the  index into the array for the output message that we are
//              now BLOCKING
//
// INCLUDES 
//    cc/hdr/cr/CRomdbMsg.H
//
// DESCRIPTION
//   This only happens when the status changes from BLOCK/PENDING to CLEAR and
//   on the NUMOFTIMEPERIODS number of time periods. 
//
// RETURNS 
//   Void
//

Void
CRomBrevityCtl::printBlockOM(int i)
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In printBlockOM for %d\n",i);
    fclose(fp);
  }
	int flag=0;
	//0 is BLOCK/CLEAR
	//1 is START

	char str[200];

	memset((char*) &str, 0, sizeof(str));

	if( ( _OMlist[i].totalomCnt == 0 ) && \
	    (_OMlist[i].status == CRBLOCK) )
	{
		flag=1;
	}
	else
	{
		if( _OMlist[i].totalomCnt == 0 )
       return; // why print a message saying nothing was block
	}

	CROMDBKEY(CRlogBlockOM, "/CR065");

  CRomdbMsg *om = new CRomdbMsg;

	if( _OMlist[i].totalomCnt == -1 )
	{
    _OMlist[i].totalomCnt = 0;
		om->setAlarmLevel(POA_INF);
    om->add("CLEARED");
	}
	else
	{
		//do it from RCV
		//om->setAlarmLevel(POA_TMN);
		if( flag == 1 )
		{
      om->add("STARTING TO BLOCK");
		}
		else
		{
      om->add("BLOCKED");
		}
	}

  om->add(CRprocname);
  om->add(_OMlist[i].totalomCnt);

  char keyArray[CROMKEYSZ]; 
	strncpy(keyArray,_OMlist[i].omKey,CROMKEYSZ);
	om->setAlarmObjectName(keyArray);
  int omtype = 0;
  char charOmtype[CROMKEYSZ];
  strncpy(charOmtype,keyArray,1);
  charOmtype[1]='\0';

	if(strncmp(keyArray,"CRASSERT",8)==0)
	{
    omtype = 10;
	}
	else if(strncmp(keyArray,"CRCFTASSERT",11)==0)
	{
    omtype = 11;
	}
	else if(strncmp(keyArray,"MINOR",5)==0)
	{
    omtype = 12;
	}
	else if(strncmp(keyArray,"ALL",3)==0)
	{
    omtype = 13;
	}
	else
	{
    omtype = atoi(charOmtype);

	}
  char *key = &keyArray[1];

	char    theSpecificProblem[20 + CROMKEYSZ];

  switch(omtype)
  {
  case CROMCRMSG:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"CRMSG OM BEING CLEARED; MSG KEY IS : "); 
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"CRMSG OM BEING BLOCKED; MSG KEY IS : "); 
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }

    strncat(str,key,CROMKEYSZ-1); 
    break;

  case CROMCROMDB:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"CROMDB OM BEING CLEARED; OMDB KEY : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"CROMDB OM BEING BLOCKED; OMDB KEY : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    strncat(str,key,CROMKEYSZ-1); 
    break;

  case CROMCRERROR:
    const char *file;
    const char *line;
    file = strtok( key, "!" ) ;
    line = strtok( NULL, "!" ) ;

    if(file == 0)
       file="UNKNOWN";
    if(line == 0)
       line="UNKNOWN";

    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"CRERROR BEING CLEARED; FILE : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"CRERROR BEING BLOCKED; FILE : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }

    strcat(str,file);
    strcat(str," LINE NUMBER : ");
    strcat(str,line);
    break;

  case CROMCRASSERT:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"CRASSERT OM BEING CLEARED; ASSERT IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"CRASSERT OM BEING BLOCKED; ASSERT IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }

    strncat(str,key,CROMKEYSZ-1); 
    break;

  case CROMCRCFTASSERT:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"CRCFTASSERT OM BEING CLEARED; CRAFT ASSERT IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"CRCFTASSERT OM BEING BLOCKED; CRAFT ASSERT IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }

    strncat(str,key,CROMKEYSZ-1); 
    break;

  case 10:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"SEVERITY LEVEL: ASSERT OMs BEING CLEARED");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED ASSERT");
    }
    else
    {
      strcpy(str,"SEVERITY LEVEL: ASSERT");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED ASSERT");
    }

    break;

  case 11:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"SEVERITY LEVEL: CRAFT ASSERT OMs BEING CLEARED");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED CASSERT");
    }
    else
    {
      strcpy(str,"SEVERITY LEVEL: CRAFT ASSERT");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED CASSERT");
    }

    break;

  case 12:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"SEVERITY LEVEL: MINOR ALARMED OMs BEING CLEARED");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED MINOR");
    }
    else
    {
      strcpy(str,"SEVERITY LEVEL: MINOR");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED MINOR");
    }
    break;

  case 13:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"SEVERITY LEVEL: ALL ASSERT, CRAFT ASSERT, and MINOR ALARMED OMs BEING CLEARED");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED ALL ALRM");
    }
    else
    {
      if(_OMlist[i].totalomCnt == 0) 
      {
        sprintf(str,"SEVERITY LEVEL: ASSERT(0), CRAFT ASSERT(0), MINOR(0)");
      }
      else
      {
        sprintf(str,"SEVERITY LEVEL: ASSERT(%d), CRAFT ASSERT(%d), MINOR(%d)",CRassertCnt,CRcrcftassertCnt,CRminorAlarmCnt);
      }
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED ALL ALRM");
    }
    break;

  default:
    if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
    {
      strcpy(str,"OM BEING CLEARED; KEY IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    else
    {
      strcpy(str,"OM BEING BLOCKED; KEY IS  : ");
      snprintf(theSpecificProblem,20 + CROMKEYSZ,"OM BLOCKED %s",key);
    }
    strncat(str,key,CROMKEYSZ-1); 
    break;
  }
  om->add(str);


	om->setSpecificProblem(theSpecificProblem);

	if( (_OMlist[i].totalomCnt == 0) && ( flag == 0) )
	{
		//do not send clear
		CRDEBUG(CRusli, ("%s", str));
	}
	else
	{
    om->spool(CRlogBlockOM);
	}

	delete om;

	_OMlist[i].totalTimePeriods = 0;
	_OMlist[i].totalomCnt       = 0;
	CRminorAlarmCnt = 0;
	CRassertCnt = 0;
	CRcrcftassertCnt = 0;

}// end of printBlockOM


// NAME
//  Void addNewOM(const char* msgKey)
//
// PURPOSE
//   This function adds an OM the the master OM list
//
// PARAMETERS 
//      const char* msgKey - the  new OM key
//      int arrayCnt - the arrayCnt;
//
// INCLUDES 
//    cc/hdr/cr/CRomdbMsg.H
//
// DESCRIPTION
//   This only happens when the status changes from BLOCK/PENDING to CLEAR and
//   on the NUMOFTIMEPERIODS number of time periods. 
//
// RETURNS 
//   Void
//
Void
CRomBrevityCtl::addNewOM(const char* msgKey)
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In addNewOM for %s\n",(const char*) msgKey);
    fclose(fp);
  }
	int i;

	for(i = 0;i < _omArrayCnt;++i)
	{
		if((strcmp(_OMlist[i].omKey,"\0")) == 0) 
		{
			strncpy(_OMlist[i].omKey,msgKey,CROMKEYSZ);
			resetOM(i);
			return;
		}
	}

	strncpy(_OMlist[_omArrayCnt].omKey,msgKey,CROMKEYSZ);
	resetOM(i);

	if(_omArrayCnt < (CRMAXLISTSIZE-1))
	{
		++_omArrayCnt; // never should be the first one again
	}
	else
	{
		// _omArrayCnt should never be this large
		CRmsg om;
		om.setMsgClass("DEBUG");
		om.add("REPT ERROR LOG %s, FILE CRomBrevityCtl.C, LINE %d\n",
           CRprocname, __LINE__);
		om.add("Brevity Control Array fill up : %d\n",_omArrayCnt);
		om.add("Can't brevity control : %s",(const char*)msgKey);
		om.spool();
		removeOM(_omArrayCnt);
	}
}// end addNewOM


// NAME
//  Void resetOM(int arrayCnt)
//
// PURPOSE
//   This function reset the OM array list back to beginning
//
// PARAMETERS 
//      int arrayCnt - the arrayCnt;
//
// INCLUDES 
//
// DESCRIPTION
//
// RETURNS 
//   Void
//
Void
CRomBrevityCtl::resetOM(int arrayCnt)
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In resetOM for %d\n",arrayCnt);
    fclose(fp);
  }
	_OMlist[arrayCnt].omCnt            = 1;
	_OMlist[arrayCnt].totalomCnt       = 0;
	_OMlist[arrayCnt].nextTimePeriod   = getOMtime() + _period;
	_OMlist[arrayCnt].totalTimePeriods = 1;
	_OMlist[arrayCnt].status           = CRCLEAR;
}// end resetOM


// NAME
//  Void printDelayOM(Long thisTimePeroid )
//
// PURPOSE
//   This function will print a OM if one has not been printed in a while
//
// PARAMETERS 
//      Long thisTimePeroid - the time peroid
//
// INCLUDES 
//
// DESCRIPTION
//
// RETURNS 
//   Void calls printOMBlock
//
Void
CRomBrevityCtl::printDelayOM(Long thisTimePeroid )
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "In printDelayOM\n");
    fclose(fp);
  }
	if(_OMlist[0].omKey[0] != 0)
	{
		for(int i = 0;i < _omArrayCnt;++i)
		{
			if(_OMlist[i].omKey[0] == 0)
			{
				continue;
			}
			//
			// printing a delay block OM if it meets the
			// following crit.
			//
			// 1) status still BLOCK or PENDING
			// 2) nextTimePeriod greater then thisTimePeriod
			//    plus 5 period thresholds

			if((_OMlist[i].status == CRBLOCK ||  // or 
			    _OMlist[i].status == CRPENDING) && //and
         ((_OMlist[i].nextTimePeriod + (5 * _period))  < thisTimePeroid))
      {
        printBlockOM(i);
        _OMlist[i].status = CRCLEAR;

				//print clear here by setting totalomCnt to -1
				_OMlist[i].totalomCnt = -1;
				printBlockOM(i);
      }
			//
			// remove a OM if :
			//
			// 1) status CLEAR
			// 2) nextTimePeriod greater then thisTimePeriod
			//    plus 5 period thresholds
			if( ((_OMlist[i].status == CRCLEAR) &&
			     ((_OMlist[i].nextTimePeriod + (5 * _period))  < thisTimePeroid)))
      {
        removeOM(i);
      }
		}// end of for
	}//end of if

	// reset timer
	_printTimer = thisTimePeroid + CRTIMEDELAY; // five minutes

	// last element NULL remove it from list
	if((strcmp(_OMlist[_omArrayCnt].omKey,"\0")) == 0) 
	{
		if(_omArrayCnt > 0)
       --_omArrayCnt;
	}

}// end printDelayOM


// NAME
//  Void removeOM(Long thisTimePeroid )
//
// PURPOSE
//   This function will remove from the master list any OM that has not been 
//   access in a while.
//
// PARAMETERS 
//      Long thisTimePeroid - the time peroid
//
// INCLUDES 
//
// DESCRIPTION
//
// RETURNS 
//   Void 
//
Void
CRomBrevityCtl::removeOM(int i)
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "CHECK HERE In removeOM for %d and key = %s\n",
            i, (const char*) _OMlist[i].omKey);
    fclose(fp);
  }
	if(_OMlist[i].totalomCnt > 0 )
     printBlockOM(i);

	_OMlist[i].omKey[0]         = '\0';
	_OMlist[i].totalomCnt       = 0;
	_OMlist[i].omCnt            = 0;
	_OMlist[i].nextTimePeriod   = 0;
	_OMlist[i].totalTimePeriods = 0;
	_OMlist[i].status           = CRCLEAR;
	if(_omArrayCnt > 0)
     if(_omArrayCnt == i) // last element remove it from list
        --_omArrayCnt;
}// end removeOM


// NAME
//  CRomBrevityCtl* Instance()
//
// PURPOSE
//   This function starts the whole ball of wax. Only do a new once
//   so we don't over write the array.
//
// PARAMETERS 
//
// INCLUDES 
//
// DESCRIPTION
//
// RETURNS 
//   Void 
//
CRomBrevityCtl*
CRomBrevityCtl::Instance()
{
  int deflag=0;
  FILE *fp;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","w");
    fprintf(fp, "STARTING BREV CONTROL \n");
    fclose(fp);
  }
	if ( _instance == 0)
  {
    _instance = new CRomBrevityCtl();
  }
  return  _instance;
}



// NAME
//  CRomBrevityCtl* attachGDO
//
// PURPOSE
//   This function attachs to the GDO where records are kept
//   for specfic limits on OMs.
//
// PARAMETERS 
//
// INCLUDES 
//
// DESCRIPTION
//
// RETURNS 
//   GLretVal
//
GLretVal
CRomBrevityCtl::attachGDO()
{

  FILE *fp;
  int deflag=0;
  if( (strcmp(CRprocname,"SCC")==0) && (CRDEBUG_FLAGSET(123)) )
  {
    deflag=1;
  }

  if(deflag){
    fp=fopen("/tmp/brev.out","a");
    fprintf(fp, "attachGDO\n");
    fclose(fp);
  }
	GLretVal retval;
	Bool isNew = FALSE;
	if( _brevGDOaddr == NULL )
	{
		retval = CRgdo.attach( CRgdoOMtableName, /* name of GDO */
                           TRUE,           /* TRUE create GDO */
                           isNew,
                           CRbrevPerm,      /* permissions */
                           /* size of gdo */
                           (Long)sizeof(CRbrevSharedMemory), 
                           _brevGDOaddr,            /* pointer to GDO */
                           /* set to default values */
                           (Long)(3* sizeof(CRbrevSharedMemory)), 
                           (void*) NULL,

                           MHGD_ALL,   /* put the gdo on all nodes */
                           (Long)sizeof(CRbrevSharedMemory));

		if( isNew == TRUE )
		{
		  CRshData = (CRbrevSharedMemory *) _brevGDOaddr;
      CRshData->noOfRecords=0; /* set state to 0 for fresh GDO */
      CRshData->whichRecordSet=0; /* set state to 0 for fresh GDO */
      CRgdo.invalidate( &CRshData->noOfRecords, (int)sizeof(int) );
      CRgdo.invalidate( &CRshData->whichRecordSet, (int)sizeof(int) );
		}
		CRDEBUG(CRusli,("ATTACHING TO GDO : %d",retval));
		return retval;
	}
	return GLsuccess;
}
