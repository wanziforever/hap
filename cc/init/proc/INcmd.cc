#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cc/hdr/init/INapi.hh>
#include <cc/hdr/msgh/MHinfoExt.hh>
#include <cc/hdr/init/INreturns.hh>
#include <cc/hdr/msgh/MHrt.hh>
#include "INlocal.hh"

enum Action {
	createProc,
	initProc,
	removeProc,
	nodeStatus,
	restartNode,
	softChk,
	noAction
};

char* commands[] = {
	"createProc",
	"initProc",
	"removeProc",
	"nodeStatus",
	"restartNode",
	"softChk"
};

Char		destINITname[2*MHmaxNameLen+1];
MHqid		myQid;
long		buffer[MHmsgLimit/sizeof(long)];

extern char * 	INparm_names[];
extern char * 	INbool_val[];
extern char * 	INcategory_vals[];
extern char * 	INrt_val[];


#define NULLPTR (char *) 0 

Short
INmatch_string(char * strings[], char * name, Short max_index)
{
	int     i;
         
	for(i = 0; i < max_index; i++){
		if(strcmp(name,strings[i]) == 0){
			break;
		}
	}

	return(i);
}

GLretVal
INgettoken(char *token, char* &file_index,int &line)
{
	int count = 0;
 
	/*
   *      Skip over all blanks, tabs and commented lines.
   */
 
	while ((*file_index == ' ') || (*file_index == '\t') ||
         (*file_index == '\n') || (*file_index == '#')) {
		/* If end of line, increment line number */
		if(*file_index == '\n') line++;
 
		/* If # skip to the end of line */
		if(*file_index == '#'){
			while(*file_index != '\n' && *file_index != '\0'){
				file_index++;
			}
        
			if(*file_index == '\n') line++;
 
			if(*file_index == '\0'){
				return(GLfail);
			}
		}
 
		file_index++;
	}
 
	/* Check for EOF */
	if(*file_index == '\0'){
		return(GLfail);
	}
 
 
	while ((count < MAX_TOKEN_LEN-1) && (*file_index != ' ') && (*file_index != '\t') &&
         (*file_index != '\n') && (*file_index != '\0') && (*file_index != '#')) {
		*token = *file_index;
		token++;
		file_index++;
		count++;
	}

	*token = '\0';
	return(GLsuccess);
}

Bool
INis_numeric(char * str_ptr, char * parm_str,int line)
{
  Bool is_numeric = TRUE;
  char * p = str_ptr;

  if(*str_ptr == '\0'){
    is_numeric = FALSE;
  }

  int count = 0;

  while(*p != '\0'){
    if(!isdigit(*p) || ++count > 7){
      is_numeric = FALSE;
      break;
    }
    p++;
  }

  if(is_numeric != TRUE){
    printf("%d PARAMETER %s VALUE MUST BE A WHOLE NUMBER\n",line,parm_str);
    return(FALSE);
  }

  return(TRUE);
}


Short
INgetNodeStatus()
{
	INsystemdata* 	status;
	INinitdataReq 	req_msg("", IN_OP_STATUS_DATA);
	int		msgSz = MHmsgLimit;
	
	if(req_msg.send(destINITname, myQid, 0) != GLsuccess){
    return(134);
	}
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, MHinitPtyp, 3000) != GLsuccess){
    return(135);
	}
	status = (INsystemdata*)(((INinitdataResp*)buffer)->data);              
	if((status->initstate != IN_NOINIT && status->initstate != IN_CUINTVL)|| 
     status->mystate != S_LEADACT){
		return(128);
	}
	return(0);
}

