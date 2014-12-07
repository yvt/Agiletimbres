//
//  ATCompactWindow.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/16/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATCompactWindow.h"

#include "ATSynthesizer.h"
#include "ATKeyboardInputView.h"
#include "Utils.h"
#include "ATKeypadView.h"
#include <tcw/twSDLDC.h>
#include "ATTabView.h"
#include <tcw/twMsgBox.h>
#include "ATMainWindow.h"
#include "main.h"

#include "xpms/KeypadIcon.xpm"
#include "xpms/KeyboardIcon.xpm"
#include "xpms/ProcessorIcon.xpm"

enum{
    IdKeypadOctaveUp=1,
    IdKeypadOctaveDown,
    IdKeyboardOctaveUp,
    IdKeyboardOctaveDown,
    IdRestore
};

ATCompactWindow::ATCompactWindow(ATMainWindow *mw):
m_originalMainWindow(mw){
    twRect rt;
    m_keyboardInput=new ATKeyboardInputView();
    
    m_keypad=new ATKeypadView();
    m_keypad->setRect(twRect(0, 14, 460, 81));
    m_keypad->setParent(this);
    m_keypad->show();
    
    // keypad octave editor.
    
    rt=rectForKeypadOctaveEditor();
    m_keypadOctaveUpButton.setRect(twRect(rt.x+rt.w-27, rt.y,
                                          13, 13));
    m_keypadOctaveUpButton.setTitle(L"+");
    m_keypadOctaveUpButton.setId(IdKeypadOctaveUp);
    m_keypadOctaveUpButton.setParent(this);
    m_keypadOctaveUpButton.show();
    
    m_keypadOctaveDownButton.setRect(twRect(rt.x+rt.w-13, rt.y,
                                            13, 13));
    m_keypadOctaveDownButton.setTitle(L"-");
    m_keypadOctaveDownButton.setId(IdKeypadOctaveDown);
    m_keypadOctaveDownButton.setParent(this);
    m_keypadOctaveDownButton.show();
    
    m_keypadOctaveLabel.setRect(twRect(rt.x+15, rt.y+1,
                                       rt.w-25-15, 11));
    m_keypadOctaveLabel.setParent(this);
    m_keypadOctaveLabel.show();
    
    m_keypadIcon=twSDLDC::loadFromXPM(KeypadIcon);
    
    updateKeypadOctaveEditor();
    
    // keyboard octave editor.
    
    rt=rectForKeyboardOctaveEditor();
    m_keyboardOctaveUpButton.setRect(twRect(rt.x+rt.w-27, rt.y,
                                            13, 13));
    m_keyboardOctaveUpButton.setTitle(L"+");
    m_keyboardOctaveUpButton.setId(IdKeyboardOctaveUp);
    m_keyboardOctaveUpButton.setParent(this);
    m_keyboardOctaveUpButton.show();
    
    m_keyboardOctaveDownButton.setRect(twRect(rt.x+rt.w-13, rt.y,
                                              13, 13));
    m_keyboardOctaveDownButton.setTitle(L"-");
    m_keyboardOctaveDownButton.setId(IdKeyboardOctaveDown);
    m_keyboardOctaveDownButton.setParent(this);
    m_keyboardOctaveDownButton.show();
    
    m_keyboardOctaveLabel.setRect(twRect(rt.x+15, rt.y+1,
                                         rt.w-25-15, 11));
    m_keyboardOctaveLabel.setParent(this);
    m_keyboardOctaveLabel.show();
    
    m_keyboardIcon=twSDLDC::loadFromXPM(KeyboardIcon);
    
    updateKeyboardOctaveEditor();
        
    m_restoreButton.setParent(this);
    m_restoreButton.setTitle(L"Restore");
    m_restoreButton.setId(IdRestore);
    m_restoreButton.setRect(rectForRestoreButton());
    m_restoreButton.show();
    
    
    m_processorIcon=twSDLDC::loadFromXPM(ProcessorIcon);
    
    m_processorLoadTimer=new twTimerWithInvocation
    (new twNoArgumentMemberFunctionInvocation<ATCompactWindow>
     (this, &ATCompactWindow::updateProcessorLoad));
    m_processorLoadTimer->setInterval(200);
    m_processorLoadTimer->addToEvent(tw_event);
}

