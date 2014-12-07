//
//  ATSplashWindow.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "ATSplashWindow.h"
#include "xpms/Logo.xpm"
#include <tcw/twSDLDC.h>
#include "main.h"

ATSplashWindow::ATSplashWindow(){
	// setup this window to be the desktop.
	setRect(twRect(twPoint(0, 0), tw_app->getScrSize()));
	setTitle(L"Splash Window");
	
	logo=twSDLDC::loadFromXPM(Logo);
	
	
	// setup the timer to start the initialization after one second.
	initTimer=new twTimerWithInvocation(new twNoArgumentMemberFunctionInvocation<ATSplashWindow>
										(this,&ATSplashWindow::initTimerFired));
	initTimer->setInterval(100);
	initTimer->addToEvent(tw_event);
	
}

ATSplashWindow::~ATSplashWindow(){
	delete initTimer;
	delete logo;
}

void ATSplashWindow::clientPaint(const twPaintStruct& p){
	twDC *dc=p.dc;
	
	// fill the background.

	dc->fillRect(0, getRect());
	
	twRect rt=getRect();
	
	twPoint pt((rt.w-256)/2, (rt.h-96)/2);
	dc->bitBlt(logo, pt, twRect(0,0,256,96));
	
	getFont()->render(dc, twRGB(200, 70, 0), twPoint(3, 59)+pt, 
					  L"Integrated Software Synthesizer");
	
	std::wstring verStr=L"Version "+ATVersion();
	twSize sz=getFont()->measure(verStr);
	getFont()->render(dc, twRGB(200, 70, 0), twPoint(254-sz.w, 89-sz.h)+pt, 
					  verStr);
}
bool ATSplashWindow::clientHitTest(const twPoint& pt) const{
	return true;
}

void ATSplashWindow::setRect(const twRect& rt){
	twWnd::setRect(rt);
	
}

void ATSplashWindow::command(int ctrlId){
	
}

void ATSplashWindow::initTimerFired(){
	// stop the timer.
	initTimer->invalidate();
	initializeMain();
}