GLretVal
INparseInitlist(const char* iFile, INprocCreate* pCreat)
{
	struct stat     stbuf;
	char            token[MAX_TOKEN_LEN];
	char *          file_index;  
	int		line = 1;

  if ((stat(iFile, &stbuf) < 0) || (stbuf.st_size == 0)) {
    printf("Non-existent or empty initlist %s\n", iFile);
    return(GLfail);
  }

  int fd = open(iFile,O_RDONLY);

  if (fd < 0) {
    printf("Cannot open initlist %s\n", iFile);
    return(GLfail);
  }

	
	char    *fptr = (char *) 0; 
	if ((fptr = (char *)malloc((stbuf.st_size+1))) == NULLPTR) {
    printf("Unable to malloc space to read in initlist %s\n", iFile);
    return(GLfail);
  }

  int nread = read(fd,fptr,stbuf.st_size);
 
  if (nread != stbuf.st_size) {
    printf("read error initlist %s, errno = %d",iFile,errno);
    return(GLfail);
  }
 
  fptr[stbuf.st_size] = '\0';

	char* 		equal_loc;
	IN_PARM_INDEX 	parameter = IN_PARM_MAX;
	IN_BOOL_VAL 	bool_val;
	IN_RT_VAL 	rt_val;
	file_index = fptr;

	/* Now process per process parameters */
	while (file_index < &fptr[stbuf.st_size]){

		if(INgettoken(token,file_index,line) != GLfail){
			/* Find = sign and replace it with string terminator */
			if((equal_loc = strchr(token,'=')) == NULLPTR){
				printf("%d NOT A NAME=VALUE ENTRY %s\n",line,token);
				return(GLfail);
			}

			/* Separate the token into name-value pair	*/
			*equal_loc = 0;
			equal_loc++;
			if(strlen(equal_loc) == 0){
				printf("%d NULL VALUE FOR PARAMETER %s\n",line,token);
				return(GLfail); 
			} 
			parameter = (IN_PARM_INDEX)INmatch_string(INparm_names,token,IN_PARM_MAX); 
		} else {
			break;
		}

		switch(parameter){
    case IN_MSGH_NAME:
			
			if(strlen(equal_loc) < IN_NAMEMX){ 
				strcpy(pCreat->msgh_name,equal_loc);
				break;
			} else if(strlen(equal_loc) >= IN_EPATHNMMX){ 
				printf("%d LENGTH OF PROCESS NAME %s GREATER THAN MAXIMUM LENGTH %d\n",line,equal_loc,IN_EPATHNMMX);
				return(GLfail);
			}
			strcpy(pCreat->ext_path,equal_loc);
			break;
    case IN_RUN_LVL:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail); 
			}
			pCreat->run_lvl = atoi(equal_loc);
			if((atoi(equal_loc) > 255) ||
         (pCreat->run_lvl == 0)){
				printf("%d run_level %s INVALID\n",line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_PATH:
			if(strlen(equal_loc) >= IN_PATHNMMX){ 
				printf("%d LENGTH OF PATH %s GREATER THAN MAXIMUM LENGTH %d\n",line,equal_loc,IN_PATHNMMX);
				return(GLfail);
			}
#ifndef EES
			if(*equal_loc != '/'){ 
				printf("%d PATH %s MUST START AT ROOT\n",line,equal_loc);
				return(GLfail);
			}
#endif
			strcpy(pCreat->full_path,equal_loc);
	
			break;
    case IN_USER_ID:
			if(!isdigit(equal_loc[0])){
				// try to lookup user id from password file
				struct passwd* pswd;
				if((pswd = getpwnam(equal_loc)) == NULL){
					printf("%d CANNOT FIND USER ID %s ERRNO %d\n",line, equal_loc, errno);
					return(GLfail);
				}
				pCreat->uid = pswd->pw_uid;
				break;
			} else if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->uid = atoi(equal_loc);
			break;
    case IN_GROUP_ID:
			if(!isdigit(equal_loc[0])){
				// try to lookup group id from password file
				struct passwd* pswd;
				if((pswd = getpwnam(equal_loc)) == NULL){
					//CR_PRM(POA_INF, "%d CANNOT FIND GROUP ID %s ERRNO %d\n",line, equal_loc, errno);
          printf("%d CANNOT FIND GROUP ID %s ERRNO %d\n",line, equal_loc, errno);
					return(GLfail);
				}
				pCreat->group_id = pswd->pw_uid;
				break;
			} else if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->group_id = atoi(equal_loc);
			break;
    case IN_PRIORITY:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			} 
			pCreat->priority = atoi(equal_loc);
			break;
    case IN_SANITY_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->sanity_tmr = atoi(equal_loc);
			break;
		case IN_MSG_LIMIT:
			if(INis_numeric(equal_loc,token,line) == FALSE) { 
				return(GLfail);
			}
			pCreat->msg_limit = atoi(equal_loc);
			break;
    case IN_RESTART_INTERVAL:
			if(INis_numeric(equal_loc,token,line) == FALSE){ 
				return(GLfail);
			}
			pCreat->rstrt_intvl = atoi(equal_loc);
			break;
    case IN_RESTART_THRESHOLD:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->rstrt_max = atoi(equal_loc);
			break;
    case IN_INHIBIT_RESTART:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->inh_restart = IN_ALWRESTART;
				break;
			case IN_YES:
				pCreat->inh_restart = IN_INHRESTART;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%d VALUE FOR inhibit_restart=%s FOR %s MUST BE YES OR NO",line,equal_loc,pCreat->msgh_name);
        printf("%d VALUE FOR inhibit_restart=%s FOR %s MUST BE YES OR NO",
               line,equal_loc,pCreat->msgh_name);
				return(GLfail);
			}
			break;
    case IN_PROCESS_CATEGORY:
			if((pCreat->proc_cat = (IN_PROC_CATEGORY)INmatch_string(INcategory_vals,equal_loc,IN_MAX_CAT)) == IN_MAX_CAT){
				//CR_PRM(POA_INF,"%d INVALID process_category=%s",line,equal_loc);
        printf("%d INVALID process_category=%s",line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_INIT_COMPLETE_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){ 
				return(GLfail);
			}
			pCreat->init_complete_timer = atoi(equal_loc);
			break;
    case IN_PROCINIT_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->procinit_timer = atoi(equal_loc);
			break;
    case IN_CREATE_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->create_timer = atoi(equal_loc);
			break;
    case IN_LV3_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->lv3_timer = atoi(equal_loc);
			break;
    case IN_GLOBAL_QUEUE_TIMER:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->global_queue_timer = atoi(equal_loc);
			break;
    case IN_Q_SIZE:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->q_size = atoi(equal_loc);
			break;
    case IN_PS:
       {
         if(INis_numeric(equal_loc,token,line) == FALSE){
           return(GLfail);
         }
         pCreat->ps = atoi(equal_loc);
         break;
       }
    case IN_ERROR_THRESHOLD:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->error_threshold = atoi(equal_loc);
			break;
    case IN_ERROR_DEC_RATE:
			if(INis_numeric(equal_loc,token,line) == FALSE){
				return(GLfail);
			}
			pCreat->error_dec_rate = atoi(equal_loc);
			break;
    case IN_CRERROR_INH:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->crerror_inh = NO;
				break;
			case IN_YES:
				pCreat->crerror_inh = YES;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%d VALUE FOR crerror_inh=%s MUST BE YES OR NO",line,equal_loc);
        printf("%d VALUE FOR crerror_inh=%s MUST BE YES OR NO",
               line,equal_loc);
				return(GLfail);
			}
			break;
#ifdef __linux
    case IN_OAMLEADONLY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->oamleadonly = NO;
				break;
			case IN_YES:
				pCreat->oamleadonly = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%d VALUE FOR oamleadonly=%s FOR %s MUST BE YES OR NO",line,equal_loc);
        printf("%d VALUE FOR oamleadonly=%s FOR %s MUST BE YES OR NO",
               line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_ACTIVE_VHOST_ONLY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->active_vhost_only = NO;
				break;
			case IN_YES:
				pCreat->active_vhost_only = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%d VALUE FOR active_vhost_only=%s MUST BE YES OR NO",line,equal_loc);
        printf("%d VALUE FOR active_vhost_only=%s MUST BE YES OR NO",
               line,equal_loc);
				return(GLfail);
			}
			break;
#endif
    case IN_THIRD_PARTY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->third_party = NO;
				break;
			case IN_YES:
				pCreat->third_party = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%d VALUE FOR third_party=%s MUST BE YES OR NO",line,equal_loc);
        printf("%d VALUE FOR third_party=%s MUST BE YES OR NO",
               line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_RT:
			rt_val = (IN_RT_VAL)INmatch_string(INrt_val,equal_loc,IN_MAX_RT);
			switch(bool_val){
			case IN_NOTRT:
			case IN_RR:
			case IN_FIFO:
				pCreat->isRT = rt_val;
				break;
			default:
				//CR_PRM(POA_INF,"%d VALUE FOR rt=%s MUST BE NO, RR or FIFO",line,equal_loc);
        printf("%d VALUE FOR rt=%s MUST BE NO, RR or FIFO",line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_INHIBIT_SOFTCHK:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				pCreat->inh_softchk = IN_ALWSOFTCHK;
				break;
			case IN_YES:
				pCreat->inh_softchk = IN_INHSOFTCHK;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%d VALUE FOR inhibit_softchk=%s MUST BE YES OR NO",line,equal_loc);
        printf("%d VALUE FOR inhibit_softchk=%s MUST BE YES OR NO",
               line,equal_loc);
				return(GLfail);
			}
			break;
    case IN_PARM_MAX:
      //CR_PRM(POA_INF,"%d INVALID PARAMETER %s",line,token);
      printf("%d INVALID PARAMETER %s",line,token);
			return(GLfail);
		default:
			return(GLfail);
		}
	};
	return(GLsuccess);
}

