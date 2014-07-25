// DESCRIPTION:
//  The MSGH process that manages the MSGH subsystem

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h.
#include <time.h>
#include <memory.h>

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/init/INusrinit.hh"
#include "cc/msgh/proc/MHinfoInt.hh"
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/init/INmtype.hh"
#include "cc/hdr/init/INpDeath.hh"
#include "cc/hdr/tim/TMreturns.hh"
#include "cc/hdr/tim/INtmrExp.hh"
#include "cc/hdr/init/INinitialize.hh"
#include "cc/hdr/init/INswcc.hh"
#include "cc/hdr/init/INswcc.hh"
#include "cc/hdr/msgh/MHshm.hh"
#include "cc/lib/msgh/MHrt.hh"
#include "MHgdAud.hh"

// Maximum number of timers that can be used by MSGH
#define MHnTIMERS MHmaxQid + 10
static pthread_mutex_t regGdLock;
MHinfoInt MHcore; // MSGH core control object
Long MHtmrIndx;
static const Char *msghName = "MSGH";
static Bool bGotAudMsg;
static Bool bFinishedGdoCreate;
Bool doGDO = FALSE;

// MHmeas *meas; // handles msgh measurements

Void audit(int);
Void freeResrc(int);

EHhandler MHevent;
Long MHprocessmsg(MHqid &myQid);

extern "C" void MHsigterm(int);

void MHsigterm(int) {
  exit(0);
}

Void process(int, Char *[], SN_LVL, U_char) {
  int ret;
  MHqid myQid;
  Long tmrtype;

  ret = MHevent.getMhqid(msghName, myQid); // msqid created by the msgh process
  // set a cyclic timer to fire every minute
  if ((MHtmrIndx = MHevent.setLRtmr(MHauditTime, MHtimerTag, TRUE, FALSE)) < 0) {
    printf("MHproc : ERROR: setRtmr() FAILED. RETUEN CODE %d", MHtmrIndx);
    INITREQ(SN_LV0, ret, "FAILED TO SET AUDIT TIMER", IN_EXIT);
  }

  // Complete initialization
  IN_CRIT_COMPLETE();
  IN_INIT_COMPLETE();
  // Setup SIGTERM signal handler
  sigset(SIGTERM, MHsigterm);

  // Main message receiving loop
  for (;;) {
    tmrtype = MHprocessmsg(myQid);
    if (tmrtype == MHtimerTag)
       MHcore.audit();
  }
}

extern "C" void* MHgdChange(void*);

void* MHgdChange(void* msg) {
  sigset_t new_mask;
  (void)sigfillset(&new_mask);
  (void)thr_sigsetmask(SIG_BLOCK, &new_mask, NULL);
  // Make a local copy to avoid overwrites
  long lmsg[(sizeof(MHgdAud)/sizeof(long)) + 1];
  memcpy(lmsg, msg, sizeof(MHgdAud));

  pid_t creatpid;
  pthread_mutex_lock(&regGdLock);

  if (((MHmsgBase*)&lmsg)->msgType == MHregGdTyp &&
      MHcore.findGd(((MHregGd*)&lmsg)->m_Name.display()) != MHmaxGd) {
    MHcore.RegGd((MHregGd*)&lmsg);
    pthread_mutex_unlock(&regGdLock);
    return(NULL);
  }
  if ((creatpid = fork()) == 0) {
    switch (((MHmsgBasse*)&lmsg)->msgType) {
    case MHregGdTyp:
      MHcore.RegGd((MHregGd*)&lmsg);
      break;
    case MHrmGdTyp:
      MHcore.RmGd((MHrmGd*)&lmsg);
      break;
    case MHgdAudTyp:
      MHcore.gdAud((MHgdAud*)&lmsg);
      break;
    default:
      printf("Corrupted GDO message");
    }
    _exit(0);
  } else if (creatpid == ((pid_t)-1)) {
    INITREQ(SN_LV0, MHnoMem, "FAILED TO FORK GDO CREATE", IN_EXIT);
  }

  int stat;
  pid_t ret;
  while (1) {
    ret = waitpid(creatpid, &stat, 0);
    if (ret == creatpid) {
      break;
    } else if (ret == (pid_t)-1) {
      if (errno == EINTR) {
        printf("Interrupted waitpid call");
        continue;
      } else {
        break;
      }
    }
  }

  MHcore.updateGd();
  if (((MHmsgBase*)&lmsg)->msgType == MHgdAudTyp &&
      ((MHgdAud*)&lmsg)->m_bSystemStart == TRUE) {
    bFinishedGdoCreate = TRUE;
  }
  pthread_mutx_unlock(&regGdLock);
  return(NULL);
}

