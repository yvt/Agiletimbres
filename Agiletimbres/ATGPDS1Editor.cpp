//
//  ATGPDS1Editor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATGPDS1Editor.h"
#include "ATTabView.h"
#include "ATSynthesizer.h"
#include "ATValueEditor.h"
#include "ATDropdownList.h"
#include "Utils.h"
#include "TXSineTable.h"

#pragma mark - Oscilator Editor

class ATGPDS1Editor::OscilatorEditor: public twWnd{
    enum{
        IdCoarseTune,
        IdFineTune,
        IdWaveform,
        IdSync,
        IdFixStart,
        IdAttackTime,
        IdAttackPower,
        IdDecayTime,
        IdDecayPower,
        IdSustainLevel,
        IdReleaseTime,
        IdReleasePower,
        IdLfoWaveform,
        IdLfoDepth,
        IdLfoFreq,
        IdLfoDelay,
        IdLfoPeriodBase
    };
    
    ATGPDS1Editor *m_editor;
    TXGPDS1::Oscilator *m_oscilator;
    
    ATValueEditor m_coarseTune;
    ATValueEditor m_fineTune;
    ATDropdownList m_waveform;
    ATDropdownList m_sync;
    twCheckBox m_fixStart;
    ATValueEditor m_attackTime;
    ATValueEditor m_attackPower;
    ATValueEditor m_decayTime;
    ATValueEditor m_decayPower;
    ATValueEditor m_sustainLevel;
    ATValueEditor m_releaseTime;
    ATValueEditor m_releasePower;
    ATDropdownList m_lfoWaveform;
    ATValueEditor m_lfoDepth;
    ATValueEditor m_lfoFreq;
    ATValueEditor m_lfoDelay;
    twCheckBox m_lfoPeriodBase;
    
    void loadParameters(){
        if(!m_oscilator)
            return;
        m_coarseTune.setValue(m_oscilator->transposeCoarse);
        m_fineTune.setValue(m_oscilator->transposeFine);
        switch(m_oscilator->waveformType){
            case TXGPDS1::WaveformTypeSine:
                m_waveform.setSelectedIndex(0);
                break;
            case TXGPDS1::WaveformTypeSawtooth:
                m_waveform.setSelectedIndex(1);
                break;
            case TXGPDS1::WaveformTypeSquare:
                m_waveform.setSelectedIndex(2);
                break;
            case TXGPDS1::WaveformTypePulse4:
                m_waveform.setSelectedIndex(3);
                break;
            case TXGPDS1::WaveformTypePulse8:
                m_waveform.setSelectedIndex(4);
                break;
            case TXGPDS1::WaveformTypePulse16:
                m_waveform.setSelectedIndex(5);
                break;
                
            case TXGPDS1::WaveformTypeNoise:
                m_waveform.setSelectedIndex(6);
                break;
        }
        m_sync.setSelectedIndex(m_oscilator->syncTarget);
        m_fixStart.setCheckState(m_oscilator->fixStart);
        m_attackTime.setValue(m_oscilator->attackTime);
        m_attackPower.setValue(m_oscilator->attackPower);
        m_decayTime.setValue(m_oscilator->decayTime);
        m_decayPower.setValue(m_oscilator->decayPower);
        m_sustainLevel.setValue(m_oscilator->sustainLevel);
        m_releaseTime.setValue(m_oscilator->releaseTime);
        m_releasePower.setValue(m_oscilator->releasePower);
        switch(m_oscilator->lfoWaveformType){
            case TXGPDS1::LfoWaveformTypeSine:
                m_lfoWaveform.setSelectedIndex(0);
                break;
            case TXGPDS1::LfoWaveformTypeSquare:
                m_lfoWaveform.setSelectedIndex(1);
                break;
            case TXGPDS1::LfoWaveformTypeTriangle:
                m_lfoWaveform.setSelectedIndex(2);
                break;
            case TXGPDS1::LfoWaveformTypeSawtoothUp:
                m_lfoWaveform.setSelectedIndex(3);
                break;
            case TXGPDS1::LfoWaveformTypeSawtoothDown:
                m_lfoWaveform.setSelectedIndex(4);
                break;
        }
        m_lfoDepth.setValue(m_oscilator->lfoDepth);
        m_lfoFreq.setValue(m_oscilator->lfoFreq);
        m_lfoDelay.setValue(m_oscilator->lfoDelay);
        m_lfoPeriodBase.setCheckState(m_oscilator->lfoPeriodBase);
    }
    
    void setParameters(){
        sendCmdToParent();
    }
    
    twRect rectForEnvelopeGraph() const{
        return twRect(2, 51, 100, 48);
    }
    
    void drawLogGraph(twDC *dc, twRect rt, twColor color,
                      float timeScale, float ampScale,
                      float time1, float time2,
                      float amp1, float amp2,
                      int power){
        int steps=16;
        if(fabsf(amp1-amp2)<.00001f)
            steps=1;
        float delta=1.f/(float)steps;
        float deltaTime=(time2-time1);
        float deltaAmp=(amp2-amp1);
        float pos=0.f;
        
        twPoint lastPoint;
        for(int i=0;i<=steps;i++){
            twPoint point;
            float pos2=pos;
            pos2*=pos2; pos2*=pos2;
            float amp=pos2;
            float time=time1+deltaTime*pos2;
            for(int i=1;i<power;i++)
                amp*=pos2;
            amp=amp1+deltaAmp*amp;
            
            point.x=(int)(time*timeScale)+rt.x;
            if(amp<=0.f){
                // -inf db.
                point.y=rt.y+rt.h*16;
            }else{
                amp=logf(amp);
                point.y=(int)(amp*ampScale)+rt.y;
            }
            
            if(i>0){
                dc->drawLine(color,lastPoint, point);
            }
            lastPoint=point;
            
            time+=deltaTime;
            pos+=delta;
        }
    }
    
