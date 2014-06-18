/*
 *      Copyright (C) 2013 Yin Yi
 *	  inngi2011@gmail.com
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "threads/SystemClock.h"
#include "system.h"
#include "utils/log.h"
#include "Telop.h"

CBackgroundTelop::CBackgroundTelop() : CThread("CBackgroundTelop")
{

}

CBackgroundTelop::~CBackgroundTelop()
{
  StopThread();
}

void CBackgroundTelop::Create()
{
  CThread::Create(false);
}

void CBackgroundTelop::Process()
{
	while (!m_bStop)
  	{ // loop around forever
  		CEvent dummy;
  		if (AbortableWait(dummy,100) == WAIT_TIMEDOUT){
			//m_labelCtl.Render();
			CLog::Log(LOGDEBUG, "Telop is Rendering!");
		}
	}
}

CGUITelopControl::CGUITelopControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath)
    : CGUILabelControl(parentID, controlID, posX, posY, width, height,labelInfo,wrapMultiLine ? CGUILabel::OVER_FLOW_WRAP : CGUILabel::OVER_FLOW_TRUNCATE,bHasPath )
{
	m_pBackgroundTelop = NULL;
	ControlType = GUICONTROL_TELOP;

}

CGUITelopControl::~CGUITelopControl(void)
{
	if (m_pBackgroundTelop)
  	{
    	// sleep until the loader finishes loading the current pic
    	CLog::Log(LOGDEBUG,"Waiting for BackgroundLoader thread to close");
    	while (m_pBackgroundTelop->IsLoading())
      	Sleep(10);
    	// stop the thread
    	CLog::Log(LOGDEBUG,"Stopping BackgroundLoader thread");
    	m_pBackgroundTelop->StopThread();
    	delete m_pBackgroundTelop;
    	m_pBackgroundTelop = NULL;
  	}
	
}

void CGUITelopControl::Render()
{
	if(!m_pBackgroundTelop){
		m_pBackgroundTelop = new CBackgroundTelop();

		m_pBackgroundTelop->Create();	//thread start
	}

}




