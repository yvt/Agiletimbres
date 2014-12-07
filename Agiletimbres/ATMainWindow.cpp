//
//  ATMainWindow.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATMainWindow.h"
#include "ATSynthesizer.h"
#include "ATKeyboardInputView.h"
#include "Utils.h"
#include "ATKeypadView.h"
#include <tcw/twSDLDC.h>
#include "ATTabView.h"
#include <tcw/twMsgBox.h>
#include "ATCompactWindow.h"
#include "ATInstrumentPane.h"
#include "ATReverbPane.h"
#include "ATChorusPane.h"
#include "main.h"

#include "xpms/KeypadIcon.xpm"
#include "xpms/KeyboardIcon.xpm"
#include "xpms/ProcessorIcon.xpm"

enum{
    IdKeypadOctaveUp=1,
    IdKeypadOctaveDown,
    IdKeyboardOctaveUp,
    IdKeyboardOctaveDown,
    IdMinimize,
    IdClose
};

ATMainWindow::ATMainWindow(){
    twRect rt;
    m_keyboardInput=new ATKeyboardInputView();
    
    m_keypad=new ATKeypadView();
    m_keypad->setRect(twRect(0, 320-91, 480, 91));
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
    
    m_mainTab=new ATTabView();
    m_mainTab->setParent(this);
    m_mainTab->setRect(rectForMainTab());
    m_mainTab->setTabCount(3);
    {
        delete m_mainTab->tabAtIndex(0);
        m_mainTab->setTabAtIndex(0, new ATInstrumentPane());
        twWnd *tab=m_mainTab->tabAtIndex(0);
        tab->setTitle(L"Instrument");
    }
    {
        delete m_mainTab->tabAtIndex(1);
        m_mainTab->setTabAtIndex(1, new ATChorusPane());
        twWnd *tab=m_mainTab->tabAtIndex(1);
        tab->setTitle(L"Insertion Effect");
    }
    {
        delete m_mainTab->tabAtIndex(2);
        m_mainTab->setTabAtIndex(2, new ATReverbPane());
        twWnd *tab=m_mainTab->tabAtIndex(2);
        tab->setTitle(L"Reverb");
    }
    m_mainTab->setRightMargin(28);
    
    m_closeButton.setParent(m_mainTab);
    m_closeButton.setTitle(L"x");
    m_closeButton.setId(IdClose);
    m_closeButton.setRect(twRect(rectForMainTab().w
                                 -13,
                                 0, 13, 13));
    m_closeButton.show();
    
    m_minimizeButton.setParent(m_mainTab);
    m_minimizeButton.setTitle(L"-");
    m_minimizeButton.setId(IdMinimize);
    m_minimizeButton.setRect(twRect(rectForMainTab().w
                                 -13*2-1,
                                 0, 13, 13));
    m_minimizeButton.show();
    
    m_mainTab->show();
    
    m_processorIcon=twSDLDC::loadFromXPM(ProcessorIcon);
    
    m_processorLoadTimer=new twTimerWithInvocation
    (new twNoArgumentMemberFunctionInvocation<ATMainWindow>
     (this, &ATMainWindow::updateProcessorLoad));
    m_processorLoadTimer->setInterval(500);
    m_processorLoadTimer->addToEvent(tw_event);
}

ATMainWindow::~ATMainWindow(){
    m_closeButton.setParent(NULL);
    m_minimizeButton.setParent(NULL);
    delete m_keyboardInput;
    delete m_keypad;
    delete m_mainTab;
    delete m_keypadIcon;
    delete m_keyboardIcon;
    delete m_processorIcon;
}

twRect ATMainWindow::rectForAbout() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+251, keypadRect.y-13, 114, 13);
}

twRect ATMainWindow::rectForMainTab() const{
    twRect keypadRect=m_keypad->getRect();
    return twRect(1, 1, 478, keypadRect.y-15);
}

twRect ATMainWindow::rectForWaveform() const{
    twRect keypadRect=m_keypad->getRect();
    return twRect(keypadRect.x+238, keypadRect.y-53, 200, 53);
}

void ATMainWindow::updateWaveform(){
    invalidateClientRect(rectForWaveform());
}

#pragma mark - Processor Load

twRect ATMainWindow::rectForProcessorLoad() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+131, keypadRect.y-13, 114, 13);
}

void ATMainWindow::updateProcessorLoad(){
  //updateWaveform();
    invalidateClientRect(rectForProcessorLoad());
}

#pragma mark - Keypad Management

twRect ATMainWindow::rectForKeypadOctaveEditor() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+1, keypadRect.y-13, 60, 13);
}

twRect ATMainWindow::rectForKeypadIcon() const{
    twRect octaveEditorRect=rectForKeypadOctaveEditor();
    return twRect(octaveEditorRect.x, octaveEditorRect.y,
                  13,13);
}

