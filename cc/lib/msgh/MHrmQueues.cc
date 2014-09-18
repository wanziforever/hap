// DESCRIPTION:
// 	This file defines a function - MHrmQueues(), which will be invoked
//	by INIT to remove all the queues controlled by MSGH.
//	INIT-controlled processes.
//
// NOTES:
//

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHresult.hh"
#include "cc/hdr/msgh/MHrt.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

// Remove all the queues controlled by INIT from the system
GLretVal
MHrmQueues()
{
  return (GLsuccess);
}

#define MHmaxClrMutex	50

char MHmutexList[MHmaxClrMutex][MHmaxNameLen + 1];

// Return true if this machine is a CC
Bool
MHisCC(Bool& isSimplex, Short& hostId)
{
  const Char *msghost = "/sn/msgh/msghosts.cc";

  FILE *fs;
  Char line[2048];
  Char * startptr;

	memset(MHmutexList, 0x0, sizeof(MHmutexList));

	isSimplex = TRUE;
	fs = fopen(msghost, "r");
  if(fs == NULL) {
    //CRDEBUG(CRmsgh+1, ("MHrt::readHostFile: Failed to open file %s, errno %d", msghost, errno));
    printf("MHrt::readHostFile: Failed to open file %s, errno %d\n",
           msghost, errno);
		return(TRUE);
  }

  Short hostnum, lineno = 0;
  Char * logicalname;
	Char  locallogical[MHmaxNameLen+1];
	memset(locallogical, 0, sizeof(locallogical));
  Char * realname;
  Char * localname;
  struct utsname un;
	int 	nCCs = 0;
  if(uname(&un) < 0)
  {
    //CRERROR("MHisCC: failed to get local machine name errno %d", errno);
    printf("MHisCC: failed to get local machine name errno %d\n",
           errno);
    return(TRUE);
  }

  localname = un.nodename;

  //CRDEBUG(CRmsgh+1, ("MHisCC: local name is '%s'",localname));
  printf("MHisCC: local name is '%s'\n",localname);

  Bool localfound = FALSE;

	while(fs != NULL && fgets(line, 2048, fs))
	{
		lineno++;
		if(line[0] == '#' || line[0] == 0)
       continue;

		// Skip any initial white space & get host number.
		startptr = strtok(line, " \t");
		if(startptr == NULL)
       continue;	// Ignore line since no tokens

		hostnum = (Short) strtol(startptr, (char **)0, 0);
		if(hostnum < 0 || hostnum >= MHmaxHostReg)
		{
			//CRERROR("MHisCC(%s'%d): bad hostnum %d, skipping",msghost,lineno,hostnum);
      printf("MHisCC(%s'%d): bad hostnum %d, skipping",
             msghost,lineno,hostnum);
			continue;
		}

		// Skip more white space and get logical hostname.
		startptr = strtok((Char *const)0, " \t");
		if(startptr == NULL)
		{
			//CRERROR("MHisCC(%s'%d): no logical host name for %d, skipping",msghost,lineno,hostnum);
      printf("MHisCC(%s'%d): no logical host name for %d, skipping\n",
             msghost,lineno,hostnum);
			continue;
		}
		logicalname = startptr;

		int i;
		for(i = 0; i < MHmaxNets; i++){
			// Skip more white space and get interface info.
			startptr = strtok((Char *const)0, " \t");
			if(startptr == NULL)
			{
				//CRERROR("MHisCC(%s'%d): no enet%d name for %d logical name %s, skipping",msghost,lineno,i,hostnum,logicalname);
        printf("MHisCC(%s'%d): no enet%d name for %d logical name %s, skipping\n",
               msghost,lineno,i,hostnum,logicalname);
				break;
			}
		}
		
		if(i < MHmaxNets){
			continue;
		}

		// Skip more white space and get hostname.
		startptr = strtok((Char *const)0, " \t");
		if(startptr == NULL)
		{
			//CRERROR("MHisCC(%s'%d): no real host name for %d logical name %s, skipping",msghost,lineno,hostnum,logicalname);
      printf("MHisCC(%s'%d): no real host name for %d logical name %s, skipping\n",
             msghost,lineno,hostnum,logicalname);
			continue;
		}
		realname = startptr;

		// Take "\n" off name
		int reallen = strlen(realname);
		if(realname[reallen-1] == '\n')
       realname[reallen-1] = 0;


		if(strcmp(localname,realname) == 0)
		{
			localfound = TRUE;
			strcpy(locallogical, logicalname);
			hostId = hostnum;
		}

		// Check to see if CCs is present
		if(strncmp(logicalname, "cc", 2) == 0){
			nCCs++;
		}
	}

	if(localfound != TRUE) {
		if(fs != NULL){
			//CRERROR("MHisCC: local host name %s not found in %s", localname, msghost);
      printf("MHisCC: local host name %s not found in %s\n",
             localname, msghost);
		} else {
			isSimplex = TRUE;
		}
		return(TRUE);
	}

	if(fs != NULL && fclose(fs) != 0) {
		//CRERROR("MHisCC: Failed to close file %s, errno %d", msghost, errno);
    printf("MHisCC: Failed to close file %s, errno %d\n",
           msghost, errno);
		return(TRUE);
	}

	if(nCCs == 2){
		isSimplex = FALSE;
	} else {
		isSimplex = TRUE;
	}

	return(TRUE);

}

