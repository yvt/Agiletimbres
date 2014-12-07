//
//  ATPhaserFilterEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/3/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//


#include "ATPhaserFilterEditor.h"
#include "ATSynthesizer.h"
#include "ATDropdownList.h"
#include "Utils.h"

enum{
    IdDepth=1,
    IdStages,
    IdFrequency,
    IdFeedback,
	IdCenterFrequency,
	IdRange
};

ATPhaserFilterEditor::ATPhaserFilterEditor(){
    m_plugin=NULL;
    
    m_depth.setParent(this);
    m_depth.setRect(twRect(39, 17, 40, 13));
    m_depth.setValueRange(0.f, 1.f);
    m_depth.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_depth.setMode(ATValueEditor::ModeLinear);
    m_depth.setId(IdDepth);
    m_depth.show();
    
    m_stages.setParent(this);
    m_stages.setRect(twRect(220, 17, 40, 13));
    m_stages.setId(IdStages);
	m_stages.items().push_back(L"4");
	m_stages.items().push_back(L"6");
	m_stages.items().push_back(L"8");
	m_stages.items().push_back(L"10");
	m_stages.items().push_back(L"12");
    m_stages.show();
    
    m_frequency.setParent(this);
    m_frequency.setRect(twRect(238, 53, 40, 13));
    m_frequency.setValueRange(0.0f, 20.f);
    m_frequency.setDisplayMode(ATValueEditor::DisplayModeReal);
    m_frequency.setMode(ATValueEditor::ModeLogarithm);
    m_frequency.setId(IdFrequency);
    m_frequency.setEpsilon(1.e-6f);
    m_frequency.show();
    
    m_feedback.setParent(this);
    m_feedback.setRect(twRect(156, 17, 40, 13));
    m_feedback.setValueRange(-1.f, 1.f);
    m_feedback.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_feedback.setMode(ATValueEditor::ModeLinear);
    m_feedback.setId(IdFeedback);
    m_feedback.show();
	
	m_centerFrequency.setParent(this);
    m_centerFrequency.setRect(twRect(43, 53, 40, 13));
    m_centerFrequency.setValueRange(100.f, 22050.f);
    m_centerFrequency.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_centerFrequency.setMode(ATValueEditor::ModeLogarithm);
    m_centerFrequency.setId(IdCenterFrequency);
    m_centerFrequency.show();
	
	m_range.setParent(this);
    m_range.setRect(twRect(144, 53, 40, 13));
    m_range.setValueRange(0.f, 22050.f);
    m_range.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_range.setMode(ATValueEditor::ModeLogarithm);
    m_range.setId(IdRange);
    m_range.show();
	
	
    
}

ATPhaserFilterEditor::~ATPhaserFilterEditor(){
    
}

twRect ATPhaserFilterEditor::rectForDeltaGraph() const{
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

void ATPhaserFilterEditor::clientPaint(const twPaintStruct &p){
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
                 L"Flanger");
    font->render(dc, labelColor, twPoint(4,18),
                 L"Depth");
    font->render(dc, labelColor, twPoint(81,18),
                 L"%");
    font->render(dc, labelColor, twPoint(106,18),
                 L"Feedback");
    font->render(dc, labelColor, twPoint(198,18),
                 L"%");
	font->render(dc, labelColor, twPoint(262,18),
                 L"stages");
    
    dc->drawLine(lineColor, twPoint(1, 34), twPoint(318, 34));
    
    font->render(dc, groupColor, twPoint(1,38),
                 L"Modulation");
    
    font->render(dc, labelColor, twPoint(4,54),
                 L"Center");
    font->render(dc, labelColor, twPoint(84,54),
                 L"Hz");
    font->render(dc, labelColor, twPoint(110,54),
                 L"Range");
    font->render(dc, labelColor, twPoint(185,54),
                 L"Hz");
	font->render(dc, labelColor, twPoint(215,54),
                 L"LFO");
    font->render(dc, labelColor, twPoint(279,54),
                 L"Hz");
    
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
        
        
        float period=1.f/m_parameter.lfoFrequency;
        float minDelayTime=m_parameter.minFrequency;
        float maxDelayTime=m_parameter.maxFrequency;
        
        float timeRange=(1.f+period*4.f)*.5f;
        float delayRange=22050.f;
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
        
        
        font->render(dc, labelColor, 
                     twPoint(r.x+3, r.y+3), L"22050Hz");
        
        
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

void ATPhaserFilterEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXPhaserFilter *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXPhaserFilter *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATPhaserFilterEditor::loadParameters(){
    m_frequency.setValue(m_parameter.lfoFrequency);
    m_depth.setValue(m_parameter.depth);
    m_centerFrequency.setValue((m_parameter.minFrequency+m_parameter.maxFrequency)*.5f);
    m_range.setValue(m_parameter.maxFrequency-m_parameter.minFrequency);
	m_feedback.setValue(m_parameter.feedback);
	switch(m_parameter.stages){
		case 4:
			m_stages.setSelectedIndex(0);
			break;
		case 6:
			m_stages.setSelectedIndex(1);
			break;
		case 8:
			m_stages.setSelectedIndex(2);
			break;
		case 10:
			m_stages.setSelectedIndex(3);
			break;
		case 12:
			m_stages.setSelectedIndex(4);
			break;
	}
}

void ATPhaserFilterEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATPhaserFilterEditor::command(int id){
    if(id==IdFrequency){
        m_parameter.lfoFrequency=m_frequency.value();
        invalidateClientRect(rectForDeltaGraph());
        setParameters();
    }else if(id==IdDepth){
        m_parameter.depth=m_depth.value();
        setParameters();
    }else if(id==IdFeedback){
        m_parameter.feedback=m_feedback.value();
        setParameters();
    }else if(id==IdStages){
        switch(m_stages.selectedIndex()){
			case 0:
				m_parameter.stages=4;
				break;
			case 1:
				m_parameter.stages=6;
				break;
			case 2:
				m_parameter.stages=8;
				break;
			case 3:
				m_parameter.stages=10;
				break;
			case 4:
				m_parameter.stages=12;
				break;
		}
        setParameters();
    }else if(id==IdCenterFrequency || id==IdRange){
        m_parameter.minFrequency=m_centerFrequency.value()-
		m_range.value()*.5f;
		m_parameter.maxFrequency=m_centerFrequency.value()+
		m_range.value()*.5f;
		if(m_parameter.minFrequency<0.f)
			m_parameter.minFrequency=0.f;
		if(m_parameter.maxFrequency>22050.f)
			m_parameter.maxFrequency=22050.f;
		invalidateClientRect(rectForDeltaGraph());
        setParameters();
    }
}

