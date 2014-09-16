
// DESCRIPTION:
// 	This file defines some member functions declared in the
//	class MHname.
//
// OWNER:
//	Shi-Chuan Tu
//
// NOTES:
//

#include "cc/lib/msgh/MHname.hh"

const Char MHmsghName[] = "MSGH";

/*
 * Returns a non-negative integer derived from the MHname object 
 * to be used in hash table. It folds a char string in groups of 4.
 */
Long
MHname::foldKey() const {
    Long fold = 0;
    int i = 0;

    do {
		fold += 2097152 * name[i] + 16384 * name[i+1] + 
				    128 * name[i+2] + name[i+3];
		i += 4;
    } while (i < MHmaxNameLen);

    return fold;
}
