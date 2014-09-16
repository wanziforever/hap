#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/init/INsharedMem.hh"
#include "cc/hdr/init/INusrinit.hh"
#include "cc/proc/msgh/MHinfoInt.hh"
#include "cc/proc/msgh/MHgdAud.hh"
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/linux/luc_compat.h"

Long MHinfoInt::step = 0;

// Called when the system is booted or when the system goes through
// a system reset initialization. Allocate a shared memory segment and
// a message queue if they do not exist. In any case, the routine will
// free all the allocated queues, if any, and initialize the MSGH
// control information structure and routing table.
// (-1) is returned when it failed.
Short MHinfoInt::sysinit() {
  Bool noExist; // flag indicates if shared memory exists
  GLretVal retval;
  int shmid;
  char *pBuffer;

  // create a shared memory segment if it is does not exists
  if ((shmid = INshmem.allocSeg(MHkey, sizeof(MHrt), 0666, noExist)) < 0) {
    printf("MHsysinit: allocSeq() FAILED! ret=%d, errn=%d", shmid, errno);
    return (-1);
  }

  // Map shared memory into process's address space
  if ((rt = (MHrt*)shmat(shmid, (Char*)0, 0)) == (MHrt*)-1) {
    printf("MHsysinit: shmat() FAILED! errn=%d", errno);
    return (-1);
  }
  // free allocated queues, if any, and initialize routing table
  rt->rtInit(noExist);

  int bufferSz = (rt->m_n256 << 8) + (rt->m_n1024 << 10) +  \
     (rt->m_n4096 << 12) + (rt->m_n16384 << 14) + rt->m_n256 +  \
     rt->m_n1024 + rt->m_n4096 + rt->m_n16384;
  bufferSz += rt->m_NumChunks + (rt->m_NumChunks * MHdataChunk);

  // create a the buffer shared memory segment if it does not exist
  if ((shmid = INshmem.allocSeg(MHkey+1, bufferSz, 0666, noExist)) < 0) {
    printf("MHsysinit: allocSeq() FAILED! ret=%d errn=%d sz %d",
           shmid, errno, bufferSz);
    return (-1);
  }
  // Map shared memory into process's address space
  if ((pBuffer = (char *) shmat(shmid, (Char*)0,
                                SHM_SHARE_MMU)) == (char *) -1) {
    printf("MHsysinit: shmat() FAILED! errn=%d", errno);
    return(-1);
  }

  memset(pBuffer, 0x1, rt->m_n256 + rt->m_n1024 + rt->m_n4096 + \
         rt->m_n16384 + rt->m_NumChunks);
  // Tell rt to read the host file and store the data
  retval = rt->readHostFile(TRUE);
  if (retval != GLsuccess) {
    printf("MHsysinit: readHostFile() FAILED! retval=%d", retval);
    return (retval);
  }
  shmdt((char*)rt);
  shmdt((char*)pBuffer);
  return (0);
}

extern EHhandler MHevent;

