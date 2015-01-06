#ifndef __CROFFICENM_H
#define  __CROFFICENM_H
/*
**      File ID:        @(#): <MID13729 () - 08/17/02, 29.1.1.1>
**
**	File:					MID13729
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:32:17
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This class manages the office name (switch name)
**      to be used in Output Message headers.  An object of this class
**      should be instantiated to follow the value of the
**      global office parameter that represents the office identifier
**      of this SCN.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

class CRofficeName
{
      public:
	CRofficeName();
	void init();
	const char* getName();
	void setName(const char* newValue);

	enum { CRmaxOfficeName = 25 };
	
      private:
	char name[CRmaxOfficeName+1];
};

extern CRofficeName CRswitchName;
#endif
