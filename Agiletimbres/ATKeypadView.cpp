//
//  ATKeypadView.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATKeypadView.h"
#include "ATSynthesizer.h"

static const int g_blackKeysCountPerOctave=5;
static const int g_blackKeyPositions[]={
    1, 2, 4, 5, 6
};
static const int g_blackKeyNotes[]={
    1, 3, 6, 8, 10
};
static const int g_whiteKeysCountPerOctave=7;
static const int g_whiteKeyNotes[]={
    0, 2, 4, 5, 7, 9, 11
};

static const int g_keyForNote[]={
    0, // white
    0, // black
    1, // white
    1, // black
    2, // white
    3, // white
    2, // black
    4, // white
    3, // black
    5, // white
    4, // black
    6, // white
};

static const int g_isKeyForNoteBlack[]={
    false,
    true,
    false,
    true,
    false,
    false,
    true,
    false,
    true,
    false,
    true,
    false
};

static const twColor g_whiteKeyColor=twRGB(192,192,192);
static const twColor g_activeWhiteKeyColor=twRGB(200,90,0);
static const twColor g_blackKeyColor=twRGB(64,64,64);
static const twColor g_activeBlackKeyColor=twRGB(200,90,0);



ATKeypadView::ATKeypadView(){
    m_baseNote=48;
    m_whiteKeyWidth=18;
    m_blackKeyWidth=14;
    m_playingNote=-1;
}

ATKeypadView::~ATKeypadView(){
    
}

int ATKeypadView::whiteKeysCount() const{
    return (getRect().w+m_whiteKeyWidth-1)/
    m_whiteKeyWidth;
}

int ATKeypadView::octavesCount() const{
    return (whiteKeysCount()+g_whiteKeysCountPerOctave-1)/g_whiteKeysCountPerOctave;
}

int ATKeypadView::blackKeyHeight() const{
    return getRect().h*7/12;
}

int ATKeypadView::whiteKeyHeight() const{
    return getRect().h;
}

int ATKeypadView::noteAtPoint(const twPoint &pt) const{
    int oct=pt.x/(g_whiteKeysCountPerOctave*
                  m_whiteKeyWidth);
    
    if(pt.y<blackKeyHeight()){
        // check for black keys
        for(int i=0;i<g_blackKeysCountPerOctave;i++){
            int note=oct*12+m_baseNote;
            note+=g_blackKeyNotes[i];
            if(rectForNote(note)&&pt)
                return note;
        }
    }
    
    // check for white keys
    for(int i=0;i<g_whiteKeysCountPerOctave;i++){
        int note=oct*12+m_baseNote;
        note+=g_whiteKeyNotes[i];
        if(rectForNote(note)&&pt)
            return note;
    }
    
    return -1;
}

twRect ATKeypadView::rectForOctave(int oct) const{
    int octWidth=m_whiteKeyWidth*g_whiteKeysCountPerOctave;
    return twRect(octWidth*oct, 0,
                  octWidth, whiteKeyHeight());
}

twRect ATKeypadView::rectForNote(int note) const{
    int oct=(note-m_baseNote)/12;
    int noteInOct=note-m_baseNote-oct*12;
    
    twRect rt;
    
    if(g_isKeyForNoteBlack[noteInOct]){
        int keyIndex=g_keyForNote[noteInOct];
        rt.x=m_whiteKeyWidth*g_blackKeyPositions[keyIndex];
        rt.x-=m_blackKeyWidth/2;
        rt.y=0;
        rt.w=m_blackKeyWidth;
        rt.h=blackKeyHeight();
    }else{
        int keyIndex=g_keyForNote[noteInOct];
        rt.x=m_whiteKeyWidth*keyIndex;
        rt.y=0;
        rt.w=m_whiteKeyWidth;
        rt.h=whiteKeyHeight();
    }
    
    rt.x+=oct*g_whiteKeysCountPerOctave*
    m_whiteKeyWidth;
    
    return rt;
}

