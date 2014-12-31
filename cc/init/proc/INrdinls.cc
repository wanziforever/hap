/*
**	File ID: 	@(#): <MID8517 () - 05/12/03, 29.1.1.1>
**
**	File:			MID8517
**	Release:		29.1.1.1
**	Date:			06/18/03
**	Time:			19:19:17
**	Newest applied delta:	05/12/03 16:47:42
**
** DESCRIPTION:
** 	This file contains the routine rdinls() that reads information
**	from the INITLIST and stores it in shared memory.
**
** FUNCTIONS:
**	INrdinls()	- Read the initlist file
**	INgettoken()	- Utility function to parse lines read from
**	INmatch_string()- Find a match for a string in a table of strings
**	INis_numeric()	- Verify that a string is numeric
**	INconvparm()	- Convert a numeric string to valid parameter value.
**	INfindproc()	- Find a process in process table
**	INgetpath()	- Get the full path name for a file
**
** NOTES:
*/

#include <stdlib.h>
#include <malloc.h>
#include <sysent.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <sched.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "hdr/GLtypes.h"
#include "hdr/GLreturns.h"
#include "cc/hdr/init/INproctab.hh"
#include "cc/init/proc/INlocal.hh"

#define	NULLPTR		(char *) 0	/* Pointer to a NULL character.	*/
#define	DIR_DIVIDE	'/'		/* Directory divide character.	*/
#define PERCENTONACTIVELOW      50
#define PERCENTONACTIVEHIGH     70
#define INmaxDebugTimer		10000	/* Max debug timer in ms	*/
#define INminSafeInterval	20	/* Minimum safe interval	*/
#define INminVhostFailover_Time	2	/* Minimum vhost failover timer */
#define INmaxVhostFailover_Time	20	/* Maxium vhost failover timer  */
#define INmaxBoot		99
#define INmaxNetworkTimeout	600
#define INminBoot		1

char	*fptr = (char *) 0;	/* Pointer to INITLIST file read in.	*/
static const char *	err_msg = "REPT INIT ERROR INITLIST ";
static const char *	lerr_msg = "REPT INIT ERROR INITLIST LINE ";

extern char * INbool_val[];
extern char * INrt_val[];
extern char * INparm_names[];
extern char * INcategory_vals[];

GLretVal
INgetNodeList(char nodes[][INmaxNodeName+1], char* token, int line, int maxNodes)
{
	char* 	p = token;
	int	num_nodes = 0;
	int	count;

	while(TRUE){
		if(num_nodes >= maxNodes){
			//CR_PRM(POA_INF, "%s%d TOO MANY NODES, SKIPPED %s", lerr_msg, line, p);
      printf("%s%d TOO MANY NODES, SKIPPED %s", lerr_msg, line, p);
			return(GLsuccess);
		}
		count = 0;
		while(*p != ',' && *p != 0){
			if(count <= INmaxNodeName){
				nodes[num_nodes][count] = *p;
				count++;
			} else {
				//CR_PRM(POA_INF, "%s%d NODE NAME TOO LONG", lerr_msg, line);
        printf("%s%d NODE NAME TOO LONG", lerr_msg, line);
				return(GLfail);
			}
			p++;
		} 
		nodes[num_nodes][count] = 0;
		if(*p == 0){
			return(GLsuccess);
		}
		num_nodes ++;
		p++;
	}
}

GLretVal
INgetResourceList(char* resources, char* token, int line)
{
	char* 	p = token;
	int	num_resources = 0;

	while(TRUE){
		if(num_resources >= INmaxResourceGroups){
			//CR_PRM(POA_INF, "%s%d TOO MANY NODES, SKIPPED %s", lerr_msg, line, p);
      printf("%s%d TOO MANY NODES, SKIPPED %s", lerr_msg, line, p);
			return(GLsuccess);
		}
		while(*p != ',' && *p != 0){
			p++;
		} 
		if(token != p){
			*p = 0;
			resources[num_resources] = atoi(token);
		} else {
			return(GLsuccess);
		}
		if(*p == 0){
			return(GLsuccess);
		}
		num_resources ++;
		p++;
		token = p;
	}
}

/*
 *	Name:	
 *		INrdinls()
 *
 *	Description:
 *		This routine reads in the INITLIST from
 *		a file and stores the information in shared
 *		data structure used by INIT.
 *
 *	Inputs:	
 *		initflg		- true if this is run during init
 *		audflg		- true if called out of audit	
 *
 *	Global Data: 
 *		IN_LDILIST	- Initlist name pulled out of shared memory
 *		IN_LDPTAB	- Process table info. populated while
 *				  reading the initlist
 *		IN_LDPNUM	- Number of processes currently being
 *				  "tracked" by INIT is updated while
 *				  reading initlist
 *
 *	Returns:
 *		If successful, INrdinls() returns the number of new
 *		processes which were read in from the INITLIST.  If
 *		the "initlist" cannot be successfully read, or its
 *		contents are not completely correct,
 *		then this routine returns GLfail.
 *
 *	Calls:	
 *
 *		INgettoken() - Parse the null terminated line returned
 *			     by INgetline() and return a pointer to the
 *			     next token.
 *
 *		malloc()   - Dynamically allocate memory (in bytes)
 *
 *		strchr()   - Find the first occurrence of a character
 *			     in a string.
 *
 *		strrchr()  - Find the last occurrence of a character
 *			     in a string.
 *
 *
 *	Called By:
 *
 *	Side Effects:
 */



