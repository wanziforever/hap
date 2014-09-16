#ifndef __MHNAME_H
#define __MHNAME_H

// DESCRIPTION:
// 	The MSGH subsystem recognizes only the first 12 characters
//	of each registered name. In other words, the first 12
//	characters of each registered name must be unique.
//	This file defines a class that handles the operation of
//	MSGH names. 
//
// OWNER:
//	Shi-Chuan Tu
//
// NOTES:
//

#include <string.h>
#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHnames.hh"


class MHname {
public:
	inline MHname();
  inline MHname(const Char *);
  inline MHname& operator=(const MHname&);  // assignment operator 
  inline MHname& operator=(const Char *);   // assignment operator
	inline Short operator==(const MHname&) const;   // equality operator
	inline Short operator==(const Char *) const;    // equality operator
	inline Short operator!=(const MHname&) const;   // inequality operator
	inline Short operator!=(const Char *) const;   // inequality operator
	inline Char *dump(Char *ptr) const;	   
	inline const Char *display() const {return(name);};
	Long foldKey() const;	   		   // get a numeric key

private:
	Char name[MHmaxNameLen+1];
};


inline 
MHname::MHname()
{
  name[MHmaxNameLen] = '\0'; 
}


inline 
MHname::MHname(const Char *ptr)
{
  strncpy(name, ptr, MHmaxNameLen);
  name[MHmaxNameLen] = '\0';
}


/* Assign a a MHname object to another MHname object */
inline MHname&
MHname::operator=(const MHname &mhname)
{
  strncpy(name, mhname.name, MHmaxNameLen+1);
  return (*this);
}


/* Assign a character string to a MHname object */
inline MHname&
MHname::operator=(const Char *ptr)
{
  strncpy(name, ptr, MHmaxNameLen);
  name[MHmaxNameLen] = '\0';
  return (*this);
}


/* Returns 1 if both objects are equal; returns 0 otherwise */
inline Short
MHname::operator==(const MHname& mhname) const
{
  return (strcmp(name, mhname.name) == 0);
}


/* Returns 1 if both objects are equal; returns 0 otherwise */
inline Short
MHname::operator==(const Char *ptr) const
{
  return (strncmp(name, ptr, MHmaxNameLen) == 0);
}


/* Returns non-zero if two objects are not equal; returns 0 otherwise */
inline Short
MHname::operator!=(const MHname& mhname) const
{
  return (strcmp(name, mhname.name));
}


/* Returns non-zero if two objects are not equal; returns 0 otherwise */
inline Short
MHname::operator!=(const Char *ptr) const
{
  return (strncmp(name, ptr, MHmaxNameLen));
}


/*
 * Fills the buffer pointed to by `ptr' with the character string of the
 * MHname object and returns the character pointer. It is up to the
 * programmer to be sure the buffer is large enough.
 */
inline Char *
MHname::dump(Char *ptr) const
{
  strncpy(ptr, name, MHmaxNameLen+1);
  return (ptr);
}

extern const Char MHmsghName[];

#endif
