// DESCRIPTION:
// 	This file contains functions which handle INIT's message processing
//	requirements.
//
// FUNCTIONS:
//	INrcvmsg()  - Processes messages received by INIT
//	INgetsudata()- Extract SU information form proclist
//	INgetMateCC() - get name of mate CC
//
// NOTES:
//
#include <stdlib.h>
#include <sysent.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "hdr/GLmsgs.h"


#include "cc/hdr/tim/TMmtype.hh"		/* TMtmrExpTyp message type */
#include "cc/hdr/tim/TMtmrExp.hh"	/* TMtmrExp "message" class */

#include "cc/hdr/eh/EHreturns.hh"
#include "cc/hdr/eh/EHhandler.hh"

//#include "cc/hdr/cr/CRdbCmdMsg.hh"
//#include "cc/hdr/cr/CRomInfo.hh"
//#include "cc/hdr/cr/CRmtype.hh"
#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INreturns.hh"	/* Return codes for INIT msgs	*/
//#include "cc/hdr/ft/FTbladeMsg.hh"

//#ifdef OLD_SU
//#include "cc/hdr/su/SUmtype.hh"
//#include "cc/hdr/su/SUcommitMsg.hh"
//#include "cc/hdr/su/SUapplyMsg.hh"
//#include "cc/hdr/su/SUbkoutMsg.hh"
//#include "cc/hdr/su/SUreturns.hh"
//#endif

/* INIT messages: */
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INrunLvl.hh"	/* Increase system run level msg */
#include "cc/hdr/init/INprcCreat.hh"	/* Temporary process crtn. msgs. */
#include "cc/hdr/init/INprcUpd.hh"	/* Temporary process crtn. msgs. */
#include "cc/hdr/init/INsetRstrt.hh"	/* ALW/INH:RESTART messages 	 */
#include "cc/hdr/init/INsetSoftChk.hh"	/* ALW/INH:SOFTCHK messages	 */
#include "cc/hdr/init/INkillProc.hh"	/* Kill (temp.) proc. msg	*/
#include "cc/hdr/init/INswcc.hh"		/* Switch CC			*/

#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INtimers.hh"
#include "cc/init/proc/INlocal.hh"
#include "cc/init/proc/INmsgs.hh"
#include "cc/hdr/init/INinitialize.hh"
#include "cc/hdr/msgh/MHgqInit.hh"
#include "cc/hdr/msgh/MHgq.hh"
#include "cc/hdr/init/INinitData.hh"
//#include "cc/hdr/ft/FTmsgs.hh"
//#include "cc/hdr/ft/FThwm.hh"

int	INoamConflictCnt = 0;
#define INmaxOamConflict 	2

/*
** NAME:
**	INrcvmsg()
**
** DESCRIPTION:
**	This function handles messages received by INIT.
**
** INPUTS:
**	msg	- Pointer to message received from MSGH queue
**	msgsize - Size of message (returned by "MHinfoExt::receive()")
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

Void
INrcvmsg(Char *msg, int msgsize)
{
	U_short	i;
	GLretVal ret;

	Short msgType = ((class MHmsgBase *)msg)->msgType;
	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR ),(POA_INF,"INrcvmsg(): entered with msg. type 0x%x, msgsize %d",msgType, msgsize));
  printf("INrcvmsg(): entered with msg. type 0x%x, msgsize %d\n",msgType, msgsize);

MHqid 		orig_qid = ((class MHmsgBase *)msg)->srcQue;
Char		name[IN_NAMEMX];
MHenvType	env;
	
INevent.getEnvType(env);

switch(msgType) {
case INinitdataReqTyp:
   {
     int  msgindx = 0;
     int  max_indx = IN_SNPRCMX - 1;
     int  indx = 0;
     GLretVal     rtn ;

     INinitdataReq *req_msg = (INinitdataReq *)msg;
     int    buffer[(MHmsgLimit + sizeof(long))/sizeof(long)];

     if(req_msg->msgh_name[0] == 0 && req_msg->ext_name[0] != 0){
       for (i = 0; i < IN_SNPRCMX; i++) {
         if (IN_VALIDPROC(i)) {
           if (strcmp(IN_LDPTAB[i].ext_pathname, req_msg->ext_name) == 0) {
             strcpy(req_msg->msgh_name, IN_LDPTAB[i].proctag);
             break;
           }
         } 
       }
       if(i == IN_SNPRCMX){
         req_msg->msgh_name[0] = ' ';
         req_msg->msgh_name[1] = 0;
       }
     }

     if(req_msg->msgh_name[0] != 0){
       if((i = INfindproc(req_msg->msgh_name)) >= IN_SNPRCMX) {
         //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg(): \"INinitdataReq\" message received with unknown proc. name %s",req_msg->msgh_name));
         printf("INrcvmsg(): \"INinitdataReq\" message received with unknown proc. name %s\n",
                req_msg->msgh_name);
         INinitdataResp pmsg;
				 pmsg.return_val = INNOPROC;
         // Send the ack back with failure code 
 
         if((rtn = pmsg.send(orig_qid, INmsgqid, sizeof(INinitdataResp), 0L)) != GLsuccess ) {
           //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_ALWAYSTR ),(POA_INF,"INrcvmsg():\"Failure sending fail ack back %d",rtn ));
           printf("INrcvmsg():\"Failure sending fail ack back %d\n",rtn );
         }
         return;
       } else {
         max_indx = i;
         indx = i;
       }
     } 
     switch(req_msg->type){
     case IN_OP_INIT_DATA:
        {
          INinitdataResp* pmsg;
          pmsg = (INinitdataResp*)buffer;

          pmsg->priType = MHoamPtyp;
          pmsg->msgType = INinitdataRespTyp;

          INopinitdata* pinit;
          pinit = (INopinitdata*)pmsg->data;

          for(;indx <= max_indx; indx++){
            if(IN_INVPROC(indx)){
              continue;
            }
            if(msgindx >= INMAXITEMSPERMESSAGE){
              pmsg->nCount = msgindx;
              pmsg->return_val = GLsuccess;

              if((rtn = pmsg->send(orig_qid, INmsgqid,((char *)&pinit[msgindx] - (char *)pmsg ), 0L)) != GLsuccess){

                //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_ALWAYSTR ),(POA_INF,"INrcvmsg():\"Failure sending response to OP_INIT %d",rtn ));
                printf("INrcvmsg():\"Failure sending response to OP_INIT %d\n",rtn );
                return;

              }
              msgindx = 0; 
            } 
            strcpy(pinit[msgindx].proctag,IN_LDPTAB[indx].proctag);
            pinit[msgindx].pid     = IN_LDPTAB[indx].pid;
            if(pinit[msgindx].pid == IN_FREEPID && IN_LDPTAB[indx].third_party){
              pinit[msgindx].pid = 0;
            }
            pinit[msgindx].proc_category = IN_LDPTAB[indx].proc_category;
            pinit[msgindx].peg_intvl = IN_LDPTAB[indx].peg_intvl;
            pinit[msgindx].procstep = IN_SDPTAB[indx].procstep;
            pinit[msgindx].procstate = IN_SDPTAB[indx].procstate;
            pinit[msgindx].startstate = IN_LDPTAB[indx].startstate;
            pinit[msgindx].softchk = IN_LDPTAB[indx].softchk;
            pinit[msgindx].run_lvl = IN_LDPTAB[indx].run_lvl;
            pinit[msgindx].ps = IN_LDPTAB[indx].ps;
            char    name[MHmaxNameLen+1];
            INgetRealName(name, IN_LDPTAB[indx].proctag);
            if(INevent.getMhqid(name, pinit[msgindx].qid) != GLsuccess ||
               INevent.Qid2Host(pinit[msgindx].qid) != INevent.getLocalHostIndex()){
              // Put INIT qid for processes that do not have a queue yet
              // so at least a right machine is showing
              pinit[msgindx].qid = INmsgqid;
            }
            INevent.convQue(pinit[msgindx].qid, INevent.Qid2Host(orig_qid));
            msgindx++;
          }/* end of for */                 
          if(msgindx > 0){
            pmsg->nCount = msgindx;
            pmsg->return_val = GLsuccess;
            if((rtn = pmsg->send(orig_qid, INmsgqid,((char *)&pinit[msgindx] - (char *)pmsg ), 0L)) != GLsuccess){
              //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_ALWAYSTR), (POA_INF,"INrcvmsg():\"Failure sending response to OP_INIT %d",rtn ));
              printf("INrcvmsg():\"Failure sending response to OP_INIT %d\n",rtn );
              return;
            } 
          }
          if(req_msg->msgh_name[0] != 0){
            return;
          }
          // Intentional fall through
        }/* end of case */
     case IN_OP_STATUS_DATA:
        {
          INinitdataResp* pmsg;
          pmsg = (INinitdataResp*)buffer;
          pmsg->priType = MHinitPtyp;
          pmsg->msgType = INinitdataRespTyp;
          /* Now send the status information */
          INsystemdata* psysdata;

          psysdata = (INsystemdata *)pmsg->data; 
          psysdata->initstate = IN_LDSTATE.initstate;
          psysdata->sn_lvl = IN_LDSTATE.sn_lvl;
          psysdata->run_lvl = IN_LDSTATE.run_lvl;
          psysdata->systep = IN_LDSTATE.systep;
          psysdata->softchk = IN_LDSTATE.softchk;
          psysdata->availsmem = IN_LDVMEM;
          psysdata->vmemalvl = IN_LDVMEMALVL;
          psysdata->info = IN_LDINFO;
          psysdata->mystate = IN_LDCURSTATE;
          psysdata->wdstate = IN_LDWDSTATE;
 
          /* Sending all the system information  */
          pmsg->nCount = 0;
          pmsg->return_val = GLsuccess;
 
          if((rtn = pmsg->send( orig_qid, INmsgqid, ((char *)&psysdata[1]-(char *)pmsg ), 0L)) != GLsuccess){
            //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_ALWAYSTR),(POA_INF, "INrcvmsg():Failure sending response to OP_INIT rtn %d", rtn));
            printf("INrcvmsg():Failure sending response to OP_INIT rtn %d\n", rtn);
            return; 
          }
          return;
        }
     case IN_RESTART_DATA:
        {
          INinitdataResp* pmsg;
          pmsg = (INinitdataResp *)buffer;
          pmsg->priType = MHoamPtyp;
          pmsg->msgType = INinitdataRespTyp;

          INoprstrtdata* prstrt = (INoprstrtdata*)pmsg->data;
          int msgindx = 0;

          for(; indx <= max_indx ; indx++){
            if(IN_INVPROC(indx)){
              continue;
            }
            if(msgindx >= INMAXITEMSPERMESSAGE) {
              pmsg->nCount = msgindx;
              pmsg->return_val = GLsuccess ;
 
              if((rtn = pmsg->send(orig_qid, INmsgqid,((char *)&prstrt[msgindx] - (char *)pmsg), 0L)) != GLsuccess) {
                //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR ),(POA_INF,"INrcvmsg():\"Failure sending response to OP_RESTART %d",rtn ));
                printf("INrcvmsg():\"Failure sending response to OP_RESTART %d\n",rtn );
                return;
              }
              msgindx = 0;
            } 
            strcpy(prstrt[msgindx].proctag,IN_LDPTAB[indx].proctag);
            prstrt[msgindx].procstep  = IN_SDPTAB[indx].procstep;
            prstrt[msgindx].startstate = IN_LDPTAB[indx].startstate;
            prstrt[msgindx].rstrt_cnt = IN_LDPTAB[indx].rstrt_cnt;
            prstrt[msgindx].tot_rstrt = IN_LDPTAB[indx].tot_rstrt;
            prstrt[msgindx].rstrt_max = IN_LDPTAB[indx].rstrt_max;
            prstrt[msgindx].rstrt_intvl = IN_LDPTAB[indx].rstrt_intvl;
            prstrt[msgindx].updstate = IN_LDPTAB[indx].updstate;
            prstrt[msgindx].error_count = IN_SDPTAB[indx].error_count;
            prstrt[msgindx].error_threshold = IN_LDPTAB[indx].error_threshold;
            prstrt[msgindx].qid = INmsgqid;
            prstrt[msgindx].brevity_low = IN_LDPTAB[indx].brevity_low;
            prstrt[msgindx].brevity_high = IN_LDPTAB[indx].brevity_high;
            prstrt[msgindx].brevity_interval = IN_LDPTAB[indx].brevity_interval;
            INevent.convQue(prstrt[msgindx].qid, INevent.Qid2Host(orig_qid));
            msgindx++;
          }/* end of for */
          if(msgindx > 0) {
            pmsg->nCount = msgindx;
            pmsg->return_val = GLsuccess;

            if((rtn = pmsg->send(orig_qid, INmsgqid,((char *)&prstrt[msgindx] - (char *)pmsg), 0L)) != GLsuccess) {
              //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR ),(POA_INF,"INrcvmsg():\"Failure sending response to OP_RESTART %d",rtn ));
              printf("INrcvmsg():\"Failure sending response to OP_RESTART %d\n",rtn );
            }
          }
          if(msgindx == INMAXITEMSPERMESSAGE) {
            /* Send a last one empty to tell CEP we're done. */
            pmsg->nCount = 0;
            pmsg->return_val = GLsuccess;
            if((rtn = pmsg->send(orig_qid, INmsgqid,((char *)&prstrt[msgindx] - (char *)pmsg), 0L)) != GLsuccess) {
              //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR ),(POA_INF,"INrcvmsg():\"Failure sending response to OP_RESTART %d",rtn ));
              printf("INrcvmsg():\"Failure sending response to OP_RESTART %d\n",rtn );
            }
          }
		      return;
        }/* end of case */
     default :
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_ALWAYSTR),(POA_INF,"INrcvmsg(): \"INinitdataReq\" message received with unknown request type %d", req_msg->type));
       printf("INrcvmsg(): \"INinitdataReq\" message received with unknown request type %d\n",
              req_msg->type);
       return;
     }/* end of switch */ 
   }/* end of case INinitdataReq */
