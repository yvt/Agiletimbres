//
//  ATLegacyMidiEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "ATLegacyMidiEditor.h"
#include "ATValueEditor.h"
#include "ATTabView.h"
#include "ATSynthesizer.h"
#include "ATListView.h"
#include <tcw/twSDLDC.h>
#include "xpms/TxMidiLogo.xpm"
#include "XTBCommondlg.h"

static twDC *txmidiLogo(){
	twDC *dc=NULL;
	if(!dc)
		dc=twSDLDC::loadFromXPM(TxMidiLogo);
	return dc;
}

#pragma mark - Routing Editor

static twPoint scalePoint(twPoint pt, float v){
    pt.x=(int)((float)pt.x*v);
    pt.y=(int)((float)pt.y*v);
    return pt;
}


class ATLegacyMidiEditor::VoiceEditor: public twWnd{
    enum{
        IdProgram,
        IdBank
    };
    
    ATLegacyMidiEditor *m_editor;
    TXLegacyMidi::Parameter *m_parameter;
    
    ATListView m_program;
	ATListView m_bank;
	
	std::vector<int> m_availableBanks;
    
	void updateBank(){
		int program=m_program.selectedIndex()-1;
		m_bank.setSelectedIndex(-1);
		
		m_availableBanks.clear();
		for(int i=0;i<128;i++){
			if(program==-1){
				if(TXLegacyMidi::isDrumsetAvailable(i)){
					m_availableBanks.push_back(i);
				}
			}else{
				if(TXLegacyMidi::isVoiceAvailable(i, program)){
					m_availableBanks.push_back(i);
				}
			}
		}
			
		
		m_bank.items().clear();
		for(size_t i=0;i<m_availableBanks.size();i++){
			int bank=m_availableBanks[i];
			if(program==-1){
				m_bank.items().push_back(twM2W(TXLegacyMidi::nameForDrumset(bank)));
			}else{
				m_bank.items().push_back(twM2W(TXLegacyMidi::nameForVoice(bank, program)));
			}
		}
		m_bank.reloadData();
		
	}
	
    void loadParameters(){
        if(!m_parameter)
            return;
		if(m_parameter->isDrum)
			m_program.setSelectedIndex(0);
		else
			m_program.setSelectedIndex(m_parameter->program+1);
		
		
		updateBank();
        
		int bank=m_parameter->bank;
		if(m_parameter->isDrum)
			bank=m_parameter->program;
		for(size_t i=0;i<m_availableBanks.size();i++){
			if(m_availableBanks[i]==bank){
				m_bank.setSelectedIndex(i);
			}
		}
    }
    
    void setParameters(){
        sendCmdToParent();
    }
	
	volatile bool workerWorking;
	
public:
    VoiceEditor(ATLegacyMidiEditor *editor){
        m_editor=editor;
        m_parameter=NULL;
        
        m_program.setParent(this);
        m_program.setRect(twRect(0, 0, 159, 186));
        m_program.setId(IdProgram);
		m_program.items().push_back(L"DRUMSET");
		for(int i=0;i<128;i++)
			m_program.items().push_back(twM2W(TXLegacyMidi::nameForVoice(0, i)));
		m_program.reloadData();
		m_program.setSelectedIndex(1);
        m_program.show();
        
        m_bank.setParent(this);
        m_bank.setRect(twRect(160, 0, 160, 186));
        m_bank.setId(IdBank);
        m_bank.show();
		
        loadParameters();
		
		workerWorking=false;
    }
    virtual ~VoiceEditor(){
        
    }
    
    void setParameter(TXLegacyMidi::Parameter *parameter){
        m_parameter=parameter;
        loadParameters();
    }
    
    virtual void clientPaint(const twPaintStruct& p){
      /*  twDC *dc=p.dc;
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
        twPoint pt;*/
		
        
        
    }
	
	void purgeSamples(){
		{
			twLock lock2(ATSynthesizer::sharedSynthesizer()->renderSemaphore());
			twLock lock(ATSynthesizer::sharedSynthesizer()->synthesizerSemaphore());
			TXLegacyMidi *midi=(TXLegacyMidi *)m_editor->m_plugin;
			midi->purgeAllSamples();
		}
	}
	
