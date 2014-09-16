#ifndef __INRUNLVL_H
#define __INRUNLVL_H

//	Description:
//		This file contains the definition of classes used to change
//		the system run level.  The following message classes are
//		defined:
//
//			INrunLvl:  Message broadcast by INIT after it has
//				   increased the system run level.
//
//			INsetRunLvl:  Message sent from the "SET:RUNLVL"
//				   command to INIT requesting that INIT
//				   INIT re-read the "initlist" and,
//				   optionally, increase the system run
//				   level.
//
//			INsetRunLvlAck: Message sent from INIT to the
//				   "SET:RUNLVL" command in response to
//				   a valid "INsetRunLvl" message
//
//			INsetRunLvlFail: Message sent from INIT to the
//				   "SET:RUNLVL" command in response to
//				   an invalid "INsetRunLvl" message
//
//

#include "hdr/GLtypes.h"
#include "cc/hdr/msgh/MHmsgBase.hh"
#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/msgh/MHpriority.hh"

#include "cc/hdr/init/INmtype.hh"

/*
**  "INrunLvl" message class:
**	This class defines the message broadcast to all CC processes when
**	the SN run level is increased via a "SET:RUNLVL" command.  Note that
**	this message will NOT be broadcast when a "SET:RUNLVL" command does
**	not increase the run level (i.e. a "SET:RUNLVL" which simply
**	causes INIT to re-read the "initlist" to start any new permanent
**	process which have been added to it at or below the current run
**	level since INIT last read the "initlist".
**
**	When the system run level is increased, this message is not
**	broadcast until all of the newly started permanent processes
**	have completed their initializations.
*/
class INrunLvl : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type (INrunLvlTyp
	 * defined in "cc/hdr/init/INmtype.H") and cast the message
	 * with this class to access the new run level.
	 */

	/* message-specific data */
	/* run_lvl represents the new SN run level */
	U_char		run_lvl;
};


/*
**  "INsetRunLvl" message class:
**	This class defines the message sent by the "SET:RUNLEVEL" command
**	to INIT when the "initlist" is to be re-read and, optionally,
**	the system run level is to be increased.  This message's type
**	is "INrunLvlTyp" as defined in "cc/hdr/init/INmtype.H".
*/
class INsetRunLvl : public MHmsgBase {
    public:
		INsetRunLvl(U_char rlvl)
			 { run_lvl = rlvl; priType = MHoamPtyp;
			   msgType = INsetRunLvlTyp;};

	inline GLretVal	send(MHqid toQid, MHqid fromQid, Long time);
	inline GLretVal	send(const Char *name, MHqid fromQid, Long time);
	inline GLretVal	send(MHqid fromQid, Long time); /* sends to "INIT"*/

	/* message-specific data */
	/* run_lvl represents the new SN run level */
	U_char		run_lvl;
};

inline GLretVal
INsetRunLvl::send(MHqid toQid, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(toQid, (Char *) this, sizeof(INsetRunLvl),
			   time);
}

inline GLretVal
INsetRunLvl::send(MHqid fromQid, Long time)
{
	return send("INIT", fromQid, time);
}

inline GLretVal
INsetRunLvl::send(const Char *name, MHqid fromQid, Long time)
{
	srcQue = fromQid;
	return MHmsgh.send(name, (Char *) this, sizeof(INsetRunLvl),
			   time);
}

/*
**  "INsetRunLvlAck" message class:
**	This class defines the message INIT send in response to a
**	"INsetRunLvl" message.  Its message type is "INsetRunLvlAckTyp"
**	as defined in "cc/hdr/init/INmtype.H".
*/
class INsetRunLvlAck : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type (INrunLvlTyp
	 * defined in "cc/hdr/init/INmtype.H") and cast the message
	 * with this class to access the new run level.
	 */

	/*
	 * message-specific data:
	 * - "num_procs" is set to the number of processes which were read
	 *   from the initlist and started as a result of this command -
	 *   if "num_procs" is < zero then INIT had a problem reading the
	 *   "initlist"
	 * - "run_lvl" represents the new SN run level
	 */
	Short		num_procs;
	U_char		run_lvl;
};

/*
**  "INsetRunLvlFail" message class:
**	This class defines the message INIT sends in response to an
**	invalid "INsetRunLvl" message.  Its message type is
**	"INsetRunLvlFailTyp" as defined in "cc/hdr/init/INmtype.H".
*/
class INsetRunLvlFail : public MHmsgBase {
    public:
	/*
	 * Since INIT is the only sender for this message, no methods
	 * need to be defined here.  All processes receiving this
	 * message simply need to switch on the message type
	 * (INsetRunLvlFailTyp defined in "cc/hdr/init/INmtype.H")
	 */

	/*
	 * message-specific data:
	 * "ret" contains one of the error returns defined in
	 * "cc/hdr/init/INreturns.H":
	 */
	GLretVal	ret;
	U_char		run_lvl;
};
#endif
