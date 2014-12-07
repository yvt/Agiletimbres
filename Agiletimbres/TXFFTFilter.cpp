//
//  TXFFTFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "TXFFTFilter.h"
#include "NHIntegerFFT.h"

// TODO: variable processing unit size

struct TXFFTFilter::StreamHandler{
	
	int32_t wave1[512], wave2[512];
	int32_t real1[257], real2[257];
	int32_t imag1[257], imag2[257];
	
	unsigned int injectPos;
	unsigned int exhaustPos;
	
	void reset(){
		injectPos=128;
		memset(wave1, 0, 128*sizeof(int32_t));
		memset(wave2, 0, 128*sizeof(int32_t));
		memset(wave1+384, 0, 128*sizeof(int32_t));
		memset(wave2+384, 0, 128*sizeof(int32_t));
		
		
		exhaustPos=0;
	}
	
	static inline int32_t processInject(int32_t v){
		if(v>65536) v=65536;
		if(v<-65536) v=-65536;
		
		return v;
	}
	
	void inject(unsigned int samples,
				const int32_t *buffer){
		register unsigned int pos=injectPos;
		register int32_t * const r1=wave1;
		register int32_t * const r2=wave2;
		
		while(samples--){
			r1[pos]=processInject(*(buffer++));
			r2[pos]=processInject(*(buffer++));
			pos++;
		}
		
		injectPos=pos;
	}
	
	void forwardFFT(){
		NHIntegerRealFFTForward<16>(9, wave1, real1, imag1);
		NHIntegerRealFFTForward<16>(9, wave2, real2, imag2);
	}
	
	void backwardFFT(){
		NHIntegerRealFFTBackward<16>(9, wave1, real1, imag1);
		NHIntegerRealFFTBackward<16>(9, wave2, real2, imag2);
	}
	
	static inline int processExhaust(int32_t v){
		return v>>7;
	}
	
	template<bool Additive, int deltaVol>
	void exhaust(unsigned int samples,
				 int32_t *buffer,
				 int startVol) // [0,128]
	{
		register unsigned int pos=exhaustPos;
		register const int32_t * const r1=wave1;
		register const int32_t * const r2=wave2;
		
		while(samples--){
			
			{
				register int vl=processExhaust(r1[pos]);
				if(deltaVol!=0)
					vl=(vl*startVol)>>7;
				if(Additive)
					*(buffer++)+=vl;
				else
					*(buffer++)=vl;
			}
			
			{
				register int vl=processExhaust(r2[pos]);
				if(deltaVol!=0)
					vl=(vl*startVol)>>7;
				if(Additive)
					*(buffer++)+=vl;
				else
					*(buffer++)=vl;
			}
			
			startVol+=deltaVol;
			pos++;
		}
		
		exhaustPos=pos;
	}
	
};

TXFFTFilter::TXFFTFilter(const TXConfig&){
	for(int i=0;i<4;i++){
		m_handlers[i]=new StreamHandler();
	}
	mute();
}

TXFFTFilter::~TXFFTFilter(){
	for(int i=0;i<4;i++){
		delete m_handlers[i];
	}
}

void TXFFTFilter::processFreqDomain(TXFFTFilter::StreamHandler *hd){
	hd->forwardFFT();
	applyStereo(hd->real1, hd->imag1,
				hd->real2, hd->imag2, 257);
	hd->backwardFFT();
}

void TXFFTFilter::applyStereo
(int32_t *outBuffer, 
 const int32_t *inBuffer, 
 unsigned int samples){
	
	while(samples!=0){
		
		unsigned int curPhaseIndex=m_phase>>7; // [0..8)
		unsigned int phasePos=m_phase-(curPhaseIndex<<7);
		unsigned int leftSamples=128-phasePos;
		if(leftSamples>samples)
			leftSamples=samples;
		
		//  PHASE  | IN 0 1 2 3 | OUT 0 1 2 3  PHASE
		// --------+------------+---------------------
		//    0    |    X       |       X   X  0
		//    1    |    X       |       X   X  128
		//    2    |        X   |     X     X  256
		//    3    |        X   |     X     X  368
		//    4    |      X     |     X   X    512
		//    5    |      X     |     X   X    640
		//    6    |          X |       X X    768
		//    7    |          X |       X X    896
		//                                     1024
		
		if(shouldWindow()){
			switch(curPhaseIndex){
				case 0:
					if(phasePos==0){
						processFreqDomain(m_handlers[3]);
						m_handlers[2]->reset();
					}
					m_handlers[0]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[3]->exhaust<true, 1>
					(leftSamples, outBuffer, phasePos);
					break;
				case 1:
					m_handlers[0]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, -1>
					(leftSamples, outBuffer, 128-phasePos);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 2:
					if(phasePos==0){
						processFreqDomain(m_handlers[0]);
						m_handlers[1]->reset();
					}
					m_handlers[2]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 1>
					(leftSamples, outBuffer, phasePos);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 3:
					m_handlers[2]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[3]->exhaust<true, -1>
					(leftSamples, outBuffer, 128-phasePos);
					break;
				case 4:
					if(phasePos==0){
						processFreqDomain(m_handlers[2]);
						m_handlers[3]->reset();
					}
					m_handlers[1]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[2]->exhaust<true, 1>
					(leftSamples, outBuffer, phasePos);
					break;
				case 5:
					m_handlers[1]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, -1>
					(leftSamples, outBuffer, 128-phasePos);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 6:
					if(phasePos==0){
						processFreqDomain(m_handlers[1]);
						m_handlers[0]->reset();
					}
					m_handlers[3]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 1>
					(leftSamples, outBuffer, phasePos);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 7:
					m_handlers[3]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[2]->exhaust<true, -1>
					(leftSamples, outBuffer, 128-phasePos);
					break;
			}
		}else{
			switch(curPhaseIndex){
				case 0:
					if(phasePos==0){
						processFreqDomain(m_handlers[3]);
						m_handlers[2]->reset();
					}
					m_handlers[0]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, phasePos);
					break;
				case 1:
					m_handlers[0]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, 128-phasePos);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 2:
					if(phasePos==0){
						processFreqDomain(m_handlers[0]);
						m_handlers[1]->reset();
					}
					m_handlers[2]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, phasePos);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 3:
					m_handlers[2]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[3]->exhaust<true, 0>
					(leftSamples, outBuffer, 128-phasePos);
					break;
				case 4:
					if(phasePos==0){
						processFreqDomain(m_handlers[2]);
						m_handlers[3]->reset();
					}
					m_handlers[1]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, phasePos);
					break;
				case 5:
					m_handlers[1]->inject(leftSamples, inBuffer);
					m_handlers[0]->exhaust<false, 0>
					(leftSamples, outBuffer, 128-phasePos);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 6:
					if(phasePos==0){
						processFreqDomain(m_handlers[1]);
						m_handlers[0]->reset();
					}
					m_handlers[3]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, phasePos);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, 128);
					break;
				case 7:
					m_handlers[3]->inject(leftSamples, inBuffer);
					m_handlers[1]->exhaust<false, 0>
					(leftSamples, outBuffer, 128);
					m_handlers[2]->exhaust<true, 0>
					(leftSamples, outBuffer, 128-phasePos);
					break;
			}
		}
		
		samples-=leftSamples;
		outBuffer+=leftSamples*2;
		inBuffer+=leftSamples*2;
		m_phase+=leftSamples;
		m_phase&=1023;
	
	}
	
}

void TXFFTFilter::mute(){
	m_phase=0;
	for(int i=0;i<4;i++){
		m_handlers[i]->reset();
	}
}