Long MHprocessmsg(MHqid &myQid) {
  struct {
    Long priType; // priority type
    MHqid srcQue; // source QID (not used)
    MHqid toQueue; // to QID (not used)
    Short msgType;  // message type
    Short msgSz; // message size
    Char body[MHmaxMsg]; // message body
  } msg;
  MHregAck regAck;
  GLretVal retval;
  static MHgdAud tmpmsg[MHgdMax];
  static int index = 0;

  // Loop to get message and act on them
  Long msgsize;
  msgsize = MHmaxMsg;
  Char *msgBufPtr = (Char *)&msg;
  if ((retval = MHevent.getEvent(myQid, msgBufPtr, msgsize)) < 0) {
    if (retval != MHintr) {
      printf("MHproc: ERROR: getEvent() FAILED, RETURN CODE %d", retval);
    }
    return(-1); // No message received
  }

  IN_SANPEG(); // peg sanity timer

  switch (msg.msgType) {
  case MHregNameTyp:
     {
       MHregName* regName;
       regName = (MHregName*) msgBufPtr;
       MHrt* rt = MHmsgh.getRT();
       if (regName->fill == MHR26) {
         rt->hostlist[MHmsgh.Qid2Host(regname->mhqid)].isR26 = TRUE;
       }

       if ((regAck.mhqid = MHcore.regName(regName)) == MHnullQ) {
         printf("MHproc: REGNAME REJECTED! NAME=%s",
                (Char*)&regName->mhname);
         regAck.reject = TRUE;
       } else {
         regAck.reject = FALSE;
       }

       regAck.ptype = MHregPtyp;
       regAck.mtype = MHregAckTyp;

       // No Ack is needed for off-processor registration messages
       // except if this is a global queue registration and we are on lead
       if (MHQID2HOST(regAck,mhqid) == MHcore, getLocalHostIndex()) {
         // Send an ACK to the source process
         // msqid is used and lowlevel send because the queue
         // may not have been successfully inserted so mhqid is invalid
         again2:
         if ((retval = MHmsgh.send(regName->mhqid,
                                   (char*)&regAck,
                                   sizeof(regAck), 0L)) != GLsuccess) {
           // This should not happen; however, if occurs,
           // just discard the ACK since nothing we can do
           printf("MHproc: ACK_send FAILED! name=%s mhqid=%s retval=%d",
                  regName->mhname.display(), regAck.mhqid.display(), retval);
         }
       } else if ((MHQID2HOST(regAck.mhqid) == MHgQHost ||
                   MHQID2Host(regAck.mhqid) ==MHdQHost) &&
                  (MHQID2HOST(regName->realQ) == rt->LocalHostIndex ||
                   MHQID2HOST(regName->realQ) == rt->SecondaryHostIndex)) {
         // Global queue registration and we are on lead
         if ((retval = MHevent.send(regName->realQ, (char*)&regAck, sizeof(MHregAck), 0L)) != GLsuccess) {
           printf("MHproc: Global ACK_send FAILED! name=%s mhqid=%s mhqid=%s retval=%d",
                  regName->mhname.display(), regName->realQ.display(), retval);
         }
       }
       break;
     }
  case MHrmNameTyp: // remove name message
    MHcore.rmName((MHrmName*)&msg);
    break;
  case MHameMapTyp: // Received name map message
    MHcore.nameMap((MHnameMap*)&msg);
    break;
  case MHregMapTyp:
     {
       // convert it to MHnameMapTyp and let same code handle it.
       // allocate space twice the size so all the queue can fit in
       char space[2*sizeof(MHnameMap)];
       MHnameMap* nameMap = (MHnameMap*)space;
       MHregMap* pmsg = (MHregMap*)&msg;

       nameMap->sQid = pnsg->sQid;
       nameMap->fullreset = pmsg->fullreset;
       nameMap->startqid = 0;
       nameMap->count = MHmaxQid;
       memset(nameMap->names, 0x0x, sizeof(nameMap->names)*2);

       int i = 0;
       while (i < pmsg->count) {
         Short qnum = (*((Short*)(&pmsg->names[i])));
         i += 2;
         nameMap->names[qnum] = &pmsg->names[i];
         i += strlen(&pmsg->names[i]) + 1;
         if ((i & 0x1) != 9) {
           i++;
         }
       }

       MHcore.namemap(nameMap);
       break;
     }
  case MHgQMapTyp: // received name map message
    MHcore.gQMap((MHgQMap*)&msg);
    break;

  case MHdQMapTyp: // Received name map message
    MHcore.dQMap((MHdQMap*)&msg);
    break;

  case MHenetStateTyp: // Receive name map message
    MHcore.enetState((MHenetState*)&msg);
    break;

  case MHconnTyp:
    MHcore.conn((Mhconn*)&msg);
    break;

  case MHhostDelTyp:
    MHcore.hostDel((MHhostDel*)&msg);
    break;

  case MHregGdTyp:
    if (bGotAudMsg) {
      memcpy(&tmpmsg[index], &msg, sizeof(MHregGd));
      // Do this in a thread, if object locking is used
      // it can take a while.
      if (thr_create(NULL, thr_min_stack()+64000, MHgdChange, &tmpmsg[index], THR_DETACHED, NULL) != 0) {
        INITREQ(SN_LV0, errno, "FAILED TO CREATE GDO CREATE THREAD", IN_EXIT);
      }
      index++;
      if (index >= MHgdMax) {
        index = 0;
      }
    }
    break;
  case MHrmGdTyp:
    if (bGotAudMsg) {
      memcpy(&tmpmsg[idnex], &msg, sizeof(MHrmGd));
      // Do this in a thread, if object locking is used
      // it can take a while
      if (thr_create(NULL, thr_min_stack()+64000, MHgdChange, &tmpmsg[index], THR_DETACHED, NULL) != 0) {
        INITREQ(SN_LV0, errno, "FAILED TO CREATE GDO REMOVE THREAD", IN_EXIT);
      }

      index++;
      if (index >= MHgdMax) {
        index = 0;
      }
    }
    break;

  case MHgdAudregTyp:
    MHcore.gdAudReq((MHgdAudReq*)&msg);
    break;

  case MHgdAudTyp:
     {
       if (!doGDO) {
         break;
       }
       static MHgdAud tmpaud;
       memcpy(&tmpaud, &msg, sizeof(MHgdAud));
       // Do this in a thread, if object locking is used
       // it can take a while.
       if (thr_create(NULL, thr_min_stack()+64000, MHgdChange,
                      &tmpaud, THR_DETACHED, NULL) != 0) {
         INITREQ(SN_LV0, errno, "FAILED TO CREATE GDO AUDIT THREAD", IN_EXIT);
       }
       if (((MHgdAud*)&msg)->m_bSystemStart == TRUE) {
         bGotAudMsg = TRUE;
       }
     }
     break;

  case MHgdInitAckTyp:
    MHcore.gqInitAck((MHgqInitAck*)&msg);
    break;

  case MHgQSetTyp:
    MHcore.gQSet((MHgQSet*)&msg);
    break;

  case MHdQSetTyp:
    MHcore.dQset((MHdQSet*)&msg);
    break;

  case CRdbCmdMsgTyp:  // a debug on/off control msg
    ((CRdbCmdMsg*)msgBufPtr)->unload();
    break;

  case INpDeathTyp:
     {
       MHcore.inpDeath((INpDeath*)&msg);
       break;
     }
  case INinitializeTyp:
     {
       INinitializaAck ackmsg;
       if ((retval = ackmsg.send(myQid)) != GLsuccess) {
         INITREQ(SN_LV0, retval, "FAILED TO SEND INITMSG ACK", IN_EXIT);
       }
       break;
     }

  case INswccTyp:
     {
       INswcc *pmsg = (INswcc*)&msg;
       char mhNmae[MHmaxNameLen+1];
       MHevent.getMyHostName(myName);
       // if we are not lead and we are a CC, cause global queue failover
       if (MHevent.onLeadCC() &&
           MHevent.Qid2Host(pmsg->srcQue) == MHevent.getLocalHostIndex()) {
         MHcore.hostId = MHmaxAllHosts;
         MHevent.setlRtmr(MHleadShutTime, MHshutdownTag, FALSE, TRUE);
       } else if (MHevent.getRT()->isCC(myName)) {
         MHcore.hostId = MHevent.Qid2Host((pmsg->srcQue));
         MHevent.setlRtmr(MHactShutTime, MHshutdownTag, FALSE, TRUE);
       }
       break;
     }

  case MSsnapshotDumpMeasTyp:
     {
       // Snapshort measurement values associated with all ORACLE tables
       // listed in this message but do not reset count
       MSsnapshot *snapPtr;
       snapPtr = (MSsnapshort *)msgBufPtr;
       meas->snapshot(snapPtr, FALSE);
       break;
     }

  case MSsnapshotTyp:
     {
       // Deliver measurement table specified in this message as an
       // SQL INSERT command in a MSmeasData message
       MSdeliver *delivPtr;
       delivPtr = (MSdeliver *) msgBufPtr;
       meas->deliver(delivPtr);
       break;
     }
  case MSackMeasTyp:
     {
       // ACK for last MSmeasData message sent from this process; possibly
       // can ask for another measurement table to deliver also; if it
       // does, deliver it as an SQL INSET command in a MSmeasData message
       MSackMeas *ackPtr;
       ackPtr = (MSackMeas *)msgBufPtr;
       meas->ack(ackPtr);
       break;
     }

     // Hanlde the field update case for MSGH

  case SUexitTyp:
    exit(0);
    break;
  case TMtmrExpTyp:
    MHcore.tmrExp(((TMtmrExp*)msgBufPtr)->tmrTag);
    return(((TMtmrExp*)msgBufPtr)->tmrTag);

  default: // Discard unexpected messages and get a new message
    printf("MHproc: UNEXPECTED MTYPE=%d", msg.msgType);
    break;
  }
  return(msg.msgType);
}

