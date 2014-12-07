//
//  TXTestInstrument.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXTestInstrument.h"
#include "TXFactory.h"
#include <stdlib.h>


static TXStandardFactory<TXTestInstrument> 
g_sharedFactory("Test Instrument",
                "com.nexhawks.TXSynthesizer.TestInstrument",
                TXPluginTypeInstrument);

TXFactory *TXTestInstrument::sharedFactory(){
    return &g_sharedFactory;
}


TXTestInstrument::TXTestInstrument(const TXConfig& config){
    m_sampleFreq=config.sampleRate;
    m_baseSpeedScale=65536.f*65536.f/m_sampleFreq;
    m_pitchbendScale=0x10000;
}

TXTestInstrument::~TXTestInstrument(){
    
}

static float nextRandom(){
    return (float)rand()/(float)RAND_MAX;
}

void TXTestInstrument::noteOn(int key, int velocity){
    Channel channel;
    float baseFreq=frequencyForNote(key);
    for(int i=0;i<8;i++){
        float scale;
        scale=1.f+(nextRandom()-.5f)*.02f;
        
        channel.phase1=(uint32_t)(rand()&255)<<24;
        channel.phase2=(uint32_t)(rand()&255)<<24;
        channel.baseSpeed1=(uint32_t)(baseFreq*m_baseSpeedScale*scale);
        
        scale=1.f+(nextRandom()-.5f)*.02f;
        channel.baseSpeed2=(uint32_t)(baseFreq*m_baseSpeedScale*scale);
        
        channel.velocity=velocity;
        channel.key=key;
        m_channels.push_back(channel);
    }
}

void TXTestInstrument::noteOff(int key, int velocity){
    int i=8;
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
    nx:
        Channel& ch=*it;
        if(ch.key==key){
            // found!
            ChannelList::iterator it2=it; it2++;
            m_channels.erase(it);
            if(!(i--))
                return;
            it=it2;
            if(it==m_channels.end())
                break;
            goto nx;
            //return;
        }
    }
    //printf("TXTestInstrument: warning: unmatched noteoff (%d, %d)\n", key, velocity);
}

void TXTestInstrument::allNotesOff(){
    m_channels.clear();
}

void TXTestInstrument::allSoundsOff(){
    m_channels.clear();
}

void TXTestInstrument::setPitchbend(int millicents){
    m_pitchbendScale=(uint32_t)(65536.f*scaleForCents(millicents/1000));
}

void TXTestInstrument::renderFragmentAdditive(int32_t *out,
                                              unsigned int samples){
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        Channel& ch=*it;
        renderChannelAdditive(out, samples, ch);
        
       
        
    }
}


static inline int32_t readWaveform(uint32_t phase){
    //return ((phase)<0x80000000UL)?32767:-32768;
    return (int32_t)(phase>>16)-32768;
}

void TXTestInstrument::renderChannelAdditive(int32_t *outBuffer,
                                             unsigned int samples,
                                             TXTestInstrument::Channel &ch){
 
    
    uint32_t phase1=ch.phase1;
    uint32_t phase2=ch.phase2;
    uint32_t speed1=ch.baseSpeed1;
    uint32_t speed2=ch.baseSpeed2;
    int vol=ch.velocity;
    
    // apply pitchbend.
    speed1=((uint64_t)speed1*m_pitchbendScale)>>16;
    speed2=((uint64_t)speed2*m_pitchbendScale)>>16;
    
    while(samples--){
        
        *(outBuffer++)+=(readWaveform(phase1)*vol)>>9>>2;
        *(outBuffer++)+=(readWaveform(phase2)*vol)>>9>>2;
        
        phase1+=speed1;
        phase2+=speed2;
        
    }
    
    ch.phase1=phase1;
    ch.phase2=phase2;
    
}


