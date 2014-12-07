//
//  ATKeyboardInputView.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATKeyboardInputView: public twWnd{
    bool m_isNoteOn[128];
    int m_baseNote;
    std::map<std::wstring, int> m_keyMap;
    int noteForKey(const std::wstring&);
public:
    ATKeyboardInputView();
    virtual ~ATKeyboardInputView();
    virtual void clientKeyDown(const std::wstring&);
    virtual void clientKeyUp(const std::wstring&);
    
    int baseNode() const{return m_baseNote;}
    void setBaseNote(int);
    
    void allNotesOff();
};
