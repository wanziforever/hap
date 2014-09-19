/*
**      File ID:        @(#): <MID19366 () - 08/17/02, 29.1.1.1>
**
**	File:					MID19366
**	Release:				29.1.1.1
**	Date:					08/21/02
**	Time:					19:40:13
**	Newest applied delta:	08/17/02
**
** DESCRIPTION:
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/
#ifdef CC
//IBM JGH 05/09/06 change path
 #ifdef __linux
 #include <termio.h>
 #else
 #include <sys/termio.h>
 #endif
#else
#include <sys/ttychars.h>
#endif
#include "cc/cr/hdr/CRterminal.H"
#include "cc/hdr/misc/GLasync.H"

CRwindow* CRtermController::curWindow = NULL;
CRterminal* CRtermController::curTerminal = NULL;

void
CRtermController::setCurTerminal(CRterminal* term)
{
	curTerminal = term;
}

void
CRtermController::beep()
{
	curTerminal->beep();
}

void
CRtermController::refresh()
{
	curTerminal->refresh();
}

CRwindow*
CRtermController::activate(CRwindow* win)
{
	CRwindow* retval = curWindow;
	if (curWindow)
	{
		if (curWindow ==  win)
			return NULL;
		curWindow->deactivate();
	}

	curWindow = win;
	if (win)
		win->activate();
	return retval;
}

void
CRtermController::setVideoAtts(CRcolor fc, CRcolor bc, Bool flash,
			       Bool underline, Bool graphic,
			       CRintensity intensity)
{
	if (curTerminal)
		curTerminal->setVideoAtts(fc, bc, flash,
					  underline, graphic, intensity);
}


CRterminal::CRterminal()
{
	curUnderline = NO;
	curGraphic = NO;
	curFlash = NO;
}

CRterminal::~CRterminal()
{
}

void
CRterminal::shutDown()
{
	CRtermController::activate(NULL);
}

void
CRterminal::refresh()
{
	GLFFLUSH(stdout);
}

void
CRterminal::beep()
{
//IBM JGH 05/09/06 use the quoted G for linux
#ifdef __linux
	GLPUTCHAR(CTRL('G'));
#else
#ifdef _SVR4
	GLPUTCHAR(CTRL('G'));
#else
	GLPUTCHAR(CTRL(G));
#endif
#endif
}

void
CRterminal::setFkey(int /*fkeynum*/, const char* /*label*/,
		    const char* /*value*/)
{
}

