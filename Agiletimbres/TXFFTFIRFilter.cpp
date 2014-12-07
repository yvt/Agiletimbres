//
//  TXFFTFIRFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/23/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "TXFFTFIRFilter.h"
#include "NHIntegerFFT.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif


#define FIXED_MUL(a, b) ((int32_t)(((int64_t)(a)*(int64_t)(b))>>16))

void TXFFTFIRFilter::processVector(int32_t *real, 
								   int32_t *imag, 
								   unsigned int size){
	
	assert(size==257);
	
	int32_t *rReal=m_real;
	int32_t *rImag=m_imag;
	while(size--){
		int32_t dReal=*real;
		int32_t dImag=*imag;
		
		*(real++)=FIXED_MUL(dReal, rReal)-FIXED_MUL(dImag, rImag);
		*(imag++)=FIXED_MUL(dReal, rImag)+FIXED_MUL(dImag, rReal);
	}
}

void TXFFTFIRFilter::applyStereo(int32_t *realRight,
								 int32_t *imagRight,
								 int32_t *realLeft,
								 int32_t *imagLeft,
								 unsigned int size){
	processVector(realRight, imagRight, size);
	processVector(realLeft, imagLeft, size);
}

void TXFFTFIRFilter::setFrequencyResponse(int32_t *real,
										  int32_t *imag,
										  unsigned int size){
	assert(size==257);
	memcpy(m_real, real, size*4);
	memcpy(m_imag, imag, size*4);
}

void TXFFTFIRFilter::setFrequencyResponseAndWindow(int32_t *real,
												   int32_t *imag,
												   unsigned int size){
	int32_t *wave=(int32_t *)alloca(4*512);
	NHIntegerRealFFTBackward<16>(9, 
								 wave, 
								 real, imag);
	
#define FIXED_SCALE7(v,s) ((((int64_t)(v))*((int64_t)(s)))>>7)
	
	for(int i=0;i<128;i++){
		register int per=128-i;
		wave[i]=FIXED_SCALE7(wave[i], per);
	}
	for(int i=128;i<385;i++)
		wave[i]=0;
	for(int i=384;i<512;i++){
		register int per=i-384;
		wave[i]=FIXED_SCALE7(wave[i], per);
	}

	NHIntegerRealFFTForward<16>(9, wave, m_real, m_imag);

	for(int i=0;i<=256;i++){
		real[i]>>=7;
		imag[i]>>=7;
	}
}

void TXFFTFIRFilter::setImpulseResponse(int32_t *wave,
										unsigned int size){
	int32_t *outWave=(int32_t *)alloca(4*512);
	memset(outWave, 0, 4*512);
	
	assert(size<256);
	
	for(unsigned int i=0;i<size;i++){
		outWave[(384+i)&511]=wave[i];
	}
	
	NHIntegerRealFFTForward<16>(9, wave, m_real, m_imag);
}