    void drawSineGraph(twDC *dc, twRect rt, twColor color,
                       float timeScale, float pitchScale,
                       float time1, float time2,
                       float depth){
        int steps=32;
        if(depth<.1f)
            steps=1;
        twPoint lastPoint;
        float pos=0.f;
        float delta=1.f/(float)steps;
        float deltaTime=(time2-time1);
        int centerY=rt.y+rt.h/2;
        
        switch(m_oscilator->lfoWaveformType){
            case TXGPDS1::LfoWaveformTypeSine:
                depth/=32768.f;
                
                for(int i=0;i<=steps;i++){
                    float time=time1+deltaTime*pos;
                    twPoint point;
                    float wave=(float)TXSineWave((uint32_t)(pos*(65536.f*65536.f)));
                    wave*=depth;
                    
                    point.x=rt.x+(int)(time*timeScale);
                    point.y=centerY+(int)(wave*pitchScale);
                    
                    if(i>0){
                        dc->drawLine(color,lastPoint, point);
                    }
                    lastPoint=point;
                    
                    pos+=delta;
                }
                break;
            case TXGPDS1::LfoWaveformTypeSquare:
                {
                    twPoint point;
                    point.x=rt.x+(int)(time1*timeScale);
                    point.y=centerY;
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time1*timeScale);
                    point.y=centerY+(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)((time1+deltaTime*.5f)*timeScale);
                    point.y=centerY+(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)((time1+deltaTime*.5f)*timeScale);
                    point.y=centerY-(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time2*timeScale);
                    point.y=centerY-(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time2*timeScale);
                    point.y=centerY;
                    dc->drawLine(color,lastPoint, point);
                }   
                break;
            case TXGPDS1::LfoWaveformTypeTriangle:
                {
                    twPoint point;
                    point.x=rt.x+(int)(time1*timeScale);
                    point.y=centerY;
                    lastPoint=point;
                    
                    point.x=rt.x+(int)((time1+deltaTime*.25f)*timeScale);
                    point.y=centerY+(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)((time1+deltaTime*.75f)*timeScale);
                    point.y=centerY-(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time2*timeScale);
                    point.y=centerY;
                    dc->drawLine(color,lastPoint, point);
                }   
                break;
            case TXGPDS1::LfoWaveformTypeSawtoothUp:
                {
                    twPoint point;
                    point.x=rt.x+(int)(time1*timeScale);
                    point.y=centerY;
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time1*timeScale);
                    point.y=centerY+(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time2*timeScale);
                    point.y=centerY-(int)(depth*pitchScale);
                    dc->drawLine(color,lastPoint, point);
                    lastPoint=point;
                    
                    point.x=rt.x+(int)(time2*timeScale);
                    point.y=centerY;
                    dc->drawLine(color,lastPoint, point);
                }   
                break;
            case TXGPDS1::LfoWaveformTypeSawtoothDown:
            {
                twPoint point;
                point.x=rt.x+(int)(time1*timeScale);
                point.y=centerY;
                lastPoint=point;
                
                point.x=rt.x+(int)(time1*timeScale);
                point.y=centerY-(int)(depth*pitchScale);
                dc->drawLine(color,lastPoint, point);
                lastPoint=point;
                
                point.x=rt.x+(int)(time2*timeScale);
                point.y=centerY+(int)(depth*pitchScale);
                dc->drawLine(color,lastPoint, point);
                lastPoint=point;
                
                point.x=rt.x+(int)(time2*timeScale);
                point.y=centerY;
                dc->drawLine(color,lastPoint, point);
            }   
                break;
        }
        
    }
    
    twRect rectForLFOGraph() const{
        return twRect(2, 121, 100, 48);
    }
