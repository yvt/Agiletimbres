//
//  TXSpectralGateFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "TXSpectralGateFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"
#include "NHIntegerFFT.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif


static TXStandardFactory<TXSpectralGateFilter> 
g_sharedFactory("Spectral Gate",
                "com.nexhawks.TXSynthesizer.SpectralGate",
                TXPluginTypeEffect);



TXFactory *TXSpectralGateFilter::sharedFactory(){
    return &g_sharedFactory;
}

TXSpectralGateFilter::TXSpectralGateFilter(const TXConfig& config):
TXFFTFilter(config){
	setParameter(Parameter());
	for(int i=0;i<=256;i++)
		m_history[i]=65536;
}

TXSpectralGateFilter::~TXSpectralGateFilter(){
	
}

static uint64_t complexAbsPowered(int32_t r, int32_t i){
	int64_t v=(int64_t)r*(int64_t)r;
	v+=(int64_t)i*(int64_t)i;
	return (uint64_t)v;
}

void TXSpectralGateFilter::createTransferFunction(int32_t *outReal,
							int32_t *outImag,
							int32_t *realRight,
							int32_t *imagRight,
							int32_t *realLeft,
							int32_t *imagLeft){
	// create transfer function.
	outReal[0]=65536;
	outImag[0]=0;
	
	int minRange=m_minFrequency;
	int maxRange=m_maxFrequency;
	
	if(minRange<1) minRange=1;
	if(maxRange>256)maxRange=256;
	
	{
	
		register uint64_t threshold=(uint64_t)m_threshold;
		register int subEnergy=m_subEnergy;
		register int superEnergy=m_superEnergy;
		for(int i=minRange;i<=maxRange;i++){
			uint64_t power=complexAbsPowered(realRight[i], imagRight[i]);
			power+=complexAbsPowered(realLeft[i], imagLeft[i]);
			
			outReal[i]=(power<threshold)?subEnergy:superEnergy;
			outImag[i]=0;
		}
			
	}
	
	for(int i=1;i<minRange;i++){
		outReal[i]=outImag[i]=0;
	}for(int i=maxRange+1;i<=256;i++){
		outReal[i]=outImag[i]=0;
	}
	
	// slowen transition.
	{
		int32_t *history=m_history;
		register int32_t speed=m_speed;
		for(int i=0;i<=256;i++){
			int32_t his=history[i];
			int32_t dis=outReal[i]-his;
			dis=((int32_t)(((int64_t)(dis)*(int64_t)(speed))>>16));
			his+=dis;
			history[i]=his;
			outReal[i]=his>>7;
		}
	}
	
	outReal[0]=65536>>7;
	
	// apply window.
	
	int32_t *transWave=(int32_t *)alloca(512*4);
	
	NHIntegerRealFFTBackward<16>(9, transWave, outReal, outImag);
	for(int i=0;i<128;i++){
		register int per=128-i;
		transWave[i]=(transWave[i]*per)>>7;
	}
	for(int i=128;i<386;i++)
		transWave[i]=0;
	for(int i=384;i<512;i++){
		register int per=i-384;
		transWave[i]=(transWave[i]*per)>>7;
	}
	
	
	NHIntegerRealFFTForward<16>(9, transWave, outReal, outImag);
	//printf("%d, %d, %d, %d\n", outReal[0], outReal[4], outReal[8], outReal[12]);
}

#define FIXED_MUL(a, b) ((int32_t)(((int64_t)(a)*(int64_t)(b))>>16))

void TXSpectralGateFilter::applyStereo
(int32_t *realRight, int32_t *imagRight, 
 int32_t *realLeft, int32_t *imagLeft, 
 unsigned int size){
	assert(size==257); // TODO: more flexible!
	
	int32_t *transReal=(int32_t *)alloca(257*4);
	int32_t *transImag=(int32_t *)alloca(257*4);
	
	createTransferFunction(transReal, transImag, 
						   realRight, imagRight, 
						   realLeft, imagLeft);
	
	

	for(int i=0;i<=256;i++){
		int tx=transReal[i];
		int ty=transImag[i];
		{
			int x=realRight[i];
			int y=imagRight[i];
			realRight[i]=FIXED_MUL(x,tx)-FIXED_MUL(y,ty);
			imagRight[i]=FIXED_MUL(x,ty)+FIXED_MUL(y,tx);
		}
		
		{
			int x=realLeft[i];
			int y=imagLeft[i];
			realLeft[i]=FIXED_MUL(x,tx)-FIXED_MUL(y,ty);
			imagLeft[i]=FIXED_MUL(x,ty)+FIXED_MUL(y,tx);
		}
	}
	
	
	
}

static int toIntValue(float v, float maxValue){
	if(v<0.f) v=0.f;
	if(v>maxValue) v=maxValue;
	return (int)(v);
}

static int64_t toInt64Value(float v, float maxValue){
	if(v<0.f) v=0.f;
	if(v>maxValue) v=maxValue;
	return (int64_t)(v);
}

void TXSpectralGateFilter::setParameter(const TXSpectralGateFilter::Parameter &param){
	m_parameter=param;
	
	m_threshold=toInt64Value((m_parameter.threshold*m_parameter.threshold)*65536.f*65536.f*512.f, 65536.f*65536.f*65536.f*16384.f);
	m_superEnergy=toIntValue((m_parameter.superEnergy)*65536.f, 65536.f*64.f);
	m_subEnergy=toIntValue((m_parameter.subEnergy)*65536.f, 65536.f*64.f);
	m_minFrequency=toIntValue((m_parameter.centerFreq-m_parameter.bandwidth*.5f)/
							  m_config.sampleRate*2.f*256.f-1.f, 256.f);
	m_maxFrequency=toIntValue((m_parameter.centerFreq+m_parameter.bandwidth*.5f)/
							  m_config.sampleRate*2.f*256.f+1.f, 256.f);
	m_speed=toIntValue((m_parameter.speed)*65536.f, 65536.f);
	
}


#pragma mark - Serialization

#define ParameterCenterFreqKey "CenterFrequency"
#define ParameterBandwidthKey "Bandwidth"
#define ParameterThresholdKey "Threshold"
#define ParameterSuperEnergyKey "SuperEnergy"
#define ParameterSubEnergyKey "SubEnergy"
#define ParameterSpeedKey "Speed"


TPLObject *TXSpectralGateFilter::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
	
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(centerFreq));
        dic->setObject(&*obj, ParameterCenterFreqKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(bandwidth));
        dic->setObject(&*obj, ParameterBandwidthKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(threshold));
        dic->setObject(&*obj, ParameterThresholdKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(superEnergy));
        dic->setObject(&*obj, ParameterSuperEnergyKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(subEnergy));
        dic->setObject(&*obj, ParameterSubEnergyKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(speed));
        dic->setObject(&*obj, ParameterSpeedKey);
    }
    return dic;
}

void TXSpectralGateFilter::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterCenterFreqKey));
        if(num)
            centerFreq=num->doubleValue();
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterBandwidthKey));
        if(num)
            bandwidth=num->doubleValue();
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterThresholdKey));
        if(num)
            threshold=num->doubleValue();
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterSuperEnergyKey));
        if(num)
            superEnergy=num->doubleValue();
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterSubEnergyKey));
        if(num)
            subEnergy=num->doubleValue();
		num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterSpeedKey));
        if(num)
            speed=num->doubleValue();
        
        
    }
}


TPLObject *TXSpectralGateFilter::serialize() const{
    return m_parameter.serialize();
    
}

void TXSpectralGateFilter::deserialize(const TPLObject *obj){
    TXSpectralGateFilter::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}
