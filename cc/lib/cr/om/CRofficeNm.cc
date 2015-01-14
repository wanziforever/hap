/*
**      File ID:        @(#): <MID18636 () - 08/17/02, 29.1.1.1>
**
**
**	File:					MID18636
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:53
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/utsname.h>
#include <ctype.h>

#include "cc/hdr/cr/CRofficeNm.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"


/* create global variable to hold the switch name
** This should really be in shared memory controlled by CSOP.
*/
CRofficeName CRswitchName;


CRofficeName::CRofficeName()
{
	init();
}

void
CRofficeName::init()
{
	setName("");
}

const char*
CRofficeName::getName()
{
	if (name[0] == '\0')
	{
		struct utsname un;
		int retval = uname(&un);
		if (retval == -1)
		{
			setName("UNKNOWN");
		}
#ifdef __mips
		else
		{
			/* copy and capitalize the system name */
			for (char* t = un.sysname, *h = name;
			     *t; t++, h++)
				*h = toupper(*t);
			*h = '\0';
		}
#endif

#ifdef EES
		else
		{
			int slen = strlen(un.nodename);
			if (un.nodename[slen-1] == '0' || 
			    un.nodename[slen-1] == '1')
			{
				if (un.nodename[slen-2] == '-') 
					un.nodename[slen-2] = '\0';
			}

			char *t, *h;
			for (t = un.nodename, h = name; *t; t++, h++)
			*h = toupper(*t);
			*h = '\0';
			return name;
		}
// IBM thomharr 20060911 - should be mutually exclusive.
//#endif
//#ifdef CC
#elif LX
		else
		{
			int slen = strlen(un.nodename);
			//remove -R-C-S from host name
			if(slen > 6)
			{
				un.nodename[slen-6]='\0';
			}

			char *t, *h;
			for (t = un.nodename, h = name; *t; t++, h++)
			*h = toupper(*t);
			*h = '\0';
			return name;
		}
#else
		else
		{
			int slen = strlen(un.nodename);
			if (un.nodename[slen-1] == '0' || 
			    un.nodename[slen-1] == '1')
			{
				if (un.nodename[slen-2] == '-') 
					un.nodename[slen-2] = '\0';
			}

			char *t, *h;
			for (t = un.nodename, h = name; *t; t++, h++)
			*h = toupper(*t);
			*h = '\0';
			return name;
		}
#endif
		
	}
	return name;
}

void
CRofficeName::setName(const char* newValue)
{
	strncpy(name, newValue, CRmaxOfficeName);
	name[CRmaxOfficeName] = '\0';
}


