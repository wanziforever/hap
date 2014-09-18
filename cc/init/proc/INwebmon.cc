
//*********************************************************************************
// This file implement WEBMON functionality for the APACHE web server.
// WEBMON is responsible for insuring that APACHE installation is clean
// starting APACHE and monitoring webserver functionality.  That is accomplished
// by a companion process, webtest, that will be started by APACHE when
// invoked by WEBMON.
//
//********************************************************************************/

#include	<stdlib.h>
#include	<memory.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<errno.h>
#include 	<sys/uio.h>
#include	<unistd.h>
#include	<hdr/GLtypes.h>
#include	<hdr/GLreturns.h>
#include	"cc/hdr/msgh/MHinfoExt.hh"
#include	"cc/hdr/init/INkillProc.hh"
#include	"cc/hdr/init/INprcCreat.hh"
#include	"cc/hdr/init/INmtype.hh"
#include	"cc/hdr/cr/CRdebugMsg.hh"

#define	START	1
#define STOP	2
#define CHECK	3

int 
apache(int action)
{
        Char vpath[256];
	Char cpath[256];
	Char spath[256];
	strcpy(vpath, "/cs/sn/init/webctl");
	strcpy(cpath, "/cs/sn/init/webctl checkApache");
	strcpy(spath, "/cs/sn/init/webctl stopApache");

	switch(action){
	case STOP:
		return(system(spath));
	case START:
		return(system(vpath));
	case CHECK:
     {
       MHqid	mhqid;
       if(MHmsgh.attach() != GLsuccess) {
         return(6);
       }
       if(MHmsgh.regName("WEBMON", mhqid, FALSE, FALSE, FALSE, MH_LOCAL, FALSE) != GLsuccess) {
         return(1);
       }
       system(cpath);
       long msgbuf[40];
       GLretVal retval;
       Long msgSz = 160;
       if((retval = MHmsgh.receive(mhqid, (char*)msgbuf, msgSz, 0, 5000L)) != GLsuccess) {
         if(retval == MHtimeOut){
           return(2);
         } else {
           return(0);
         }
       }
       MHmsgBase* msg_p = (MHmsgBase*)msgbuf;
       if(msg_p->msgType != INsetSoftChkTyp){
         return(3);
       } 
       MHmsgh.rmName(mhqid, "WEBMON");
       return(0);
     }
	}
}

int
tomcat(int action)
{
        Char vpath[256];
	Char cpath[256];
#ifdef EES
	//this needs to be a relative path for INIT

/* IBM wyoes 20061111 script fix cc/vendor to cc/vendor_linux */
#ifdef __linux
        strcpy(vpath, "cc/vendor_linux/Apache/LE/tomcat/bin/startup.psp");
#else
        strcpy(vpath, "cc/vendor/Apache/EE/tomcat/bin/startup.psp");
#endif


	//need to find this in the VPATH
        strcpy(cpath, "cc/init/proc/INwebstart");
        if (GLfullpath(cpath,256) != GLsuccess)
        {
                sprintf(cpath,"cc/init/proc/INwebstart" );
        }
        strcat(cpath, " checkTomcat");
#else
        strcpy(vpath, "/vendorpackages/Apache/tomcat/bin/startup.psp");
        strcpy(cpath, "/cs/sn/init/webctl checkTomcat");
#endif

	GLretVal	ret;
	if((ret = MHmsgh.attach()) != GLsuccess){
		return(1);
	}
	switch(action){
	case STOP:
	{
		INkillProc tomcatKill;
		strcpy(tomcatKill.msgh_name, "TOMCAT");
		tomcatKill.bAll = FALSE;
		if ( tomcatKill.send(MHnullQ,0) != GLsuccess) {
			return(1);
		}
		return(0);
	}
	case START:
	{
		INkillProc tomcatKill;
		strcpy(tomcatKill.msgh_name, "TOMCAT");
		tomcatKill.bAll = FALSE;
		if ( tomcatKill.send(MHnullQ,0) != GLsuccess) {
			return(1);
		}

		//wait for Tomcat to exit
		sleep(5);

		INprocCreate tomcatProcess;
		strcpy(tomcatProcess.full_path, vpath);
		strcpy(tomcatProcess.msgh_name, "TOMCAT");
		tomcatProcess.proc_cat = IN_NON_CRITICAL;
		tomcatProcess.inh_restart = FALSE;
		tomcatProcess.priority = (U_char) 20;
		//ainet user for mas, root for 5400
		#ifdef LX
		tomcatProcess.uid = 0;
                #else
		tomcatProcess.uid = 612;
		#endif
		tomcatProcess.run_lvl = (U_char) 85;
		tomcatProcess.rstrt_max = 3;
		tomcatProcess.rstrt_intvl = 750;

		if ( tomcatProcess.send(MHnullQ,0) != GLsuccess) {
			return(2);
		}
		return(0);
	}
	case CHECK:
		ret = system(cpath);
		if(ret != 0){
			CRDEBUG_PRINT(1,("Tomcat check failed ret %d", ret));
			return(1);
		}
		return(0);
	}

	return(0);
}

