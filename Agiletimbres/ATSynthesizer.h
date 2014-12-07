//
//  ATSynthesizer.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"
#include "TXInstrument.h"
#include "TXMatrixReverb1.h"
#include "TXChorusFilter.h"

class ATSynthesizer{
protected:
    ATSynthesizer();
public:
    static ATSynthesizer *sharedSynthesizer();
    
    TXInstrument *instrument();
    void setInstrument(TXInstrument *);
    
    TXEffect *reverb();
    TXEffect *chorus();
    
    void setReverbEnable(bool);
    bool reverbEnable();
	void setChorus(TXEffect *);
    void setChorusEnable(bool);
    bool chorusEnable();
	
	void setMute(bool);
    
    twSemaphore *renderSemaphore();
    twSemaphore *synthesizerSemaphore();
    
    void note(int key, int velocity,
              int duration);
    void noteOn(int key, int velocity);
    void noteOff(int key, int velocity);
    void setPitchbend(int millicents);
    
    void render(int16_t *outBuffer,
                unsigned int samples);
    
    TXConfig config();
    
    
    float processorLoad();

    short *waveform();
};
