#ifndef __ININITSCN_H
#define __ININITSCN_H

// DESCRIPTION:
//	This header contains the definition of the "INinitSCN",
//	"INinitSCNAck" and "INinitSCNFail" messages.  It is expected
//	that the "INIT:SCN" CEP will generate an INinitSCN message
//	and send it to INIT to cause either a single process or a
//	system-wide initialization.  INIT will respond to the INinitSCN
//	message either with a INinitSCNAck or INinitSCNFail message.
//	
//
// NOTES:
//

#include <string.h>

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"

#include "cc/hdr/init/INinit.hh"
#include "cc/hdr/init/INproctab.hh"
#include "cc/hdr/init/INmtype.hh"

//#include "cc/hdr/cr/CRomInfo.hh"

/*
**  "INinitSCN" message class:
**	This class defines the message processes should send to INIT when
**	they wish to request an initialization.  For single process
**	initializations (recovery levels SN_LV0 and SN_LV1) "msgh_name"
**	must be the MSGH queue name associated with a process currently
**	under INIT's control.
**
**	For system-wide initializations (recovery levels SN_LV2, SN_LV3, or
**	SN_LV4) "msgh_name" not used.  Receipt of a valid INinitSCN
**	message will result in INIT's returning an INinitSCNAck message
**	to the sender before starting the requested initialization action.
**
**	The message type for this message is "INinitSCNTyp" as defined in
**	"cc/hdr/init/mtype.h".
**
*/
class INinitSCN : public MHmsgBase {
public:
  INinitSCN()
     { priType = MHoamPtyp;
       msgType = INinitSCNTyp;
       useCepOmInfo = NO; 
       ucl = NO; };
	inline	INinitSCN(SN_LVL req_sn_lvl, Char *name, 
                    const CRomInfo *omInfo = 0, Bool ucl = NO);

	/*
	 * system-wide init. requests don't require a MSGH name:
	 */
	inline	INinitSCN(SN_LVL req_sn_lvl, const CRomInfo *omInfo = 0,
                    Bool ucl = NO);

	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(const Char *name, MHqid fromQid, Long time);
	/*
	 * Note that this method does not require the destination queue
	 * name because this message is always sent TO INIT:
	 */
	inline GLretVal	send(MHqid fromQid, Long time);

	/*
	 *  Data - For single-process initializations "msgh_name" must
	 *	   be set to the MSGH queue name of a process under
	 * 	   INIT's control.
	 */
	SN_LVL		sn_lvl;
	Char 		msgh_name[IN_NAMEMX];
	CRomInfo	cepOmInfo;
	Bool		useCepOmInfo;
	Bool		ucl;
};

inline
INinitSCN::INinitSCN(SN_LVL req_sn_lvl, const CRomInfo *omInfo, Bool ucl_flg)
{
	msgh_name[0] = 0;
	sn_lvl = req_sn_lvl;
	priType = MHoamPtyp;
	msgType = INinitSCNTyp;
	ucl = ucl_flg;
	if (omInfo != 0)
	{
		cepOmInfo = *omInfo;
		useCepOmInfo = YES;
	}
	else
	{
		useCepOmInfo = NO;
	}
}

inline
INinitSCN::INinitSCN(SN_LVL req_sn_lvl, Char *name, const CRomInfo *omInfo, 
                     Bool ucl_flg)
{
	Short len = strlen(name);
	if (len >= IN_NAMEMX) {
		strncpy(msgh_name, name, (IN_NAMEMX-1));
		msgh_name[IN_NAMEMX-1] = (Char)0;
	}
	else {
		strcpy(msgh_name, name);
	}
	sn_lvl = req_sn_lvl;
	priType = MHoamPtyp;
	msgType = INinitSCNTyp;
	ucl = ucl_flg;
	if (omInfo != 0)
	{
		cepOmInfo = *omInfo;
		useCepOmInfo = YES;
	}
	else
	{
		useCepOmInfo = NO;
	}
}

inline GLretVal
INinitSCN::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INinitSCN),
                     time);
}

inline GLretVal
INinitSCN::send(MHqid fromQid, Long time)
{
	return send("INIT", fromQid, time);
}

inline GLretVal
INinitSCN::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INinitSCN),
                     time);
}


/*
**  "INinitSCNAck"
**	This class defines the message INIT will return in response to
**	a "INinitSCN" message before INIT initiate the requested
**	initialization.  Its message type is
**	"INinitSCNAckTyp" as defined in "cc/hdr/init/INmtype.H".
*/
class INinitSCNAck : public MHmsgBase {
public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on its message type.
	 * Also, if the requested initialization was system-wide
	 * then "msgh_name" will be null.
	 */
	SN_LVL	sn_lvl;
	Char msgh_name[IN_NAMEMX];
};


/*
**  "INinitSCNFail" message class:
**	This class defines the message INIT returns in response to a
**	"INinitSCN" message when INIT is not able grant the original
**	initialization request.  The following possible failure codes
**	(defined in "cc/hdr/init/INreturns.H") may be included in
**	this message:
**
**		INNOPROC   - The process name (associated with an SN_LV0
**			     or SN_LV1 initialization level) is not
**			     registered with INIT.
**		INNOTTEMP  - The process is not a temporary process
**		INVINITSTATE - The system was in an initialization interval
**			       when INIT received the INinitSCN message.
**		INVSNLVL   - The sn_lvl included in the INinitSCN message
**			     was not SN_LV0, SN_LV1, SN_LV2, SN_LV3, or
**			     SN_LV4.
**		INBLOCKED   - A critical process init was attempted without UCL
**			      option
**
**
*/
class INinitSCNFail : public MHmsgBase {
public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * and cast the message with this class to access the data
	 * members.
	 */

	/* message-specific data */
	GLretVal	ret;	/* error return - 			*/
  /* 	see "cc/hdr/init/INreturns.H"	*/
	SN_LVL		sn_lvl;
	Char		msgh_name[IN_NAMEMX];
};


#endif