// Called during process initialization. Note that this routine is called
// based on assumption that shared memory segment and message queue already
// exist. (-1) is returned when it failed.
Short MHinfoInt::procinit(SN_LVL sn_lvl) {
  Bool noExist; // flag indicates if shared memory exists
  Short retval;
  Bool deleteQ;
  GLretVal rtn;

  if ((rtn = MHevent.attach()) != GLsuccess) {
    printf("Failed to attach to MSGH, ret %d", rtn);
    return(GLfail);
  }
  pid = getpid(); // get process id
  rt = MHevent.getRT();

  int shmid;
  // Find and create all global data objects
  memset(gdName, 0x0, sizeof(gdName));
  for (int i = 0; i < MHmaxGd; i++) {
    if ((shmid = INshmem.getSeg(TRUE, (IN_SHM_KEY)i)) >=0) {
      rt->m_gdCtl[i].m_shmid = shmid;
      gd[i] = new MHgd;
      if ((rtn = gd[i]->attach(rt->m_gdCtl[i].m_shmid,rt)) != GLsuccess) {
        printf("Failed to attach %d, retval %d", i, rtn);
      }
      strcpy(gdName[i], gd[i]->getName());
      gd[i]->detach();
    } else {
      rt->m_gdCtl[i].m_shmid = -1;
      rt->m_gdCtl[i].m_RprocIndex = -1;
    }
  }

  // If not level 0, re-read host file
  // Do not read on sbc, takes too much time
  if (sn_lvl != SN_LV0 && strncmp(rt->hostlist[rt->getLocalHostIndex()]. \
                                  hostname.display(), "sb", 2) != 0) {
    rt->readHostFile(FALSE);
  }

  // SN_LV4 is active initialization, SecondaryHostIndex only has
  // meaning on lead
  if (sn_lvl == SN_LV4 || rt->m_envType == MH_peerCluster ||
      (sn_lvl != SN_LV5 && rt->m_leadcc != rt->LocalHostIndex)) {
    rt->SecondaryHostIndex = MHnone;
  }

  if (rt->insertName(MHmsghName, MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ),
                     pid, MH_allNodes, FALSE, 0, TRUE, IN_Q_SIZE_LIMIT(),
                     FALSE, MHnullQ, FALSE, FALSE, IN_MSG_LIMIT()) == MHnullQ) {
    printf("MHprocinit: insertName FAILED, pid=%d", pid);
    return (-1);
  }
  return (GLsuccess);
}

