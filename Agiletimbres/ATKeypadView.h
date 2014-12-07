//
//  ATKeypadView.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATKeypadView: public twWnd{
    int m_whiteKeyWidth;
    int m_blackKeyWidth;
    int m_baseNote;
    
    int m_playingNote;
    
    int whiteKeysCount() const;
    int octavesCount() const;
    
    int blackKeyHeight() const;
    int whiteKeyHeight() const;
    
    int noteAtPoint(const twPoint&) const;
    twRect rectForNote(int) const;
    twRect rectForOctave(int) const;
    
    void noteOn(int);
    void noteOff(int);
    
public:
    ATKeypadView();
    virtual ~ATKeypadView();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
    
	virtual void setRect(const twRect&);
	virtual void command(int);
  
    int baseNode() const{return m_baseNote;}
    void setBaseNote(int);
};