Short
INprocessInit(const char* name, const char* reqName, int level)
{
	int			msgSz = MHmsgLimit;
	INinitdataReq		statReq("", IN_OP_INIT_DATA);
	
	if(strlen(name) < IN_NAMEMX){
		strcpy(statReq.msgh_name, name);
	}
#ifdef __linux
	else {
		strncpy(statReq.ext_name, name, IN_EPATHNMMX);
	}
#endif
	
	if(statReq.send(destINITname, myQid, 0) != GLsuccess){
    return(128);
	}
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}

	INinitdataResp* statResp = (INinitdataResp*)buffer;
	if(statResp->msgType == INinitdataRespTyp){
		if(statResp->return_val != GLsuccess){
			return(129);
		} 
	} else { 
		return(128);
	}

	
	
	SN_LVL	sn_lvl = SN_LV0;
	if(level == 1) {
		sn_lvl = SN_LV1;
	}
	//INinitSCN init_msg(sn_lvl, ((INopinitdata*)statResp->data)->proctag,
  //                   NULL, TRUE);
  //
	//if(init_msg.send(destINITname, myQid, 0) != GLsuccess){
  //  return(128);
	//}

	msgSz = MHmsgLimit;
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}
	MHmsgBase* initResp = (MHmsgBase*)buffer;
	if(initResp->msgType == INinitSCNAckTyp){
		return(GLsuccess);
	} else if(initResp->msgType == INinitSCNFailTyp){
		return(130);
	} 
	return(128);
}