case INsetRunLvlTyp:
   {
     class INsetRunLvl *rlvl_msg = (class INsetRunLvl *)msg;

     if (msgsize != sizeof(class INsetRunLvl)) {
       /*
        * Note error and break out of processing for
        * this message...i.e. dump it.
        */
       //INIT_ERROR(("\"INsetRunLvl\" message received with size %d, expected %d", msgsize, sizeof(class INsetRunLvl)));
       printf("\"INsetRunLvl\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INsetRunLvl));
       return;
     }

     if(IN_LDCURSTATE == S_OFFLINE){
       class INlrunLvlAck rlvl_ack(0, IN_LDSTATE.run_lvl);
       (Void)rlvl_ack.send(orig_qid, INmsgqid, 0);
       return;
     }

     if (rlvl_msg->run_lvl < IN_LDSTATE.final_runlvl) {
       if(rlvl_msg->run_lvl != 0){
         //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SSEQTR),(POA_INF,"INrcvmsg(): Error - requested run level %d below current level %d",rlvl_msg->run_lvl, IN_LDSTATE.run_lvl));
         printf("INrcvmsg(): Error - requested run level %d below current level %d\n",
                rlvl_msg->run_lvl, IN_LDSTATE.run_lvl);

         class INlrunLvlFail rlvl_fail(ININVRUNLVL, IN_LDSTATE.run_lvl);
         (Void)rlvl_fail.send(orig_qid, INmsgqid, 0);
         return;
       } else {
         /* Run level of 0 means use current run level,
         ** in order to update INITLIST parameters.
         */
         rlvl_msg->run_lvl = IN_LDSTATE.final_runlvl;
       }
     }

     /* Do not accept set runlvl command during initialization
     ** of if other process synchronization is in progress
     ** i.e. software update apply or backout.
     */
     if ((IN_LDSTATE.initstate != IN_NOINIT && 
          IN_LDSTATE.initstate != IN_CUINTVL) || 
         (IN_LDSTATE.sync_run_lvl < IN_LDSTATE.final_runlvl)) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SSEQTR),(POA_INF,"INrcvmsg(): INIT request made while in system init. state %s...ignored",IN_STATENM(IN_LDSTATE.initstate)));
       printf("INrcvmsg(): INIT request made while in system init. state %s...ignored\n",
              IN_STATENM(IN_LDSTATE.initstate));

       class INlrunLvlFail rlvl_fail(INVINITSTATE, IN_LDSTATE.run_lvl);
       (Void)rlvl_fail.send(orig_qid, INmsgqid, 0);
       return;
     }

     /*
      * At this point we will initiate "set:runlvl" processing
      * - first, send an ACK back to the requesting process:
      */
     U_char orun_lvl = IN_LDSTATE.final_runlvl;
     IN_LDSTATE.final_runlvl = rlvl_msg->run_lvl;		
     IN_LDSTATE.run_lvl = rlvl_msg->run_lvl;		

     /*
      * Now we're ready to re-read the "initlist" and
      * schedule any new procs for startup:
      */
     Short num_procs = INrdinls(FALSE,FALSE);

     /* check return from reading initlist */
     if (num_procs < 0) {
       IN_LDSTATE.run_lvl = orun_lvl;		
       IN_LDSTATE.final_runlvl = orun_lvl;		
       /*
        * Something is wrong, generate error message
        * and continue:
        */
       //INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrcvmsg(): Error %d returned from \"INrdinls()\"\n\twhile increasing system run level to %d",num_procs, IN_LDSTATE.run_lvl));
       printf("INrcvmsg(): Error %d returned from \"INrdinls()\"\n\twhile increasing system run level to %d\n",
              num_procs, IN_LDSTATE.run_lvl);
       class INlrunLvlFail rlvl_fail(INBADINITLIST, IN_LDSTATE.run_lvl);
       (Void)rlvl_fail.send(orig_qid, INmsgqid, 0);
       return;
     }

     { 	/* make C++ happy! */

       class INlrunLvlAck rlvl_ack(num_procs, IN_LDSTATE.run_lvl);
       (Void)rlvl_ack.send(orig_qid, INmsgqid, 0);
     }

     /*
      * Set up information about this init. for future reference
      * in IN_LDINFO:
      */
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.run_lvl;
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = IN_RUNLVL;
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = 0;
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = num_procs;
     IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name[0] = 0;

     if (num_procs == 0) {
       IN_LDINFO.init_data[IN_LDINFO.ld_indx].psn_lvl = SN_NOINIT;
       /*
        * If no processes are to be initialized consider
        * the initialization to be "completed":
        */
       if (IN_LDSTATE.run_lvl > orun_lvl) {
         /* Update sync_run_lvl	*/
         IN_LDSTATE.sync_run_lvl = IN_LDSTATE.run_lvl;
         /*
          * The new run level is greater than
          * the old run level so we need to broadcast
          * the new system run level:
          */
         class INlrunLvl lrlvl(IN_LDSTATE.run_lvl);
         (Void)lrlvl.broadcast(INmsgqid, 0);
       }

       IN_LDINFO.init_data[IN_LDINFO.ld_indx].end_time = time((time_t *)0);
       /*
        * Update INIT data load and unload indices so
        * they'll be correct for the next init:
        */
       IN_LDINFO.ld_indx = IN_NXTINDX(IN_LDINFO.ld_indx, IN_NUMINITS);
       if (IN_LDINFO.ld_indx == IN_LDINFO.uld_indx) {
         IN_LDINFO.uld_indx = IN_NXTINDX(IN_LDINFO.uld_indx, IN_NUMINITS);
       }

       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SSEQTR | IN_BSEQTR),(POA_INF,"INrcvmsg(): \"INrdinls()\" returned with no new processes to start up"));
       printf("INrcvmsg(): \"INrdinls()\" returned with no new processes to start up\n");
       return;
     }

     IN_LDINFO.init_data[IN_LDINFO.ld_indx].psn_lvl = SN_LV4;

     /*
      * Now initialize shared memory state variables to set
      * a "full boot" recovery in motion
      */
     if(INevent.onLeadCC()){
       IN_LDSTATE.sn_lvl = SN_LV5;
     } else {
       IN_LDSTATE.sn_lvl = SN_LV4;
     }
     IN_LDSTATE.initstate = INITING;

     INworkflg = TRUE;
     IN_LDSTATE.systep = IN_SYSINIT;
     IN_LDSTATE.sync_run_lvl = 0;
     INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);

     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SSEQTR | IN_BSEQTR),(POA_INF,"INrcvmsg(): %d procs. scheduled for first init. at system run level %d",num_procs, IN_LDSTATE.run_lvl));
     printf("INrcvmsg(): %d procs. scheduled for first init. at system run level %d\n",
            num_procs, IN_LDSTATE.run_lvl);
     return;
   }

case INinitializeTyp:
   {
     class INinitialize *initmsg = (class INinitialize *)msg;
     if (msgsize != sizeof(class INinitialize)) {
       /*
        * Note error and break out of processing for
        * this message...i.e. dump it.
        */
       //INIT_ERROR(("\"INinitialize\" message received with size %d, expected %d", msgsize, sizeof(class INinitialize)));
       printf("\"INinitialize\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INinitialize));
       return;
     }
     if(initmsg->sn_lvl == SN_LV5){
       // Reboot this node if not offline
       if(IN_LDCURSTATE != S_OFFLINE){
         INsysboot(IN_INTENTIONAL, FALSE);
       }
     } else {
       //INIT_ERROR(("\"INinitialize\" invalid level %d", initmsg->sn_lvl));
       printf("\"INinitialize\" invalid level %d\n", initmsg->sn_lvl);
     }
     break;
   }
case INprocCreateTyp:
   {

     // If in the first boot phase ignore this message
     if(IN_LDSTATE.run_lvl == IN_LDSTATE.first_runlvl){
       return;
     }

     Short 			indx = 0;
     Bool 			avail_slot = FALSE;
     class INprocCreate*	pcreate = (class INprocCreate *)msg;

     /* Guarantee that the process name is properly terminated */
     pcreate->msgh_name[IN_NAMEMX-1] = (Char)0;

     if(pcreate->msgh_name[0] == 0){
       if( pcreate->ext_path[0] != 0 && pcreate->third_party == TRUE){
         /* Verify ext_path and find */
         for (i = 0; i < IN_SNPRCMX; i++) {
           if (IN_VALIDPROC(i)) {
             if (strcmp(IN_LDPTAB[i].ext_pathname, pcreate->ext_path) == 0) {
               strcpy(pcreate->msgh_name, IN_LDPTAB[i].proctag);
               break;
             }
           } 
         }
         if(i == IN_SNPRCMX){
           /* Create a mangled MSGH name based on ext_path	
           ** Take first 4 chars and last 10 chars. 
           ** If conflict, add a digit to the 5th character
           */
           char name[IN_NAMEMX];
           if(pcreate->ext_path[0] != '.'){
             strncpy(name, pcreate->ext_path, 4);
           } else {
             strncpy(name, &pcreate->ext_path[1], 4);
           }

           int nLen = strlen(pcreate->ext_path);
           char tmp[IN_NAMEMX];
           tmp[IN_NAMEMX-1] = 0;
           strncpy(tmp, &pcreate->ext_path[nLen - IN_NAMEMX + 1], IN_NAMEMX- 1);
           int j;
           int k;
           for(j = IN_NAMEMX - 2, k = IN_NAMEMX - 2; j >= 0 && k >= 5; j--){
             if(tmp[j] != '_'){
               name[k] = tmp[j];
               k--;
             }
           }
           name[4] = '_';
           name[IN_NAMEMX - 1] = 0;

           for(k = -1; k < 10; k++){
             for(j = 0; j < IN_SNPRCMX; j++){
               if (strcmp(IN_LDPTAB[j].proctag, name) == 0) {
                 break;
               }
             }
             if(j == IN_SNPRCMX){
               strcpy(pcreate->msgh_name, name);
               break;
             } else {
               name[4] = '0' + k + 1;
             }
           }
           if(k == 10){
             class INlprocCreateFail pfail(pcreate->msgh_name, INMAXPROCS);
             pfail.send(orig_qid, INmsgqid, 0);
             return;
           }
         }
       }  else {
         class INlprocCreateFail pfail(pcreate->msgh_name, INDUPMSGNM);
         pfail.send(orig_qid, INmsgqid, 0);
         return;
       }
     }

     /* Verify "msgh_name" and find */
     for (i=0; i < IN_SNPRCMX; i++) {
       if (IN_VALIDPROC(i)) {
         if (strcmp(IN_LDPTAB[i].proctag, pcreate->msgh_name) == 0) {
           if(pcreate->msgh_qid == INfixQ && IN_LDPTAB[i].msgh_qid > 0){
             pcreate->msgh_qid = IN_LDPTAB[i].msgh_qid;
           }
           break;
         }
       }
       else if (avail_slot == FALSE) {
         avail_slot = TRUE;
         indx = i;
       }
     }
		
     /*
      * Duplicate proc names are accepted but only if a process
      * is dead, is not in a process of being restarted and is
      * not a permament process.
      * Also guarantee that the
      * new process doesn't use INIT's queue name and explicitly
      * prevent MSGH from being started as a temporary process
      * -- this is important because "IN_LDMSGHLVL" is set in
      * "INrdinls()" while reading PERMANENT process names from
      * the "initlist":
      */
     if (((i < IN_SNPRCMX) && ((IN_LDPTAB[i].syncstep != INV_STEP) ||
                               (IN_SDPTAB[i].procstate != IN_DEAD) || 
                               (IN_LDPTAB[i].permstate != IN_TEMPPROC))) ||
         (strcmp(IN_MSGHQNM, pcreate->msgh_name) == 0) ||
         (strcmp("MSGH", pcreate->msgh_name) == 0))	{
       //INIT_ERROR(("\"INprocCreateType\" message received with duplicate\n\t(or reserved) proc. name \"%s\"",pcreate->msgh_name));
       printf("\"INprocCreateType\" message received with duplicate\n\t(or reserved) proc. name \"%s\"\n",
              pcreate->msgh_name);
			
       class INlprocCreateFail pfail(pcreate->msgh_name, INDUPMSGNM);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, INDUPMSGNM, IN_SOFT, INIT_INDEX);
       }
       return;
     }

     if(i < IN_SNPRCMX){
       indx = i;
       avail_slot = TRUE;
     }

     if (avail_slot == FALSE) {
       //INIT_ERROR(("\"INprocCreateType\" message received for proc. \"%s\"\n\tnot started because process table is full with %d procs.",pcreate->msgh_name, IN_SNPRCMX));
       printf("\"INprocCreateType\" message received for proc. \"%s\"\n\tnot "
              "started because process table is full with %d procs.\n",
              pcreate->msgh_name, IN_SNPRCMX);
			
       class INlprocCreateFail pfail(pcreate->msgh_name, INMAXPROCS);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, INMAXPROCS, IN_SOFT, INIT_INDEX);
       }
       return;
     }

     INinitptab(indx);
     strcpy(IN_LDPTAB[indx].proctag,pcreate->msgh_name);
     IN_LDPTAB[indx].proctag[IN_NAMEMX-1] = 0;  /* for audits */

     /* Verify path name */
     pcreate->full_path[IN_PATHNMMX-1] = (Char)0;

     strcpy(IN_LDPTAB[indx].pathname,pcreate->full_path);
     IN_LDPTAB[indx].pathname[IN_PATHNMMX-1] = (Char)0;

     /* Verify path name */
     pcreate->ofc_path[IN_OPATHNMMX-1] = (Char)0;

     strcpy(IN_LDPTAB[indx].ofc_pathname,pcreate->ofc_path);

     pcreate->ext_path[IN_EPATHNMMX-1] = (Char)0;

     strcpy(IN_LDPTAB[indx].ext_pathname,pcreate->ext_path);

     /* Make sure that at least the official path is valid,
     ** if it is do not check for existence of the executeable
     */ 
     if (pcreate->ofc_path[0] != 0){ 
       if(INgetpath(IN_LDPTAB[indx].ofc_pathname, TRUE) != GLsuccess) {
         //INIT_ERROR(("Failed to find executable file:\n\t\"%s\"", pcreate->ofc_path));
         printf("Failed to find executable file:\n\t\"%s\"\n", pcreate->ofc_path);
         class INlprocCreateFail pfail(pcreate->msgh_name, INNOTEXECUT);
         pfail.send(orig_qid, INmsgqid, 0);
         if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
           INescalate(SN_LV4, INNOTEXECUT, IN_SOFT, INIT_INDEX);
         }
         return;
       }
       /* Verify that directory exists */
       if(INgetpath(IN_LDPTAB[indx].pathname, TRUE, TRUE) != GLsuccess) {
         //INIT_ERROR(("Failed to find directory for file :\n\t\"%s\"", pcreate->full_path));
         printf("Failed to find directory for file :\n\t\"%s\"\n",
                pcreate->full_path);
         class INlprocCreateFail pfail(pcreate->msgh_name, INNOTEXECUT);
         pfail.send(orig_qid, INmsgqid, 0);
         if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
           INescalate(SN_LV4, INNOTEXECUT, IN_SOFT, INIT_INDEX);
         }
         return;
       }
     } else if(pcreate->ext_path[0] != 0 && pcreate->third_party != TRUE){
       if(INgetpath(IN_LDPTAB[indx].ext_pathname, TRUE) != GLsuccess) {
         //INIT_ERROR(("Failed to find executable file:\n\t\"%s\"", pcreate->ext_path));
         printf("Failed to find executable file:\n\t\"%s\"",
                pcreate->ext_path);
         class INlprocCreateFail pfail(pcreate->msgh_name, INNOTEXECUT);
         pfail.send(orig_qid, INmsgqid, 0);
         if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
           INescalate(SN_LV4, INNOTEXECUT, IN_SOFT, INIT_INDEX);
         }
         return;
       }
     } else  if (INgetpath(IN_LDPTAB[indx].pathname, TRUE) != GLsuccess) {
       //INIT_ERROR(("Failed to find executable file:\n\t\"%s\"", pcreate->full_path));
       printf("Failed to find executable file:\n\t\"%s\"\n",
              pcreate->full_path);
       class INlprocCreateFail pfail(pcreate->msgh_name, INNOTEXECUT);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, INNOTEXECUT, IN_SOFT, INIT_INDEX);
       }
       return;
     }

     /* Verify proc's priority */
     U_char max_prio = IN_MAXPRIO - 1;
     if (pcreate->priority > max_prio) {
       //INIT_ERROR(("Invalid priority %d in \"INprocCreate\" message greater than %d\n\tprocess name \"%s\"", pcreate->priority, max_prio, pcreate->msgh_name));
       printf("Invalid priority %d in \"INprocCreate\" message greater than %d\n\tprocess name \"%s\"\n",
              pcreate->priority, max_prio, pcreate->msgh_name);
       class INlprocCreateFail pfail(pcreate->msgh_name, ININVPRIO);
       pfail.send(orig_qid, INmsgqid, 0);
       return;
     } else if(pcreate->priority == 0){
       /* 0 is set in default constructor so make it default.
       ** This prohibits temporary processes from being set
       ** to maximum priority.
       */
       pcreate->priority = IN_procdata->default_priority;
     }

     IN_LDPTAB[indx].priority = pcreate->priority;

     /* Make sure the UID is valid */
     if (pcreate->uid < 0) {
       //INIT_ERROR(("\"INprocCreate\" message for process name \"%s\"\n\tincluded invalid PID %d", pcreate->msgh_name, pcreate->uid));
       printf("\"INprocCreate\" message for process name \"%s\"\n\tincluded invalid PID %d\n",
              pcreate->msgh_name, pcreate->uid);
       class INlprocCreateFail pfail(pcreate->msgh_name, ININVUID);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, ININVUID, IN_SOFT, INIT_INDEX);
       }
       return;
     }
     IN_LDPTAB[indx].uid = pcreate->uid;
     IN_LDPTAB[indx].group_id = pcreate->group_id;

     if (pcreate->inh_restart != TRUE && 
         ((pcreate->rstrt_intvl < IN_MIN_RESTART && pcreate->rstrt_intvl != 0) ||
          (pcreate->rstrt_max < IN_MIN_RESTART_THRESHOLD && pcreate->rstrt_max != 0))) { 
       //INIT_ERROR(("Invalid restart parameters in \"INprocCreate\" message \n\tprocess name \"%s\"", pcreate->msgh_name));
       printf("Invalid restart parameters in \"INprocCreate\" message \n\tprocess name \"%s\"\n",
              pcreate->msgh_name);
       class INlprocCreateFail pfail(pcreate->msgh_name, ININVPARM);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, ININVPARM, IN_SOFT, INIT_INDEX);
       }
       return;
     }

     /* Store restart intervals */
     IN_LDPTAB[indx].rstrt_intvl = pcreate->rstrt_intvl;
     IN_LDPTAB[indx].rstrt_max = pcreate->rstrt_max;

     if (pcreate->inh_restart == TRUE) {
       IN_LDPTAB[indx].startstate = IN_INHRESTART;
     }
     else {
       IN_LDPTAB[indx].startstate = IN_ALWRESTART;
       /* Do not allow 0 restart interval if restart
       ** is allowed.
       */
       if(pcreate->rstrt_intvl == 0){
         IN_LDPTAB[indx].rstrt_intvl = IN_MIN_RESTART;
       }
     }

     if (pcreate->inh_softchk == TRUE) {
       IN_LDPTAB[indx].softchk = IN_INHSOFTCHK;
     }
     else {
       IN_LDPTAB[indx].softchk = IN_ALWSOFTCHK;
     }

     if(pcreate->proc_cat >= IN_MAX_CAT){
       //INIT_ERROR(("Invalid process category in \"INprocCreate\" message \n\tprocess name \"%s\"", pcreate->msgh_name));
       printf("Invalid process category in \"INprocCreate\" message \n\tprocess name \"%s\"\n",
              pcreate->msgh_name);
       class INlprocCreateFail pfail(pcreate->msgh_name, ININVPARM);
       pfail.send(orig_qid, INmsgqid, 0);
       INinitptab(indx);
       return;
     }

     IN_LDPTAB[indx].proc_category = pcreate->proc_cat;
     IN_LDPTAB[indx].crerror_inh = pcreate->crerror_inh;

     if(pcreate->error_threshold < IN_MIN_ERROR_THRESHOLD ||
        pcreate->error_dec_rate < IN_MIN_ERROR_DEC_RATE ||
        (pcreate->run_lvl > IN_LDSTATE.run_lvl &&
         pcreate->run_lvl != 255) ||
        (pcreate->sanity_tmr < IN_MIN_SANITY && 
         pcreate->sanity_tmr != 0)  ||
        pcreate->init_complete_timer < IN_MIN_INITCMPL ||
        pcreate->procinit_timer < IN_MIN_PROCCMPL ||
        pcreate->global_queue_timer < IN_MIN_PROCCMPL ||
        pcreate->lv3_timer < IN_MIN_PROCCMPL ||
        pcreate->create_timer < IN_MIN_PROCCMPL ||
        (pcreate->q_size < MHmsgSz &&
         pcreate->q_size != -1 ) ){
       //INIT_ERROR(("Invalid error threshold or init timer in \"INprocCreate\" message \n\tprocess name \"%s\" resetting to default", pcreate->msgh_name));
       printf("Invalid error threshold or init timer in \"INprocCreate\" message "
              "\n\tprocess name \"%s\" resetting to default", pcreate->msgh_name);
     }

     IN_LDPTAB[indx].peg_intvl = 
        (pcreate->sanity_tmr == 0 ? 0 : (pcreate->sanity_tmr < IN_MIN_SANITY ? IN_MIN_SANITY : (pcreate->sanity_tmr != MAXSHORT ? pcreate->sanity_tmr : IN_procdata->default_sanity_timer)));

     IN_LDPTAB[indx].error_threshold = 
        (pcreate->error_threshold < IN_MIN_ERROR_THRESHOLD ? IN_MIN_ERROR_THRESHOLD : (pcreate->error_threshold != MAXSHORT ? pcreate->error_threshold : IN_procdata->default_error_threshold));

     IN_LDPTAB[indx].error_dec_rate = 
        (pcreate->error_dec_rate < IN_MIN_ERROR_DEC_RATE ? IN_MIN_ERROR_DEC_RATE : (pcreate->error_dec_rate != MAXSHORT ? pcreate->error_dec_rate : IN_procdata->default_error_dec_rate));

     IN_LDPTAB[indx].init_complete_timer = 
        (pcreate->init_complete_timer < IN_MIN_INITCMPL ? IN_MIN_INITCMPL : (pcreate->init_complete_timer != MAXSHORT ? pcreate->init_complete_timer : IN_procdata->default_init_complete_timer));

     IN_LDPTAB[indx].procinit_timer = 
        (pcreate->procinit_timer < IN_MIN_PROCCMPL ? IN_MIN_PROCCMPL : (pcreate->procinit_timer != MAXSHORT ? pcreate->procinit_timer : IN_procdata->default_procinit_timer));

     IN_LDPTAB[indx].lv3_timer = 
        (pcreate->lv3_timer < IN_MIN_PROCCMPL ? IN_MIN_PROCCMPL : (pcreate->lv3_timer != MAXSHORT ? pcreate->lv3_timer : IN_procdata->default_lv3_timer));

     IN_LDPTAB[indx].global_queue_timer = 
        (pcreate->global_queue_timer < IN_MIN_PROCCMPL ? IN_MIN_PROCCMPL : (pcreate->global_queue_timer != MAXSHORT ? pcreate->global_queue_timer : IN_procdata->default_global_queue_timer));
		
     if(pcreate->on_active == -1){
       pcreate->on_active = FALSE;
     }
     if(pcreate->on_active != TRUE){
       pcreate->on_active = FALSE;
     }

     if(pcreate->on_active == TRUE && INevent.onLeadCC()){
       // Create this temporary process on the Active as well
       char mateName[MHmaxNameLen+1];
       INgetMateCC(mateName);
       strcat(mateName, ":INIT");
       pcreate->send(mateName, INmsgqid, 0L);
     }

     IN_LDPTAB[indx].on_active = pcreate->on_active;

     if(pcreate->third_party != TRUE){
       pcreate->third_party = FALSE;
     }
     IN_LDPTAB[indx].third_party = pcreate->third_party;
     if(pcreate->oamleadonly != TRUE){
       pcreate->oamleadonly = FALSE;
     }
     IN_LDPTAB[indx].oamleadonly = pcreate->oamleadonly;

     if(pcreate->active_vhost_only != TRUE){
       pcreate->active_vhost_only = FALSE;
     }
     IN_LDPTAB[indx].active_vhost_only = pcreate->active_vhost_only;

     IN_LDPTAB[indx].create_timer = 
        (pcreate->create_timer < IN_MIN_PROCCMPL ? IN_MIN_PROCCMPL : (pcreate->create_timer != MAXSHORT ? pcreate->create_timer : IN_procdata->default_create_timer * 15));

     if(pcreate->q_size == -1){
       IN_LDPTAB[indx].q_size = IN_procdata->default_q_size;
     } else if(pcreate->q_size < MHmsgSz){
       IN_LDPTAB[indx].q_size = MHmsgSz;
     } else {
       IN_LDPTAB[indx].q_size = pcreate->q_size;
     }

     IN_LDPTAB[indx].run_lvl = 
        (pcreate->run_lvl >= IN_LDSTATE.run_lvl ? IN_LDSTATE.run_lvl : pcreate->run_lvl);

     if(pcreate->brevity_low < 0){
       IN_LDPTAB[indx].brevity_low = IN_procdata->default_brevity_low;
     } else {
       IN_LDPTAB[indx].brevity_low = pcreate->brevity_low;
     }

     if(pcreate->brevity_high < 0){
       IN_LDPTAB[indx].brevity_high = IN_procdata->default_brevity_high;
     } else {
       IN_LDPTAB[indx].brevity_high = pcreate->brevity_high;
     }

     if(pcreate->brevity_interval < 0){
       IN_LDPTAB[indx].brevity_interval = IN_procdata->default_brevity_interval;
     } else {
       IN_LDPTAB[indx].brevity_interval = pcreate->brevity_interval;
     }

     if(pcreate->msg_limit < 0){
       IN_LDPTAB[indx].msg_limit = IN_procdata->default_msg_limit;
     } else {
       IN_LDPTAB[indx].msg_limit = pcreate->msg_limit;
     }

     if(pcreate->ps != -1){
       /* Check to see if this ps is defined	*/
       if(pcreate->ps < 0 || pcreate->ps >= INmaxPsets || IN_LDPSET[pcreate->ps] < 0){
         class INlprocCreateFail pfail(pcreate->msgh_name, ININVPSET);
         pfail.send(orig_qid, INmsgqid, 0);
         INinitptab(indx);
         return;
       }
       IN_LDPTAB[indx].ps = pcreate->ps;
     }

     IN_LDPTAB[indx].isRT = pcreate->isRT;

     int	j;
     /* Check the msgh_qid for uniqness 			*/
     if(pcreate->msgh_qid == INfixQ){
       int	q;
       for(q = MHmaxQid - 1; q >= MHminTempProc; q--){
         if(!INqInUse[q]){
           INqInUse[q] = TRUE;
           pcreate->msgh_qid = q;
           break;
         }
       }

       if(q < MHminTempProc){
         //CRDEBUG_PRINT(0x1, ("No free queues available for temporary processes"));
         printf("No free queues available for temporary processes\n");
         class INlprocCreateFail pfail(pcreate->msgh_name, INMAXPROCS);
         pfail.send(orig_qid, INmsgqid, 0);
         INinitptab(indx);
         return;
       }
     } else if(pcreate->msgh_qid > 0){
       /* for out of range value		*/
       if(pcreate->msgh_qid < MHminTempProc || 
          pcreate->msgh_qid >= MHmaxQid ||
          (INqInUse[pcreate->msgh_qid] && IN_LDPTAB[indx].msgh_qid != pcreate->msgh_qid)){
         //INIT_ERROR(("Invalid msgh_qid %d in \"INprocCreate\" message \n\tprocess name \"%s\"", pcreate->msgh_qid,pcreate->msgh_name));
         printf("Invalid msgh_qid %d in \"INprocCreate\" message \n\tprocess name \"%s\"\n",
                pcreate->msgh_qid,pcreate->msgh_name);
         class INlprocCreateFail pfail(pcreate->msgh_name, ININVPARM);
         pfail.send(orig_qid, INmsgqid, 0);
         if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
           INescalate(SN_LV4, ININVPARM, IN_SOFT, INIT_INDEX);
         }
         return;
       }

       for(j = 0; j < IN_SNPRCMX; j++){
         if(pcreate->msgh_qid == IN_LDPTAB[j].msgh_qid && j != i){
           //INIT_ERROR(("Invalid msgh_qid %d in \"INprocCreate\" message \n\tprocess name \"%s\"", pcreate->msgh_qid,pcreate->msgh_name));
           printf("Invalid msgh_qid %d in \"INprocCreate\" message \n\tprocess name \"%s\"\n",
                  pcreate->msgh_qid,pcreate->msgh_name);
           class INlprocCreateFail pfail(pcreate->msgh_name, ININVPARM);
           pfail.send(orig_qid, INmsgqid, 0);
           if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
             INescalate(SN_LV4, ININVPARM, IN_SOFT, INIT_INDEX);
           }
           return;
         }
       }
       INqInUse[pcreate->msgh_qid] = TRUE;
     }

     IN_LDPTAB[indx].msgh_qid = pcreate->msgh_qid;
     IN_LDPTAB[indx].print_progress = pcreate->print_progress;

		
     /*
      * We've validated the data in the "INprocCreate" message -
      * now set the process's state appropriately and create
      * it.  Note that this is treated as a "process restart"
      * even though the process is being started for the first
      * time.
      */
     IN_LDPTAB[indx].permstate = IN_TEMPPROC;
     IN_LDPTAB[indx].syncstep = INV_STEP;
     IN_SDPTAB[indx].procstate = IN_DEAD;
     ret = ININVPARM;
     SN_LVL	sn_lvl;
     if(INevent.onLeadCC()){
       sn_lvl = SN_LV5;
     } else {
       sn_lvl = SN_LV4;
     }
     if((INsetrstrt(sn_lvl,indx,IN_INIT) == GLfail) ||
        ((ret = INsync(indx,IN_READY)) != GLsuccess)){
       // Invalidate process entry
       INinitptab(indx);
       class INlprocCreateFail pfail(pcreate->msgh_name, ret);
       pfail.send(orig_qid, INmsgqid, 0);
       if(pcreate->proc_cat == IN_CP_CRITICAL || pcreate->proc_cat == IN_PSEUDO_CRITICAL){
         INescalate(SN_LV4, ret, IN_SOFT, INIT_INDEX);
       }
       return;
     }

     /*
      *  Send ACK message:
      */
     {	/* Make C++ happy */

       class INlprocCreateAck ackmsg(pcreate->msgh_name,
                                     IN_LDPTAB[indx].pid);
       (Void)ackmsg.send(orig_qid, INmsgqid, 0);
     }

     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SNCRTR),(POA_INF,"INrcvmsg(): \"INprocCreateType\" message received\n\t\"%s\" proc successfully created", IN_LDPTAB[indx].proctag));
     printf("INrcvmsg(): \"INprocCreateType\" message received\n\t\"%s\" proc successfully created\n",
            IN_LDPTAB[indx].proctag);
     return;
   }

