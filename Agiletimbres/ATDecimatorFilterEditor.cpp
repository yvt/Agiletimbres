//
//  ATDecimatorFilterEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/4/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "ATDecimatorFilterEditor.h"
#include "ATSynthesizer.h"
#include "Utils.h"
#include "TXSineTable.h"

enum{
    IdMix=1,
	IdDrive,
	IdGain,
	IdBits,
	IdDownsample
};

ATDecimatorFilterEditor::ATDecimatorFilterEditor(){
    m_plugin=NULL;
    
    m_mix.setParent(this);
    m_mix.setRect(twRect(119, 1, 40, 13));
    m_mix.setValueRange(0.f, 1.f);
    m_mix.setDisplayMode(ATValueEditor::DisplayModePercent);
    m_mix.setMode(ATValueEditor::ModeLinear);
    m_mix.setId(IdMix);
    m_mix.show();
    
    
    m_drive.setParent(this);
    m_drive.setRect(twRect(32, 17, 40, 13));
    m_drive.setValueRange(0.0f, 64.f);
    m_drive.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_drive.setMode(ATValueEditor::ModeLogarithm);
    m_drive.setId(IdDrive);
    m_drive.show();
    
    m_gain.setParent(this);
    m_gain.setRect(twRect(140, 17, 40, 13));
    m_gain.setValueRange(0.f, 64.f);
    m_gain.setDisplayMode(ATValueEditor::DisplayModeDecibel);
    m_gain.setMode(ATValueEditor::ModeLogarithm);
    m_gain.setId(IdGain);
    m_gain.show();
	
	
    m_bits.setParent(this);
    m_bits.setRect(twRect(244, 1, 40, 13));
    m_bits.setValueRange(1, 16);
    m_bits.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_bits.setMode(ATValueEditor::ModeInteger);
    m_bits.setId(IdBits);
    m_bits.show();
	
	m_downsample.setParent(this);
    m_downsample.setRect(twRect(269, 17, 40, 13));
    m_downsample.setValueRange(1, 100);
    m_downsample.setDisplayMode(ATValueEditor::DisplayModeInteger);
    m_downsample.setMode(ATValueEditor::ModeInteger);
    m_downsample.setId(IdDownsample);
    m_downsample.show();
	
	
    
}

ATDecimatorFilterEditor::~ATDecimatorFilterEditor(){
	
}



#pragma mark - Main Geometry

twRect ATDecimatorFilterEditor::rectForGraph() const{
    return twRect(4, 32, 312, 164);
}

void ATDecimatorFilterEditor::clientPaint(const twPaintStruct &p){
    twRect r;
    const twFont *font=getFont();
    std::wstring str;
    twSize size;
    twColor groupColor=twRGB(128, 128, 128);
    //twColor lineColor=groupColor;
    twColor borderColor=twRGB(192, 192, 192);
    twColor graphColor=twRGB(128, 220, 32);
	//twColor graphSubColor=twRGB(200, 90, 0);
    twColor gridColor=twRGB(64, 64, 64);
    twColor labelColor=tw_curSkin->getWndTextColor();
    twDC *dc=p.dc;
    
    font->render(dc, groupColor, twPoint(1,2),
                 L"Decimator");
    font->render(dc, labelColor, twPoint(75,2),
                 L"Wet Mix");
    font->render(dc, labelColor, twPoint(160,2),
                 L"%");
    font->render(dc, labelColor, twPoint(187,2),
                 L"Resolution");
    font->render(dc, labelColor, twPoint(286,2),
                 L"bits");
    font->render(dc, labelColor, twPoint(2,18),
                 L"Drive");
	font->render(dc, labelColor, twPoint(74,18),
                 L"db");
    font->render(dc, labelColor, twPoint(92,18),
                 L"Out Gain");
    font->render(dc, labelColor, twPoint(182,18),
                 L"db");
	font->render(dc, labelColor, twPoint(201,18),
                 L"Downsample");
    font->render(dc, labelColor, twPoint(311,18),
                 L"x");
    
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
		
		twPoint lastPoint;
		unsigned int shift=16-m_parameter.bits;
		int mask=((1<<shift)-1)^0xffffffffU;
		int bias=(1<<shift)>>1;
		unsigned int downsampling=m_parameter.downsampling;
		uint32_t phase=0, phaseDelta;
		unsigned int stp=downsampling;
		int yScale=r.h*3/8;
		phaseDelta=0xffffffffU/r.w;
		phaseDelta*=downsampling;
		for(int x=0;x<r.w;x++){
			twPoint pt;
			pt.x=x+r.x;
			
			int sine=TXSineWave(phase);
			if(sine>0)
				sine=((sine+bias)&mask);
			else
				sine=-((-sine+bias)&mask);
			
			sine=(sine*yScale)>>15;
			sine+=r.y;
			sine+=r.h>>1;
			
			pt.y=sine;
			
			if(pt.y!=lastPoint.y || x==0){
				if(pt.x>lastPoint.x+1){
					dc->drawLine(graphColor, lastPoint, twPoint(pt.x-1, lastPoint.y));
					lastPoint=twPoint(pt.x-1, lastPoint.y);
				}
				if(x>0){
					dc->drawLine(graphColor, lastPoint, pt);
				}
				lastPoint=pt;
			}
			
			stp--;
			if(stp==0){
				stp=downsampling;
				phase+=phaseDelta;
			}
		}
		dc->drawLine(graphColor, lastPoint, twPoint(r.x+r.w, lastPoint.y));
		
        /*
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
        */
        
        dc->setClipRect(oldClipRect);
    }
}

void ATDecimatorFilterEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXDecimatorFilter *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXDecimatorFilter *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATDecimatorFilterEditor::loadParameters(){
    m_mix.setValue(m_parameter.mix);
    m_bits.setValue(m_parameter.bits);
    m_downsample.setValue(m_parameter.downsampling);
    m_drive.setValue(m_parameter.drive);
    m_gain.setValue(m_parameter.outGain);
}

void ATDecimatorFilterEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock(synth->renderSemaphore());
    m_plugin->setParameter(m_parameter);
}

void ATDecimatorFilterEditor::command(int id){
    if(id==IdMix){
        m_parameter.mix=m_mix.value();
        invalidateClientRect(rectForGraph());
        setParameters();
    }else if(id==IdBits){
        m_parameter.bits=m_bits.value();
        invalidateClientRect(rectForGraph());
        setParameters();
    }else if(id==IdDownsample){
        m_parameter.downsampling=m_downsample.value();
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