// Scan the routing table and free all the resources allocated to those
// dead process. Note that the MSGH process can only remove those UNIX
// queues associated with the processes wtih the same user ID when it is
// not running under superuser.
Void MHinfoInt::audit(Bool sendMSGH) {
  Short ret;
  static unsigned int count = 0;
  Bool deleteQ;
  struct msqid_ds status;

  if (sendMSGH) {
    // Just insert MSGH name to trigger far end table downloads
    if (rt->insertName(MHmsghName,
                       MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ),
                       pid, MH_allNodes, TRUE, 0, TRUE, IN_Q_SIZE_LIMIT(),
                       FALSE, MHnullQ, FALSE, FALSE,
                       IN_MSG_LIMIT()) == MHnullQ) {
      printf("MHaudit: insetName FAILED pid=%d", pid);
    }
    return;
  }

  int i;

  for (i = 0; i < MHmaxQid; i++) {
    if (rt->localdata[i].inUse = TRUE) {
      if (rt->localdata[i].pid == MHempty ||    \
          kill(rt->localdata[i].pid, 0) < 0) {
        rt->localdata[i].auditcount++;
        // errnno=EPERM impies that effective user id of the MSGH
        // process is not super-user, and neither its real nor
        // effective user id matches the real or saved set-user
        // ID of the receiving process. (IGNORE !!)
        if (errno != EPERM) {
          MHname mhname;
          if (rt->localdata[i].auditcount > MHmaxAuditCount) {
            if (rt->hostlist[rt->getLocalHostIndex()].  \
                indexlist[i] != MHempty) {
              mhname = rt->rt[rt->hostlist[rt->getLocalHostIndex()].  \
                              indexlist[i]].mhname;
            } else {
              rt->localdata[i].inUse = FALSE;
              mhname = "";
            }
            printf("MHaidt: CLEANED UP OLD QUEUE FOR \n");
            printf("MSQID=%d PID=%d NAME=%s",
                   i, rt->localdata[i].pid, mhname.display());
            rt->deleteName(mhname, MHMAKEMHQID(rt->getLocalHostIndex(), i),
              rt->localdata[i].pid);
          }
        }
      }
    }
  }

  char name[MHmaxNameLen+1];
  // If on lead, make sure that the real queues mapped to global
  // and distributive queues are still there
  if (rt->LocalHostIndex == rt->m_leadcc) {
    for (i = MHclusterGlobal; i < MHsystemGlobal; i++) {
      for (int j = 0; j < MHmaxRealQs; j++) {
        if (rt->gqdata[i].m_realQ[j] != MHnullQ) {
          MHname mhname = rt->rt[rt->hostlist[MHgQHost].indexlist[i]].mhname;
          if (MHmsgh.getName(rt->gqdata[i].m_realQ[j], name) != GLsuccess) {
            printf("MHaudit: CLEANED UP OLD, REAL QUEUE FOR\n");
            printf("MHQID=%s NAME=%s", rt->gqdata[i].m_realQ[j].display(),
                   mhname.display());
            rt->gqdata[i].m_realQ[j] = MHnullQ;
            if (rt->gqdata[i].m_selectedQ == j) {
              rt->gqdata[i].m_selectedQ = MHempty;
              rt->setGlobalQ(i);
            }
          }
        }
      }
    }
    for (i = 0; i < MHmaxQid; i++) {
      for (int j = 0; j < MHmaxDistQs; j++) {
        if (rt->gqdata[i].m_realQ[j] != MHnullQ) {
          MHname mhname = rt->rt[rt->hostlist[MHgQHost].indexlist[i]].mhname;
          if (MHmsgh.getName(rt->gqdata[i].m_realQ[j], name) != GLsuccess) {
            printf("MHaudit: CLEANED UP OLD REAL QUEUE FOR \n");
            printf("MHQID=%s name=%s",
                   rt->gqdata[i].m_realQ[j].display(), mhname.display());
            rt->gqdata[i].m_realQ[j] = MHnullQ;
            rt->dqdata[i].m_enabled[j] = FALSE;
            rt->setDistQ(i);
          }
        }
      }
    }
  }
  if (rt->LocalHostIndex == rt->m_clusterLead ||
      rt->SecondaryHostIndex == rt->m_clusterLead) {
    for (i = 0; i < MHclusterGlobal; i++) {
      for (int j = 0; j < MHmaxRealQs; j++) {
        if (rt->gqdata[i].m_realQ[j] != MHnullQ) {
          MHname mhname = rt->rt[rt->hostlist[MHgQHost].indexlist[i]].mhname;
          if (MHmsgh.getName(rt->gqdata[i].m_realQ[i], name) != GLsuccess) {
            printf("MHaudit: CLEANED UP OLD REAL QUEUE FOR\n");
            printf("MHQID=% NAME=%s",
                   rt->gqdata[i].m_realQ[j].display(), mhname.display());
            rt->gqdata[i].m_realQ[j] = MHnullQ;
            if (rt->gqdata[i].m_selectedQ == j) {
              rt->gqdata[i].m_selectedQ = MHempty;
              rt->setGlobalQ(i);
            }
          }
        }
      }
    }
  }

  if (rt->LocalHostIndex == rt->m_oamLead) {
    for (i = MHsystemGlobal; i < MHmaxQid; i++) {
      for (int j = 0; j < MHmaxRealQs; j++) {
        if (rt->gqdata[i].m_realQ[i] != MHnullQ) {
          MHname mhname = rt->rt[rt->hostlist[MHgQHost].indexlist[i]].mhname;
          if (MHmsgh.getName(rt->gqdata[i].m_realQ[j], name) != GLsuccess) {
            printf("MHaudi: CLEANED UP OLD REAL QUEUE FOR \n");
            printf("MHQID=%s NAME=%s",
                   rt->gqdata[i].m_realQ[j].display(), mhname.display());
            rt->gqdata[i].m_realQ[j] = MHnullQ;
            if (rt->gqdata[i].m_selectedQ == j) {
              rt->gqdata[i].m_selectedQ = MHempty;
              rt->setGlobalQ(i);
            }
          }
        }
      }
    }
  }

  MHgdAudReq audreq;
  audreq.m_bSystemStart = FALSE;

  for (i = 0; i < MHmaxHostReg; i++) {
    rt->hostlist[i].nRouteSync -= 2;
    if (rt->hostlist[i].nRouteSync < 0) {
      rt->hostlist[i].nRouteSync = 0;
    }
    if ((i & 0x7) == (count & 0x7)) {
      if (rt->hostlist[i].ping > 1 && ((count >> 3) & 0x1) == 0) {
        // for nodes with longer ping internal, sync less frequently
        continue;
      }
      rt->checkNetAudit(MHmsghName, MHMAKEMHQID(i, MHmsghQ), i, FALSE);
      if ((rt->hostlist[i].isactive) &&
          (i != rt->LocalHostIndex) &&
          (rt->m_envType != MH_peerCluster) &&
          strncmp(rt->hostlist[i].hostname.display(), "as", 2) != 0 &&
          (rt->LocalHostIndex == rt->m_leadcc)) {
        audreq.srcQue = MHMAKEMHQID(i, MHmsghQ);
        gdAudReq(&audreq);
      }
    }
  }
  if (!(count & 0x1f)) {
    // Audit the buffers every 16 min
    MHmsgh.auditQueues();
  }
  count ++;
}