Short
INrdinls(Bool initflg, Bool audflg)
{
	struct stat	stbuf;
	int		proc_parms_idx;
	int		parm_idx;
	Bool		parm_updated[IN_PARM_MAX+1]; // Keep track which parameters were updated;
	IN_SYS_PARMS    sys_parms;		// Temporary system parameters		
	IN_PROC_PARMS   proc_parms[IN_SNPRCMX];	// Temporary process parameters 
	char		token[MAX_TOKEN_LEN];
	char *		file_index;		// Keeps track of a postion where
						// search for another token should start.
	int		line = 1;		// Line number in initlist file
	int		j,k;
						   

	//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): entered initflg %d, audflg %d",(int)initflg,(int)audflg));
  printf("INrdinls(): entered initflg %d, audflg %d",(int)initflg,(int)audflg);
	Short proc_cnt = 0;
	CRALARMLVL a_lvl;

	/* Hardcode alarm level to be critical, but leave code in 
	** to make it possible to change to major alarm in the future.
	*/

	a_lvl = POA_CRIT;

	/*
	 *	If the INITLIST file does not exist or is empty, print a
	 *	message to the craft and return an error indication.
	 */

	if ((stat(IN_LDILIST, &stbuf) < 0) || (stbuf.st_size == 0)) {
		//CR_PRM(a_lvl,"%sMISSING OR EMPTY INITLIST %s",err_msg,IN_LDILIST);
		//CR_PRM(POA_INF,"%sMISSING OR EMPTY INITLIST %s",err_msg,IN_LDILIST);
    printf("%sMISSING OR EMPTY INITLIST %s\n",err_msg,IN_LDILIST);
    return(GLfail);
	}

	int fd = open(IN_LDILIST,O_RDONLY);

	if (fd < 0) {
		//CR_PRM(a_lvl,"%sCANNOT OPEN INITLIST %s",err_msg,IN_LDILIST);
		//CR_PRM(POA_INF,"%sCANNOT OPEN INITLIST %s",err_msg,IN_LDILIST);
    printf("%sCANNOT OPEN INITLIST %s\n",err_msg,IN_LDILIST);
    return(GLfail);
	}

	/*
	 *  Check to see if the memory allocated on the last invocation
	 *  of this routine was NOT deallocated and, if so, deallocate
	 *  it.
	 */
	if (fptr != NULLPTR ) {
		free(fptr);
		fptr = NULLPTR;
    //INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): deallocated space for INITLIST from previous invocation"));
    printf("INrdinls(): deallocated space for INITLIST from previous invocation\n");
	}

	/*
	 *	Dynamically get the space needed to read in the INITLIST
	 *	file with one read.  This saves some time because there
	 *	is only one I/O request.
	 */

	if ((fptr = (char *)malloc((stbuf.st_size+1))) == NULLPTR) {
    //INIT_ERROR(("Unable to malloc space to read in INITLIST"));
    printf("Unable to malloc space to read in INITLIST");
		close(fd);
    return(GLfail);
	}

	int nread = read(fd,fptr,stbuf.st_size);

	close(fd);

	if (nread != stbuf.st_size) {
    //CR_PRM(a_lvl,"%sREAD ERROR ON INITLIST %s, ERRNO = %d",err_msg,IN_LDILIST,errno);
    //CR_PRM(POA_INF,"%sREAD ERROR ON INITLIST %s, ERRNO = %d",err_msg,IN_LDILIST,errno);
    printf("%sREAD ERROR ON INITLIST %s, ERRNO = %d\n",err_msg,IN_LDILIST,errno);
		free(fptr);
		fptr = NULLPTR;
	    	return(GLfail);
	}

	fptr[stbuf.st_size] = '\0';

	//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls():\n\tINITLIST file \"%s\" read successfully",IN_LDILIST));
  printf("INrdinls():\n\tINITLIST file \"%s\" read successfully\n",
         IN_LDILIST);

	file_index = fptr;

	for(parm_idx = 0; parm_idx <= IN_PARM_MAX; parm_idx++){
		parm_updated[parm_idx] = FALSE;
	}

	memset(&sys_parms, 0x0, sizeof(sys_parms));
	memset(sys_parms.pset, 0xff, sizeof(sys_parms.pset));
	memset(sys_parms.resource_groups, 0xff, sizeof(sys_parms.resource_groups));
	sys_parms.network_timeout = 10;
	memcpy(sys_parms.vhost, IN_procdata->vhost,  sizeof(sys_parms.vhost));
	sys_parms.vhostfailover_time = 10;

	char * equal_loc;
	IN_PARM_INDEX parameter = IN_PARM_MAX;
	IN_BOOL_VAL bool_val;
	IN_RT_VAL rt_val;
	Bool	process_info = FALSE;
	Bool	first_proc_parm;

	/* Process global system parameters first */
	while(process_info == FALSE && INgettoken(token,file_index,line) != GLfail) {
		/* Find = sign and replace it with string terminator */
		if((equal_loc = strchr(token,'=')) == NULLPTR){
			//CR_PRM(a_lvl,"%s%d NOT A NAME=VALUE ENTRY %s",lerr_msg,line,token);
			//CR_PRM(POA_INF,"%s%d NOT A NAME=VALUE ENTRY %s",lerr_msg,line,token);
      printf("%s%d NOT A NAME=VALUE ENTRY %s\n",lerr_msg,line,token);
			return(GLfail);
		}
		
		/* Separate the token into name-value pair	*/
		*equal_loc = 0;
		equal_loc++;
		if(strlen(equal_loc) == 0){
			//CR_PRM(POA_INF,"%s%d NULL VALUE FOR PARAMETER %s",lerr_msg,line,token);
      printf("%s%d NULL VALUE FOR PARAMETER %s\n",lerr_msg,line,token);
			return(GLfail);
		}

		parameter = (IN_PARM_INDEX)INmatch_string(INparm_names,token,IN_PARM_MAX);
		if(parm_updated[parameter] == TRUE){
			//CR_PRM(POA_INF,"%s%d DUPLICATE PARAMETER %s",lerr_msg,line,token);
      printf("%s%d DUPLICATE PARAMETER %s\n",lerr_msg,line,token);
			return(GLfail);
		}

		//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): token %s, value %s",token,equal_loc));
    //printf("INrdinls(): token %s, value %s\n",token,equal_loc);
		parm_updated[parameter] = TRUE;
		
		/* Parse the processor set first	*/
		if(parameter >= IN_PS000 && parameter <= IN_PS127){
			char  pSet[INmaxProcessors * 8];
			strncpy(pSet, equal_loc, INmaxProcessors * 8);
			char* pNum[INmaxProcessors];
			memset(pNum, 0x0, sizeof(pNum));
			char* cProc = pSet;
			int	nSets = 1;

			if(pSet[0] == 0){
				//CR_PRM(POA_INF,"%s%d INVALID VALUE %s FOR %s",lerr_msg, line, equal_loc, token);
        printf("%s%d INVALID VALUE %s FOR %s\n",lerr_msg, line, equal_loc, token);
				return(GLfail);
			}
			pNum[0] = pSet;
			while(*cProc != 0){
				if(!isdigit(*cProc) && *cProc != ','){
					//CR_PRM(POA_INF,"%s%d INVALID VALUE %s FOR %s",lerr_msg, line, equal_loc, token);
          printf("%s%d INVALID VALUE %s FOR %s\n",
                 lerr_msg, line, equal_loc, token);
					return(GLfail);
				}
				if(*cProc == ','){
					*cProc = 0;
					cProc++;
					if(nSets == INmaxProcessors || !isdigit(*cProc)){
						//CR_PRM(POA_INF,"%s%d INVALID VALUE %s FOR %s",lerr_msg, line, equal_loc, token);
            printf("%s%d INVALID VALUE %s FOR %s\n",
                   lerr_msg, line, equal_loc, token);
						return(GLfail);
					}
					pNum[nSets] = cProc;
					nSets++;
				}
				cProc++;	
			}
			int 	i;
			for(i = 0; i < nSets; i++){
				sys_parms.pset[parameter - IN_PS000][i] = atoi(pNum[i]);
			}
			continue;
		}

		/* Process the parameter */
		switch(parameter){
        	case IN_SYS_RUN_LVL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.sys_run_lvl = INconvparm(equal_loc,token,FALSE,255,line);
			break;
        	case IN_FIRST_RUNLVL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.first_runlvl = INconvparm(equal_loc,token,FALSE,255,line);
			break;
        	case IN_MAX_CARGO:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.maxCargo = atoi(equal_loc);
			break;
        	case IN_MIN_CARGO_SZ:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.minCargoSz = atoi(equal_loc);
			break;
        	case IN_CARGO_TMR:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.cargoTmr = atoi(equal_loc);
			break;
        	case IN_MSGH_PING:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.msgh_ping = INconvparm(equal_loc,token,TRUE,40,line);
			break;
        	case IN_NETWORK_TIMEOUT:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			if((sys_parms.network_timeout = INconvparm(equal_loc,token,TRUE,1,line)) > INmaxNetworkTimeout){
				sys_parms.network_timeout = INconvparm(equal_loc,token,FALSE,INmaxNetworkTimeout,line);
			}
			break;
        	case IN_NUM256:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			// Need to be aligned on 64 bit boundary
			sys_parms.num256 = (INconvparm(equal_loc,token,TRUE,200,line)/8)*8;
			break;
        	case IN_NUM1024:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			// Need to be aligned on 64 bit boundary
			sys_parms.num1024 = (INconvparm(equal_loc,token,TRUE,100,line)/8)*8;
			break;
        	case IN_NUM4096:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			// Need to be aligned on 64 bit boundary
			sys_parms.num4096 = (INconvparm(equal_loc,token,TRUE,50,line)/8)*8;
			break;
        	case IN_NUM16384:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			// Need to be aligned on 64 bit boundary
			sys_parms.num16384 = (INconvparm(equal_loc,token,TRUE,25,line)/8)*8;
			break;
        	case IN_NUM_MSGH_OUTGOING:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			// Need to be aligned on 64 bit boundary
			sys_parms.num_msgh_outgoing = (INconvparm(equal_loc,token,TRUE,512,line)/8)*8;
			break;
        	case IN_NUM_LRG_BUF:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.num_lrg_buf = INconvparm(equal_loc,token,TRUE,6,line);
			break;
        	case IN_ARU_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.aru_timer = INconvparm(equal_loc,token,TRUE,10,line);
			break;
        	case IN_PROCESS_FLAGS:
			sys_parms.process_flags = (U_long) strtol(equal_loc, (char **)NULL, 0);
			break;
        	case IN_INIT_FLAGS:
			sys_parms.init_flags = (U_long) strtol(equal_loc, (char **)NULL, 0);
			IN_trace = sys_parms.init_flags | IN_ALWAYSTR;

			break;
        	case IN_SYS_ERROR_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			sys_parms.sys_error_threshold = INconvparm(equal_loc,token,TRUE,10,line);
			break;
        	case IN_SYS_ERROR_DEC_RATE:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.sys_error_dec_rate = INconvparm(equal_loc,token,TRUE,4,line);
			break;
        	case IN_SYS_INIT_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			INsys_init_threshold = INconvparm(equal_loc,token,TRUE,IN_MIN_INIT_THRESHOLD,line);
			break;
        	case IN_VMEM_MINOR:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			/* Convert to Kbytes from Mbytes */
			INvmem_minor = atol(equal_loc) * 1024;
			break;
        	case IN_VMEM_MAJOR:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			/* Convert to Kbytes from Mbytes */
			INvmem_major = atol(equal_loc) * 1024;
			break;
        	case IN_VMEM_CRITICAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			/* Convert to Kbytes from Mbytes */
			INvmem_critical = atol(equal_loc) * 1024;
			break;
        	case IN_FAILOVER_ALARM:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			switch(atoi(equal_loc)){
			case 0: 
				INfailover_alarm = POA_INF;
				break;
			case 1: 
				INfailover_alarm = POA_TCR;
				break;
			case 2: 
				INfailover_alarm = POA_CRIT;
				break;
			default:
				//CR_PRM(a_lvl,"%s%d VALUE FOR failover_alarm=%s MUST BE 0, 1 or 2, reset to 0",lerr_msg,line,equal_loc);
        printf("%s%d VALUE FOR failover_alarm=%s MUST BE 0, 1 or 2, reset to 0",
               lerr_msg,line,equal_loc);
				INfailover_alarm = POA_INF;
				break;
			}
			break;
		case IN_DEFAULT_RESTART_INTERVAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_restart_interval = INconvparm(equal_loc,token,TRUE,IN_MIN_RESTART,line);
			 break;
		case IN_DEFAULT_SANITY_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_sanity_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_SANITY,line);
			 break;
		case IN_DEFAULT_BREVITY_LOW:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_brevity_low = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_DEFAULT_BREVITY_HIGH:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_brevity_high = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_DEFAULT_BREVITY_INTERVAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_brevity_interval = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_DEFAULT_MSG_LIMIT:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_msg_limit = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_DEFAULT_RESTART_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_restart_threshold = INconvparm(equal_loc,token,TRUE,IN_MIN_RESTART_THRESHOLD,line);
			 break;
		case IN_DEFAULT_INIT_COMPLETE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_init_complete_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_INITCMPL,line);
			 break;
		case IN_DEFAULT_PROCINIT_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_procinit_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			 break;
		case IN_DEFAULT_CREATE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_create_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			 break;
		case IN_DEFAULT_LV3_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_lv3_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			 break;
		case IN_DEFAULT_GLOBAL_QUEUE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_global_queue_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			 break;
		case IN_DEFAULT_Q_SIZE:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_q_size = INconvparm(equal_loc,token,TRUE,MHmsgSz,line);
			 break;
		case IN_DEFAULT_ERROR_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_error_threshold = INconvparm(equal_loc,token,TRUE,IN_MIN_ERROR_THRESHOLD,line);
			 break;
		case IN_DEFAULT_ERROR_DEC_RATE:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_error_dec_rate = INconvparm(equal_loc,token,TRUE,IN_MIN_ERROR_DEC_RATE,line);
			 break;
		case IN_DEFAULT_PRIORITY:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			sys_parms.default_priority = INconvparm(equal_loc,token,FALSE,IN_MAXPRIO-1 ,line);
			 break;
                case IN_PERCENT_LOAD_ON_ACTIVE:
                {    
                        if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
                                return(GLfail);
                        }
                        sys_parms.percent_load_on_active = INconvparm(equal_loc,token,FALSE,PERCENTONACTIVEHIGH ,line);
                        if(sys_parms.percent_load_on_active < PERCENTONACTIVELOW){
                                sys_parms.percent_load_on_active = INconvparm(equal_loc,token,TRUE,PERCENTONACTIVELOW ,line);
                        }
                        /* make sure it is in increments of 4, round to the closest number */
                        int     remainder;

                        remainder = (sys_parms.percent_load_on_active - PERCENTONACTIVELOW)
%4;
                        if(remainder <= 2){
                                sys_parms.percent_load_on_active -= remainder;
                        } else {
                                sys_parms.percent_load_on_active += 1;
                        }
                         break;
                }
		case IN_SHUTDOWN_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			if((sys_parms.shutdown_timer = INconvparm(equal_loc,token,TRUE,10,line)) > 1800){
				sys_parms.shutdown_timer = INconvparm(equal_loc,token,FALSE,1800,line);
			}
			break;
		case IN_MAX_BOOTS:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			if(!INissimplex){
				//CR_PRM(POA_INF,"%s%d max_boots NOT SUPPORTED ON ACTIVE/ACTIVE - IGNORED",lerr_msg,line);
        printf("%s%d max_boots NOT SUPPORTED ON ACTIVE/ACTIVE - IGNORED",
               lerr_msg,line);
				break;
			}
			if((INmax_boots = INconvparm(equal_loc,token,TRUE,INminBoot,line)) > INmaxBoot){
				INmax_boots = INconvparm(equal_loc,token,FALSE,INmaxBoot,line);
			}
			break;
		case IN_DEBUG_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			sys_parms.debug_timer = INconvparm(equal_loc,token,FALSE,INmaxDebugTimer,line);
			break;
		case IN_SAFE_INTERVAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			sys_parms.safe_interval = INconvparm(equal_loc,token,TRUE, INminSafeInterval, line);
			break;
		case IN_VHOST_FAILOVER_TIME:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			sys_parms.vhostfailover_time = INconvparm(equal_loc,token, FALSE, INmaxVhostFailover_Time, line);
			if(sys_parms.vhostfailover_time < INminVhostFailover_Time){
				sys_parms.vhostfailover_time = INconvparm(equal_loc,token, TRUE, INminVhostFailover_Time, line);
			}
			break;
		case IN_CORE_FULL_MINOR:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			sys_parms.core_full_minor = INconvparm(equal_loc,token,FALSE,100,line);
			break;
		case IN_CORE_FULL_MAJOR:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) {
        			return(GLfail);
			}
			sys_parms.core_full_major = INconvparm(equal_loc,token,FALSE,100,line);
			break;
		case IN_ACTIVE_NODES:
			if(INgetNodeList(sys_parms.active_nodes, equal_loc, line, INmaxResourceGroups) == GLfail){
				return(GLfail);
			}
			break;
		case IN_RESOURCE_GROUPS:
			if(INgetResourceList(sys_parms.resource_groups, equal_loc, line) == GLfail){
				return(GLfail);
			}
			break;
		case IN_OAM_LEAD:
			if(INgetNodeList(sys_parms.oam_lead, equal_loc, line, INmaxResourceGroups) == GLfail){
				return(GLfail);
			}
			break;
		case IN_VHOST:
			if(INgetNodeList(sys_parms.vhost, equal_loc, line, INmaxVhosts) == GLfail){
				return(GLfail);
			}
			if(sys_parms.vhost[0][0] == 0 || sys_parms.vhost[1][0] == 0){
				//CR_PRM(POA_INF,"%s%d VALUE FOR vhost=%s MUST HAVE TWO HOSTS",lerr_msg,line,equal_loc);
        printf("%s%d VALUE FOR vhost=%s MUST HAVE TWO HOSTS",
               lerr_msg,line,equal_loc);
				return(GLfail);
			}
			break;
		case IN_OAM_OTHER:
			if(INgetNodeList(sys_parms.oam_other, equal_loc, line, INmaxResourceGroups) == GLfail){
				return(GLfail);
			}
			break;
        	case IN_BUFFERED:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				sys_parms.buffered = NO;
				break;
			case IN_YES:
				sys_parms.buffered = YES;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%s%d VALUE FOR buffered=%s MUST BE YES OR NO",lerr_msg,line,equal_loc);
        printf("%s%d VALUE FOR buffered=%s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc);
				return(GLfail);
			}
			break;
        	case IN_SYS_CRERROR_INH:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				sys_parms.sys_crerror_inh = NO;
				break;
			case IN_YES:
				sys_parms.sys_crerror_inh = YES;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%s%d VALUE FOR sys_crerror_inh=%s MUST BE YES OR NO",lerr_msg,line,equal_loc);
        printf("%s%d VALUE FOR sys_crerror_inh=%s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc);
				return(GLfail);
			}
			break;
		case IN_MSGH_NAME:
			process_info = TRUE;
			first_proc_parm = TRUE;
			break;
		case IN_PARM_MAX:
			//CR_PRM(POA_INF,"%s%d INVALID PARAMETER %s",lerr_msg,line,token);
      printf("%s%d INVALID PARAMETER %s\n",lerr_msg,line,token);
			return(GLfail);
		default:
			//CR_PRM(POA_INF,"%s%d PROCESS PARAMETER %s APPEARING BEFORE msgh_name WAS DEFINED",lerr_msg,line,token);
      printf("%s%d PROCESS PARAMETER %s APPEARING BEFORE msgh_name WAS DEFINED\n",
             lerr_msg,line,token);
			return(GLfail);
		}
	}
	
	/* Insure that all system level parameters are initialized */
	Bool sys_all_set = TRUE;
	for(parm_idx = IN_SYS_RUN_LVL; parm_idx < IN_PS000; parm_idx++){
		if(parm_updated[parm_idx] == FALSE){
      //CR_PRM(POA_INF,"%s%d MISSING SYSTEM PARAMETER %s",lerr_msg,line,INparm_names[parm_idx]);
      printf("%s%d MISSING SYSTEM PARAMETER %s\n",
             lerr_msg,line,INparm_names[parm_idx]);
      sys_all_set = FALSE;
		}
	}
	
	/* Check virtual memory thresholds to insure consistent values:
	** i.e. vmem_minor >= vmem_major >= vmem_critical.  If that is not
	** true then reset the tresholds and print the PRM indicating that.
	*/
	if(INvmem_major < INvmem_critical){
		//CR_PRM(POA_INF,"%s vmem_major=%d VALUE TOO SMALL - RESET TO %d",err_msg,INvmem_major,INvmem_critical);
    printf("%s vmem_major=%d VALUE TOO SMALL - RESET TO %d\n",
           err_msg,INvmem_major,INvmem_critical);
		INvmem_major = INvmem_critical;
	}

	if(INvmem_minor < INvmem_major){
		//CR_PRM(POA_INF,"%s vmem_minor=%d VALUE TOO SMALL - RESET TO %d",err_msg,INvmem_minor,INvmem_major);
    printf("%s vmem_minor=%d VALUE TOO SMALL - RESET TO %d",
           err_msg,INvmem_minor,INvmem_major);
		INvmem_minor = INvmem_major;
	}

	if(sys_all_set == FALSE){
		return(GLfail);
	}

	// At this point system defaults are known so that proc_parms can be
	// initialized now.

	for(proc_parms_idx = 0; proc_parms_idx < IN_SNPRCMX; proc_parms_idx++){
		INdef_parm_init(&proc_parms[proc_parms_idx],&sys_parms);
	}

	proc_parms_idx = -1;

	/* Now process per process parameters */
	while (process_info == TRUE){
		// Get a new token only if this is a second process 
		// We already have a token when we get out of the system parameter
		// processing loop.

		if(first_proc_parm == FALSE){
			if(INgettoken(token,file_index,line) != GLfail){
				/* Find = sign and replace it with string terminator */
				if((equal_loc = strchr(token,'=')) == NULLPTR){
					//CR_PRM(POA_INF,"%s%d NOT A NAME=VALUE ENTRY %s",lerr_msg,line,token);
          printf("%s%d NOT A NAME=VALUE ENTRY %s\n",lerr_msg,line,token);
					return(GLfail);
				}

				/* Separate the token into name-value pair	*/
				*equal_loc = 0;
				equal_loc++;
				if(strlen(equal_loc) == 0){
					//CR_PRM(POA_INF,"%s%d NULL VALUE FOR PARAMETER %s",lerr_msg,line,token);
          printf("%s%d NULL VALUE FOR PARAMETER %s\n",lerr_msg,line,token);
					return(GLfail);
				}

				parameter = (IN_PARM_INDEX)INmatch_string(INparm_names,token,IN_PARM_MAX);
				if(parm_updated[parameter] == TRUE){
					//CR_PRM(POA_INF,"%s%d DUPLICATE PARAMETER %s",lerr_msg,line,token);
          printf("%s%d DUPLICATE PARAMETER %s\n",lerr_msg,line,token);
					return(GLfail);
				}

				parm_updated[parameter] = TRUE;
				//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): token %s, value %s",token,equal_loc));
        //printf("INrdinls(): token %s, value %s\n",token,equal_loc);
			} else {
				process_info = FALSE;
				// This will force required argument checking  for 
				// the last process entry in the initlist file
				parameter = IN_MSGH_NAME;
			}
		}

		switch(parameter){
        	case IN_MSGH_NAME:
			if(first_proc_parm == FALSE){
				/* Verify that previous process had all the required parameters */
				for(parm_idx = IN_MSGH_NAME + 1; parm_idx <= IN_USER_ID; parm_idx++){
					if(parm_updated[parm_idx] == FALSE){
						//CR_PRM(POA_INF,"%s%d REQUIRED PARAMETER %s NOT DEFINED FOR %s",lerr_msg,line,INparm_names[parm_idx],proc_parms[proc_parms_idx].msgh_name);
            printf("%s%d REQUIRED PARAMETER %s NOT DEFINED FOR %s\n",
                   lerr_msg,line,INparm_names[parm_idx],
                   proc_parms[proc_parms_idx].msgh_name);
						return(GLfail);
					}
				}
			}

			first_proc_parm = FALSE;

			if(process_info == FALSE){
				// Done with all file entries
				break;
			}
	
			/* Reinitialize parm_updated information */
			for(parm_idx = IN_MSGH_NAME; parm_idx < IN_PARM_MAX; parm_idx++){
				parm_updated[parm_idx] = FALSE;
			}

			proc_parms_idx++;
			if(proc_parms_idx >= IN_SNPRCMX){
				//CR_PRM(POA_INF,"%s%d TOO MANY PROCESSES IN %s INITLIST",lerr_msg,line,IN_LDILIST);
        printf("%s%d TOO MANY PROCESSES IN %s INITLIST\n",
               lerr_msg,line,IN_LDILIST);
				return(GLfail);
			}
			
			/* Verify that there are no duplicate process names */
			for(j = 0; j < proc_parms_idx; j++){
				if(strcmp(equal_loc,proc_parms[j].msgh_name) == 0){
					//CR_PRM(POA_INF,"%s%d DUPLICATE MSGH_NAME %s",lerr_msg,line,equal_loc);
          printf("%s%d DUPLICATE MSGH_NAME %s\n",
                 lerr_msg,line,equal_loc);
					return(GLfail);
				}
			}
			/* Verify that process name is not INIT name */
			if (strcmp(equal_loc, IN_MSGHQNM) == 0) {
				//CR_PRM(POA_INF,"%s%d MSGH_NAME %s IS RESERVED",lerr_msg,line, equal_loc);
        printf("%s%d MSGH_NAME %s IS RESERVED\n",lerr_msg,line, equal_loc);
				return(GLfail);
			}
			
			if(strlen(equal_loc) >= IN_NAMEMX){ 
				//CR_PRM(POA_INF,"%s%d LENGTH OF PROCESS NAME %s GREATER THAN MAXIMUM LENGTH %d",lerr_msg,line,equal_loc,IN_NAMEMX);
        printf("%s%d LENGTH OF PROCESS NAME %s GREATER THAN MAXIMUM LENGTH %d\n",
               lerr_msg,line,equal_loc,IN_NAMEMX);
				return(GLfail);
			}
			strcpy(proc_parms[proc_parms_idx].msgh_name,equal_loc);
			break;
        	case IN_RUN_LVL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail); 
			}
			proc_parms[proc_parms_idx].run_lvl = atoi(equal_loc);
			if((atoi(equal_loc) > 255) ||
				(proc_parms[proc_parms_idx].run_lvl == 0)){
				//CR_PRM(POA_INF,"%s%d run_level %s INVALID FOR %s",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d run_level %s INVALID FOR %s\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_MSGH_QID:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail); 
			}
			proc_parms[proc_parms_idx].msgh_qid = atoi(equal_loc);
			if(atoi(equal_loc) > MHmaxPermProc){
				//CR_PRM(POA_INF,"%s%d msgh_qid %s INVALID FOR %s",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d msgh_qid %s INVALID FOR %s\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			/* Check for unique value, duplicated msgh_qids are not allowed */
			Short	msgh_qid;
		 	msgh_qid = proc_parms[proc_parms_idx].msgh_qid;

			for(k = 0; k < proc_parms_idx; k++){
				if(msgh_qid == proc_parms[k].msgh_qid){
					//CR_PRM(POA_INF,"%s%d msgh_qid %s IS NOT UNIQUE FOR %s",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
          printf("%s%d msgh_qid %s IS NOT UNIQUE FOR %s\n",
                 lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
					return(GLfail);
				}
			}
			break;
        	case IN_PATH:
			if(strlen(equal_loc) >= IN_PATHNMMX){ 
				//CR_PRM(POA_INF,"%s%d LENGTH OF PATH %s GREATER THAN MAXIMUM LENGTH %d",lerr_msg,line,equal_loc,IN_PATHNMMX);
        printf("%s%d LENGTH OF PATH %s GREATER THAN MAXIMUM LENGTH %d\n",
               lerr_msg,line,equal_loc,IN_PATHNMMX);
				return(GLfail);
			}
#ifdef CC
			if(*equal_loc != '/'){ 
				//CR_PRM(POA_INF,"%s%d PATH %s MUST START AT ROOT",lerr_msg,line,equal_loc);
        printf("%s%d PATH %s MUST START AT ROOT\n",lerr_msg,line,equal_loc);
				return(GLfail);
			}
#endif

			/*
		 	* Verify that the path references an existing executable
		 	* file only if official path not specfied, otherwise
			* just make sure that directory exists.
		 	*/
			if(proc_parms[proc_parms_idx].ofc_path[0] == 0){
				if (INgetpath(equal_loc, TRUE) != GLsuccess) {
#ifdef OLD_SU
					/* If SU is in progress, file may have been added
					** and therefore it may not be present
					*/
					if(INsupresent == TRUE){
						if((j = INsudata_find(equal_loc)) < SU_MAX_OBJ){
							char tmp_path[IN_PATHNMMX];
							strcpy(tmp_path,INsudata[j].obj_path);
							strcat(tmp_path,".new");
							if(INsudata[j].new_obj == FALSE ||
								INgetpath(tmp_path,TRUE) != GLsuccess){
								//CR_PRM(POA_INF,"%s%d CANNOT FIND EXECUTABLE FILE %s",lerr_msg,line, equal_loc);
                printf("%s%d CANNOT FIND EXECUTABLE FILE %s\n",
                       lerr_msg,line, equal_loc);
								return(GLfail);
							}
						}
					} else {
#endif
						//CR_PRM(POA_INF,"%s%d CANNOT FIND EXECUTABLE FILE %s",lerr_msg,line, equal_loc);
            printf("%s%d CANNOT FIND EXECUTABLE FILE %s\n",
                   lerr_msg,line, equal_loc);
						return(GLfail);
#ifdef OLD_SU
					}
#endif
				}
			} else if(INgetpath(equal_loc, TRUE, TRUE) != GLsuccess){
				//CR_PRM(POA_INF,"%s%d CANNOT FIND DIRECTORY %s",lerr_msg,line, equal_loc);
        printf("%s%d CANNOT FIND DIRECTORY %s",lerr_msg,line, equal_loc);
				return(GLfail);
			}
			strcpy(proc_parms[proc_parms_idx].path,equal_loc);
			break;
    case IN_OFC_PATH:
      //CR_PRM(a_lvl,"%s%d OFC_PATH PARAMETER NOT SUPPORTED ON CONTROL SERVER",lerr_msg,line);
      printf("%s%d OFC_PATH PARAMETER NOT SUPPORTED ON CONTROL SERVER\n",
             lerr_msg,line);
      return(GLfail);
      if(strlen(equal_loc) >= IN_OPATHNMMX){ 
        //CR_PRM(POA_INF,"%s%d LENGTH OF OFC_PATH %s GREATER THAN MAXIMUM LENGTH %d",lerr_msg,line,equal_loc,IN_OPATHNMMX);
        printf("%s%d LENGTH OF OFC_PATH %s GREATER THAN MAXIMUM LENGTH %d\n",
               lerr_msg,line,equal_loc,IN_OPATHNMMX);
        return(GLfail);
      }

			/*
		 	* Verify that the path references an existing executable
		 	* file:
		 	*/
			if (audflg == TRUE && INgetpath(equal_loc, TRUE) != GLsuccess) {
				/* If SU is in progress, file may have been added
				** and therefore it may not be present
				*/
				if(INsupresent == TRUE){
					/* Provide SU validity checking for added files when SU 
					** strategy is implemented
					*/
				} else {
					//CR_PRM(POA_INF,"%s%d CANNOT FIND EXECUTABLE FILE %s",lerr_msg,line, equal_loc);
          printf("%s%d CANNOT FIND EXECUTABLE FILE %s\n",
                 lerr_msg,line, equal_loc);
					return(GLfail);
				}
			}
			strcpy(proc_parms[proc_parms_idx].ofc_path,equal_loc);
			break;
        	case IN_EXT_PATH:
			if(strlen(equal_loc) >= IN_EPATHNMMX){ 
				//CR_PRM(POA_INF,"%s%d LENGTH OF EXT_PATH %s GREATER THAN MAXIMUM LENGTH %d",lerr_msg,line,equal_loc,IN_EPATHNMMX);
        printf("%s%d LENGTH OF EXT_PATH %s GREATER THAN MAXIMUM LENGTH %d\n",
               lerr_msg,line,equal_loc,IN_EPATHNMMX);
				return(GLfail);
			}

			/*
		 	* Verify that the path references an existing executable
		 	* file:
		 	*/
			if (audflg == TRUE && INgetpath(equal_loc, TRUE) != GLsuccess) {
				//CR_PRM(POA_INF,"%s%d CANNOT FIND EXECUTABLE FILE %s",lerr_msg,line, equal_loc);
        printf("%s%d CANNOT FIND EXECUTABLE FILE %s\n",
               lerr_msg,line, equal_loc);
				return(GLfail);
			}
			strcpy(proc_parms[proc_parms_idx].ext_path,equal_loc);
			break;
        	case IN_USER_ID:
			if(!isdigit(equal_loc[0])){
				// try to lookup user id from password file
				struct passwd* pswd;
				if((pswd = getpwnam(equal_loc)) == NULL){
					//CR_PRM(POA_INF,"%s%d CANNOT FIND USER ID %s ERRNO %d",lerr_msg,line, equal_loc, errno);
          printf("%s%d CANNOT FIND USER ID %s ERRNO %d\n",
                 lerr_msg,line, equal_loc, errno);
					return(GLfail);
				}
				proc_parms[proc_parms_idx].user_id = pswd->pw_uid;
				break;
			} else if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].user_id = atoi(equal_loc);
			break;
        	case IN_GROUP_ID:
			if(!isdigit(equal_loc[0])){
				// try to lookup group id from password file
				struct passwd* pswd;
				if((pswd = getpwnam(equal_loc)) == NULL){
					//CR_PRM(POA_INF,"%s%d CANNOT FIND GROUP ID %s ERRNO %d",lerr_msg,line, equal_loc, errno);
          printf("%s%d CANNOT FIND GROUP ID %s ERRNO %d\n",
                 lerr_msg,line, equal_loc, errno);
					return(GLfail);
				}
				proc_parms[proc_parms_idx].group_id = pswd->pw_uid;
				break;
			} else if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].group_id = atoi(equal_loc);
			break;
        	case IN_PRIORITY:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			} 
			if(proc_parms[proc_parms_idx].isRT != IN_NOTRT){
				proc_parms[proc_parms_idx].priority = INconvparm(equal_loc,token,FALSE,INmaxPrioRT - 1, line);
			} else {
				proc_parms[proc_parms_idx].priority = INconvparm(equal_loc,token,FALSE,IN_MAXPRIO-1,line);
			}

			break;
        	case IN_SANITY_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			if((proc_parms[proc_parms_idx].sanity_timer = atoi(equal_loc)) != 0){
				proc_parms[proc_parms_idx].sanity_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_SANITY,line);
			}
			break;
		case IN_BREVITY_LOW:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].brevity_low = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_BREVITY_HIGH:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].brevity_high = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_BREVITY_INTERVAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].brevity_interval = INconvparm(equal_loc,token,TRUE,0,line);
			break;
		case IN_MSG_LIMIT:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE) { 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].msg_limit = INconvparm(equal_loc,token,TRUE,0,line);
			break;
        	case IN_RESTART_INTERVAL:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){ 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].restart_interval = INconvparm(equal_loc,token,TRUE,IN_MIN_RESTART,line);
			break;
        	case IN_RESTART_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].restart_threshold = INconvparm(equal_loc,token,TRUE,IN_MIN_RESTART_THRESHOLD,line);
			break;
        	case IN_INHIBIT_RESTART:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].inhibit_restart = IN_ALWRESTART;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].inhibit_restart = IN_INHRESTART;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%s%d VALUE FOR inhibit_restart=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR inhibit_restart=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_PROCESS_CATEGORY:
			if((proc_parms[proc_parms_idx].proc_category = (IN_PROC_CATEGORY)INmatch_string(INcategory_vals,equal_loc,IN_MAX_CAT)) == IN_MAX_CAT){
				//CR_PRM(POA_INF,"%s%d INVALID process_category=%s FOR %s",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d INVALID process_category=%s FOR %s\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_INIT_COMPLETE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){ 
				return(GLfail);
			}
			proc_parms[proc_parms_idx].init_complete_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_INITCMPL,line);
			break;
        	case IN_PROCINIT_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].procinit_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			break;
        	case IN_CREATE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].create_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			break;
        	case IN_LV3_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].lv3_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			break;
        	case IN_GLOBAL_QUEUE_TIMER:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].global_queue_timer = INconvparm(equal_loc,token,TRUE,IN_MIN_PROCCMPL,line);
			break;
        	case IN_Q_SIZE:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].q_size = INconvparm(equal_loc,token,TRUE,MHmsgSz,line);
			break;
        	case IN_PS:
		{
			int	ps;
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			ps = atoi(equal_loc);
			if(ps < 0 || ps >=INmaxPsets || 
				(sys_parms.pset[ps][0] == -1 && IN_LDPSET[ps] == -1)){
				//CR_PRM(POA_INF,"%s%d INVALID PROCESSOR SET=%s FOR %s",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d INVALID PROCESSOR SET=%s FOR %s\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			proc_parms[proc_parms_idx].ps = ps;
			break;
		}
        	case IN_ERROR_THRESHOLD:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].error_threshold = INconvparm(equal_loc,token,TRUE,IN_MIN_ERROR_THRESHOLD,line);
			break;
        	case IN_ERROR_DEC_RATE:
			if(INis_numeric(equal_loc,token,a_lvl,line) == FALSE){
				return(GLfail);
			}
			proc_parms[proc_parms_idx].error_dec_rate = INconvparm(equal_loc,token,TRUE,IN_MIN_ERROR_DEC_RATE,line);
			break;
        	case IN_CRERROR_INH:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].crerror_inh = NO;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].crerror_inh = YES;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%s%d VALUE FOR crerror_inh=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR crerror_inh=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_ON_ACTIVE:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].on_active = NO;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].on_active = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%s%d VALUE FOR on_active=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR on_active=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_OAMLEADONLY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].oamleadonly = NO;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].oamleadonly = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%s%d VALUE FOR oamleadonly=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR oamleadonly=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_ACTIVE_VHOST_ONLY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].active_vhost_only = NO;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].active_vhost_only = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%s%d VALUE FOR active_vhost_only=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR active_vhost_only=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_THIRD_PARTY:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].third_party = NO;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].third_party = YES;
				break;
			default:
				//CR_PRM(POA_INF,"%s%d VALUE FOR third_party=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR third_party=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_RT:
			rt_val = (IN_RT_VAL)INmatch_string(INrt_val,equal_loc,IN_MAX_RT);
			switch(bool_val){
			case IN_NOTRT:
			case IN_RR:
			case IN_FIFO:
				proc_parms[proc_parms_idx].isRT = rt_val;
				break;
			default:
				//CR_PRM(POA_INF,"%s%d VALUE FOR rt=%s FOR %s MUST BE NO, RR or FIFO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR rt=%s FOR %s MUST BE NO, RR or FIFO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
        	case IN_INHIBIT_SOFTCHK:
			bool_val = (IN_BOOL_VAL)INmatch_string(INbool_val,equal_loc,IN_MAX_BOOL);
			switch(bool_val){
			case IN_NO:
				proc_parms[proc_parms_idx].inh_softchk = IN_ALWSOFTCHK;
				break;
			case IN_YES:
				proc_parms[proc_parms_idx].inh_softchk = IN_INHSOFTCHK;
				break;
			case IN_MAX_BOOL:
				//CR_PRM(POA_INF,"%s%d VALUE FOR inhibit_softchk=%s FOR %s MUST BE YES OR NO",lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
        printf("%s%d VALUE FOR inhibit_softchk=%s FOR %s MUST BE YES OR NO\n",
               lerr_msg,line,equal_loc,proc_parms[proc_parms_idx].msgh_name);
				return(GLfail);
			}
			break;
    case IN_PARM_MAX:
      //CR_PRM(POA_INF,"%s%d INVALID PARAMETER %s FOR %s",lerr_msg,line,token,proc_parms[proc_parms_idx].msgh_name);
      printf("%s%d INVALID PARAMETER %s FOR %s\n",
             lerr_msg,line,token,proc_parms[proc_parms_idx].msgh_name);
			return(GLfail);
		default:
			//CR_PRM(POA_INF,"%s%d SYSTEM PARAMETER %s FOUND WHILE PROCESSING PROCESS %s ENTRIES",lerr_msg,line,token,proc_parms[proc_parms_idx].msgh_name);
      printf("%s%d SYSTEM PARAMETER %s FOUND WHILE PROCESSING PROCESS %s ENTRIES\n",
             lerr_msg,line,token,proc_parms[proc_parms_idx].msgh_name);
			return(GLfail);
		}

	};

	
	// If run in audit mode return at this point
	if(audflg == TRUE){
		free(fptr);
		fptr = NULLPTR;
		//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): returning success in audit mode"));
    printf("INrdinls(): returning success in audit mode\n");
		return(GLsuccess);
	}

	U_short pt_indx;
	int max_procs = proc_parms_idx;

	proc_cnt = 0;

	// Count how many processes there are now
	for(j = 0; j < IN_SNPRCMX; j++){
		if(IN_VALIDPROC(j)){
			proc_cnt++;
		}
	}

	/* Verify that there is enough room in the process table
	** and that the run level of an existing process did not change.
	*/

	/* Do not change run levels, unless full system start occurred */
	if(initflg == TRUE && (IN_LDSTATE.sn_lvl == SN_LV4 || IN_LDSTATE.sn_lvl == SN_LV3 ||
				IN_LDSTATE.sn_lvl == SN_LV5)){ 
		IN_LDSTATE.run_lvl = sys_parms.first_runlvl;
		IN_LDSTATE.first_runlvl = sys_parms.first_runlvl;
		IN_LDSTATE.final_runlvl = sys_parms.sys_run_lvl;
		IN_procdata->maxCargo = sys_parms.maxCargo;
		IN_procdata->minCargoSz = sys_parms.minCargoSz;
		IN_procdata->buffered = sys_parms.buffered;
		IN_procdata->num256 = sys_parms.num256;
		IN_procdata->num1024 = sys_parms.num1024;
		IN_procdata->num4096 = sys_parms.num4096;
		IN_procdata->num16384 = sys_parms.num16384;
		IN_procdata->num_msgh_outgoing = sys_parms.num_msgh_outgoing;
		if(IN_LDSTATE.run_lvl > IN_LDSTATE.final_runlvl){
			IN_LDSTATE.run_lvl = IN_LDSTATE.final_runlvl;
		}
	}

	for(proc_parms_idx = 0; proc_parms_idx <= max_procs; proc_parms_idx++) {

		/*
		 *  Check if this process already exists, and if it does, 
		 *  reuse it entry.
		 */
		if((pt_indx = INfindproc(proc_parms[proc_parms_idx].msgh_name)) >= IN_SNPRCMX){
			/*
			*  Process entry was not found.
		 	*  Check to see if the process's run level qualifies it
		 	*  to be placed in the process table:
		 	*/
			if (proc_parms[proc_parms_idx].run_lvl > IN_LDSTATE.run_lvl) {
				/*
				 *  This process should not be started at the
				 *  current run level.  Skip over it and go
				 *  on.
				 */
				//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): run level %d lower than proc's %d in INITLIST for process %s",IN_LDSTATE.run_lvl,proc_parms[proc_parms_idx].run_lvl,proc_parms[proc_parms_idx].msgh_name));
        printf("INrdinls(): run level %d lower than proc's %d in INITLIST for process %s\n",
               IN_LDSTATE.run_lvl,proc_parms[proc_parms_idx].run_lvl,
               proc_parms[proc_parms_idx].msgh_name);
				continue;
	    		}
			/* Check to see if we are going active or lead and do not
			** bringup processes only running on active.
			*/
			if(proc_parms[proc_parms_idx].on_active == FALSE &&
				IN_LDCURSTATE == S_ACT){
				//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s not running on active",proc_parms[proc_parms_idx].msgh_name));
        printf("INrdinls(): process %s not running on active\n",
               proc_parms[proc_parms_idx].msgh_name);
				continue;
			}
			/* If we are on oamlead load oamlead only processes
			** otherwise skip
			*/
			if(proc_parms[proc_parms_idx].oamleadonly == TRUE && INevent.getOAMLead() != INmyPeerHostId){
				//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s only running on oamlead",proc_parms[proc_parms_idx].msgh_name));
        printf("INrdinls(): process %s only running on oamlead\n",
               proc_parms[proc_parms_idx].msgh_name);
				continue;
			}
			/* If we are on active vhost load on_active_vhost only processes
			** otherwise skip
			*/
			if(proc_parms[proc_parms_idx].active_vhost_only == TRUE && !INevent.isVhostActive()){
				//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s only running on active vhost",proc_parms[proc_parms_idx].msgh_name));
        printf("INrdinls(): process %s only running on active vhost\n",
               proc_parms[proc_parms_idx].msgh_name);
				continue;
			}
			proc_cnt++;
			if (proc_cnt >= IN_SNPRCMX) {
				/*  No more available process table entries... */
				//CR_PRM(POA_INF,"%sEXCEEDED MAXIMUM NUMBER OF PROCESSES %d IN PROCESS TABLE",err_msg, pt_indx);
        printf("%sEXCEEDED MAXIMUM NUMBER OF PROCESSES %d IN PROCESS TABLE\n",
               err_msg, pt_indx);
				return(GLfail);
			}
		} else {

			/* Existing process entry found, verify that its run level is 
			** not being changed.
			*/
			if(IN_LDPTAB[pt_indx].run_lvl != proc_parms[proc_parms_idx].run_lvl){
				//CR_PRM(POA_INF,"%srun_level CHANGE OF EXISTING PROCESS %s FROM %d TO %d CAN ONLY OCCUR DURING SYSTEM INIT",err_msg, IN_LDPTAB[pt_indx].proctag,IN_LDPTAB[pt_indx].run_lvl,proc_parms[proc_parms_idx].run_lvl);
        printf("%srun_level CHANGE OF EXISTING PROCESS %s FROM %d TO %d CAN ONLY OCCUR DURING SYSTEM INIT\n",
               err_msg, IN_LDPTAB[pt_indx].proctag,IN_LDPTAB[pt_indx].run_lvl,proc_parms[proc_parms_idx].run_lvl);
				return(GLfail);
			}
			/* Verify that msgh_qid level is not being changed.
			*/
			if(IN_LDPTAB[pt_indx].msgh_qid != proc_parms[proc_parms_idx].msgh_qid){
				//CR_PRM(POA_INF,"%smsgh_qid CHANGE OF EXISTING PROCESS %s FROM %d TO %d CAN ONLY OCCUR DURING SYSTEM INIT",err_msg, IN_LDPTAB[pt_indx].proctag,IN_LDPTAB[pt_indx].msgh_qid,proc_parms[proc_parms_idx].msgh_qid);
        printf("%smsgh_qid CHANGE OF EXISTING PROCESS %s FROM %d TO %d CAN ONLY OCCUR DURING SYSTEM INIT\n",
               err_msg, IN_LDPTAB[pt_indx].proctag,IN_LDPTAB[pt_indx].msgh_qid,
               proc_parms[proc_parms_idx].msgh_qid);
				return(GLfail);
			}

			/* Only permament process info can be changed through initlist */
			if(IN_LDPTAB[pt_indx].permstate != IN_PERMPROC){
				//CR_PRM(POA_INF,"%sINITLIST CANNOT CHANGE PROCESS PARAMETERS FOR AN EXISTING NON-PERMAMENT PROCESS %s",err_msg, IN_LDPTAB[pt_indx].proctag);
        printf("%sINITLIST CANNOT CHANGE PROCESS PARAMETERS FOR AN EXISTING NON-PERMAMENT PROCESS %s\n",
               err_msg, IN_LDPTAB[pt_indx].proctag);
				return(GLfail);
			}
		}
	}

	/*
	 *  Update system level information that has changed  
	 */
	IN_LDSTATE.crerror_inh = sys_parms.sys_crerror_inh;
	IN_LDARUINT = sys_parms.aru_timer;
	IN_LDE_THRESHOLD = sys_parms.sys_error_threshold;
	IN_LDE_DECRATE = sys_parms.sys_error_dec_rate;
	IN_procdata->default_restart_interval = sys_parms.default_restart_interval;
	IN_procdata->default_restart_threshold = sys_parms.default_restart_threshold;
	IN_procdata->default_sanity_timer = sys_parms.default_sanity_timer;
	IN_procdata->default_init_complete_timer = sys_parms.default_init_complete_timer;
	IN_procdata->default_procinit_timer = sys_parms.default_procinit_timer;
	IN_procdata->default_lv3_timer = sys_parms.default_lv3_timer;
	IN_procdata->default_global_queue_timer = sys_parms.default_global_queue_timer;
	IN_procdata->default_create_timer = sys_parms.default_create_timer;
	IN_procdata->default_q_size = sys_parms.default_q_size;
	IN_procdata->default_error_threshold = sys_parms.default_error_threshold;
	IN_procdata->default_error_dec_rate = sys_parms.default_error_dec_rate;
	IN_procdata->process_flags = sys_parms.process_flags;
	IN_procdata->default_brevity_low = sys_parms.default_brevity_low;
	IN_procdata->default_brevity_high = sys_parms.default_brevity_high;
	IN_procdata->default_brevity_interval = sys_parms.default_brevity_interval;
	IN_procdata->default_msg_limit = sys_parms.default_msg_limit;
        IN_LDPERCENTLOADONACTIVE = sys_parms.percent_load_on_active;
	IN_procdata->shutdown_timer = sys_parms.shutdown_timer;
	IN_procdata->debug_timer = sys_parms.debug_timer;
	IN_procdata->safe_interval = sys_parms.safe_interval;
	IN_procdata->core_full_minor = sys_parms.core_full_minor;
	IN_procdata->core_full_major = sys_parms.core_full_major;
	IN_procdata->cargoTmr = sys_parms.cargoTmr;
	IN_procdata->msgh_ping = sys_parms.msgh_ping;
	IN_procdata->network_timeout = sys_parms.network_timeout;
	IN_procdata->num_lrg_buf = sys_parms.num_lrg_buf;

	/* Create processor sets.  Processor sets can only be added, not deleted,
	** what this code will do is check for processor set existence and if it
	** is already defined, the entry from initlist will be ignored.
	*/

	int	psNum;
	for(psNum = 0; psNum < INmaxPsets; psNum++){
		if(IN_LDPSET[psNum] == -1 && sys_parms.pset[psNum][0] != -1){
#ifdef __sun
			char	plist[INmaxProcessors * 8];
			plist[0] = 0;
			int	i;
			for(i = 0; i < INmaxProcessors && sys_parms.pset[psNum][i] != -1; i++){
				sprintf(&plist[strlen(plist)],"%d,", sys_parms.pset[psNum][i]);
			}
			plist[strlen(plist) - 1] = 0;
			/* New set, create it	*/
			//CR_PRM(POA_INF, "REPT INIT CREATING PROCESSOR SET %d - %s", psNum, plist);
      printf("REPT INIT CREATING PROCESSOR SET %d - %s\n",
             psNum, plist);
			if(pset_create(&IN_LDPSET[psNum]) == 0){
				psetid_t 	opset;
				for(i = 0; i < INmaxProcessors && sys_parms.pset[psNum][i] != -1; i++){
					opset = -1;
					if(pset_assign(IN_LDPSET[psNum], sys_parms.pset[psNum][i], &opset) != 0){
						//CR_PRM(POA_MIN, "REPT INIT FAILED TO ADD PROCESSOR %d to PROCESSOR SET %d, ERRNO %d",
            //       sys_parms.pset[psNum][i], psNum, errno);
            printf("REPT INIT FAILED TO ADD PROCESSOR %d to PROCESSOR SET %d, ERRNO %d",
                   sys_parms.pset[psNum][i], psNum, errno);
					}
					if(opset != -1){
						int 	j;
						for(j = 0; j < INmaxPsets; j++){
							if(IN_LDPSET[j] == opset){
								CR_PRM(POA_INF, "REPT INIT PROCESSOR %d WAS REMOVED FROM PROCESSOR SET %d",
                       sys_parms.pset[psNum][i], j);
                printf("REPT INIT PROCESSOR %d WAS REMOVED FROM PROCESSOR SET %d\n",
                       sys_parms.pset[psNum][i], j);
								break;
							}
						}
					}
				}
			} else {
				//CR_PRM(POA_MIN, "REPT INIT FAILED TO CREATE PROCESSOR SET %d, ERRNO %d", psNum, errno);
        printf("REPT INIT FAILED TO CREATE PROCESSOR SET %d, ERRNO %d\n", psNum, errno);
				IN_LDPSET[psNum] = -1;
			}
#endif
		}
	}
	// Update resource groups and oam_cluster information.  
	memcpy(IN_procdata->resource_groups, sys_parms.resource_groups, sizeof(IN_procdata->resource_groups));
	memcpy(IN_procdata->oam_lead, sys_parms.oam_lead, sizeof(IN_procdata->oam_lead));
	memcpy(IN_procdata->oam_other, sys_parms.oam_other, sizeof(IN_procdata->oam_other));
	memcpy(IN_procdata->active_nodes, sys_parms.active_nodes, sizeof(IN_procdata->active_nodes));
	memcpy(IN_procdata->vhost, sys_parms.vhost, sizeof(IN_procdata->vhost));
	IN_procdata->vhostfailover_time = sys_parms.vhostfailover_time;

	U_short tmp_indx;
	proc_cnt = 0;

	for( proc_parms_idx = 0; proc_parms_idx <= max_procs; proc_parms_idx++) {

		tmp_indx = IN_SNPRCMX;
		/*
		 *  Check if this process already exists, and if it does, 
		 *  reuse it entry and update its information.
		 */
		for (pt_indx = 0; pt_indx < IN_SNPRCMX; pt_indx++) {
			if (IN_VALIDPROC(pt_indx)) { 
			     	if(strcmp(IN_LDPTAB[pt_indx].proctag,proc_parms[proc_parms_idx].msgh_name) == 0){
					break;
				}
			} else {
				if(tmp_indx == IN_SNPRCMX){
					tmp_indx = pt_indx;
				}
			}
		}

		/* If process entry was not found, reset pt_indx to first available entry*/
		if(pt_indx == IN_SNPRCMX){
			pt_indx = tmp_indx;
		}


		/*
		 *  Check to see if the process's run level qualifies it
		 *  to be placed in the process table:
		 */
		if (proc_parms[proc_parms_idx].run_lvl > IN_LDSTATE.run_lvl) {
			/*
			 *  This process should not be started at the
			 *  current run level.  Skip over it and go
			 *  on.
			 */
			continue;
	    	}

		/* Check to see if we are going active or lead and do not
		** bringup processes only running on active.
		*/
		if(proc_parms[proc_parms_idx].on_active == FALSE &&
			IN_LDCURSTATE == S_ACT){
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s not running on active",proc_parms[proc_parms_idx].msgh_name));
      printf("INrdinls(): process %s not running on active\n",
             proc_parms[proc_parms_idx].msgh_name);
			continue;
		}
		if(proc_parms[proc_parms_idx].oamleadonly == TRUE && INevent.getOAMLead() != INmyPeerHostId){
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s only running on oamlead",proc_parms[proc_parms_idx].msgh_name));
      printf("INrdinls(): process %s only running on oamlead\n",
             proc_parms[proc_parms_idx].msgh_name);
			continue;
		}

		/* If we are on active vhost load on_active_vhost only processes
		** otherwise skip
		*/
		if(proc_parms[proc_parms_idx].active_vhost_only == TRUE && !INevent.isVhostActive()){
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): process %s only running on active vhost",proc_parms[proc_parms_idx].msgh_name));
      printf("INrdinls(): process %s only running on active vhost\n",
             proc_parms[proc_parms_idx].msgh_name);
			continue;
		}
		strcpy(IN_LDPTAB[pt_indx].proctag,proc_parms[proc_parms_idx].msgh_name);

		/* Make last character a null for audit purposes: */
		IN_LDPTAB[pt_indx].proctag[IN_NAMEMX-1] = 0;

		strcpy(IN_LDPTAB[pt_indx].pathname,proc_parms[proc_parms_idx].path);
		strcpy(IN_LDPTAB[pt_indx].ofc_pathname,proc_parms[proc_parms_idx].ofc_path);
		strcpy(IN_LDPTAB[pt_indx].ext_pathname,proc_parms[proc_parms_idx].ext_path);

		/* Make last character a null for audit purposes: */
		IN_LDPTAB[pt_indx].pathname[IN_PATHNMMX-1] = 0;
	
		IN_LDPTAB[pt_indx].priority = proc_parms[proc_parms_idx].priority;
		IN_LDPTAB[pt_indx].uid = proc_parms[proc_parms_idx].user_id;
		IN_LDPTAB[pt_indx].group_id = proc_parms[proc_parms_idx].group_id;
		IN_LDPTAB[pt_indx].peg_intvl = proc_parms[proc_parms_idx].sanity_timer;
		IN_LDPTAB[pt_indx].rstrt_intvl = proc_parms[proc_parms_idx].restart_interval;
		IN_LDPTAB[pt_indx].rstrt_max = proc_parms[proc_parms_idx].restart_threshold;
		IN_LDPTAB[pt_indx].crerror_inh = proc_parms[proc_parms_idx].crerror_inh;
		IN_LDPTAB[pt_indx].proc_category = proc_parms[proc_parms_idx].proc_category;
		IN_LDPTAB[pt_indx].error_threshold = proc_parms[proc_parms_idx].error_threshold;
		IN_LDPTAB[pt_indx].error_dec_rate = proc_parms[proc_parms_idx].error_dec_rate;
		IN_LDPTAB[pt_indx].init_complete_timer = proc_parms[proc_parms_idx].init_complete_timer;
		IN_LDPTAB[pt_indx].procinit_timer = proc_parms[proc_parms_idx].procinit_timer;
		IN_LDPTAB[pt_indx].lv3_timer = proc_parms[proc_parms_idx].lv3_timer;
		IN_LDPTAB[pt_indx].global_queue_timer = proc_parms[proc_parms_idx].global_queue_timer;
		IN_LDPTAB[pt_indx].on_active = proc_parms[proc_parms_idx].on_active;
		IN_LDPTAB[pt_indx].oamleadonly = proc_parms[proc_parms_idx].oamleadonly;
		IN_LDPTAB[pt_indx].active_vhost_only = proc_parms[proc_parms_idx].active_vhost_only;
		IN_LDPTAB[pt_indx].create_timer = proc_parms[proc_parms_idx].create_timer;
		IN_LDPTAB[pt_indx].q_size = proc_parms[proc_parms_idx].q_size;
		IN_LDPTAB[pt_indx].brevity_low = proc_parms[proc_parms_idx].brevity_low;
		IN_LDPTAB[pt_indx].brevity_high = proc_parms[proc_parms_idx].brevity_high;
		IN_LDPTAB[pt_indx].brevity_interval = proc_parms[proc_parms_idx].brevity_interval;
		IN_LDPTAB[pt_indx].msg_limit = proc_parms[proc_parms_idx].msg_limit;
		IN_LDPTAB[pt_indx].isRT = proc_parms[proc_parms_idx].isRT;
		IN_LDPTAB[pt_indx].ps = proc_parms[proc_parms_idx].ps;

		/*
		 *	NOTE -- a process entry is only valid
		 *	if the syncstep is not set to IN_MAXSTEP.
		 */
		if(IN_INVPROC(pt_indx)){
			/* The following code is only executed for processes not already running.*/
			IN_LDPTAB[pt_indx].run_lvl = proc_parms[proc_parms_idx].run_lvl;
			IN_LDPTAB[pt_indx].msgh_qid = proc_parms[proc_parms_idx].msgh_qid;
			IN_LDPTAB[pt_indx].third_party = proc_parms[proc_parms_idx].third_party;
			IN_SDPTAB[pt_indx].procstate = IN_NOEXIST;
			IN_LDPTAB[pt_indx].syncstep = IN_SYSINIT;
			IN_LDPTAB[pt_indx].startstate = proc_parms[proc_parms_idx].inhibit_restart;
			IN_LDPTAB[pt_indx].softchk = proc_parms[proc_parms_idx].inh_softchk;
			if(IN_LDSTATE.sn_lvl != SN_NOINIT && IN_LDSTATE.initstate != IN_CUINTVL){
				IN_LDPTAB[pt_indx].sn_lvl = IN_LDSTATE.sn_lvl;
			} else if(INevent.onLeadCC()){
				IN_LDPTAB[pt_indx].sn_lvl = SN_LV5;
			} else {
				IN_LDPTAB[pt_indx].sn_lvl = SN_LV4;
			}
			/*
		 	* Check for MSGH:
		 	*/
			if (strcmp(IN_LDPTAB[pt_indx].proctag,"MSGH") == 0) {
				IN_LDMSGHINDX = pt_indx;

			} 

#ifdef OLD_SU
			/* If software update is in progress and this process is in
			** the software update package, set it up so the correct 
			** version of the process is started. Also, since this
			** process is not yet running, it should have all the 
			** associated files moved in place immediately.
			*/
			if(INsupresent && IN_LDBKOUT == FALSE && initflg == FALSE){
				if(INsudata_find(IN_LDPTAB[pt_indx].pathname) < SU_MAX_OBJ){
					SN_LVL sn_lvl;
					if(INmvsufiles(pt_indx,sn_lvl,INSU_APPLY) == GLsuccess){
						IN_LDPTAB[pt_indx].updstate = UPD_PRESTART;
					} else {
						/* Backout the whole SU */
						INautobkout(FALSE,FALSE);
					}
				}
			}
#endif

			/* Increment the number of additional processes included */
			proc_cnt++;
		} else if(IN_SDPTAB[pt_indx].procstep == IN_STEADY){
			/* VALID process, and state is IN_STEADY so update
			** sanity pegging data.
			*/
			IN_LDPTAB[pt_indx].sent_missedsan = FALSE;
			IN_LDPTAB[pt_indx].time_missedsan = 0;
		}

		IN_LDPTAB[pt_indx].permstate = IN_PERMPROC;

	}

	free(fptr);
	fptr = NULLPTR;

	//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INrdinls(): INITLIST table initialization completed"));
  printf("INrdinls(): INITLIST table initialization completed");
	return(proc_cnt);
}



