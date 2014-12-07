//
//  ATKeyboardInputView.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATKeyboardInputView.h"
#include "ATSynthesizer.h"

ATKeyboardInputView::ATKeyboardInputView(){
    for(size_t i=0;i<128;i++)
        m_isNoteOn[i]=false;
    
    m_keyMap[L"z"]=0;
    m_keyMap[L"s"]=1;
    m_keyMap[L"x"]=2;
    m_keyMap[L"d"]=3;
    m_keyMap[L"c"]=4;
    m_keyMap[L"v"]=5;
    m_keyMap[L"g"]=6;
    m_keyMap[L"b"]=7;
    m_keyMap[L"h"]=8;
    m_keyMap[L"n"]=9;
    m_keyMap[L"j"]=10;
    m_keyMap[L"m"]=11;
    m_keyMap[L"-"]=12;
    
     m_baseNote=60;
}

ATKeyboardInputView::~ATKeyboardInputView(){
    allNotesOff();
    
}

int ATKeyboardInputView::noteForKey(const std::wstring &key){
    std::map<std::wstring, int>::iterator it=m_keyMap.find(key);
    if(it==m_keyMap.end())
        return -1;
    return it->second+m_baseNote;
}

void ATKeyboardInputView::clientKeyDown(const std::wstring &key){
    int note=noteForKey(key);
    if(note==-1)
        return;
    
    if(!m_isNoteOn[note]){
        
        ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
        synth->noteOn(note, 127);
        
        m_isNoteOn[note]=true;
    }
}

void ATKeyboardInputView::clientKeyUp(const std::wstring &key){
    int note=noteForKey(key);
    if(note==-1)
        return;
    if(m_isNoteOn[note]){
        
        ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
        synth->noteOff(note, 127);
        
        m_isNoteOn[note]=false;
    }
}

void ATKeyboardInputView::allNotesOff(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    for(int i=0;i<128;i++){
        if(m_isNoteOn[i]){
            synth->noteOff(i, 127);
            m_isNoteOn[i]=false;
        }
    }
}

void ATKeyboardInputView::setBaseNote(int note){
    if(note==m_baseNote)
        return;
    
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    
    // shift already playing notes.
    for(int i=0;i<128;i++){
        if(m_isNoteOn[i]){
            synth->noteOff(i, 127);
            
            int newNote=i+note-m_baseNote;
            if(newNote>=0 && newNote<128)
                synth->noteOn(newNote, 127);
        }
    }
    
    if(note>m_baseNote){
        for(int i=127;i>=0;i--){
            int oldNote=i-note+m_baseNote;
            if(oldNote>=0 && oldNote<128)
                m_isNoteOn[i]=m_isNoteOn[oldNote];
            else
                m_isNoteOn[i]=false;
        }
    }else{
        for(int i=0;i<128;i++){
            int oldNote=i-note+m_baseNote;
            if(oldNote>=0 && oldNote<128)
                m_isNoteOn[i]=m_isNoteOn[oldNote];
            else
                m_isNoteOn[i]=false;
        }
    }
    
    m_baseNote=note;
}



