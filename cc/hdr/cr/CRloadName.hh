#ifndef __CRLOADNAME_H
#define __CRLOADNAME_H
/*
**      File ID:        @(#): <MID13733 () - 08/17/02, 29.1.1.1>
**
**	File:					MID13733
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:32:19
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This class manages the current load name
**      to be used in Output Message headers.  An object of this class
**      should be instantiated to follow the value of the
**      global office parameter that represents the current load name.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

class CRloadName
{
      public:
	CRloadName();
	void init();
	const char* getName();
	void setName(const char* newValue);

	enum { CRmaxLoadName = 25 };
	
      private:
	char name[CRmaxLoadName+1];
};

extern CRloadName CRcurrentLoad;
#endif
