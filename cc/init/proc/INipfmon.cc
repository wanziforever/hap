//
// FIX for roll over and large integar - DONE
// FIX for finding max number of block packets
// FIX for large number (longlong) - DONE
// FIX default message for seconds and threshold - DONE
// FIX for using popen
// FIX add comments
//
// This file implements the code to receive the RC/V changes from 
// SERVICE ACCESS form and apply the approperiate rules to ipfilter.
//
//

#include	<stdlib.h>
#include	<memory.h>
#include	<stdio.h>
#include 	<unistd.h>
#include 	<netdb.h>
#include 	<errno.h>
#include	<sys/types.h>
#include	<sys/errno.h>
#include 	<sys/uio.h>
#include	<hdr/GLtypes.h>
#include	<cc/hdr/su/SUexitMsg.hh>
#include	<cc/hdr/init/INusrinit.hh>
#include	<cc/hdr/init/INmtype.hh>
#include	<cc/hdr/cr/CRdebugMsg.hh>
#include 	<cc/hdr/eh/EHhandler.hh>
#include	<cc/hdr/tim/TMtmrExp.hh>
#include 	<cc/hdr/cr/CRdbCmdMsg.hh>
#include 	<cc/hdr/cr/CRomdbMsg.hh>
#include   	<cc/hdr/msgh/MHinfoExt.hh>
#include        "cc/hdr/cr/CRindStatus.hh"
#include 	"cc/hdr/db/DBintfcFuncs.hh"
#include 	"cc/hdr/db/DBmtype.h"
#include 	"cc/hdr/db/DBfmUpdate.hh"
#include 	"cc/hdr/db/DBfmInsert.hh"
#include 	"cc/hdr/db/DBfmDelete.hh"
#include 	"cc/hdr/db/DBintfaces.hh"
#include	"cc/hdr/timesten/TTdebug.h"
#include	"cc/init/proc/INipfhelper.h"
#include	"String.h"

EHhandler	INevent;
MHqid		INmhqid;		

#define         INmaxTimers             30
#define         INserverCheckTag        1L
#define         INserverCheckInterval   30
#define         INserverFailTag         2L
#define         INserverFailInterval    15
#define         INsanPegTag             3L
#define         INsanPegInterval        75

#define		IPFMON	"ipfmon"
#define 	APPLY_RULE "/sbin/ipf -F a -f /etc/ipf.conf"  
		// -F a flush the old rules

#ifndef __linux
#define 	INDOSconfFile   "/etc/ipf/DenialOfServiceAlarm.conf"
#define		INfileDefault	"# is a comment (must be column 1)\n# Interval has a range of 0 - 60 seconds\n# 0 turns off checking\n# Threshold has a range of 10 - 100000 packets\nInterval 0\nThreshold 100000"

int      INtimeToCheckForDoS = 0;
int      INtheThreshold = 100000;

const U_short INauditDoStimerTag = 100;

Void readDoSfile();
Void AuditTimer();
Void RunADoSAudit();

CROMDBKEY(alarmOM, "/SEC014");
CROMDBKEY(clearOM, "/SEC015");
#endif

// Class to handle SERVICE_ACCESS form messages
class ServiceAccessTbl
{
	public:
		enum ServiceAccessField
		{
      			LOGICAL_NAME,
      			PROTOCOL,
      			SERVICE_HOST,
      			SERVICE_HOST_2,
      			SERVICE_HOST_3,
      			SERVICE_HOST_4,
      			PORT_NO,
      			IN_OR_OUT,
      			ALLOW_DENY
    		};
      
    	static GLretVal triggerFunc(DBoperation typ, DBattribute *record, 
					char *&retResult);
    	static DBattribute rbuf[];
};

// Class to handle SERVICE_ACCESS form messages
class replNodesTbl 
{
  public:
    enum ReplicationField
    {
      HOSTNAME,
      HOSTNAME2,
      REP_TYPE,
      INH,
      end_null
    };

    static DBattribute rbuf[];

    static GLretVal triggerFunc( DBoperation typ, DBattribute *record, char *&retResult );
};

  // defines the buffer
