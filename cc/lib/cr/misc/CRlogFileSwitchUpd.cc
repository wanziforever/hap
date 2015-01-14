/*
**      File ID:        
**
**	File:					
**	Release:				
**	Date:					03/13/03
**	Time:					
**	Newest applied delta:	
**
** DESCRIPTION:
**	This file defines the class used by CSOP to give information
**	regarding logfile switch overs.
**
** OWNER: 
**	Nagaraj Narayanaswamy
**
** NOTES:
**
*/
#include "string.h"
#include "cc/hdr/cr/CRmtype.hh"
#include "cc/hdr/cr/CRlogFileSwitchUpd.hh"

//Global Variable - to keep track of the queueNames which are interested in
//the switch over event.

std::vector<std::string> interestedQueues;

CRlogFileSwitchEvent::CRlogFileSwitchEvent()
{
    priType = MHoamPtyp;
    msgType = CRlogFileSwitchEventTyp;
}

CRlogFileSwitchEvent::~CRlogFileSwitchEvent()
{
}

void CRlogFileSwitchEvent::setNewLogFileName(const char *filename)
{
    strncpy(newLogFile, filename, MAX_LOGFILENAME_LEN);
    newLogFile[MAX_LOGFILENAME_LEN-1] = 0;
}

const char* CRlogFileSwitchEvent::getNewLogFileName() 
{ 
    return newLogFile; 
}
    
void CRlogFileSwitchEvent::setOldLogFileName(const char *filename)
{
    strncpy(oldLogFile, filename, MAX_LOGFILENAME_LEN);
    oldLogFile[MAX_LOGFILENAME_LEN-1] = 0;
}

const char *CRlogFileSwitchEvent::getOldLogFileName()
{
    return oldLogFile;
}

CRlogFileSwitchUpd::CRlogFileSwitchUpd()
{
    priType = MHoamPtyp;
    msgType = CRlogFileSwitchUpdTyp;
}

CRlogFileSwitchUpd::~CRlogFileSwitchUpd()
{
}

GLretVal
CRlogFileSwitchUpd::statusReg(const Char *msghName)
{
    /* Null Queue or QueueName with Zero Length - do not Deregister */
    if((!msghName) || (strlen(msghName) <= 0)) 
       return GLfail;

    type = REGISTER;
    strncpy(qName, msghName, MHmaxNameLen);
    qName[MHmaxNameLen-1] = 0;

    Short msgsz = CRlogFileSwitchUpdSz;

    return (send("CSOP", MHnullQ, msgsz, -1));
}

GLretVal
CRlogFileSwitchUpd::statusDereg(const Char *msghName)
{
    /* Null Queue or QueueName with Zero Length - do not Deregister */
    if((!msghName) || (strlen(msghName) <= 0)) 
       return GLfail;


    type = DEREGISTER;
    strncpy(qName, msghName, MHmaxNameLen);
    qName[MHmaxNameLen-1] = 0;

    Short msgsz = CRlogFileSwitchUpdSz;

    return (send("CSOP", MHnullQ, msgsz, -1));
}

