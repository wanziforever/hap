/*
**      File ID:        @(#): <MID19368 () - 08/17/02, 29.1.1.1>
**
**	File:					MID19368
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:40:14
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file defines the CRinputDevice and CRrealKeyBoard classes
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include "cc/cr/hdr/CRshtrace.H"
#include "cc/cr/hdr/CRinputDev.H"
#include "cc/hdr/cr/CRsysError.H"

const int STDIN_FD = 0;	 /* stdin file descriptor */
const int STDOUT_FD = 1; /* stdout file descriptor */


CRinputDevice::CRinputDevice(int intChr) : interruptChar(intChr)
{
	interruptFlag = NO;
	origSettingsFromUser = NO;
}

CRinputDevice::~CRinputDevice()
{
}

void
CRinputDevice::origSettings(const CRTERMIO& userSettings)
{
	orig_settings = userSettings;
	origSettingsFromUser = YES;
}

Bool
CRinputDevice::interruptPending() const
{
	return interruptFlag;
}


void
CRinputDevice::clearInterrupt()
{
	interruptFlag = NO;
	storedChars_V = ""; /* deleted saved characters (including the 
		            ** character that caused the interrupt
			    */
}

void
CRinputDevice::setInterrupt()
{
	interruptFlag = YES;
}

Bool
CRinputDevice::charsReady() const
{
	return storedChars_V.is_empty() ? NO : YES;
}

int
CRinputDevice::getChar()
{
	if (charsReady() == NO)
	{
		int c = getNewChar();
		if (c == interruptChar)
			setInterrupt();
		return c;

#ifdef CURSESARROWKEYS
		switch (c)
		{
		case KEY_DOWN: return DOWN_ARROW;
		case KEY_UP: return UP_ARROW;
		default: return (char) c;
		}
#endif
	}

	char retchar = '\0';
	storedChars_V.getX(retchar);
	return retchar;
}

/* Look at the first character in the queue.
** If the queue is empty, try to quickly read at least one char.
*/
int
CRinputDevice::lookAtChar()
{
	if (charsReady() == NO)
	{
		if (loadNchars(1) == 0)
			return EOF;
	}

	return storedChars_V.char_at(0);
}

/* Look at the next N chars currently in the queue.
** If there are not N chars in the queue, try to quickly read some.
** Returns number of chars in queue.
*/
const char*
CRinputDevice::lookAtAvailableChars()
{
	if (storedChars_V.length() < 2)
	{
		loadNchars(2 - storedChars_V.length());
	}
	return storedChars_V;
}

/* remove N chars from front of queue */
void
CRinputDevice::removeNchars(int n)
{
	for (int i = 0; i < n; i++)
	{
		if (!storedChars_V.is_empty())
			storedChars_V.get();
	}
}

/* add char to end of queue */
void
CRinputDevice::unGetChar(char c)
{
	CRDEBUG(CRusli, ("unGetchar(%c)", c));
	storedChars_V.put(c);

	if (c == interruptChar)
		setInterrupt();
}

/* add chars to end of queue */
void
CRinputDevice::unGetChars(const char* s, int len)
{
	CRDEBUG(CRusli, ("unGetchars(%s,%d)", s, len));

	for (int i = 0; i < len; i++)
	{
		unGetChar(s[i]);
	}
}

/* CRrealKeyBoard represents just stdin that might be redirected from
** a file.
*/

CRrealKeyBoard::CRrealKeyBoard()
{
}

CRrealKeyBoard::~CRrealKeyBoard()
{
	restoreDevice();
}

GLretVal
CRrealKeyBoard::init()
{
	if (isatty(STDIN_FD))
		return conditionDevice("/dev/tty");
	else
		return GLsuccess;
}

void
CRrealKeyBoard::suspendInput()
{
}

void
CRrealKeyBoard::resumeInput()
{
}

GLretVal
CRrealKeyBoard::processMsg(char*, int, Bool&)
{
	return GLfail;
}

void
CRrealKeyBoard::prepForInput()
{
}

void
CRrealKeyBoard::restoreDevice()
{
	if (isatty(STDIN_FD))
	{
		if (ioctl(STDIN_FD, TCSETAF, &orig_settings) == -1)
		{
			CRERROR("ioctl() to restore terminal failed");
		}
	}
}


GLretVal
CRrealKeyBoard::conditionDevice(const char* /* device */)
{
	CRTERMIO term_st;
#ifdef _SVR4
#ifndef SOCKET
	if (tcgetattr(STDIN_FD, &term_st) == -1)
	{
		CRERROR("tcgetattr of MCC tty failed because of %s",
#else
	if (ioctl(STDIN_FD, TCGETA, &term_st) == -1)
	{
		CRERROR("ioctl(TCGETA) of MCC tty failed because of %s",
#endif
#else
	if (ioctl(STDIN_FD, TCGETA, &term_st) == -1)
	{
		CRERROR("ioctl(TCGETA) of MCC tty failed because of %s",
#endif
			  strerror(errno));
		return GLfail;
	}

	orig_settings = term_st;

	/* should set ^H as erase char and use symbolic values */
	term_st.c_iflag = IXON|ICRNL|ISTRIP|IGNPAR|BRKINT;
	term_st.c_oflag = ONLCR|OPOST;
	term_st.c_lflag = ECHO|ECHOE|ECHOK|ICANON|ISIG;
	term_st.c_lflag &= ~(ICANON|ISIG|ECHO);
#ifndef _SVR4
	term_st.c_line = 0;
#endif
	term_st.c_cc[VERASE] = 0x08;	/* CTRL-H */
	term_st.c_cc[4] = 5; /* MIN */
	term_st.c_cc[5] = 1; /* TIME */
#ifdef _SVR4
#ifndef SOCKET
	term_st.c_cc[VSUSP] = 0; /* SUSP */

	if (tcsetattr(STDIN_FD, TCSANOW, &term_st) == -1)
	{
		CRERROR("tcsetattr of MCC tty failed because of %s.",
#else
	if (ioctl(STDIN_FD, TCSETA, & term_st) == -1)
	{
		CRERROR("ioctl of MCC tty failed because of %s.",
#endif
#else
	if (ioctl(STDIN_FD, TCSETA, & term_st) == -1)
	{
		CRERROR("ioctl of MCC tty failed because of %s.",
#endif
			  strerror(errno));
		return GLfail;
	}
	return GLsuccess;
}

int
CRrealKeyBoard::getNewChar()
{
	return getchar();

#ifdef CURSESARROWKEYS
	switch (c)
	{
	        case KEY_DOWN: return DOWN_ARROW;
		case KEY_UP: return UP_ARROW;
		default: return (char) c;
	}
#endif
}

int
CRrealKeyBoard::loadNchars(int n)
{
	for (int i = 0; i < n; i++)
	{
		int c = getchar();
#ifdef CURSESARROWKEYS
		switch (c)
		{
		    case KEY_DOWN:
			c = DOWN_ARROW;
			break;
		    case KEY_UP:
			c = UP_ARROW;
			break;
		}
#endif
		storedChars_V.put(getchar());
	}
	return storedChars_V.length();
}
