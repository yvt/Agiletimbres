//
//  TXOverdriveFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "TXOverdriveFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif

#define OversampleBits			1
#define OversampleScale			(1<<OversampleBits)

static TXStandardFactory<TXOverdriveFilter> 
g_sharedFactory("Overdrive",
                "com.nexhawks.TXSynthesizer.Overdrive",
                TXPluginTypeEffect);



TXFactory *TXOverdriveFilter::sharedFactory(){
    return &g_sharedFactory;
}

TXOverdriveFilter::TXOverdriveFilter(const TXConfig& config):
m_config(config){
	m_interpolator[0]=new TXSRC::CICBinaryInterpolator<OversampleBits, 2, 2>();
	m_interpolator[1]=new TXSRC::CICBinaryInterpolator<OversampleBits, 2, 2>();
	m_decimator[0]=new TXSRC::CICBinaryDecimator<OversampleBits, 2, 2>();
	m_decimator[1]=new TXSRC::CICBinaryDecimator<OversampleBits, 2, 2>();
	mute();
	setParameter(m_parameter);
}


TXOverdriveFilter::~TXOverdriveFilter(){
	delete m_interpolator[0];
	delete m_interpolator[1];
	delete m_decimator[0];
	delete m_decimator[1];
	
}

void TXOverdriveFilter::setParameter(const TXOverdriveFilter::Parameter &param){
	m_parameter=param;
	m_drive=(unsigned int)(param.drive*65536.f);
	m_wetGain=(unsigned int)(param.mix*param.outGain*65536.f);
	m_dryGain=(unsigned int)((1.f-param.mix)*65536.f);
	m_tone=(unsigned int)(param.tone*65536.f*65535.f);
}

void TXOverdriveFilter::applyStereo(int32_t *outBuffer,
									const int32_t *inBuffer, 
									unsigned int samples){
	int32_t *upBuf=(int32_t *)alloca(samples*8*OversampleScale);
	m_interpolator[0]->process(upBuf, inBuffer, samples);
	m_interpolator[1]->process(upBuf+1, inBuffer+1, samples);
	
	processOverdrive(upBuf, samples*OversampleScale);

	int32_t *downBuf=(int32_t *)alloca(samples*8);
	m_decimator[0]->process(downBuf, upBuf, samples);
	m_decimator[1]->process(downBuf+1, upBuf+1, samples);
	
	register int wetGain=m_wetGain;
	register int dryGain=m_dryGain;
	samples<<=1;
	while(samples--){
		int32_t wet=*(downBuf++);
		int32_t dry=*(outBuffer);
		wet=(int32_t)(((int64_t)wet*(int64_t)wetGain)>>16);
		dry=(int32_t)(((int64_t)dry*(int64_t)dryGain)>>16);
		*(outBuffer++)=wet+dry;
	}
}

void TXOverdriveFilter::processOverdrive(int32_t *buffer, unsigned int samples){
	register int drive=m_drive;
	register uint32_t tone=m_tone;
	register int cap1=m_capacitor[0];
	register int cap2=m_capacitor[1];
	while(samples--){
		
		register int tmp;
		
		tmp=*buffer-cap1;
		tmp=(int)(((int64_t)(tmp)*(uint64_t)tone)>>32);
		tmp=(int)(((int64_t)(tmp)*(uint64_t)tone)>>32);
		cap1+=tmp;
		tmp=(int)(((int64_t)cap1*(int64_t)drive)>>16);
		if(tmp>=32768){
			tmp=21845;
		}else if(tmp<-32768){
			tmp=-21845;
		}else{
			// y = x - (1/3)x^3
			register int tmp2=tmp;
			tmp2=(tmp2*tmp)>>15;
			tmp2=(tmp2*tmp)>>15;
			tmp2=(tmp2*11)>>5;
			
			tmp-=tmp2;
		}
		*(buffer++)=tmp;
		
		tmp=*buffer-cap2;
		tmp=(int)(((int64_t)(tmp)*(uint64_t)tone)>>32);
		tmp=(int)(((int64_t)(tmp)*(uint64_t)tone)>>32);
		cap2+=tmp;
		tmp=(int)(((int64_t)cap2*(int64_t)drive)>>16);
		if(tmp>=32768){
			tmp=21845;
		}else if(tmp<-32768){
			tmp=-21845;
		}else{
			// y = x - (1/3)x^3
			register int tmp2=tmp;
			tmp2=(tmp2*tmp)>>15;
			tmp2=(tmp2*tmp)>>15;
			tmp2=(tmp2*11)>>5;
			
			tmp-=tmp2;
		}
		*(buffer++)=tmp;
	}
	
	m_capacitor[0]=cap1;
	m_capacitor[1]=cap2;
}

void TXOverdriveFilter::mute(){
	m_capacitor[0]=0;
	m_capacitor[1]=0;
}

#pragma mark - Serialization

#define ParameterDriveKey "Drive"
#define ParameterOutGainKey "Gain"
#define ParameterMixKey "DryWetMix"
#define ParameterToneKey "Tone"

TPLObject *TXOverdriveFilter::Parameter::serialize() const{
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
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(tone));
        dic->setObject(&*obj, ParameterToneKey);
    }
    return dic;
}

void TXOverdriveFilter::Parameter::deserialize(const TPLObject *obj){
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
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterToneKey));
        if(num)
            tone=num->doubleValue();
        
    }
}


TPLObject *TXOverdriveFilter::serialize() const{
    return m_parameter.serialize();
    
}

void TXOverdriveFilter::deserialize(const TPLObject *obj){
    TXOverdriveFilter::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}