/*
 *	Name:	
 *			INgettoken()
 *
 *	Description:
 *			This routine copies a token into the
 *			passed  token string pointer. 
 *			It uses	the file_index passed to it as the string 
 *			to search for the first token.  Internally 
 *
 *	Inputs:	
 *			token - pointer to the string into which token
 *			should be copied.
 *			file_index - index into the file parsed so far
 *			line	- keep track of line numbers in parsed file
 *
 *	Returns:
 *			GLfail if no more tokens can be found
 *			GLsuccess otherwise
 *
 *	Calls:	
 *			none
 *
 *	Called By:
 *
 *	Side Effects:
 */



GLretVal
INgettoken(char *token, char* &file_index,int &line)
{
	int count = 0;

	/*
	 *	Skip over all blanks, tabs and commented lines.
	 */

	while ((*file_index == ' ') || (*file_index == '\t') || 
	       (*file_index == '\n') || (*file_index == '#')) {
		/* If end of line, increment line number */
		if(*file_index == '\n') line++;

		/* If # skip to the end of line */
		if(*file_index == '#'){
			while(*file_index != '\n' && *file_index != '\0'){
				file_index++;
			}
			
			if(*file_index == '\n') line++;

			if(*file_index == '\0'){
				return(GLfail);
			}
		}

		file_index++;
	}

	/* Check for EOF */
	if(*file_index == '\0'){
		return(GLfail);
	}


	while ((count < MAX_TOKEN_LEN-1) && (*file_index != ' ') && (*file_index != '\t') &&
		(*file_index != '\n') && (*file_index != '\0') && (*file_index != '#')) {
		*token = *file_index;
		token++;
		file_index++;
		count++;
	}

	*token = '\0';
	return(GLsuccess);
}