case INprocUpdateTyp:
   {
     if (msgsize != sizeof(class INprocUpdate)) {
       /*
        * Note error and break out of processing for
        * this message...i.e. dump it.
        */
       //INIT_ERROR(("\"INprocUpdate\" message received with size %d, expected %d", msgsize, sizeof(class INprocUpdate)));
       printf("\"INprocUpdate\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INprocUpdate));
       return;
     }

     class INprocUpdate *pupdate = (class INprocUpdate *)msg;

     /* Guarantee that the process name is properly terminated */
     pupdate->msgh_name[IN_NAMEMX-1] = (Char)0;

     if(pupdate->bAll && INevent.onLeadCC() && env != MH_peerCluster){
       // Set restart on active as well
       char mateName[MHmaxNameLen+1];
       INgetMateCC(mateName);
       strcat(mateName, ":INIT");
       pupdate->send(mateName, INmsgqid, 0L);
     }

     if(pupdate->proc_cat >= IN_MAX_CAT){
       INlprocUpdateFail ufail(pupdate->msgh_name, ININVPARM);
       ufail.send(orig_qid, INmsgqid, 0);
       //INIT_ERROR(("Invalid process category in \"INprocUpdate\" message \n\tprocess name \"%s\"", pupdate->msgh_name));
       printf("Invalid process category in \"INprocUpdate\" message \n\tprocess name \"%s\"",
              pupdate->msgh_name);
       return;
     }

     if((i = INfindproc(pupdate->msgh_name)) < IN_SNPRCMX){	
       if(pupdate->proc_cat > IN_MAX_CAT){
         INlprocUpdateFail ufail(pupdate->msgh_name, ININVPARM);
         ufail.send(orig_qid, INmsgqid, 0);
         //INIT_ERROR(("Invalid process category in \"INprocUpdate\" message \n\tprocess name \"%s\"", pupdate->msgh_name));
         printf("Invalid process category in \"INprocUpdate\" message \n\tprocess name \"%s\"",
                pupdate->msgh_name);
         return;
       } else if(pupdate->proc_cat < IN_MAX_CAT){
         /* if proc_cat == IN_MAX_CAT leave unchanged */
         IN_LDPTAB[i].proc_category = pupdate->proc_cat;
       }
       if (pupdate->inh_restart == TRUE) {
         IN_LDPTAB[i].startstate = IN_INHRESTART;
       } else {
         IN_LDPTAB[i].startstate = IN_ALWRESTART;
       }
       /* -1 means do not change the on_active value */
       if(pupdate->on_active != -1){
         IN_LDPTAB[i].on_active = pupdate->on_active;
       }
       INlprocUpdateAck uackmsg(pupdate->msgh_name);
       (Void)uackmsg.send(orig_qid, INmsgqid, 0);
     } else {
       INlprocUpdateFail ufail(pupdate->msgh_name, INNOPROC);
       ufail.send(orig_qid, INmsgqid, 0);
       //INIT_ERROR(("\"INprocUpdate\" message for non existent process %s", pupdate->msgh_name));
       printf("\"INprocUpdate\" message for non existent process %s\n",
              pupdate->msgh_name);
     }

     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_SNCRTR),(POA_INF,"INrcvmsg(): \"INprocUpdateType\" message received\n\t\"%s\" proc successfully updated", IN_LDPTAB[i].proctag));
     printf("INrcvmsg(): \"INprocUpdateType\" message received\n\t\"%s\" proc "
            "successfully updated\n", IN_LDPTAB[i].proctag);

     return;
   }

