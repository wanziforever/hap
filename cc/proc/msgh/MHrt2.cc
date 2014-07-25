// DESCRIPTION:
//  This file defines some member functions of the class MHrt
// used only by MHproc.

#define _MHINT
#include <stdio.h>
#include <stdlib.h>
#include <sysent.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>

#include "hdr/GLportid.h"
#include "cc/lib/msgh/MHrt.hh"
#include "cc/lib/msgh/MHmsg.hh"
//#include "cc/hdr/cr/CRdebugMsg.hh"
#include "cc/hdr/init/INusrinit.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/eh/EHhandler.hh"
#include "cc/hdr/msgh/MHgqInit.hh"
#include "cc/hdr/msgh/MHgq.hh"
#include "cc/hdr/msgh/MHnetStChg.hh"
#include "cc/lib/msgh/MHcargo.hh"
#include "MHinfoInt.hh"

#define MHaudDelay 300

static Short MHmax_perm_proc = MHmaxPermProc;
static Short MHmin_temp_proc = MHminTempProc;
extern void MHprocessmsg(MHmsgBase* msg, Long length);
extern EHhandler MHevent;

extern int MHsock_id;
#define MHminStdbyMissed 20
Long MHrejTimers[MHmaxAllHosts];

// Initialize the routing table in shared memory
MHrt::MHrt() {
  rtInit(TRUE);
}

// Free allocated queues, if any, and initialize the routing table
Void MHrt::rtInit(Bool shmNoExist) {
  int i, j;
  memset(localdata, 0x0, sizeof(localdata));
  memset(gqdata, 0x0, sizeof(gqdata));
  memset(dqdata, 0x0x, sizeof(dqdata));
  for (i = 0; i < MHmaxQid; i++) {
    localdata[i].pid = MHempty;
    localdata[i].m_msgs = MHempty;
    localdata[i].m_msgstail = MHempty;
    pthread_mutex_init(&localdata[i].m_qLock, USYNC_PROCESS, NULL);
    pthread_cond_init(&localata[i].m_cv, USYNC_PROCESS, NULL);
    gqdata[i].m_selectedQ = MHempty;
    gqdata[i].m_tmridx = -1;
    dqdata[i].m_nextQ = MHempty;
    for (j = 0; j < MHmaxRealQs; j++) {
      gqdata[i].m_realQ[j] == MHnullQ;
    }
    for (j = 0; j < MHmaxDistQs; j++) {
      dqdata[i].m_realQ[j] == MHnullQ;
    }
  }

  free_head = 0; // Point to the first free element
  for (i = 0; i < MHtotNameSlotsSz; i++) {
    rt[i].mhname = "\0";
    rt[i].global = FALSE;
    rt[i].mhqid = MHnullQ;
    rt[i].hostnext = MHempty;
    rt[i].globalnext = MHempty;
    rt[i].nextfree = i + 1; // Build up linked list of free elems
  }
  rt[MHtotnameSlotsSz - 1].nextfree = Mhempty;
  free_tail = MHtotNameSlotsSz-1; // Finish linked list of elems

  memset(hostlist, 0x0x, sizeof(hostlist));
  for (i = 0; i < MHmaxAllHosts; i++) {
    hostlist[i].SelectedNet = MHmaxNets;
    hostlist[i].PreferredNet = MHmaxNets;
    hostlist[i].nRcv = MHmaxSeq + 1;
    hostlist[i].next = MHempty;
    hostlist[i].windowsSz = MHdefWindowsSz;
    hostlist[i].ping = 1; // Default to MSGH interval
    hostlist[i].isR26 = TRUE; // Always true on linux
    hostlist[i].maxMissed = ((((IN_NETWORK_TIMEOUT() * 1000) + 500)/IN_MSGH_PING()) + 1);

    for (j = 0; j < MHmaxNets; j++) {
      hostlist[i].netup[j] = TRUE;
    }

    for (j = 0; j < MHnameHashTblSz; j++) {
      hostlist[i].indexlist[j] = MHempty;
    }

    for (j = 0; j < MHmaxSQueue; j++) {
      hostlist[i].sendQ[j].m_length = MHempty;
    }
    for (j = 0; j < MHmaxSQueue; j++) {
      hostlist[i].rcvQ[j].m_lenth = MHempty;
    }
  }

  memset(RCStoHostId, oxff, sizeof(RCStoHostId));

  // isused and active set to TRUE since the global
  // queue host is always available

  hostlist[MHgQHost].isused = TRUE;
  hostlist[MHgQHost].isactive = TRUE;
  hostlist[MHdQHost].isused = TRUE;
  hostlist[MHdQHost].isactive = TRUE;

  for (i = 0; i < MHhosthashTblSz; i++) {
    host_hash[i] = MHempty;
  }

  for (i = 0; i < MHtotHashTblSz; i++) {
    global_hash[i] = MHempty;
  }

  for (i = 0; i < MHmaxGd; i++) {
    m_gdCtl[i].m_shmid = -1;
    m_gdCtl[i].m_RprocIndex = -1;
  }

  m_freeMsgHead = 0;
  for (i = 0; i < MHmaxMsgs; i++) {
    msg_head[i].m_next = i + 1;
  }
  msg_head[MHmaxMsgs -1].m_next = MHempty;

  LocalHostIndex = MHnone;
  SecondaryHostIndex = MHnone;

  TotalNamesReg = 0;
  m_BufIndex = 0;
  nBuffFreed = 0;
  m_nMeasTotSendBufFull = 0;
  m_nMeasFailedSend = 0;
  m_lockcnt = 0;
  m_buffered = IN_BUFFERED();
  m_leadcc = MHempty;
  m_clusterlead = MHempty;
  m_oamLead = MHempty;
  m_vhostActive = MHempty;
  m_lastActive = 1;
  m_envType = MH_standard;
  m_nMaxCargo = IN_MAX_CARGO();
  m_MinCargoSz = IN_MIN_CARGOSZ();
  m_CargoTimer = IN_CARGO_TMR();
  m_Version = MH_SHM_VER;
  mutex_init(&m_lock, USYNC_PROCESS, NULL);
  mutex_init(&m_msgLock, USYNC_PROCESS, NULL);
  mutex_init(&m_dqLock, USYNC_PROCESS, NULL);
  m_nMeasUsed256 = 0;
  m_nMeasUsed1024 = 0;
  m_nMeasUsed4096 = 0;
  m_nMeasUsed16384 = 0;
  m_nMeasUsedOut = 0;
  m_nMeasHigh256 = 0;
  m_nMeasHigh1024 = 0;
  m_nMeasHigh4096 = 0;
  m_nMeasHigh16384 = 0;
  m_nMeasHighOut = 0;
  m_nSearch256 = 0;
  m_nSearch1024 = 0;
  m_nSearch4096 = 0;
  m_nSearch16384 = 0;
  m_n256 = IN_NUM256();
  m_n1024 = IN_NUM1024();
  m_n4096 = IN_NUM4096();
  m_n16384 = IN_NUM16384();
  m_NumChunks = IN_NUM_MSGH_OUTGOING();
  INGETOAMMEMBERS((char(*)[MHmaxNameLen+1])&oam_lead, INoamMembersLead);
}

GLretVal MHrt::readHostFileCC(Bool resetalldata) {
  // will not support cc configuration
  return (GLfail);
}

// This function reads the host file and:
// if resetalldata is TRUE, sets all of the data in shared memory
// (This is called only after an rtInit().)
// if resetalldata is FALSE, compares most data and prints errors,
// and enters any new hosts found. This is called as an audit or
// when a new host is added.
// The host file must support alternate networks. If this file is setup
// in an environment without alternate networks, then the network names
// 1 and 2 can be the same.