Short
INprocessCreate(const char* initlist, Bool ucl)
{
	INprocCreate 		pCreat;
	INprocCreateFail*	pFail;
//	INprocCreateAck		pAck;
	int			msgSz;
	
	if(INparseInitlist(initlist, &pCreat) != GLsuccess){
		return(132);
	}

	INinitdataReq	statReq(pCreat.msgh_name, IN_OP_INIT_DATA);
#ifdef __linux
	strcpy(statReq.ext_name, pCreat.ext_path);
#endif
	int count = 0;	
	while(count < 40){
		if(statReq.send(destINITname, myQid, 0) != GLsuccess){
      return(128);
		}
		msgSz = MHmsgLimit;
		if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
      return(128);
		}

		INinitdataResp* statResp = (INinitdataResp*)buffer;
		if(statResp->msgType == INinitdataRespTyp){
			if(statResp->return_val == GLsuccess){
				INopinitdata*	pdata = (INopinitdata*)statResp->data;
				if(pdata->procstep >= IN_BCLEANUP && pdata->procstep <= IN_ECLEANUP){
					IN_SLEEPN(0,300000000);
					count++;
					continue;
				}
				if(!ucl){
					return(130);
				} else {
					return(INprocessInit(pdata->proctag, NULL, 0));
				}
			} 
			break;
		} else { 
			return(128);
		}
	}
	

	if(pCreat.send(destINITname, myQid, 0) != GLsuccess){
    return(128);
	}

	msgSz = MHmsgLimit;

	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}
	pFail = (INprocCreateFail*)buffer;
	if(pFail->msgType == INprocCreateAckTyp){
		/* Should I do something here to make sure that this is for a valid process
		** not some other, delayed Ack?
		*/
		return(GLsuccess);
	} else if(pFail->msgType == INprocCreateFailTyp){
		switch(pFail->ret){
		case INDUPMSGNM:
			return(130);
		case INMAXPROCS:
			return(131);
		}
	} 
	return(133);
}