DBattribute ServiceAccessTbl::rbuf[] =
{
        {"LOGICAL_NAME",     	30,      FALSE,  ""},
        {"PROTOCOL",   		30,     FALSE,  ""},
        {"SERVICE_HOST",   	64,     FALSE,  ""},
        {"SERVICE_HOST_2",   	64,     FALSE,  ""},
        {"SERVICE_HOST_3",   	64,     FALSE,  ""},
        {"SERVICE_HOST_4",   	64,     FALSE,  ""},
        {"PORT_NO",   		10,     FALSE,  ""},
        {"IN_OR_OUT",  		3,     FALSE,  ""},
        {"ALLOW_DENY",  	1,     FALSE,  ""},
        {"",            0,      FALSE,  ""}
};

DBattribute replNodesTbl::rbuf[] =
{
        {"HOSTNAME",     32, FALSE, ""},
        {"HOSTNAME2",    32, FALSE, ""},
        {"REP_TYPE",     1,  FALSE, ""},
        {"INH",      1,  FALSE, ""},
        {"",           0,  FALSE, ""}
};

//
// when there is a message passed to DBman, this function will be called
//
GLretVal 
ServiceAccessTbl::triggerFunc(DBoperation typ, DBattribute *record, 
				char *&retResult)
{
	static char err[ 1024 ];
  	err[ 1023 ] = 0;

  	String logicalName, protocol, host, host2, host3, host4, inout, 
		portno, allow;

       	CRDEBUG( CRinit, ( "ServiceAccessTbl::triggerFunc(): entering ") );
  	if ( record[ LOGICAL_NAME ].value != 0 )
	{
    		logicalName = record[ LOGICAL_NAME ].value;
	}

  	if ( record[ PROTOCOL ].value != 0 )
	{
    		protocol = record[ PROTOCOL ].value;
	}

  	if ( record[ SERVICE_HOST ].value != 0 )
	{
    		host = record[ SERVICE_HOST ].value;
	}

  	if ( record[ SERVICE_HOST_2 ].value != 0 )
	{
    		host2 = record[ SERVICE_HOST_2 ].value;
	}
	
  	if ( record[ SERVICE_HOST_3 ].value != 0 )
	{
    		host3 = record[ SERVICE_HOST_3 ].value;
	}

  	if ( record[ SERVICE_HOST_4 ].value != 0 )
	{
    		host4 = record[ SERVICE_HOST_4 ].value;
	}

  	if ( record[ IN_OR_OUT ].value != 0 ) 
	{
    		inout = record[ IN_OR_OUT ].value;
	}

  	if ( record[ PORT_NO ].value != 0 ) 
	{
    		portno = record[ PORT_NO ].value;
	}

  	if ( record[ ALLOW_DENY ].value != 0 ) 
	{
    		allow = record[ ALLOW_DENY ].value;
	}

  	IPFhelper helper;

    	// no need to handle other protocols
  	if ( ( protocol == "JDBCODBC" ) || ( protocol == "TCPIP" ) )
	{
		;
	}
	else
	{
    		return GLsuccess;
	}

  	bool isDelete = false;

  	switch (typ)
  	{
    		case DBinsertOp:
       			CRDEBUG( CRinit, 
			   ("serviceAccessTbl::triggerFunc(): insert type ") );
    		case DBupdateOp:  // Recent Change, update
       			CRDEBUG( CRinit, 
			   ("serviceAccessTbl::triggerFunc(): update type ") );
      			if ( allow != "A" && portno != "113" )
        			return GLsuccess;  
			// insert a 'deny' record doesn't affect current rules
      			break;
    		case DBdeleteOp:
      			if ( allow != "A" && portno != "113" )
        			return GLsuccess;  
				// delete a 'deny' record doesn't affect 
				// current rules
      			isDelete = true;
      			break;
    		default:
      			return GLsuccess;
  	}

  	String msg,repMsg;
	// get the rest of the rules
  	msg = helper.selectDB( logicalName, protocol, host);  
  	if ( strncmp( ( const char * )msg, "ERROR", 5 ) == 0 )
  	{
       		CRDEBUG( CRinit, ("serviceAccessTbl::selectDB error is %s ",
				(const char *)msg) );
    		strncpy( err, ( const char * )msg, 1023 );
    		retResult = err;
    		return GLfail;
  	}

       	CRDEBUG( CRinit, 
		("serviceAccessTbl::entering handleOneRecord(): isDelete is %d",
			isDelete) );
  	if ( !isDelete )
	{
      		msg += helper.handleOneRecord( host, host2, host3, host4, 
					       inout, portno, allow );

    		CRDEBUG( CRinit, 
			( "serviceAccessTbl::triggerFunc(): msg = %s ",
				( const char * )msg) );
	}

    	CRDEBUG( CRinit, 
		("serviceAccessTbl::entering selectRep(): isDelete is %d ",
				isDelete) );
	// get the rules for the ip addresses in the replication table
	repMsg = helper.selectRep();
	if ( strncmp( ( const char * )repMsg, "ERROR", 5 ) == 0 )
	{
		strncpy( err, ( const char * )repMsg, 1023 );
		retResult = err;
		return GLfail;
	}

    	CRDEBUG( CRinit, 
		( "serviceAccessTbl::triggerFunc(): repMsg = %s ",
				( const char * )repMsg) );
	// add the rules in repMsg to the msg
	msg += repMsg;
  	msg = helper.applyRules( msg );
  	if ( msg == "ipfilter not running" )
	{
		CRDEBUG_PRINT(0,("ipfilter service is off - line"));
		return GLsuccess;
	}

  	if ( msg != "" )
  	{
    		strncpy( err, ( const char * )msg, 1023 );
    		retResult = err;
    		return GLfail;
  	}

	return GLsuccess;
}

  // register those tables with DBman