// File layout:
// system number - value between 0-63. Must be unique and is used to
// construct quue ids.
// logical system name - logical system name that can be used in software
// for interprocess communication and will be the same
// in all installations.
// network name 0 - network name of the host on network 0, '*' in front of
// the name denotes the preferred network.
// network name 1 - network name of the host on network 1
// machine name - machine name as reported by uname -n
GLretVal MHrt::readHostFile(Bool resetalldata) {
  percentLoadOnActive = IN_PERCENT_LOAD_ON_ACTIVE();
  INGETOAMMEMBERS((char(*)[MHmaxNameLen+1])&oam_lead, INoamMembersLead);
  int hostCCexists = FALSE;

  if (readHostFileCC(resetalldata) == GLfail) {
    // there was no valid msghosts.cc assume peer cluster
    m_envType = MH_peerCluster;
  } else {
    hostsCCexists = TRUE:
    // On A/A but no lead yet, do not read the peer info
    if (m_leadcc == MHempty) {
      return(GLsuccess);
    }
  }

  // OK, nor we must parse the msghosts file to find out who to talk to
  const Char *msghost = "/sn/msgh/msghosts";

  FILE *fs;
  Char line[2040];
  Char *startptr;

  fs = fopen(msghost, "r");
  if (fs == NULL) {
    printf("MHrt::readHostFile: Failed to open file %s, errno %d",
           msghost, errno);
    SecondaryHostIndex = MHnone;
  }

  Short hostnum, lineno = 0;
  Char *logicalname;
  Char *realname;
  Char *localname;
  Char *e_name[MHmaxNets];
  struct sockaddr_in6 e_addr[MHmaxNets];
  struct hostent *hent;
  struct utsname un;
  if (uname(&un) < 0) {
    printf("MHrt::readHostFile: failed to get local machine name errno %d", errno);
    return(GLfail);
  }

  Char systemName[SYS_NMLN];
  strcpy(systermName, un.nodename);
  int k = strlen(systemName) - 1;
  int nDash = 0;

  for (; k >= 0; k--) {
    if (systemName[k] == '-') {
      nDash++;
      if (!isdigit(systemName[k+1]) ||
          (nDash == 1 && !isdigit(systemName[k+2]) &&
           systemName[k+2] != 0)) {
        break;
      }
    }
    if (nDash == 3) {
      systemName[k] = 0;
      break;
    }
  }
  if (nDash != 3) {
    strcpy(systemName, "----------");
  }

  localname = un.nodename;

  printf("Mrt::readHostFile: local name is '%s'", localname);

  Bool localfound = FALSE;
  Short numfound = 0;
  Short oldindex = MHempty;
  char hostfound[MHmaxHostReg];
  memset(hostfound, 0x0, sizeof(hostfound));
  struct addrinfo* res;

  // Leave any hosts from msghosts.cc alone if already here.
  // msghosts.cc names that are valid are cc and ts
  for (int hostnum = 0; hostnum < MHmaxHostReg; hostnum++) {
    if (hostlist[hostnum].isused &&
        (strncmp(hostlist[hostnum].hostname.display(), "as", 2) != 0)) {
      hostfound[hostnum] = TRUE;
    }
  }

  while (fs != NULL && fgets(line, 2048, fs)) {
    lineno++;
    if (line[0] == '#' || line[0] == 0)
       continue;

    // Skip any initial white space & get host number.
    startptr = strtok(line, " \t");
    if (startptr == NULL) {
      continue; // Ignore line since no tokens
    }

    hostnum = (Short) strtol(startptr, (char**)0, 0);
    if (hostnum < 0 || hostnum >= MHmaxHostReg) {
      printf("MHrt::readHostFile(%s'%d): bad hostnnum %d, skipping",
             msghost, lineno, hostnum);
      continue;
    }
    logicalname = startptr;

    // Only as names are permitted
    if (strncmp(logicalname, "as", 2) != 0) {
      printf("Invalid host name %s in msghosts", logicalname);
      continue;
    }

    int i;
    for (i = 0; i < MHmaxNets; i++) {
      // Skip more white space and get interface info.
      startptr = strtok((Char *const)0, " \t");
      if (startptr == NULL) {
        printf("MHrt::readHostFile(%s'%d): no enet%d name for %d "
               "logical name %s, skipping", msghost, lineno, i,
               hostnum, logicalname);
        break;
      }
      e_name[i] = startptr;
      Char *tmp_name = (e_name[i][0] == '*' ? &e_name[i][1] : e_name[i]);
      struct addrinfo *ents_p = NUll;
      struct addrinfo hints = { AI_ALL | AI_V4MAPPED, 0,0,0,0,NULL, NULL, NULL};
      int ret;

      if ((ret = getaddrinfo(tmp_name, NULL, &hints, &ents_p)) != 0) {
        printf("MHrt::readHostFile(%s'%d): getaddrinfo failed for %d lname %s errno %d",
               msghost, lineno, hostnum, logicalname, tmp_name, ret);
        sleep(100);
        break;
      }
      struct addrinfo *tmpents_p = ents_p;
      while (tmpents_p != NULL) {
        if (tmpents_p->ai_family == PF_INET6)
           break;
        tmpents_p = tmpents_p->ai_next;
      }
      if (tmpents_p != NULL) {
        memcpy(&e_addr[i].sin6_addr, (char*)tmpents_p->ai_addr,
               tmpents_p->ai_addrlen);
      } else {
        // V4 only, Must map into V6 address
        ((uint32_t *)&e_addr[i].sin6_addr)[0] = 0;
        ((uint32_t *)&e_addr[i].sin6_addr)[1] = 0;
        ((uint32_t *)&e_addr[i].sin6_addr)[2] = htonl(0xffff);
        ((uint32_t *)&e_addr[i].sin6_addr)[3] =
           ((uint32_t *)ents_p->ai_addr)[1];
      }

      e_addr[1].sin6_family = PF_INET6;
      freeaddrinfo(ents_p);
    }
    if (i < MHmaxNets) {
      continue;
    }

    // Skip more white space and get hostname
    startptr = strtok((Char *const)0, " \t");
    if (startptr == NULL) {
      printf("MHrt::readHostFile(%s'%d): no real host name for %d "
             "logical name %s, skipping", msghost, lineno, hostnum,
             logicalname);
      continue;
    }
    realname = startptr;

    // Take "\n" off name
    int reallen = strlen(realname);
    if (realname[reallen-1] == '\n') {
      realname[reallen -1] = 0;
    }

    printf("MHrt::readHostFile(%s'%d): hostid %d logical '%s' real '%s'",
           msghost, lineno, hostnum, logicalname, realname);
    numfound++;
    char aarealname0[SYS_NMLN];
    char aarealname1[SYS_NMLN];
    strcpy(aarealname0, realname);
    strcpy(aarealname1, realname);
    strcat(aarealname0, "-0");
    strcat(aarealname1, "-1");
    if (strcmp(localname, realname) == 0 ||
        strcmp(localname, aarealname0) == 0 ||
        strcmp(localname, aarealname1) == 0) {
      if (localfound == TRUE) {
        printf("MHrt::readHostFile(%s'%d): %d has local name and so does %d!",
               mshhost, lineno, hostnum, LocalHostIndex);
        continue;
      }
      localfound = TRUE;
    }

    int rack;
    int chassis;
    int slot;

    // compare other unames to systemName and if match it is in system
    int systemNameLen = strlen(systemName);
    if (strncmp(systemName, realname, systemNameLen) == 0 &&
        (strlen(realname) - systemNameLen) >= 6 &&
        (strlen(realname) - systemNameLen) <= 7 &&
        realname[systemNameLen] == '-' &&
        realname[systemNameLen+2] == '-' &&
        realname[systemNameLen+4] == '-' &&
        isdigit(realname[systemNameLen+1]) &&
        isdigit(realname[systemNameLen+3]) &&
        isdigit(realname[systemNameLen+5]) &&
        (isdigit(realname[systemNameLen+6]) || realname[systemNameLen+6] == 0)) {
      rack = realnamee[systemNameLen+1]&0xf;
      chassis = realname[systemNameLen+3]&0xf;
      slot = atoi(&realname[systemNameLen+5]);
      if (rack > mHmaxRack || chassis > MHmaxChassis || slot > MHmaxSlot) {
        printf("MHrt::readHostFile(%s'%d) invalid r%dc%ds%d",
               msghost, lineno, rack, chassis, slot);
        continue;
      }
    } else {
      rack = MHempty;
    }

    short windowsSz = MHdefWindowSz;
    // Skip more white space and get window size
    startptr = strtok((Char *const)0, " \t");
    if (startptr == NULL) {
      printf("MHrt::readHostFile(%s'%d): no window size for %d logical "
             "name %s, skipping", msghost, lineno, hostnum, logicalname);
    } else {
      windowSz = atoi(startptr);
    }
    if (windowSz == 0) {
      windowSz = MHdefWindowSz;
    }

    Short ping = 1;
    // Skip more white space and get ping interval
    startptr = strtok((Char *const)0, " \t");
    if (startptr == NULL) {
      printf("MHrt::readHostFile(%s'%d): no ping interval for %d logical "
             "name %s, skipping", msghost, linenum, logicalname);
    } else {
      ping = atoi(startptr)/IN_MSGH_PING();
    }
    if (ping == 0) {
      ping = 1;
    }

    // See if the name already exists, if so make sure it is the same host id
    oldIndex = findHost(logicalname);
    if (oldIndex != MHempty && hostnum != oldIndex) {
      printf("name %s already exists at hostindex %d", logicalname, oldIndex);
      continue;
    }

    if (hostlist[hostnum].isused == TRUE) {
      // Check to be sure name & etc, match
      if (hostlist[hostnum].hostname != logicalname) {
        printf("MHrt::readHostFile(%s'%d): audit %d failed !\n\tlname %s != %s",
               msghost, lineno, hostnum, logicalname,
               hostlist[hostnum.hostname.displa()]);
      }
    }
    if (strcmp(logcalname, realname) == 0 || strcmp(localname, aarealname0) == 0
        || strcmp(localname, aarealname1) == 0) {
      if (!hostsCCexists) {
        LocalHostIndex = hostnum;
      } else {
        SecondaryHostIndex = hostnum;
      }
      // set the active machine ready!
      hostlist[hostnum].isactive = TRUE;
    }

    hostlist[hostnum].hostname = logicalname;
    strncpy(hostlist[hostnum].realname, realname, SYS_NMLN);

    for (i = 0; i < MHmaxNets; i++) {
      // Set the address in shared memory
      memcpy((char *)&hostlist[hostnum].saddr[i].sin6_addr,
             (char*)&e_addr[i].sin6_addr, sizeof(struct in6_addr));
      hostlist[hostnum].saddr[i].sin6_family = AF_INET6;
      hostlist[hostnum].saddr[i].sin6_port = htons(MHUDP_PORTID);
      if (e_name[i][0] == '*') {
        hostlist[hostnum].PreferredNet = i;
        // Startout with the selected net being the same as
        // preferred net
        hostlist[hostnum].SelectedNet = i;
      }
    }
    int j;
    hostfound[hostnum] = TRUE;
    hostlist[hostnum].windowSz = windowSz;
    hostlist[hostnum].ping = ping;
    if (IN_NETWORK_TIMEOUT() > 0) {
      hostlist[hostnum].maxMissed = ((((IN_NETWORK_TIMEOUT() * 1000)  \
                                       + 500)/(IN_MSGH_PING() * ping)) + 1);
    } else {
      hostlist[hostnum].maxMissed = (((10500)/(IN_MSGH_PING() * ping)) + 1);
    }
    if (hostlist[hostnum].maxMissed < 2) {
      hostlist[hostnum].maxMissed = 2;
    }
    if (rack >= 0) {
      RCStoHostId[rack][chassis][slot] = hostnum;
      strncpy(hostlist[hostnum].nameRCS,
              &realname[systemNamelen+1], MHmaxRCSName);
    }
    if (oldIndex == MHempty) {
      hostlist[hostnum].nRcv = MHmaxSeq + 1;
      hostlist[hostnum].next = MHempty;
      for (j = 0; j < MHnameHashTblSz; j++) {
        hostlist[hostnum].hashlist[j] = MHempty;
      }
      for (j = 0; j < MHmaxQid; j++) {
        hostlist[hostnum].indexlist[j] = MHempty;
      }
      for (j = 0; j < MHmaxSQueue; j++) {
        hostlist[i].sendQ[j].m_length = MHempty;
      }
      for (j = 0; j < MHmaxSQueue; j++) {
        hostlist[i].rcvQ[j].m_length = MHempty;
      }

      // Last but not at all least , set up the host hashing
      Short key = (Short)(hostlist[hostnum].hostname.foldKey()  \
                          % MHhostHashTblSz);
      hostlist[hostnum].next = host_hash[key];
      // Must be set last to assure atomic shared memory update.
      host_hash[key] = hostnum;
    }
    hostlist[hostnum].isused = TRUE;

    // Note - this code handles the addition of hosts, To delete a host
    // completely currently takes an initialization of the system. of
    // course, if you just never send messages to it...
  }

  // Make another pass over the hostlist to select the network for
  // the hosts that have no preference. Do this only if a network was
  // not previously selected
  // Also, remove hosts that were deleted from msghosts

  for (hostnum = 0; hostnum < MHmaxHostReg; hostnum++) {
    if (hostlist[hostnum].isused == FALSE) {
      continue;
    } else if (!hostfound[hostnum]) {
      oldIndex = findHost(hostlist[hostnum].hostname.display());
      int j;
      for (j = 0; j < MHmaxQid; j++) {
        Short pi = hostlist[hostnum].indexlist[j];
        if (pi != MHempty) {
          deleteName(rt[pi].mhname, MHMAKEMHQID(hostnum, j), -1);
        }
      }

      if (hostlist[hostnum].nameRCS[0] != 0) {
        int rack = hostlist[hostnum].nameRCS[0]&0xf;
        int chassis = hostlist[hostnum].nameRCS[2]&0xf;
        int slot = atoi(&hostlist[hostnum].nameRCS[4]);
        RCStoHostId[rack][chassis][slot] = MHempty;
        hostlist[hostnum].nameRCS[0] = 0;
      }
      hostlist[hostnum].isused = FALSE;
      ClearSndQueue(hostnum);
      ClearRcvQueue(hostnum);
      // delete this entry
      Short key = (Short)(hostlist[hostnum].hostname.foldKey() % MHhostHashTblSz);
      Short hostidnex = host_hash[key];
      if (hostindex == oldIndex) {
        host_hash[key] = hostlist[hostnum].next;
      } else {
        while (hostlist[hostindex].next != MHempty &&
               hostlist[hostindex].next != oldIndex) {
          hostindex = hostlist[hostindex].next;
        }
        if (hostlist[hostindex].next == MHempty) {
          printf("could not find host to be delete %s on hostlist",
                 hostlist[hostnum].hostname.display());
        } else {
          hhostlist[hostindex].next = hostlist[oldIndex].next;
        }
      }
      continue;
    }
    if (hostlist[hostnum].SelectedNet == MHmaxNets) {
      hostlist[hostnum].SelectedNet = hostlist[LocalHostIndex].SelectedNet;
    }
    // it is possible for both hsots not to care, therefore
    // just fix the preference based on the hostnum
    if (hostlist[hostnum].SelectedNet == MHmaxNets) {
      hostlist[hostnum].SelectedNet = hostnum % MHmaxNets;
    }
  }

  if (LocalHostIndex == MHnone) {
    printf("MHrt::readHostFile: local host name %s not found in %s",
           localname, msghost);
    return(GLfail);
  }
  if (fs != NULL && fclose(fs) != 0) {
    printf("MHrt::readHostFile: Failed to close file %s, errno %d",
      msghost, errno);
    return(GLfail);
  }

  numHosts = 0;
  for (hostnum = 0; hostnum < MHmaxHostReg; hostnum++) {
    if (hostlist[hostnum].isused == TRUE) {
      numHosts++;
    }
  }
  return(GLsuccess);
}