public:
    OscilatorEditor(ATGPDS1Editor *editor){
        m_editor=editor;
        m_oscilator=NULL;
        
        m_coarseTune.setParent(this);
        m_coarseTune.setRect(twRect(60, 1, 40, 13));
        m_coarseTune.setValueRange(-36, 36);
        m_coarseTune.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_coarseTune.setMode(ATValueEditor::ModeInteger);
        m_coarseTune.setId(IdCoarseTune);
        m_coarseTune.show();
        
        m_fineTune.setParent(this);
        m_fineTune.setRect(twRect(115, 1, 40, 13));
        m_fineTune.setValueRange(-100, 100);
        m_fineTune.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_fineTune.setMode(ATValueEditor::ModeInteger);
        m_fineTune.setId(IdFineTune);
        m_fineTune.show();
        
        m_waveform.setParent(this);
        m_waveform.setRect(twRect(246, 1, 70, 13));
        m_waveform.setId(IdWaveform);
        m_waveform.items().push_back(L"Sine");
        m_waveform.items().push_back(L"Sawtooth");
        m_waveform.items().push_back(L"Square");
        m_waveform.items().push_back(L"Pulse 1/4");
        m_waveform.items().push_back(L"Pulse 1/8");
        m_waveform.items().push_back(L"Pulse 1/16");
        m_waveform.items().push_back(L"Noise");
        m_waveform.show();
        
        m_sync.setParent(this);
        m_sync.setRect(twRect(86, 17, 70, 13));
        m_sync.setId(IdSync);
        m_sync.items().push_back(L"Disable");
        m_sync.items().push_back(L"Oscillator 1");
        m_sync.items().push_back(L"Oscillator 2");
        m_sync.items().push_back(L"Oscillator 3");
        m_sync.show();
        
        m_fixStart.setParent(this);
        m_fixStart.setRect(twRect(170, 17, 140, 13));
        m_fixStart.setId(IdFixStart);
        m_fixStart.setTitle(L"Fixed Initial Phase");
        m_fixStart.show();
        
        m_attackTime.setParent(this);
        m_attackTime.setRect(twRect(180, 38, 60, 13));
        m_attackTime.setValueRange(0.f, 10.f);
        m_attackTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_attackTime.setMode(ATValueEditor::ModeLogarithm);
        m_attackTime.setId(IdAttackTime);
        m_attackTime.show();
        
        m_attackPower.setParent(this);
        m_attackPower.setRect(twRect(285, 38, 30, 13));
        m_attackPower.setValueRange(1, 10);
        m_attackPower.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_attackPower.setMode(ATValueEditor::ModeInteger);
        m_attackPower.setId(IdAttackPower);
        m_attackPower.show();
        
        m_decayTime.setParent(this);
        m_decayTime.setRect(twRect(180, 54, 60, 13));
        m_decayTime.setValueRange(0.f, 10.f);
        m_decayTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_decayTime.setMode(ATValueEditor::ModeLogarithm);
        m_decayTime.setId(IdDecayTime);
        m_decayTime.show();
        
        m_decayPower.setParent(this);
        m_decayPower.setRect(twRect(285, 54, 30, 13));
        m_decayPower.setValueRange(1, 10);
        m_decayPower.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_decayPower.setMode(ATValueEditor::ModeInteger);
        m_decayPower.setId(IdDecayPower);
        m_decayPower.show();
        
        m_sustainLevel.setParent(this);
        m_sustainLevel.setRect(twRect(180, 70, 60, 13));
        m_sustainLevel.setValueRange(0.f, 1.f);
        m_sustainLevel.setDisplayMode(ATValueEditor::DisplayModeDecibel);
        m_sustainLevel.setMode(ATValueEditor::ModeLogarithm);
        m_sustainLevel.setEpsilon(1.e-3f);
        m_sustainLevel.setId(IdSustainLevel);
        m_sustainLevel.show();
        
        m_releaseTime.setParent(this);
        m_releaseTime.setRect(twRect(180, 86, 60, 13));
        m_releaseTime.setValueRange(0.f, 10.f);
        m_releaseTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_releaseTime.setMode(ATValueEditor::ModeLogarithm);
        m_releaseTime.setId(IdReleaseTime);
        m_releaseTime.show();
        
        m_releasePower.setParent(this);
        m_releasePower.setRect(twRect(285, 86, 30, 13));
        m_releasePower.setValueRange(1, 10);
        m_releasePower.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_releasePower.setMode(ATValueEditor::ModeInteger);
        m_releasePower.setId(IdReleasePower);
        m_releasePower.show();
        
        m_lfoWaveform.setParent(this);
        m_lfoWaveform.setRect(twRect(180, 108, 60, 13));
        m_lfoWaveform.setId(IdLfoWaveform);
        m_lfoWaveform.items().push_back(L"Sine");
        m_lfoWaveform.items().push_back(L"Square");
        m_lfoWaveform.items().push_back(L"Triangle");
        m_lfoWaveform.items().push_back(L"Saw Up");
        m_lfoWaveform.items().push_back(L"Saw Down");
        m_lfoWaveform.show();
        
        m_lfoPeriodBase.setParent(this);
        m_lfoPeriodBase.setRect(twRect(245, 108, 70, 13));
        m_lfoPeriodBase.setId(IdLfoPeriodBase);
        m_lfoPeriodBase.setTitle(L"Period Base");
        m_lfoPeriodBase.show();
        
        m_lfoDepth.setParent(this);
        m_lfoDepth.setRect(twRect(180, 124, 60, 13));
        m_lfoDepth.setValueRange(0.f, 3600.f);
        m_lfoDepth.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_lfoDepth.setMode(ATValueEditor::ModeLogarithm);
        m_lfoDepth.setId(IdLfoDepth);
        m_lfoDepth.show();
        
        m_lfoFreq.setParent(this);
        m_lfoFreq.setRect(twRect(180, 140, 60, 13));
        m_lfoFreq.setValueRange(0.01f, 10.f);
        m_lfoFreq.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_lfoFreq.setMode(ATValueEditor::ModeLogarithm);
        m_lfoFreq.setId(IdLfoFreq);
        m_lfoFreq.show();
        
        m_lfoDelay.setParent(this);
        m_lfoDelay.setRect(twRect(180, 156, 60, 13));
        m_lfoDelay.setValueRange(-10.f, 10.f);
        m_lfoDelay.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_lfoDelay.setMode(ATValueEditor::ModeLogarithm);
        m_lfoDelay.setId(IdLfoDelay);
        m_lfoDelay.setEpsilon(0.01f);
        m_lfoDelay.show();
        
        loadParameters();
    }
    virtual ~OscilatorEditor(){
        
    }
    
    void setOscilator(TXGPDS1::Oscilator *oscilator){
        m_oscilator=oscilator;
        loadParameters();
    }
    
    virtual void clientPaint(const twPaintStruct& p){
        twDC *dc=p.dc;
        const twFont *font=getFont();
        std::wstring str;
        twColor groupColor=twRGB(128, 128, 128);
        twColor lineColor=groupColor;
        twColor borderColor=twRGB(192, 192, 192);
        twColor graphColor=twRGB(128, 220, 32);
        twColor gridColor=twRGB(64, 64, 64);
        twColor labelColor=tw_curSkin->getWndTextColor();
        twRect r;
        
        font->render(dc, groupColor, twPoint(1,2),
                     L"Oscillator");
        font->render(dc, labelColor, twPoint(101, 2),
                     L"st");
        font->render(dc, labelColor, twPoint(156, 2),
                     L"cents");
        font->render(dc, labelColor, twPoint(193, 2),
                     L"Waveform");
        
        font->render(dc, labelColor, twPoint(60, 18),
                     L"Sync");
        
        dc->drawLine(lineColor, twPoint(1, 34), twPoint(318, 34));
        
        font->render(dc, groupColor, twPoint(1,38),
                     L"Envelope");
        
        font->render(dc, labelColor, twPoint(104,39),
                     L"Attack Time");
        font->render(dc, labelColor, twPoint(241,39),
                     L"s");
        font->render(dc, labelColor, twPoint(255,39),
                     L"Curve");
        
        font->render(dc, labelColor, twPoint(104,55),
                     L"Decay Time");
        font->render(dc, labelColor, twPoint(241,55),
                     L"s");
        font->render(dc, labelColor, twPoint(255,55),
                     L"Curve");
        
        font->render(dc, labelColor, twPoint(104,71),
                     L"Sustain Level");
        font->render(dc, labelColor, twPoint(241,71),
                     L"db");
        
        font->render(dc, labelColor, twPoint(104,87),
                     L"Release Time");
        font->render(dc, labelColor, twPoint(241,87),
                     L"s");
        font->render(dc, labelColor, twPoint(255,87),
                     L"Curve");
        
        r=rectForEnvelopeGraph();
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
            
            r.y+=8;
            r.h-=8;
            
            float timeRange=m_oscilator->attackTime;
            float sustainTime;
            timeRange+=m_oscilator->decayTime;
            timeRange+=m_oscilator->releaseTime;
            if(timeRange<.2f)
                timeRange=.2f;
            sustainTime=timeRange*.1f;
            timeRange+=sustainTime;
           
            float timeScale=(float)(r.w-1)/(float)timeRange;
            float ampScale=(float)r.h/(logf(1.e-3f));
            
            // db grids.
            dc->drawLine(gridColor, twPoint(r.x, r.y), 
                         twPoint(r.x+r.w, r.y));
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h/3), 
                         twPoint(r.x+r.w, r.y+r.h/3));
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h*2/3), 
                         twPoint(r.x+r.w, r.y+r.h*2/3));
            
            // time grids.
            float timeGridInterval;
            if(timeRange<.5f){
                timeGridInterval=.1f;
            }else if(timeRange<1.f){
                timeGridInterval=.2f;
            }else if(timeRange<2.f){
                timeGridInterval=.5f;
            }else if(timeRange<5.f){
                timeGridInterval=1.f;
            }else if(timeRange<10.f){
                timeGridInterval=2.f;
            }else if(timeRange<20.f){
                timeGridInterval=5.f;
            }else{
                timeGridInterval=10.f;
            }
            for(float time=0;time<timeRange;time+=timeGridInterval){
                int x=r.x;
                x+=(int)(time*timeScale);
                dc->drawLine(gridColor, twPoint(x, r.y-8),
                             twPoint(x, r.y+r.h));
            }
            
            // envelope.
            drawLogGraph(dc, r, graphColor, timeScale, ampScale, 
                         0.f, 
                         m_oscilator->attackTime, 
                         0.f, 1.f, 
                         m_oscilator->attackPower);
            drawLogGraph(dc, r, graphColor, timeScale, ampScale, 
                         m_oscilator->attackTime+m_oscilator->decayTime, 
                         m_oscilator->attackTime, 
                         m_oscilator->sustainLevel, 1.f, 
                         m_oscilator->decayPower);
            drawLogGraph(dc, r, graphColor, timeScale, ampScale,  
                         timeRange-m_oscilator->releaseTime, 
                         m_oscilator->attackTime+m_oscilator->decayTime,
                         m_oscilator->sustainLevel, m_oscilator->sustainLevel, 
                         1);
            drawLogGraph(dc, r, graphColor, timeScale, ampScale,  
                         timeRange,
                         timeRange-m_oscilator->releaseTime, 
                         0.f, m_oscilator->sustainLevel, 
                         m_oscilator->releasePower);
            
            if(timeRange>=1.f)
                str=NHFormatStd(L"%.2fs", timeRange);
            else
                str=NHFormatStd(L"%dms", (int)(timeRange*1000.f));
            twSize sz=font->measure(str);
            font->render(dc, labelColor, twPoint(r.x+r.w-sz.w-3,
                                                 r.y+2-8), 
                         str);
            
            dc->setClipRect(oldClipRect);
        }
        
        dc->drawLine(lineColor, twPoint(1, 104), twPoint(318, 104));
        
        font->render(dc, groupColor, twPoint(1,108),
                     L"LFO");
        
        font->render(dc, labelColor, twPoint(104,109),
                     L"Waveform");
        
        font->render(dc, labelColor, twPoint(104,125),
                     L"Depth");
        font->render(dc, labelColor, twPoint(241,125),
                     L"cents");
        
        font->render(dc, labelColor, twPoint(104,141),
                     L"Frequency");
        font->render(dc, labelColor, twPoint(241,141),
                     L"Hz");
        
        font->render(dc, labelColor, twPoint(104,157),
                     L"Delay");
        font->render(dc, labelColor, twPoint(241,157),
                     L"s");
        
        r=rectForLFOGraph();
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
            
            float timeRange=.5f;
            if(m_oscilator->lfoFreq<2.f){
                timeRange=1.f/m_oscilator->lfoFreq;
            }
            if(m_oscilator->lfoDelay>0.f)
            timeRange+=m_oscilator->lfoDelay;
            
            float timeScale=(float)r.w/(float)timeRange;
            float pitchScale=(float)r.h/200.f;
            
            // time grids.
            float timeGridInterval;
            if(timeRange<.5f){
                timeGridInterval=.1f;
            }else if(timeRange<1.f){
                timeGridInterval=.2f;
            }else if(timeRange<2.f){
                timeGridInterval=.5f;
            }else if(timeRange<5.f){
                timeGridInterval=1.f;
            }else if(timeRange<10.f){
                timeGridInterval=2.f;
            }else if(timeRange<20.f){
                timeGridInterval=5.f;
            }else{
                timeGridInterval=10.f;
            }
            for(float time=0;time<timeRange;time+=timeGridInterval){
                int x=r.x;
                x+=(int)(time*timeScale);
                dc->drawLine(gridColor, twPoint(x, r.y),
                             twPoint(x, r.y+r.h));
            }
            
            // center line.
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h/2), 
                         twPoint(r.x+r.w, r.y+r.h/2));
            
            // draw graph.
            dc->drawLine(graphColor, twPoint(r.x, r.y+r.h/2), 
                         twPoint(r.x+(int)(m_oscilator->lfoDelay*timeScale),
                                 r.y+r.h/2));
            
            float period=1.f/m_oscilator->lfoFreq;
            for(float time=m_oscilator->lfoDelay;time<timeRange;
                time+=period){
                drawSineGraph(dc, r, graphColor, 
                              timeScale, pitchScale, 
                              time, time+period, 
                              m_oscilator->lfoDepth);
            }
            
            // legend.
            if(timeRange>=1.f)
                str=NHFormatStd(L"%.2fs", timeRange);
            else
                str=NHFormatStd(L"%dms", (int)(timeRange*1000.f));
            twSize sz=font->measure(str);
            font->render(dc, labelColor, twPoint(r.x+r.w-sz.w-3,
                                                 r.y+2), 
                         str);
            
            dc->setClipRect(oldClipRect);
        }
        
        
        
    }
    
    virtual void command(int id){
        if(id==IdCoarseTune){
            m_oscilator->transposeCoarse=m_coarseTune.intValue();
            setParameters();
        }else if(id==IdFineTune){
            m_oscilator->transposeFine=m_fineTune.intValue();
            setParameters();
        }else if(id==IdWaveform){
            switch(m_waveform.selectedIndex()){
                case 0:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypeSine;
                    break;
                case 1:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypeSawtooth;
                    break;
                case 2:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypeSquare;
                    break;
                case 3:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypePulse4;
                    break;
                case 4:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypePulse8;
                    break;
                case 5:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypePulse16;
                    break;
                case 6:
                    m_oscilator->waveformType=TXGPDS1::WaveformTypeNoise;
                    break;
            }
            setParameters();
        }else if(id==IdSync){
            m_oscilator->syncTarget=m_sync.selectedIndex();
            setParameters();
        }else if(id==IdFixStart){
            m_oscilator->fixStart=m_fixStart.getCheckState();
            setParameters();
        }else if(id==IdAttackTime){
            m_oscilator->attackTime=m_attackTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdAttackPower){
            m_oscilator->attackPower=m_attackPower.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdDecayTime){
            m_oscilator->decayTime=m_decayTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdDecayPower){
            m_oscilator->decayPower=m_decayPower.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdSustainLevel){
            m_oscilator->sustainLevel=m_sustainLevel.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdReleaseTime){
            m_oscilator->releaseTime=m_releaseTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdReleasePower){
            m_oscilator->releasePower=m_releasePower.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdLfoWaveform){
            switch(m_lfoWaveform.selectedIndex()){
                case 0:
                    m_oscilator->lfoWaveformType=TXGPDS1::LfoWaveformTypeSine;
                    break;
                case 1:
                    m_oscilator->lfoWaveformType=TXGPDS1::LfoWaveformTypeSquare;
                    break;
                case 2:
                    m_oscilator->lfoWaveformType=TXGPDS1::LfoWaveformTypeTriangle;
                    break;
                case 3:
                    m_oscilator->lfoWaveformType=TXGPDS1::LfoWaveformTypeSawtoothUp;
                    break;
                case 4:
                    m_oscilator->lfoWaveformType=TXGPDS1::LfoWaveformTypeSawtoothDown;
                    break;
            }
            setParameters();
        }else if(id==IdLfoDepth){
            m_oscilator->lfoDepth=m_lfoDepth.value();
            setParameters();
            invalidateClientRect(rectForLFOGraph());
        }else if(id==IdLfoFreq){
            m_oscilator->lfoFreq=m_lfoFreq.value();
            setParameters();
            invalidateClientRect(rectForLFOGraph());
        }else if(id==IdLfoDelay){
            m_oscilator->lfoDelay=m_lfoDelay.value();
            setParameters();
            invalidateClientRect(rectForLFOGraph());
        }else if(id==IdLfoPeriodBase){
            m_oscilator->lfoPeriodBase=m_lfoPeriodBase.getCheckState();
            setParameters();
        }
    }
    
};