/*
** NAME:
**	INgetpath()
**
** DESCRIPTION:
**	This function checks to see if a file exists corresponding to
**	the path passed to it.
**
** INPUTS:
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/

GLretVal
INgetpath(char *path, Bool exec_flg, Bool check_dir)
{
	struct stat	stbuf;
	GLretVal ret;
	char		tmp_path[IN_PATHNMMX];

#ifdef EES
	char	*envpath, *root;
	char 	vpath[IN_PATHNMMX*8];  /* Max path size- arbitrarily large*/
	char	newpath[IN_PATHNMMX];

	if (path[0] != DIR_DIVIDE) {
		/*
		 * Check to see if a VPATH is available:
		 */
		envpath = getenv("VPATH");

		if (envpath == (char *const) 0) {
			//CR_PRM(POA_INF,"INgetpath(): can't access %s\n\trelative path w/no VPATH defined", path);
      printf("INgetpath(): can't access %s\n\trelative path w/no VPATH defined\n",
             path);
			return(GLfail);
		}

		int strsize = strlen(envpath);
		if ((strsize == 0) || (strsize >= (IN_PATHNMMX*8))) {
			//CR_PRM(POA_INF,"INgetpath(): VPATH size %d is invalid\n\tcan't access %s\n\trelative path invalid VPATH defined", strsize, path);
      printf("INgetpath(): VPATH size %d is invalid\n\tcan't access %s\n\trelative path invalid VPATH defined\n",
             strsize, path);
			return(GLfail);
		}

		strcpy(vpath, envpath);	/* Make a local copy of VPATH */

		root = strtok(vpath, ":");
		while (root != (char *)0)  {
			if ((strlen(root) + strlen(path)) >= (IN_PATHNMMX+1)) {
				//CR_PRM(POA_INF,"INgetpath(): path name too long, skipping over it\n\t\"%s/%s\"",root, path);
        printf("INgetpath(): path name too long, skipping over it\n\t\"%s/%s\"\n",
               root, path)
				continue;
			}

			strcpy(newpath, root);
			strcat(newpath, "/");
			strcat(newpath, path);
			
			ret = stat(newpath, &stbuf);
			if (ret >= 0) {
				/*
				 * We've found a file in the view path...
				 */
				strcpy(path, newpath);
				goto NXT;
			}
			root = strtok((char *const)0, ":");
		}
		//CR_PRM(POA_INF,"INgetpath(): could not find executable along VPATH for:\n\t%s", path);
    printf("INgetpath(): could not find executable along VPATH for:\n\t%s\n",
           path);
		return(GLfail);
	}