DBman ServiceAccessMan("SERVICE_ACCESS", (U_short) 1, ServiceAccessTbl::rbuf, ServiceAccessTbl::triggerFunc);

GLretVal replNodesTbl::triggerFunc( DBoperation typ, DBattribute *record, char *&retResult )
{

  IPFhelper helper;
  String hostname, hostname2, inh,node,mate;
  String logicalName = " ", protocol = " ", host = " ";
  static char err[ 1024 ];
  err[ 1023 ] = 0;

       CRDEBUG( CRinit, ( "replNodesTbl::triggerFunc(): Entering .... ") );
  switch (typ)
  {
    case DBupdateOp:  // Recent Change, update
    {
       CRDEBUG( CRinit, ( "replNodesTbl::update operation .... ") );
		// if neither of the hostnames are set then return. The 
		// hostnames have not been changed
		if ((!record[ HOSTNAME ].set) && (!record[ HOSTNAME2 ].set))
		return GLsuccess;

  		if ( record[ HOSTNAME ].value != 0 )
		    hostname = record[ HOSTNAME ].value;

  		if ( record[ HOSTNAME2 ].value != 0 )
		    hostname2 = record[ HOSTNAME2 ].value;

      break;

    }

    default:        // Ignore other cases
       CRDEBUG( CRinit, ( "replNodesTbl::unknown operation type %d.... ",typ) );
		return GLsuccess;
      break;

  } // end switch(typ)

	// If we get to this place it means that the type was an update 
	// and the hostnames were set.

       CRDEBUG( CRinit, ( "replNodesTbl::triggerFunc(): Entering selectDB ") );
  String msg,tmpMsg;
  msg = helper.selectDB( logicalName, protocol, host);  // get the rest of the rules
  if ( strncmp( ( const char * )msg, "ERROR", 5 ) == 0 )
  {
    strncpy( err, ( const char * )msg, 1023 );
    retResult = err;
    return GLfail;
  }
       CRDEBUG( CRinit, ( "replNodesTbl::triggerFunc(): msg = %s ",( const char * )msg) );

	// add the entry in this update
      tmpMsg = helper.handleRepTblRecord( hostname, hostname2 );
  if ( strncmp( ( const char * )tmpMsg, "ERROR", 5 ) == 0 )
  {
    strncpy( err, ( const char * )tmpMsg, 1023 );
    retResult = err;
    return GLfail;
  }
       CRDEBUG( CRinit, ("replNodesTbl::triggerFunc(): tmpMsg = %s ",(const char *)tmpMsg) );
  if ( strncmp( ( const char * )tmpMsg, "INVALID", 5 ) != 0 )
  {
    msg += tmpMsg;
  }


	// add the rules in repMsg to the msg
  	msg = helper.applyRules( msg );
  	if ( msg == "ipfilter not running" )
    	{
      		CRDEBUG_PRINT(0,("ipfilter service is off - line"));
		return GLsuccess;
	}

  if ( msg != "" )
  {
    strncpy( err, ( const char * )msg, 1023 );
    retResult = err;
    return GLfail;
  }

  return GLsuccess;
}