Bool MHinfoInt::allActive() {
  for(int i = 0; i < MHmaxHostReg; i++) {
    if(rt->hostlist[i].isused == FALSE)
       continue;
    if(rt->hostlist[i].isactive == FALSE)
       return(FALSE);
  }
  return(TRUE);
}

// Function to check to see if all hosts are active.
// We wait 10 seconds during procinit to see if they all
// respond to the query
void MHinfoInt::switchNets() {
  for (int i = 0; i < MHmaxHostReg; i++) {
    if (rt->hostlist[i].isused == FALSE) {
      continue;
    }
    if (rt->hostlist[i].isactive == FALSE) {
      rt->hostlist[i].SelectedNet =
         (rt->hostlist[i].SelectedNet + 1) & MHmaxNets;
    }
  }
}

#ifdef _LP64
#define MHmaxTotGDOSz 0xffffffffff // maximum GDO size per MHGDPROC
#define MHmaxGDProcs 1
#else
#define MHmaxTotGDOSz 0xB0000000 // Maximum GDO size per MHGDPROC
#define MHmaxGDProcs 4
#endif

void MHinfoInt::RegGd(MHregGd* regMsg) {
  int gd_idx = MHmaxGd;
  GLretVal retval;
  U_LongLong gdProcsSz[MHmaxGDProcs];
  int i;

  // If on lead CC, try to find the object, otherwise create one
  if (MHmsgh.onLeadCC()) {
    if (regMsg->m_shmkey != MHempty) {
      printf("Received initialized shmkey %d", regMsg->m_shmkey);
    }

    gd_idx = findGd(regMsg->m_Name.display());

    if (gd_idx == MHmaxGd) {
      for (i = 0; i < MHmaxGd; i++) {
        if (gd[i] != NULL) {
          if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
            printf("Failed to attach %d, retval %d", i, retval);
            continue;
          }
          if (strcmp(gd[i]->getName(), regMsg->m_Name.display()) == 0) {
            gd_idx = 1;
            gd[i]->detach();
            break;
          }
          gd[i]->detach();
        } else if (gd_idx == MHmaxGd) {
          gd_idx =i;
        }
      }

      if (i == MHmaxGd) {
        // Did not find the entry, if object were not to be created
        // return failure
        if (!regMsg->m_bCreate) {
          MHregGdAck ackMsg;
          ackMsg.m_RetVal = MHnoEnt;
          ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
          return;
        }
        if (gd_idx < MHmaxGd) {
          gd[gd_idx] = new MHgd;
        } else {
          // no more room in gd table, fail to create
          MHregGdAck ackMsg;
          ackMsg.m_RetVal = MH2Big;
          ackMsg.send(regMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
          return;
        }
      }
    }
    regMsg->m_shmkey = gd_idx;
  } else {
    gd_idx = regMsg->m_shmkey;
    if (gd_idx == -1 || gd_idx >= MHmaxGd) {
      // do something
      printf("gd_idx = %d out of range", gd_idx);
      return;
    }
    if (rt->m_gdCtl[gd_idx].m_shmid != -1) {
      if (gd[gd_idx] != NULL) {
        if ((retval = gd[gd_idx]->attach(rt->m_gdCtl[gd_idx].m_shmid,
                                         rt)) != GLsuccess) {
          printf("Failed to attach %d, retval %d, gd_idx, retval");
        }
      }
      if (gd[gd_idx] == NULL ||
          strcmp(gd[gd_idx]->getName(), regMsg->m_Name.display()) != 0) {
        // That segment better be empty or it should have the same name
        printf("Non empty name %s != %s",
               gd[gd_idx]->getName(), regMsg->m_Name.display());
        gd[gd_idx]->detach();
        return;
      }
      gd[gd_idx]->detach();
    } else {
      if (gd[gd_idx] != NULL) {
        printf("gd_idx %d should be emppty", gd_idx);
        return;
      }
      gd[gd_idx] = new MHgd;
    }
  }

  if (rt->m_gdCtl[gd_idx].m_shmid == -1) {
    memset(gdProcsSz, 0x0, sizeof(gdProcsSz));
    // figgure out which MHGDPROC should have this GDO
    for (i = 0; i < MHmaxGd; i++) {
      if (gd[i] != NULL && rt->m_gdCtl[i].m_shmid != -1) {
        if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
          printf("Failed to attach %d, retval %d", i, retval);
          continue;
        }
        gdProcsSz[rt->m_gdCtl[i].m_RprocIndex -1] += gd[i]->m_p->m_Size +
           gd[i]->m_p->m_bufferSz + gd[i]->m_p->m_msgBufSz +
           sizeof(MHgdShm);
        gd[i]->detach();
      }
    }

    U_LongLong needSize = regMsg->m_Size + regMsg->m_bufferSz + \
       regMsg->m_msgBufSz + sizeof(MHgdShm);
    for (i = 0; i < MHmaxGDProcs && rt->m_gdCtl[gd_idx].m_RprocIndex == -1; i++) {
      if (gdProcsSz[i] + needSize < MHmaxTotGDOSz) {
        rt->m_gdCtl[gd_idx].m_RprocIndex = i + 1;
        break;
      }
    }
  }

  if (rt->m_gdCtl[gd_idx].m_shmid != -1) {
    if ((retval = gd[gd_idx]->attach(rt->m_gdCtl[gd_idx].m_shmid,
                                     rt)) != GLsuccess) {
      printf("Failed to attach %d, retval %d\n", gd_idx, retval);
    }
  }

  if ((retval = gd[gd_idx]->create(regMsg, rt, (IN_SHM_KEY)gd_idx)) != GLsuccess) {
    printf("Failed to create GDO %s, size %d, retval %d",
           regMsg->m_Name.display(), regMsg->m_Size, retval);
    gd[gd_idx]->detach();
    delete gd[gd_idx];
    gd[gd_idx] = NULL;
    rt->m_gdCtl[gd_idx].m_shmid = -1;
    rt->m_gdCtl[gd_idx].m_RprocIndex = -1;
    if (!MHmsgh.onLeadCC()) {
      MHgd::remove(regMsg->m_Name.display());
    }
    return;
  }
  gd[gd_idx]->detach();
}