#endif
	if(check_dir == TRUE){
		/* Verify only the directory, not the executable */
		int slen = strlen(path);
		strcpy(tmp_path, path);
		int	i;
		for(i = slen - 1;  i > 0 && tmp_path[i] != DIR_DIVIDE ; i--);
		if(i > 0){
			tmp_path[i] = 0;
		} else {
			//CR_PRM(POA_INF,"INgetpath(): invalid path %s", path);
      printf("INgetpath(): invalid path %s\n", path);
			return(GLfail);
		}
		ret = stat(tmp_path, &stbuf);
		if (ret < 0) {
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): can't access %s\n\t\"stat()\" returned errno %d", tmp_path, errno));
      printf("INgetpath(): can't access %s\n\t\"stat()\" returned errno %d", tmp_path, errno);
			return(GLfail);
		}
		if(stbuf.st_mode & S_IFDIR) { 
			return(GLsuccess);
		} else {
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): %s not a directory, file %s", tmp_path, path));
      printf("INgetpath(): %s not a directory, file %s\n",
             tmp_path, path);
			return(GLfail);
		}
	}

	ret = stat(path, &stbuf);
	if (ret < 0) {
		//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): can't access %s\n\t\"stat()\" returned errno %d", path, errno));
    printf("INgetpath(): can't access %s\n\t\"stat()\" returned errno %d\n",
           path, errno);
		return(GLfail);
	}

	if (stbuf.st_size == 0) {
		/*
		 * Zero length file...skip this one!
		 */
		//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): %s is zero length!", path));
    printf("INgetpath(): %s is zero length!\n", path);
		return(GLfail);
	}
		
	if (exec_flg != TRUE) {
		if (!(stbuf.st_mode & S_IROTH)) {
			/*
			 * Not readable to others, skip this entry:
			 */
			//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): %s not readable by others", path));
      printf("INgetpath(): %s not readable by others", path);
			return(GLfail);
		}
		return(GLsuccess);
	}

	if (!(stbuf.st_mode & S_IXOTH)) {
		/*
		 * Not executable to others, skip this entry:
		 */
		//INIT_DEBUG((IN_DEBUG | IN_RDINTR),(POA_INF,"INgetpath(): %s not executable to others", path));
    printf("INgetpath(): %s not executable to others\n", path);
		return(GLfail);
	}
	return(GLsuccess);
}

