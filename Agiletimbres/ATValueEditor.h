//
//  ATValueEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATValueEditor: public twWnd{
public:
    enum Mode{
        ModeLogarithm,
        ModeLinear,
        ModeInteger
    };
    enum DisplayMode{
        DisplayModeReal,
        DisplayModeScientific,
        DisplayModePrefix,
        DisplayModeInteger,
        DisplayModeDecibel,
		DisplayModePercent
    };
private:
    float m_value;
    float m_minValue;
    float m_maxValue;
    float m_epsilon;
    Mode m_mode;
    DisplayMode m_displayMode;
    bool m_drag;
    
    twPoint m_dragStartPos;
    float m_dragStartValue;
    
public:
    ATValueEditor();
    virtual ~ATValueEditor();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
    
    virtual void setMode(Mode);
    Mode mode() const{return m_mode;}
    
    virtual void setDisplayMode(DisplayMode);
    DisplayMode displayMode() const{return m_displayMode;}
    
    virtual void setValue(int);
    virtual void setValue(float);
    float value() const{return m_value;}
    int intValue() const;
    
    virtual void setMinValue(float);
    virtual void setMaxValue(float);
    virtual void setValueRange(float, float);
    float minValue() const{return m_minValue;}
    float maxValue() const{return m_maxValue;}
    
    float epsilon() const{return m_epsilon;}
    void setEpsilon(float e){m_epsilon=e;}
};
