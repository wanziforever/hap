/*
**      File ID:        @(#): <MID19367 () - 08/17/02, 29.1.1.1>
**
**	File:					MID19367
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:40:13
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**      This file defines the implementation for the CRwindow and CRfileDevice
**      classes.
**
** OWNER: 
**	Roger McKee
**
** NOTES:
*/
#include <string.h>
#include "cc/cr/hdr/CRterminal.H"
#include "cc/cr/hdr/CRwindow.H"
#include "cc/cr/hdr/CRomText.H"

CRfileDevice::CRfileDevice(FILE* filePtr) : fp(filePtr)
{
	lastCharPrinted = '\n';
}

CRfileDevice::~CRfileDevice()
{
	putc('\n', fp);
}

void
CRfileDevice::addch(char c)
{
	lastCharPrinted = c;
	putc(c, fp);
}

void
CRfileDevice::refresh()
{
	fflush(stdout);
}

void
CRfileDevice::addstr(const char* s)
{
	fputs(s, fp);
	lastCharPrinted = s[strlen(s) - 1];
}

void
CRfileDevice::printPrompt(int, int, const char* text, Bool /* underLine */)
{
	addstr(text);
	refresh();
}

Position
CRfileDevice::cursorPos()
{
	int xcoord = 1;

	if (lastCharPrinted == '\n')
		xcoord = 0;

	return Position(xcoord, 0);
}

CRwindow::CRwindow()
{
	state = winUninit;
	underlineLastLine = NO;
}

CRwindow::~CRwindow()
{
	// nothing to do
}

void
CRwindow::init()
{
	state = winUninit;
	curUnderline	= NO;
	curGraphic	= NO;
	curFlash	= NO;
	curForeGround	= White;
	curBackGround	= Black;
	windowMode      = CRsmallTextWindow;
	curIntensity = NormIntensity;
}

void
CRwindow::shutDown()
{
	state = winUninit;
}

void
CRwindow::setWindowMode(int)
{
}

/* toggle the value and return new value */
CRwindowMode
CRwindow::toggleShrinkExpand()
{
	if (windowMode == CRfullTextWindow)
		windowMode = CRsmallTextWindow;
	else
		windowMode = CRfullTextWindow;

	return windowMode;
}

void
CRwindow::activate()
{
	state = winActive;
	CRtermController::setVideoAtts(curForeGround, curBackGround, curFlash,
				       curUnderline, curGraphic);
}

void
CRwindow::deactivate()
{
	state = winInactive;
}

void
CRwindow::setVideoAtts(CRcolor fc, CRcolor bc, Bool flash, Bool underline,
		       Bool graphic, CRintensity intensity)
{
	CRtermController::setVideoAtts(fc, bc, flash, underline, graphic,
				       intensity);
	curForeGround	= fc;
	curBackGround	= bc;
	curUnderline	= underline;
	curGraphic	= graphic;
	curFlash	= flash;
	curIntensity    = intensity;
}

void
CRwindow::printPrompt(int /*row*/,int /*column*/, const char* /*prompt*/,
		      Bool /*underLine*/)
{
}


CRstatusLine::CRstatusLine()
{
}

CRstatusLine::~CRstatusLine()
{
}