ATCompactWindow::~ATCompactWindow(){
    delete m_keyboardInput;
    delete m_keypad;
    delete m_keypadIcon;
    delete m_keyboardIcon;
    delete m_processorIcon;
}

twRect ATCompactWindow::rectForAbout() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+251, keypadRect.y-13, 114, 13);
}

twRect ATCompactWindow::rectForRestoreButton() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+455-46, keypadRect.y-13, 46, 13);
}

#pragma mark - Processor Load

twRect ATCompactWindow::rectForProcessorLoad() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+131, keypadRect.y-13, 114, 13);
}

void ATCompactWindow::updateProcessorLoad(){
    //updateWaveform();
    invalidateClientRect(rectForProcessorLoad());
}

#pragma mark - Keypad Management

twRect ATCompactWindow::rectForKeypadOctaveEditor() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+1, keypadRect.y-13, 60, 13);
}

twRect ATCompactWindow::rectForKeypadIcon() const{
    twRect octaveEditorRect=rectForKeypadOctaveEditor();
    return twRect(octaveEditorRect.x, octaveEditorRect.y,
                  13,13);
}

void ATCompactWindow::updateKeypadOctaveEditor(){
    int currentOctave=m_keypad->baseNode()/12; // baseNote?
    currentOctave--;
    m_keypadOctaveLabel.setTitle(NHFormatStd(L"C%d", currentOctave));
    {
        twWndStyle style=m_keypadOctaveUpButton.getStyle();
        style.enable=(currentOctave<4);
        m_keypadOctaveUpButton.setStyle(style);
    }
    {
        twWndStyle style=m_keypadOctaveDownButton.getStyle();
        style.enable=(currentOctave>1);
        m_keypadOctaveDownButton.setStyle(style);
    }
}


#pragma mark - Keyboard Management

twRect ATCompactWindow::rectForKeyboardOctaveEditor() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+66, keypadRect.y-13, 60, 13);
}

twRect ATCompactWindow::rectForKeyboardIcon() const{
    twRect octaveEditorRect=rectForKeyboardOctaveEditor();
    return twRect(octaveEditorRect.x, octaveEditorRect.y,
                  13,13);
}

void ATCompactWindow::updateKeyboardOctaveEditor(){
    int currentOctave=m_keyboardInput->baseNode()/12; // baseNote?
    currentOctave--;
    m_keyboardOctaveLabel.setTitle(NHFormatStd(L"C%d", currentOctave));
    {
        twWndStyle style=m_keyboardOctaveUpButton.getStyle();
        style.enable=(currentOctave<7);
        m_keyboardOctaveUpButton.setStyle(style);
    }
    {
        twWndStyle style=m_keyboardOctaveDownButton.getStyle();
        style.enable=(currentOctave>1);
        m_keyboardOctaveDownButton.setStyle(style);
    }
}



#pragma mark - Events

void ATCompactWindow::clientMouseDown(const twPoint &, twMouseButton){
    
    /*
     int ky=20+(rand()%12);
     
     for(int i=0;i<7;i++)
     synth->note(ky+12*i, 32, 200-i*20);
     */
    
}

void ATCompactWindow::clientMouseMove(const twPoint &){
    
}

void ATCompactWindow::clientMouseUp(const twPoint &, twMouseButton){
    
}

void ATCompactWindow::clientKeyDown(const std::wstring &k){
    if(k==L"Escape"){
        twInvocation *inv=new twNoArgumentMemberFunctionInvocation
        <ATCompactWindow>(this, &ATCompactWindow::gotoMainWindow);
        tw_event->invokeOnMainThread(inv);
        return;
    }
    m_keyboardInput->clientKeyDown(k);
    clientKeyEventHandled();
}

void ATCompactWindow::clientKeyPress(wchar_t ch){
    clientKeyEventHandled();
}