void ATMainWindow::updateKeypadOctaveEditor(){
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

twRect ATMainWindow::rectForKeyboardOctaveEditor() const{
    twRect keypadRect=m_keypad->getRect();
    keypadRect.x+=4;
    return twRect(keypadRect.x+66, keypadRect.y-13, 60, 13);
}

twRect ATMainWindow::rectForKeyboardIcon() const{
    twRect octaveEditorRect=rectForKeyboardOctaveEditor();
    return twRect(octaveEditorRect.x, octaveEditorRect.y,
                  13,13);
}

void ATMainWindow::updateKeyboardOctaveEditor(){
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

void ATMainWindow::clientMouseDown(const twPoint &, twMouseButton){
   
   /*
    int ky=20+(rand()%12);
    
    for(int i=0;i<7;i++)
    synth->note(ky+12*i, 32, 200-i*20);
    */
    
}

void ATMainWindow::clientMouseMove(const twPoint &){
    
}

void ATMainWindow::clientMouseUp(const twPoint &, twMouseButton){
    
}

void ATMainWindow::clientKeyDown(const std::wstring &k){
    if(k==L"Escape"){
        if(SDL_GetModState()&KMOD_CAPS){
            if(twMsgBox(NULL, L"Are you sure to exit Agiletimbres?",
                        twMBB_okCancel, L"Exit", true)==twDR_ok){
                exit(0);
            }
        }else{
            gotoCompactWindow();
            return;
        }
        /**/
    }
    m_keyboardInput->clientKeyDown(k);
   
}

void ATMainWindow::clientKeyUp(const std::wstring &k){
    m_keyboardInput->clientKeyUp(k);
}

void ATMainWindow::clientPaint(const twPaintStruct &p){
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
   
/*
     if(rectForWaveform() && p.paintRect){
        twRect rt=rectForWaveform();
         ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
        dc->fillRect(twRGB(128, 128, 128), 
                     rt);
        
	short *wave=synth->waveform();

	for(int i=0;i<1024;i++){
	  int x,y;
	  x=rt.x;
	  x+=(rt.w*i)>>10;
	  y=wave[i];
	  y+=32768;
	  y=((int)rt.h*y)>>16;
	  y+=rt.y;
	  dc->fillRect(0xffffff,twRect(x,y,1,1));
	}
        
      
    }*/
}

bool ATMainWindow::clientHitTest(const twPoint &pt) const{
    return true;
}

void ATMainWindow::setRect(const twRect &rt){
    twWnd::setRect(rt);
}

#include "TXBiquadFilter.h"
void ATMainWindow::command(int cmdId){
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
    }else if(cmdId==IdClose){
        if(twMsgBox(NULL, L"Are you sure to exit Agiletimbres?",
                    twMBB_okCancel, L"Exit", true)==twDR_ok){
            exit(0);
        }
    }else if(cmdId==IdMinimize){
        gotoCompactWindow();
        /*
        int32_t *buf1;
        int32_t *buf2;
        buf1=new int32_t[65536];
        buf2=new int32_t[65536];
        
        for(int i=0;i<65536;i++){
            buf2[i]=(rand()&0xffff)-0x8000;
        }
        
        TXBiquadFilter flt=TXBiquadFilter::highShelfFilter
        (44100.f, 8000.f, 5.f, .3f);
        
        twTicks ot=twGetTicks();
        int evals=0;
        while((twGetTicks()-ot)<3000){
            flt.applyMonoOutplace(buf1, buf2, 65536);
            evals++;
        }
        
        float speed; // sample/sec
        speed=(float)evals/3.f*65536.f;
        twMsgBox(NULL, NHFormatStd(L"Outplace Mono: %f samples/sec",
                                   speed), 
                 twMBB_ok, L"Biquad Bench");
        
        delete[] buf1;
        delete[]buf2;*/
    }
}

#pragma mark - Compaction

void ATMainWindow::gotoCompactWindow(){
    ATCompactWindow *cw=new ATCompactWindow(this);
    
    // stop already playing sounds.
    m_keypad->onMouseUp(twPoint(0,0), twMB_left);
    m_keyboardInput->allNotesOff();
    
    cw->setKeypadBaseNote(m_keypad->baseNode());
    cw->setKeyboardInputBaseNote(m_keyboardInput->baseNode());
    twSetDesktop(cw);
    tw_app->setFullScreen(false);
    tw_app->setScrSize(twSize(460, 95));
    
    tw_event->invokeOnMainThread
    (new twNoArgumentMemberFunctionInvocation<twWnd>
     ((twWnd *)cw, (void(twWnd::*)())&twWnd::invalidate));

    
}
void ATMainWindow::restoreFromCompactWindow(){
    ATCompactWindow *cw=dynamic_cast<ATCompactWindow *>(twGetDesktop());
    assert(cw);
    
    m_keypad->setBaseNote(cw->keypadBaseNote());
    m_keyboardInput->setBaseNote(cw->keyboardInputBaseNote());
    
    updateKeypadOctaveEditor();
    updateKeyboardOctaveEditor();
    
    twSetDesktop(this);
    
#ifdef AT_EMBEDDED
    tw_app->setFullScreen(true);
#else
    tw_app->setFullScreen(false);
#endif
    tw_app->setScrSize(twSize(480, 320));
    
    tw_event->invokeOnMainThread
    (new twNoArgumentMemberFunctionInvocation<twWnd>
     ((twWnd *)this, (void(twWnd::*)())&twWnd::invalidate));
}
