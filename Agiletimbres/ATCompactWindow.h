//
//  ATCompactWindow.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/16/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once


#include "StdAfx.h"
#include "ATListView.h"

class ATKeyboardInputView;
class ATKeypadView;
class ATTabView;
class ATMainWindow;

class ATCompactWindow: public twWnd{
    twTimer *m_processorLoadTimer;
    
    ATKeyboardInputView *m_keyboardInput;
    ATKeypadView *m_keypad;
    
    twButton m_restoreButton;
    
    twDC *m_keypadIcon;
    twButton m_keypadOctaveUpButton;
    twButton m_keypadOctaveDownButton;
    twLabel m_keypadOctaveLabel;
    twRect rectForKeypadOctaveEditor() const;
    twRect rectForKeypadIcon() const;
    void updateKeypadOctaveEditor();
    
    twDC *m_keyboardIcon;
    twButton m_keyboardOctaveUpButton;
    twButton m_keyboardOctaveDownButton;
    twLabel m_keyboardOctaveLabel;
    twRect rectForKeyboardOctaveEditor() const;
    twRect rectForKeyboardIcon() const;
    void updateKeyboardOctaveEditor();
    
    twDC *m_processorIcon;
    twRect rectForProcessorLoad() const;
    twRect rectForAbout() const;
    twRect rectForRestoreButton() const;
    
    ATMainWindow *m_originalMainWindow;
    
    void updateProcessorLoad();
    void gotoMainWindow();
public:
    ATCompactWindow(ATMainWindow*);
    virtual ~ATCompactWindow();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
	
	virtual void clientKeyDown(const std::wstring&);
    virtual void clientKeyPress(wchar_t);
	virtual void clientKeyUp(const std::wstring&);
	
    
	virtual void setRect(const twRect&);
	virtual void command(int);
    
    void setKeyboardInputBaseNote(int);
    void setKeypadBaseNote(int);
    int keyboardInputBaseNote() const;
    int keypadBaseNote() const;
};