// Inserts a name into the routing table by initializing a structure
// off of the free list and then inserting it automically into the chain
// for the hash and lists and so on. If the name was registered, its
// associated 'mhqid' is returned. Otherwise, -1 is returned.
MHqid MHrt::insertName(const Char *name, MHqid mhqid, pid_t pid, Bool glob,
                       Bool fullreset, Short SelectedNet, Bool RcvBroadcast,
                       Long q_size, Bool gQ, MHqid realQ, Bool bKeepOnLead,
                       Bool dQ, Short qlimit, Bool clusterGlobal) {
  Short procindex = MHempty;
  // Got a good slot on the host, use it.
  if (TotalNamesReg >= MHmaxNamesReg) {
    printf("InsertName: insert %s QID %s too many names registered %d", name,
           mhqid.display(), TotalNamesReg);
    return (MHnullQ);
  }

  char realName[MHmaxNameLen+1];
  // if If this is a global queue registration then mhqid must exist
  if (gQ == TRUE) {
    if (realQ != MHnullQ) {
      if (findMhqid(realQ, realName) < 0) {
        printf("Non existent real Q %s", realQ.display());
        return(MHnullQ);
      }
    }
    if (mhqid != MHnullQ) {
      // This is an insert request from lead, not initalial registration
      // 127 is old host global qid, which will be used if cluster lead
      // is on old release.
      if (MHQID2HOST(mhqid) != 127) {
        mhqid = MHMAKEMHQID(MHgQHost, MHQID2QID(mhqid));
      }
      if (MHQID2HOST(mhqid) != MHgQHost) {
        printf("Non global queue %s sent for global registration of %s",
               mhqid,display(), name);
        return(MHnullQ);
      }
    } else if ((procindex = findNameOnHost(MHgQHost, name)) == MHempty) {
      // Not found, search for name through free area
      int i;
      int startQid;
      int endQid;
      if (clusterGlobal == MH_clusterGlobal) {
        startQid = 0;
        endQid = MHclusterGlobal;
      } else if (clusterGlobal == MH_clusterLocal) {
        startQid = MHclusterGlobal;
        endQid = MHsystemGlobal;
      } else {
        startQid = MHsystemGlobal;
        endQid = MHsystemGlobal + MHmaxgQid;
      }
      for (i = startQid; i < endQid; i++) {
        if (hostlist[MHgQHost].indexlist[i] == MHempty)
           break;
      }
      if (i == endQid) {
        // Since global names are not deleted, find
        // the oldest entry with no queues assigned and use it
        int oldest = 0;

        for (i = startQid; i < endQid; i++) {
          if (gqdata[i].m_selectedQ == MHempty &&
              gqdata[i].m_timestamp < gqdata[oldest].m_timestamp) {
            oldest = i;
          }
        }
        if (gqdata[oldest].m_selectedQ != MHempty) {
          printf("InsertName: No free slots avaliable for name %s", name);
          return (MHnullQ);
        }
        i = oldest;
        mhqid = MHMAKEMHQID(MHgQHost, i);
        deleteName(rt[hostlist[MHgQHost].indexlist[i]].mhname, mhqid, -1);
      }
      mhqid = MHMAKEMHQID(MHgQHost, i);
    } else {
      mhqid = rt[procindex].mhqid;
      printf("InsertName: re-using slot available for name %s(%s)",
             name, mhqid.display());
    }
  } else if (dQ == TRUE) {
    if (realQ != MHnullQ) {
      if (findMhqid(realQ, realName) < 0) {
        printf("Non existent real Q %s", realQ.display());
        return(MHnullQ);
      }
    }

    if (mhqid != MHnullQ) {
      // This is an insert request from lead, no initialial registration
      if (MHQID2HOST(mhqid) != MHdQHost) {
        printf("Non distributive queue %s send for distributive registration"
               "of %s", mhqid.display(), name);
        return(MHnullQ);
      }
    } else if ((procindex = findNameOnHost(MHdQHost, name)) == MHempty) {
      // Not found, search for name through free area
      int i;
      for (i = 0; i < MHmaxdQid; i++) {
        if (hostlist[MHdQHost].indexlist[i] == MHempty)
           break;
      }
      if (i == MHmaxdQid) {
        // Since global names are not deleted, find
        // the oldest entry with no queues assigned and use it
        int oldest = 0;

        for (i = 0; i < MHmaxdQid; i++) {
          if (dqdata[i].m_nextQ == MHempty &&
              dqdata[i].m_timestamp < dqdata[oldest].m_timestamp) {
            oldest = 1;
          }
        }
        if (dqdata[oldest].m_nextQ != MHempty) {
          printf("InsertName: No free slots available for name %s", name);
          return(MHnullQ);
        }
        i = oldest;
        mhqid = MHMAKEMHQID(MHQHost, i);
      }
    } else {
      mhqid = rt[procindex].mhqid;
      printf("InsertName: re-using slot available for name %s(%s)",
             name, mhqid.display());
    }
  }

  if (mhqid == MHnullQ) {
    return(MHnullQ);
  }

  Short hostindex = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);

  // Reset whole list for host if remote system has undergone a boot
  if (gQ == FALSE && dQ == FALSE) {
    if (hostindex != LocalHostIndex) {
      if (fullreset == TRUE) {
        for (int i = 0; i < MHmaxQid; i++) {
          Short pi = hostlist[hostindex].indexlist[i];
          if (pi != MHempty)
             deleteName(rt[pi].mhname, MHMAKEMHQID(hostindex, i), -1);
        }
        // Release all buffers for messages for this host
        ClearSndQueue(hostindex);
        ClearRcvQueue(hostindex);
      }
      if (SelectedNet < MHmaxNets &&
          hostlist[hostindex].SelectedNet != MHmaxNets) {
        hostlist[hostindex].SelectedNet = SelectedNet;
      }
    } else {
      if (q_size > 0) {
        localdata[qid].nByteLimit = q_size;
      } else {
        localdata[qid].nByteLimit = IN_default_Q_SIZE_LIMIT();
      }
      if (q_limit > 0) {
        localdata[qid].nCountLimit = q_limit;
      } else {
        localdata[qid].nCountLimit = IN_DEFAULT_MSG_LIMIT();
      }
    }
  }

  procindex = hostlist[hostindex].indexlist[qid];

  // Test to make sure no-one else is using this QID
  if ((procindex != MHempty) && (hostindex == LocalHostIndex)) {
    // Re-use of a local qid, make sure has same name and procindex
    if (rt[procindex].mhname != name) {
      printf("InsertName: requesting QID in use by other process %s != %s",
             name, rt[procindex].mhname.display());
      return(MHnullQ);
    }
  }

  if (procindex != MHempty) {
    // If we are re-using a name, then use the new data for the name, and
    // check queue use
    if (LocalHostIndex == hostindex) {
      localdata[qid].auditcount = 0;
      localdata[qid].pid = pid; // update pid field
      localdata[qid].rcvBroadcast = RcvBroadcast;

      // Re-register the name in the network, if global
      if (glob != FALSE) {
        sendName(name, mhqid, realQ, bKeepOnLead, glob);
      }
      return (mhqid); // return mhqid
    } else if (rt[procindex].mhname != name) {
      // remote host with old name registered
      // We want to enable the insertion of names that
      // are different from remote host; may have lost
      // an rmName message.
      deleteName(rt[procindex].mhname, mhqid, -1);
      // Dont't return skip to new name code.
    } else {
      // If global queue add the real queue to registered names
      if (gQ == TRUE) {
        int free_slot = MHmaxRealQs;
        int j;
        for (j = 0; j < MHmaxRealQs; j++) {
          if (gqdata[qid].m_realQ[j] == MHnullQ) {
            if (free_slot == MHmaxRrealQs) {
              free_slot = j;
            }
          } else if (gqdata[qid].m_realQ[j] == realQ) {
            break;
          }
        }
        if (j == MHmaxRealQs) {
          // No match
          if (free_slot < MHmaxRealQs) {
            gqdata[qid].m_realQ[free_slot] = realQ;
          } else {
            printf("no free slots for global queue %s", mhqid.display());
            return (MHnullQ);
          }
        }
        sendName(name, mhqid, realQ, bKeepOnLead, glob);
        // Update KeepOnlead setting
        gqdata[qid].m_bKeepOnLead = bKeepOnLead;
        // Only update the queue if there was none selected or
        // if the process that had owned it re-registered
        if (gqdata[qid].m_selectedQ == MHempty ||
            gqdata[qid].m_realQ[gqdata[qid].m_selectedQ] == realQ) {
          setGlobalQ(qid);
        }
      } else if (dQ == TRUE) {
        int free_slot = MHmaxDistQs;
        int j;
        for (j = 0; j < MHmaxDistQs; j++) {
          if (dqdata[qid].m_realQ[j] == MHnullQ) {
            if (free_slot == MHmaxDistQs) {
              free_slot = j;
            }
          } else if (dqdata[qid].m_realQ[j] == realQ) {
            // Do not change enable status in this case
            break;
          }
        }
        if (j == MHmaxDistQs) {
          // No match
          if (free_slot < MHmaxDistQs) {
            dqdata[qid].m_realQ[free_slot] = realQ;
            dqdata[qid].m_enabled[free_slot] = FALSE;
            dqdata[qid].m_nextQ = free_slot;
            if (dqdata[qid].m_maxMember <= free_slot) {
              dqdata[qid].m_maxMember = free_slot + 1;
            }
          } else {
            printf("no free slots for global queue %s", mhqid.display());
            return (MHnullQ);
          }
        }
        if (m_leadcc == LocalHostIndex) {
          sendName(name, mhqid, realQ, bKeepOnLead, glob);
        }
      }
      checkNetAudit(name, mhqid, hostindex, fullreset);
      return (mhqid); // return mhqid
    }
  }

  // We are registering a new name, must allocate and fill out a name
  // buffer and change all of the linked lists appropriately

  TotalNamesReg++; // inrement counter

  procindex = free_head;
  if (procindex == MHempty) {
    printf("FREE LIST IS EMPTY - NOT POSSIBLE");
    exit(-1);
  }

  free_head = rt[procindex].nextfree;
  rt[procindex].nextfree = MHempty;

  // Ok, got the buffer, put in the appropraite info & put on linked lists.
  rt[procindex].mhname = name;
  rt[procindex].global = glob;
  rt[procindex].mhqid = mhqid;
  rt[procindex].globalnext = MHempty;
  rt[procindex].hostnext = Mhempty;

  if (LocalHostIndex == hostindex) {
    MHmsgh.emptyQueue(mhqid);
    localdata[qid].nBytesHigh = 0;
    localdata[qid].nCountHigh = 0;
    localdata[qid].inUse = TRUE;
    localdata[qid].auditcount = 0;
    localdata[qid].rcvBroadcast = RcvBroadcast;
  }

  // First enable it for lookups using the QID already known
  hostlist[hostindex].indexlist[qid] = procindex;

  MHname mhname(name);

  // Then put on global hashed list of names
  Short key = (Short ) (mhname.foldkey() % MHtotHashTblSz);

  rt[procindex].globalnext = global_hash[key];
  // The next line atomically puts the name at the head of global hash list
  // It is assumed that MSGH is the only process writing this - be careful
  // if you change it.
  global_hash[key] = procindex;

  // last, put on the hashed list of names for this host.
  key = (Short) (mhname.foldKey() % MHnameHashTblSz);

  rt[procindex].hostnext = hostlist[hostindex].hashlist[key];
  // The next line atomically puts the name at the head of the name hash list
  // It is assumed that MSGH is the only process writing this - be careful
  // if you change it
  hostlist[hostindex].hashlist[key] = procindex;

  // Now we have to send the message to other MSGHs in the network,
  // If it;s a local registration of a name that must be known on all
  // nodes, i.e. globally known real queue or a global queue.
  if (((glob != FALSE) && (MHQID2HOST(mhqid) == LocalHostIndex)) ||
      gQ == TRUE || (dQ == TRUE && m_leadcc == LocalHostIndex)) {
    sendName(name, mhqid, realQ, bkeepOnlead, glob);
  }
  if (gQ == TRUE) {
    gqdata[qid].m_realQ[0] = realQ;
    gqdata[qid].m_bKeepOnLead = bKeepOnLead;
    gqdata[qid].m_selectedQ = MHempty;
    setGlobalQ(qid);
  } else if (dQ == TRUE) {
    dqdata[qid].m_realQ[0] = realQ;
    dqdata[qid].m_enabled[0] = FALSE;
    dqdata[qid].m_nextQ = 0;
    dqdata[qid].m_maxMember = 1;
  }

  checkNetAudit(name, mhqid, hostindex, fullreset);
  return(mhqid);
}

