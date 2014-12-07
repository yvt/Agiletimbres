//
//  ATValueEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATValueEditor.h"
#include "Utils.h"

ATValueEditor::ATValueEditor(){
    twWndStyle style=getStyle();
    style.border=twBS_panel;
    setStyle(style);
    m_drag=false;
    m_mode=ModeLinear;
    m_displayMode=DisplayModeReal;
    m_value=0.f;
    m_minValue=-10.f;
    m_maxValue=10.f;
    m_epsilon=1.e-3f;
}

ATValueEditor::~ATValueEditor(){
    
}

void ATValueEditor::clientPaint(const twPaintStruct &p){
    twColor bgColor, fgColor;
    
    bgColor=tw_curSkin->getWndColor();
    fgColor=tw_curSkin->getWndTextColor();
    
    if(m_drag){
        bgColor=tw_curSkin->getSelectionColor();
        fgColor=tw_curSkin->getSelectionTextColor();
    }
    
    twDC *dc=p.dc;
    std::wstring str;
    const twFont *font=getFont();
    dc->fillRect(bgColor, p.boundRect);
    
    switch(m_displayMode){
        case DisplayModeInteger:
            str=NHFormatStd(L"%d", intValue());
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
		case DisplayModePercent:
			// TODO: correct rounding
            str=NHFormatStd(L"%d", (int)(m_value*100.f+.5f));
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
        case DisplayModeDecibel:
            if(m_value<=m_epsilon){
                str=L"-inf";
            }else{
                str=NHFormatStd(L"%d", (int)(20.f*logf(m_value)/logf(10.f)+.5f));
            }
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
        case DisplayModeReal:
            if(fabsf(m_value)<m_epsilon){
                if(m_value<-m_epsilon*0.2f){
                    str=L"-0.00";
                }else{
                    str=L"0.00";
                }
            }else{
                str=NHFormatStd(L"%.02f", m_value);
            }
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
        case DisplayModeScientific:
            str=NHFormatStd(L"%.02e", m_value);
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
        case DisplayModePrefix:
            /*if(fabsf(m_value)<5.e-5f){
                str=NHFormatStd(L"%.02fu", (m_value*1.e+6f));
            }else if(fabsf(m_value)<5.e-4f){
                str=NHFormatStd(L"%.01fu", (m_value*1.e+6f));
            }else if(fabsf(m_value)<5.e-3f){
                str=NHFormatStd(L"%du", (int)(m_value*1.e+6f));
            }else */if(fabsf(m_value)<5.e-2f){
                str=NHFormatStd(L"%.03fm", (m_value*1.e+3f));
            }else if(fabsf(m_value)<5.e-1f){
                str=NHFormatStd(L"%.02fm", (m_value*1.e+3f));
            }else if(fabsf(m_value)<5.f){
                str=NHFormatStd(L"%.01fm", (m_value*1.e+3f));
            }else if(fabsf(m_value)<5.e+1f){
                str=NHFormatStd(L"%.03f", (m_value));
            }else if(fabsf(m_value)<5.e+2f){
                str=NHFormatStd(L"%.02f", (m_value));
            }else if(fabsf(m_value)<5.e+3f){
                str=NHFormatStd(L"%.01f", (m_value));
            }else if(fabsf(m_value)<5.e+4f){
                str=NHFormatStd(L"%.03fK", (m_value*1.e-3f));
            }else if(fabsf(m_value)<5.e+5f){
                str=NHFormatStd(L"%.02fK", (m_value*1.e-3f));
            }else if(fabsf(m_value)<5.e+6f){
                str=NHFormatStd(L"%.01fK", (m_value*1.e-3f));
            }else if(fabsf(m_value)<5.e+7f){
                str=NHFormatStd(L"%.03fM", (m_value*1.e-6f));
            }else if(fabsf(m_value)<5.e+8f){
                str=NHFormatStd(L"%.02fM", (m_value*1.e-6f));
            }else if(fabsf(m_value)<5.e+9f){
                str=NHFormatStd(L"%.01fM", (m_value*1.e-6f));
            }else if(fabsf(m_value)<5.e+10f){
                str=NHFormatStd(L"%.03fG", (m_value*1.e-9f));
            }else if(fabsf(m_value)<5.e+11f){
                str=NHFormatStd(L"%.02fG", (m_value*1.e-9f));
            }else{
                str=NHFormatStd(L"%.01fG", (m_value*1.e-9f));
            }
            
            font->render(dc, fgColor, twPoint(0,0),
                         str);
            break;
    }
}

bool ATValueEditor::clientHitTest(const twPoint &pt) const{
    return true;
}

void ATValueEditor::clientMouseDown
(const twPoint &pt, twMouseButton mb){
    if(mb!=twMB_left)
        return;
    
    m_drag=true;
    m_dragStartPos=pt;
    m_dragStartValue=m_value;
    
    twSetCapture(this);
    
    invalidateClientRect();
    
}

void ATValueEditor::clientMouseMove(const twPoint &pt){
    if(m_drag){
        twPoint deltaPos=pt-m_dragStartPos;
        int deltaStep=deltaPos.x-deltaPos.y;
        
        switch(m_mode){
            case ModeLogarithm:
                if(m_minValue<m_epsilon){
                    // do semi-logarithm.
                    m_value=m_dragStartValue;
                    if(fabsf(m_value)<m_epsilon){
                        m_value=m_epsilon*.99f*((m_value<-m_epsilon*.1f)?-1.f:1.f);
                    }
                    if(m_value<0.f)
                        deltaStep=-deltaStep;
                    m_value*=powf(2.f, (float)deltaStep/32.f);
                    if(fabsf(m_value)<m_epsilon*0.5f){
                        m_value=-m_value*2.f;
                        m_value=m_epsilon*m_epsilon/m_value;
                    }else if(fabsf(m_value)<m_epsilon*0.999f){
                        m_value=((m_value>0.f)?1.f:-1.f)*1.e-20f;
                    }
                }else{
                    m_value=m_dragStartValue;
                    m_value*=powf(2.f, (float)deltaStep/32.f);
                }
                break;
            case ModeLinear:
                m_value=m_dragStartValue;
                m_value+=(m_maxValue-m_minValue)*
                (float)deltaStep/512.f;
                break;
            case ModeInteger:
                m_value=m_dragStartValue;
                m_value+=(float)deltaStep/8.f;
                m_value=floorf(m_value+.5f);
                break;
        }
        
        if(m_value<m_minValue)
            m_value=m_minValue;
        if(m_value>m_maxValue)
            m_value=m_maxValue;
        
        sendCmdToParent();
        
        invalidateClientRect();
    }
   
    
}

void ATValueEditor::clientMouseUp
(const twPoint & pt, twMouseButton mb){
    m_drag=false;
    twReleaseCapture();
    invalidateClientRect();
}

void ATValueEditor::setMode(ATValueEditor::Mode mode){
    m_mode=mode;
}

void ATValueEditor::setDisplayMode(ATValueEditor::DisplayMode m){
    m_displayMode=m;
    invalidateClientRect();
}

void ATValueEditor::setMinValue(float v){
    m_minValue=v;
}

void ATValueEditor::setMaxValue(float v){
    m_maxValue=v;
}

void ATValueEditor::setValueRange(float a, float b){
    setMinValue(a);
    setMaxValue(b);
}

void ATValueEditor::setValue(float value){
    if(value==m_value)
        return;
    m_value=value;
    invalidateClientRect();
}

void ATValueEditor::setValue(int value){
    setValue((float)value);
}

int ATValueEditor::intValue() const{
    return (int)floorf(m_value+.5f);
}