//case INinitSCNTyp:
//   {
//     if (msgsize != sizeof(class INinitSCN)) {
//       /*
//        * Note error and return
//        * ...i.e. dump this message
//        */
//       //INIT_ERROR(("\"INinitSCN\" message received with size %d, expected %d", msgsize, sizeof(class INinitSCN)));
//       printf("\"INinitSCN\" message received with size %d, expected %d",
//              msgsize, sizeof(class INinitSCN));
//       return;
//     }
//
//     class INinitSCN *initmsg = (class INinitSCN *)msg;
//
//     if(initmsg->sn_lvl == IN_MAXSNLVL || initmsg->sn_lvl == SN_LV5){
//       Bool boot = TRUE;
//       if(initmsg->sn_lvl == SN_LV5){
//         boot = FALSE;
//       }
//       //CR_PRM(POA_INF, "REPT INIT RECEIVED EXTERNAL RESTART REQUEST, BOOT %d", boot);
//       printf("REPT INIT RECEIVED EXTERNAL RESTART REQUEST, BOOT %d\n",
//              boot);
//
//       // If this is pilot and not ucl, check the mate
//       if((INevent.isOAMLead() || INevent.isVhostActive()) && initmsg->ucl != TRUE){
//         long buffer[(MHmsgSz/sizeof(long)) + 1];
//         Long	msgSz = MHmsgSz;
//         // Check the status of the mate
//         INinitdataReq req_msg("", IN_OP_STATUS_DATA);
//         if(req_msg.send(INvhostMateName, INmsgqid, 0) != GLsuccess){
//           class INlinitSCNFail fmsg(INNOSTDBY, initmsg->sn_lvl, NULL);
//           (Void)fmsg.send(orig_qid, INmsgqid, 0);
//           return;
//         }
//         if(INevent.receive(INmsgqid, (char*)buffer, msgSz, MHinitPtyp, 1000) != GLsuccess){
//           class INlinitSCNFail fmsg(INNOSTDBY, initmsg->sn_lvl, NULL);
//           (Void)fmsg.send(orig_qid, INmsgqid, 0);
//           return;
//         }
//         INsystemdata* status = (INsystemdata*)(((INinitdataResp*)buffer)->data);
//         if((status->initstate != IN_NOINIT &&
//             status->initstate != IN_CUINTVL) ||
//		  			status->mystate != S_LEADACT){
//           class INlinitSCNFail fmsg(INNOSTDBY, initmsg->sn_lvl, NULL);
//           (Void)fmsg.send(orig_qid, INmsgqid, 0);
//           return;
//         }
//       }
//
//       class INlinitSCNAck ackmsg(initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)ackmsg.send(orig_qid, INmsgqid, 0);
//       INsysboot(0, boot, FALSE);
//       return;
//     }
//
//     if ((initmsg->sn_lvl < SN_LV0) || (initmsg->sn_lvl >= SN_LV5)) {
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_IREQTR),(POA_INF,"INrcvmsg(): INinitSCN request had an invalid recovery level %d",initmsg->sn_lvl));
//       printf("INrcvmsg(): INinitSCN request had an invalid recovery level %d",
//              initmsg->sn_lvl);
//
//       class INlinitSCNFail init_fail(INVSNLVL, initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)init_fail.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     if (initmsg->sn_lvl >= SN_LV2) {
//       /*
//        * System-wide initialization requested, return
//        * an acknowledgement, wait a "bit" to allow the
//        * originating CEP time to output the results of
//        * the "INIT:SCN" command and then start the
//        * system-wide reset:
//        */
//       class INlinitSCNAck init_ack(initmsg->sn_lvl, "");
//       (Void)init_ack.send(orig_qid, INmsgqid, 0);
//
//       IN_SLEEP(3);
//       INsysreset(initmsg->sn_lvl, 0, IN_MANUAL, INIT_INDEX);
//       return;
//     }
//
//     /*
//      * A single-process reset has been requested, verify that
//      * the MSGH name is valid:
//      * ...first, Guarantee that the string WILL end with a
//      * null character...
//      */
//     initmsg->msgh_name[IN_NAMEMX-1] = (Char)0;
//
//     if ((i = INfindproc(initmsg->msgh_name)) >= IN_SNPRCMX) {
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_PSYNCTR | IN_IREQTR),(POA_INF,"INrcvmsg(): \"INinitSCN\" message received with unknown proc. name %s",initmsg->msgh_name));
//       printf("INrcvmsg(): \"INinitSCN\" message received with unknown proc. name %s\n",
//              initmsg->msgh_name);
//       /* Couldn't find the process, send error return msg */
//       class INlinitSCNFail init_fail(INNOPROC, initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)init_fail.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /* Initialization of critical processes at level > 0 can only be 
//     ** performed if UCL option is used or unless they are already dead
//     ** They could be dead only if restart on them was previously inhibited
//     */
//
//     if((initmsg->ucl == FALSE) && 
//        (IN_LDPTAB[i].proc_category == IN_CP_CRITICAL ||
//         IN_LDPTAB[i].proc_category == IN_PSEUDO_CRITICAL) &&
//		    (IN_SDPTAB[i].procstate != IN_DEAD)){
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INinitSCN\" message rejected. Process \"%s\" is critical", IN_LDPTAB[i].proctag));
//       printf("INrcvmsg(): \"INinitSCN\" message rejected. Process \"%s\" is critical/",
//              IN_LDPTAB[i].proctag);
//       class INlinitSCNFail init_fail(INBLOCKED, initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)init_fail.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /*
//      * Now check to make sure the process's state is valid:
//      */
//     if (INreqinit(initmsg->sn_lvl,i,0,IN_MANUAL,"MANUAL ACTION") != GLsuccess){
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_PSYNCTR | IN_IREQTR),(POA_INF,"INrcvmsg(): \"INinitSCN\" message received for proc %s\n\tmessage ignored as proc. is in \"%s\" state, \"%s\" step", initmsg->msgh_name, IN_PROCSTNM(IN_SDPTAB[i].procstate), IN_SQSTEPNM(IN_SDPTAB[i].procstep) ));
//       printf("INrcvmsg(): \"INinitSCN\" message received for proc %s\n\tmessage "
//              "ignored as proc. is in \"%s\" state, \"%s\" step",
//              initmsg->msgh_name, IN_PROCSTNM(IN_SDPTAB[i].procstate),
//              IN_SQSTEPNM(IN_SDPTAB[i].procstep));
//       class INlinitSCNFail init_fail(INVPROCSTATE, initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)init_fail.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /*
//      * OK, send the "ACK" and then re-init. the process:
//      */
//     {	/* Make C++ happy */
//       class INlinitSCNAck ackmsg(initmsg->sn_lvl, initmsg->msgh_name);
//       (Void)ackmsg.send(orig_qid, INmsgqid, 0);
//     }
//
//     if (IN_VALIDPROC(i)) {
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_PSYNCTR | IN_IREQTR),(POA_INF,"INrcvmsg(): \"INinitSCN\" message caused INIT to re-init \"%s\"", initmsg->msgh_name));
//       printf("INrcvmsg(): \"INinitSCN\" message caused INIT to re-init \"%s\"\n",
//              initmsg->msgh_name);
//       return;
//     }
//
//     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSEQTR | IN_PSYNCTR | IN_IREQTR),(POA_INF,"INrcvmsg(): \"INinitSCN\" re-init caused \"%s\" to be removed from the system", initmsg->msgh_name));
//     printf("INrcvmsg(): \"INinitSCN\" re-init caused \"%s\" to be removed "
//            "from the system\n", initmsg->msgh_name);
//     return;
//   }

case INinitProcTyp:
   {
     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_IREQTR | IN_ERROR),(POA_INF,"INrcvmsg(): \"INinitProcTyp\" message received...not yet implemented!!!"));
     printf("INrcvmsg(): \"INinitProcTyp\" message received...not yet implemented!!!\n");
     return;
   }

case INsetRstrtTyp:
   {
     if (msgsize != sizeof(class INsetRstrt)) {
       /*
        * Note error and return
        * ...i.e. dump this message
        */
       //INIT_ERROR(("\"INsetRstrt\" message received with size %d, expected %d", msgsize, sizeof(class INsetRstrt)));
       printf("\"INsetRstrt\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INsetRstrt));
       return;
     }
     class INsetRstrt *rmsg = (class INsetRstrt *)msg;

     if(rmsg->bAll && INevent.onLeadCC() && env != MH_peerCluster){
       // Set restart on active as well
       char mateName[MHmaxNameLen+1];
       INgetMateCC(mateName);
       strcat(mateName, ":INIT");
       rmsg->send(mateName, INmsgqid, 0L);
     }

     /* Guarantee that the string WILL end with a null character */
     rmsg->msgh_name[IN_NAMEMX-1] = (Char)0;

     if ((i = INfindproc(rmsg->msgh_name)) >= IN_SNPRCMX) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" msg received with unknown proc. name %s",rmsg->msgh_name));
       printf("INrcvmsg(): \"INsetRstrt\" msg received with unknown proc. name %s",
              rmsg->msgh_name);
       /* Couldn't find the process, send error return msg */
       class INlsetRstrtFail fmsg(rmsg->msgh_name, INNOPROC);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }
	
     /* Restart can only be inhibited on critical processes only if
     ** UCL option is used, unless restart is already inhibited.
     */

     if((rmsg->inh_restart == TRUE) && (rmsg->ucl == FALSE) &&
        (IN_LDPTAB[i].startstate == IN_ALWRESTART) && 
        (IN_LDPTAB[i].proc_category == IN_CP_CRITICAL ||
         IN_LDPTAB[i].proc_category == IN_PSEUDO_CRITICAL)){
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" message rejected. Process \"%s\" is critical", IN_LDPTAB[i].proctag));
       printf("INrcvmsg(): \"INsetRstrt\" message rejected. Process \"%s\" is critical\n",
              IN_LDPTAB[i].proctag);
       class INlsetRstrtFail fmsg(rmsg->msgh_name, INBLOCKED);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     /*
      * Reject messages that try to inhibit restart on
      * updating procs.
      */
     if ((rmsg->inh_restart == TRUE) &&
         (IN_LDPTAB[i].updstate == UPD_PRESTART)) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" message rejected. Process \"%s\" updating", IN_LDPTAB[i].proctag));
       printf("INrcvmsg(): \"INsetRstrt\" message rejected. Process \"%s\" updating\n",
              IN_LDPTAB[i].proctag);
       class INlsetRstrtFail fmsg(rmsg->msgh_name, INDEADPROC);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     /*
      * Now check to make sure the process's state is valid:
      * ...we shouldn't be able to get here...
      */
     if ((IN_SDPTAB[i].procstate == IN_DEAD) &&
         (IN_LDPTAB[i].startstate == IN_INHRESTART) &&
         (IN_LDPTAB[i].permstate == IN_TEMPPROC)) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" message received for proc %s\n\tmessage ignored as proc. is scheduled to be removed from proc tables", rmsg->msgh_name));
       printf("INrcvmsg(): \"INsetRstrt\" message received for proc %s\n\tmessage "
              "ignored as proc. is scheduled to be removed from proc tables",
              rmsg->msgh_name);

       class INlsetRstrtFail fmsg(rmsg->msgh_name, INDEADPROC);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     IN_LDPTAB[i].startstate = (rmsg->inh_restart == TRUE) ?
        IN_INHRESTART : IN_ALWRESTART;

     {	/* Make C++ happy... */
       class INlsetRstrtAck amsg(rmsg->msgh_name, rmsg->inh_restart);

       (Void)amsg.send(orig_qid, INmsgqid, 0);
     }

     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" message set \"%s\"s\n\trestart state to %s",rmsg->msgh_name, ((rmsg->inh_restart == TRUE)?"IN_INHRESTART":"IN_ALWRESTART")));
     printf("INrcvmsg(): \"INsetRstrt\" message set \"%s\"s\n\trestart state to %s",
            rmsg->msgh_name,
            ((rmsg->inh_restart == TRUE)?"IN_INHRESTART":"IN_ALWRESTART"));

     if ((IN_SDPTAB[i].procstate == IN_DEAD) &&
         (IN_LDPTAB[i].startstate == IN_INHRESTART)) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" message received for (DEAD) proc %s\n\tmessage caused proc. to be removed from INIT's tables", rmsg->msgh_name));
       printf("INrcvmsg(): \"INsetRstrt\" message received for (DEAD) "
              "proc %s\n\tmessage caused proc. to be removed from INIT's tables",
              rmsg->msgh_name);
       IN_LDPTAB[i].syncstep = INV_STEP;
       /* Do not try to restart this process any more */
       IN_LDPTAB[i].next_rstrt = IN_NO_RESTART;
       INCLRTMR(INproctmr[i].sync_tmr);
     }
     return;
   }

case INsetSoftChkTyp:
   {
     if (msgsize != sizeof(class INsetSoftChk)) {
       /*
        * Note error and return
        * ...i.e. dump this message
        */
       //INIT_ERROR(("\"INsetSoftChk\" message received with size %d, expected %d", msgsize, sizeof(class INsetSoftChk)));
       printf("\"INsetSoftChk\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INsetSoftChk));
       return;
     }
     class INsetSoftChk *smsg = (class INsetSoftChk *)msg;
     IN_SOFTCHK softchk;

     softchk = (smsg->inh_softchk == TRUE) ?
        IN_INHSOFTCHK : IN_ALWSOFTCHK;

     /* Guarantee that the string WILL end with a null character */
     smsg->msgh_name[IN_NAMEMX-1] = (Char)0;

     /* Global software check inhibits are affected */
     if(strlen(smsg->msgh_name) == 0){
       // Clear the softcheck file
       if(softchk == IN_ALWSOFTCHK){
         if((unlink(INinhfile) < 0) && (errno != ENOENT)){
           //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetSoftChk\" could not unlink %s, errno %d",INinhfile,errno));
           printf("INrcvmsg(): \"INsetSoftChk\" could not unlink %s, errno %d\n",
                  INinhfile,errno);
           class INlsetSoftChkFail fmsg(smsg->msgh_name, INFILEOPFAIL);
           (Void)fmsg.send(orig_qid, INmsgqid, 0);
           return;
         }

         INCLRTMR( INsoftchktmr );  
         if(IN_LDALMSOFTCHK != POA_INF){
           //CR_X733PRM(POA_CLEAR, "SYSTEM SOFTCHK", qualityOfServiceAlarm,
           //           unspecifiedReason, NULL, ";202", "REPT INIT SYSTEM SOFTWARE CHECKS INHIBITED");
           printf("REPT INIT SYSTEM SOFTWARE CHECKS INHIBITED\n");
           IN_LDALMSOFTCHK = POA_INF;
         }
       } else {	// Inhibit softchk
         int	fd;

         if((fd = creat(INinhfile,0444)) < 0) {
           //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetSoftChk\" failed to create %s, errno %d",INinhfile,errno));
           printf("INrcvmsg(): \"INsetSoftChk\" failed to create %s, errno %d\n",
                  INinhfile,errno);
           class INlsetSoftChkFail fmsg(smsg->msgh_name, INFILEOPFAIL);
           (Void)fmsg.send(orig_qid, INmsgqid, 0);
           return;
         }

         INSETTMR(INsoftchktmr, INSOFTCHKTMR, INSOFTCHKTAG, TRUE );
         close(fd);
       }

       IN_LDSTATE.softchk = softchk;
       class INlsetSoftChkAck amsg(smsg->msgh_name, smsg->inh_softchk);
       (Void)amsg.send(orig_qid, INmsgqid, 0);
       //FTbladeStChgMsg	msg(IN_LDCURSTATE, IN_LDSTATE.initstate, IN_LDSTATE.softchk);
       //if(INvhostMate >= 0){
       //  msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
       //  if(INevent.isVhostActive()){
       //    msg.setVhostState(INactive);
       //  } else {
       //    msg.setVhostState(INstandby);
       //  }
       //}
       //msg.send();

       return;
     }

     if ((i = INfindproc(smsg->msgh_name)) >= IN_SNPRCMX) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INsetRstrt\" msg received with unknown proc. name %s",smsg->msgh_name));
       printf("INrcvmsg(): \"INsetRstrt\" msg received with unknown proc. name %s\n",
              smsg->msgh_name);
       /* Couldn't find the process, send error return msg */
       class INlsetSoftChkFail fmsg(smsg->msgh_name, INNOPROC);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     IN_LDPTAB[i].softchk = softchk;

     {
       class INlsetSoftChkAck amsg(smsg->msgh_name, smsg->inh_softchk);
       (Void)amsg.send(orig_qid, INmsgqid, 0);
     }
     return;
   }
case INkillProcTyp:
   {
     Bool bSigterm = FALSE;
     class INkillProc *killmsg = (class INkillProc *)msg;

     if (msgsize == sizeof(class INkillProc)) {
       /* Current version of INkillProc received */
       bSigterm = killmsg->bSigterm;
     } else {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INkillProc\" message received with msgsize %d != %d", msgsize, sizeof(class INkillProc)));
       printf("INrcvmsg(): \"INkillProc\" message received with msgsize %d != %d\n",
              msgsize, sizeof(class INkillProc));
     }

     /* Guarantee that the string WILL end with a null character */
     killmsg->msgh_name[IN_NAMEMX-1] = (Char)0;

     if(killmsg->bAll && INevent.onLeadCC() && env != MH_peerCluster){
       // Set restart on active as well
       char mateName[MHmaxNameLen+1];
       INgetMateCC(mateName);
       strcat(mateName, ":INIT");
       killmsg->send(mateName, INmsgqid, 0L);
     }
     if ((i = INfindproc(killmsg->msgh_name)) >= IN_SNPRCMX) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INkillProc\" message received with unknown proc. name %s",killmsg->msgh_name));
       printf("INrcvmsg(): \"INkillProc\" message received with unknown proc. name %s\n",
              killmsg->msgh_name);
       /* Couldn't find the process, send error return msg */
       class INlkillProcFail kpmsg(INNOPROC, killmsg->msgh_name);
       (Void)kpmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     if ((IN_LDPTAB[i].permstate != IN_TEMPPROC) && (IN_LDPTAB[i].updstate != UPD_PRESTART)) {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INkillProc\" message received for permanent proc %s ignored", killmsg->msgh_name));
       printf("INrcvmsg(): \"INkillProc\" message received for permanent proc %s ignored\n",
              killmsg->msgh_name);
       class INlkillProcFail fmsg(INNOTTEMP, killmsg->msgh_name);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     /*
      * OK, send the "ACK" and then kill the process so it stays
      * dead:
      */
     {	/* Make C++ happy */
       class INlkillProcAck ackmsg(killmsg->msgh_name);
       (Void)ackmsg.send(orig_qid, INmsgqid, 0);
     }

     /*
      * Don't inhibit procs in the UPD_PRESTART state
      */
     if (IN_LDPTAB[i].updstate != UPD_PRESTART) {
       IN_LDPTAB[i].startstate = IN_INHRESTART;
     }

     /* Kill the process and let the normal death of process code
     ** handle it.
     */
     if(IN_LDPTAB[i].pid > 1){
       if(bSigterm == TRUE){
         (void)INkill(i,SIGTERM);
       } else {
         (void)INkill(i,SIGKILL);
       }
       // Updeate the ecode in shared memory
       if(killmsg->send_pdeath == FALSE){
         IN_SDPTAB[i].ecode = IN_KILLPROC;
       }
     } else {
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INkillProc\" message invalid pid for \"%s\"", killmsg->msgh_name));
       printf("INrcvmsg(): \"INkillProc\" message invalid pid for \"%s\"\n",
              killmsg->msgh_name);
     }

     //INIT_DEBUG((IN_DEBUG | IN_MSGHTR | IN_RSTRTR),(POA_INF,"INrcvmsg(): \"INkillProc\" message caused INIT to kill \"%s\"", killmsg->msgh_name));
     printf("INrcvmsg(): \"INkillProc\" message caused INIT to kill \"%s\"\n",
            killmsg->msgh_name);
     return;
   }

