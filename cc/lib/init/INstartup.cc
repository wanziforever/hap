
//
// 	This file contains the "main()" routine defined within the
//	INIT library. This is a C file so that global constructors
// 	can be called under INIT's control.
//
// FUNCTIONS:
//	main()
//
// NOTES:
//


/*
** NAME:
**	main()
**
** DESCRIPTION:
**	This is the definition of "main()" for all processes which
**	include the INIT library.  This function performs high level
**	shared memory setup, calls global constructors and then calls
**	main processing function.
**
** INPUTS:
**	argc	- Count of the number of command line parameters
**		  passed in *argv[]
**	argv	- Command line arguments
**
** RETURNS:
**	Never
**
** CALLS:
**	INmain_init() - initialize shared memory pointers and attach to
**	INIT's shared memory.
**	INmain() - main processing function
**	_main()	 - global constructors
**
** CALLED BY:
**	UNIX - this is the entry point for the process.
**
** SIDE EFFECTS:
**	This function must never return, it must always exit, since this is
**	the only way to insure that the global desctructors are called.
*/

#include "cc/hdr/cr/CRprocname.h"

extern void INmain_init();
extern void INmain();
extern void _main();	/* C++ created function initializing global constructors */

int
main(argc,argv)
short	argc;
char *	argv[];
{
	/* set up CRprocname[] */
	strcpy( CRprocname, argv[0] );
	/* Setup shared memory 		*/
	INmain_init(argc,argv);
	/* Execute C++ global constructors 	*/
	_main();
	/* Main processing		*/
	INmain(argc,argv);
	/* INmain should never return 	*/
	exit(0);
}
