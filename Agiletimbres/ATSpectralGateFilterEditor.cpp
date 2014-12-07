//
//  ATSpectralGateFilterEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "ATSpectralGateFilterEditor.h"
#include "ATSynthesizer.h"
#include "Utils.h"

enum{
    IdThreshold=1,
	IdSuperEnergy,
	IdSubEnergy,
	IdSpeed,
	IdCenterFreq,
	IdBandwidth
};

ATSpectralGateFilterEditor::ATSpectralGateFilterEditor(){
    m_plugin=NULL;
    
    m_threshold.setParent(this);
    m_threshold.setRect(twRect(135, 1, 40, 13));
    m_threshold.setValueRange(1.f/65536.f, 65536.f);
    m_threshold.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_threshold.setMode(ATValueEditor::ModeLogarithm);
    m_threshold.setId(IdThreshold);
	m_threshold.setEpsilon(1.e-10f);
    m_threshold.show();
    
    m_superEnergy.setParent(this);
    m_superEnergy.setRect(twRect(130, 17, 40, 13));
    m_superEnergy.setValueRange(1.f/65536.f, 10.f);
    m_superEnergy.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_superEnergy.setMode(ATValueEditor::ModeLogarithm);
    m_superEnergy.setId(IdSuperEnergy);
	m_superEnergy.setEpsilon(1.e-10f);
    m_superEnergy.show();
    
    m_subEnergy.setParent(this);
    m_subEnergy.setRect(twRect(260, 17, 40, 13));
    m_subEnergy.setValueRange(1.f/65536.f, 10.f);
    m_subEnergy.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_subEnergy.setMode(ATValueEditor::ModeLogarithm);
    m_subEnergy.setId(IdSubEnergy);
	m_subEnergy.setEpsilon(1.e-10f);
    m_subEnergy.show();
	
	m_speed.setParent(this);
    m_speed.setRect(twRect(240, 1, 40, 13));
    m_speed.setValueRange(0.f, 1.f);
    m_speed.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_speed.setMode(ATValueEditor::ModeLinear);
    m_speed.setId(IdSpeed);
	m_speed.setEpsilon(1.e-10f);
    m_speed.show();
    
    m_centerFreq.setParent(this);
    m_centerFreq.setRect(twRect(146, 37, 40, 13));
    m_centerFreq.setValueRange(0.f, 22050.f);
    m_centerFreq.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_centerFreq.setMode(ATValueEditor::ModeLinear);
    m_centerFreq.setId(IdCenterFreq);
    m_centerFreq.show();
	
	m_bandwidth.setParent(this);
    m_bandwidth.setRect(twRect(260, 37, 40, 13));
    m_bandwidth.setValueRange(0.f, 22050.f);
    m_bandwidth.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_bandwidth.setMode(ATValueEditor::ModeLogarithm);
    m_bandwidth.setId(IdBandwidth);
    m_bandwidth.show();
    
}

ATSpectralGateFilterEditor::~ATSpectralGateFilterEditor(){
	
}



#pragma mark - Main Geometry

twRect ATSpectralGateFilterEditor::rectForFreqGraph() const{
    return twRect(4, 52, 312, 196-52);
}

static int pixelsForLevel(float vl){
	if(vl<=0.f)
		return 1000;
	return (int)(-logf(vl)*5.f);
}