case INswccTyp:
   {
     if (msgsize != sizeof(class INswcc)) {
       /*
        * Note error and return
        * ...i.e. dump this message
        */
       //INIT_ERROR(("\"INswcc\" message received with size %d, expected %d", msgsize, sizeof(class INswcc)));
       printf("\"INswcc\" message received with size %d, expected %d\n",
              msgsize, sizeof(class INswcc));
       return;
     }
     /* This functionality only supported in duplex mode
      */
     if(IN_LDSTATE.issimplex && INvhostMate < 0){
       class INlswccFail fmsg(INNOTSUPP);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }

     class INswcc *sw_msg = (class INswcc *)msg;
     // Check the status of the mate
     // if not ready, fail the request
     long buffer[(MHmsgSz/sizeof(long)) + 1];
     Long	msgSz = MHmsgSz;
     char mateName[(2*MHmaxNameLen)+1];
     if(INvhostMate >= 0){
       strcpy(mateName, INvhostMateName);
     } else {
       INgetMateCC(mateName);
       strcat(mateName, ":INIT");
     }
     INinitdataReq req_msg("", IN_OP_STATUS_DATA);
     if(req_msg.send(mateName, INmsgqid, 0) != GLsuccess){
       class INlswccFail fmsg(INNOSTDBY);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }
     if(INevent.receive(INmsgqid, (char*)buffer, msgSz, MHinitPtyp, 1000) != GLsuccess){
       class INlswccFail fmsg(INNOSTDBY);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }
     INsystemdata* status = (INsystemdata*)(((INinitdataResp*)buffer)->data);
     if((status->initstate != IN_NOINIT &&
         status->initstate != IN_CUINTVL) ||
        (INvhostMate < 0 && status->mystate != S_ACT)){
       class INlswccFail fmsg(INNOSTDBY);
       (Void)fmsg.send(orig_qid, INmsgqid, 0);
       return;
     }
     class INlswccAck amsg;
     (Void)amsg.send(orig_qid, INmsgqid, 0);
     (void)INlv4_count(TRUE);
     if(INvhostMate < 0){
       //CR_PRM(POA_MAN + POA_MAXALARM, "REPT INIT STARTING CC SWITCHOVER");
       printf("REPT INIT STARTING CC SWITCHOVER\n");
       INsysboot(GLsuccess);
     } else {
       //CR_PRM(POA_MAN + POA_MAXALARM, "REPT INIT STARTING PILOT SWITCHOVER");
       printf("REPT INIT STARTING PILOT SWITCHOVER\n");
       INsysboot(GLsuccess, TRUE);
     }
     return;
   }

//case CRdbCmdMsgTyp:
//   {
//     if (msgsize != sizeof(class CRdbCmdMsg)) {
//       /*
//        * Note error and return
//        * ...i.e. dump this message
//        */
//       //INIT_ERROR(("\"CRdbCmdMsgTyp\" message received with size %d, expected %d", msgsize, sizeof(class CRdbCmdMsg)));
//       printf("\"CRdbCmdMsgTyp\" message received with size %d, expected %d\n",
//              msgsize, sizeof(class CRdbCmdMsg));
//       return;
//     }
//
//     /*
//      * At this point, we need to grab INIT's trace bits and
//      * stuff them into the trace mask.
//      *
//      * For now, we have to define our own msg. structure
//      * to access the trace bits in the trace message
//      * as the "CRdbCmdMsg" class defines the bit map
//      * as as PRIVATE data member!  Hopefully we can
//      * get this changed to "protected" so we can derive
//      * a class which gets the bits is which we're
//      * interested:
//      */
//     typedef struct {
//       Bool isDebugOn;
//       unsigned char bitmap[CRnumTraceBytes];
//     } IN_TSTRUCT;
//		
//     CRdbCmdMsg *cmdmsg_p = (CRdbCmdMsg *)msg;
//     IN_TSTRUCT *trace_p = (IN_TSTRUCT *)&cmdmsg_p->isDebugOn;
//
//     IN_trace = trace_p->bitmap[(CRinit>>3)] +
//        (trace_p->bitmap[(CRinit>>3) + 1] << 8) +
//        (trace_p->bitmap[(CRinit>>3) + 2] << 16) +
//        (trace_p->bitmap[(CRinit>>3) + 3] << 24);
//
//     if (trace_p->isDebugOn == FALSE) {
//       IN_trace = 0;
//     }
//     IN_trace |= IN_ALWAYSTR;
//
//     ((CRdbCmdMsg *) msg)->unload();
//     return;
//   }
//
//#ifdef OLD_SU
//case SUapplyTyp:
//   {
//     if (msgsize != sizeof(class SUapplyMsg)) {
//       /*
//        * Note error and return
//        * ...i.e. dump this message
//        */
//       //INIT_ERROR(("\"SUapply\" message received with size %d, expected %d", msgsize, sizeof(class SUapplyMsg)));
//       printf("\"SUapply\" message received with size %d, expected %d\n",
//              msgsize, sizeof(class SUapplyMsg));
//       return;
//     }
//
//     class SUapplyMsg *applymsg = (class SUapplyMsg *)msg;
//
//     if (IN_LDSTATE.initstate != IN_NOINIT) {
//       //INIT_DEBUG((IN_DEBUG | IN_ERROR),(POA_INF,"INrcvmsg(): ignoring SUapply request while in system init. state %s",IN_STATENM(IN_LDSTATE.initstate)));
//       printf("INrcvmsg(): ignoring SUapply request while in system init. state %s\n",
//              IN_STATENM(IN_LDSTATE.initstate));
//       class SUapplyFail fmsg(SU_SYSINIT);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /* If INsufile file exists, that means the SU is in progress
//     ** so fail this SU
//     */
//
//     struct stat     stbuf;
//     if ((stat(INsufile, &stbuf) >= 0) && (stbuf.st_size > 0)) {
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg():%s exists",INsufile));
//       printf("INrcvmsg():%s exists\n",INsufile);
//       class SUapplyFail fmsg(SU_PREVUPD);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//		
//     /*
//      * Can only apply one set of changes at a time. So all
//      * processes must be in NO_UPD state before starting.
//      */
//     for (i=0; i<IN_SNPRCMX; i++) {
//       if (IN_INVPROC(i)) {
//         continue;
//       }
//
//       if (IN_LDPTAB[i].updstate != NO_UPD) {
//         /* Process already updating, report error */
//         //INIT_DEBUG((IN_DEBUG | IN_ERROR),(POA_INF,"INrcvmsg(): \"SUapply\" message rcv'd with proc %s in updating state", IN_LDPTAB[i].proctag));
//         printf("INrcvmsg(): \"SUapply\" message rcv'd with proc %s in updating state\n",
//                IN_LDPTAB[i].proctag);
//         class SUapplyFail fmsg(SU_PREVUPD);
//         (Void)fmsg.send(orig_qid, INmsgqid, 0);
//         return;
//       }
//       if (IN_LDPTAB[i].rstrt_cnt > 0 || IN_SDPTAB[i].procstate != IN_RUNNING) {
//         /* Process is in a restart interval or not running  */
//         //INIT_DEBUG((IN_DEBUG | IN_ERROR),(POA_INF,"INrcvmsg(): \"SUapply\" message rcv'd with proc %s in a restart interval", IN_LDPTAB[i].proctag));
//         printf("INrcvmsg(): \"SUapply\" message rcv'd with proc %s in a restart interval\n",
//                IN_LDPTAB[i].proctag);
//         class SUapplyFail fmsg(SU_PROCINIT);
//         (Void)fmsg.send(orig_qid, INmsgqid, 0);
//         return;
//       }
//     }
//
//     if((ret = INgetsudata(applymsg->proclist,TRUE)) != GLsuccess){
//       /* Problems found parsing proclist */
//       class SUapplyFail fmsg(ret);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /* Save the SU path in the INsufile */
//     int fd;
//     if((fd = open(INsufile,(O_WRONLY | O_CREAT | O_TRUNC),0660)) < 0){
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg(): failed to open %s, errno=%d",INsufile,errno));
//       printf("INrcvmsg(): failed to open %s, errno=%d\n",
//              INsufile,errno);
//       class SUapplyFail fmsg(SU_INITERROR);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     int wcount;
//     if((wcount = write(fd,applymsg->proclist,strlen(applymsg->proclist))) < 0 ||
//		    wcount != strlen(applymsg->proclist)){
//       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg(): failed to write %s, errno=%d",INsufile,errno));
//       printf("INrcvmsg(): failed to write %s, errno=%d\n",
//              INsufile,errno);
//       class SUapplyFail fmsg(SU_INITERROR);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       close(fd);
//       (void)unlink(INsufile);
//       return;
//     }
//
//     close(fd);
//
//     INsupresent = TRUE;
//
//
//     for (int c=0; c < SU_MAX_OBJ && INsudata[c].obj_path[0] != '\0'; c++) { 
//
//       for (i=0; i<IN_SNPRCMX; i++) { /* Find the process's index */
//         if (IN_VALIDPROC(i) &&
//				     (strcmp(IN_LDPTAB[i].pathname, INsudata[c].obj_path) == 0)){
//           /* Change UPD_STATE only if the process
//           ** actually has changed.
//           */
//           if(INsudata[c].changed == TRUE){
//             IN_LDPTAB[i].updstate = UPD_PRESTART;
//           }
//           IN_LDPTAB[i].startstate = IN_ALWRESTART;
//           IN_LDPTAB[i].syncstep = IN_BSU;
//           IN_SDPTAB[i].procstep = IN_BSU;
//         }
//       }
//     }
//
//
//     /* Record the queue id of the apply CEP */
//     IN_LDAQID = orig_qid;
//
//     /* Apply all the other files */
//     SN_LVL sn_lvl;
//     if(INmvsufiles(IN_SNPRCMX,sn_lvl,INSU_APPLY) != GLsuccess){
//       INautobkout(FALSE,FALSE);
//       return;
//     }
//
//     /* If INIT is part of the SU, move the INIT image and
//     ** cause it to exit.
//     */
//     if(INinit_su_idx >= 0){
//       if(INmvsuinit(INSU_APPLY) != GLsuccess){
//         INautobkout(FALSE,FALSE);
//         return;
//       }
//       /* Cause exit from INIT's main while loop */
//       IN_LDEXIT = TRUE;
//     }
//
//     //CR_PRM(POA_INF,"REPT INIT STARTING SU APPLY");
//     printf("REPT INIT STARTING SU APPLY\n");
//
//     /* Start the sequencing code */
//     INworkflg = TRUE;
//     INsettmr(INpolltmr,INPROCPOLL,(INITTAG | INPOLLTAG),TRUE,TRUE);
//     IN_LDSTATE.sync_run_lvl = 0;
//     INnext_rlvl();
//
//     return;
//   }
//
//case SUcommitTyp:
//   {
//
//     if (msgsize != sizeof(class SUcommitMsg)) {
//       /*
//        * Note error and return
//        * ...i.e. dump this message
//        */
//       //INIT_ERROR(("\"SUcommit\" message received with size %d, expected %d", msgsize, sizeof(class SUcommitMsg)));
//       printf("\"SUcommit\" message received with size %d, expected %d\n",
//              msgsize, sizeof(class SUcommitMsg));
//       return;
//     }
//
//     class SUcommitMsg *commitmsg = (class SUcommitMsg *)msg;
//
//     /* Do not accept commit if backout is in progress */
//     if(IN_LDBKOUT == TRUE){
//       class SUcommitFail fmsg(SU_BKOUT_INPROG);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     /* Do not accept commit if SU not present */
//     if(INsupresent == FALSE){
//       class SUcommitFail fmsg(SU_NO_SU);
//       (Void)fmsg.send(orig_qid, INmsgqid, 0);
//       return;
//     }
//
//     for (i=0; i<IN_SNPRCMX; i++) {
//       if(IN_INVPROC(i)){
//         continue;
//       }
//       if (IN_LDPTAB[i].updstate == UPD_PRESTART ||
//			     IN_SDPTAB[i].procstep != IN_STEADY) { 
//         /* SU not yet fully applied */
//         class SUcommitFail fmsg(SU_OLDVERS);
//         (Void)fmsg.send(orig_qid, INmsgqid, 0);
//         return;
//       }
//     }
//
//     SN_LVL sn_lvl;
//     ret = INmvsufiles(IN_SNPRCMX,sn_lvl,INSU_COMMIT);
//
//     for (i=0; i<IN_SNPRCMX; i++) {
//       IN_LDPTAB[i].updstate = NO_UPD;
//     }
//
//     if(INinit_su_idx >= 0){
//       if(INmvsuinit(INSU_COMMIT) == GLfail){
//         ret = GLfail;
//       }
//       INinit_su_idx = -1;
//     }
//
//
//     /* Remove INsufile */
//     if(unlink(INsufile) < 0){
//       //INIT_ERROR(("SUcommit: failed to remove %s, errno = %d ", INsufile,errno));
//       printf("SUcommit: failed to remove %s, errno = %d \n",
//              INsufile,errno);
//     }
//
//     INsupresent = FALSE;
//
//     if(ret == GLsuccess){
//       class SUcommitAck ackmsg;
//       (Void)ackmsg.send(orig_qid, INmsgqid, 0);
//       //CR_PRM(POA_INF,"REPT INIT COMPLETED SU COMMIT");
//       printf("REPT INIT COMPLETED SU COMMIT\n");
//     } else {
//       /* Return failure if failed to move any of the files */
//       class SUcommitFail failmsg(SU_FILE_SYS);
//       (Void)failmsg.send(orig_qid, INmsgqid, 0);
//     }
//     return;
//   }
//
//case SUbkoutTyp:
//   {
//     if (msgsize != sizeof(class SUbkoutMsg)) {
//       /*
//        * Note error and return
//        * ...i.e. dump this message
//        */
//       //INIT_ERROR(("\"SUbkout\" message received with size %d, expected %d", msgsize, sizeof(class SUbkoutMsg)));
//       printf("\"SUbkout\" message received with size %d, expected %d\n",
//              msgsize, sizeof(class SUbkoutMsg));
//       return;
//     }
//		
//     class SUbkoutMsg *bkoutmsg = (class SUbkoutMsg *)msg;
//
//     /* If no SU is in effect, simply send an ack msg and return */
//     if(INsupresent == FALSE){
//       SUbkoutAck bkackmsg;
//       (void)bkackmsg.send(orig_qid,INmsgqid,0);
//       return;
//     }
//
//     /* If a backout CEP is already waiting on a message do not 
//     ** accept any more backout requests.
//     */
//
//     if((IN_LDBQID != MHnullQ && IN_LDBKOUT == TRUE) ||
//        IN_LDBKPID != IN_FREEPID){
//       SUbkoutFail bkfailmsg(SU_BKOUT_INPROG);
//       (void)bkfailmsg.send(orig_qid,INmsgqid,0);
//       return;
//     }
//
//     /* Record other backout parameters 	*/
//     IN_LDBKUCL = bkoutmsg->ucl;
//
//     /* Record the queue id of the backout CEP */
//     IN_LDBQID = orig_qid;
//     /* TRUE implies manual backout,
//     ** FALSE means this is not system reset.
//     */
//     INautobkout(TRUE, FALSE);
//     return;
//   }
//#endif
//
case INoamLeadTyp:
   {
     char	oamLeadName[MHmaxNameLen + 1];
     Short	oamLead = INevent.Qid2Host(orig_qid);

     oamLeadName[0] = 0;
     INnoOamLeadMsg = 0;
     if(INevent.getOAMLead() == oamLead){
       return;
     } else if(oamLead == INmyPeerHostId){
       // This node is being promoted to active cluster blade
       // set it to OAM lead and start up all the processes
       // that are supposed to be on oam cluster lead
       INevent.setOamLead(orig_qid);
       INoamInitialize  oamInit;
       INevent.broadcast(INmsgqid, (char*)&oamInit, sizeof(oamInit));
       //CR_PRM(POA_INF, "REPT INIT TRANSITIONING OAM LEAD TO %s", INmyPeerHostName);
       printf("REPT INIT TRANSITIONING OAM LEAD TO %s\n", INmyPeerHostName);
       INSETTMR(INoamReadyTmr, INOAMREADYTMR, INOAMREADYTAG, FALSE);
       return;
     } else if(INevent.getOAMLead() != INmyPeerHostId){
       // OAM lead is either not initialized or the one that is
       // being set is not me.  Since I do not think I am oam lead
       // I do not care who else claims to be one, I just accept
       // the claim
       INevent.setOamLead(orig_qid);
       INevent.hostId2Name(oamLead, oamLeadName);	
       //CR_PRM(POA_INF, "REPT INIT SETTING OAM LEAD TO %s", oamLeadName);
       printf("REPT INIT SETTING OAM LEAD TO %s\n", oamLeadName);
       return;
     }
			
     // At this point there is a conflict, if my host id is less then
     // the claimer, then I do not obey, otherwise I reboot myself. 
     // Because of possible race conditions when shutting down previous lead
     // ignore the first conflict.
     if(oamLead < INmyPeerHostId){
       if((++INoamConflictCnt) <= INmaxOamConflict){
         //CR_PRM(POA_INF, "REPT INIT OAM LEAD CONFLICT CNT %d", INoamConflictCnt);
         printf("REPT INIT OAM LEAD CONFLICT CNT %d\n",
                INoamConflictCnt);
         return;
       }
       // Softchecks might help but most likely this machine will
       // panic as the other host will take over the disks
       if(IN_LDSTATE.softchk == IN_INHSOFTCHK){
         return;
       }
       INsysboot(IN_OAMLEADCONFLICT);
     }
		
     break;
   }
