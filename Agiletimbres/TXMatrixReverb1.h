//
//  TXMatrixReverb1.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"
#include "TXEffect.h"

class TXAllpassDelayFilter;
struct TXMatrixReverb1Network;
struct TXMatrixReverb1Reflections;

class TXMatrixReverb1: public TXEffect{
public:
    struct Parameter{
        float inGain;
        float reverbTime;
        float highFrequencyDamp;
        float earlyReflectionGain;
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
    };
private:
    enum{
        AllpassFiltersCount=8
    };
    TXConfig m_config;
    TXAllpassDelayFilter *m_allpassFilters[AllpassFiltersCount];
    TXMatrixReverb1Network *m_network;
    TXMatrixReverb1Reflections *m_reflections;
    Parameter m_parameter;
    unsigned int m_inGain;
    unsigned int m_erGain;
    
    void updateParameter();
    
public:
    TXMatrixReverb1(const TXConfig&);
    virtual ~TXMatrixReverb1();
    
    void setParameter(const Parameter&);
    const Parameter& parameter() const{return m_parameter;}
    
    virtual void applyStereo(int32_t *,
                             const int32_t *,
                             unsigned int);
    
    static TXFactory *sharedFactory();
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    virtual TPLObject *serialize() const;
    virtual void deserialize(const TPLObject *);
    
};