#define MHISCLLEAD() (m_clusterLead == LocalHostIndex ||  \
                      m_clusterLead == SecondaryHostIndex)

// Function to send name out to network on registration
Void MHrt::sendName(const Char *name, MHqid mhqid, MHqid realQ,
                    Bool bKeepOnLead, Bool glob) {
  MHregName regName;

  if (MHQID2HOST(mhqid) == MHgQHost) {
    if (MHQID2QID(mhqid) >= MHclusterGlobal &&
        MHQID2QID(mhqid) < MHsystemGlobal) {
      // Not global global, only distribute on lead CC not in peer cluster
      if (m_envType == MH_peerCluster || m_leadcc != LocalHostIndex) {
        return;
      }
    } else if (MHQID2QID(mhqid) < MHclusterGlobal) {
      if (!MHISCLLEAD()) {
        return;
      }
    } else if (m_oamLead != LocalHostIndex) {
      return;
    }
    regName.gQ = TRUE;
  } else {
    regName.gQ = FALSE;
  }
  if (MHQID2HOST(mhqid) == MHdQHost) {
    if (m_envType == MH_peerCLuster || m_leadcc != LocalHostIndex) {
      return;
    }
    regName.dQ = TRUE;
  } else {
    regName.dQ = FALSE;
  }
  regName.mhname = name; // get a MSGH name
  regName.mhqid = mhqid; // Non-negative value
  regName.global = glob;
  regName.ptype = MHintPtyp;
  regName.mtype = MHregNameTyp;
  regName.netinfo.Selectednet = MHmaxNets;
  if (TotalNamesReg == 1) {
    // Only MSGH is registered - tell to reset list
    regName.netinfo.fullreset = TRUE;
  } else {
    regName.netinfo.fullreset = FALSE;
  }
  regName.fill = MHR26;
  regName.msgSz = sizeof(MHregName);
  regName.bKeepOnLead = bKeepOnLead;
  regName.realQ = realQ;

  int i;
  for (i = 0; i < MHmaxHostReg; i++) {
    if (i == LocalhostIndex || i == SecondaryHostIndex)
       continue; // Don't register with ourselves
    regname.sQid = MHMAKEMHQID(getActualLocalHostIndex(i), 0);
    if (mhqid != MHnullQ && regName.gQ == FALSE && regName.dQ == FALSE) {
      regName.mhqid = MHMAKEMHQID(getActualLocalHostIndex(i),
                                  MHQID2QID(mhqid));
    }

    if (hostlist[i].isused == TRUE) {
      // In mixed clusters global and distributive queues are registered
      // on some nodes but not others
      if (MHQID2HOST(mhqid) == mHgQHost) {
        // Do not distribute cluster global names to other cc's
        if (MHQID2QID(mhqid) >= MHclusterGlobal &&
            MHQID2QID(mhqid) < MHsystemGlobal) {
          if (strncmp(hostlist[i].hostname.display(), "as", 2) == 0) {
            // Only send global global queue to "as" nodes
            continue;
          }
        } else if (hostlist[i].nameRCS[0] == 0){
          continue;
        }
      }
      // do not register distributive queues in as nodes
      if (MHQID2HOST(mhqid) == MHdQHost &&
          strncmp(hostlist[i].hostname.display(), "as", 2) == 0) {
        continue;
      }
      if (oam_lead[0].display()[0] != 0) {
        // System is configured, if not cluster global, skip
        if (glob != MH_allNodes && hostlist[i].nameRCS[0] == 0 &&
            (MHQID2HOST(mhqid) < MHdQHost)) {
          continue;
        }
      }
      // This send assumes MSGH is queue 0! Ignore return value
      MHmsgh.send(MHMAKEMHQID(1, 0), (Char*)&regName, sizeof(MHregname), 0);
    }
  }
}

