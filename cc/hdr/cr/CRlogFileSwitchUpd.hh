#ifndef __CRLOGFILESWITCHUPD_H
#define __CRLOGFILESWITCHUPD_H
/*
**      File ID:        
**
**      File:                                   
**      Release:                                
**      Date:                                   03/13/03
**      Time:                                   
**      Newest applied delta:   
**
** DESCRIPTION:
**      This file represents the class definition for communications
**      between CSOP and any process registering for LogFileSwitchEvents
**
** OWNER:
**      Nagaraj Narayanaswamy
**
** NOTES:
**
*/

#include "cc/hdr/msgh/MHnames.hh"
#include "cc/hdr/msgh/MHmsgBase.hh"

#include <string>
#include <vector>

const int MAX_LOGFILENAME_LEN = 32;

/*
**
** DESCRIPTION:
**      This class represent MSGH message that CSOP will send to all registed  
**      processes when the logFile Switches.
**
*/
class CRlogFileSwitchEvent : public MHmsgBase 
{
public:
    CRlogFileSwitchEvent();
    ~CRlogFileSwitchEvent(); 

    void setNewLogFileName(const char *filename);
    const char *getNewLogFileName();

    void setOldLogFileName(const char *filename);
    const char *getOldLogFileName();

private:
    char newLogFile[MAX_LOGFILENAME_LEN];
    char oldLogFile[MAX_LOGFILENAME_LEN];
};

const int CRlogFileSwitchEventSz = sizeof(CRlogFileSwitchEvent);

/*
**
** DESCRIPTION:
**      This class represent MSGH message that will register processes that
**      want LogFile Switch Notification. 
*/

enum { REGISTER, DEREGISTER };

class CRlogFileSwitchUpd : public MHmsgBase 
{

public:
    CRlogFileSwitchUpd() ;
   
    ~CRlogFileSwitchUpd() ;

    GLretVal statusReg(const Char *msghName);

    GLretVal statusDereg(const Char *msghName);

    Short    getType() { return type; }

    const char* getQueueName() { return qName; }

private:
    char qName[MHmaxNameLen];

    Short type;

};

const int CRlogFileSwitchUpdSz = sizeof(CRlogFileSwitchUpd);

extern std::vector<std::string> interestedQueues;

#endif
