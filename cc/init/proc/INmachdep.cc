#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#include <sys/user.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

#include <fcntl.h>
#include <time.h>

#ifdef LX
#include <dirent.h>
#endif
 
#include "cc/hdr/init/INinit.hh"
#include "cc/init/proc/INlocal.hh"


/*
** NAME:
**	INchkcore()
**
** DESCRIPTION:
**	This function checks to see if a core file has been created in the
**	directory passed to it.  If a core file does exist, it moves it
**	to make way for any future core files which may be created.
**
** INPUTS:
**	cdir	- directory under "INexecdir" which is to be checked
**
** RETURNS:
**
** CALLS:
**
** CALLED BY:
**
** SIDE EFFECTS:
*/
Void
INchkcore(const Char *cdir)
{
	Char corefname[IN_NAMEMX+IN_PATHNMMX+5];
	struct stat	stbuf;

	if (INroot == TRUE) {
		/*
		 * We only manipulate core files when INIT is running as
		 * root
		 */
		strcpy(corefname, INexecdir);
		strcat(corefname, "/");
		strcat(corefname, cdir);
	
#ifdef LX
		struct dirent*	dp;
		DIR*		dirp;
		if((dirp = opendir(corefname)) == NULL){
			//CRERROR("Failed to open core directory %s, errno %d", corefname, errno);
      printf("Failed to open core directory %s, errno %d", corefname, errno);
		}

		char fullName[300];
		char coreName[300];
		sprintf(coreName,"%s/core",corefname);
		while(dirp != NULL && (dp = readdir(dirp)) != NULL){
			sprintf(fullName,"%s/%s",corefname,dp->d_name);
			if(stat(fullName, &stbuf) == -1){
				continue;
			}
			if(strncmp(dp->d_name, "core.", 5) == 0){
				// rename it
				if (rename(fullName, coreName) < 0) {
					//CRERROR("Failed to rename %s to core, errno %d", fullName, errno);
          printf("Failed to rename %s to core, errno %d", fullName, errno);
				}
			}
		}		

		if(dirp != NULL){
			closedir(dirp);
		}
#endif

		strcat(corefname, "/core");
		if (stat(corefname, &stbuf) < 0) {
			/*
			 * Check to see if we got an unexpected error return
			 * from "stat()":
			 */
			if (errno != ENOENT) {
				//CRERROR("INchkcore():Error\n\terror return from \"stat()\", errno %d...\n\tattempt to stat \"%s\"",errno, corefname);
        printf("INchkcore():Error\n\terror return from \"stat()\", errno %d...\n\tattempt to stat \"%s\"\n",
               errno, corefname);
			}
			return;
		}

		/* we have a new core file. print msg at rop */

		INcheckDirSize();

		{
			char *ptime;
#ifndef UNIXWARE
			int fd;
			struct user prochdr;

			/* read core file to determine guilty process */
			if (((fd = open(corefname, O_RDONLY)) >= 0) &&
			    (read(fd, (char *)&prochdr, sizeof(prochdr)) > 0))
			{
				/* get time of core file */
				ptime = ctime(&stbuf.st_mtime);

				/* print msg to rop */
				if (ptime != 0 && prochdr.u_comm != 0)
				{
					//CR_PRM(POA_MAJ,"REPT INIT - CORE FILE %s DETECTED FOR PROCESS %s\nCORE FILE CREATED AT %s\n", corefname, prochdr.u_comm, ptime);
          printf("REPT INIT - CORE FILE %s DETECTED FOR PROCESS %s\nCORE FILE CREATED AT %s\n",
                 corefname, prochdr.u_comm, ptime);
				}

			}

			close(fd);
#else
			/* get time of core file */
			ptime = ctime(&stbuf.st_mtime);
			/* print msg to rop */
			if (ptime != 0)
			{
				//CR_PRM(POA_MAJ,"REPT INIT - CORE FILE %s DETECTED FOR PROCESS %s\nCORE FILE CREATED AT %s\n", corefname, cdir, ptime);
        printf("REPT INIT - CORE FILE %s DETECTED FOR PROCESS %s\nCORE FILE CREATED AT %s\n",
               corefname, cdir, ptime);
			}
#endif
		}

		Char new_core[IN_NAMEMX+IN_PATHNMMX+6];
		int newindx;
		int app_indx = strlen(corefname);
		time_t mtime = 0;

		strcpy(new_core, corefname);

		/* Make sure new core file name will be terminated */
		new_core[app_indx+1] = 0;

		for (newindx=0; newindx < INNUMCORE; newindx++) {
			new_core[app_indx] = 'A' + newindx;
			if (stat(new_core, &stbuf) < 0) {
				break;
			}
			if ((mtime == 0) ||(stbuf.st_mtime < mtime)) {
				mtime = stbuf.st_mtime;
			}
		}

		if (newindx >= INNUMCORE) {
			new_core[app_indx] = 'A' + newindx;
		}

		//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INchkcore: new core file %s.", new_core));
    printf("INchkcore: new core file %s.\n", new_core);
		if (rename(corefname, new_core) < 0) {
			//CRERROR("INchkcore():Error\n\terror return from \"rename()\", errno %d...\n\tattempt to rename \"%s\" to \"%s\"",errno, corefname, new_core);
      printf("INchkcore():Error\n\terror return from \"rename()\", errno %d...\n\tattempt to rename \"%s\" to \"%s\"\n",
             errno, corefname, new_core);
		}
		else {
			//INIT_DEBUG((IN_DEBUG | IN_RSTRTR),(POA_INF,"INchkcore():\n\tmoved core file from \"%s\"\n\tto \"%s\"", corefname, new_core));
      printf("INchkcore():\n\tmoved core file from \"%s\"\n\tto \"%s\"\n",
             corefname, new_core);
		}
	}
}