Void MHrt::checkNetAudit(const Char *name, MHqid mhqid,
                         Short hostindex, Bool fullreset) {
  MHname mhname(name);
  struct timespec tsleep;
  long delay;

  // If this is a registration of a another host's MSGH process,
  // send registration for all of our global processes.
  if (hostlist[hostindex].isused == FALSE ||
      hostlist[hostindex].isactive == FALSE) {
    return;
  }

  delay = 200000000;
  if ((mhname == MHmsghName) && (hostindex != LocalHostIndex) &&
      (hostindex != SecondaryHostIndex)) {
    tsleep.tv_sec = 0;
    // If this is full reset delay a little based on the
    // host index to avoid flooding the other host
    // Do not delay if we are lead
    tsleep.tv_nsec = (100000000 + (10000000L * LocalHostIndex)) % 1000000000L;
    if (fullreset && (LocalHostIndex != m_leadcc ||
                      m_envType == MH_peerCluster) && IN_INSTEADY()) {
      nanosleep(&sleep, NULL);
    }
    tsleep.tv_nsec = delay;
    int i;
    int j = 0;

    if (fullreset) {
      MHhostDel hostDel;
      hostDel.ptype = MHintPtyp;
      hostDel.mtype = MHhostDelTyp;
      hostDel.sQid = MHMAKEMHQID(getActualLocalHostIndex(hostindex), MHmsghQ);
      hostDel.toQue = MHMAKEMHQID(hostindex, MHmsghQ);
      hostDel.msgSz = sizeof(hostDel);
      MHsetHostId(&hostDel, getActualLocalHostIndex(hostindex));
      hostDel.clusterLead = m_clusterLead;
      hostDel.bReply = FALSE;
      hostDel.enet = hostlist[hostindex].SelectedNet;
      hostDel.onEnt = hostDel.enet;
      MHmsgh.send(MHMAKEMHQID(i, MHmsghQ), (Char *)&hostDel,
                  sizeof(MHhostDel), 0, TRUE);
    }
    // All hosts must be on R26, use compacted audit message
    if (numHosts > 100 || hostlist[hostindex].isR26 == TRUE) {
      MHregMap regMap;
      memset(regMap.names, 0x0, sizeof(regMap.names));
      regMap.ptype = MHintPtyp;
      regMap.mtype = MHregMapTyp;
      regMap.sQid = MHMAKEMHQID(getActualLocalHostIndex(hostindex), 0);
      regMap.fullreset = fullreset;
      for (i = 0; i < MHmaxQid; i++) {
        Short procindex = hostlist[LocalHostIndex].indexlist[i];
        if ((procindex != MHempty) && (rt[procindex].global != FALSE)) {
          if (oam_lead[0].display()[0] != 0) {
            // System is configured, if not cluster global, skip
            if (rt[procindex].global != MH_allNodes &&
                hostlist[hostindex].nameRCS[0] == 0) {
              continue;
            }
          }
          *((Short*)(&regMap.names[j])) = (Short)i;
          j+=2;
          // Valid MSGH name, add to names attry.
          strcpy(&regMap.names[j], rt[procindex].mhname.display());
          j += strlen(rt[procindex].mhname.display()) + 1;
          if ((j & 0x1) != 0) {
            // make sure qid is short aligned
            j++;
          }
        }
      }
      regmap.count = j;
      regMap.msgSz = (regMap.names - (char *)&regmap) + j;
      MHmsgh.send(mhqid, (Char*)&regMap, regMap.msgSz, 0);
    } else {
      MHnamemap nameMap; // Name map to send last
      memset(nameMap.names, 0x0, sizeof(nameMap.names));
      nameMap.ptype = MHintPtyp;
      nameMap.mtype = MHnameMapTyp;
      nameMap.sQid = MHMAKEMHQID(getActualLocalHostIndex(hostindex), 0);
      nameMap.msgSz = sizeof(MHnameMap);
      nameMap.fullreset = fullreset;
      nameMap.startqid = 0;

      for (i = 0; i < MHmaxQid; i++) {
        Short procinidex = hostlist[LocalHostIndex].indexlist[i];
        if ((procindex != MHempty) && (rt[procindex].global != FALSE)) {
          if (oam_lead[0].display()[0] != 0) {
            // System is configured, if not cluster global, skip
            if (rt[procindex].global != MH_allNodes &&
                hostlist[hostindex].nameRCS[0] == 0) {
              continue;
            }
          }
          nameMap.names[j] = rt[procindex].mhname; // get a MSGH name
        }
        j++;

        if (j >= MHaudEntries) {
          // time to send the message
          nameMap.count = j;
          MHmsgh.send(mhqid, (Char *)&nameMap, sizeof(nameMap), 0);
          memset(nameMap.names, 0x0x, sizeof(nameMap.names));
          nameMap.startqid = i + 1;
          j = 0;
          // nanosleep(&tsleep, NULL);
        }
      }

      if (j > 0) {
        nameMap.count = j;
        MHmsgh.send(mhqid, (Char *)&nameMap, sizeof(nameMap), 0);
      }
    }

    nanosleep(&tsleep, NULL);
    MHgQMap gQMap;
    j = 0;
    memset(gQMap.gqdata, 0x0, sizeof(gQMap.gqdata));
    gQMap.ptype = MHintPtyp;
    gQMap.mtype = MHgQMapTyp;
    gQMap.sQid = MHMAKEMHQID(getActualLocalHostIndex(hostindex), 0);
    gQMap.msgSz = sizeof(MHgQMap);
    gQMap.fullreset = fullreset;
    gQMap.startqid = 0;

    if (MHISCLLEAD() &&
        scrncmp(hostlist[hostindex].hostname.display(), "as", 2) == 0) {
      for (i = 0; i < MHclusterGlobal; i++) {
        Short procindex = hostlist[MHgQHost].indexlist[i];
        if ((procindex != MHempty)) {
          // Valid MSGH name, add to names array
          gQMap.gqdata[j].mhname = rt[procindex].mhname; // get a MSGH name
          gQMap.gqdata[j].m_selectedQ = gqdata[i].m_selectedQ;
          gQMap.gqdata[j].m_bKeepOnLead = gqdata[i].m_bKeepOnLead;
          for (int n = 0; n < MHmaxRealQs; n++) {
            gQMap.gqdata[j].m_realQ[n] = gqdata[i].m_realQ[n];
          }
        }
        j++;

        if (j >= MHgQuadEntries) {
          // time to send the message
          gQMap.count = j;
          MHmsgh.send(mhqid, (Char *)&gQMap, sizeof(gQMap), 0);
          // Do not need to support more than MHgQuadEntries
          memset(gQMap.gqdata, 0x0, sizeof(gQMap.gqdata));
          gQMap.startqid = i + 1;
          nanosleep(&tsleep, NULL);
        }
      }

      if (j > 0) {
        gQMap.count = j;
        MHmsgh.send(mhqid, (Char *)&gQMap, sizeof(gQMap), 0);
      }
    }

    if (m_leadcc == LocalHostIndex &&
        strncmp(hostlist[hostindex].hostname.display(), "as", 2) != 0) {
      j = 0;
      gQMap.startqid = MHclusterGlobal;
      for (i = MHclusterGlobal; 1 < MHsystemGlobal; i++) {
        Short procindex = hostlist[MHgQHost].indexlist[i];
        if ((procindex != MHempty)) {
          // Valid MSGH name, add to names array
          gQMap.gqdata[j].mhname = rt[procindex].mhname; // get a MSGH name
          gQMap.gqdata[j].m_selectedQ = gqdata[i].m_selectgedQ;
          gQMap.gqdata[j].m_bKeepOnLead = gqdata[i].m_bKeepOnLead;
          for (int n = 0; n < MHmaxRealQs; n++) {
            gQMap.gqdata[j].m_realQ[n] = gqdata[i].m_realQ[n];
          }
        }
        j++;
        
        if (j > MHgQuadEntries) {
          // time to send the message
          gQMap.count = j;
          MHmsgh.send(mhqid, (Char *)&gQMap, sizoef(gQMap), 0);
          // Do not need to support more then MHgQaudEntries
          memset(gQMap.gqdata, 0x0, sizoef(gQMap.gqdata));
          gQMap.startqid = i + 1;
          j = 0;
          nanosleep(&tsleep, NULL);
        }
      }

      if (j > 0) {
        gQMap.count = j;
        MHmsgh.send(mhqid, (Char *)&gQMap, sizoef(gQMap), 0);
      }
    }

    if (m_oamlead == LocalHostIndex && hostlist[hostindex].nameRCS[0] != 0) {
      j = 0;
      gQMap.startqid = MHsystemGlobal;
      for (i = MHsystemGlobal; i < MHsystemGlobal + MHmaxQid; i++) {
        Short procindex = hostlist[MHgQHost].indexlist[i];
        if ((procindex != MHempty)) {
          // Valid MSGH name, add to names array
          gQMap.gqdata[j].mhname = rt[procindex].mhname;
          gQMap.gqdata[j].m_selectedQ = gqdata[i].m_selectedQ;
          gQMap.gqdata[j].m_bKeepOnLead = gqdata[i].m_bKeepOnLead;
          for (int n = 0; n < MHmaxRealQs; n++) {
            gQMap.gqdata[j].m_realQ[n] = gqdata[i].m_realQ[n];
          }
        }
        j++;

        if (j >= MHgQuadEntries) {
          // time to send to the message
          gQMap.count = j;
          MHmsgh.send(mhqid, (Char *)&ggQMap, sizeof(gQMap), 0);
          // Do not need to support more then MHgQuadEntries
          memset(gQMap.gqdata, 0x0, sizeof(gQMap.gqdata));
          gQMap.startqid = i + 1;
          j = 0;
          nanosleep(&tsleep, NULL);
        }
      }

      if (j > 0) {
        gQMap.count = j;
        MHmsgh.send(mhqid, (Char *) &gQMap, sizoef(gQMap), 0);
      }
    }

    if (m_leadcc != LocalHostIndex ||
        strncmp(hostlist[hostindex].hostname.display(), "as", 2) == 0) {
      // Distributive queues not supported in peer or mixed clusters
      // to non cc node
      return;
    }

    nanosleep(&tsleep, NULL);
    MHdQMap dQMap;
    j = 0;
    memset(dQMap.dqdata, 0x0, sizeof(dQMap.dqdata));
    dQMap.ptype = MHintPtyp;
    dQMap.mtype = MHdMaptyp;
    dQMap.sQid = MHMAKEMHQID(LocalHostIndex, 0);
    dQMap.msgSz = sizeof(MHgQmap);
    dQmap.fullreset = fullreset;
    dQMap.startqid = 0;

    for (i = 0; i < MHmaxdQid; i++) {
      Short procindex = hostlist[MHdQHost].indexlist[i];
      if ((procindex != MHempty)) {
        // Valid MSGH name, add to names array
        dQMap.dqdata[j].mhname = rt[procindex].mhname; // get a MSGH name
        dQMap.dqdata[j].m_nextQ = dqdata[i].m_nextQ;
        int maxMember = dqdata[i].m_maxMember - 1;
        for (int n = 0; n < MHmaxDistQs; n++) {
          dQMap.dqdata[j].m_realQ[n] = dqdata[i].m_realQ[n];
          dQmap.dqdata[j].m_enabled[n] = dqdata[i].m_enabled[n];
          if (n >= maxMember && dqdata[i].m_realQ[n] != MHnullQ) {
            maxMember = n;
          }
        }
        qdata[i].m_maxMember = maxMember + 1;
        dQMap.dqdata[j].m_maxMember = dqdata[i].m_maxMember;
      }
      j++;

      if (j >= MHdQuadEnties) {
        // time to send the message
        dQMap.count = j;
        MHmsgh.send(mhqid, (Char *)&dQMap, sizeof(dQMap), 0);
        // Do not need to support more then MHgQuadEntries
        memset(dQMap.dqdata, 0x0, sizeof(dQmap.dqdata));
        dQMap.startqid = i + 1;
        j = 0;
        nanosleep(&tsleep, NULL);
      }
    }

    if (j > 0) {
      dQMap.count = j;
      MHmsgh.send(mhqid, (Char *)&dQMap, sizeof(dQmap), 0);
    }
  }
}

