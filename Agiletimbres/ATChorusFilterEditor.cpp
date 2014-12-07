//
//  ATChorusFilterEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATChorusFilterEditor.h"
#include "ATSynthesizer.h"
#include "Utils.h"

enum{
    IdMinDelayTime=1,
    IdSpread,
    IdGain,
    IdFrequency
};

ATChorusFilterEditor::ATChorusFilterEditor(){
    m_plugin=NULL;
    
    m_gain.setParent(this);
    m_gain.setRect(twRect(59, 17, 40, 13));
    m_gain.setValueRange(0.f, 1.f);
    m_gain.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_gain.setMode(ATValueEditor::ModeLogarithm);
    m_gain.setId(IdGain);
    m_gain.show();
    
    m_minDelayTime.setParent(this);
    m_minDelayTime.setRect(twRect(31, 53, 60, 13));
    m_minDelayTime.setValueRange(0.f, .08f);
    m_minDelayTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
    m_minDelayTime.setMode(ATValueEditor::ModeLogarithm);
    m_minDelayTime.setId(IdMinDelayTime);
    m_minDelayTime.setEpsilon(1.e-6f);
    m_minDelayTime.show();
    
    m_spread.setParent(this);
    m_spread.setRect(twRect(148, 53, 60, 13));
    m_spread.setValueRange(0.0f, .08f);
    m_spread.setDisplayMode(ATValueEditor::DisplayModePrefix);
    m_spread.setMode(ATValueEditor::ModeLogarithm);
    m_spread.setId(IdSpread);
    m_spread.setEpsilon(1.e-6f);
    m_spread.show();
    
    m_frequency.setParent(this);
    m_frequency.setRect(twRect(173, 17, 40, 13));
    m_frequency.setValueRange(.02f, 100.f);
    m_frequency.setDisplayMode(ATValueEditor::DisplayModeReal);
    m_frequency.setMode(ATValueEditor::ModeLogarithm);
    m_frequency.setId(IdFrequency);
    m_frequency.show();
    
}

ATChorusFilterEditor::~ATChorusFilterEditor(){
    
}

twRect ATChorusFilterEditor::rectForDeltaGraph() const{
    return twRect(4, 70, 312, 200-70-4);
}


#pragma mark - Main Geometry

static float preferredGridInterval(float range){
    if(range<1.e-6f)
        return range;
    if(range<.1f)
        return preferredGridInterval(range*10.f)*.1f;
    if(range<.5f){
        return .1f;
    }else if(range<1.f){
        return .2f;
    }else if(range<2.f){
         return .5f;
    }else if(range<5.f){
        return 1.f;
    }else if(range<10.f){
        return 2.f;
    }else if(range<20.f){
        return 5.f;
    }else{
        return preferredGridInterval(range*.1f)*10.f;
    }
}