int
aaa(int action)
{
        Char vpath[256];
	Char cpath[256];
#ifdef EES
	//this needs to be a relative path for INIT
        strcpy(vpath, "cc/cr/osmon/startStopAAAinEE");

	//need to find this in the VPATH
        strcpy(cpath, "cc/init/proc/INwebstart");
        if (GLfullpath(cpath,256) != GLsuccess)
        {
                sprintf(cpath,"cc/init/proc/INwebstart" );
        }
        strcat(cpath, " checkTomcat");
#else
        strcpy(vpath, "/sn/cr/startup.sh");
#endif

	GLretVal	ret;
	if((ret = MHmsgh.attach()) != GLsuccess){
		CRDEBUG_PRINT(1,("AAA failed to attach"));
		return(1);
	}
	switch(action){
	case STOP:
	{
		INkillProc tomcatKill;
		strcpy(tomcatKill.msgh_name, "AAA");
		tomcatKill.bAll = FALSE;
		if ( tomcatKill.send(MHnullQ,0) != GLsuccess) {
			return(1);
		}
		return(0);
	}
	case START:
	{
		INkillProc tomcatKill;
		strcpy(tomcatKill.msgh_name, "AAA");
		tomcatKill.bAll = FALSE;
		if ( tomcatKill.send(MHnullQ,0) != GLsuccess) {
			return(1);
		}

		//wait for Tomcat to exit
		sleep(5);

		INprocCreate tomcatProcess;
		strcpy(tomcatProcess.full_path, vpath);
		strcpy(tomcatProcess.msgh_name, "AAA");
		tomcatProcess.proc_cat = IN_NON_CRITICAL;
		tomcatProcess.inh_restart = FALSE;
		tomcatProcess.priority = (U_char) 20;
		//ainet user
		tomcatProcess.uid = 0;
		tomcatProcess.run_lvl = (U_char) 85;
		tomcatProcess.rstrt_max = 3;
		tomcatProcess.rstrt_intvl = 750;
		tomcatProcess.msg_limit = 1000;
                tomcatProcess.q_size=300000;



		if ( tomcatProcess.send(MHnullQ,0) != GLsuccess) {
			return(2);
		}
		return(0);
	}
	case CHECK:
		//ret = system(cpath);
		//if(ret != 0){
			//CRDEBUG_PRINT(1,("AAA check failed ret %d", ret));
			//return(1);
		//}
		return(0);
	}

	return(0);
}

int
main(int argc, char* argv[])
{
	if(argc != 2){
		return(1);
	}
	int	action;
	if(strcmp(argv[1], "start") == 0){
		action = START;
	} else if(strcmp(argv[1], "stop") == 0){
		action = STOP;
	} else if(strcmp(argv[1], "check") == 0){
		action = CHECK;
	} else {
		return(20);
	}

	if(strcmp(argv[0], "APACHE") == 0){
		return(apache(action)); 
	} else if(strcmp(argv[0], "TOMCATMON") == 0){
		return(tomcat(action));
	} else if(strcmp(argv[0], "AAAMON") == 0){
		return(aaa(action));
	}
	return(19);
}