/*
 *	Name:	
 *			INmatch_string()
 *
 *	Description:
 *			This routine matches a string to one of the strings
 *			in the passed table of strings.
 *	Inputs:	
 *			strings - table of strings to match against.
 *			name - pointer to the string containing parameter name.
 *			max_index - maximum number of entries in the table.
 *
 *	Returns:
 *			max_index if the name did not match anything
 *			otherwise the matching postion in strings[].
 *
 *	Calls:	
 *
 *	Called By:
 *
 *	Side Effects:
 */

Short
INmatch_string(char * strings[], char * name, Short max_index)
{
int	i;	
	
	for(i = 0; i < max_index; i++){
		if(strcmp(name,strings[i]) == 0){
			break;
		}
	}

	return(i);
}

/*
 *	Name:	
 *			INis_numeric()
 *
 *	Description:
 *			This routine takes a string and checks if
 *			it consists of positive numberic values.  It also limits the
 * 			length of the numeric to 5 digits.
 *	Inputs:	
 *			str_ptr - pointer to the string that is supposed to be numeric.
 *			parm_str - pointer to the parameter name string.
 *
 *	Returns:
 *			TRUE if string contains only numeric values 
 *			FALSE otherwise. NULL string is considered non-numeric.
 *
 *	Calls:	
 *
 *	Called By:
 *
 */