void ATCompactWindow::clientKeyUp(const std::wstring &k){
    m_keyboardInput->clientKeyUp(k);
    clientKeyEventHandled();
}

void ATCompactWindow::clientPaint(const twPaintStruct &p){
    twDC *dc=p.dc;
    dc->fillRect(0x000000, getRect());
    
    dc->bitBlt(m_keypadIcon, rectForKeypadIcon().loc(), 
               twRect(0, 0, 13, 13));
    
    dc->bitBlt(m_keyboardIcon, rectForKeyboardIcon().loc(), 
               twRect(0, 0, 13, 13));
    
    if(rectForProcessorLoad() && p.paintRect){
        twRect rt=rectForProcessorLoad();
        
        dc->bitBlt(m_processorIcon, rt.loc(), 
                   twRect(0, 0, 13, 13));
        
        rt.x+=14;
        rt.w-=14;
        ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
        dc->fillRect(twRGB(128, 128, 128), 
                     rt);
        
        twRect rt2=rt;
        float per=synth->processorLoad();
        rt2.w=(int)((float)rt2.w*per)/100;
        dc->fillRect(tw_curSkin->getSelectionColor(), 
                     rt2);
        
        std::wstring str=NHFormatStd(L"%d%lc", (int)per, L'%');
        const twFont *font=getFont();
        twSize sz=font->measure(str);
        font->render(dc,tw_curSkin->getSelectionTextColor(), 
                     twPoint(rt.x+(rt.w-sz.w)/2,
                             rt.y+(rt.h-sz.h)/2), 
                     str);
        
        
    }
    
    if(rectForAbout()&&p.paintRect){
        twRect rt=rectForAbout();
        std::wstring str;
        str=L"Nexhawks Agiletimbres ";
        str+=ATVersion();
        
        twSize sz=getFont()->measure(str);
        getFont()->render(dc, twRGB(192,192,192), 
                          twPoint(rt.x,rt.y+(rt.h-sz.h)/2), str);
    }
    
}

bool ATCompactWindow::clientHitTest(const twPoint &pt) const{
    return true;
}

void ATCompactWindow::setRect(const twRect &rt){
    twWnd::setRect(rt);
}

void ATCompactWindow::command(int cmdId){
    if(cmdId==IdKeypadOctaveUp){
        m_keypad->setBaseNote(m_keypad->baseNode()+12);
        updateKeypadOctaveEditor();
    }else if(cmdId==IdKeypadOctaveDown){
        m_keypad->setBaseNote(m_keypad->baseNode()-12);
        updateKeypadOctaveEditor();
    }else if(cmdId==IdKeyboardOctaveUp){
        m_keyboardInput->setBaseNote(m_keyboardInput->baseNode()+12);
        updateKeyboardOctaveEditor();
    }else if(cmdId==IdKeyboardOctaveDown){
        m_keyboardInput->setBaseNote(m_keyboardInput->baseNode()-12);
        updateKeyboardOctaveEditor();
    }else if(cmdId==IdRestore){
        twInvocation *inv=new twNoArgumentMemberFunctionInvocation
        <ATCompactWindow>(this, &ATCompactWindow::gotoMainWindow);
        tw_event->invokeOnMainThread(inv);
        
    }
}

void ATCompactWindow::gotoMainWindow(){
    // stop already playing sounds.
    m_keypad->onMouseUp(twPoint(0,0), twMB_left);
    m_keyboardInput->allNotesOff();
    
    m_originalMainWindow->restoreFromCompactWindow();
    delete this;
    return;
}

void ATCompactWindow::setKeyboardInputBaseNote(int k){
    m_keyboardInput->setBaseNote(k);
    updateKeyboardOctaveEditor();
}

void ATCompactWindow::setKeypadBaseNote(int k){
    m_keypad->setBaseNote(k);
    updateKeypadOctaveEditor();
}

int ATCompactWindow::keypadBaseNote() const{
    return m_keypad->baseNode();
}

int ATCompactWindow::keyboardInputBaseNote() const{
    return m_keyboardInput->baseNode();
}
