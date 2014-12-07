//
//  TXPhaserFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/2/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include "TXEffect.h"

class TXPhaserFilter: public TXEffect{
public:
    struct Parameter{
		int stages; // 4, 6, ..., 12
        float depth;
        float minFrequency;
		float maxFrequency;
        float lfoFrequency;
        float feedback;
		
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
    };
    
private:
	
	struct Channel{
		unsigned int lfoPosition;
		int lastIntensity;
		
		int y1[6], y2[6], x1[6], x2[6];
		
		void mute();
	};
    
    TXConfig m_config;
    Parameter m_parameter;
    unsigned int m_period;
    unsigned int m_halfPeriod;
	int m_minIntensity; // u0.32
	int m_maxIntensity; // u0.32
    unsigned int m_depth;
	int m_feedback; // s16.16
	Channel m_channel1, m_channel2;
	
	void stereoToMono(int32_t *outBuffer,
					  const int32_t *inBuffer,
					  unsigned int samples);
	
	template<int stages>
	void processOneChannel(int32_t *outBuffer,
						   const int32_t *monoBuffer,
						   unsigned int samples,
						   Channel& channel);
    
public:
    TXPhaserFilter(const TXConfig& config);
    virtual ~TXPhaserFilter();
    
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
