//
//  TXDecimatorFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/4/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//


#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include "TXEffect.h"
#include "TXSRC.h"

class TXDecimatorFilter: public TXEffect{
public:
    struct Parameter{
        float drive;
		float outGain;
		float mix;
		int bits;
        int downsampling;
		
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
		
		Parameter(){
			drive=1.f;
			outGain=1.f;
			mix=1.f;
			bits=8;
			downsampling=4;
		}
    };
    
private:
    
    TXConfig m_config;
    Parameter m_parameter;
    unsigned int m_drive;
    unsigned int m_wetGain;
	unsigned int m_dryGain;
	unsigned int m_bits;
	unsigned int m_downsampling;
	
	unsigned int m_carrySamples;
	int32_t m_carry1, m_carry2;
	
	template<int shift>
	void processDistortion(int32_t *buffer,
						   const int32_t *inBuffer,
						  unsigned int samples);
    
public:
    TXDecimatorFilter(const TXConfig& config);
    virtual ~TXDecimatorFilter();
    
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