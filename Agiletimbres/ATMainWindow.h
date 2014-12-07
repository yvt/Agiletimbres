//
//  ATMainWindow.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"
#include "ATListView.h"

class ATKeyboardInputView;
class ATKeypadView;
class ATTabView;

class ATMainWindow: public twWnd{
    twTimer *m_processorLoadTimer;
    
    ATKeyboardInputView *m_keyboardInput;
    ATKeypadView *m_keypad;
    ATTabView *m_mainTab;
    
    twButton m_minimizeButton;
    twButton m_closeButton;
    
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
    twRect rectForWaveform() const;
    twRect rectForMainTab() const;
    
    
    void updateProcessorLoad();
    void updateWaveform();
public:
    ATMainWindow();
    virtual ~ATMainWindow();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
	
	virtual void clientKeyDown(const std::wstring&);
	virtual void clientKeyUp(const std::wstring&);
	
    
	virtual void setRect(const twRect&);
	virtual void command(int);
    
    void gotoCompactWindow();
    void restoreFromCompactWindow();
    
};