//case FToamReadyTyp:
//   {
//     //CR_PRM(POA_INF, "REPT INIT STARTING OAM LEAD PROCESSES");
//     printf("REPT INIT STARTING OAM LEAD PROCESSES\n");
//     INCLRTMR(INvhostReadyTmr);
//     // start up other processes
//     int	numProcs;
//     if((numProcs = INrdinls(FALSE, FALSE)) <  0){
//       //CR_PRM(POA_INF, "REPT INIT CORRUPTED INITLIST FAILED TO START OAM LEAD PROCESSES");
//       printf("REPT INIT CORRUPTED INITLIST FAILED TO START OAM LEAD PROCESSES\n");
//       INescalate(SN_LV5, INBADINITLIST, IN_SOFT, INIT_INDEX);
//       return;
//     } else if(numProcs == 0){
//       return;
//     } else {
//       IN_LDSTATE.sn_lvl = SN_LV5;
//       IN_LDSTATE.systep = IN_SYSINIT;
//       IN_LDSTATE.sync_run_lvl = 0;
//       INworkflg = TRUE;
//       INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);
//       // Initialize init history if failover
//       if(IN_LDSTATE.initstate != INITING){
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.run_lvl;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = IN_RUNLVL;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = 0;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = numProcs;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name[0] = 0;
//       }
//       IN_LDSTATE.initstate = INITING;
//       // Run failover startup scripts
//       if((ret = fork()) == 0){
//         static int type = INscriptsFailover;
//         //mutex_unlock(&CRlockVarible);
//         //CR_PRM(POA_INF, "REPT INIT RUNNING FAILOVER SCRIPTS");
//         printf("REPT INIT RUNNING FAILOVER SCRIPTS\n");
//         IN_LDSCRIPTSTATE = INscriptsRunning;
//         INrunScriptList(&type);
//         exit(0);
//       } else if(ret < 0){
//         //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO FORK FAILOVER PROCESS, RET %d", ret);
//         printf("REPT INIT ERROR FAILED TO FORK FAILOVER PROCESS, RET %d\n");
//         INescalate(SN_LV2, IN_SCRIPTSFAILED, IN_SOFT, INIT_INDEX);
//       }
//
//     }
//     INoamLead       oamLeadMsg;
//
//     oamLeadMsg.srcQue = INmsgqid;
//     MHmsgh.sendToAllHosts("INIT", (char*)&oamLeadMsg, sizeof(INoamLead), MH_scopeSystemOther);
//
//     const char* logfile = "/oam/snlog/oamleadlog";
//     // Log the OAM history message 
//     FILE*	fp;
//     if((fp = fopen(logfile, "a")) != NULL){
//       time_t clock;
//       time(&clock);
//       fprintf(fp, "%s IS OAM LEAD AT %s\n", INmyPeerHostName, ctime(&clock));
//       fclose(fp);
//     }
//     break;
//   }
//case FTbladeQueryTyp:
//   {
//     FTbladeStChgMsg	msg(IN_LDCURSTATE, IN_LDSTATE.initstate, IN_LDSTATE.softchk);
//     if(INvhostMate >= 0){
//       if(INevent.isVhostActive()){
//         msg.setVhostState(INactive);
//       } else {
//         msg.setVhostState(INstandby);
//       }
//       msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
//     }
//     msg.send(TRUE, orig_qid);
//     break;
//   }
//case FTbladeStQryAckTyp:
//   {
//     extern int INsnstop;
//     class FTbladeStQryMsg *resp = (class FTbladeStQryMsg *)msg;
//     if(resp->getState() == S_OFFLINE || resp->getState() == S_UNAV){
//       INsnstop = TRUE;
//     }
//     break;
//   }
case INsetActiveVhostTyp:
   {
     if(INvhostMate < 0){
       //CR_PRM(POA_INF, "REPT INIT ERROR THIS NODE IS NOT A MEMBER OF VHOST PAIR, SRCQ %s", orig_qid.display());
       printf("REPT INIT ERROR THIS NODE IS NOT A MEMBER OF VHOST PAIR, SRCQ %s\n",
              orig_qid.display());
       break;
     }

     char	fromName[MHmaxNameLen + 1];
     if((ret = INevent.hostId2Name(INevent.Qid2Host(orig_qid), fromName)) != GLsuccess){
       //INIT_ERROR(("Could not get mate vhost name orig_qid %s, reval %d", orig_qid.display(), ret));
       printf("Could not get mate vhost name orig_qid %s, reval %d\n",
              orig_qid.display(), ret);
       break;
     }
     if(strcmp(fromName, IN_procdata->vhost[INvhostMate]) != 0){
       //CR_PRM(POA_INF, "REPT INIT ERROR SET VHOST MESSAGE FROM NAME %s DOES NOT MATCH VHOST MATE NAME %s", 
       //       fromName, IN_procdata->vhost[INvhostMate]);
       printf("REPT INIT ERROR SET VHOST MESSAGE FROM NAME %s DOES NOT MATCH VHOST MATE NAME %s", 
              fromName, IN_procdata->vhost[INvhostMate]);
       break;
     }

     if(!INevent.isVhostActive()){
       // If I am not an active vhost just accept it and peg heartbeat count
       if(INcanBeOamLead){
         if(INevent.getOAMLead() < 0){
           //CR_PRM(POA_INF, "REPT INIT SETTING OAM LEAD TO %s", fromName);
           printf("REPT INIT SETTING OAM LEAD TO %s\n", fromName);
         }
         INevent.setOamLead(orig_qid);
       } else {
         if(INevent.getActiveVhost() < 0){
           //CR_PRM(POA_INF, "REPT INIT SETTING ACTIVE VHOST TO %s", fromName);
           printf("REPT INIT SETTING ACTIVE VHOST TO %s\n", fromName);
         }
       }
       INevent.setActiveVhost(orig_qid);
       INCLRTMR(INsetActiveVhostTmr);
       INSETTMR(INsetActiveVhostTmr, IN_procdata->vhostfailover_time, INSETACTIVEVHOSTTAG, TRUE);
     } else if(INevent.Qid2Host(orig_qid) < INmyPeerHostId){
       //CR_PRM(POA_MAJ, "REPT INIT ACTIVE VHOST CONFLICT, REBOOTING");
       printf("REPT INIT ACTIVE VHOST CONFLICT, REBOOTING\n");
       if(IN_LDSTATE.softchk == IN_INHSOFTCHK){
         return;
       }
       INsysboot(IN_ACTIVEVHOSTCONFLICT);
     } else {
       // Reset the other node
       //CR_PRM(POA_MAJ, "REPT INIT ACTIVE VHOST CONFLICT, RESETTING %s",IN_procdata->vhost[INvhostMate]);
       printf("REPT INIT ACTIVE VHOST CONFLICT, RESETTING %s\n",
              IN_procdata->vhost[INvhostMate]);
       //FThwmMachineReset(IN_procdata->vhost[INvhostMate]);
     }
     break;
   }
case INvhostInitializeTyp:
   {
     // This message will be sent from the vhost if it knows it is
     // going down.   Promote this one to active and do not reset the
     // other node
     if(INvhostMate < 0){
       //CR_PRM(POA_INF, "REPT INIT ERROR THIS NODE IS NOT A MEMBER OF VHOST PAIR, SRCQ %s", orig_qid.display());
       printf("REPT INIT ERROR THIS NODE IS NOT A MEMBER OF VHOST PAIR, SRCQ %s\n",
              orig_qid.display());
       break;
     }

     INvhostInitialize* vhostInitMsg = (INvhostInitialize*) msg;

     if(strcmp(vhostInitMsg->fromHost, IN_procdata->vhost[INvhostMate]) != 0){
       //CR_PRM(POA_INF, "REPT INIT ERROR SET VHOST INITIALIZE FROM NAME %s DOES NOT MATCH VHOST MATE NAME %s", 
       //       vhostInitMsg->fromHost, IN_procdata->vhost[INvhostMate]);
       printf("REPT INIT ERROR SET VHOST INITIALIZE FROM NAME %s DOES NOT MATCH VHOST MATE NAME %s\n", 
              vhostInitMsg->fromHost, IN_procdata->vhost[INvhostMate]);
       break;
     }
		
     if(!INevent.isVhostActive()){
       if(INevent.getActiveVhost() >= 0){
         INdidFailover = TRUE;
       }
       INevent.setActiveVhost(INmsgqid);
       INCLRTMR(INsetActiveVhostTmr);
       if(INcanBeOamLead){
         //CR_PRM(POA_INF, "REPT INIT TRANSITIONING OAM LEAD TO ACTIVE");
         printf("REPT INIT TRANSITIONING OAM LEAD TO ACTIVE\n");
         INevent.setOamLead(INmsgqid);
         INoamInitialize  oamInit;  
         INevent.broadcast(INmsgqid, (char*)&oamInit, sizeof(oamInit));
       } else {
         //CR_PRM(POA_INF, "REPT INIT TRANSTIONING VHOST TO ACTIVE");
         printf("REPT INIT TRANSTIONING VHOST TO ACTIVE\n");
         INvhostInitialize   vhostInit("");
         INevent.broadcast(INmsgqid, (char*)&vhostInit, sizeof(vhostInit));
       }

       INSETTMR(INvhostReadyTmr, INVHOSTREADYTMR, INVHOSTREADYTAG, FALSE);
       // Send FT state change message
       //FTbladeStChgMsg msg(IN_LDCURSTATE, IN_LDSTATE.initstate, IN_LDSTATE.softchk);
       //msg.setVhostState(INactive);
       //msg.setVhostMate(IN_procdata->vhost[INvhostMate]);
       //msg.send();
     } else {
       // Spurious message, should the other node be reset ?
       //CR_PRM(POA_INF, "REPT INIT ERROR UNEXPECTED VHOST INITIALIZE FROM %s", IN_procdata->vhost[INvhostMate]);
       printf("REPT INIT ERROR UNEXPECTED VHOST INITIALIZE FROM %s\n",
              IN_procdata->vhost[INvhostMate]);
     }
     break;	
   }
//case FTvhostReadyTyp:
//   {
//     //CR_PRM(POA_INF, "REPT INIT STARTING ACTIVE VHOST PROCESSES");
//     printf("REPT INIT STARTING ACTIVE VHOST PROCESSES\n");
//     INCLRTMR(INvhostReadyTmr);
//     // start up other processes
//     int     numProcs;
//     if((numProcs = INrdinls(FALSE, FALSE)) <  0){
//       //CR_PRM(POA_INF, "REPT INIT CORRUPTED INITLIST FAILED TO START OAM LEAD PROCESSES");
//       printf("REPT INIT CORRUPTED INITLIST FAILED TO START OAM LEAD PROCESSES\n");
//       INescalate(SN_LV5, INBADINITLIST, IN_SOFT, INIT_INDEX);
//       return;
//     } else if(numProcs == 0){
//       return;
//     } else {
//       IN_LDSTATE.sn_lvl = SN_LV5;
//       IN_LDSTATE.systep = IN_SYSINIT;
//       IN_LDSTATE.sync_run_lvl = 0;
//       INworkflg = TRUE;
//       INsettmr(INpolltmr,INITPOLL,(INITTAG|INPOLLTAG), TRUE, TRUE);
//       // Initialize init history if failover
//       if(IN_LDSTATE.initstate != INITING){
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].prun_lvl = IN_LDSTATE.run_lvl;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].str_time = time((time_t *)0);
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].source = IN_RUNLVL;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].ecode = 0;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].num_procs = numProcs;
//         IN_LDINFO.init_data[IN_LDINFO.ld_indx].msgh_name[0] = 0;
//       }
//       IN_LDSTATE.initstate = INITING;
//       // Run failover startup scripts
//       if((ret = fork()) == 0){
//         static int type = INscriptsFailover;
//         //mutex_unlock(&CRlockVarible);
//         //CR_PRM(POA_INF, "REPT INIT RUNNING FAILOVER SCRIPTS");
//         printf("REPT INIT RUNNING FAILOVER SCRIPTS\n");
//         IN_LDSCRIPTSTATE = INscriptsRunning;
//         INrunScriptList(&type);
//         exit(0);
//       } else if(ret < 0){
//         //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO FORK FAILOVER PROCESS, RET %d", ret);
//         printf("REPT INIT ERROR FAILED TO FORK FAILOVER PROCESS, RET %d\n", ret);
//         INescalate(SN_LV2, IN_SCRIPTSFAILED, IN_SOFT, INIT_INDEX);
//       }
//     }
//     break;
//   }
case INsetLeadTyp:
   {
     if(IN_LDSTATE.issimplex){
       //INIT_ERROR(("Unexpected INsetLead message"));
       printf("Unexpected INsetLead message");
       return;
     }
     /* First check if the other side is booting	*/
     INsetLead*	pSetLead = (INsetLead*)msg;
     if(IN_LDCURSTATE == S_LEADACT){
       /* We have a problem, looks like both CCs believe
       ** that they are lead.  In cases of conflicts, always
       ** give preference to CC with even host id, reboot if we are odd,
       ** otherwise send a mesage back to the other INIT
       ** so that it reboots immediately.
       ** This of course assumes/requires that Active/Active CCs
       ** have even/odd host ids.  Preferably they should be in pairs,
       ** i.e. 10 and 11 or 12 and 13 etc.
       */
       if((INevent.Qid2Host(orig_qid) & 0x1) == 0x1){
         // reboot, even numbered hosts take precedence
         if(IN_LDSTATE.softchk == IN_INHSOFTCHK){
           return;
         }
         INsysboot(IN_LEADCONFLICT);
       } else {
         INsetLead	setLead;
         setLead.srcQue = INmsgqid;
         if((ret = INevent.send(orig_qid, (char*)&setLead, sizeof(setLead), 0L)) != GLsuccess){
           //INIT_ERROR(("Failed to send INsetLead orig_qid %s, ret %d", orig_qid.display(), ret));
           printf("Failed to send INsetLead orig_qid %s, ret %d\n",
                  orig_qid.display(), ret);
         }
       }
     } else if(IN_LDCURSTATE == S_ACT){
       /* Kill the setLead timer, just wait for INcheckLeadAck response
       ** before advancing initalization.
       */
       INCLRTMR(INsetLeadTmr);
     }
     break;
   }

