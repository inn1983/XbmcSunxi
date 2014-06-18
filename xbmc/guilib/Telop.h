#ifndef GUILIB_GUITELOPCONTROL_H
#define GUILIB_GUITELOPCONTROL_H

#pragma once


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

#include <set>
#include "guilib/GUIWindow.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/SortUtils.h"
//#include "GUIControl.h"
//#include "GUILabel.h"
#include "GUILabelControl.h"
#include "threads/Event.h"


class CBackgroundTelop : public CThread
{
public:
  CBackgroundTelop();
  ~CBackgroundTelop();
  virtual void Create();

  //void Create(CGUIWindowSlideShow *pCallback);
  
  bool IsLoading() { return m_isLoading;};

private:
  	void Process();

  	//CGUILabelControl m_labelCtl;
  	bool m_isLoading;

};

class CGUITelopControl : public CGUILabelControl
{
public:
	CGUITelopControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath);
	virtual ~CGUITelopControl(void);
	virtual void Render();
	CBackgroundTelop* m_pBackgroundTelop;
	
};
#endif

