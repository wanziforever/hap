#ifndef __ININITDATA_H
#define __ININITDATA_H

// DESCRIPTION:
//	This header contains the definition of the "INinitdataReq",
//      "INinitdataResp". It is expected that OP:INIT will generate a 
//      request to INIT for all the processes data. And INIT will 
//      respond to the INinitdataReq message with multiple INinitdataResp
//      message.
//	
//
// NOTES:
//


#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"

#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/hdr/init/INmtype.hh"

/* #include "cc/hdr/cr/CRomInfo.H" */

struct INopinitdata {
       char           proctag[IN_NAMEMX];
       pid_t          pid;
       IN_PROC_CATEGORY  proc_category;
       U_short        peg_intvl;
       IN_SYNCSTEP    procstep;
       IN_PROCSTATE   procstate;
       IN_STARTSTATE  startstate;
       Bool           softchk;
       U_char         run_lvl;
       MHqid          qid;
       Short	      ps;
};

struct INsystemdata {

        INITSTATE  	initstate;
        SN_LVL     	sn_lvl;
        U_char     	run_lvl;
        IN_SYNCSTEP 	systep;
        IN_SOFTCHK  	softchk;
        Long        	availsmem;
        unsigned char 	vmemalvl;
        IN_INFO        	info;
	char		mystate;
	char		wdstate;

};

struct INoprstrtdata {

       char             proctag[IN_NAMEMX];
       IN_SYNCSTEP      procstep;
       IN_STARTSTATE    startstate;
       U_short          rstrt_cnt;
       U_short          tot_rstrt;
       U_short          rstrt_max;
       U_short          rstrt_intvl;
       IN_UPDSTATE      updstate;
       Short            error_count;
       U_short          error_threshold;
       MHqid		qid;
       int		brevity_low;
       int		brevity_high;
       int		brevity_interval;
};

#define  IN_RESTART_DATA     1
#define  IN_OP_INIT_DATA     2
#define  IN_OP_STATUS_DATA   3

#define  INMAXITEMSPERMESSAGE ((MHmsgSz - 100)/(sizeof(INopinitdata) + sizeof(Long)))
#define  INMAXHOSTS          20
/*
**  "INinitdataReq" message class:
**
**	The message type for this message is "INinitSCNTyp" as defined in
**	"cc/hdr/init/mtype.h".
**
*/
class INinitdataReq : public MHmsgBase {
    public:
		INinitdataReq()
			 { priType = MHoamPtyp;
			   msgType = INinitdataReqTyp;
			 };
        inline INinitdataReq( const char *name, int msg_type );
	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(const Char *name, MHqid fromQid, Long time);
        inline int sendToAllHosts(const Char *name, MHqid fromQid, Long time );

	Char 		msgh_name[IN_NAMEMX];
#ifdef __linux
	Char		ext_name[IN_PATHNMMX];
#endif
        int             type;  // OP_INIT or OP_RESTART      
};


inline GLretVal
INinitdataReq::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INinitdataReq),
			   time);
}


inline GLretVal
INinitdataReq::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INinitdataReq),
			   time);
}

inline int
INinitdataReq::sendToAllHosts( const Char *name, MHqid fromQid, Long time )
{
    // This is broadcast to all the machine

    srcQue = fromQid;
    return MHmsgh.sendToAllHosts( name, ( Char *) this, sizeof(INinitdataReq),                                   time, TRUE );

} 
inline
INinitdataReq::INinitdataReq( const char *name, int msg_type )      
{
     priType = MHoamPtyp;
     msgType = INinitdataReqTyp;

     Short len = strlen(name);
     if(len >= IN_NAMEMX) {
          strncpy(msgh_name, name, ( IN_NAMEMX - 1 ));
          msgh_name[IN_NAMEMX-1] = (Char)0;
     } else {
          strcpy( msgh_name, name );
     }
     type = msg_type;
#ifdef __linux
     ext_name[0] = 0;
#endif
}

class   INinitdataResp  : public MHmsgBase {

   public :
            INinitdataResp()
	       { priType = MHinitPtyp;
	         msgType = INinitdataRespTyp;
                 return_val = GLsuccess;
               }
     
     inline GLretVal  send(MHqid toQid, MHqid fromQid, Long msg_size, Long time);

     int    	nCount; // if 0 this status and last message 
     GLretVal   return_val;
     short   	fill;
     char    	data[1]; 
};    

inline GLretVal
INinitdataResp::send(MHqid toQid, MHqid fromQid, Long msg_size, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, msg_size, time);
}


#endif
