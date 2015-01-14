/*
**      File ID:        @(#): <MID21311 () - 08/17/02, 29.1.1.1>
**
**	File:					MID21311
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:39:46
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**	This file defines the two utility functions: CRdirCheck and
**      CRcreateDir.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include "cc/hdr/cr/CRdirCheck.hh"
//#include "cc/cr/hdr/CRshtrace.H"

Bool CRcreateDir(const char* dir_name, const char* userId) {
	/* Temporarily change the umask to 0, so when the directory
	** is created it will REALLY have permissions of 777 and
	** not be influenced by the user's umask.
	** This is mainly needed for processes that are invoked from UNIX,
	** such as the uslsh.
	*/
	int curmask = (int) umask(0);
	int retcode = mkdir(dir_name, 0777);
	umask(curmask);
	if (retcode == -1)
		return NO;

	if (access(dir_name, W_OK|X_OK|F_OK) == -1)
		return NO;

	if (userId) {
		struct passwd *pw = getpwnam(userId);
		if (pw != NULL) {
			/* set to 'ainet' login */
			if (chown(dir_name, pw->pw_uid, pw->pw_gid) == -1) {
				//CRSHERROR("chown of %s to %d,%d failed due to %d",
        //          dir_name, pw->pw_uid,
        //          pw->pw_gid, errno);
			}
		}
		return YES;
	}
}

Bool CRdirCheck(const char* dir_name, Bool createFlag, const char* userId) {
	if (access(dir_name, W_OK|X_OK|F_OK) == -1) {
		if (errno == ENOENT && createFlag == YES)
			return CRcreateDir(dir_name, userId);

		if (errno == EACCES) {
			int curmask = (int) umask(0);
			int retval = chmod(dir_name, 0777);
			umask(curmask);

			if (retval == -1) {
				//CRDEBUG(CRusli,
        //        ("chmod of %s failed due to errno %d",
        //         dir_name, errno));
			} else {
				return YES;
			}
		}
		return NO;
	}
	return YES;
}