Bool
INis_numeric(char * str_ptr, char * parm_str, CRALARMLVL a_lvl,int line)
{
	Bool is_numeric = TRUE;
	char * p = str_ptr;

	if(*str_ptr == '\0'){
		is_numeric = FALSE;
	}

	int count = 0;

	while(*p != '\0'){
		if(!isdigit(*p) || ++count > 7){
			is_numeric = FALSE;
			break;
		}
		p++;
	}
	
	if(is_numeric != TRUE){
		//CR_PRM(POA_INF,"%s%d PARAMETER %s VALUE MUST BE A WHOLE NUMBER",lerr_msg,line,parm_str,str_ptr);
    printf("%s%d PARAMETER %s VALUE MUST BE A WHOLE NUMBER\n",
           lerr_msg,line,parm_str,str_ptr);
		return(FALSE);
	}

	return(TRUE);
}

/*
 *	Name:	
 *			INconvparm()
 *
 *	Description:
 *			This routine takes a numeric string, converts it to
 *			integer and checks it against a limit (high or low).
 *			If the comparison fails, limit value is returned.
 *	Inputs:	
 *			str_ptr - pointer to the string that is supposed to be numeric.
 *			parm_str - pointer to the parameter name string.
 *			low	- TRUE if less then comparison should be made.
 *			limit	- limit comparison
 *			line	- initlist line number of the error
 *
 *	Returns:
 *			TRUE if string contains only numeric values 
 *			FALSE otherwise. NULL string is considered non-numeric.
 *
 *	Calls:	
 *
 *	Called By:
 *
 */