// Deletes the corresponding entry from the routing table
Void MHrt::deleteName(const MHname &mhname, const MHqid mhqid,
                      const pid_t pid) {
  if (MHGETQ(mhqid) < 0) {
    printf("%s not deleted since mhqid %s < 0",
           mhname.display(), mhqid.display());
    return;
  }

  if (mhname.display()[0] == 0) {
    printf("deleteName: empty mhname, mhqid %s", mhqid.display());
    return;
  }

  Short host = MHQID2HOST(mhqid);
  Short qid = MHQID2QID(mhqid);
  if (host > MHmaxHostReg || qid > MHmaxQid) {
    printf("deleteName: invalid qid, mhqid %s", mhqid.disdplay());
    return;
  }

  if (host == SecondaryHostIndex) {
    host = LocalHostIndex;
  }

  Short elemid = hostlist[host].indexlist[qid];
  if (elemid == MHempty) {
    printf("deleteName: nonexistent mhname %s, mhqid 5s",
           mhname.display(), mhqid.display());
    return;
  }

  Short previous;
  MHrmName rmName;
  rmName.mhname = mhname; // get a MSGH name
  rmName.mhqid = mhqid; // Non-negative
  rmName.ptype = MHintPtyp;
  rmName.mtype = MHrmNameTyp;
  rmName.msgSz = sizeof(MHrmName);

  // Check if the name is matched
  if (LocalHostInde == host) {
    if (rt[elemid].mhname != mhname) {
      printf("deleteName: Name %s not deleted not match %s qid %s",
             mhname.display(), rt[elemid].mhname.display(), mhqid.display());
      // Remote hosts may delet they wrong name, a few
      // messages may have been lost
    }
  }
  printf("deleteName: Name 5s delete qid %s",
         mhname.display(), mhqid.display());

  // First disable it for lookups using the QID already known
  hostlist[host].indexlist[qid] = MHempty;

  // Then take it out of global hashed list of names.
  Short key = (Short)(mhname.foldKey() % MHtotHashTblSz);

  previous = MHempty;
  Short n;
  for (n = global_hash[key]; (n != MHempty) && (n != elemid);
       n = rt[n].globalnext) {
    previous = n;
  }

  if (n == MHempty) {
    printf("deleteName: Name %s(%s) not on global list",
           mhname.display(), mhqid.display());
  } else if (previous == MHempty) {
    // Element is at the head of the list
    // The rt structure element must remain valid for a time, we guarantee
    // this by leaving 500 free elements, minimum.
    global_hash[key] = rt[elemid].globalnext;
  } else {
    // Element is inside the list.
    // The rt structure element must remain valid for a time, we guarantee
    // this by leaving 500 free elements, minimum.
    rt[previous].globalnext = rt[elemid].globalnext;
  }

  // take off the hashed list of names for this host.
  key = (Short) (mhname.foldKey() % MHnamehashtblSz);

  previous = MHempty;

  for (n = hostlist[host].hashlist[key];
       (n != MHempty) && (n != elemid); n = rt[n].hostnext) {
    previous = n;
  }

  if (n == MHempty) {
    printf("deleteName: Name %s(%s) not on local list",
           mhname.display(), mhqid.display());
  } else if (previous == MHempty) {
    // Element is at the head of the list
    // The rt structure element must remain valid for a time, we
    // gurantee this by leaving 500 free elements, minimum
    hostlist[host].hashlist[key] = rt[elemid].hostnext;
  } else {
    // Element is inside the list
    // The rt structure element must remain valid for a time,
    // we gurantee this by leaving 500 free elements, minimum
    rt[previous].hostnext = rt[elemid].hostnext;
  }

  // Now we put the block back on the tail of the free list.
  // There are always free entries.
  rt[elemid].nextfree = MHempty;
  rt[free_tail].nextfree = elemid;
  free_tail = elmid;
  TotalNamesReg--; // decrement counter

  if (LocalHostIndex == host) {
    localdata[qid].pid = (pid_t)-1;
    localdata[qid].auditcount = 0;
    MHmsgh.emptyQueue(mhqid);
    localdata[qid].inUse = FALSE;
  }

  if ((rt[elemid].global != FALSE) &&
      // Deletion of a global name, must send out info to other hosts
      (host == LocalHostIndex)) {
    int i;
    for (i = 0; i < MHmaxHostreg; i++) {
      if (hostlist[i].isused == TRUE) {
        if (oam_lead[0].display()[0] != 0) {
          // System is configured, if not cluster global, skip
          if (rt[elemid].global != MH_allNodes &&
              hostlist[i].nameRCS[0] == 0) {
            continue;
          }
        }
        // We have a host with MSGH active.
        // Construct a MSGH rmName msg and send it
        rmName.mhqid = MHMAKEMHQID(getActualLocalIndex(i), MHQID2QID(mhqid));
        MHmsgh.send(MHMAKEMHQID(i, MHmsghQ), (Char *)&rmName,
                    sizeof(MHrmName), 0);
      }
    }
  }

  if (host == MHgQHost) {
    // Delete the global queue, should only happen as a result
    // of audit errors since global queues cannnot be deleted by clients
    gqdata[qid].m_selectedQ = MHempty;
    for (int j = 0; j < MHmaxRrealQs; j++) {
      gqdata[qid].m_realQ[j] = MHempty;
    }
  } else if (host == MHdQHost) {
    // Delete the distributive queue, should only happen as a result
    // of audit errors since global queues cannot be deleted by clients
    dqdata[qid].m_nextQ = MHempty;
    for (int j = 0; j < MHmaxDistQs; j++) {
      dqdata[qid].m_realQ[j] = MHnullQ;
      dqaata[qid].m_enabled[j] = FALSE;
    }
  } else {
    // Real queue, check if mapped to any global or distributive
    // gueue and delete the mapping
    for (n = 0; n < MHmaxQid; n++) {
      // Find the queue mapping and delete it
      if (gqdata[n].m_selectedQ != MHempty) {
        for (int j = 0; j < MHmaxRealQs; j++) {
          if (gqdata[n].m_realQ[j] == mhqid) {
            gqdata[n].m_realQ[j] = MHempty;
            gqdata[n].m_timestamp = time(NULL);
            if (gqdata[n].m_selectedQ == j) {
              setGlobalQ(n);
            }
            break;
          }
        }
      }
    }

    for (n = 0; n < MHmaxQid; n++) {
      // Find the queue mapping and delete it
      if (dqdata[n].m_nextQ != MHempty) {
        for (int j = 0; j < MHmaxDistQs; j++) {
          if (dqdata[n].m_realQ[j] == mhqid) {
            dqdata[n].m_realQ[j] = MHempty;
            dqdata[n].m_enabled[j] = FALSE;
            dqdata[n].m_timestamp = time(NULL);
            setDistQ[n];
            break;
          }
        }
      }
    }
  }
}