void ATChorusFilterEditor::clientPaint(const twPaintStruct &p){
    twRect r;
    const twFont *font=getFont();
    std::wstring str;
    twSize size;
    twColor groupColor=twRGB(128, 128, 128);
    twColor lineColor=groupColor;
    twColor borderColor=twRGB(192, 192, 192);
    twColor graphColor=twRGB(128, 220, 32);
    twColor gridColor=twRGB(64, 64, 64);
    twColor labelColor=tw_curSkin->getWndTextColor();
    twDC *dc=p.dc;
    
    font->render(dc, groupColor, twPoint(1,2),
                 L"Chorus");
    font->render(dc, labelColor, twPoint(4,18),
                 L"Input Gain");
    font->render(dc, labelColor, twPoint(100,18),
                 L"db");
    font->render(dc, labelColor, twPoint(120,18),
                 L"Frequency");
    font->render(dc, labelColor, twPoint(214,18),
                 L"Hz");
    
    dc->drawLine(lineColor, twPoint(1, 34), twPoint(318, 34));
    
    font->render(dc, groupColor, twPoint(1,38),
                 L"Delay Time");
    
    font->render(dc, labelColor, twPoint(4,54),
                 L"Base");
    font->render(dc, labelColor, twPoint(92,54),
                 L"s");
    font->render(dc, labelColor, twPoint(110,54),
                 L"Spread");
    font->render(dc, labelColor, twPoint(210,54),
                 L"s");
    
    
    r=rectForDeltaGraph();
    if(r&&p.paintRect){
        dc->fillRect(borderColor, twRect(r.x+1, r.y,
                                         r.w-2, 1));
        dc->fillRect(borderColor, twRect(r.x+1, r.y+r.h-1,
                                         r.w-2, 1));
        dc->fillRect(borderColor, twRect(r.x, r.y+1,
                                         1, r.h-2));
        dc->fillRect(borderColor, twRect(r.x+r.w-1, r.y+1,
                                         1, r.h-2));
        
        r=r.inflate(-1);
        twRect oldClipRect=dc->getClipRect();
        dc->addClipRect(r);
        
        
        float period=1.f/m_parameter.frequency;
        float minDelayTime=m_parameter.baseDelayTime;
        float maxDelayTime=minDelayTime+m_parameter.deltaDelayTime;
        
        float timeRange=(1.f+period*4.f)*.5f;
        float delayRange=(50.e-3f+maxDelayTime*2.f)*.5f;
        float timeScale=(float)(r.w-1)/timeRange;
        float delayScale=(float)(r.h-1)/delayRange;
        
        float halfPeriod=period*.5f;
        
        int minDelayY=r.y+r.h-1-(int)(minDelayTime*delayScale);
        int maxDelayY=r.y+r.h-1-(int)(maxDelayTime*delayScale);
        
        float timeGridInterval=preferredGridInterval(timeRange*.5f);
        float delayGridInterval=preferredGridInterval(delayRange*.5f);
        
        for(float i=0;i<timeRange;i+=timeGridInterval){
            int x=r.x+(int)(i*timeScale);
            dc->drawLine(gridColor, twPoint(x, r.y), twPoint(x, r.y+r.h));
        }
        
        for(float i=0;i<delayRange;i+=delayGridInterval){
            int y=r.y+r.h-1-(int)(i*delayScale);
            dc->drawLine(gridColor, twPoint(r.x, y), 
                         twPoint(r.x+r.w, y));
        }
        
        if(delayRange>timeRange){
            int y=r.y+r.h-1-(int)(timeRange*delayScale);
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h-1), 
                         twPoint(r.x+r.w-1, y));
        }else{
            int x=(int)(delayRange*timeScale);
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h-1), 
                         twPoint(x, r.y));
        }
        
        for(float time=0.f;time<timeRange;time+=period){
            twPoint point, lastPoint;
            
            point.x=r.x+(int)(time*timeScale);
            point.y=minDelayY;
            
            lastPoint=point;
            
            point.x=r.x+(int)((time+halfPeriod)*timeScale);
            point.y=maxDelayY;
        
            dc->drawLine(graphColor, lastPoint, point);
            lastPoint=point;
            
            point.x=r.x+(int)((time+period)*timeScale);
            point.y=minDelayY;
            
            dc->drawLine(graphColor, lastPoint, point);
            
        }
        
        
        str=NHFormatStd(L"%dms", (int)(delayRange*1000.f));
        font->render(dc, labelColor, 
                     twPoint(r.x+3, r.y+3), str);
        
        
        if(timeRange>=1.f)
            str=NHFormatStd(L"%.2fs", timeRange);
        else
            str=NHFormatStd(L"%dms", (int)(timeRange*1000.f));
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(r.x+r.w-3-size.w, r.y+r.h-3-size.h), str);
        
        
        
        dc->setClipRect(oldClipRect);
    }
}

void ATChorusFilterEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXChorusFilter *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXChorusFilter *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATChorusFilterEditor::loadParameters(){
    m_frequency.setValue(m_parameter.frequency);
    m_gain.setValue(m_parameter.inGain);
    m_minDelayTime.setValue(m_parameter.baseDelayTime);
    m_spread.setValue(m_parameter.deltaDelayTime);
}

void ATChorusFilterEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATChorusFilterEditor::command(int id){
    if(id==IdFrequency){
        m_parameter.frequency=m_frequency.value();
        invalidateClientRect(rectForDeltaGraph());
        setParameters();
    }else if(id==IdGain){
        m_parameter.inGain=m_gain.value();
        setParameters();
    }else if(id==IdMinDelayTime){
        m_parameter.baseDelayTime=m_minDelayTime.value();
         invalidateClientRect(rectForDeltaGraph());
        setParameters();
    }else if(id==IdSpread){
        m_parameter.deltaDelayTime=m_spread.value();
         invalidateClientRect(rectForDeltaGraph());
        setParameters();
    }
}

