#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc/hdr/msgh/MHinfoExt.hh"
#include "cc/hdr/cr/CRprocname.hh"
#include "cc/hdr/cr/CRprmMsg.hh"


int 
main(int argc, char* argv[])
{
	if(argc < 4) {
		return(128);
	}

	strncpy(CRprocname, argv[2], MHmaxNameLen);
	
	MHmsgh.attach();
	
	CRALARMLVL alvl = POA_INF;
	
	if(strcmp(argv[1], "CRIT") == 0){
		alvl = POA_CRIT;
	} else if (strcmp(argv[1], "MAJOR") == 0){
		alvl = POA_MAJ;
	} else if(strcmp(argv[1], "MINOR") == 0){
		alvl = POA_MIN;
	}

	CR_PRM(alvl, "REPT INIT %s %s", argv[2], argv[3]);

return(0);
}
