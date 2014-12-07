//
//  ATOverdriveFilterEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "ATOverdriveFilterEditor.h"
#include "ATSynthesizer.h"
#include "Utils.h"
#include "TXSineTable.h"

enum{
    IdMix=1,
	IdTone,
	IdDrive,
	IdGain
};

ATOverdriveFilterEditor::ATOverdriveFilterEditor(){
    m_plugin=NULL;
    
    m_mix.setParent(this);
    m_mix.setRect(twRect(119, 1, 40, 13));
    m_mix.setValueRange(0.f, 1.f);
    m_mix.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_mix.setMode(ATValueEditor::ModeLinear);
    m_mix.setId(IdMix);
    m_mix.show();
    
    m_tone.setParent(this);
    m_tone.setRect(twRect(245, 1, 40, 13));
    m_tone.setValueRange(0.f, 1.f);
    m_tone.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_tone.setMode(ATValueEditor::ModeLinear);
    m_tone.setId(IdTone);
    m_tone.show();
    
    m_drive.setParent(this);
    m_drive.setRect(twRect(119, 17, 40, 13));
    m_drive.setValueRange(0.0f, 64.f);
    m_drive.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_drive.setMode(ATValueEditor::ModeLogarithm);
    m_drive.setId(IdDrive);
    m_drive.show();
    
    m_gain.setParent(this);
    m_gain.setRect(twRect(245, 17, 40, 13));
    m_gain.setValueRange(0.f, 64.f);
    m_gain.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_gain.setMode(ATValueEditor::ModeLogarithm);
    m_gain.setId(IdGain);
    m_gain.show();
    
}

ATOverdriveFilterEditor::~ATOverdriveFilterEditor(){
	
}



#pragma mark - Main Geometry

twRect ATOverdriveFilterEditor::rectForGraph() const{
    return twRect(4, 32, 312, 164);
}

void ATOverdriveFilterEditor::clientPaint(const twPaintStruct &p){
    twRect r;
    const twFont *font=getFont();
    std::wstring str;
    twSize size;
    twColor groupColor=twRGB(128, 128, 128);
    //twColor lineColor=groupColor;
    twColor borderColor=twRGB(192, 192, 192);
    twColor graphColor=twRGB(128, 220, 32);
	twColor graphSubColor=twRGB(200, 90, 0);
    twColor gridColor=twRGB(64, 64, 64);
    twColor labelColor=tw_curSkin->getWndTextColor();
    twDC *dc=p.dc;
    
    font->render(dc, groupColor, twPoint(1,2),
                 L"Overdrive");
    font->render(dc, labelColor, twPoint(65,2),
                 L"Wet Mix");
    font->render(dc, labelColor, twPoint(160,2),
                 L"%");
    font->render(dc, labelColor, twPoint(181,2),
                 L"Tone");
    font->render(dc, labelColor, twPoint(286,2),
                 L"%");
    font->render(dc, labelColor, twPoint(65,18),
                 L"In Gain");
	font->render(dc, labelColor, twPoint(160,18),
                 L"db");
    font->render(dc, labelColor, twPoint(181,18),
                 L"Out Gain");
    font->render(dc, labelColor, twPoint(286,18),
                 L"db");
    
    r=rectForGraph();
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
        
        for(int i=1;i<8;i++){
            int x=r.x+(r.w*i)/8;
            dc->drawLine(gridColor, twPoint(x, r.y), 
                         twPoint(x, r.y+r.h));
        }
        for(int i=1;i<4;i++){
            int y=r.y+(r.h*i)/4;
            dc->drawLine(gridColor, twPoint(r.x, y), 
                         twPoint(r.x+r.w, y));
        }
        
        twPoint lastFilteredWavePoint;
		twPoint lastOutWavePoint;
		const int graphStepsShift=6;
        const int graphSteps=1<<graphStepsShift;
		float baseY=(float)(r.y+r.h/2);
		float scaleY=(float)(r.h/4);
		
		for(int i=0;i<=graphSteps;i++){
			int x=r.x+(r.w*i)/graphSteps;
			
			float sineWave=(float)TXSineWave(i<<(32-graphStepsShift))/32768.f;
			float triWave=(float)i/(float)graphSteps;
			if(triWave<.25f){
				triWave*=4.f;
			}else if(triWave<.75f){
				triWave=1.f-(triWave-.25f)*4.f;
			}else{
				triWave=(triWave-.75f)*4.f-1.f;
			}
			
			float inWave=triWave;
			float filteredWave=sineWave;
			filteredWave+=(triWave-filteredWave)*m_parameter.tone;
			
			filteredWave*=m_parameter.drive;
			
			if(filteredWave>1.f){
				filteredWave=1.f;
			}else if(filteredWave<-1.f){
				filteredWave=-1.f;
			}else{
				filteredWave-=filteredWave*filteredWave*
				filteredWave/3.f;
				filteredWave*=3.f/2.f;
			}
			filteredWave*=m_parameter.outGain;
			
			float outWave=inWave;
			outWave+=m_parameter.mix*(filteredWave-outWave);
			
			twPoint filteredWavePoint;
			filteredWavePoint.x=x;
			filteredWavePoint.y=(int)(baseY+scaleY*filteredWave);
			if(i>0){
				dc->drawLine(graphSubColor, lastFilteredWavePoint, 
							 filteredWavePoint);
			}
			lastFilteredWavePoint=filteredWavePoint;
			
			twPoint outWavePoint;
			outWavePoint.x=x;
			outWavePoint.y=(int)(baseY+scaleY*outWave);
			if(i>0){
				dc->drawLine(graphColor, lastOutWavePoint, 
							 outWavePoint);
			}
			lastOutWavePoint=outWavePoint;
			
			
			
		}
        
        
        dc->setClipRect(oldClipRect);
    }
}

void ATOverdriveFilterEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXOverdriveFilter *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXOverdriveFilter *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATOverdriveFilterEditor::loadParameters(){
    m_mix.setValue(m_parameter.mix);
    m_tone.setValue(m_parameter.tone);
    m_drive.setValue(m_parameter.drive);
    m_gain.setValue(m_parameter.outGain);
}

void ATOverdriveFilterEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATOverdriveFilterEditor::command(int id){
    if(id==IdMix){
        m_parameter.mix=m_mix.value();
        invalidateClientRect(rectForGraph());
        setParameters();
    }else if(id==IdTone){
        m_parameter.tone=m_tone.value();
        invalidateClientRect(rectForGraph());
        setParameters();
    }else if(id==IdDrive){
        m_parameter.drive=m_drive.value();
        invalidateClientRect(rectForGraph());
        setParameters();
    }else if(id==IdGain){
        m_parameter.outGain=m_gain.value();
		invalidateClientRect(rectForGraph());
        setParameters();
    }
}