Short
INprocessRemove(const char* name)
{
	int			msgSz = MHmsgLimit;
	INinitdataReq		statReq("", IN_OP_INIT_DATA);
	
	if(strlen(name) < IN_NAMEMX){
		strcpy(statReq.msgh_name, name);
	}
#ifdef __linux
	else {
		strncpy(statReq.ext_name, name, IN_EPATHNMMX);
	}
#endif
	
	if(statReq.send(destINITname, myQid, 0) != GLsuccess){
    return(128);
	}
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}

	INinitdataResp* statResp = (INinitdataResp*)buffer;
	if(statResp->msgType == INinitdataRespTyp){
		if(statResp->return_val != GLsuccess){
			return(129);
		} 
	} else { 
		return(128);
	}

	INsetRstrt rstrt_msg(((INopinitdata*)statResp->data)->proctag, TRUE, TRUE);
	if(rstrt_msg.send(destINITname, myQid, 0) != GLsuccess){
    return(128);
	}

	char	realName[IN_NAMEMX]; 
	strcpy(realName, ((INopinitdata*)statResp->data)->proctag);

	MHmsgBase* resp = (MHmsgBase*)buffer;

	msgSz = MHmsgLimit;
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}

	if(resp->msgType != INsetRstrtAckTyp){
		return(131);
	} 
	
	//INinitSCN init_msg(SN_LV0, realName, NULL, TRUE);
  //
	//if(init_msg.send(destINITname, myQid, 0) != GLsuccess){
  //  return(128);
	//}

	msgSz = MHmsgLimit;
	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}

	if(resp->msgType == INinitSCNAckTyp){
		return(GLsuccess);
	} else if(resp->msgType == INinitSCNFailTyp){
		return(130);
	} 
	return(128);
}

Short
INnodeRestart(Bool boot, Bool ucl)
{
	int	msgSz = MHmsgLimit;
	SN_LVL	sn_lvl = SN_LV5;

	if(boot){
		sn_lvl = IN_MAXSNLVL;
	}
	
	//INinitSCN init_msg(sn_lvl, NULL, ucl);
  //
	//if(init_msg.send(destINITname, myQid, 0) != GLsuccess){
  //  return(128);
	//}

	if(MHmsgh.receive(myQid, (char*)buffer, msgSz, 0, 3000) != GLsuccess){
    return(128);
	}

	MHmsgBase* msg = (MHmsgBase*)buffer;
	if(msg->msgType == INinitSCNAckTyp){
		return(GLsuccess);
	} 
	
	return(128);
}