DBman ReplicationTblMan( "REPLICATION", (U_short) 1, replNodesTbl::rbuf, replNodesTbl::triggerFunc);

//
// 	Comments for this function
//
Short
sysinit(int, char *[], SN_LVL, U_char)
{
	return(GLsuccess);
}// end of sysinit 

//
// 	Comments for this function
//
Short
procinit(int, char *argv[], SN_LVL sn_lvl, U_char)
{
	GLretVal	retval;

	CRERRINIT( IPFMON );
	
	retval = MHmsgh.attach();  // attach to message queue
	if ( retval != GLsuccess )
	{
		INITREQ(SN_LV0, retval, "FAILED TO ATTACH TO MSGH", IN_EXIT);
	}

	retval = MHmsgh.regName( IPFMON, INmhqid, FALSE );
	if ( retval != GLsuccess) 
	{
		INITREQ(SN_LV0, GLfail, "FAILED TO OBTAIN MSGH QID", IN_EXIT);
	}

	CRDEBUG(CRinit, ("procinit entry with SN_LVL = %d ",sn_lvl));

	if ((retval = INevent.tmrInit()) != GLsuccess)
			 return retval;


#ifndef __linux
	// load the rule into ipfilter kernel
	//start creating a new /etc/ipf.conf file
	IPFhelper helper;
	String msg,repMsg;
	// get the rest of the rules
	// note the variables are not being used
	msg = helper.selectDB( "NOT USED", "NOT USED", "NOT USED");  
	if ( strncmp( ( const char * )msg, "ERROR", 5 ) == 0 )
	{
		CRDEBUG( CRinit, 
			("Failed on getting SERVICE_ACCESS records\n%s",
				(const char *)msg) );
		msg = "";
	}

	repMsg = helper.selectRep();
	if ( strncmp( ( const char * )repMsg, "ERROR", 5 ) == 0 )
	{
		CRDEBUG( CRinit, 
			("Failed on getting REPLICATION records\n%s",
				(const char *)repMsg) );
		repMsg = "";
	}

	msg += repMsg;
	msg = helper.applyRules( msg );

  	if ( msg == "ipfilter not running" )
    	{
      		CRDEBUG_PRINT(0,("ipfilter service is off - line"));
		return GLsuccess;
	}

	if ( !msg.is_empty() )
	{
		INITREQ(SN_LV0, GLfail, "FAILED TO APPLY ipf RULE", IN_EXIT);
	}

	//
	// Check to see if theres a config file if not create 
	// a default one - feature 68450
	//
	readDoSfile();
	AuditTimer();

#endif
	return(GLsuccess);
}// end pf procinit 



Short
cleanup(int, char *[], SN_LVL, U_char)
{
    return(GLsuccess);
}// end of cleanup

