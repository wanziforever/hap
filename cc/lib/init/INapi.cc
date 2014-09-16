#include <unistd.h>
// IBM thomharr 20060908 - extra headers.
#ifdef __linux
#include <string.h>
#endif

#include <cc/hdr/init/INapi.hh>
#include <cc/hdr/msgh/MHinfoExt.hh>
#include <cc/hdr/init/INreturns.hh>
#include <cc/hdr/init/INrunLvl.hh>

#define INtimeOut	5000
#define INbufLen	100

static int	count = 0;

GLretVal
INsetup(char* machine, char* InitName, MHqid& rcvQid, char* name)
{
	GLretVal	ret;

  if((ret = MHmsgh.attach()) != GLsuccess && ret != MHexist) {
    return(ret);
  }

	pid_t		pid = getpid();

  sprintf(name, "INAPI%5d%1d", pid,count);
  count++;

  if((ret = MHmsgh.regName(name, rcvQid, FALSE, TRUE, FALSE, MH_LOCAL, FALSE)) != GLsuccess){          
    return(ret);
  }

  char    RealHostName[80];
        
  if(machine != NULL){
    if(MHmsgh.getRealHostname(machine, RealHostName) < 0){
      return(MHbadName);
    }
    sprintf(InitName, "%s:INIT", machine);
  } else {
    strcpy(InitName, "INIT");
  }

	return(GLsuccess);
}

