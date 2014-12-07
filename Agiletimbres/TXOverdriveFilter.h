//
//  TXOverdriveFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//


#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include "TXEffect.h"
#include "TXSRC.h"

class TXOverdriveFilter: public TXEffect{
public:
    struct Parameter{
        float drive;
		float outGain;
		float mix;
		float tone;
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
		
		Parameter(){
			drive=4.f;
			outGain=.8f;
			mix=.7f;
			tone=.5f;
		}
    };
    
private:
    
    TXConfig m_config;
    Parameter m_parameter;
    unsigned int m_drive;
    unsigned int m_wetGain;
	unsigned int m_dryGain;
	unsigned int m_tone;
	int m_capacitor[2];
	
	TXSRC::IntInterpolator *m_interpolator[2];
	TXSRC::IntDecimator *m_decimator[2];
	
	void processOverdrive(int32_t *buffer,
						  unsigned int samples);
    
public:
    TXOverdriveFilter(const TXConfig& config);
    virtual ~TXOverdriveFilter();
    
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