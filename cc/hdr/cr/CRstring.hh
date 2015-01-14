#ifndef __CRSTRING_H
#define __CRSTRING_H

/*
**      File ID:        @(#): <MID17536 () - 08/17/02, 29.1.1.1>
**
**	File:					MID17536
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:32:20
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This function contains the declaration (and definition)
**      of the generic CRstr class which
**      represents a static (fixed maximum size) string.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**      The operator '<' is needed by the Map class.
**
** dump() Fills the buffer pointed to by `ptr' with the string of the
** CRstr(len) object and returns the character pointer. It is up to the
** programmer to be sure the buffer is large enough.
*/

#include <string.h>
/* IBM jerrymck 20060601 */
/* Generic.h does not exist on Linux - add new generic header defs for Linux */
#include <luc_compat.h>

#define CRstr(CRSLEN)   name2(CRstr,CRSLEN)

#define CRstrdeclare(CRSLEN) \
class CRstr(CRSLEN) { \
    private: \
	char s[CRSLEN+1]; \
    public: \
	CRstr(CRSLEN)() { s[0] = '\0'; s[CRSLEN] = '\0'; } \
   	CRstr(CRSLEN)(const char *p) { strncpy(s, p, CRSLEN); s[CRSLEN] = '\0'; } \
/*	CRstr(CRSLEN)(const CRstr(CRSLEN)&);	memberwise initialization is ok */ \
/*	~CRstr(CRSLEN)();		nothing to destruct */ \
	operator const char*() const { return s; } \
	operator char*() { return s; } \
  	CRstr(CRSLEN)& operator=(const char *p) { \
		strncpy(s, p, CRSLEN); s[CRSLEN] = '\0'; return *this; } \
	CRstr(CRSLEN)& operator+=(const char*p) { \
		strncat(s, p, CRSLEN-strlen(s)); s[CRSLEN] = '\0'; return *this; } \
	int operator==(const char *p) const { return strncmp(s, p, CRSLEN) == 0; } \
	int operator!=(const char *p) const { return strncmp(s, p, CRSLEN); } \
	int operator<(const CRstr(CRSLEN)& p) const { return strcmp(s, p.s) < 0; } \
	int length() const { return strlen(s); } \
	char *dump(char *p) const { strncpy(p, s, CRSLEN+1); return p; } \
};


/* String class that keeps the string in upper case */

#define CRucstr(CRSLEN)   name2(CRucstr,CRSLEN)

#define CRucstrdeclare(CRSLEN) \
class CRucstr(CRSLEN) { \
    private: \
	char s[CRSLEN+1]; \
    public: \
	CRucstr(CRSLEN)() { s[0] = '\0'; s[CRSLEN] = '\0'; } \
   	CRucstr(CRSLEN)(const char *p) { CRstrnUCcpy(s, p, CRSLEN); \
                s[CRSLEN] = '\0'; } \
/*	CRucstr(CRSLEN)(const CRucstr(CRSLEN)&); memberwise init is ok */ \
	operator const char*() const { return s; } \
  	CRucstr(CRSLEN)& operator=(const char *p) { \
	       CRstrnUCcpy(s, p, CRSLEN); s[CRSLEN] = '\0'; \
	       return *this; } \
	int operator==(const CRucstr(CRSLEN)& p) const { \
               return strncmp(s, p, CRSLEN) == 0; } \
	int operator==(const char *p) const { \
               return CRstrnUCcmp(s, p, CRSLEN) == 0; } \
	int operator!=(const CRucstr(CRSLEN)& p) const { \
               return strncmp(s, p, CRSLEN); } \
	int operator!=(const char *p) const { \
               return CRstrnUCcmp(s, p, CRSLEN); } \
	int operator<(const CRucstr(CRSLEN)& p) const { \
               return strcmp(s, p.s) < 0; } \
	char *dump(char *p) const { strncpy(p, s, CRSLEN+1); return p; } \
};

extern char* CRstrnUCcpy(char*, const char*, int);
extern int CRstrnUCcmp(const char* ucname, const char* lcname, int);


#endif
