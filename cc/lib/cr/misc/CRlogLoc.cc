/*
**      File ID:        @(#): <MID18617 () - 09/06/02, 29.1.1.1>
**
**	File:					MID18617
**	Release:				29.1.1.1
**	Date:					09/12/02
**	Time:					10:39:15
**	Newest applied delta:	09/06/02
**
** DESCRIPTION:
**	This file implements the class that keeps track of a location
**	in a logical log file.
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/

#include <errno.h>
#include <String.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cc/hdr/cr/CRlogFile.hh"
#include "cc/hdr/cr/CRdebugMsg.hh"

CRlogLoc::CRlogLoc() {
	byteOffset = -1;
}

CRlogLoc::~CRlogLoc() {
}

void CRlogLoc::init(const char* logical_fname) {
  curOpenIndex.init(CRlogFile::DEFNUMFILES);
	byteOffset = 0;
	lastChange = 0;
	logFname = CRDEFLOGDIR;
	logFname += "/";
	logFname += logical_fname;
}

void CRlogLoc::init(const char* logical_fname, const char* directory,
                    Long log_offset, int numFiles, const char* daily,
                    int logAge) {

	byteOffset = log_offset;
	lastChange = 0;
	curOpenIndex.init(numFiles);

	logFname = directory;
	logFname += "/";
	logFname += logical_fname;
	
	if (strlen(logical_fname) + strlen(directory) + 1 != logFname.length()) {
		CRERROR("logical_fname# '%s/%s' is too long", 
		       directory, logical_fname);
	}
}

Bool CRlogLoc::isValid() {
	return (byteOffset >= 0) ? YES : NO;
}

time_t CRlogLoc::curTime() const {
	return lastChange;
}

void CRlogLoc::getLogicalName(CRlogFname& result) const {
	result = logFname;
}

void CRlogLoc::getSubFileName(int sub_index, CRlogFname& result) const {
	result = logFname;
	result += int_to_str(sub_index).c_str();
}

void CRlogLoc::getSubFileName(CRlogFname& result) const {
	result = logFname;
	result += int_to_str(curOpenIndex.value()).c_str();
}

/* returns NO if there is no OM ready to print */
Bool CRlogLoc::moveToNextPrintAble(FILE* &openFp) {
	if (isAtEof() == NO)
		return YES;

	if (moveToNextNewerFile() == NO)
		return NO;

	if (openFp) {
		fclose(openFp);
		openFp = NULL;
	}

	if (isAtEof() == NO)
		return YES;

	return NO;
}

void CRlogLoc::seek(FILE* fp) const {
	fseek(fp, byteOffset, 0);
}


void CRlogLoc::setSubIndex(int newIndex) {
	byteOffset = 0;
	curOpenIndex.set(newIndex);
}

int CRlogLoc::nextSubIndex() {
	byteOffset = 0;
	return curOpenIndex.incr();
}

int CRlogLoc::incr(int numChars) {
	byteOffset += numChars;
	return byteOffset;
}

void CRlogLoc::dump() const {
	fprintf(stderr, "CRlogLoc dump(%s: curindx=%d, offset=%d, time=%d)\n",
		(const char *) logFname, curOpenIndex.value(),
		byteOffset, lastChange);
}

/* move to end of newest file */
void CRlogLoc::moveToEndOfNewestFile() {
	int newestIndex = 0;
	time_t modTime = 0;
	Long fileLen = 0;

	if (findNewestFile(newestIndex, modTime, fileLen) == YES) {
		setSubIndex(newestIndex);
		byteOffset = fileLen;
		lastChange = modTime;
	}
}


Bool CRlogLoc::findNewestFile(int& newest_index, time_t& latest_time,
                              Long& file_len) const {
	latest_time = 0;
	Bool file_found = NO;
	newest_index = -1;

	CRlogFname filename;

	for (int indx = 0; indx < curOpenIndex.getSize(); indx++) {
		getSubFileName(indx, filename);
		struct stat stbuf;

		if (stat(filename, &stbuf) == -1) {
			if (errno != ENOENT) {
				CRDEBUG(CRusli, ("stat() of %s failed with errno %d",
                         (const char*) filename, errno));
				return NO;
			}
		} else {
			file_found = YES;
			if (stbuf.st_mtime > latest_time) {
				file_len = stbuf.st_size;
				newest_index = indx;
				latest_time = stbuf.st_mtime;
			}
			//
			//	There are cases where the new file
			//	has the same st_mtime as the old one(0)
			//	so we miss some OM's until
			//	the st_mtime changes
			//
			else if ((stbuf.st_mtime == latest_time) &&
			         (file_len > stbuf.st_size)) {
				file_len = stbuf.st_size;
				newest_index = indx;
				latest_time = stbuf.st_mtime;
			}
		}
	}

	return file_found;
}

Bool CRlogLoc::isAtEof() const {
	CRlogFname filename;
	getSubFileName(filename);
	struct stat stbuf;

	if (stat(filename, &stbuf) == -1) {
		if (errno != ENOENT) {
			CRERROR("stat() failed with error number %d\n",
				errno);
			return YES;
		}
		return YES;
	}

	if (byteOffset >= stbuf.st_size)
		return YES;

/*fprintf(stderr, "CRlogLoc dump(%s: offset=%d, stbuf.st_size=%d)\n",
	logFname, 
	byteOffset, stbuf.st_size);
	*/
	return NO;
}

Bool CRlogLoc::moveToNextNewerFile() {
	int startIndex = curOpenIndex.value();

	CRlogFname filename;

	for (curOpenIndex.incr();
	     curOpenIndex.value() != startIndex;
	     curOpenIndex.incr()) {
		getSubFileName(filename);
		struct stat stbuf;

		if (stat(filename, &stbuf) == -1) {
			if (errno != ENOENT) {
				curOpenIndex.set(startIndex);
				CRERROR("stat() failed with error number %d\n",
					errno);
				return NO;
			}
		} else {
			if (stbuf.st_mtime > lastChange) {
				byteOffset = 0;
				lastChange = stbuf.st_mtime;
				return YES;
			}
		}
	}
	return NO;
}
