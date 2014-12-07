//
//  TXTestInstrument.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXInstrument.h"

#include <list>

class TXTestInstrument: public TXInstrument{
    
    struct Channel{
        uint32_t phase1, phase2;
        uint32_t baseSpeed1, baseSpeed2;
        int velocity: 8;
        int key: 8;
    };
    
    typedef std::list<Channel> ChannelList;
    
    ChannelList m_channels;
    float m_sampleFreq;
    float m_baseSpeedScale;
    uint32_t m_pitchbendScale;
    
public:
    
    TXTestInstrument(const TXConfig&config);
    virtual ~TXTestInstrument();
    
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    static TXFactory *sharedFactory();
    
protected:
    
    virtual void noteOn(int key, int velocity);
    
    virtual void noteOff(int key, int velocity);
    
    virtual void setPitchbend(int millicents);
    
    virtual void allNotesOff();
    virtual void allSoundsOff();
    
    virtual void renderFragmentAdditive(int32_t *stereoOutputBuffer,
                                        unsigned int samples);
    
    
    void renderChannelAdditive(int32_t *outBuffer,
                               unsigned int samples,
                               Channel& ch);
    
};
