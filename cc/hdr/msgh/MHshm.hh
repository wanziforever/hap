#ifndef _MHSHM_H
#define _MHSHM_H
/*
** File ID:	@(#): <MID41651 () - 08/17/02, 14.1.1.1>
**
** File:		MID41651
** Release:		14.1.1.1
** Date:		08/21/02
** Time:		19:43:42
** Newest applied delta:08/17/02
*/
#include "cc/hdr/msgh/MHrt.hh"
#include "cc/hdr/msgh/MHgd.hh"
#include "cc/hdr/init/INshmkey.hh"

#define MHgdMax		IN_GD_MAX

struct MHshm {
	MHrt	m_rt;
	MHgd	m_gd[MHgdMax];
};

#endif
