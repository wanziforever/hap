/*
**      File ID:        @(#): <MID18633 () - 08/17/02, 29.1.1.1>
**
**	File:					MID18633
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:52
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	The function getLoadName() returns the current load name
**      to be used in Output Message headers.  The current load should
**      be set in a special file (/sn/release/version).
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <String.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cc/hdr/cr/CRloadName.hh"

/* global variable to store the current load.
** This should probably come from a global parameter,
** but is currently taken from the environment variable $SNLOAD
** when the build was performed.  The head CRcurLoad.h is generated
** during the build with this information in it.
*/

CRloadName CRcurrentLoad;

CRloadName::CRloadName()
{
	init();
}

void
CRloadName::init()
{
	name[0] = 1;
}

const char*
CRloadName::getName()
{
	if (name[0] == 1)
	{
		const char* fileName = "/sn/release/version";
		FILE* fp = fopen(fileName, "r");
		if (fp == NULL)
		{
			setName("");
			return name;
		}

		char inbuf[CRmaxLoadName+1];
		if (fgets(inbuf, sizeof(inbuf), fp) != NULL)
		{
			for (char* tmp = inbuf; *tmp; tmp++)
			{
				if (isspace(*tmp))
				{
					*tmp = '\0';
					break;
				}
			}
			
			setName(inbuf);
		}
		
		fclose(fp);
	}
	return name;
}

void
CRloadName::setName(const char* newValue)
{
	strncpy(name, newValue, CRmaxLoadName);
	name[CRmaxLoadName] = '\0';
}