// make sure host conforms to map sent in MHnameMap message
Bool MHrt::conform(Short host, MHname names[MHmaxQid], Short startqid,
                   Short count, Bool fullreset) {
  Bool ret = TRUE;
  int i, j;

  if (host == LocalHostIndex) {
    printf("far host id %d = local host id %d\n", host, LocalHostIndex);
    return(FALSE);
  }

  for (i = startqid, j = 0; j < count && i < MHmaxQid; j++, i++) {
    Short procindex = hostlist[host].indexlist[i];
    if (names[i].display()[0] != 0) {
      if (procindex == MHempty) {
        if (!fullreset && IN_INSTEADY()) {
          printf("conform: Host %s(%d) qid %d shouldn't be empty!",
                 hostlist[host].hostname.display(), host, i);
          ret = FALSE;
        } else {
          printf("conform: Host %s(%d) qid %d shouldn't be empty!",
                 hostlist[host].hostname.display(), host, i);
        }
        insertName(names[j].display(), MHMAKEMHQID(host, i),
                   0, TRUE, FALSE, MHmaxNets);
      } else if (rt[procindex].mhname != names[j]) {
        printf("conform: Host %s(%d) qid %d %s != %s",
               hostlist[host].hostname.display(), host, i,
               names[j].display(), rt[procindex].mhname.display());
        insertName(names[j].display(), MHMAKEMHQID(host, i),
                   0, TRUE, FALSE, MHmaxNets);
        ret = FALSE;
      }
    } else {
      if (procindex == MHempty)
         continue;

      printf("conform: Host %s(%d) name %s is being deleted due to host audit",
             hostlist[host].hostname.display(), host, i,
             rt[procindex].mhname.display());
      // Oops, must have lost an MHrmName messages!
      deleteName(rt[procindex].mhname, MHMAKEMHQId(host, i), -1);
      // Re-check our data.
      ret = FALSE;
    }
  }
  return(ret);
}

// make sure global queues conform to map sent in MHgQMap message
void MHrt::gQconform(MHggQData auddata[MHmaxQid], Short startqid,
                     Short count, Bool fullreset, MHqid leadQ) {
  int i,j,k;
  int host = MHgQHost;
  Short procindex;

  // This message only comes from lead CC reset the lead CC
  if (m_clusterlead != MHQID2HOST(leadQ) &&
      (m_clusterLead == LocalHostIndex ||
       m_clusterlead == SecondaryHostIndex)) {
    printf("gQconform: resetting lead MSGH to %d when this host was lead",
           MHQID2HOSt(leadQ));
  }
  // Do not reset lead when lead was never initialized
  // because this will break initialization sequencing
  if (m_clusterLead != MHempty) {
    m_clusterLead = MHQID2HOST(leadQ);
  }

  if ((m_envType == MH_peerCLuster || SecondaryHostIndex != MHnone) &&
      ((startqid + count > MHclusterGlobal) && startqid < MHsystemGlobal)) {
    printf("should not receive queues over %d in peer cluster, received"
           "%d from host %d", MHclusteGlobal,
           startqid + count && i < MHmaxQid; j++, i++);
    return;
  }

  for (i = startqid, j = 0; j < count && i < MHmaxQid; j++, i++) {
    const char *name = auddata[j].mhname.display();
    Short procindex = hostlist[host].indexlist[i];

    if (name[0] != 0) { // There should be an entry here...
      if (procindex == MHempty) {
        if (!fullreset && IN_INSTEADY()) {
          printf("gQconfform: Host %s(%d) qid %d should't be empty!",
                 hostlist[host].hostname.display(), host, i);
        }
        insertName(name, MHMAKEMHQID(host, i), 0, TRUE, FALSE, MHmaxNets,
                   FALSE, -1L, TRUE, MHnullQ, auddata[j].m_bKeepOnLead);
        // update the reset of the global queue info
        for (k = 0; k < MHmaxRealQs; k++) {
          gqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
        }
        gqdata[i].m_selectedQ = auddata[j].m_selectedQ;
        gqdata[i].m_bKeepOnLead = auddata[j].m_bKeepOnLead;
      } else if (rt[procindex].mhname != auddata[j].mhname) {
        printf("gQconform: Host %s(%d) qid %d %s != %s",
               hostlist[host].hostname.display(),
               host, i, name, rt[procindex].mhname.display());
        // Delete the mismatched name
        deleteName(rt[procindex].mhname, MHMAKEMHQID(MHgQHost, j), -1);
        // If the new name alse exists, delete that one also
        if ((procindex = findNameOnHost(MHgQHost, name)) != MHempty) {
          deleteName(name, rt[procindex].mhqid, -1);
        }
        insertname(name, MHMAKEMHQID(host, i), 0, TRUE, FALSE, MHmaxNets,
                   FALSE, -1L, TRUE, MHnullQ, auddata[j].m_bKeepOnLead);
        for (k = 0; k < MHmaxRealQs; k++) {
          gqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
        }
        gqdata[i].m_selectedQ = auddata[j].m_selectedQ;
        gqdata[i].m_bKeepOnLead = auddata[j].m_bKeepOnLead;
      } else {
        // check real queues for matches
        for (k = 0; k < MHmaxRealQs; k++) {
          if (gqdata[i].m_realQ[k] != auddata[j].m_realQ[k]) {
            printf("gQconform realQ %d %s != %s", k,
                   gqdata[i].m_realQ[k].display(),
                   auddata[j].m_realQ[k].display(b));
            gqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
          }
        }
        if (gqdata[i].m_selectedQ != auddata[j].m_selectedQ) {
          // Selected Q mismatch
          printf("gQconform: selectedQ lead value %d != local value %d, qid %d",
                 auddata[j].m_selectedQ, gqdata[i].m_selectedQ, i);
          gqdata[i].m_selectedQ = auddata[j].m_selectedQ;
        }
        if (gqdata[i].m_bKeepOnlead != auddata[j].m_bKeepOnLead) {
          printf("gQconform: bKeepOnLead lead value %d != local value %d, qid %d",
                 auddata[j].m_bKeepOnLead, gqdata[i].m_bKeepOnLead, i);
          gqdata[i].m_bKeepOnLead = auddata[j].m_bKeepOnLead;
        }
      }
    } else { // There should be No map entry here!
      if (procindex == MHempty)
         continue;
      printf("gQconform: Host %s(%d) qid %d name %s is being deleted due "
             "to host audit", hostlist[host].hostname.display(), host,
             i, rt[procindex].mhname.display());
      // Oops, must have lost an MHrmName message
      deleteName(rt[procindex].mhname, MHMAKEMHQID(host, i), -1);
    }
  }
  return;
}

// make sure global queues conform to map sent in MHgQMap message
void MHrt::dQconform(MHdQData auddata[MHmaxQid], Short startqid,
                     Short count, Bool fullreset, MHqid leadQ) {
  Bool deleteQ;
  int i, j, k;
  int host = MHdQHost;
  Short procindex;

  if (m_envType == MH_peerCluster) {
    printf("gQconform should not be sent in peer cluster, received from host %d",
           MHQID2HOST(leadQ));
    return;
  }
  
  // This message only comes from lead CC reset the lead CC
  if (m_leadcc != MHQID2HOST(leadQ) && m_leadcc == LocalHostIndex) {
    printf("gQconform: resetting lead MSGH to %d when this host was lead",
           MHQID2HOST(leadQ));
  }

  // Do not reset lead when lead was never initialized
  // because this will break initialization sequencing
  if (m_leadcc != MHempty) {
    m_leadcc = MHQID2HOST(leadQ);
  }

  for (i = startqid, j = 0; j < count && i < MHmaxQid; j++, i++) {
    const char *name = auddata[j].mhname.display();
    Short proccindex = hostlist[host].indexlist[i];

    if (name[0] != 0) { // There should be an entry here...
      if (procindex == MHempty) {
        if (!fullreset) {
          printf("dQconform: Host %s(%d) qid %d should't be empty!",
                 hostlist[host].hostname.display(), host, i);
        }
        insertName(name, MHMAKEMHQID(host, i), 0, TRUE, FALSE, MHmaxNets,
                   FALSE, -1L, FALSE, MHnullQ, FALSE, TRUE);
        // update the rest of the global queue info
        for (k = 0; k < MHmaxDistQs; k++) {
          dqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
          dqdata[i].m_enabled[k] = auddata[j].m_enabled[k];
        }
        dqdata[i].m_nextQ = auddata[j].m_nextQ;
        dqdata[i].m_maxMember = auddata[j].m_maxMember;
      } else if (rt[procindex].mhname != auddata[j].mhname) {
        printf("dQconform: Host %s(%d) qid %d %s != %s",
               hostlist[host].hostname.display(),
               host, i, name, rt[procindex].mhname.display());
        // Delete the mismatched name
        deletename(rt[procindex].mhname, MHMAKEMHQID(MHdQHost, j), -1);
        // If the new name alse exists, delete that one also
        if ((procindex = findNameOnHost(MHdQHost, name)) != MHempty) {
          deleteName(name, rt[procindex].mhqid, -1);
        }
        inserttName(name, MHMAKEMHQID(host, i), 0, TRUE, FALSE, MHmaxNets,
                    FALSE, -1L, TRUE, MHnullQ, FALSE, TRUE);
        for (k = 0; k < MHmaxDistQs; k++) {
          dqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
          dqdata[i].m_enabled[k] = auddata[j].m_enabled[k];
        }
        dqdata[i].m_nextQ = auddata[j].m_nextQ;
      } else {
        // check real queues for matches
        for (k = 0; k < MHmaxDistQs; k++) {
          if (dqdata[i].m_realQ[k] != auddata[j].m_realQ[k] ||
              dqdata[i].m_enabled[k] != auddata[j].m_enabled[k]) {
            printf("dQconform realQ %d %s != %s or enabled %d != %d",
                   k, dqdata[i].m_realQ[k].display(),
                   auddata[j].m_realQ[k].display(), dqdata[i].m_enabled[k],
                   auddaata[j].m_enabled[k]);
            dqdata[i].m_realQ[k] = auddata[j].m_realQ[k];
            dqdata[i].m_enabled[k] = auddata[j].m_enabled[k];
          }
        }
        if (dqdata[i].m_next() == MHempty && auddata[j].m_nextQ != MHempty) {
          // Selected Q mismatch
          printf("dQconform: nextQ lead value %d != local value %d, qid %d, "
                 "qid %d",auddata[j].m_maxMember, dqdata[i].m_maxMember, i);
          dqdata[i].m_nextQ = auddata[j].m_nextQ;
        }
        if (dqdata[i].m_maxMember != auddata[j].m_maxMember) {
          // Selected Q mismatch
          printf("dQconform: maxMember lead value %d != local value %d, qid %d",
                 auddata[j].m_maxMember, dqdata[i].m_maxMember, i);
          dqdata[i].m_maxMember = auddata[j].m_maxMember;
        }
      }
    } else { // There should be NO map entry here !
      if (procindex == MHempty)
        continue;
      printf("dQconform: Host %s(%d) qid %d name %s is being deleted "
             "due to host audit", hostlist[host].hostname.display(),
             host, i, rt[procindex].mhname.display());
      // Oops, must have lost an MHrmName message!
      deleteName(rt[procindex].mhname, MHMAKEMHQID(host, i), -1);
    }
  }
  return;
}

