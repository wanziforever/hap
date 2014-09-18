#ifndef _CRCIMEM_H
#define _CRCIMEM_H

#define CRNUM_INDICATORS 20
#define CRIND_BUFFSZ 24

struct CRciMem
{
	char		indicator[CRNUM_INDICATORS][CRIND_BUFFSZ];
	int		overload_value;
	int		fill;		// 64 bit alignment
};

#endif