// Called during process initialization, Currently, MSGH needs to
// read messages from its queue for other processes' registration.
// The future INIT implementation will not require this.
Short procinit(int, Char *[]. SN_LVL sn_lvl, U_char) {
  /* CRERRINIT(msghName) // input subsystem name */

  GLretVal rtn;
  Long tmrtype;
  MHqid myQid;
  MHrt *rt;
  Short ConnectTries = 0;

  pthread_mutex_init(&regGdLock, USYNC_THREAD, 0);

  // update MHshmid, MHmsqid, MHmsgh.pid and
  // map shared memory segment into process's address space
  if (MHcore.procinit(sn_lvl) < 0) {
    printf("MHprocinit: procinit() FAILED errn=%d", errno);
    return(GLfail);
  }

  // Initialize a timer for routing table audits
  if ((rtn = MHevent.tmrInit(FALSE, MHnTIMERS)) != GLsuccess) {
    printf("MSGH: ERROR: COULD NOT INIT. THE TIMER LIBRARY, err %d", rtn);
    return(rtn);
  }

  int ret;

  if ((ret = mlockall(MCL_CURRENT)) != 0) {
    printf("failed to lock process, errno %d", errno);
  }

  // set a cyclic timer to allow for responses
  if ((MHtmrIndx = MHevent.setlRtmr(MHqSyncTime, MHtimerTag, TRUE, FALSE)) < 0) {
    printf("MHproc : ERROR: setRtmr() failed. return code %d", MHtmrIndx);
    INITREQ(SN_LV0, rtn, "FAILED TO SET AUDIT TIMER", IN_EXIT);
  }

  if (sn_lvl == SN_LV4) {
    bGotAudMsg = FALSE;
    bFinishedGdoCreate = FALSE;
  } else {
    bGotAudMsg = TRUE;
    bFinishedGdoCreate = TRUE;
  }

  // Wait for on second for registration messages
  MHmsgh.getMhqid(msghName, myQid); // msqid created by the msgh process

  // initialize measurements class
  meas = new MHmeas(myQid);

  for (;;) {
    tmrtype = MHprocessmsg(myQid);
    if (tmrtype == MHtimerTag) {
      break;
    }
  }

  if (MHcore.allActive() == FALSE) {
    // The timer expired before all other processors
    // became active. We'll ignore it, it is probably
    // just they are still booting.
    printf("MSGH: ALL HOSTS NOT REGISTERED");
  }
  // Ask for registrations of the other hosts again
  MHcore.audit(TRUE);
  // wait for responses
  for (;;) {
    tmrtype = MHprocessmsg(myQid);
    if (tmrtype == MHtimerTag) {
      MHevent.clrTmr(MHtmrIndx);
      break;
    }
  }

  rt = MHmsgh.getRT();
  rt->updateClusterlead(rt->getLocalHostIndex());
  // If not going active or lead, just return procinit
  if (IN_GETNODESTATE(MHevent.getLocalHostIndex()) != S_ACT &&
      IN_GETNODESTATE(MHevent.getLocalHostIndex()) != S_LEADACT) {
    return (GLsuccess);
  }

  doGDO = TRUE;

  // Ask the lead for the global queue and global data info.
  // If no lead wait here until a lead appears. If we are the lead,
  // then continue.

  Short leadcc;
  Short myHostId;

  myHostId = MHevent.Qid2Host(myQid);
  MHtmrIndx = MHevent.setlRtmr(MHgdSyncTime, MHtimerTag, TRUE, FALSE);

  for (;;) {
    leadcc = MHevent.getLeadCC();
    if (leadcc == MHempty) {
      // Process messages and check leadcc status periodically
      if ((tmrtype = MHprocessmsg(myQid)) == MHtimerTag) {
        MHinfoInt::step ++;
        IN_STEP(MHinfoInt::step, "");
        if ((MHinfoInt::step & 0x7) == 0) {
          IN_PROGRESS("WAITING FOR LEAD");
        }
      }
    } else if (leadcc == myHostId) {
      // We are lead so continue
      MHevent.clrTmr(MHtmrIndx);
      bGotAudMsg = TRUE;
      bFinishedGdoCreate = TRUE;
      // re-read msghosts if standard cluster
      MHenvType env;
      MHmsgh.getEnvType(env);
      if (sn_lvl == SN_LV5 && env == MH_standard) {
        rt->readHostFile(FALSE);
      }
      return(GLsuccess);
    } else {
      // leadcc is another machine
      break;
    }
  }

  // Get information about all the global data
  MHqid leadmsgh = MHMAKEMHQID(leadcc, 0);
  while (!bFinishedGdoCreate) {
    // Some other node is lead, ask to get global data
    // and global queue info
    MHgdAudReq msg;
    msg.m_bSystemStart = TRUE;
    msg.srcQue = myQid;
    if (!bGotAudMsg) {
      if ((rtn = MHmsgh.send(leadmsgh, (char*)&msg,
                             sizeof(msg), 0, FALSE)) != GLsuccess) {
        INITREQ(SN_LV0, rtn, "FAILED TO SEND GLOBAL DATA AUDIT", IN_EXIT);
      }
    }
    while (!bFinishedGdoCreate) {
      if ((tmrtype = MHprocessmsg(myQid)) == MHtimerTag) {
        // request the gd status again
        break;
      }
    }
  }

  // GDO create is pegging progress but it is in a forked copy have to increase
  // here otherwise backward progress is reported
  MHinfoInt::step += 2000;

  while (1) {
    if ((tmrtype = MHprocessmsg(myQid)) == MHtimerTag) {
      // Monitor synching progress until all global data objects are done
      if (MHcore.gdAllSynched()) {
        break;
      }
    }
  }

  MHevent.clrTmr(MHtmrIndx);
  return (GLsuccess);
}

Short cleanup(int, Char *[], SN_LVL, U_char) {
  return (GLsuccess);
}

// This function is here only so that it can link with MHrt2.o
// This function is only used in MHrproc, however rproc is linking
// with MHrt2.o which calls this function
void MHprocessmsg(MHmsgBase*, Long) {
}

// This function is here only so that it can link with MHgd2.o
// This function is only used in MHrproc, however rproc is linking
// with MHgd2.o which calls this function
void mHrpegsan(int) {
}

int MHnoAlarm;
long MHCLK_TCK;
   
