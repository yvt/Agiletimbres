//
//  NHHardRoundSkin.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "NHHardRoundSkin.h"

void NHHardRoundSkin::drawSolidBorder(twDC *dc, const twRect& rt, const twWndStatus&, const std::wstring&){
	dc->fillRect(getBorderColor(), twRect(rt.x, rt.y, rt.w, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x, rt.y+rt.h-1, rt.w, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x, rt.y, 1, rt.h));
	dc->fillRect(getBorderColor(), twRect(rt.x+rt.w-1, rt.y, 1, rt.h));
	
}
bool NHHardRoundSkin::hitTestSolidBorder(const twRect& rt, const twPoint& pt){
	return true;
}
twRect NHHardRoundSkin::getSolidBorderCliRect(const twRect& rt){
	return twRect(rt.x+1, rt.y+1, rt.w-2, rt.h-2);
}

void NHHardRoundSkin::drawPanelBorder(twDC *dc, const twRect& rt, const twWndStatus&, const std::wstring&){
	dc->fillRect(getBorderColor(), twRect(rt.x+1, rt.y, rt.w-2, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x+1, rt.y+rt.h-1, rt.w-2, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x, rt.y+1, 1, rt.h-2));
	dc->fillRect(getBorderColor(), twRect(rt.x+rt.w-1, rt.y+1, 1, rt.h-2));
}
bool NHHardRoundSkin::hitTestPanelBorder(const twRect& rt, const twPoint& pt){
    if((pt.x==rt.x || pt.x==rt.x+rt.w-1) &&
       (pt.y==rt.y || pt.y==rt.y+rt.h-1))
        return false;
	return true;
}
twRect NHHardRoundSkin::getPanelBorderCliRect(const twRect& rt){
	return twRect(rt.x+1, rt.y+1, rt.w-2, rt.h-2);
}

void NHHardRoundSkin::drawDialogBorder(twDC *dc, const twRect& rt, const twWndStatus& s, const std::wstring& str){
	twRect rt2;
	
	dc->fillRect(getBorderColor(), twRect(rt.x+1, rt.y, rt.w-2, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x+1, rt.y+rt.h-1, rt.w-2, 1));
	dc->fillRect(getBorderColor(), twRect(rt.x, rt.y+1, 1, rt.h-2));
	dc->fillRect(getBorderColor(), twRect(rt.x+rt.w-1, rt.y+1, 1, rt.h-2));
	
	
	rt2=twRect(rt.x+1, rt.y+1, rt.w-2, getTitleHeight());
	if(s.focused){
		dc->fillRect(getActiveTitleColor(), rt2);
		tw_defFont->render(dc, getActiveTitleTextColor(), twPoint(rt2.x+2, rt2.y+1), str);
	}else{
		dc->fillRect(getInactiveTitleColor(), rt2);
		tw_defFont->render(dc, getInactiveTitleTextColor(), twPoint(rt2.x+2, rt2.y+1), str);
	}
}
bool NHHardRoundSkin::hitTestDialogBorder(const twRect& rt, const twPoint& pt){
    if((pt.x==rt.x || pt.x==rt.x+rt.w-1) &&
       (pt.y==rt.y || pt.y==rt.y+rt.h-1))
        return false;
	return true;
}
twRect NHHardRoundSkin::getDialogBorderCliRect(const twRect& rt){
	return twRect(rt.x+1, rt.y+1+getTitleHeight(), rt.w-2, rt.h-2-getTitleHeight());
}

void NHHardRoundSkin::drawOverlappedBorder(twDC * dc, const twRect& rt, const twWndStatus& s, const std::wstring& str){
	
	// TODO: implement overlapped border...
	twRect rt2;
	
	drawEdge(dc, rt, twES_raised);
	dc->drawRect(getCtrlColor(), twRect(rt.x+2, rt.y+2, rt.w-5, rt.h-5));
	dc->drawRect(getCtrlColor(), twRect(rt.x+3, rt.y+3, rt.w-7, rt.h-7));
	
	rt2=twRect(rt.x+4, rt.y+4, rt.w-8, getTitleHeight());
	if(s.focused){
		dc->fillRect(getActiveTitleColor(), rt2);
		tw_defFont->render(dc, getActiveTitleTextColor(), twPoint(rt2.x+2, rt2.y+1), str);
	}else{
		dc->fillRect(getInactiveTitleColor(), rt2);
		tw_defFont->render(dc, getInactiveTitleTextColor(), twPoint(rt2.x+2, rt2.y+1), str);
	}
}
bool NHHardRoundSkin::hitTestOverlappedBorder(const twRect&, const twPoint&){
	return true;
}
twRect NHHardRoundSkin::getOverlappedBorderCliRect(const twRect& rt){
	return twRect(rt.x+4, rt.y+4+getTitleHeight(), rt.w-8, rt.h-8-getTitleHeight());
}

