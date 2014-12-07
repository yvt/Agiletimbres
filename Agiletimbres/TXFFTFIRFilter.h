//
//  TXFFTFIRFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/23/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "TXFFTFilter.h"

class TXFFTFIRFilter: public TXFFTFilter{
	int32_t m_real[257];
	int32_t m_imag[257];
protected:
	virtual void applyStereo(int32_t *realRight,
							 int32_t *imagRight,
							 int32_t *realLeft,
							 int32_t *imagLeft,
							 unsigned int size);
	
	virtual bool shouldWindow(){return false;}
	
	void processVector(int32_t *real,
					   int32_t *imag,
					   unsigned int size);
	
	void setFrequencyResponse(int32_t *real, int32_t *imag,
							  unsigned int size);
	void setFrequencyResponseAndWindow(int32_t *real, int32_t *imag,
							  unsigned int size);
	void setImpulseResponse(int32_t *wave, unsigned int size);
	
};