#pragma mark - Filter Editor


class ATGPDS1Editor::FilterEditor: public twWnd{
    enum{
        IdFilterType,
        IdResonance,
        IdInitialFreq,
        IdAttackTime,
        IdAttackFreq,
        IdDecayTime,
        IdSustainFreq,
        IdReleaseTime,
        IdReleaseFreq
    };
    
    ATGPDS1Editor *m_editor;
    TXGPDS1::Filter *m_filter;
    
    ATValueEditor m_resonance;
    ATDropdownList m_filterType;
    ATValueEditor m_initialFreq;
    ATValueEditor m_attackTime;
    ATValueEditor m_attackFreq;
    ATValueEditor m_decayTime;
    ATValueEditor m_sustainFreq;
    ATValueEditor m_releaseTime;
    ATValueEditor m_releaseFreq;
    
    void loadParameters(){
        if(!m_filter)
            return;
        m_resonance.setValue(m_filter->resonance);
        if(m_filter->enable){
            switch(m_filter->filterType){
                case TXGPDS1::FilterTypeLpf6LC:
                    m_filterType.setSelectedIndex(1);
                    break;
                case TXGPDS1::FilterTypeLpf12LC:
                    m_filterType.setSelectedIndex(2);
                    break;
                case TXGPDS1::FilterTypeLpf12SID:
                    m_filterType.setSelectedIndex(3);
                    break;
                case TXGPDS1::FilterTypeLpf24Moog:
                    m_filterType.setSelectedIndex(4);
                    break;
                case TXGPDS1::FilterTypeHpf6LC:
                    m_filterType.setSelectedIndex(5);
                    break;
                case TXGPDS1::FilterTypeHpf12LC:
                    m_filterType.setSelectedIndex(6);
                    break;
                case TXGPDS1::FilterTypeHpf12SID:
                    m_filterType.setSelectedIndex(7);
                    break;
                case TXGPDS1::FilterTypeHpf24Moog:
                    //m_filterType.setSelectedIndex(8);
                    break;
					
                case TXGPDS1::FilterTypeBpf6SID:
                    m_filterType.setSelectedIndex(8);
                    break;
				case TXGPDS1::FilterTypeNotch6SID:
                    m_filterType.setSelectedIndex(9);
                    break;
                    
            }
        }else{
        m_filterType.setSelectedIndex(0);
        }
        m_initialFreq.setValue(m_filter->initialFreq);
        m_attackTime.setValue(m_filter->attackTime);
        m_attackFreq.setValue(m_filter->attackFreq);
        m_decayTime.setValue(m_filter->decayTime);
        m_sustainFreq.setValue(m_filter->sustainFreq);
        m_releaseTime.setValue(m_filter->releaseTime);
        m_releaseFreq.setValue(m_filter->releaseFreq);
    }
    