Short
INsoftChk(const char* processName, Bool allNodes, Bool clear, char* dest)
{
	int	msgSz = MHmsgLimit;
	char	inhfile[1000];
	int	rack;
	int	chassis;
	int	slot;
  int 	fd;

	INsetSoftChk 	softchk_msg((char*)processName, !clear);

	if(allNodes && strlen(processName) == 0){
		MHmsgh.sendToAllHosts("INIT", (char*)&softchk_msg, sizeof(softchk_msg));
		for(rack = 0; rack < MHmaxRack; rack++){
			for(chassis = 0; chassis < MHmaxChassis; chassis++){
				for(slot = 0; slot < MHmaxSlot; slot++){
					sprintf(inhfile,"/opt/config/servers/%d-%d-%d/local/opt/config/status/softchk", rack, chassis, slot);
          // update the overlay for this host
          if(clear == FALSE){
            int fd;
            if((fd = creat(inhfile,0444)) >= 0) {
              close(fd);
            }
          } else {
            unlink(inhfile);
          }
				}
			}
		}
		sleep(1);
		return(0);
	}

	if(strlen(processName) == 0){
		MHmsgh.send(destINITname, (char*)&softchk_msg, sizeof(softchk_msg));
		sprintf(inhfile,"/opt/config/servers/%s/local/opt/config/status/softchk", dest);
    if(clear == FALSE){
      creat(inhfile,0444);
		} else {
      unlink(inhfile);
		}
		return(0);
	} else if(allNodes){
		MHmsgh.sendToAllHosts("INIT", (char*)&softchk_msg, sizeof(softchk_msg));
		sleep(1);
		return(0);
	} else {
		MHmsgh.send(destINITname, (char*)&softchk_msg, sizeof(softchk_msg));
		return(0);
	}
	
	return(128);
}


