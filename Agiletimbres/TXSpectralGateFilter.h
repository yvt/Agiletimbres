//
//  TXSpectralGateFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "TXFFTFilter.h"

class TXSpectralGateFilter: public TXFFTFilter{
public:
    struct Parameter{
		
		float centerFreq;
		float bandwidth;
		
		float threshold;
		float superEnergy;
		float subEnergy;
		
		float speed;
		
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
		
		Parameter(){
			centerFreq=11025.f;
			bandwidth=22050.f;
			
			threshold=0.1f;
			superEnergy=1.f;
			subEnergy=0.001f;
			speed=.4f;
		}
    };
    
private:
    
    TXConfig m_config;
    Parameter m_parameter;
	
	int64_t m_threshold;
	int32_t m_superEnergy;
	int32_t m_subEnergy;
	int m_minFrequency;
	int m_maxFrequency;
	int32_t m_history[257];
	int32_t m_speed;
	
	void createTransferFunction(int32_t *outReal,
								int32_t *outImag,
								int32_t *realRight,
								int32_t *imagRight,
								int32_t *realLeft,
								int32_t *imagLeft);
	
protected:
	
	virtual void applyStereo(int32_t *realRight,
							 int32_t *imagRight,
							 int32_t *realLeft,
							 int32_t *imagLeft,
							 unsigned int size);
	
	virtual bool shouldWindow(){return false;}
	
public:
    TXSpectralGateFilter(const TXConfig& config);
    virtual ~TXSpectralGateFilter();
    
    void setParameter(const Parameter&);
    const Parameter& parameter() const{return m_parameter;}
    
    static TXFactory *sharedFactory();
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    virtual TPLObject *serialize() const;
    virtual void deserialize(const TPLObject *);
};