    void setParameters(){
        sendCmdToParent();
    }
    
    twRect rectForEnvelopeGraph() const{
        return twRect(2, 51, 160, 100);
    }
    
    void drawFreqGraph(twDC *dc, twRect rt, twColor color,
                      float timeScale, float freqScale,
                      float time1, float time2,
                      float freq1, float freq2){
        twPoint pt1, pt2;
        
        pt1.x=rt.x+(int)(time1*timeScale);
        pt1.y=rt.y+rt.h-1-(int)(freq1*freqScale);
        
        pt2.x=rt.x+(int)(time2*timeScale);
        pt2.y=rt.y+rt.h-1-(int)(freq2*freqScale);
        
        dc->drawLine(color, pt1, pt2);
    }
    
    void drawSineGraph(twDC *dc, twRect rt, twColor color,
                       float timeScale, float pitchScale,
                       float time1, float time2,
                       float depth){
        int steps=32;
        if(depth<.1f)
            steps=1;
        twPoint lastPoint;
        float pos=0.f;
        float delta=1.f/(float)steps;
        float deltaTime=(time2-time1);
        int centerY=rt.y+rt.h/2;
        
        depth/=32768.f;
        
        for(int i=0;i<=steps;i++){
            float time=time1+deltaTime*pos;
            twPoint point;
            float wave=(float)TXSineWave((uint32_t)(pos*(65536.f*65536.f)));
            wave*=depth;
            
            point.x=rt.x+(int)(time*timeScale);
            point.y=centerY+(int)(wave*pitchScale);
            
            if(i>0){
                dc->drawLine(color,lastPoint, point);
            }
            lastPoint=point;
            
            pos+=delta;
        }
    }
    
public:
    FilterEditor(ATGPDS1Editor *editor){
        m_editor=editor;
        m_filter=NULL;
        
        m_resonance.setParent(this);
        m_resonance.setRect(twRect(100, 17, 40, 13));
        m_resonance.setValueRange(0.f, .99f);
        m_resonance.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_resonance.setMode(ATValueEditor::ModeLinear);
        m_resonance.setId(IdResonance);
        m_resonance.show();
        
        m_filterType.setParent(this);
        m_filterType.setRect(twRect(100, 1, 200, 13));
        m_filterType.setId(IdFilterType);
        m_filterType.items().push_back(L"No filter");
        m_filterType.items().push_back(L"LC circuit LPF 6db/oct");
        m_filterType.items().push_back(L"LC circuit LPF 12db/oct");
        m_filterType.items().push_back(L"MOS6581 LPF 12db/oct");
        m_filterType.items().push_back(L"Moog LPF 24db/oct");
        m_filterType.items().push_back(L"LC circuit HPF 6db/oct");
        m_filterType.items().push_back(L"LC circuit HPF 12db/oct");
        m_filterType.items().push_back(L"MOS6581 HPF 12db/oct");
        m_filterType.items().push_back(L"MOS6581 BPF 6db/oct");
        m_filterType.items().push_back(L"MOS6581 Notch 6db/oct");
        //m_filterType.items().push_back(L"Moog HPF 24db/oct");
        m_filterType.show();
        
        m_initialFreq.setParent(this);
        m_initialFreq.setRect(twRect(240, 38, 60, 13));
        m_initialFreq.setValueRange(1.f, 22050.f);
        m_initialFreq.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_initialFreq.setMode(ATValueEditor::ModeLogarithm);
        m_initialFreq.setId(IdInitialFreq);
        m_initialFreq.show();
        
        m_attackTime.setParent(this);
        m_attackTime.setRect(twRect(240, 54, 60, 13));
        m_attackTime.setValueRange(0.f, 10.f);
        m_attackTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_attackTime.setMode(ATValueEditor::ModeLogarithm);
        m_attackTime.setId(IdAttackTime);
        m_attackTime.show();
        
        m_attackFreq.setParent(this);
        m_attackFreq.setRect(twRect(240, 70, 60, 13));
        m_attackFreq.setValueRange(1.f, 22050.f);
        m_attackFreq.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_attackFreq.setMode(ATValueEditor::ModeLogarithm);
        m_attackFreq.setId(IdAttackFreq);
        m_attackFreq.show();
        
        m_decayTime.setParent(this);
        m_decayTime.setRect(twRect(240, 86, 60, 13));
        m_decayTime.setValueRange(0.f, 10.f);
        m_decayTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_decayTime.setMode(ATValueEditor::ModeLogarithm);
        m_decayTime.setId(IdDecayTime);
        m_decayTime.show();
        
        m_sustainFreq.setParent(this);
        m_sustainFreq.setRect(twRect(240, 102, 60, 13));
        m_sustainFreq.setValueRange(1.f, 22050.f);
        m_sustainFreq.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_sustainFreq.setMode(ATValueEditor::ModeLogarithm);
        m_sustainFreq.setId(IdSustainFreq);
        m_sustainFreq.show();
        
        m_releaseTime.setParent(this);
        m_releaseTime.setRect(twRect(240, 118, 60, 13));
        m_releaseTime.setValueRange(0.f, 10.f);
        m_releaseTime.setDisplayMode(ATValueEditor::DisplayModePrefix);
        m_releaseTime.setMode(ATValueEditor::ModeLogarithm);
        m_releaseTime.setId(IdReleaseTime);
        m_releaseTime.show();
        
        m_releaseFreq.setParent(this);
        m_releaseFreq.setRect(twRect(240, 134, 60, 13));
        m_releaseFreq.setValueRange(1, 22050.f);
        m_releaseFreq.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_releaseFreq.setMode(ATValueEditor::ModeLogarithm);
        m_releaseFreq.setId(IdReleaseFreq);
        m_releaseFreq.show();
        
        
        loadParameters();
    }
    virtual ~FilterEditor(){
        
    }
    