int 
main(int argc, char* argv[])
{

	GLretVal	ret;
	Action		action = noAction;
	int		c;
	const char*	initlist = "";
	char 		dest[MHmaxRCSName+1];
	char 		logical[MHmaxNameLen+1];
	const char*	reqName = "";
	int		level = -1;
	const char*	processName = "";
	extern int	optind, optopt;
	extern char*	optarg;
	int		unconditional = FALSE;
	int		boot = FALSE;
	int		allNodes = FALSE;
	int		clear = FALSE;
	Short		hostid;
	Bool		isActive;
	Bool		isUsed;
	Char		myQName[MHmaxNameLen+1];

	if(argc == 1){
		printf("INcmd -a createProc -i initlist_snippet [-d destination_node] [-u]  \n"); 
		printf("INcmd -a initProc -p process_name -n requesting_process -l level [-d destination_node]\n"); 
		printf("INcmd -a removeProc -p process_name -n requesting_process [-d destination_node]\n"); 
		printf("INcmd -a nodeStatus [-d destination_node]\n"); 
		printf("INcmd -a restartNode [-d destination_node] [-b] [-u]\n"); 
		printf("INcmd -a softChk [-d destination_node] [-p process_name] [-c] [-s]\n"); 
		exit(255);
	}

	//strcpy(CRprocname, "INcmd");

	if((ret = MHmsgh.attach()) != GLsuccess){
		printf("Platform software not running, failed to attach ret %d \n", ret);
		exit(254);
	}
	
	if((ret = MHmsgh.getMyHostName(logical)) != GLsuccess){
		printf("Failed to get logical name ret %d \n", ret);
		exit(253);
	}
	if((ret = MHmsgh.logicalToSystem(logical,dest)) != GLsuccess ||
     strncmp(logical,dest, MHmaxRCSName) == 0){
		strcpy(dest, logical);
#ifdef NOTIMP
		printf("No RCS name for %s %s\n", logical, dest);
		exit(252);
#endif
	}

	while((c = getopt(argc, argv,":ubcsi:d:n:p:l:a:")) != -1){
		switch(c){
		case 'u':
       {
         unconditional = TRUE;
         break;
       }
		case 'b':
       {
         boot = TRUE;
         break;
       }
		case 'c':
       {
         clear = TRUE;
         break;
       }
		case 's':
       {
         allNodes = TRUE;
         break;
       }
		case 'i':
       {
         initlist = optarg;
         break;
       }
		case 'd':
       {
         if(strlen(optarg) > MHmaxRCSName){
           printf("Invalid destination name %s length, %d\n", optarg, strlen(optarg));
           exit(251);
         }
         if((ret = MHmsgh.systemToLogical(optarg, logical)) != GLsuccess ||
            strncmp(optarg, logical, MHmaxRCSName) == 0){
           printf("Invalid or unconfigured RCS name %s\n", optarg);
           exit(250);
         }
         strcpy(dest,optarg);
         break;
       }
		case 'a':
       {
         int	act;
         for(act = 0; act < noAction; act++){
           if(strcmp(commands[act], optarg) == 0){
             break;
           }
         }
         if(act == noAction){
           printf("Invalid action %s\n", optarg); 
           return(249);
         } 
         action = (Action)act;
         break;
       }
		case 'p':
       {
         processName = optarg;
         break;
       }
		case 'n':
       {
         reqName = optarg;
         break;
       }
		case 'l':
       {
         level = atoi(optarg);
         break;
       }
		case ':':
       {
         printf("Option -%c requires an operand\n", optopt);
         exit(248);
       }
		case '?':
       {
         printf("Unrecognized option: -%c\n", optopt);
         exit(247);
       }
		}
	}

// printf("action %d, pName %s, req %s, i %s, d %s l %d\n", action, processName, reqName, initlist, dest, level);

	if(access("/sn/init/incmd", F_OK) == 0){
		pid_t	ppid = getppid();
		char	command[1000];
		sprintf(command, "echo `date` pp %d a %d, p %s, req %s, i %s, d %s l %d >> /sn/core/INIT/INcmd; ps -ef >> /sn/core/INIT/INcmd", ppid, action, processName, reqName,initlist, dest, level);
		system(command);
		
	}
	// Verify the destination status
	if((ret = MHmsgh.name2HostId(hostid, dest)) != GLsuccess){
		printf("Cannot convert dest %s to hostid, ret %d\n", dest, ret);
		exit(246);
	}
	if((ret = MHmsgh.Status(hostid, isUsed, isActive)) != GLsuccess){
		printf("Cannot get stauts of dest %s ret %d\n", dest, ret);
		exit(245);
	}

	if(!isActive){
		exit(128);
	}

	sprintf(destINITname, "%s:INIT", dest);
	sprintf(myQName, "INC%d",getpid());

	if((ret = MHmsgh.regName(myQName, myQid, FALSE, TRUE, FALSE, MH_LOCAL, FALSE)) != GLsuccess){
		printf("Failed to register my queue, ret %d\n", ret);
		exit(244);
	}

	switch(action){
	case createProc:
     {
       ret = INprocessCreate(initlist, unconditional);
       break;
     }
	case initProc:
     {
       if(level < 0 || level > 1){
         ret = 244;
         break;
       }
       ret = INprocessInit(processName, reqName, level);
       break;
     } 

	case removeProc: 
     { 
       ret = INprocessRemove(processName);
       break;
     }
	case nodeStatus:
     {
       ret = INgetNodeStatus();
       break;
     }
	case restartNode:
     {
       ret = INnodeRestart(boot, unconditional);
       break;
     }
	case softChk:
     {
       ret = INsoftChk(processName, allNodes, clear, dest);
       break;
     }
	};

	MHmsgh.rmName(myQid, myQName, TRUE);
	return(ret);
}

