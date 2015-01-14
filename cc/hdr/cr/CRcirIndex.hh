#ifndef __CRCIRINDEX_H
#define __CRCIRINDEX_H

/*
**      File ID:        @(#): <MID6912 () - 08/17/02, 29.1.1.1>
**
**	File:					MID6912
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:15:53
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file defines a class (CRcirIndex) that implements a
**	circular index.  
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/


class CRcirIndex {
public:
	CRcirIndex() { cur_value = 0; }
	void init(int max_size) { cur_value = 0; size = max_size; }
	inline int incr();
	int value() const { return cur_value; }
	void set(int new_value) { cur_value = new_value; }
	int getSize() const { return size; }

private:
	int cur_value;
	int size;
};

int CRcirIndex::incr() {
	cur_value++;
	if (cur_value >= size)
		cur_value = 0;
	return cur_value;
}

#endif