    void setFilter(TXGPDS1::Filter *filter){
        m_filter=filter;
        loadParameters();
    }
    
    virtual void clientPaint(const twPaintStruct& p){
        twDC *dc=p.dc;
        const twFont *font=getFont();
        std::wstring str;
        twColor groupColor=twRGB(128, 128, 128);
        twColor lineColor=groupColor;
        twColor borderColor=twRGB(192, 192, 192);
        twColor graphColor=twRGB(128, 220, 32);
        twColor gridColor=twRGB(64, 64, 64);
        twColor labelColor=tw_curSkin->getWndTextColor();
        twRect r;
        
        font->render(dc, groupColor, twPoint(1,2),
                     L"Filter");
        font->render(dc, labelColor, twPoint(40, 18),
                     L"Resonance");
        font->render(dc, labelColor, twPoint(40, 2),
                     L"Type");
        
        dc->drawLine(lineColor, twPoint(1, 34), twPoint(318, 34));
        
        font->render(dc, groupColor, twPoint(1,38),
                     L"Envelope");
        
        font->render(dc, labelColor, twPoint(164,39),
                     L"Initial Freq");
        font->render(dc, labelColor, twPoint(301,39),
                     L"Hz");
        
        font->render(dc, labelColor, twPoint(164,55),
                     L"Attack Time");
        font->render(dc, labelColor, twPoint(301,55),
                     L"s");
        
        font->render(dc, labelColor, twPoint(164,71),
                     L"Attack Freq");
        font->render(dc, labelColor, twPoint(301,71),
                     L"Hz");
        
        font->render(dc, labelColor, twPoint(164,87),
                     L"Decay Time");
        font->render(dc, labelColor, twPoint(301,87),
                     L"s");
        
        font->render(dc, labelColor, twPoint(164,103),
                     L"Sustain Freq");
        font->render(dc, labelColor, twPoint(301,103),
                     L"Hz");
        
        font->render(dc, labelColor, twPoint(164,119),
                     L"Release Time");
        font->render(dc, labelColor, twPoint(301,119),
                     L"s");
        
        font->render(dc, labelColor, twPoint(164,135),
                     L"Release Freq");
        font->render(dc, labelColor, twPoint(301,135),
                     L"Hz");
        
        r=rectForEnvelopeGraph();
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
            
            
            float timeRange=m_filter->attackTime;
            float sustainTime;
            timeRange+=m_filter->decayTime;
            timeRange+=m_filter->releaseTime;
            if(timeRange<.2f)
                timeRange=.2f;
            sustainTime=timeRange*.1f;
            timeRange+=sustainTime;
            
            float timeScale=(float)(r.w-1)/(float)timeRange;
            float freqScale=(float)(r.h-1)/22050.f;
            
            // freq grids.
            dc->drawLine(gridColor, twPoint(r.x, r.y), 
                         twPoint(r.x+r.w, r.y));
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h/4), 
                         twPoint(r.x+r.w, r.y+r.h/4));
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h*2/4), 
                         twPoint(r.x+r.w, r.y+r.h*2/4));
            dc->drawLine(gridColor, twPoint(r.x, r.y+r.h*3/4), 
                         twPoint(r.x+r.w, r.y+r.h*3/4));
            
            // time grids.
            float timeGridInterval;
            if(timeRange<.5f){
                timeGridInterval=.1f;
            }else if(timeRange<1.f){
                timeGridInterval=.2f;
            }else if(timeRange<2.f){
                timeGridInterval=.5f;
            }else if(timeRange<5.f){
                timeGridInterval=1.f;
            }else if(timeRange<10.f){
                timeGridInterval=2.f;
            }else if(timeRange<20.f){
                timeGridInterval=5.f;
            }else{
                timeGridInterval=10.f;
            }
            for(float time=0;time<timeRange;time+=timeGridInterval){
                int x=r.x;
                x+=(int)(time*timeScale);
                dc->drawLine(gridColor, twPoint(x, r.y-8),
                             twPoint(x, r.y+r.h));
            }
            
            // envelope.
            drawFreqGraph(dc, r, graphColor, timeScale, freqScale, 
                          0.f, m_filter->attackTime, 
                          m_filter->initialFreq, 
                          m_filter->attackFreq);
            drawFreqGraph(dc, r, graphColor, timeScale, freqScale, 
                          m_filter->attackTime+m_filter->decayTime,
                          m_filter->attackTime, 
                          m_filter->sustainFreq, 
                          m_filter->attackFreq);
            drawFreqGraph(dc, r, graphColor, timeScale, freqScale, 
                          m_filter->attackTime+m_filter->decayTime,
                          timeRange-m_filter->releaseTime, 
                          m_filter->sustainFreq, 
                          m_filter->sustainFreq);
            drawFreqGraph(dc, r, graphColor, timeScale, freqScale, 
                          timeRange,
                          timeRange-m_filter->releaseTime, 
                          m_filter->releaseFreq, 
                          m_filter->sustainFreq);
            
            if(timeRange>=1.f)
                str=NHFormatStd(L"%.2fs", timeRange);
            else
                str=NHFormatStd(L"%dms", (int)(timeRange*1000.f));
            twSize sz=font->measure(str);
            font->render(dc, labelColor, twPoint(r.x+r.w-sz.w-3,
                                                 r.y+2), 
                         str);
            
            dc->setClipRect(oldClipRect);
        }
                
    }
    
    virtual void command(int id){
        if(id==IdResonance){
            m_filter->resonance=m_resonance.value();
            setParameters();
        }else if(id==IdFilterType){
            switch(m_filterType.selectedIndex()){
                case 0:
                    m_filter->enable=false;
                    break;
                case 1:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeLpf6LC;
                    break;
                case 2:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeLpf12LC;
                    break;
				case 3:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeLpf12SID;
                    break;
                case 4:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeLpf24Moog;
                    break;
                case 5:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeHpf6LC;
                    break;
                case 6:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeHpf12LC;
                    break;
				case 7:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeHpf12SID;
                    break;
				case 8:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeBpf6SID;
                    break;
				case 9:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeNotch6SID;
                    break;
                /*case 8:
                    m_filter->enable=true;
                    m_filter->filterType=TXGPDS1::FilterTypeHpf24Moog;
                    break;*/
            }
            setParameters();
        }else if(id==IdInitialFreq){
            m_filter->initialFreq=m_initialFreq.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdAttackTime){
            m_filter->attackTime=m_attackTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdAttackFreq){
            m_filter->attackFreq=m_attackFreq.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdDecayTime){
            m_filter->decayTime=m_decayTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdSustainFreq){
            m_filter->sustainFreq=m_sustainFreq.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdReleaseTime){
            m_filter->releaseTime=m_releaseTime.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }else if(id==IdReleaseFreq){
            m_filter->releaseFreq=m_releaseFreq.value();
            setParameters();
            invalidateClientRect(rectForEnvelopeGraph());
        }
    }
    
};

#pragma mark - Routing Editor

static twPoint scalePoint(twPoint pt, float v){
    pt.x=(int)((float)pt.x*v);
    pt.y=(int)((float)pt.y*v);
    return pt;
}