Void
MHinfoExt::setLeadCC(MHqid qid)
{
	if(isAttach == FALSE){
		//CRERROR(("Cannot set lead CC, MHmsgh is not attached"));
    printf("Cannot set lead CC, MHmsgh is not attached");
		return;
	}
	rt->m_leadcc = MHQID2HOST(qid);
}

Void
MHinfoExt::setOamLead(MHqid qid)
{
	if(isAttach == FALSE){
		//CRERROR(("Cannot set OAM lead, MHmsgh is not attached"));
    printf("Cannot set OAM lead, MHmsgh is not attached");
		return;
	}
	rt->m_oamLead = MHQID2HOST(qid);
}

Void
MHinfoExt::setActiveVhost(MHqid qid)
{
	if(isAttach == FALSE){
		//CRERROR(("Cannot set Vhost lead, MHmsgh is not attached"));
    printf("Cannot set Vhost lead, MHmsgh is not attached");
		return;
	}
	rt->m_vhostActive = MHQID2HOST(qid);
}

Bool
MHinfoExt::audMutex()
{
	if(isAttach == FALSE){
		return(FALSE);
	}
	return(rt->AuditMutex());
}

// This function returns TRUE if mutex was forcibly cleared
// FALSE otherwise

Bool MHrt::AuditMutex()
{
  static U_long   lastcount = 0;
  static U_long   lastMsgLock = 0;
  static U_long   lastdqLock = 0;
  Bool            retlock = 0;
	int		firstFree;

	for(firstFree = 0; firstFree < MHmaxClrMutex - 1 && 
         MHmutexList[firstFree][0] != 0; firstFree++);
         
  // If lock count incremented, return FALSE
  if(m_lockcnt != lastcount){
    lastcount = m_lockcnt;
  } else if(mutex_trylock(&m_lock) == 0){
    mutex_unlock(&m_lock);
  } else {
    // This was forcible unlock
    mutex_unlock(&m_lock);
    retlock++ ;
		if(firstFree < MHmaxClrMutex - 1){
			strcpy(MHmutexList[firstFree], "main_mutex");
			firstFree ++;
		}
  }

  // If dq lock count incremented, return FALSE
  if(m_dqcnt != lastdqLock){
    lastdqLock = m_dqcnt;
  } else if(mutex_trylock(&m_dqLock) == 0){
    mutex_unlock(&m_dqLock);
  } else {
    // This was forcible unlock
    mutex_unlock(&m_dqLock);
    retlock++ ;
		if(firstFree < MHmaxClrMutex - 1){
			strcpy(MHmutexList[firstFree], "dq_mutex");
			firstFree ++;
		}
  }

  if(m_msgLockCnt != lastMsgLock){
    lastMsgLock = m_msgLockCnt;
  } else if(mutex_trylock(&m_msgLock) == 0){
    mutex_unlock(&m_msgLock);
  } else {
    // This was forcible unlock
    mutex_unlock(&m_msgLock);
    retlock++ ;
  }

	MHqData*	pData;
	int		ret;
	for(pData = localdata; pData < localdata + MHmaxQid; pData++){
		if(!pData->inUse){
			continue;
		}
		if(pData->m_qLockCnt != pData->m_qLockCntLast){
			pData->m_qLockCntLast = pData->m_qLockCnt;
		} else if((ret = mutex_trylock(&pData->m_qLock)) == 0){
			pData->m_qLockCnt++;
			mutex_unlock(&pData->m_qLock);
    } else if(ret == EBUSY){
      struct timespec         tsleep;
      tsleep.tv_sec = 0;
      tsleep.tv_nsec = 10000000;
      nanosleep(&tsleep, NULL);
			if(pData->m_qLockCnt != pData->m_qLockCntLast){
				pData->m_qLockCntLast = pData->m_qLockCnt;
      } else if((ret = mutex_trylock(&pData->m_qLock)) == 0){
        pData->m_qLockCnt++;
        mutex_unlock(&pData->m_qLock);
      } else if(ret == EBUSY && pData->m_qLockPid > 0 && kill(pData->m_qLockPid, 0) >= 0){
        mutex_unlock(&pData->m_qLock);
        retlock++;
        char    queue[MHmaxNameLen + 1];
        MHqid   qid = MHMAKEMHQID(LocalHostIndex, pData - localdata);
        if(firstFree < MHmaxClrMutex - 1 && findMhqid(qid, queue) == GLsuccess){
          strcpy(MHmutexList[firstFree], queue);
          firstFree ++;
        }
      } else {
        mutex_unlock(&pData->m_qLock);
      }
		}
	}
	
	MHmutexList[MHmaxClrMutex -1][0] = 0; 
 
  if(retlock){
    return(TRUE);
  } else {
    return(FALSE);
  }
}
