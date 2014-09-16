// Description:
//    This file contains an auxillary routine (_inargchk) used by
//    initialization client processes (via the INIT macro) to check
//    lab-mode processing is desired (i.e., without INIT)

#include <stdlib.h>
#include <string.h>

#include "cc/lib/init/INlibinit.hh"
#include "cc/hdr/init/INreturns.hh"


/*
 *	Name:		_inargchk()
 *
 *	Unit:		INIT Subsystem
 *
 *	Description:
 *			This routine verifies consistency of command line 
 *			arguments and the mode of the program. If the program
 *			is running in lab mode, this routine collects run level
 *			and sn level values from command line. The format
 *			of the process started in lab mode should be 
 *			process_name lab sn_lvl run_lvl 
 *
 *			If process is running under INIT control, no arguments
 *			are expected and IN_MSGH_ENV (defined in cc/hdr/INinit.h)
 *			environment variable is set. In lab mode, that variable
 *			must not be set.
 *
 *	Inputs:
 *			Standard UNIX argc, argv[] paramters passed via
 *			the INIT macro.
 *
 *	Outputs:
 *		*sn_lvl_p:	
 *			A value for the sn level to be passed to process
 *			initialization routines when running in lab mode.
 *		*runlvl_p:
 *			If a positive integer argument directly follows
 *			a "lab" argument on the command line, then
 *			*runlvl_p is set to the positive integer value.
 *			Otherwise, *runlvl_p is set to 0.
 *	Calls:
 *			strcmp() - libc function to compare two strings
 *	
 *			atoi() - libc function to convert an ASCII string
 *				 to an integer.
 *	Called By:
 *			Initialization client processes via
 *			the INIT macro.
 *
 */



Void
_inargchk(Short argc,Char *argv[],SN_LVL *snlvl_p,U_char *runlvl_p)
{
	U_short	indx, lvl;
	char * msgh_name;

	/*
	 *  Set default values for recovery and run levels:
	 */
	*runlvl_p = 0;
	*snlvl_p = SN_LV0;

	msgh_name = getenv(IN_MSGH_ENV);
	
	if(msgh_name != 0){
		/* Verify the validity of the process */
		if((strcmp(msgh_name,argv[0]) != 0 ||
			_ingetindx(argv[0]) < 0 || IN_IS_LAB() || argc > 1)){
			INITREQ(SN_LV0,INBADSHMEM,"CORRUPTED PROCESS ENVIRONMENT, MSGH NAME MUST BE THE PROCESS NAME",IN_EXIT);
		}

		return;
	}

	if(!IN_IS_LAB()){
		INITREQ(SN_LV0,INBADSHMEM,"CORRUPTED PROCESS ENVIRONMENT, MSGH NAME MUST BE SPECIFIED",IN_EXIT);
	}

	indx = 1;
	while (argc > indx) {
		if (strcmp(argv[indx],"lab") == 0) {
			if ((argc > ++indx) && ((lvl = (U_short) atoi(argv[indx])) > 0)) {
				if (lvl == 0) {
					*snlvl_p = SN_LV0;
				} else if (lvl == 1) {
					*snlvl_p = SN_LV1;
				} else if (lvl == 2) {
					*snlvl_p = SN_LV2;
				} else if (lvl == 3) {
					*snlvl_p = SN_LV3;
				} else if (lvl == 4) {
					*snlvl_p = SN_LV4;
				} else {
					*snlvl_p = SN_LV5;
				}
				
				if ((argc > ++indx) && ((lvl = (U_short) atoi(argv[indx])) > 0)) {
					*runlvl_p = lvl;
				}
			}
			return;
		}
		indx++;
	}
	
	/* Should not get to this point, unless not running in lab mode.
	** Return with an error message.
	*/

	INITREQ(SN_LV0,INBADSHMEM,"CORRUPTED PROCESS ENVIRONMENT",IN_EXIT);

	return;
}

