//
//  TXFFTFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include "TXEffect.h"

class TXFFTFilter: public TXEffect{
	
	class StreamHandler;
	
	unsigned int m_phase; // 0...256...1024
	
	StreamHandler *m_handlers[4];
	
	void processFreqDomain(StreamHandler *);
	
protected:
	
	virtual void applyStereo(int32_t *realRight,
							 int32_t *imagRight,
							 int32_t *realLeft,
							 int32_t *imagLeft,
							 unsigned int size){}
	
	virtual bool shouldWindow(){return true;}
	
public:
    TXFFTFilter(const TXConfig& config);
    virtual ~TXFFTFilter();
    
    virtual void applyStereo(int32_t *outBuffer,
                             const int32_t *inBuffer,
                             unsigned int samples);
    
    void mute();
};