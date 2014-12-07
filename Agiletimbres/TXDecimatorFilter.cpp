//
//  TXDecimatorFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/4/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//


#include "TXDecimatorFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif


static TXStandardFactory<TXDecimatorFilter> 
g_sharedFactory("Decimator",
                "com.nexhawks.TXSynthesizer.Decimator",
                TXPluginTypeEffect);



TXFactory *TXDecimatorFilter::sharedFactory(){
    return &g_sharedFactory;
}

TXDecimatorFilter::TXDecimatorFilter(const TXConfig& config):
m_config(config){
	mute();
	setParameter(m_parameter);
}


TXDecimatorFilter::~TXDecimatorFilter(){
}

void TXDecimatorFilter::setParameter(const TXDecimatorFilter::Parameter &param){
	m_parameter=param;
	m_drive=(unsigned int)(param.drive*65536.f);
	m_wetGain=(unsigned int)(param.mix*param.outGain*65536.f);
	m_dryGain=(unsigned int)((1.f-param.mix)*65536.f);
	m_bits=param.bits;
	if(m_bits<1)
		m_bits=1;
	if(m_bits>16)
		m_bits=16;
	m_downsampling=param.downsampling;
	if(m_downsampling<1)
		m_downsampling=1;
	
}

template<int shift>
void TXDecimatorFilter::processDistortion(int32_t *outBuffer, 
										  const int32_t *inBuffer,
										  unsigned int samples){
	// TODO: antialiased
	
	if(m_carrySamples){
		register unsigned int carrySamples=m_carrySamples;
		register int32_t carry1=m_carry1, carry2=m_carry2;
		register const int dryGain=m_dryGain;
		if(carrySamples>samples)
			carrySamples=samples;
		// apply gain to wet
		carry1=(int32_t)(((int64_t)carry1*(int64_t)m_wetGain)>>16);
		carry2=(int32_t)(((int64_t)carry2*(int64_t)m_wetGain)>>16);
		while(carrySamples--){
			register int dry;
			
			dry=*(outBuffer);
			dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
			*(outBuffer++)=dry+carry1;
			
			dry=*(outBuffer);
			dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
			*(outBuffer++)=dry+carry2;
			
			inBuffer+=2;
		}
		samples-=m_carrySamples;
		m_carrySamples=0;
	}
	
	register const unsigned int downsampling=m_downsampling;
	register const int wetGain=m_wetGain;
	register const int dryGain=m_dryGain;
	register unsigned int drive=m_drive;
	
	while(samples>0){
		register unsigned int leftSamples;
		register int32_t fill1, fill2;
		
		fill1=*(inBuffer+0);
		fill2=*(inBuffer+1);
		
		// apply drive
		fill1=(int32_t)(((int64_t)fill1*(int64_t)drive)>>16);
		fill2=(int32_t)(((int64_t)fill2*(int64_t)drive)>>16);
		if(fill1>32768) fill1=32768;
		if(fill2>32768) fill2=32768;
		if(fill1<-32768) fill1=-32768;
		if(fill2<-32768) fill2=-32768;
		
		// decimate
		fill1=(fill1>>shift)<<shift;
		fill2=(fill2>>shift)<<shift;
		
		leftSamples=samples;
		if(leftSamples>downsampling){
			leftSamples=downsampling;
		}else{
			// carry occured.
			m_carrySamples=downsampling-leftSamples;
			m_carry1=fill1;
			m_carry2=fill2;
		}
		
		inBuffer+=leftSamples<<1;
		
		// apply gain to wet
		fill1=(int32_t)(((int64_t)fill1*(int64_t)wetGain)>>16);
		fill2=(int32_t)(((int64_t)fill2*(int64_t)wetGain)>>16);
		
		while(leftSamples--){
			
			register int dry;
			
			dry=*(outBuffer);
			dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
			*(outBuffer++)=dry+fill1;
			
			dry=*(outBuffer);
			dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
			*(outBuffer++)=dry+fill2;
			
			samples--;
		}
		
		
	}
	
}


void TXDecimatorFilter::applyStereo(int32_t *outBuffer,
									const int32_t *inBuffer, 
									unsigned int samples){
	switch(m_bits){
		case 1: processDistortion<15>(outBuffer, inBuffer, samples); break;
		case 2: processDistortion<14>(outBuffer, inBuffer, samples); break;
		case 3: processDistortion<13>(outBuffer, inBuffer, samples); break;
		case 4: processDistortion<12>(outBuffer, inBuffer, samples); break;
		case 5: processDistortion<11>(outBuffer, inBuffer, samples); break;
		case 6: processDistortion<10>(outBuffer, inBuffer, samples); break;
		case 7: processDistortion<9>(outBuffer, inBuffer, samples); break;
		case 8: processDistortion<8>(outBuffer, inBuffer, samples); break;
		case 9: processDistortion<7>(outBuffer, inBuffer, samples); break;
		case 10: processDistortion<6>(outBuffer, inBuffer, samples); break;
		case 11: processDistortion<5>(outBuffer, inBuffer, samples); break;
		case 12: processDistortion<4>(outBuffer, inBuffer, samples); break;
		case 13: processDistortion<3>(outBuffer, inBuffer, samples); break;
		case 14: processDistortion<2>(outBuffer, inBuffer, samples); break;
		case 15: processDistortion<1>(outBuffer, inBuffer, samples); break;
		case 16: processDistortion<0>(outBuffer, inBuffer, samples); break;
		default: assert(false); break;
	}
	/*
	int32_t *downBuf=(int32_t *)alloca(samples*8);
	
	register int wetGain=m_wetGain;
	register int dryGain=m_dryGain;
	samples<<=1;
	while(samples--){
		int32_t wet=*(downBuf++);
		int32_t dry=*(outBuffer);
		wet=(int32_t)(((int64_t)wet*(int64_t)wetGain)>>16);
		dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
		*(outBuffer++)=wet+dry;
	}*/
}

void TXDecimatorFilter::mute(){
	m_carrySamples=0;
}

#pragma mark - Serialization

#define ParameterDriveKey "Drive"
#define ParameterOutGainKey "Gain"
#define ParameterMixKey "DryWetMix"
#define ParameterBitsKey "Bits"
#define ParameterDownsamplingKey "DownsamplingFactor"

TPLObject *TXDecimatorFilter::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(drive));
        dic->setObject(&*obj, ParameterDriveKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(outGain));
        dic->setObject(&*obj, ParameterOutGainKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(mix));
        dic->setObject(&*obj, ParameterMixKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(bits));
        dic->setObject(&*obj, ParameterBitsKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(downsampling));
        dic->setObject(&*obj, ParameterDownsamplingKey);
    }
    return dic;
}

void TXDecimatorFilter::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterDriveKey));
        if(num)
            drive=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterOutGainKey));
        if(num)
            outGain=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterMixKey));
        if(num)
            mix=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterBitsKey));
        if(num)
            bits=num->intValue();
		
		num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterDownsamplingKey));
        if(num)
            downsampling=num->intValue();
        
    }
}


TPLObject *TXDecimatorFilter::serialize() const{
    return m_parameter.serialize();
    
}

void TXDecimatorFilter::deserialize(const TPLObject *obj){
    TXDecimatorFilter::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}