void ATKeypadView::clientPaint(const twPaintStruct &p){
    int octs=octavesCount();
    twDC *dc=p.dc;
    for(int oct=0;oct<octs;oct++){
        if(!(rectForOctave(oct)&&p.paintRect))
            continue;
        
        int octNote=12*oct+m_baseNote;
        
        // render white keys
        for(int i=0;i<g_whiteKeysCountPerOctave;i++){
            int note=octNote;
            note+=g_whiteKeyNotes[i];
            twRect rt=rectForNote(note);
            if(rt&&p.paintRect){
                dc->drawRect(0, twRect(rt.x, rt.y,
                                       rt.w-1,
                                       rt.h-1));
                dc->fillRect((m_playingNote==note)?
                             g_activeWhiteKeyColor:
                             g_whiteKeyColor,
                             rt.inflate(-1));
                
            }
        }
        
        // render black keys
        for(int i=0;i<g_blackKeysCountPerOctave;i++){
            int note=octNote;
            note+=g_blackKeyNotes[i];
            twRect rt=rectForNote(note);
            if(rt&&p.paintRect){
                dc->drawRect(0, twRect(rt.x, rt.y,
                                       rt.w-1,
                                       rt.h-1));
                dc->fillRect((m_playingNote==note)?
                             g_activeBlackKeyColor:
                             g_blackKeyColor,
                             rt.inflate(-1));
                
            }
        }
        
        
    }
    
    {
        twRect rt=getClientRect();
        rt.x=0; rt.y=0;
        dc->drawRect(0, twRect(rt.x, rt.y,
                               rt.w-1, rt.h-1));
        
        // draw corners.
        rt=rt.inflate(-1);
        
        dc->fillRect(0, twRect(rt.x, rt.y,
                               3, 1));
        dc->fillRect(0, twRect(rt.x, rt.y,
                               1, 3));
        
        dc->fillRect(0, twRect(rt.x+rt.w-3, rt.y,
                               3, 1));
        dc->fillRect(0, twRect(rt.x+rt.w-1, rt.y,
                               1, 3));
        
        dc->fillRect(0, twRect(rt.x, rt.y+rt.h-1,
                               3, 1));
        dc->fillRect(0, twRect(rt.x, rt.y+rt.h-3,
                               1, 3));
        
        dc->fillRect(0, twRect(rt.x+rt.w-3, rt.y+rt.h-1,
                               3, 1));
        dc->fillRect(0, twRect(rt.x+rt.w-1, rt.y+rt.h-3,
                               1, 3));
      
        
    }
}

bool ATKeypadView::clientHitTest(const twPoint &pt) const{
    return true;
}

void ATKeypadView::clientMouseDown(const twPoint &pt,
                                   twMouseButton mb){
    if(mb!=twMB_left)
        return;
    
    twSetCapture(this);
    
    if(m_playingNote!=-1){
        noteOff(m_playingNote);
        invalidateClientRect(rectForNote(m_playingNote));
        m_playingNote=-1;
    }
    
    m_playingNote=noteAtPoint(pt);
    if(m_playingNote!=-1){
        noteOn(m_playingNote);
        invalidateClientRect(rectForNote(m_playingNote));
    }
}

void ATKeypadView::clientMouseMove(const twPoint &pt){
    twPoint p=pt;
    if(m_playingNote!=-1){
        if(p.y<0)
            p.y=0;
        if(p.y>=whiteKeyHeight())
            p.y=whiteKeyHeight()-1;
        
        int newNote=noteAtPoint(p);
        if(newNote==-1)
            return;
        
        if(newNote!=m_playingNote){
            noteOff(m_playingNote);
            invalidateClientRect(rectForNote(m_playingNote));
            SDL_Delay(10);
            m_playingNote=newNote;
            noteOn(m_playingNote);
            invalidateClientRect(rectForNote(m_playingNote));
        }
    }
}

void ATKeypadView::clientMouseUp(const twPoint &pt,
                                 twMouseButton mb){
    if(m_playingNote!=-1){
        noteOff(m_playingNote);
        invalidateClientRect(rectForNote(m_playingNote));
        m_playingNote=-1;
    }
    
    twReleaseCapture();
}

void ATKeypadView::setRect(const twRect &rt){
    twWnd::setRect(rt);
}

void ATKeypadView::command(int cmdId){
    
}

void ATKeypadView::setBaseNote(int note){
    if(m_baseNote==note)
        return;
    
    if(m_playingNote!=-1){
        noteOff(m_playingNote);
        m_playingNote+=note;
        if(m_playingNote>=0 && m_playingNote<128){
            noteOn(m_playingNote);
        }else{
            m_playingNote=-1;
            invalidateClientRect();
        }
    }
    m_baseNote=note;
}

void ATKeypadView::noteOn(int k){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    synth->noteOn(k, 127);
}
void ATKeypadView::noteOff(int k){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    synth->noteOff(k, 100);
}