Void MHinfoInt::gdAudReq(MHgdAudReq* audReq) {
  MHgdAud audmsg;
  MHgdShm* pGd;
  GLretVal retVal;

  audmsg.msgSz = sizeof(audmsg);
  audmsg.m_bSystemStart = audReq->m_bSystemStart;

  // Send the global data info to the requester
  for (int i = 0; i < MHmaxGd; i++) {
    if (gd[i] != NULL) {
      if ((retVal = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
        printf("failed to attach %d, retval %d", i, retVal);
        continue;
      }
      pGd = gd[i]->m_p;
      audmsg.gd[i].m_bInUse = TRUE;
      audmsg.gd[i].m_Name = pGd->m_Name;
      audmsg.gd[i].m_Size = pGd->m_Size;
      audmsg.gd[i].m_Permissions = pGd->m_Permissions;
      audmsg.gd[i].m_Dist = pGd->m_Dist;
      audmsg.gd[i].m_Uid = pGd->m_bufferSz;
      audmsg.gd[i].m_msgBufSz = pGd->m_msgBufSz;
      audmsg.gd[i].m_doDelaySend = pGd->m_doDelaySend;
      audmsg.gd[i].m_maxUpdSz = pGd->m_maxUpdSz;
      audmsg.gd[i].m_RprocIndex = rt->m_gdCtl[i].m_RprocIndex;
      gd[i]->detach();
    } else {
      audmsg.gd[i].m_bInUse = FALSE;
    }
  }
  if ((retVal = MHmsgh.send(audReq->srcQue,
                            (char*)&audmsg,
                            sizeof(audmsg), 0L))!= GLsuccess) {
    printf("failed to send gdAud response to %s retVal %d",
           audReq->srcQue.display(), retVal);
  }
}

Void MHinfoInt::gdAud(MHgdAud* aud) {
  MHregGd regGd;
  MHgdShm* gdshm;
  GLretVal retVal;

  // Do not handle audit messages when the system is not up
  if (!aud->m_bSystemStart && !IN_INSTEADY()) {
    return;
  }
  // Create new objects if there are any mission
  for (int i=0; i < MHmaxGd; i++) {
    if (aud->gd[i].m_bInUse) {
      if (!aud->m_bSystemStart) {
        // Audit the data
        if (aud->gd[i].m_Dist == MHGD_NONE) {
          if (gd[i] == NULL) {
            continue;
          }
        } else if ((aud->gd[i].m_Dist == MHGD_ALL) ||
                   rt->isCC(rt->hostlist[rt->LocalHostIndex]. \
                            hostname.display())) {
          if (gd[i] == NULL) {
            printf("Nonexistent GDO %d", i);
            INITREQ(SN_LV4, MHnoName, "NON EXISTENT GDO", IN_EXIT);
          }
        } else {
          if (gd[i] != NULL) {
            if ((retVal = gd[i]->attach(rt->m_gdCtl[i].m_shmid,
                                        rt)) != GLsuccess) {
              printf("failed to attach %d, retval %d", i, retVal);
              continue;
            } else {
              printf("GDO %s should not be on this machine",
                     aud->gd[i].m_Name.display());
              gd[i]->detach();
            }
            INITREQ(SN_LV4, MHexist, "GDO SHOULD NOT BE ON THIS MACHINE",
                    IN_EXIT);
          }
          continue;
        }
        if ((retVal = gd[i]->attach(rt->m_gdCtl[i].m_shmid,
                                    rt)) != GLsuccess) {
          printf("Failed to attach %d, retval %d", i , retVal);
          INITREQ(SN_LV4, MHnoName, "COULD NOT ATTACH GDO", IN_EXIT);
          continue;
        }
        gdshm = gd[i]->m_p;
        if (strcmp(gdshm->m_Name, aud->gd[i].m_Name.display()) != 0 ||
            gdshm->m_Permissions != aud->gd[i].m_Permissions ||
            gdshm->m_Uid != aud->gd[i].m_Uid ||
            gdshm->m_bufferSz != aud->gd[i].m_bufferSz ||
            gdshm->m_msgBufSz != aud->gd[i].m_msgBufSz ||
            gdshm->m_doDelaySend != aud->gd[i].m_doDelaySend ||
            gdshm->m_maxUpdSz != aud->gd[i].m_maxUpdSz ||
            gdshm->m_Size != aud->gd[i].m_Size ||
            gdshm->m_Dist != aud->gd[i].m_Dist ||
            rt->m_gdCtl[i].m_RprocIndex != aud->gd[i].m_RprocIndex) {
          printf("Invalid global data object %d", i);
        }
        gd[i]->detach();
      } else {
        // create the object and star synching
        if (gd[i] != NULL) {
          printf("GDO %d exists during system start", i);
          INITREQ(SN_LV4, MHexist, "GDO EXISTS DURING SYSTEM START", IN_EXIT);
        }
        if ((aud->gd[i].m_Dist == MHGD_NONE) ||
            (aud->gd[i].m_Dist != MHGD_ALL &&
             rt->isCC(rt->hostlist[rt->LocalHostIndex].hostname.display()))) {
          continue;
        }
        gd[i] == new MHgd;
        // Make the srcQue different then this machine to avoid ack messages
        regGd.srcQue = MHMAKEMHQID(rt->LocalHostIndex + 1, MHmsghQ);
        regGd.m_Name = aud->gd[i].m_Name;
        regGd.m_Uid = aud->gd[i].m_Uid;
        regGd.m_Permissions = aud->gd[i].m_Permissions;
        regGd.m_Dist = aud->gd[i].m_Dist;
        regGd.m_Size = aud->gd[i].m_Size;
        regGd.m_bufferSz = aud->gd[i].m_bufferSz;
        regGd.m_msgBufSz = aud->gd[i].m_msgBufSz;
        regGd.m_doDelaySend = aud->gd[i].m_doDelaySend;
        regGd.m_maxUpdSz = aud->gd[i].m_maxUpdSz;
        regGd.m_bReplicate = FALSE;
        regGd.m_shmkey = -1;
        rt->m_gdCtl[i].m_RprocIndex = aud->gd[i].m_RprocIndex;
        gd[i]->create(&regGd, rt, (IN_SHM_KEY)i, TRUE);
        gd[i]->detach();
        step++;
        IN_STEP(step, "");
      }
    } else {
      // Make sure we don't have one in use
      if (gd[i] != NULL) {
        printf("Global object %d exists but deleted on lead", i);
        INITREQ(SN_LV4, MHbadName, "GDO PERSENT BUT DELETED ON LEAD", IN_EXIT);
      }
    }
  }
}

Bool MHinfoInt::gdAllSynched() {
  char message[80];
  MHgdShm * p;
  static LongLong lastsync = 0;
  //FTgdoSyncMsg syncMsg;
  static float tot_size = 0;
  float cur_size = 0;
  int i;
  static Long count = 0;
  GLretVal retval;

  count++;

  if (tot_size == 0) {
    // comput total size of all objects
    for (i = 0; i < MHmaxGd; i++) {
      if (gd[i] != NULL) {
        if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid,
                                    rt)) != GLsuccess) {
          printf("Failed to attach %d, retval %d", i , retval);
          continue;
        }
        tot_size += gd[i]->m_p->m_Size;
        gd[i]->detach();
      }
    }
  }

  for (i = 0; i < MHmaxGd; i++) {
    if (gd[i] == NULL) {
      continue;
    }
    if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
      printf("Failed to attach %d, retval %d", i , retval);
      continue;
    }
    p = gd[i]->m_p;
    cur_size += p->m_SyncAddress;
    if (p->m_SyncAddress < p->m_Size) {
      sprintf(message, "GLOBAL DATA %s %lld OF %lld",
              p->m_Name, p->m_SyncAddress, p->m_Size);
      if (lastsync != p->m_SyncAddress) {
        step++;
        lastsync = p->m_SyncAddress;
        IN_STEP(step, message);
        if (count & 0x3) {
          IN_PROGRESS(message);
        }
      }
      //syncMsg.progress(p->m_Name, (cur_size * 100)/tot_size);
      //syncMsg.send(MHMAKEMHQID(rt->getLocalHostIndex(), MHmsgQ));
      gd[i]->detach();
      return(FALSE);
    }
    gd[i]->detach();
  }
  //syncMsg.progress(NULL, 100);
  //syncMsg.send(MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ));
  return(TRUE);
}

