/*
**	File:					MID3740
**	Release:				2.1.1.1
**	Date:					4/5/90
**	Time:					15:44:16
**	Newest applied delta:	4/5/90
**
** DESCRIPTION:
**	These are the member functions for the base class of all messages
**
** OWNER:
**	Ed Weiss
**
** NOTES:
*/

//	#include's go here

#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"

//	const's & global static constants go here

static const char *sccs_id = "@(#): <MID3740 () - 4/5/90, 2.1.1.1>";

//	macros go here

//	static global variable definitions/initalizations go here

//	global variable definitions go here

//	static functions go here

//	functions go here (including member functions)

GLretVal
MHmsgBase::send(MHqid toQid, MHqid fromQid, Long len, Long time, Bool buffered) {
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, len, time, buffered);
}

GLretVal
MHmsgBase::send(const char *name, MHqid fromQid, Long len, Long time, Bool buffered) {
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, len, time, buffered);
}

Void
MHmsgBase::display() {
/* To be added in load1
	CRmsg	debugMsg;
	debugMsg.add("\tMessage Type\t%ld\n", msgType);
	debugMsg.add("\tSource Queue\t%d\n", srcQue);
	debugMsg.add("\tPriority\t%d\n", priType);
	debugMsg.spool();
*/
}

//	Each derived class must override these

Void
MHmsgBase::printName() {
/* to be added in load  1
	CRmsg	debugMsg;
	debugMsg.add("Class Name is %d\n", (const char *) asString());
	debugMsg.spool();
*/
}