class ATGPDS1Editor::RoutingEditor: public twWnd{
    enum{
        IdVolume,
        IdPolyphonics,
        IdPolyphonicsMode,
        IdAnalog
    };
    
    enum DragMode{
        DragNone,
        DragTriangle
    } m_drag;
    
    ATGPDS1Editor *m_editor;
    TXGPDS1::Parameter *m_parameter;
    
    ATValueEditor m_volume;
    ATValueEditor m_polyphonics;
    ATDropdownList m_polyphonicsMode;
    ATValueEditor m_analog;
    float m_gain2, m_gain3; // gain1 = 1 - gain2 - gain3
    
    void loadParameters(){
        if(!m_parameter)
            return;
        float total=m_parameter->osc1.volume+
        m_parameter->osc2.volume+
        m_parameter->osc3.volume;
        m_volume.setValue(total);
        
        m_gain2=m_parameter->osc2.volume/total;
        m_gain3=m_parameter->osc3.volume/total;
        invalidateClientRect(rectForTriangle().inflate(30));
        
        m_polyphonics.setValue(m_parameter->polyphonics);
        invalidateClientRect(twRect(282, 43, 40, 13));
        
        switch(m_parameter->polyphonicsMode){
            case TXGPDS1::PolyphonicsModePoly:
                m_polyphonicsMode.setSelectedIndex(0);
                break;
            case TXGPDS1::PolyphonicsModePolyUnison:
                m_polyphonicsMode.setSelectedIndex(1);
                break;
            case TXGPDS1::PolyphonicsModeMono:
                m_polyphonicsMode.setSelectedIndex(2);
                break;
        }
        
        m_analog.setValue(m_parameter->maxSpread);
    }
    
    void setParameters(){
        sendCmdToParent();
    }
    
    void updateGainParameters(){
        float total=m_volume.value();
        m_parameter->osc1.volume=total*(1.f-m_gain2-m_gain3);
        m_parameter->osc2.volume=total*m_gain2;
        m_parameter->osc3.volume=total*m_gain3;
    }
    
    twRect rectForTriangle() const{
        return twRect(50, 40, 110, 95);
    }
    
    bool triangleHitTest(const twPoint& pt){
        twRect rt=rectForTriangle().inflate(3);
        if(rt && pt){
            int ww=rt.w*(pt.y-rt.y)/rt.h;
            int left=rt.x+(rt.w-ww)/2;
            int right=left+ww;
            if(pt.x>=left && pt.x<right)
                return true;
        }
        return false;
    }
    
    void setGainForPoint(const twPoint& pt){
        twRect rt=rectForTriangle();
        float invGain1=(float)(pt.y-rt.y)/(float)rt.h;
        int ww=rt.w*(pt.y-rt.y)/rt.h;
        if(invGain1<0.f)
            invGain1=0.f;
        if(invGain1>1.f)
            invGain1=1.f;
        if(ww<=0){
            // prevent overflow.
            m_gain2=0.f;
            m_gain3=0.f;
        }else{
            int left=rt.x+(rt.w-ww)/2;
            int right=left+ww;
            m_gain3=(float)(pt.x-left)*invGain1/(float)(right-left);
            if(m_gain3>invGain1)
                m_gain3=invGain1;
            if(m_gain3<0.f)
                m_gain3=0.f;
            m_gain2=invGain1-m_gain3;
        }
        updateGainParameters();
        setParameters();
        invalidateClientRect(rectForTriangle().inflate(30));
    }
    
public:
    RoutingEditor(ATGPDS1Editor *editor){
        m_editor=editor;
        m_parameter=NULL;
        m_drag=DragNone;
        
        m_volume.setParent(this);
        m_volume.setRect(twRect(240, 22, 40, 13));
        m_volume.setValueRange(0.01f, 1.f);
        m_volume.setDisplayMode(ATValueEditor::DisplayModeDecibel);
        m_volume.setMode(ATValueEditor::ModeLogarithm);
        m_volume.setId(IdVolume);
        m_volume.show();
        
        m_polyphonics.setParent(this);
        m_polyphonics.setRect(twRect(240, 42, 40, 13));
        m_polyphonics.setValueRange(1, 8);
#ifndef AT_EMBEDDED
        m_polyphonics.setValueRange(1, 32);
#endif
        m_polyphonics.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_polyphonics.setMode(ATValueEditor::ModeInteger);
        m_polyphonics.setId(IdPolyphonics);
        m_polyphonics.show();
        
        m_polyphonicsMode.setParent(this);
        m_polyphonicsMode.setRect(twRect(240, 62, 70, 13));
        m_polyphonicsMode.items().push_back(L"Poly");
        m_polyphonicsMode.items().push_back(L"Poly Unison");
        m_polyphonicsMode.items().push_back(L"Mono");
        m_polyphonicsMode.setId(IdPolyphonicsMode);
        m_polyphonicsMode.show();
        
        m_analog.setParent(this);
        m_analog.setRect(twRect(240, 92, 40, 13));
        m_analog.setValueRange(0.0f, 100.f);
        m_analog.setDisplayMode(ATValueEditor::DisplayModeReal);
        m_analog.setMode(ATValueEditor::ModeLogarithm);
        m_analog.setId(IdAnalog);
        m_analog.show();

                
        loadParameters();
    }
    virtual ~RoutingEditor(){
        
    }
    
    void setParameter(TXGPDS1::Parameter *parameter){
        m_parameter=parameter;
        loadParameters();
    }
    