void NHHardRoundSkin::drawButton(twDC *dc, const twRect& rt, const twWndStatus& s, bool pressed, const std::wstring& str, const twFont *fnt){
	
	twColor bgColor=twRGB(192, 192, 192);
    twColor textColor=twRGB(0, 0, 0);
    twSize sz=fnt->measure(str);
    
    if(pressed){
        bgColor=twRGB(200,90,0);
        textColor=twRGB(255, 255, 255);
    }
    if(!s.enable){
        bgColor=twRGB(128, 128, 128);
    }
    
    dc->fillEllipse(bgColor, twRect(rt.x, rt.y,
                                    rt.h-1, rt.h-1));
    dc->fillEllipse(bgColor, twRect(rt.x+rt.w-rt.h, rt.y,
                                    rt.h-1, rt.h-1));
    dc->fillRect(bgColor, twRect(rt.x+rt.h/2, rt.y,
                                 rt.w-rt.h, rt.h));
	
	fnt->render(dc, textColor, twPoint(rt.x+(rt.w-sz.w)/2, rt.y+(rt.h-sz.h)/2), str);
	
	
}

static twRect fixEllipseRect(twRect rt){
    rt.w--; rt.h--;
    return rt;
}

void NHHardRoundSkin::drawRadioBtn(twDC *dc, const twRect& rt, const twWndStatus& s, bool pressed, bool on, const std::wstring& str, const twFont *fnt){
    
    twSize checkSize(10, 10);
    twRect checkRect(rt.x, rt.y+(rt.h-checkSize.h)/2,
                     checkSize.w, checkSize.h);
    
    twColor checkBgColor=twRGB(192, 192, 192);
    twColor checkFgColor=twRGB(0, 0, 0);
    
    if((!s.enable)){
        checkBgColor=twRGB(128, 128, 128);
    }else if(pressed){
        checkBgColor=twRGB(200,90,0);
    }
    
    checkRect=fixEllipseRect(checkRect);
    
    dc->fillEllipse(checkBgColor, checkRect);
    
    if(on){
        dc->fillEllipse(checkFgColor, checkRect.inflate(-2));
    }
    
	
	fnt->render(dc, s.enable?getCtrlTextColor():getDisabledCtrlTextColor(), twPoint(rt.x+checkSize.w+1, rt.y+1),
				str);
	
	
}

void NHHardRoundSkin::drawCheckBox(twDC *dc, const twRect& rt, const twWndStatus& s, bool pressed, bool on, const std::wstring& str, const twFont *fnt){
	
    twSize checkSize(10, 10);
    twRect checkRect(rt.x, rt.y+(rt.h-checkSize.h)/2,
                     checkSize.w, checkSize.h);
    
    twColor checkBgColor=twRGB(192, 192, 192);
    twColor checkFgColor=twRGB(0, 0, 0);
    
    if((!s.enable)){
        checkBgColor=twRGB(128, 128, 128);
    }else if(pressed){
        checkBgColor=twRGB(200,90,0);
    }
    
    dc->fillRect(checkBgColor, checkRect);
    
    if(on){
        dc->fillRect(checkFgColor, checkRect.inflate(-2));
    }
    
	
	fnt->render(dc, s.enable?getCtrlTextColor():getDisabledCtrlTextColor(), twPoint(rt.x+checkSize.w+1, rt.y+1),
				str);
	
}

twColor NHHardRoundSkin::getBorderColor(){
	return twRGB(192, 192, 192);
}
twColor NHHardRoundSkin::getWndColor(){
	return twRGB(0, 0, 0);
}
twColor NHHardRoundSkin::getCtrlColor(){
	return twRGB(0,0,0);
}
twColor NHHardRoundSkin::getWndTextColor(){
	return twRGB(192, 192, 192);
}
twColor NHHardRoundSkin::getCtrlTextColor(){
	return twRGB(192, 192, 192);
}
twColor NHHardRoundSkin::getSelectionColor(){
	return twRGB(200,90,0);
}
twColor NHHardRoundSkin::getSelectionTextColor(){
	return twRGB(255,255,255);
}
twColor NHHardRoundSkin::getDisabledWndTextColor(){
	return twRGB(128, 128, 128);
}
twColor NHHardRoundSkin::getDisabledCtrlTextColor(){
	return twRGB(128, 128, 128);
}
twColor NHHardRoundSkin::getDarkCtrlColor(){
	twColor c=getCtrlColor();
	return (c&0xfefefe)>>1;
}
twColor NHHardRoundSkin::getDarkDarkCtrlColor(){
	twColor c=getDarkCtrlColor();
	return (c&0xfcfcfc)>>2;
}
twColor NHHardRoundSkin::getBrightCtrlColor(){
	twColor c=getCtrlColor();
	return 0xffffff-(((0xffffff-c)&0xfcfcfc)>>2);
}

twColor NHHardRoundSkin::getActiveTitleColor(){
	return twRGB(192, 192, 192);
}
twColor NHHardRoundSkin::getActiveTitleTextColor(){
	return twRGB(0, 0, 0);
}
twColor NHHardRoundSkin::getInactiveTitleColor(){
	return twRGB(0, 0, 0);
}
twColor NHHardRoundSkin::getInactiveTitleTextColor(){
	return twRGB(192, 192, 192);
}