Void
process(int, Char *argv[], SN_LVL sn_lvl, U_char)
{

	GLretVal	retval;
	MHmsgBase	*msg_p;
	Char 		msgbuf[ sizeof(MHmsgBase) + MHmsgSz ];
	Long		msglen;
        
	CRDEBUG(CRinit, ("process entry with SN_LVL = %d ",sn_lvl));

	while(1) 
	{
		char * ptr;

		IN_SANPEG();
		CRDEBUG(CRmeas, ("process: measuring how often we loop "));
		msglen = MHmsgSz;
		//retval = MHmsgh.receive( INmhqid, msgbuf, msglen, 0, 6000 );
		retval = INevent.getEvent(INmhqid, msgbuf, msglen);
		if(retval == GLsuccess) 
		{

			msg_p = (MHmsgBase *) msgbuf;

			switch (msg_p->msgType) {
			case SUexitTyp:
				exit(0);
				break;

            		// a debug on/off control msg
            		case CRdbCmdMsgTyp:
 				((CRdbCmdMsg*) msg_p)->unload();
                		break;
			
			case DBfmInsertTyp:
			{
				CRDEBUG(CRinit,
					("insert msgtype = %d received", 
						msg_p->msgType));
				DBfmInsert *fmMsgp = (DBfmInsert *)msgbuf;
            			DBman::dbFmMsg(fmMsgp, msglen, DBinsertOp, 
						DBfmInsertAckTyp, 
							fmMsgp -> srcQue);
				break;
			}

			case DBfmUpdateTyp:
			{
				CRDEBUG(CRinit,
					("update msgtype = %d received", 
						msg_p->msgType));
				DBfmUpdate *fmMsgp = (DBfmUpdate *)msgbuf;
            			DBman::dbFmMsg(fmMsgp, msglen, DBupdateOp, 
					DBfmUpdateAckTyp, fmMsgp -> srcQue);
				break;
			}

			case DBfmDeleteTyp:
			{
				CRDEBUG(CRinit,
					("delete msgtype = %d received", 
						msg_p->msgType));
				DBfmDelete *fmMsgp = (DBfmDelete *)msgbuf;
            			DBman::dbFmMsg(fmMsgp, msglen, DBdeleteOp, 
					DBfmDeleteAckTyp, fmMsgp -> srcQue);
				break;
			}

#ifndef __linux
			// new timer for feature 68450
			case TMtmrExpTyp:
			{
				CRDEBUG(CRinit,
					("TMtmrExpTyp msgtype = %d received", 
						msg_p->msgType));
				switch(((TMtmrExp *)msg_p)->tmrTag)
				{
				        // DoS audit timer
					case INauditDoStimerTag:
						CRDEBUG(CRinit,
					  ("Got a timer to run DOS" ));
						RunADoSAudit();
						break;

                                        default:
                                        // do nothing
                                                break;
				}// end timer switch
			}
			break;
#endif

			default:
				CRDEBUG(CRinit,
					("Unknown msgtype = %d received", 
						msg_p->msgType));
				break;
			}
       		}
	}
}// end of process() 

#ifndef __linux
//
// Check to see if theres a configure file is not create
// a default one - feature 68450
//
Void
readDoSfile()
{

	FILE *fp;
	if((fp = fopen( INDOSconfFile, "r" )) != NULL )
	{
		//Read the configuation from the file
		String 	strBuffer;
		String 	theNewlineChar = "\n";

        	int start = 0;
		int amt = 0;
		int value = 0;

        	strBuffer.make_empty();

		//read file in strBuffer
        	while( fgets(strBuffer, fp) != NULL)
        	{
			if ( strBuffer.index( "Interval" ) == 0 )
			{
        		   start = strBuffer.match("Interval") + 1;
			   // -1 is for newline
			   amt = strBuffer.length() - start - 1; 
			   value = atoi( strBuffer.chunk(start, amt));
			   INtimeToCheckForDoS = atoi( strBuffer.chunk(start, 
									amt));
			   if(( INtimeToCheckForDoS < 0 ) ||
			      ( INtimeToCheckForDoS > 60 ) )
			   {
				CRERROR("INTERVAL MUST BE BETWEEN 0 AND 60");
			   	INtimeToCheckForDoS=0;
			   }
			}

			if ( strBuffer.index( "Threshold" ) == 0 )
			{
        		   start = strBuffer.match("Threshold") + 1;
			   // -1 is for newline
			   amt = strBuffer.length() - start - 1; 
			   value = atoi( strBuffer.chunk(start, amt));
			   INtheThreshold = atoi( strBuffer.chunk(start, 
								  amt));
			   if(( INtheThreshold < 10 ) ||
			      ( INtheThreshold > 100000 ) )
			   {
				CRERROR("THRESHOLD MUST BE BETWEEN 10 AND 100000");
			   	INtheThreshold=100000;
			   }
			}
		}//end while loop
	}
	else
	{
		fclose(fp);
		//Create default file
		if((fp = fopen( INDOSconfFile, "w" )) != NULL )
		{
			//INtimeToCheckForDoS and INtheThreshold 
			//are set to default
			//values at initialization time.
			fprintf(fp,"%s\n", INfileDefault);
			fclose(fp);
		}
		else
		{
			CRERROR("COULD NOT CREATE %s FILE", INDOSconfFile);
		}
	}
}// end of readDoSfile 

//
// This function sets a timer to fire at (INtimeToCheckForDoS)
//
Short DoSTmrBlock = -1;