GLretVal 
INkillProcess(INkillProc* killmsg, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}

	if((ret = killmsg->send(InitName, rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INkillProcAckTyp:
			ret = GLsuccess;
			break;
		case INkillProcFailTyp:
			ret = ((INkillProcFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}

GLretVal 
INprcCreat(INprocCreate * pCreat, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}

	if((ret = pCreat->send(InitName, rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INprocCreateAckTyp:
			ret = GLsuccess;
			break;
		case INprocCreateFailTyp:
			ret = ((INprocCreateFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}

//GLretVal INinitProc(INinitSCN* pInit, char* machine) {
//	MHqid		rcvQid;
//	GLretVal	ret;
//	long		mbuf[INbufLen];
//	char		InitName[2*MHmaxNameLen + 1];
//	char		name[MHmaxNameLen + 1];
//
//	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
//		return(ret);
//	}
//
//	if((ret = pInit->send(InitName, rcvQid, (Long)0)) != GLsuccess){
//		return(ret);
//	}
//
//	int	numTry = 0;
//	int	msgsz;
//  tryAgain:
//	msgsz = INbufLen * sizeof(long);
//	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
//		if(ret == MHintr && numTry < 5){
//			numTry++;
//			goto tryAgain;
//		}
//	} else {
//		switch(((MHmsgBase*)mbuf)->msgType){
//		case INinitSCNAckTyp:
//			ret = GLsuccess;
//			break;
//		case INinitSCNFailTyp:
//			ret = ((INinitSCNFail*)mbuf)->ret;
//			break;
//		}
//	}
//
//	MHmsgh.rmName(rcvQid, name, TRUE);
//
//	return(ret);
//}

GLretVal 
INsetRestart(INsetRstrt* rstrt_msg, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}
	
	if((ret = rstrt_msg->send(InitName, rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INsetRstrtAckTyp:
			ret = GLsuccess;
			break;
		case INsetRstrtFailTyp:
			ret = ((INsetRstrtFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}


GLretVal 
INsetSoftCheck(INsetSoftChk* softchk_msg, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}
	
	if((ret = softchk_msg->send(InitName, rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INsetSoftChkAckTyp:
			ret = GLsuccess;
			break;
		case INsetSoftChkFailTyp:
			ret = ((INsetSoftChkFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}

GLretVal 
INswitchCC(Bool ucl_flg)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(NULL, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}

	INswcc swcc_msg(ucl_flg);
	
	if((ret = swcc_msg.send(rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INswccAckTyp:
			ret = GLsuccess;
			break;
		case INswccFailTyp:
			ret = ((INswccFail*)mbuf)->retval;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);

	return(ret);
}

GLretVal 
INprcUpd(INprocUpdate* prcUpd, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}

	if((ret = prcUpd->send(InitName, rcvQid, (Long)0)) != GLsuccess){
		return(ret);
	}

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INprocUpdateAckTyp:
			ret = GLsuccess;
			break;
		case INprocUpdateFailTyp:
			ret = ((INprocUpdateFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}

GLretVal
INopInit(void* result, int& count, int type, char* proc, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];
	
	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}

	Bool 	send_broadcast = FALSE;
	
	if(machine == NULL && proc != NULL){
		strcpy(InitName, "INIT");
		send_broadcast = TRUE;
	}


  long     rcv_msg[4000];
  Short proc_index = 0;
  INinitdataResp *Resp;
  INsystemdata *sysdata;
  int      i = 0;  
  Bool  done = FALSE;
	Short	msgsz;
	int	host_num;

	if(type != IN_RESTART_DATA){

    INinitdataReq  req_msg((proc != NULL ? proc : ""), IN_OP_INIT_DATA);
 
    if(!send_broadcast){
      if((ret = req_msg.send(InitName, rcvQid, 0)) != GLsuccess) {
        return(ret);
      }
      // Since proc field was not null, act as if a broadcast was sent
      if(proc != NULL ){
        send_broadcast = TRUE;
        host_num = 1;
      }
    } else {
      if(( host_num = req_msg.sendToAllHosts("INIT", rcvQid, 0 )) <= 0 ){
        return(GLfail);
      }
    }  
 
    struct INopinitdata   proctable[IN_SNPRCMX];
 
    if(!send_broadcast){
      while(!done){
        msgsz = MHmsgSz;
        if((ret = MHmsgh.receive(rcvQid, (char *)rcv_msg, msgsz , 0 , 5000)) != GLsuccess){
          return(ret);
        }
        Resp = (INinitdataResp *)rcv_msg;
        if(Resp->nCount == 0) {
          /* fill up the all the global info */
          sysdata = (INsystemdata *)&(Resp->data);
          done = TRUE;
        } else {
          INopinitdata* pinit = (INopinitdata *)&Resp->data;
          for(int j = 0; j < Resp->nCount; j++ ) {
            proctable[proc_index] = pinit[j];
            proc_index++;
          }/* end of for loop */
        }
      }/* end of while loop */
    } else {
      for(i = 0 ; i < host_num ; i++ ){
        msgsz = MHmsgSz;

        if((ret = MHmsgh.receive(rcvQid, (char *)rcv_msg, msgsz , 0 , 5000)) != GLsuccess){
          return(ret);
        }
        Resp = (INinitdataResp *)rcv_msg;
        if(Resp->return_val == INNOPROC){
          continue;
        }
        INopinitdata* pinit = (INopinitdata *)&Resp->data;
        proctable[proc_index] = pinit[0];
        proc_index++;
      }
    }

    count = proc_index;
    if(type == IN_OP_INIT_DATA){
      memcpy(result, proctable, sizeof(proctable[0])*count);
    } else {
      memcpy(result, sysdata, sizeof(INsystemdata));
    }

	} else {	// Do op restart
    INinitdataReq  req_msg((proc != NULL ? proc : ""), IN_RESTART_DATA);

    if ( !send_broadcast ) {
      if(( ret = req_msg.send(InitName , rcvQid , 0)) != GLsuccess ) {
        return(ret);
      }
    } else {
      if ( ( host_num = req_msg.sendToAllHosts("INIT", rcvQid, 0) ) <= 0 ) {


        return(GLfail);
      }
    }

    struct INoprstrtdata proctable[IN_SNPRCMX];

    INoprstrtdata   *prstrt;

    if(!send_broadcast){
      while(!done){
        msgsz = MHmsgSz;
        if((ret = MHmsgh.receive(rcvQid, (char *)rcv_msg, msgsz , 0 , 5000)) != GLsuccess){
          return(ret);
        }
        Resp = (INinitdataResp *)rcv_msg;
        if( Resp->nCount < INMAXITEMSPERMESSAGE ){
          done = TRUE;
        }
        INoprstrtdata* prstrt = (INoprstrtdata *)&Resp->data;
        for(int j = 0; j < Resp->nCount; j++ ) {
          proctable[proc_index] = prstrt[j];
          proc_index++;
        }/* end of for loop */
      }/* end of while loop */
    } else {
      for(i = 0 ; i < host_num ; i++ ){
        msgsz = MHmsgSz;

        if((ret = MHmsgh.receive(rcvQid, (char *)rcv_msg, msgsz , 0 , 5000)) != GLsuccess){
          return(ret);
        }
        Resp = (INinitdataResp *)rcv_msg;
        if(Resp->return_val == INNOPROC){
          continue;
        }
        INoprstrtdata* prstrt = (INoprstrtdata *)&Resp->data;
        proctable[proc_index] = prstrt[0];
        proc_index++;
      }
    }
    count = proc_index;
    memcpy(result, proctable, sizeof(proctable[0])*count);

	}

	return(GLsuccess);
}

GLretVal
INsetRunlvl(unsigned char run_lvl, char* machine)
{
	MHqid		rcvQid;
	GLretVal	ret;
	long		mbuf[INbufLen];
	char		InitName[2*MHmaxNameLen + 1];
	char		name[MHmaxNameLen + 1];

	if((ret = INsetup(machine, InitName, rcvQid, name)) != GLsuccess){
		return(ret);
	}
	
  INsetRunLvl rlvl_msg((Char)run_lvl);
  if ((ret = rlvl_msg.send(InitName, rcvQid, 0)) != GLsuccess) {
    return(GLfail);
  }

	int	numTry = 0;
	int	msgsz;
  tryAgain:
	msgsz = INbufLen * sizeof(long);
	if((ret = MHmsgh.receive(rcvQid, (char*)mbuf, msgsz, 0, INtimeOut)) != GLsuccess){
		if(ret == MHintr && numTry < 5){
			numTry++;
			goto tryAgain;
		}
	} else {
		switch(((MHmsgBase*)mbuf)->msgType){
		case INsetRunLvlAckTyp:
			ret = GLsuccess;
			break;
		case INsetRunLvlFailTyp:
			ret = ((INsetRunLvlFail*)mbuf)->ret;
			break;
		}
	}

	MHmsgh.rmName(rcvQid, name, TRUE);
	return(ret);
}