	std::wstring currentVoiceName(){
		if(m_program.selectedIndex()==0){
			if(m_bank.selectedIndex()==-1 ||
			   m_bank.selectedIndex()>=(int)m_availableBanks.size()){
				return L"DRUMSET";
			}else{
				return m_bank.items()[m_bank.selectedIndex()];
			}
		}else{
			if(m_bank.selectedIndex()==-1 ||
			   m_bank.selectedIndex()>=(int)m_availableBanks.size()){
				return m_program.items()[m_program.selectedIndex()];
			}else{
				return m_bank.items()[m_bank.selectedIndex()];
			}
		}
	}
	
	
	
	void loadVoiceWorker(){
		
		XTBSetProgressText(L"Loading "+currentVoiceName());
		ATSynthesizer::sharedSynthesizer()->setMute(true);
		purgeSamples();
		setParameters();
		ATSynthesizer::sharedSynthesizer()->setMute(false);
		workerWorking=false;
	}
    
    virtual void command(int id){
		if(id==IdBank){
			if(m_bank.selectedIndex()==-1)
				return;
			if(m_bank.selectedIndex()>=(int)m_availableBanks.size())
				return;
			
			if(m_program.selectedIndex()==0){
				
				// drum
				
				if(m_parameter->program==m_availableBanks[m_bank.selectedIndex()])
					return;
				m_parameter->program=m_availableBanks[m_bank.selectedIndex()];
				m_parameter->bank=0;
				
			}else{
			
				if(m_parameter->bank==m_availableBanks[m_bank.selectedIndex()])
					return;
				m_parameter->program=m_program.selectedIndex()-1;
				m_parameter->bank=m_availableBanks[m_bank.selectedIndex()];
					
			}
			
			if(workerWorking)
				return;
			workerWorking=true;
			XTBInvokeWithProgressOverlay(new twNoArgumentMemberFunctionInvocation<VoiceEditor>
										 (this, &VoiceEditor::loadVoiceWorker));
		}else if(id==IdProgram){
			if(m_program.selectedIndex()==0){
				if(m_parameter->isDrum)
					return;
			}else{
				if(m_parameter->program==m_program.selectedIndex()-1 &&
				   !m_parameter->isDrum)
					return;
			}
			updateBank();
			if(!m_availableBanks.empty()){
				m_bank.setSelectedIndex(0);
				if(m_program.selectedIndex()==0){
					m_parameter->program=0;
					m_parameter->bank=0;
					m_parameter->isDrum=true;
				}else{
					m_parameter->program=m_program.selectedIndex()-1;
					m_parameter->bank=0;
					m_parameter->isDrum=false;
				}
				if(workerWorking)
					return;
				workerWorking=true;
				XTBInvokeWithProgressOverlay(new twNoArgumentMemberFunctionInvocation<VoiceEditor>
											 (this, &VoiceEditor::loadVoiceWorker));
			}
		}
    }
    
    virtual bool clientHitTest(const twPoint&) const{
        return true;
    }
    
};

class ATLegacyMidiEditor::RoutingEditor: public twWnd{
    enum{
        IdVolume,
        IdPolyphonics,
		IdTransposeCoarse,
		IdTransposeFine
    };
    
    ATLegacyMidiEditor *m_editor;
    TXLegacyMidi::Parameter *m_parameter;
    
    ATValueEditor m_volume;
    ATValueEditor m_polyphonics;
	ATValueEditor m_transposeCoarse;
	ATValueEditor m_transposeFine;
    
    void loadParameters(){
        if(!m_parameter)
            return;
        m_volume.setValue(m_parameter->volume);
		m_polyphonics.setValue(m_parameter->polyphonics);
        invalidateClientRect(twRect(282, 43, 40, 13));
		
		m_transposeCoarse.setValue(m_parameter->transposeCorase);
		m_transposeFine.setValue(m_parameter->transposeFine);
        
    }
    
    void setParameters(){
        sendCmdToParent();
    }
        
public:
    RoutingEditor(ATLegacyMidiEditor *editor){
        m_editor=editor;
        m_parameter=NULL;
        
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
		
		m_transposeCoarse.setParent(this);
        m_transposeCoarse.setRect(twRect(70, 22, 40, 13));
        m_transposeCoarse.setValueRange(-36, 36);
        m_transposeCoarse.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_transposeCoarse.setMode(ATValueEditor::ModeInteger);
        m_transposeCoarse.setId(IdTransposeCoarse);
        m_transposeCoarse.show();
		
		m_transposeFine.setParent(this);
        m_transposeFine.setRect(twRect(70, 42, 40, 13));
        m_transposeFine.setValueRange(-100, 100);
        m_transposeFine.setDisplayMode(ATValueEditor::DisplayModeInteger);
        m_transposeFine.setMode(ATValueEditor::ModeInteger);
        m_transposeFine.setId(IdTransposeFine);
        m_transposeFine.show();
        
		
        loadParameters();
    }
    virtual ~RoutingEditor(){
        
    }
    
