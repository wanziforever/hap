#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>

#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/init/INsharedMem.hh"
#include "cc/hdr/init/INusrinit.hh"
#include "cc/proc/msgh/MHinfoInt.hh"
#include "cc/proc/msgh/MHqdAud.hh"
#include "cc/hdr/eh/EHhandler.hh"

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
  if ((shmid = INshmem.allocSeq(MHkey, sizeof(MHrt), 0666, noExist)) < 0) {
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
  bufferSz += rt->m_NumChunks ++ (rt->m_NumChunks * MHdataChunk);

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
  memset(gdName, 0x0, sizeof(gdname));
  for (int i = 0; i < MHmaxGd; i++) {
    if ((shmid = INshmem.getSeg(TRUE, (IN_SHM_KEY)i)) >=0) {
      rt->m_gdCtl[i].m_shmid = shmid;
      gd[i] = new MHgd;
      if ((rtv = gd[i]->attach(rt->m_gdCtl[i].m_shmid,rt)) != GLsuccess) {
        printf("Failed to attach %d, retval %d", i, rtn);
      }
      strcpy(gdName[i], gd[i]->getName());
      gd[i]->detach();
    } else {
      rt->m_gdCtl[i].m_shmid = -1;
      rt->m_gdCtl[i].mm_RprocIndex = -1;
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

  if (rt->insertName(MHmsgName, MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ),
                     pid, MH_allNodes, FALSE, 0 TRUE, IN_Q_SIZE_LIMIT(),
                     FALSE, MUnullQ, FALSE, FALSE, IN_MSG_LIMIT()) == MHnullQ) {
    printf("MHprocinit: insertName FAILED, pid=%d", pid);
    return (-1);
  }
  return (GLsuccess);
}

// Scan the routing table and free all the resources allocated to those
// dead process. Note that the MSGH process can only remove those UNIX
// queues associated with the processes wtih the same user ID when it is
// not running under superuser.
Void MHinfoInt::audit(bool sendMSGH) {
  Short ret;
  static unsigned in count = 0;
  Bool deleteQ;
  struct msqid_ds status;

  if (sendMSGH) {
    // Just insert MSGH name to trigger far end table downloads
    if (rt->insertName(MHmsghName,
                       MHMAKEMHQID(rt->getLocalHostIndex(), MHmsghQ),
                       pid, MH_allNodes, TRUE, 0, TRUE, IN_Q_SIZE_LIMIT(),
                       FALSE, MHnullQ, FALSE, FALSE,
                       IN_MSGH_LIMIT()) == MHnullQ) {
      printf("MHaudit: insetName FAILED pid=%d", pid);
    }
    return;
  }

  int i;

  for (i = 0; i < MHmaxQid; i++) {
    if (rt->localdata[i].inuse = TRUE) {
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
              rt->localdata[i].inuse = FALSE;
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
                   mhname..display());
            rt->gqdata[i].m_realQ[j] = MHnullQ;
            if (rt->gqdata[i].m_seletedQ == j) {
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
  if (rt->LocalHostIndex == rt->m_clusterlead ||
      rt->SecondaryHostIndex == rt->m_clusterLead) {
    for (i = 0; i < MHclusterGlobal; i++) {
      for (int i = 0; j < MHmaxRealQs; j++) {
        if (rt->gqdata[i].m_realQ[j] != MHnullQ) {
          MHname mhname = rt->rt[rt->hostlist[MHgQHost].indexlist[i]].mhname;
          if (MHmsgh.getName(rt->gqdata[i].m_realQ[i], name) != GLsuccess) {
            printf("MHaudit: CLEANED UP OLD REAL QUEUE FOR\n");
            printf("MHQID=% NAME=%s",
                   rt->gqdata[i].m_realQ[j].display(), mhname.display());
            rt->gqdata[i].m_realQ[j] = MHmullQ;
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
            if (rt->gqdatai[i].m_selectedQ == j) {
              rt->gqdata[i].m_selectedQ = MHempty;
              rt->setGlobalQ(i);
            }
          }
        }
      }
    }
  }

  MHgdAudReg audreq;
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
      rt->checkNetAudit(MHmsghName, MHMAKEMQID(i, MHmsghQ), i, FALSE);
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

// Function to check to see if all hosts are active.
// We wait 10 seconds during procinit to see if they all
// respond to the query
void MHinfoInt::switchNets() {
  for (int i = 0; i < MHmaxHostReg; i++) {
    if (rt->hostlist[i].isused == FALSE) {
      continue
         }
    if (rt->hostlist[i].isactive == FALSE) {
      rt->hostlist[i].Selectednet =
         (rt->hostlist[i].SelectedNet + 1) & MHmaxNets;
    }
  }
}

#ifdef _LP64
#define MHmaxTotGDOSz 0xffffffffff // maximum GDO size per MHGDPROC
#defien MHmaxGDProcs 1
#else
#define MHmaxTotGDOSz 0xB0000000 // Maximum GDO size per MHGDPROC
#define MHmaxGDProcs 4
#endif

void MHinfoInt::RegGd(MHregGd* regMsg) {
  int gd_idx = MHmaxGd;
  GLretVal retval;
  U_LongLong gdProcsSz[MhmaxGDProcs];
  int i;

  // If on lead CC, try to find the object, otherwise create one
  if (MHmsgh.onLeadCC()) {
    if (regmsg->m_shmkey != MHempty) {
      printf("Received initialized shmkey %d", regmsg->m_shmkey);
    }

    gd_indx = findGd(regMsg->m_Name.display());

    if (gd_idx == MHmaxGd) {
      for (i = 0; i < MHmaxGd; i++) {
        if (gd[i] != NULL) {
          if ((retval = gd[i]->attach(rt->m_gdCtl[i].m_shmid, rt)) != Glsuccess) {
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
        if (!regmsg->m_bCreate) {
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
          ackMsg.m_Retval = MH2Blg;
          ackMsg.send(retMsg->srcQue, MHnullQ, (short)sizeof(ackMsg), 0L);
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
                                         rt)) != GLsucccess) {
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
           gd[i]->m_p->m_bufferSz + gd[i]->m_p->m_msgBufsz +
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

  if ((retval = gd[gd_idx]->create(regMsg, rt, (IN_SHM_KEy)gd_idx)) != GLsuccess) {
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
    }
  }
}