case INcheckLeadTyp: 
   {
     if (msgsize != sizeof(INcheckLead)){
       //INIT_ERROR(("INcheckLead message received with size %d, expected %d", msgsize, sizeof(INcheckLead)));
       printf("INcheckLead message received with size %d, expected %d",
              msgsize, sizeof(INcheckLead));
       return;
     }
     int	nProcs = 0;
     int	indx;
     // If request from TS do not fill in temporary process info
     char	hname[MHmaxNameLen+1];
     if((ret = INevent.hostId2Name(INevent.Qid2Host(orig_qid), hname)) != GLsuccess){
       //INIT_ERROR(("INcheckLead, nonexistent host orig_qid %s", orig_qid.display()));
       printf("INcheckLead, nonexistent host orig_qid %s\n",
              orig_qid.display());
     }
     if(strncmp(hname, "ts", 2) == 0){
       indx = IN_SNPRCMX;
     } else {
       indx = 0;
     }
     // See if we are lead and if initialization is completed.
     // If true, send only the node status info
     if(IN_LDCURSTATE == S_LEADACT){  
       // if duplex without RCC, reply that we are LEAD
       if(!IN_LDSTATE.issimplex){
         INsetLead	setLead;
         setLead.srcQue = INmsgqid;
         if((ret = INevent.send(orig_qid, (char*)&setLead, sizeof(setLead), 0L)) != GLsuccess){
           //INIT_ERROR(("Failed to send INsetLead orig_qid %s, ret %d", orig_qid.display(), ret));
           printf("Failed to send INsetLead orig_qid %s, ret %d\n", orig_qid.display(), ret);
         }
       }
       // 
       if(IN_LDSTATE.initstate == IN_NOINIT ||
          IN_LDSTATE.initstate == IN_CUINTVL){
         INcheckLeadAck	ack;
         ack.srcQue = INmsgqid;
         // Include all temporary processes that should be
         // running on active
         for(; nProcs < INmaxProcInfo  && indx < IN_SNPRCMX; indx++){
           if(IN_INVPROC(indx)){
             continue;
           }
           if(IN_LDPTAB[indx].permstate != IN_TEMPPROC ||
				   		IN_LDPTAB[indx].on_active == FALSE){
             continue;
           }
           INprocInfo * p = &ack.m_ProcInfo[nProcs];
           strcpy(p->msgh_name, IN_LDPTAB[indx].proctag);
           strcpy(p->full_path, IN_LDPTAB[indx].pathname);
           strcpy(p->ofc_path, IN_LDPTAB[indx].ofc_pathname);
           strcpy(p->ext_path, IN_LDPTAB[indx].ext_pathname);
           p->priority = IN_LDPTAB[indx].priority;
           p->uid = IN_LDPTAB[indx].uid;
           p->group_id = IN_LDPTAB[indx].group_id;
           p->sanity_tmr = IN_LDPTAB[indx].peg_intvl;
           p->rstrt_intvl = IN_LDPTAB[indx].rstrt_intvl;
           p->rstrt_max = IN_LDPTAB[indx].rstrt_max;
           p->inh_restart = IN_LDPTAB[indx].startstate;
           p->proc_cat = IN_LDPTAB[indx].proc_category;
           p->error_threshold = IN_LDPTAB[indx].error_threshold;
           p->error_dec_rate = IN_LDPTAB[indx].error_dec_rate;
           p->init_complete_timer = IN_LDPTAB[indx].init_complete_timer;
           p->procinit_timer = IN_LDPTAB[indx].procinit_timer;
           p->crerror_inh = IN_LDPTAB[indx].crerror_inh;
           p->inh_softchk = IN_LDPTAB[indx].softchk;
           p->run_lvl = IN_LDPTAB[indx].run_lvl;
           p->msgh_qid = IN_LDPTAB[indx].msgh_qid;
           p->create_timer = IN_LDPTAB[indx].create_timer;
           p->print_progress = IN_LDPTAB[indx].print_progress;
           p->q_size = IN_LDPTAB[indx].q_size;
           p->on_active = IN_LDPTAB[indx].on_active;
           p->global_queue_timer = IN_LDPTAB[indx].global_queue_timer;
           p->lv3_timer = IN_LDPTAB[indx].lv3_timer;
           p->brevity_low = IN_LDPTAB[indx].brevity_low;
           p->brevity_high = IN_LDPTAB[indx].brevity_high;
           p->brevity_interval = IN_LDPTAB[indx].brevity_interval;
           p->msg_limit = IN_LDPTAB[indx].msg_limit;
           p->third_party = IN_LDPTAB[indx].third_party;
           p->ps = IN_LDPTAB[indx].ps;
           p->isRT = IN_LDPTAB[indx].isRT;
           nProcs++;
         }
         ack.m_nProcs = nProcs;
         if(indx != IN_SNPRCMX){
           //CR_X733PRM(POA_TMJ,"NOT ALL CREATED", processingErrorAlarm, softwareError, NULL, ";231", 
           //           "REPT INIT ERROR MORE THEN %d TEMPORARY PROCESSES, NOT ALL CREATED", INmaxProcInfo);
           printf("REPT INIT ERROR MORE THEN %d TEMPORARY PROCESSES, NOT ALL CREATED\n",
                  INmaxProcInfo);
         }
         int	msize = (char*)(&ack.m_ProcInfo[nProcs]) - (char*)(&ack);
         if((ret = INevent.send(orig_qid, (char*)&ack, msize, 0L)) != GLsuccess){
           //INIT_ERROR(("Failed to send INcheckLeadAck orig_qid %s, ret %d", orig_qid.display(), ret));
           printf("Failed to send INcheckLeadAck orig_qid %s, ret %d\n",
                  orig_qid.display(), ret);
         }
			
       } 
     }
     break;
   }
case INcheckLeadAckTyp:
   {
     INcheckLeadAck* ack =  (INcheckLeadAck*) msg;
     int expsize = (char*)(&(ack->m_ProcInfo[ack->m_nProcs])) - (char*)ack;
     if (msgsize != expsize){
       //INIT_ERROR(("INcheckLeadAck message received with size %d, expected %d", msgsize, expsize));
       printf("INcheckLeadAck message received with size %d, expected %d\n",
              msgsize, expsize);
       return;
     }
     if(INevent.onLeadCC()){
       //INIT_ERROR(("INcheckLeadAck message received on lead CC"));
       printf("INcheckLeadAck message received on lead CC\n");
       return;
     }

     INevent.setLeadCC(orig_qid);
		
     // Do not create temporary processes on the first pass
     if(IN_LDSTATE.run_lvl == IN_LDSTATE.first_runlvl || IN_LDCURSTATE != S_ACT){
       INCLRTMR(INcheckleadtmr);
       return;
     }

     if(IN_LDSTATE.systep != IN_STEADY){
       //INIT_DEBUG((IN_DEBUG | IN_MSGHTR ),(POA_INF,"INrcvmsg(): INcheckleadAck when not in STEADY"));
       printf("INrcvmsg(): INcheckleadAck when not in STEADY\n");
       return;
     }
     INCLRTMR(INcheckleadtmr);
     int	indx;
     // Find all the temporary processes with on_active set
     Bool toDelete[IN_SNPRCMX];
     memset(toDelete, 0x0, sizeof(toDelete));
     for(i = 0; i < IN_SNPRCMX; i++){
       if(IN_INVPROC(i)){
         continue;
       }
       if(IN_LDPTAB[i].permstate == IN_TEMPPROC &&
          IN_LDPTAB[i].on_active == TRUE){
         toDelete[i] = TRUE;
       }
     }
		
     // Insert all the temporary processes into init process table
     // No need to check values since the data is already
     // validated by the lead INIT
     for(i = 0; i < ack->m_nProcs; i++){
       if((indx = INfindproc(ack->m_ProcInfo[i].msgh_name)) < IN_SNPRCMX){
         toDelete[indx] = FALSE;
         continue;
       }

       // Find an empty slot
       for(indx = 0; indx < IN_SNPRCMX && IN_VALIDPROC(indx); indx++);
       if(indx == IN_SNPRCMX){
         //CR_X733PRM(POA_TMJ,"NO MORE PROCESS SLOTS", processingErrorAlarm, softwareError, NULL, ";232", 
         //           "REPT INIT ERROR NO MORE PROCESS SLOTS AVAILABLE, SOME PROCESS NOT CREATED");
         printf("REPT INIT ERROR NO MORE PROCESS SLOTS AVAILABLE, SOME PROCESS NOT CREATED\n");
         break;
       }
       // Copy all the process data
       INprocInfo * p = &ack->m_ProcInfo[i];
       strcpy(IN_LDPTAB[indx].proctag, p->msgh_name);
       strcpy(IN_LDPTAB[indx].pathname, p->full_path);
       strcpy(IN_LDPTAB[indx].ofc_pathname, p->ofc_path);
       strcpy(IN_LDPTAB[indx].ext_pathname, p->ext_path);
       IN_LDPTAB[indx].priority = p->priority;
       IN_LDPTAB[indx].uid = p->uid;
       IN_LDPTAB[indx].group_id = p->group_id;
       IN_LDPTAB[indx].peg_intvl = p->sanity_tmr;
       IN_LDPTAB[indx].rstrt_intvl = p->rstrt_intvl;
       IN_LDPTAB[indx].rstrt_max = p->rstrt_max;
       IN_LDPTAB[indx].startstate = p->inh_restart;
       IN_LDPTAB[indx].proc_category = p->proc_cat;
       IN_LDPTAB[indx].error_threshold = p->error_threshold;
       IN_LDPTAB[indx].error_dec_rate = p->error_dec_rate;
       IN_LDPTAB[indx].init_complete_timer = p->init_complete_timer;
       IN_LDPTAB[indx].procinit_timer = p->procinit_timer;
       IN_LDPTAB[indx].crerror_inh = p->crerror_inh;
       IN_LDPTAB[indx].softchk = p->inh_softchk;
       IN_LDPTAB[indx].run_lvl = p->run_lvl;
       IN_LDPTAB[indx].msgh_qid = p->msgh_qid;
       if(INqInUse[p->msgh_qid]){
         //CRDEBUG_PRINT(1, ("Qid %d already in use", p->msgh_qid));
         printf("Qid %d already in use", p->msgh_qid);
       }
       INqInUse[p->msgh_qid] = TRUE;
       IN_LDPTAB[indx].create_timer = p->create_timer;
       IN_LDPTAB[indx].print_progress = p->print_progress;
       IN_LDPTAB[indx].q_size = p->q_size;
       IN_LDPTAB[indx].on_active = p->on_active;
       IN_LDPTAB[indx].third_party = p->third_party;
       IN_LDPTAB[indx].ps = p->ps;
       IN_LDPTAB[indx].isRT = p->isRT;
       IN_LDPTAB[indx].global_queue_timer = p->global_queue_timer;
       IN_LDPTAB[indx].lv3_timer = p->lv3_timer;
       IN_LDPTAB[indx].brevity_low = p->brevity_low;
       IN_LDPTAB[indx].brevity_high = p->brevity_high;
       IN_LDPTAB[indx].brevity_interval = p->brevity_interval;
       IN_LDPTAB[indx].msg_limit = p->msg_limit;
       IN_LDPTAB[indx].permstate = IN_TEMPPROC;
       IN_LDPTAB[indx].syncstep = INV_STEP;
       IN_SDPTAB[indx].procstate = IN_DEAD;
       if((INsetrstrt(SN_LV4,indx,IN_INIT) == GLfail) ||
		   		((ret = INsync(indx,IN_READY)) != GLsuccess)){
         INinitptab(indx);
         //CR_PRM(POA_INF, "REPT INIT ERROR FAILED TO REPLICATE %s", p->msgh_name);
         printf("REPT INIT ERROR FAILED TO REPLICATE %s", p->msgh_name);
       }
     }
     // Kill all processes that are not supposed to be here
     for(i = 0; i < IN_SNPRCMX; i++){
       if(!toDelete[i]){
         continue;
       }
       // Delete this process
       //CR_PRM(POA_INF, "REPT INIT DELETING PROCESS %s", IN_LDPTAB[i].proctag);
       printf("REPT INIT DELETING PROCESS %s\n", IN_LDPTAB[i].proctag);
       IN_LDPTAB[i].startstate = IN_INHRESTART;

       /* Kill the process and let the normal death of process code
       ** handle it.
       */
       if(IN_LDPTAB[i].pid > 1){
         (void)INkill(i,SIGKILL);
       } else {
         //INIT_DEBUG((IN_ALWAYSTR), (POA_INF, "INcheckLeadAck: invalid pid for %s", IN_LDPTAB[i].proctag));
         printf("INcheckLeadAck: invalid pid for %s\n", IN_LDPTAB[i].proctag);
       }
     }
		
     break;
   }
case INinitializeAckTyp:
   {
     if (msgsize != sizeof(INinitializeAck)){
       //INIT_ERROR(("INinitializeAck: message received with size %d, expected %d", msgsize, sizeof(INinitializeAck)));
       printf("INinitializeAck: message received with size %d, expected %d\n",
              msgsize, sizeof(INinitializeAck));
       return;
     }
     // Find the process this message is from
     if((ret = INevent.getName(orig_qid, name)) != GLsuccess){
       //INIT_ERROR(("INinitializeAck: Could not match name to qid %s", orig_qid.display()));
       printf("INinitializeAck: Could not match name to qid %s\n",
              orig_qid.display());
       return;
     }
     char*	pname;
     if(name[0] == '_'){
       pname = &name[1];
     } else {
       pname = name;
     }
     if ((i = INfindproc(pname)) >= IN_SNPRCMX) {
       /* Couldn't find the process, send error return msg */
       //INIT_ERROR(("INinitializeAck: Could not find name %s", pname));
       printf("INinitializeAck: Could not find name %s\n", pname);
       return;
     }

     if(IN_SDPTAB[i].procstep == IN_PROCINIT){
       IN_SDPTAB[i].procstep = IN_STEADY;
       INCLRTMR(INproctmr[i].sync_tmr);
       //CR_PRM(POA_INF, "REPT INIT %s PROGRESS INIT COMPLETE", pname);
       printf("REPT INIT %s PROGRESS INIT COMPLETE\n", pname);
       INnext_rlvl();
       IN_LDPTAB[i].sn_lvl = SN_NOINIT;
     } else {
       //INIT_ERROR(("INinitializeAck: Invalid state %d name %s", IN_SDPTAB[i].procstep, pname));
       printf("INinitializeAck: Invalid state %d name %s\n",
              IN_SDPTAB[i].procstep, pname);
     }
     break;
   }
case MHgqInitTyp:
   {
     // This message is sent by MSGH to trigger global queue transition
     if (msgsize != sizeof(MHgqInit)){
       //INIT_ERROR(("MHgqInitTyp: message received with size %d, expected %d", msgsize, sizeof(MHgqInit)));
       printf("MHgqInitTyp: message received with size %d, expected %d\n",
              msgsize, sizeof(MHgqInit));
       return;
     }
     MHqid realQ = ((MHgqInit*)msg)->m_realQ;
     MHqid gqid = ((MHgqInit*)msg)->m_gqid;
     MHqid rcvdQid = gqid;
     // Do this to be backward compatible with R25.  Remove
     // this in the future once R25 is not supported
     if(INevent.Qid2Host(gqid) == 127){
       // Message is from old host
       extern MHqid MHmakeqid(int, int);
       gqid = MHmakeqid(MHgQHost, INevent.Qid2Qid(gqid));
     }
     // Find the process this message is from
     if((ret = INevent.getName(realQ, name)) != GLsuccess){
       //INIT_ERROR(("MHgqInit: Could not match name to qid %s", realQ.display()));
       printf("MHgqInit: Could not match name to qid %s\n",
              realQ.display());
       MHgqInitAck ack(rcvdQid, ret);
       INevent.send(orig_qid, (char*)&ack, sizeof(ack), 0L);
       return;
     }
     if(name[0] == '_'){
       strcpy(name, &name[1]);
     }
     if ((i = INfindproc(name)) >= IN_SNPRCMX) {
       /* Couldn't find the process, send error return msg */
       //INIT_ERROR(("MHgqInit: Could not find name %s", name));
       printf("MHgqInit: Could not find name %s\n", name);
       MHgqInitAck ack(rcvdQid, INNOPROC);
       INevent.send(orig_qid, (char*)&ack, sizeof(ack), 0L);
       return;
     }

     int	j;
     int	empty = -1;
     for(j = 0; j < INmaxgQids; j++){
       if(IN_LDPTAB[i].gqid[j] == gqid){
         break;
       } else if(IN_LDPTAB[i].gqid[j] == MHnullQ){
         empty = j;
       }
     }

     if(j == INmaxgQids){
       if(empty >= 0){
         IN_LDPTAB[i].gqid[empty] = gqid;
       } else {
         //INIT_ERROR(("MHgqInit: Too many global qids %s", name));
         printf("MHgqInit: Too many global qids %s\n", name);
         MHgqInitAck ack(rcvdQid, INNOPROC);
         INevent.send(orig_qid, (char*)&ack, sizeof(ack), 0L);
         return;
       }
     }

     // Send the positive ack and send the queue message to the process
     MHgqInitAck ack(rcvdQid, GLsuccess);
     if((ret = INevent.send(orig_qid, (char*)&ack, sizeof(ack), 0L)) != GLsuccess){
       //CRDEBUG_PRINT(0x1, ("Failed to send to %s ret %d", orig_qid.display(), ret));
       printf("Failed to send to %s ret %d\n",
              orig_qid.display(), ret);
     }

     if(!INgqfailover){
       INsync(i, IN_GQ, realQ, gqid);
     } else {
       IN_LDPTAB[i].gqsync = IN_BGQ;
       IN_LDPTAB[i].realQ = realQ;
     }
     break;
   }