    virtual void clientPaint(const twPaintStruct& p){
        twDC *dc=p.dc;
        const twFont *font=getFont();
        std::wstring str;
        twColor groupColor=twRGB(128, 128, 128);
        //twColor lineColor=groupColor;
        twColor borderColor=twRGB(192, 192, 192);
        twColor graphColor=twRGB(128, 220, 32);
        twColor gridColor=twRGB(64, 64, 64);
        twColor labelColor=tw_curSkin->getWndTextColor();
        twRect r;
        twSize size;
        twPoint pt;
        
        font->render(dc, groupColor, twPoint(1,2),
                     L"Oscillators");
        font->render(dc, labelColor, twPoint(180, 23),
                     L"Volume");
        font->render(dc, labelColor, twPoint(282, 23),
                     L"db");
        font->render(dc, labelColor, twPoint(180, 43),
                     L"Polyphonics");
        if(m_polyphonics.intValue()>1)
            font->render(dc, labelColor, twPoint(282, 43),
                         L"notes");
        else
            font->render(dc, labelColor, twPoint(282, 43),
                        L"note");
        font->render(dc, labelColor, twPoint(180, 63),
                     L"Mode");
        font->render(dc, labelColor, twPoint(180, 93),
                     L"Spread");
        font->render(dc, labelColor, twPoint(282, 93),
                     L"cents");
        
        float gain1=1.f-m_gain2-m_gain3;
        float gain2=m_gain2;
        float gain3=m_gain3;
        
        r=rectForTriangle().inflate(4);
        if(r&&p.paintRect){
            
            r=rectForTriangle();
            
            
            
            // vertices
            twPoint p1, p2, p3;
            p1=twPoint(r.x+r.w/2, r.y);
            p2=twPoint(r.x, r.y+r.h);
            p3=twPoint(r.x+r.w, r.y+r.h);
            
            // control point
            twPoint p=p1;
            p+=scalePoint(p2-p1, gain2);
            p+=scalePoint(p3-p1, gain3);
            
            // grid
            twColor color=gridColor;
            if(m_drag==DragTriangle)
                color=tw_curSkin->getSelectionColor();
            dc->drawLine(color, p1+scalePoint(p2-p1, gain2),
                         p3+scalePoint(p2-p3, gain2));
            dc->drawLine(color, p1+scalePoint(p3-p1, gain3),
                         p2+scalePoint(p3-p2, gain3));
            dc->drawLine(color, p2+scalePoint(p1-p2, gain1),
                         p3+scalePoint(p1-p3, gain1));
            
            
            dc->drawLine(borderColor, p1, p2);
            dc->drawLine(borderColor, p2, p3);
            dc->drawLine(borderColor, p3, p1);
            
            
            
            dc->fillRect(labelColor, twRect(p.x-2, p.y-1, 5, 3));
            dc->fillRect(labelColor, twRect(p.x-1, p.y-2, 3, 5));
            dc->fillRect(graphColor, twRect(p.x-1, p.y-1, 3, 3));
            
            
        }
        
        r=rectForTriangle().inflate(2);
        
        pt=twPoint(r.x+r.w/2, r.y);
        str=L"Oscillator1";
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y-size.h*2), str);
        str=NHFormatStd(L"%d%lc", (int)(100.f*gain1+
                                       100.f/1025.f), L'%');
        size=font->measure(str);
        font->render(dc, (gain1<(1.f/1024.f))?groupColor:labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y-size.h), str);
        
        pt=twPoint(r.x, r.y+r.h);
        str=L"Oscillator2";
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y+size.h), str);
        str=NHFormatStd(L"%d%lc", (int)(100.f*gain2+
                                        100.f/1025.f), L'%');
        size=font->measure(str);
        font->render(dc, (gain2<(1.f/1024.f))?groupColor:labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y), str);
        
        pt=twPoint(r.x+r.w, r.y+r.h);
        str=L"Oscillator3";
        size=font->measure(str);
        font->render(dc, labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y+size.h), str);
        str=NHFormatStd(L"%d%lc", (int)(100.f*gain3+
                                        100.f/1025.f), L'%');
        size=font->measure(str);
        font->render(dc, (gain3<(1.f/1024.f))?groupColor:labelColor, 
                     twPoint(pt.x-size.w/2,
                             pt.y), str);
        
        
    }
    
    virtual void clientMouseDown(const twPoint& pt, twMouseButton mb){
        if(mb!=twMB_left)
            return;
        
        twSetCapture(this);
        
        if(triangleHitTest(pt)){
            m_drag=DragTriangle;
            setGainForPoint(pt);
        }
        
    }
	virtual void clientMouseMove(const twPoint& pt){
        if(m_drag==DragTriangle){
            setGainForPoint(pt);
        }
    }
	virtual void clientMouseUp(const twPoint& pt, twMouseButton mb){
        twReleaseCapture();
        if(m_drag==DragTriangle){
            invalidateClientRect(rectForTriangle().inflate(4));
        }
        m_drag=DragNone;
    }
    
    virtual void command(int id){
       if(id==IdVolume){
           updateGainParameters();
            setParameters();
       }else if(id==IdPolyphonics){
           m_parameter->polyphonics=m_polyphonics.intValue();
           invalidateClientRect(twRect(282, 43, 40, 13));
           setParameters();
       }else if(id==IdPolyphonicsMode){
           switch (m_polyphonicsMode.selectedIndex()) {
               case 0:
                   m_parameter->polyphonicsMode=TXGPDS1::PolyphonicsModePoly;
                   break;
               case 1:
                   m_parameter->polyphonicsMode=TXGPDS1::PolyphonicsModePolyUnison;
                   break;
               case 2:
                   m_parameter->polyphonicsMode=TXGPDS1::PolyphonicsModeMono;
                   break;
           }
           setParameters();
       }else if(id==IdAnalog){
           m_parameter->maxSpread=m_analog.value();
           setParameters();
       }
    }
    
    virtual bool clientHitTest(const twPoint&) const{
        return true;
    }
    
};

#pragma mark - ATGPDS1Editor

ATGPDS1Editor::ATGPDS1Editor(){
    m_plugin=NULL;
    
    m_tabView=new ATTabView();
    m_tabView->setTabCount(5);
    
    {
        delete m_tabView->tabAtIndex(0);
        m_oscilatorEditors[0]=new OscilatorEditor(this);
        m_tabView->setTabAtIndex
        (0, m_oscilatorEditors[0]);
        twWnd *tab=m_tabView->tabAtIndex(0);
        tab->setTitle(L"Oscillator 1");
    }
    {
        delete m_tabView->tabAtIndex(1);
        m_oscilatorEditors[1]=new OscilatorEditor(this);
        m_tabView->setTabAtIndex
        (1, m_oscilatorEditors[1]);
        twWnd *tab=m_tabView->tabAtIndex(1);
        tab->setTitle(L"Oscillator 2");
    }
    {
        delete m_tabView->tabAtIndex(2);
        m_oscilatorEditors[2]=new OscilatorEditor(this);
        m_tabView->setTabAtIndex
        (2, m_oscilatorEditors[2]);
        twWnd *tab=m_tabView->tabAtIndex(2);
        tab->setTitle(L"Oscillator 3");
    }
    {
        delete m_tabView->tabAtIndex(3);
        m_filterEditor=new FilterEditor(this);
        m_tabView->setTabAtIndex
        (3, m_filterEditor);
        twWnd *tab=m_tabView->tabAtIndex(3);
        tab->setTitle(L"Filter");
    }
    {
        delete m_tabView->tabAtIndex(4);
        m_routingEditor=new RoutingEditor(this);
        m_tabView->setTabAtIndex
        (4, m_routingEditor);
        twWnd *tab=m_tabView->tabAtIndex(4);
        tab->setTitle(L"Routing");
    }
    
    m_tabView->setRect(rectForTabView());
    m_tabView->setParent(this);
    m_tabView->show();
}

ATGPDS1Editor::~ATGPDS1Editor(){
    delete m_tabView;
}



#pragma mark - Main Geometry

twRect ATGPDS1Editor::rectForTabView() const{
    return twRect(0, 0, 320, 200);
}

void ATGPDS1Editor::clientPaint(const twPaintStruct &p){
    
}

void ATGPDS1Editor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXGPDS1 *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXGPDS1 *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATGPDS1Editor::loadParameters(){
    m_oscilatorEditors[0]->setOscilator(&m_parameter.osc1);
    m_oscilatorEditors[1]->setOscilator(&m_parameter.osc2);
    m_oscilatorEditors[2]->setOscilator(&m_parameter.osc3);
    m_filterEditor->setFilter(&m_parameter.flt);
    m_routingEditor->setParameter(&m_parameter);
}

void ATGPDS1Editor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock2(synth->renderSemaphore());
    twLock lock(synth->synthesizerSemaphore());
    
    m_plugin->setParameter(m_parameter);
}

void ATGPDS1Editor::command(int id){
    setParameters();
}