Void
AuditTimer()
{
	if ( INtimeToCheckForDoS == 0 )
	{
		return;
	}

	DoSTmrBlock = INevent.setRtmr(INtimeToCheckForDoS, INauditDoStimerTag,
					TRUE);
	if (TMINTERR(DoSTmrBlock))
	{
		CRERROR("INevent.setRtmr failed  setAudit Timer; code %d",
			DoSTmrBlock);
	}
}// end of AuditTimer() 

Void
RunADoSAudit()
{
	CRDEBUG(0,("IN RunADoSAudit"));

	static Bool SentAlarmOM = NO;
	static int firstTime = 1;
	static u_long lastBlockedPackets = 0;
	u_long blockedPackets = 0;

	if ( INtimeToCheckForDoS == 0 )
	{
		return;
	}

	// run ipfstat -h to see how many packets were block

	FILE *pipefp, *stderrfp;
	String tmpfname = tmpnam(NULL);
	//char cmdStrFile[] = "/usr/sbin/ipfstat -h | /usr/bin/grep ^\"input packets:\" | /usr/bin/cut -d: -f2";
	char cmdStrFile[] = "/usr/sbin/ipfstat -h ";
	char linebuf[100];
	linebuf[0] = 0;

	// create file to trap stderr output from /bin/sh
	// If the user tries to execute a non-existent command
	// /bin/sh will print a message on stderr

	stderrfp = freopen(tmpfname, "w", stderr);
	if (stderrfp == NULL)
	{
		CRERROR("redirection of stderr to '%s' failed (errno = %d)",
	                        (const char*) tmpfname, errno);
		return;
	}

	pipefp = popen(cmdStrFile, "r");
	if (pipefp == NULL)
	{
		CRERROR("popen of '%s' failed", cmdStrFile);
		fclose(stderrfp);
		unlink(tmpfname);
		return;
	}
	fclose(stderrfp);

	//
	//	get blocked packets
	//
	String 	strBuffer;
        while( fgets(strBuffer, pipefp) != NULL)
        {
		if ( strBuffer.index( " input packets:" ) == 0 )
		{
// input packets:         blocked 123 passed 524273 nomatch 0 counted 0 short 0

			int begin = strBuffer.index( "blocked" ) + 8;
        		int end = strBuffer.index("passed");
			int number = end - begin - 1;
        		blockedPackets = atol(strBuffer.chunk(begin,number));
			break; //break out of loop
		}
        }

	pclose(pipefp);
	unlink(tmpfname);
	if( blockedPackets < 0 )
	{
		CRERROR("ipfstat command failed to obtain blocked packets");
		return;
	}
	else
	{
		CRDEBUG(CRinit, ("Number of blocked packets is %d",
			blockedPackets));
	}

	if(firstTime)
	{
		lastBlockedPackets = blockedPackets;
		firstTime = 0;
	}

	//If the last number of blocked packets were greater then
	//the new number of blocked packets then we had a roll over
	u_long missingBlockPackets=0;
	u_long totalBlockPackets=0;
	if( lastBlockedPackets > blockedPackets )
	{
		//need to set maxNumbetrOfBlockedPackets to max number of packets
		//missingBlockPackets=maxNumberOfBlockedPackets-lastBlockedPackets;
		//totalBlockPackets=blockedPackets+missingBlockedPackets;
		totalBlockPackets=blockedPackets;
	}
	else
	{
		totalBlockPackets=blockedPackets-lastBlockedPackets;
	}
	CRDEBUG(CRinit, 
		("totalBlockedPackets > INtheThreshold %ld > %ld",
			totalBlockPackets, INtheThreshold));

	if(totalBlockPackets > INtheThreshold)
	{
		//print alarmed OM
		SentAlarmOM = YES;
		CRomdbMsg om;
		om.add((long)totalBlockPackets);
		om.add(INtimeToCheckForDoS);
		om.spool(alarmOM);
	}
	else
	{
		//print clear OM
		if(SentAlarmOM)
		{
			CRomdbMsg om;
			om.add((long)totalBlockPackets);
			om.add(INtimeToCheckForDoS);
			om.spool(clearOM);
		}
		SentAlarmOM = NO;
	}
	lastBlockedPackets = blockedPackets;

}// end of RunADoSAudit() 
#endif