Void MHinfoInt::tmrExp(Long tag) {
  switch(tag & MHTAGMASK) {
  case MHGQTMR:
    rt->setGlobalQ(tag & MHQMASK);
    break;
  case MHtimerTag:
    break;
  case MHshutdownTag:
    if (hostId == MHmaxAllHosts) {
      isolateNode();
      exit(0);
    } else {
      MHconn connmsg;
      conn(&connmsg, hostId);
    }
    break;
  default:
    printf("Invalid timer tag 0x%x", tag);
    break;
  }
}

// This function keeps this node from sending any messages out
Void MHinfoInt::isolateNode() {
  for (int i = 0; i < MHmaxHostReg; i++) {
    rt->hostlist[i].isactive = FALSE;
    if (rt->SecondaryHostIndex != MHnone &&
        strncmp(rt->hostlist[i].hostname.display(), "as", 2) == 0) {
      rt->hostlist[i].isused = FALSE;
    }
  }
}

void MHinfoInt::RmGd(MHrmGd* rmMsg) {
  int gd_idx = MHmaxGd;
  int i;
  MHrmGdAck ackMsg;
  MHqid fromQ = rmMsg->srcQue;
  struct timespec tsleep;
  GLretVal retval;

  // Try to find the object, otherwise fail
  for (i = 0; i < MHmaxGd; i++) {
    if (gd[i] != NULL) {
      if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
        printf("Failed to attach %d, retval %d", i, retval);
        continue;
      }
      if (strcmp(gd[i]->getName(), rmMsg->m_Name.display()) == 0) {
        gd_idx = i;
        break;
      }
      gd[i]->detach();
    }
  }
  if (i == MHmaxGd) {
    if (MHmsgh.onLeadCC() && fromQ != MHnullQ) {
      ackMsg.m_RetVal = MHnoName;
      ackMsg.send(rmMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
    }
    return;
  }
  if (MHmsgh.onLeadCC() && rt->m_envType != MH_peerCluster) {
    // Send the remove message to all the other MSGH and MHRPROCs
    rmMsg->srcQue = MHnullQ;
    MHmsgh.sendToAllHosts("MSGH", (char*)rmMsg, sizeof(MHrmGd), 0L);
  }
  INshmem.deallocSeg(gd[gd_idx]->m_shmid);
  gd[gd_idx]->detach();
  delete gd[gd_idx];
  gd[gd_idx] = NULL;
  gdName[gd_idx][0] = 0;
  rt->m_gdCtl[gd_idx].m_shmid = -1;
  int tmp_RprocIndex = rt->m_gdCtl[gd_idx].m_RprocIndex;
  rt->m_gdCtl[gd_idx].m_RprocIndex = -1;
  // Send message to local gdproc
  MHgdAudReq audmsg;
  MHmsgh.send(MHMAKEMHQID(rt->LocalHostIndex,
                         MHrprocQ + tmp_RprocIndex),
             (char*)&audmsg, sizeof(audmsg), 0L);
  tsleep.tv_sec = 1;
  tsleep.tv_nsec = 0;
  nanosleep(&tsleep, NULL);

  ackMsg.m_RetVal = GLsuccess;
  ackMsg.send(fromQ, MHnullQ, (short)sizeof(ackMsg), 0L);
}

// update the status of gd entries to match m_gdCtl
Void MHinfoInt::updateGd() {
  int shmid;
  int i;
  GLretVal retval;

  // Find and create all global data objects
  for (i = 0; i < MHmaxGd; i++) {
    if (rt->m_gdCtl[i].m_shmid >= 0) {
      if (gd[i] == NULL) {
        gd[i] = new MHgd;
        if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != GLsuccess) {
          printf("Failed to attach %d, retval %d", i, retval);
          continue;
        }
        strcpy(gdName[i], gd[i]->getName());
        gd[i]->detach();
      }
    } else if (gd[i] != NULL) {
      MHgd* tmpgd = gd[i];
      gd[i] = NULL;
      gdName[i][0] = 0;
      delete tmpgd;
    }
  }
}

int MHinfoInt::findGd(const char *name) {
  int i;
  for (i = 0; i < MHmaxGd; i++) {
    return(i);
  }
  return(MHmaxGd);
}