case MHgqRcvAckTyp:
   {
     if (msgsize != sizeof(MHgqRcvAck)){
       //INIT_ERROR(("MHgqRcvAck: message received with size %d, expected %d", msgsize, sizeof(MHgqRcvAck)));
       printf("MHgqRcvAck: message received with size %d, expected %d\n",
              msgsize, sizeof(MHgqRcvAck));
       return;
     }
     MHqid gqid = ((MHgqRcvAck*)msg)->gqid;
     MHqid realQ = ((MHgqRcvAck*)msg)->srcQue;
     name[0] = 0;
     // Find the process this message is from
     if((realQ != MHnullQ) && (ret = INevent.getName(realQ, name)) != GLsuccess){
       //INIT_ERROR(("MHgqAck: Could not match name to qid %s", realQ.display()));
       printf("MHgqAck: Could not match name to qid %s\n",
              realQ.display());
       return;
     }
     if(name[0] == '_'){
       strcpy(name, &name[1]);
     }
 
     int	j;

     for (i = 0; i < IN_SNPRCMX; i++) {
       if (!IN_VALIDPROC(i)){
         continue;
       }
       int	j;
       for(j = 0; j < INmaxgQids && (IN_LDPTAB[i].gqid[j] != gqid || 
                                     (realQ != MHnullQ && strcmp(name, IN_LDPTAB[i].proctag) != 0)); j++);
       if(j != INmaxgQids){
         break;
       }
     }
     if(i == IN_SNPRCMX){
       //INIT_ERROR(("MHgqRcvAck: Could not find global qid %s", gqid.display()));
       printf("MHgqRcvAck: Could not find global qid %s\n",
              gqid.display());
       return;
     }


     // Should be waiting for the response
     if(IN_LDPTAB[i].gqsync != IN_GQ){
       //INIT_ERROR(("MHgqRcvAck: %s current gqsync %s != IN_GQ", name, IN_SQSTEPNM(IN_LDPTAB[i].gqsync)));
       printf("MHgqRcvAck: %s current gqsync %s != IN_GQ\n",
              name, IN_SQSTEPNM(IN_LDPTAB[i].gqsync));
			
       // reinit the process
       INreqinit(SN_LV0, i, IN_GQTIMEOUT, IN_SOFT, "UNEXPECTED GLOBAL QUEUE MSG");
       return;
     }

     IN_LDPTAB[i].gqCnt --;
     if(IN_LDPTAB[i].gqCnt <= 0){
       IN_LDPTAB[i].gqsync = IN_MAXSTEP;
       IN_LDPTAB[i].gqCnt = 0;
       INCLRTMR(INproctmr[i].gq_tmr);
     }
     IN_LDPTAB[i].realQ = MHnullQ;
     //CR_PRM(POA_INF, "REPT INIT %s GLOBAL QUEUE TRANSITIONED", IN_LDPTAB[i].proctag);
     printf("REPT INIT %s GLOBAL QUEUE TRANSITIONED\n",
            IN_LDPTAB[i].proctag);
     INgqsequence();
     break;

   }
case MHfailoverTyp:
   {
     if (msgsize != sizeof(MHfailover)){
       //INIT_ERROR(("MHfailover: message received with size %d, expected %d", msgsize, sizeof(MHfailover)));
       printf("MHfailover: message received with size %d, expected %d\n",
              msgsize, sizeof(MHfailover));
       return;
     }

     if(INgqfailover == FALSE){
       INgqfailover = TRUE;
     } else {
       // Start transitioning gqs, but if this node is not
       // ready just do a level 5
       if(IN_LDSTATE.initstate == INITING && IN_LDCURSTATE != S_LEADACT){
         //CR_X733PRM(POA_TCR, "NO ACTIVE", processingErrorAlarm, applicationSubsystemFailure, NULL, ";211", 
         //           "REPT INIT ESCALATING TO LEVEL 5 NO ACTIVE PRESENT");
         printf("REPT INIT ESCALATING TO LEVEL 5 NO ACTIVE PRESENT\n");
         INescalate(SN_LV5, INNOSTDBY, IN_SOFT, INIT_INDEX);
       }
       IN_LDSTATE.gq_run_lvl = 0;
       //CR_X733PRM(POA_TCR + POA_MAXALARM, "GQ TRANSITION", processingErrorAlarm, applicationSubsystemFailure, NULL, ";212", 
       //           "REPT INIT STARTING TRANSITIONING GLOBAL QUEUES");
       printf("REPT INIT STARTING TRANSITIONING GLOBAL QUEUES\n");
     }
   }
//case CRindQryMsgTyp:
//  /* Check vmem and send indicator updates but do not print OMs
//  ** unless vmem status has changed.
//  */
//  INvmem_check(TRUE,FALSE);
//  //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg(): responed to CRindQryMsg"));
//  printf("INrcvmsg(): responed to CRindQryMsg\n");
//  break;
default:
  /* Ignore unrecognized messages */
  //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INrcvmsg(): received unknown message type 0x%x, dumping it", msgType));
  printf("INrcvmsg(): received unknown message type 0x%x, dumping it\n",
         msgType);
  return;
}
}

#ifdef OLD_SU
/*
** NAME:
**	INgetsudata()
**
** DESCRIPTION:
**	This function parses SU package inormation contained in proclist
**
** INPUTS:
**	proclist - Pointer to SU proclist path
**	apply	 - TRUE if called following SUapplyMsg
**		   FALSE if called as part of system boot
**
** RETURNS:
**	GLsuccess - if INsudata successfully loaded
**	SU_BADPROCLIST - if there were problems parsing proclist
**	SU_INITERROR - if internal INIT error occured	
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
**	INsudata is initialized with SU information
*/

#define IN_SU_OBJ_PATH	1	/* Object file is first in proclist	*/
#define IN_SU_SN_LVL	2	/* SU initialization level is second	*/
#define IN_SU_FST_FILE	3	/* First image or other associated file */

GLretVal
INgetsudata(char * proclist, Bool apply)
{
	static char * pl_ptr = (char *) 0; /* Pointer to malloc'd proclist file buffer */
  struct stat     stbuf;

	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): entered with proclist %s",proclist));
  printf("INgetsudata(): entered with proclist %s\n",
         proclist);

	INinit_su_idx = -1;	/* Assume INIT not present in the SU */

	if(proclist[IN_PATHNMMX-1] != '\0'){
		proclist[IN_PATHNMMX-1] = '\0';
		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): non null terminated proclist %s",proclist));
    printf("INgetsudata(): non null terminated proclist %s\n",
           proclist);
		return(SU_BADPROCLIST);
	}

  if (stat(proclist, &stbuf) < 0) {
		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): non existent or null proclist %s",proclist));
    printf("INgetsudata(): non existent or null proclist %s\n",
           proclist);
    return(SU_BADPROCLIST);
  }

  Short fd = open(proclist,O_RDONLY);
 
  if (fd < 0) {
		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): failed to open  proclist %s, errno = %d",proclist,errno));
    printf("INgetsudata(): failed to open  proclist %s, errno = %d\n",
           proclist,errno);
    return(SU_BADPROCLIST);
  }

	/* Initialize the path of tbe current SU */
	strncpy(INsupath,proclist,IN_PATHNMMX);
	char * p = &INsupath[IN_PATHNMMX-1];
	/* Find the base path of the SU 	*/
	while(p >= INsupath){
		if(*p == '/'){
			*p = '\0';
			break;
		}
		p--;
	}

	
	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): INsupath = %s"));
  printf("INgetsudata(): INsupath = %s\n");

	/* Deallocate old file buffer if necessary */
	if(pl_ptr != (char *)0){
		free(pl_ptr);
		pl_ptr = (char *) 0;
	}

  if ((pl_ptr = (char *)malloc((Short)(stbuf.st_size+1))) == (char *)0) {
    //INIT_ERROR(("Unable to malloc space to read in proclist"));
    printf("Unable to malloc space to read in proclist\n");
    close(fd);
    return(SU_INITERROR);
  }
 
  Short nread = read(fd,(void*)pl_ptr,(Short)stbuf.st_size);
 
  close(fd);
 
  if (nread != stbuf.st_size) {
    //INIT_ERROR(("Failed to read in proclist, errno=%d, size=%d",errno,nread));
    printf("Failed to read in proclist, errno=%d, size=%d\n",
           errno,nread);
    return(SU_INITERROR);
  }

  pl_ptr[stbuf.st_size] = '\0';

  char *          file_index = pl_ptr;    /* Keeps track of a postion where
                                        	** search for another token should start.
                                          */
  int             line = 1;       	/* Line number in proclist file	*/
	int		prev_line = 0;		/* Previous line number		*/

	int		token_pos = 2;		/* Position of token on current line */
	char		token_buf[MAX_TOKEN_LEN];
	char		*token = token_buf;
	int		su_index;
	int		ret;
	char		tmp_path[IN_PATHNMMX + 4];

  /* Initialize SU information */
  memset((char *)INsudata,0x0,sizeof(IN_SU_PROCINFO)*SU_MAX_OBJ);

	su_index = -1;

	while(INgettoken(token_buf,file_index,line) != GLfail){
		token = token_buf;
		if(su_index >= SU_MAX_OBJ){
			//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): too many entries in proclist"));
      printf("INgetsudata(): too many entries in proclist\n");
      return(SU_BADPROCLIST);
		}

		token_pos ++;

		if(line > prev_line) {		/* New object file entry */
			if(token_pos < 3){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): missing sn_lvl line=%d",prev_line));
        printf("INgetsudata(): missing sn_lvl line=%d\n",
               prev_line);
        return(SU_BADPROCLIST);
			}
			token_pos = 1;
			prev_line = line;
			su_index++;
		}

		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): token = %s, token_pos = %d",token,token_pos));
    printf("INgetsudata(): token = %s, token_pos = %d\n",
           token,token_pos);

		switch(token_pos){
		case IN_SU_OBJ_PATH:
			
			/* Do not allow duplicate files */
			if(INisdupfile(token)){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): duplicate file %s, line=%d",token,line));
        printf("INgetsudata(): duplicate file %s, line=%d\n",
               token,line);
        return(SU_BADPROCLIST);
			}

			/* Check if the file is changed as part of the SU */
			if(token[0] == '*' && token[1] != '\0'){
				if(strncmp(&token[1],INinitpath,IN_PATHNMMX) == 0){
					INinit_su_idx = su_index;
				}
				strncpy(INsudata[su_index].obj_path,token + 1,IN_PATHNMMX);
				strcpy(tmp_path,&token[1]);
				strcat(tmp_path,".new");
				if(INgetpath(tmp_path, TRUE) == GLsuccess){
					INsudata[su_index].changed = TRUE;
					//CRDEBUG_PRINT((IN_DEBUG | IN_MSGHTR),("INgetsudata(): .new file exits for executable file %s -  treating as changed, line=%d",token,line));
          printf("INgetsudata(): .new file exits for executable file %s -  "
                 "treating as changed, line=%d\n",token,line);
				} else {
					INsudata[su_index].changed = FALSE;
				}

				if(INgetpath(&token[1],TRUE) != GLsuccess){
					//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): executable file %s does not exist,line=%d",&token[1],line));
          printf("INgetsudata(): executable file %s does not exist,line=%d\n",
                 &token[1],line);
          return(SU_BADPROCLIST);
				}
				break;
			} else {
				INsudata[su_index].changed = TRUE;
				strncpy(INsudata[su_index].obj_path,token,IN_PATHNMMX);
				if(strncmp(token,INinitpath,IN_PATHNMMX) == 0){
					INinit_su_idx = su_index;
				}
			}

			/* If object file name is '*' do not make file validity checks */
			if(token[0] == '*' && token[1] == '\0'){
				break;
			}

			/* Check if the file is added in this SU */
			strncpy(INsudata[su_index].obj_path,token,IN_PATHNMMX);
			if(INgetpath(token,TRUE) == GLsuccess){
				INsudata[su_index].new_obj = FALSE;
			} else {
				//CRDEBUG_PRINT((IN_DEBUG | IN_MSGHTR),("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d",token,line));
        printf("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d\n",
               token,line);
				INsudata[su_index].new_obj = TRUE;
			}

			if(INsudata[su_index].obj_path[IN_PATHNMMX-1] != '\0'){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): object path %s too long,line=%d",token,line));
        printf("INgetsudata(): object path %s too long,line=%d\n",
               token,line);
        return(SU_BADPROCLIST);
			}
			
			strcat(token, ".new");

			/* Check file validity only if apply is TRUE,
			** since during system boot we just want to clean up
			** any SU files indepenent of their existence
			*/
			ret = stat(token, &stbuf);
			if (apply == TRUE && (ret < 0 || stbuf.st_size == 0 || !(stbuf.st_mode & S_IXOTH))) {
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): invalid or nonexistent %s",token));
        printf("INgetsudata(): invalid or nonexistent %s\n",
               token);
        return(SU_BADPROCLIST);
			}
			break;
		case IN_SU_SN_LVL:
       {
         int lvl_indx = 0;
         if(token[0] == '-'){
           INsudata[su_index].kill = TRUE;
           lvl_indx = 1;
         } else {
           INsudata[su_index].kill = FALSE;
         }

         if(token[lvl_indx] == '0') {
           INsudata[su_index].sn_lvl = SN_LV0;
         } else if (token[lvl_indx] == '1'){
           INsudata[su_index].sn_lvl = SN_LV1;
         } else {
           //INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): invalid sn_lvl %s,line=%d",token,line));
           printf("INgetsudata(): invalid sn_lvl %s,line=%d\n",
                  token,line);
           return(SU_BADPROCLIST);
         }
         break;
       }
		default:	/* Other related files */
			if(token_pos >= (IN_SU_FST_FILE + SU_MAX_OFILES)){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): too many image files line=%d",line));
        printf("INgetsudata(): too many image files line=%d\n",
               line);
        return(SU_BADPROCLIST);
			}

			/* Do not allow duplicate files */
			if(INisdupfile(token)){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): duplicate file %s, line=%d",token,line));
        printf("INgetsudata(): duplicate file %s, line=%d\n",
               token,line);
        return(SU_BADPROCLIST);
			}

			/* Figure out if the file was new in this SU or not */
			if(apply == TRUE) {
				if(INgetpath(token,FALSE) == GLsuccess){
					INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = FALSE;
				} else {
					//CRDEBUG_PRINT((IN_DEBUG | IN_MSGHTR),("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d",token,line));
          printf("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d\n",
                 token,line);
					INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = TRUE;
				}
			} else {
				strcpy(tmp_path,token);
				if(INgetpath(tmp_path,FALSE) == GLsuccess){
					strcat(tmp_path,".new");
					if(INgetpath(tmp_path,FALSE) == GLsuccess){
						INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = FALSE;
					} else {
						strcpy(tmp_path,token);
						strcat(tmp_path,".old");
						if(INgetpath(tmp_path,FALSE) == GLsuccess){
							INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = FALSE;
						} else {
							INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = TRUE;
							//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d",token,line));
              printf("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d\n",
                     token,line);
						}
					}
				} else {
					INsudata[su_index].new_file[token_pos-IN_SU_FST_FILE] = TRUE;
					//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d",token,line));
          printf("INgetsudata(): executable file %s DOES NOT exist, treated as new, line=%d\n",
                 token,line);
				}
			}
			strncpy(INsudata[su_index].file_path[token_pos-IN_SU_FST_FILE],token,IN_PATHNMMX);
			if(INsudata[su_index].file_path[token_pos-IN_SU_FST_FILE][IN_PATHNMMX-1] != '\0'){
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): image path %s too long,line=%d",token,line));
        printf("INgetsudata(): image path %s too long,line=%d\n",
               token,line);
        return(SU_BADPROCLIST);
			}

			strcat(token, ".new");

			/* Check file validity */
			ret = stat(token, &stbuf);
			if (apply == TRUE && (ret < 0 || stbuf.st_size == 0)) {
				//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): invalid or nonexistent %s",token));
        printf("INgetsudata(): invalid or nonexistent %s\n",token);
        return(SU_BADPROCLIST);
			}
		}
		
	}

	if(su_index >= 0 && INsudata[su_index].obj_path[0] != '\0' && INsudata[su_index].sn_lvl == SN_INV){
		//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): missing sn_lvl line=%d",prev_line));
    printf("INgetsudata(): missing sn_lvl line=%d\n",
           prev_line);
    return(SU_BADPROCLIST);
	}

	//INIT_DEBUG((IN_DEBUG | IN_MSGHTR),(POA_INF,"INgetsudata(): returned successfully"));
  printf("INgetsudata(): returned successfully\n");
	return(GLsuccess);
}

/*
** NAME:
**	INisdupfile()
**
** DESCRIPTION:
**	This function check INsudata to verify that no duplicate entries found
**
** INPUTS:
**	path - Object file path
**
**
** RETURNS:
**	TRUE 	- if duplicate file found in INsudata 
**	FALSE   - otherwise
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
**	None.
*/
Bool
INisdupfile(char * path)
{
	int	i,j;
	
	if(path[0] == '*' && path[1] == '\0'){
		return(FALSE);
	}

	for(i = 0; i < SU_MAX_OBJ && INsudata[i].obj_path[0] != '\0'; i++){
		if(strcmp(INsudata[i].obj_path,path) == 0){
			return(TRUE);
		}
		/* Check images files too */
		for(j = 0; j < SU_MAX_OFILES && INsudata[i].file_path[j][0] != '\0'; j++){
			if(strcmp(INsudata[i].file_path[j],path) == 0){
				return(TRUE);
			}
		}
	}

	return(FALSE);
}
#endif

Void
INgetMateCC(Char* name, Bool getMyName)
{
	if(getMyName){
		INevent.getMyHostName(name);
	}
	if(name[2] == '0'){
		name[2] = '1';
	} else {
		name[2] = '0';
	}
}

