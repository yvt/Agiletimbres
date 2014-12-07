//
//  ATMatrixReverb1Editor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATMatrixReverb1Editor.h"
#include "ATSynthesizer.h"
#include "Utils.h"

enum{
    IdReverbTime=1,
    IdGain,
    IdHighFreqDamp,
    IdEr
};

ATMatrixReverb1Editor::ATMatrixReverb1Editor(){
    m_plugin=NULL;
    
    m_gain.setParent(this);
    m_gain.setRect(twRect(119, 1, 40, 13));
    m_gain.setValueRange(0.f, 1.f);
    m_gain.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_gain.setMode(ATValueEditor::ModeLogarithm);
    m_gain.setId(IdGain);
    m_gain.show();
    
    m_reverbTime.setParent(this);
    m_reverbTime.setRect(twRect(245, 1, 40, 13));
    m_reverbTime.setValueRange(0.1f, 30.f);
    m_reverbTime.setDisplayMode(ATValueEditor::DisplayModeReal);
    m_reverbTime.setMode(ATValueEditor::ModeLogarithm);
    m_reverbTime.setId(IdReverbTime);
    m_reverbTime.show();
    
    m_highFreqDamp.setParent(this);
    m_highFreqDamp.setRect(twRect(180, 17, 40, 13));
    m_highFreqDamp.setValueRange(0.0f, 1.f);
    m_highFreqDamp.setDisplayMode(ATValueEditor::DisplayModeReal);
    m_highFreqDamp.setMode(ATValueEditor::ModeLinear);
    m_highFreqDamp.setId(IdHighFreqDamp);
    m_highFreqDamp.show();
    
    m_er.setParent(this);
    m_er.setRect(twRect(260, 17, 40, 13));
    m_er.setValueRange(0.f, 1.f);
    m_er.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_er.setMode(ATValueEditor::ModeLogarithm);
    m_er.setId(IdEr);
    m_er.show();
    
}

ATMatrixReverb1Editor::~ATMatrixReverb1Editor(){
  
}



#pragma mark - Main Geometry

twRect ATMatrixReverb1Editor::rectForFreqGraph() const{
    return twRect(4, 32, 312, 164);
}

void ATMatrixReverb1Editor::clientPaint(const twPaintStruct &p){
    twRect r;
    const twFont *font=getFont();
    std::wstring str;
    twSize size;
    twColor groupColor=twRGB(128, 128, 128);
    //twColor lineColor=groupColor;
    twColor borderColor=twRGB(192, 192, 192);
    twColor graphColor=twRGB(128, 220, 32);
    twColor gridColor=twRGB(64, 64, 64);
    twColor labelColor=tw_curSkin->getWndTextColor();
    twDC *dc=p.dc;
    
    font->render(dc, groupColor, twPoint(1,2),
                 L"Reverbrator");
    font->render(dc, labelColor, twPoint(65,2),
                 L"Input Gain");
    font->render(dc, labelColor, twPoint(160,2),
                 L"db");
    font->render(dc, labelColor, twPoint(181,2),
                 L"Reverb Time");
    font->render(dc, labelColor, twPoint(286,2),
                 L"s");
    font->render(dc, labelColor, twPoint(65,18),
                 L"High Frequency Damp");
    font->render(dc, labelColor, twPoint(225,18),
                 L"Early Ref.");
    font->render(dc, labelColor, twPoint(300,18),
                 L"db");
    
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
        
        const float referenceDelayTime=5843.f/44100.f;
        float gain;
        float lowpass=m_parameter.highFrequencyDamp;
        float negLowpass=lowpass-1.f;
        float reverbTimeNorm=(float)(r.h-1)/m_parameter.reverbTime;
        gain=powf(1e-3f, referenceDelayTime/m_parameter.reverbTime);
        
        const int steps=16;
        float freqRange=16000.f;
        float freqScale=(float)(r.w-1)/freqRange;
        float freqNormScale=(float)M_PI/freqRange;
        float freqStep=1.f/(float)steps;
        twPoint lastPoint;
        
        float freq2=0.f;
        for(int i=0;i<=steps;i++){
            float freq=freq2*freq2;
            freq*=freqRange;
            float normFreq=freq*freqNormScale;
            float freqGain=lowpass;
            freqGain/=sqrtf(1.f+2.f*negLowpass*cosf(normFreq)+
                            negLowpass*negLowpass);
            freqGain=gain*(freqGain);
            
            float reverbTime;
            reverbTime=logf(freqGain)/logf(1.e-3f);
            reverbTime=referenceDelayTime/reverbTime;
            
            twPoint point;
            
            point.x=r.x+(int)(freq*freqScale);
            point.y=r.y+r.h-1-(int)(reverbTime*reverbTimeNorm);
            
            if(i>0)
                dc->drawLine(graphColor, lastPoint, point);
            
            lastPoint=point;
            
            freq2+=freqStep;
        }
        
        str=NHFormatStd(L"%.2fs", m_parameter.reverbTime);
        font->render(dc, labelColor, 
                     twPoint(r.x+3, r.y+3), str);
        
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

void ATMatrixReverb1Editor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXMatrixReverb1 *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXMatrixReverb1 *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATMatrixReverb1Editor::loadParameters(){
    m_reverbTime.setValue(m_parameter.reverbTime);
    m_gain.setValue(m_parameter.inGain);
    m_highFreqDamp.setValue(m_parameter.highFrequencyDamp);
    m_er.setValue(m_parameter.earlyReflectionGain);
}

void ATMatrixReverb1Editor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATMatrixReverb1Editor::command(int id){
    if(id==IdReverbTime){
        m_parameter.reverbTime=m_reverbTime.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdGain){
        m_parameter.inGain=m_gain.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdHighFreqDamp){
        m_parameter.highFrequencyDamp=m_highFreqDamp.value();
        invalidateClientRect(rectForFreqGraph());
        setParameters();
    }else if(id==IdEr){
        m_parameter.earlyReflectionGain=m_er.value();
        setParameters();
    }
}


