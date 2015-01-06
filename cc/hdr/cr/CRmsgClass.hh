#ifndef __CRMSGCLASS_H
#define __CRMSGCLASS_H
/*
**      File ID:        @(#): <MID13732 () - 08/17/02, 29.1.1.1>
**
**	File:					MID13732
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:32:18
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file contains the declaration of the CROMclass class
**      which represents an Output Message Class name.  This class
**      is basically a simplified version of the String class with
**      the name having a fixed upper limit, so no dynamic memory
**      allocation is performed.
**
**      This file also contains declarations of several Output Message
**      class constants.  These are for backward compatability with
**      the CRmsg class.
**
** OWNER:
**	Roger McKee
**
** NOTES:
**      The operator "<" is required by the Map class.
**      The const char*() conversion operator is not implemented,
**      since it causes a problem with the CRmsg class.  The problem
**      has to do with implicit conversion of parameters to functions
**      and function overloading.  DO NOT implement this function.
**      The function 'getCharStar()' is provided to perform the 'const char*'
**      conversion, but it must be explicitly called.
*/


/* Output message classes */

enum CROMCLASS {
/* improperly formatted spooler messages */
	CL_SPERR = 0,

/* General purpose maintenance messages,
** such as indication of removals and restorals.
*/
	CL_MAINT = 1,

/* class used for low priority general purpose maintenance messages */
	CL_MAIPR = 2,

/* output messages related to debugging */
	CL_DEBUG = 3,

/* audit messages that are to be outputted to a terminal immediately */
	CL_AUDT = 4,

/* audit messages that are normally routed to a log file. */
	CL_AUDL = 5,

/* recent change messages */
	CL_RC = 6,

/* assert messages (should be logged) */
	CL_ASSERT = 7,

/* output class for system integrity */
	CL_SIM = 8,

/* plant measurements */
	CL_PLNT = 9,

/* traffic measurements */
	CL_TRFM = 10,

/* input message echoing */
	CL_INECHO = 11,

/* trap and trace */
	CL_TTLOG = 12,

	CL_LAST
};

/* maximum number of characters in a message class name */
#define CRmaxClassSz  8

class CROMclass 
{
    public:
	CROMclass();
	CROMclass(CROMCLASS omClassNum);
   	CROMclass(const char *);
//	CROMclass(const CROMclass&);		memberwise initialization is ok
//	~CROMclass();		nothing to destruct

  	CROMclass& operator=(const CROMclass&);  // assignment operator 
  	CROMclass& operator=(const char *);   // assignment operator
	short operator==(const CROMclass&) const;   // equality operator
	short operator==(const char *) const;    // equality operator
	short operator!=(const CROMclass&) const;   // inequality operator
	short operator!=(const char *) const;   // inequality operator
	int operator<(const CROMclass&) const; // less than operator 
	char *dump(char *ptr) const;	   
//	operator const char*() const;
	const char* getCharStar() const;

    private:
	char name[CRmaxClassSz+1];
};


#include <string.h>

inline 
CROMclass::CROMclass()
{
	name[0] = '\0';
}

inline 
CROMclass::CROMclass(const char *ptr)
{
    strncpy(name, ptr, CRmaxClassSz);
    name[CRmaxClassSz] = '\0';
}

/* Assign a a CROMclass object to another CROMclass object */
inline CROMclass&
CROMclass::operator=(const CROMclass &srcname)
{
    strcpy(name, srcname.name);
    return (*this);
}

/* Assign a character string to a CROMclass object */
inline CROMclass&
CROMclass::operator=(const char *ptr)
{
    strncpy(name, ptr, CRmaxClassSz);
    name[CRmaxClassSz] = '\0';
    return (*this);
}

/* Returns 1 if both objects are equal; returns 0 otherwise */
inline short
CROMclass::operator==(const CROMclass& mhname) const
{
    return (strcmp(name, mhname.name) == 0);
}

/* Returns 1 if both objects are equal; returns 0 otherwise */
inline short
CROMclass::operator==(const char *ptr) const
{
    return (strncmp(name, ptr, CRmaxClassSz) == 0);
}

/* Returns non-zero if two objects are not equal; returns 0 otherwise */
inline short
CROMclass::operator!=(const CROMclass& rhsname) const
{
    return (strcmp(name, rhsname.name));
}

/* Returns non-zero if two objects are not equal; returns 0 otherwise */
inline short
CROMclass::operator!=(const char *ptr) const
{
    return (strncmp(name, ptr, CRmaxClassSz));
}

/*
 * Fills the buffer pointed to by `ptr' with the character string of the
 * CROMclass object and returns the character pointer. It is up to the
 * programmer to be sure the buffer is large enough.
 */
inline char *
CROMclass::dump(char *ptr) const
{
    strncpy(ptr, name, CRmaxClassSz+1);
    return (ptr);
}

inline const char*
CROMclass::getCharStar() const
{
	return name;
}

/* less than operator 
** This operator is necessary for the Map macro library to work properly
*/
inline int
CROMclass::operator<(const CROMclass& rhsname) const
{
	return (strcmp(name, rhsname.name) < 0);
}
#endif