int
INconvparm(char * str_ptr, char * parm_str, Bool low, int limit, int line)
{
	int intvalue = atoi(str_ptr);

	if(low == TRUE){
		if(intvalue < limit){
			//CR_PRM(POA_INF,"%s%d %s=%s VALUE TOO SMALL - RESET TO %d",lerr_msg,line,parm_str,str_ptr,limit);
      printf("%s%d %s=%s VALUE TOO SMALL - RESET TO %d\n",
             lerr_msg,line,parm_str,str_ptr,limit);
			return(limit);
		}
	} else {
		if(intvalue > limit){
			//CR_PRM(POA_INF,"%s%d %s=%s VALUE TOO LARGE - RESET TO %d",lerr_msg,line,parm_str,str_ptr,limit);
      printf("%s%d %s=%s VALUE TOO LARGE - RESET TO %d\n",
             lerr_msg,line,parm_str,str_ptr,limit);
			return(limit);
		}
	}

	return(intvalue);
}

/*
 *	Name:	
 *			INfindproc()
 *
 *	Description:
 *			This routine finds a process with a given MSGH name
 *			in the process table.
 *	Inputs:	
 *			msgh_name - msgh name of the process
 *
 *	Returns:
 *			process index if found
 *			IN_SNPRCMX if not found
 *
 *	Calls:	
 *
 *	Called By:
 *
 */
int
INfindproc(const char * msgh_name)
{
	
	int	pt_indx;
	for (pt_indx = 0; pt_indx < IN_SNPRCMX; pt_indx++) {
		if (IN_VALIDPROC(pt_indx) &&
		    strcmp(IN_LDPTAB[pt_indx].proctag,msgh_name) == 0){
				break;
		}
	}

	return(pt_indx);
}
