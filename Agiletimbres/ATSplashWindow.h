//
//  ATSplashWindow.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATSplashWindow : public twWnd{
private:
	twDC *logo;
	twTimer *initTimer;
	
	void initTimerFired();
	
public:
	ATSplashWindow();
	virtual ~ATSplashWindow();
	
	virtual void setRect(const twRect&);
	virtual void clientPaint(const twPaintStruct&);
	virtual bool clientHitTest(const twPoint&) const;
	virtual void command(int);
	
};