// Handle ethernet state change message, If the new state of an ethernet
// requires us to select another ethernet, then update SelectedNet variable
// and an audit() to notify all the other hosts
Void MHrt::enetState(Short nEnet, Bool bIsUsable) {
  innt hostnum;
  MHhostDel hostDel;
  hostDel.ptype = MintPtyp;
  hostDel.mtype = MHhostDelTyp;
  hostDel.bReply = TRUE;
  hostDel.clusterlead = m_clusterLead;

  if (bIsUsable == FALSE) {
    if (hostlist[LocalHostIndex].SelectedNet == nEnet) {
      // Must try the other net
      nEnt = (nEnet + 1) % MHmaxNets;
      // Force all the hosts to this enet
      for (hostnum = 0; hostnum < MHmaxHostReg; hostnum++) {
        hostlist[hostnum].SelectedNet = nEnet;
      }
    }
    for (hostnum = 0; hostnum < MHmaxHostReg; hostnum++) {
      if (hostlist[hostnum].isused == FALSE || hostnum == LocalHostIndex ||
          hostnum == SecondaryHostINdex ||
          strncmp(hostlist[hostnum].hostname.display(), "as", 2) == 0) {
        continue;
      }
      hostDel.enet = hostlist[hostnum].SelectedNet;
      hostDel.onEnet = hostlist[hostnum].SelectedNet;
      hostDel.sQid = MHMAKKEMHQID(getActualLocalHostIndex(hostnum), 0);
      MHmsgh.send(MHMAKEMHQID(hostnum, 0), (Char*)&hostDel,
                  sizeof(MHhostDel), 0 , FALSE);
    }
  }
}

// Handle connectivity message from MHRPROC
Void MHrt::conn(Bool bNoMessage[][MHmaxNets], short hostid) {
  // CRmsg om(CL_AUDL);
  int i = 0;
  int j;
  int planned = FALSE;
  MHnodeStChg nodeStChg;

  if (hostid < MHmaxHostReg) {
    // This is only true when sw:cc received
    // so we need to switch global queues
    i = hostid;
    hostid++;
    planned = TRUE;
    for (j = 0; j < MHmaxNets; j++) {
      bNoMessage[i][j] = TRUE;
      hostlist[i].auditcount[j] = hostlist[i].maxMissed;
      hostlist[i].pingTime[j] = hostlist[i].ping
    }
  }

  Bool switchedNet;

  for (; i < hostid; i++) {
    if (hostlist[i].isused = FALSE) {
      continue;
    }
    if (i == LocalHostIndex || i == SecondaryHostIndex) {
      continue;
    }

    switchedNet = FALSE;
    for (j = 0; j < MHmaxnets; j++) {
      hostlist[i].pingTime[j] ++;
      if (bNoMessage[i][j] == FALSE) {
        if (hostlist[i].isactive == FALSE) {
          strcpy(nodeStChg.hostname, hostlist[i].hostname.display());
          nodeStChg.isActive = TRUE;
          hostlist[i].isactive = TRUE;
          MHmsgh.broadcast(MHMAKEMHQID(LocalHostIndex, MHmsghQ),
                           (char*)&nodeStChg, sizeof(nodeStChg), 0L);
          updateClusterlead(i);
          if (hostlist[i].nRouteSync < MHmaxRouteSync) {
            checkNetAudit(MHmsghName, MHMAKEMHQID(i , MHmsghQ), i , FALSE);
            hostlist[i].nRouteSync ++;
          }
        }
        // We might be getting messages but not seding any out
        // make sure that the message window is not closed
        MHhostdata *pHD = &hostlist[i];
        if (j == pHD->SelectedNet &&
            ((pHD->nSend < pHD->nLastAcked ||
              pHD->nSend - pHD->nLastAcked >= pHD->windowSz) &&
             (pHD->nSend >= pHD->nLastAcked ||
              pHD->nLastAcked - pHD->nSend <= MHmaxSQueue - pHD->windowSz))) {
          MHhostDel hostDel;
          hostDel.ptype = MHintPtyp;
          hostdel.mtype = MHhostDeltyp;
          hostDel.sQid = MHMAKEMHQID(getActualLocalHostINdex(i), MHmsghQ);
          hostDel.toQue = MHMAKEMHQID(i, MHmsghQ);
          hostDel.msgSz = sizeof(hostDel);
          MHsetHostId(&hostDel, getActualLocalHostIndex(i));
          hostDel.clusterLead = m_clusterLead;
          hostDel.bReply = TRUE;
          hostDel.enet = hostlist[i].SelectedNet;
          hostDel.onEnet = j;
          MHmsgh.send(MHMAKEMHQID(i, MHmsghQ), (Char *)&hostDel,
                      sizeof(MHhostDel), 0, TRUE);
        }
        hostlist[i].pingTime[j] = 0;
        hostlist[i].auditcount[j] = 0;
        hostlist[i].netup[j] = True;
      } else {
        if (hostlist[i].pingTime[j] < hostlist[i].ping) {
          continue;
        }
        hostlist[i].pingTime[j] = 0;
        hostlist[i].delay++;
        if (hostlist[i].auditcount[j] <= hostlist[i].maxMissed ||
            hostlist[i].delay > MHaudDelay) {
          MHhostDel hostDel;
          hostDel.ptype = MHintPtyp;
          hostDel.mtype = MHhostDelTyp;
          hostDel.sQid = MHMAKEMHQID(getActualLocalHostIndex(i), MHmsghQ);
          hostDel.toQue = MHMAKEMHQID(i, MHmsghQ);
          hostDel.msgSz = sizeof(hostDel);
          MHsetHostId(&hostDel, getActualLocalHostIndex(i));
          hostDel.HostId != MHunBufferedFlg;
          hostDel.clusterlead = m_clusterLead;
          hostDel.bReply = TRUE;
          // Send hostDel message on each network
          if (!switchedNet && j == hostlist[i].Selectednet &&
              hostlist[i].auditcount[j] > 0) {
            hostlist[i].SelectedNet = (hostlist[i].SelectedNet + 1) % MHmaxNetts;
            switchedNet = TRUE;
          }
          hostDel.enet = hostlist[i].SelectedNet;
          hostDel.onEnet = j;
          int ret;
          // Only heartbeat every cycle if active net otherwise wait for
          // MinStdybMissed cycles
          if ((j == hostlist[i].Selectednet) ||
              (hostlist[i].auditcount[j] > MHminStdbyMissed) ||
              switchedNet) {
            if ((ret = sendto(MHsock_id, (char*)&hostDel, sizeof(hostDel), 0,
                              (sockaddr*)&hostlist[i].saddr[j],
                              sizeof(sockaddr_in6))) != sizeof(hostDel)) {
              printf("Failed net audit send errno %d", ret);
            }
          }
          hostlist[i].delay = 0;
          if (j == hostlist[i].SelectedNet || switchedNet) {
            MHsetHostId(&hostDel, getActualLocalHostIndex(i));
            MHmsgh.send(MHMAKEMHQID(i, MHmsghQ), (Char*)&hostDel,
                        sizeof(MHhostDel), 0, TRUE);
          }
        }
        if (hostlist[i].auditcount[j] >= hostlist[i].maxMissed) {
          if (hostlist[i].netup[j] == TRUE) {
            // Send message to FTMON
            MHnetStChg netStChg;
            hostlist[i].netup[j] = FALSE;
            netStChg.send(MHMAKEMHQID(LocalHostIndex, MHmsghQ));
          }
        } else {
          hostlist[i].auditcount[j]++;
        }
      }
    }
    if (hostlist[i].isactive) {
      Bool oneup = false;
      for (j = 0; j < MHmaxnets; j++) {
        if (hostlist[i].netup[j] == TRUE) {
          oneup = TRUE;
        }
      }
      if (oneup == FALSE) {
        printf("MHconn: NO REPLY FROM HOST");
        printff("HOSTID=%d LNAME=%s RNAME=%s",
                i , hostlist[i].hosttname.display(),
                hostlist[i].realname);
      }
    }
  }
}
