//
//  TXChorusFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include "TXEffect.h"

class TXChorusFilter: public TXEffect{
public:
    struct Parameter{
        float inGain;
        float baseDelayTime;
        float deltaDelayTime;
        float frequency;
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
    };
    
private:
    
    enum {
        BufferSamples=8192
    };
    TXConfig m_config;
    int32_t *m_buffer;
    Parameter m_parameter;
    unsigned int m_bufferPosition;
    unsigned int m_lfoPosition;
    unsigned int m_period;
    unsigned int m_halfPeriod;
    unsigned int m_minDelayTime; // u16.16 samples
    unsigned int m_maxDelayTime; // u16.16 samples
    unsigned int m_lastDelayTime; // u16.16 samples
    unsigned int m_inGain;
    
    float maxDelayTime() const;
    
public:
    TXChorusFilter(const TXConfig& config);
    virtual ~TXChorusFilter();
    
    void setParameter(const Parameter&);
    const Parameter& parameter() const{return m_parameter;}
    
    virtual void applyStereo(int32_t *outBuffer,
                             const int32_t *inBuffer,
                             unsigned int samples);
    
    void mute();
    
    static TXFactory *sharedFactory();
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    virtual TPLObject *serialize() const;
    virtual void deserialize(const TPLObject *);
};