    void setParameter(TXLegacyMidi::Parameter *parameter){
        m_parameter=parameter;
        loadParameters();
    }
    
    virtual void clientPaint(const twPaintStruct& p){
        twDC *dc=p.dc;
        const twFont *font=getFont();
        std::wstring str;
        twColor groupColor=twRGB(128, 128, 128);
        twColor lineColor=groupColor;/*
        twColor borderColor=twRGB(192, 192, 192);
        twColor graphColor=twRGB(128, 220, 32);
        twColor gridColor=twRGB(64, 64, 64);*/
        twColor labelColor=tw_curSkin->getWndTextColor();
        twRect r;
        twSize size;
        twPoint pt;
       
        font->render(dc, groupColor, twPoint(1,2),
                     L"Global");
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
		
		font->render(dc, labelColor, twPoint(10, 23),
                     L"Transpose");
        font->render(dc, labelColor, twPoint(112, 23),
                     L"st");
		font->render(dc, labelColor, twPoint(112, 43),
                     L"cents");
		
		dc->bitBlt(txmidiLogo(), twPoint(310-120, 176-58), 
				   twRect(0, 0, 120, 58));
		
		dc->drawLine(lineColor, twPoint(1, 80), twPoint(318, 80));
        
    }
    
    virtual void command(int id){
		if(id==IdVolume){
			m_parameter->volume=m_volume.value();
            setParameters();
		}else if(id==IdPolyphonics){
			m_parameter->polyphonics=m_polyphonics.intValue();
			invalidateClientRect(twRect(282, 43, 40, 13));
			setParameters();
		}else if(id==IdTransposeCoarse){
			m_parameter->transposeCorase=m_transposeCoarse.intValue();
			setParameters();
		}else if(id==IdTransposeFine){
			m_parameter->transposeFine=m_transposeFine.intValue();
			setParameters();
		}
    }
    
    virtual bool clientHitTest(const twPoint&) const{
        return true;
    }
    
};

#pragma mark - ATLegacyMidiEditor

ATLegacyMidiEditor::ATLegacyMidiEditor(){
    m_plugin=NULL;
    
    m_tabView=new ATTabView();
    m_tabView->setTabCount(2);
    

    {
        delete m_tabView->tabAtIndex(0);
        m_voiceEditor=new VoiceEditor(this);
        m_tabView->setTabAtIndex
        (0, m_voiceEditor);
        twWnd *tab=m_tabView->tabAtIndex(0);
        tab->setTitle(L"Voice");
    }
    {
        delete m_tabView->tabAtIndex(1);
        m_routingEditor=new RoutingEditor(this);
        m_tabView->setTabAtIndex
        (1, m_routingEditor);
        twWnd *tab=m_tabView->tabAtIndex(1);
        tab->setTitle(L"Parameter");
    }
    
    m_tabView->setRect(rectForTabView());
    m_tabView->setParent(this);
    m_tabView->show();
}

ATLegacyMidiEditor::~ATLegacyMidiEditor(){
    delete m_tabView;
}



#pragma mark - Main Geometry

twRect ATLegacyMidiEditor::rectForTabView() const{
    return twRect(0, 0, 320, 200);
}

void ATLegacyMidiEditor::clientPaint(const twPaintStruct &p){
    
}

void ATLegacyMidiEditor::setSelectedPlugin(TXPlugin *plugin){
    if(!plugin){
        // detach
        m_plugin=NULL;
        return;
    }
    
    assert(dynamic_cast<TXLegacyMidi *>(plugin)!=NULL);
    
    m_plugin=static_cast<TXLegacyMidi *>(plugin);
    m_parameter=m_plugin->parameter();
    
    loadParameters();
}

void ATLegacyMidiEditor::loadParameters(){
    m_voiceEditor->setParameter(&m_parameter);
    m_routingEditor->setParameter(&m_parameter);
}

void ATLegacyMidiEditor::setParameters(){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twLock lock2(synth->renderSemaphore());
    twLock lock(synth->synthesizerSemaphore());
    
    m_plugin->setParameter(m_parameter);
}

void ATLegacyMidiEditor::command(int id){
    setParameters();
}


