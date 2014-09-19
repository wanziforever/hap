#include <stdio.h>
#include <String.h>
#include "cc/hdr/cr/CRstringList.H"

CRstringList::CRstringList() {}

CRstringList::~CRstringList()
{
	clear();
}

void
CRstringList::clear()
{
	String* sptr;
	while (sptr = (String*) list.pop())
		delete sptr;
}

int
CRstringList::length() const
{
	return list.length();
}

void
CRstringList::put(String* sptr)
{
	if (sptr != NULL)
		list.append((void*) sptr);
}

void
CRstringList::append(String& s)
{
	if (!in_list(s))
	{
		String *s_ptr = new String(s);
		list.append((void*) s_ptr);
	}
}


// return index of entry in list (starting with 1)
// returns 0 (alias false) if not in list
int
CRstringList::in_list(const String& s)
{
	CRstringListIterator iter(this);
	String* str_ptr;

	int i = 1;

	while (str_ptr = iter.next())
	{
		if (s == *str_ptr)
			return i;
		i++;
	}

	return 0;
}

void
CRstringList::dump() const
{
	CRstringListIterator iter((CRstringList*) this);
	String* str_ptr;

	while (str_ptr = iter.next())
		fprintf(stderr, "'%s' ", (const char*) (*str_ptr));
}
