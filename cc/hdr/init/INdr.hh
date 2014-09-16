#ifndef _INDR_H
#define _INDR_H

#define INmaxCCs	2

struct INdr 
{
	char	m_ccState[INmaxCCs];		/* State of CC			*/
	char	m_bStarted;			/* True if system is Astarted	*/
	char	m_bShutdownFinished;		/* True if lead shutdown is finished*/
};

#endif
