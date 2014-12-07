//
//  TXImpulseInstrument.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 12/24/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXImpulseInstrument.h"
#include "TXFactory.h"

static TXStandardFactory<TXImpulseInstrument> 
g_sharedFactory("Impulse Generator",
                "com.nexhawks.TXSynthesizer.ImpulseInstrument",
                TXPluginTypeInstrument);

TXImpulseInstrument::TXImpulseInstrument(const TXConfig& config){
    m_impulsed=false;
}

TXImpulseInstrument::~TXImpulseInstrument(){
    
}

TXFactory *TXImpulseInstrument::sharedFactory(){
    return &g_sharedFactory;
}

void TXImpulseInstrument::noteOn(int key, int velocity){
    m_impulsed=true;
}

void TXImpulseInstrument::noteOff(int key, int velocity){
}

void TXImpulseInstrument::setPitchbend(int millicents){}

void TXImpulseInstrument::allNotesOff(){
    
}

void TXImpulseInstrument::allSoundsOff(){
    
}

void TXImpulseInstrument::renderFragmentAdditive
(int32_t *stereoOutputBuffer, unsigned int samples){
    if(!samples)
        return;
    if(m_impulsed){
        stereoOutputBuffer[0]+=16384;
        stereoOutputBuffer[1]+=16384;
        m_impulsed=false;
    }
}