void ATSpectralGateFilterEditor::clientPaint(const twPaintStruct &p){
    twRect r;
    const twFont *font=getFont();
    std::wstring str;
    twSize size;
    twColor groupColor=twRGB(128, 128, 128);
    twColor lineColor=groupColor;
    twColor borderColor=twRGB(192, 192, 192);
    twColor graphColor=twRGB(128, 220, 32);
	twColor graphSubColor=twRGB(200, 90, 0);
	twColor graphMarkerColor=twRGB(45, 70, 200);
    twColor gridColor=twRGB(64, 64, 64);
    twColor labelColor=tw_curSkin->getWndTextColor();
    twDC *dc=p.dc;
    
    font->render(dc, groupColor, twPoint(1,2),
                 L"Spectral Gate");
    font->render(dc, labelColor, twPoint(83,2),
                 L"Threshold");
    font->render(dc, labelColor, twPoint(177,2),
                 L"db");
	font->render(dc, labelColor, twPoint(203,2),
                 L"Speed");
    font->render(dc, labelColor, twPoint(282,2),
                 L"%");
    font->render(dc, labelColor, twPoint(64,18),
                 L"Super Energy");
    font->render(dc, labelColor, twPoint(172,18),
                 L"db");
    font->render(dc, labelColor, twPoint(203,18),
                 L"Sub Energy");
    font->render(dc, labelColor, twPoint(302,18),
                 L"db");
	
	dc->drawLine(lineColor, twPoint(1, 34), twPoint(318, 34));
    
    font->render(dc, groupColor, twPoint(1,38),
                 L"Band-pass Filter");
	
    font->render(dc, labelColor, twPoint(85,38),
                 L"Center Freq");
	font->render(dc, labelColor, twPoint(188,38),
                 L"Hz");
	
	font->render(dc, labelColor, twPoint(205,38),
                 L"Bandwidth");
	font->render(dc, labelColor, twPoint(302,38),
                 L"Hz");
    
    r=rectForFreqGraph();
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
        
        for(int i=1;i<16;i++){
            int x=r.x+(r.w*i)/16;
            dc->drawLine(gridColor, twPoint(x, r.y), 
                         twPoint(x, r.y+r.h));
        }
        for(int i=1;i<8;i++){
            int y=r.y+(r.h*i)/8;
            dc->drawLine(gridColor, twPoint(r.x, y), 
                         twPoint(r.x+r.w, y));
        }
        
        
        
        int minFreq=(int)((m_parameter.centerFreq-m_parameter.bandwidth*.5f)/
						  22050.f*(float)r.w);
		int maxFreq=(int)((m_parameter.centerFreq+m_parameter.bandwidth*.5f)/
						  22050.f*(float)r.w);
		int zeroY=r.h/3;
		int superMinY=zeroY+pixelsForLevel(m_parameter.threshold*
										   m_parameter.superEnergy);
		int subMaxY=zeroY+pixelsForLevel(m_parameter.threshold*
										   m_parameter.subEnergy);
		int thresholdY=zeroY+pixelsForLevel(m_parameter.threshold);
		
		
		for(int x=1;x<r.w;x+=4){
			if(x<minFreq || x>maxFreq) continue;
			
			if(superMinY>=0){
				dc->drawLine(graphSubColor, twPoint(x+r.x+2, r.y), 
							 twPoint(x+r.x+2, r.y+superMinY));
			}
			
			if(subMaxY<r.h){
				dc->drawLine(graphColor, twPoint(x+r.x, r.y+subMaxY), 
							 twPoint(x+r.x, r.y+r.h));
			}
			
		}
		
		dc->drawLine(graphMarkerColor, twPoint(r.x, r.y+thresholdY), 
					 twPoint(r.x+r.w, r.y+thresholdY));
		
		
        str=L"DC";
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(r.x+3, r.y+r.h-3-size.h), str);
        
        
        str=L"22050Hz";
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(r.x+r.w-3-size.w, r.y+r.h-3-size.h), str);
        
        dc->setClipRect(oldClipRect);
    }
}

void ATSpectralGateFilterEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXSpectralGateFilter *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXSpectralGateFilter *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATSpectralGateFilterEditor::loadParameters(){
    m_threshold.setValue(m_parameter.threshold);
    m_superEnergy.setValue(m_parameter.superEnergy);
    m_subEnergy.setValue(m_parameter.subEnergy);
    m_centerFreq.setValue(m_parameter.centerFreq);
    m_bandwidth.setValue(m_parameter.bandwidth);
	m_speed.setValue(m_parameter.speed);
}

void ATSpectralGateFilterEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATSpectralGateFilterEditor::command(int id){
    if(id==IdThreshold){
        m_parameter.threshold=m_threshold.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdSuperEnergy){
        m_parameter.superEnergy=m_superEnergy.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdSubEnergy){
        m_parameter.subEnergy=m_subEnergy.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdCenterFreq){
        m_parameter.centerFreq=m_centerFreq.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdBandwidth){
        m_parameter.bandwidth=m_bandwidth.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdSpeed){
        m_parameter.speed=m_speed.value();
        setParameters();
    }
}


