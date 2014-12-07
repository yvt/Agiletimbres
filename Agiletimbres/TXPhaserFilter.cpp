//
//  TXPhaserFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/2/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//


#include "TXPhaserFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif

static TXStandardFactory<TXPhaserFilter> 
g_sharedFactory("Phaser",
                "com.nexhawks.TXSynthesizer.Phaser",
                TXPluginTypeEffect);

TXFactory *TXPhaserFilter::sharedFactory(){
    return &g_sharedFactory;
}

TXPhaserFilter::TXPhaserFilter(const TXConfig& c):
m_config(c){
    
    m_period=0;
    
	m_parameter.stages=6;
    m_parameter.depth=.5f;
	m_parameter.minFrequency=830.f;
	m_parameter.maxFrequency=6200.f;
	m_parameter.lfoFrequency=.14f;
	m_parameter.feedback=.43f;
    setParameter(m_parameter);
    
    mute();
}

TXPhaserFilter::~TXPhaserFilter(){
}

static int intensityValue(float freq, float sampleFreq){
	freq/=sampleFreq*.5f;
	freq=1.f-freq;
	if(freq<-.99f)
		freq=-.99f;
	if(freq>.99f)
		freq=.99f;
	return (int)(freq*65536.f*32768.f);
}

static unsigned int integerValue(float v, float minVal=0.f, float maxVal=.99999f){
    if(v<minVal)
        v=minVal;
    if(v>maxVal)
        v=maxVal;
    return (unsigned int)(v*65536.f*65536.f);
}

void TXPhaserFilter::setParameter(const TXPhaserFilter::Parameter &param){
    m_parameter=param;
    if(m_parameter.feedback>.99f)
		m_parameter.feedback=.99f;
	if(m_parameter.feedback<-.99f)
		m_parameter.feedback=-.99f;
	
	if(m_parameter.maxFrequency<m_parameter.minFrequency){
		m_parameter.maxFrequency=m_parameter.minFrequency;
	}
    
    assert(m_parameter.lfoFrequency>0.f);
    float periodTime=1.f/m_parameter.lfoFrequency;
    
	m_depth=integerValue(m_parameter.depth);
	int newMaxIntensity=intensityValue(m_parameter.minFrequency, m_config.sampleRate);
	int newMinIntensity=intensityValue(m_parameter.maxFrequency, m_config.sampleRate);
	m_feedback=(int)(m_parameter.feedback*65536.f);
    
    if((newMaxIntensity!=m_maxIntensity || newMinIntensity!=m_minIntensity) && 
	   (m_minIntensity!=m_maxIntensity)){
		unsigned int per;
		
		per=m_channel1.lastIntensity-m_minIntensity;
		per=(unsigned int)(((uint64_t)per<<32)/(uint64_t)(m_maxIntensity-m_minIntensity));
		per=(unsigned int)(((uint64_t)per*(uint64_t)(newMaxIntensity-newMinIntensity))>>32);
		per+=newMinIntensity;
		m_channel1.lastIntensity=per;
		
		per=m_channel2.lastIntensity-m_minIntensity;
		per=(unsigned int)(((uint64_t)per<<32)/(uint64_t)(m_maxIntensity-m_minIntensity));
		per=(unsigned int)(((uint64_t)per*(uint64_t)(newMaxIntensity-newMinIntensity))>>32);
		per+=newMinIntensity;
		m_channel2.lastIntensity=per;
		
		
	}
	
	m_minIntensity=newMinIntensity;
	m_maxIntensity=newMaxIntensity;
	
    if((int)(periodTime*m_config.sampleRate)!=m_period || 
	   (m_minIntensity==m_maxIntensity)){
        // reset LFO oscilator.
        m_period=(int)(periodTime*m_config.sampleRate);
        m_halfPeriod=m_period>>1;
        m_channel1.lfoPosition=0;
		m_channel2.lfoPosition=m_period>>1;
		m_channel1.lastIntensity=m_minIntensity;
		m_channel2.lastIntensity=m_maxIntensity;
    }
    
}

void TXPhaserFilter::Channel::mute(){
	for(int i=0;i<6;i++)
	y1[i]=y2[i]=x1[i]=x2[i]=0;
	
}

void TXPhaserFilter::mute(){
	m_channel1.mute();
	m_channel2.mute();
}



void TXPhaserFilter::stereoToMono(int32_t *outBuffer, 
								  const int32_t *inBuffer, 
								  unsigned int samples){
	while(samples--){
		int32_t v=*(inBuffer++);
		v+=*(inBuffer++);
		*(outBuffer++)=v>>1;
	}
}

template<int stages> void TXPhaserFilter::processOneChannel(int32_t *outBuffer, 
									   const int32_t *monoBuffer, 
									   unsigned int samples, 
									   TXPhaserFilter::Channel &channel){
	unsigned int leftSamples;
	int nextIntensity;
    
	assert(channel.lfoPosition<m_period);
    
    if(channel.lfoPosition<m_halfPeriod){
        // raising.
        leftSamples=m_halfPeriod-channel.lfoPosition;
        nextIntensity=m_maxIntensity;
    }else{
        // lowering.
        leftSamples=m_period-channel.lfoPosition;
        nextIntensity=m_minIntensity;
    }
	
	// we have to do in two ways because intensity is u0.32
	int deltaDelayTime;
	if(nextIntensity>channel.lastIntensity){
		unsigned int udeltaDelayTime=nextIntensity-channel.lastIntensity;
		udeltaDelayTime/=(unsigned int)leftSamples;
		deltaDelayTime=(int)udeltaDelayTime;
	}else{
		unsigned int udeltaDelayTime=channel.lastIntensity-nextIntensity;
		udeltaDelayTime/=(unsigned int)leftSamples;
		deltaDelayTime=-(int)udeltaDelayTime;
	}
    
    if(leftSamples>samples){
        leftSamples=samples;
    }
    
	int currentIntensity=channel.lastIntensity;
    
	// advance LFO posiiton.
    channel.lfoPosition+=leftSamples;
    assert(channel.lfoPosition<=m_period);
    if(channel.lfoPosition==m_period){
        channel.lfoPosition=0;
    }
	
	register const unsigned int depth=m_depth;
	register const int feedback=m_feedback;
#define LDSTAGE(n) \
	register int y##n##1=channel.y1[n-1]; \
	register int y##n##2=channel.y2[n-1]; \
	register int x##n##1=channel.x1[n-1]; \
	register int x##n##2=channel.x2[n-1];
	LDSTAGE(1);
	LDSTAGE(2);
	LDSTAGE(3);
	LDSTAGE(4);
	LDSTAGE(5);
	LDSTAGE(6);
#undef LDSTAGE
	
	for(unsigned int i=0;i<leftSamples;i++){
		
		register int x0=*(monoBuffer++)<<8, y0;
		
		// feedback
		if(stages>=12){
			if(y61>0x1000000) y61=0x1000000;
			if(y61<-0x1000000) y61=-0x1000000;
			x0+=(int)(((int64_t)y61*(int64_t)feedback)>>16);
		}else if(stages>=10){
			if(y51>0x1000000) y51=0x1000000;
			if(y51<-0x1000000) y51=-0x1000000;
			x0+=(int)(((int64_t)y51*(int64_t)feedback)>>16);
		}else if(stages>=8){
			if(y41>0x1000000) y41=0x1000000;
			if(y41<-0x1000000) y41=-0x1000000;
			x0+=(int)(((int64_t)y41*(int64_t)feedback)>>16);
		}else if(stages>=6){
			if(y31>0x1000000) y31=0x1000000;
			if(y31<-0x1000000) y31=-0x1000000;
			x0+=(int)(((int64_t)y31*(int64_t)feedback)>>16);
		}else if(stages>=4){
			if(y21>0x1000000) y21=0x1000000;
			if(y21<-0x1000000) y21=-0x1000000;
			x0+=(int)(((int64_t)y21*(int64_t)feedback)>>16);
		}
		
		
		// do biquad all-pass filter.
		// function: y[0]-2*a*y[1]+a*a*y[2]=a*a*x[0]-2*a*x[1]+x[2]
		//           y[0]=(a*a)*(x[0]-y[2])+2*a*(y[1]-x[1])+x[2]
		//           y[0]=a*(a*(x[0]-y[2])+2*(y[1]-x[1]))+x[2]
		// (yvt.jp Digital Filter Analysis format)
		
#define STAGE(n) {\
		y0=x0-y##n##2; \
		y0=(int)(((int64_t)y0*(int64_t)currentIntensity)>>31); \
		y0+=(y##n##1-x##n##1)<<1; \
		y0=(int)(((int64_t)y0*(int64_t)currentIntensity)>>31); \
		y0+=x##n##2; \
		x##n##2=x##n##1; y##n##2=y##n##1; \
		x##n##1=x0; y##n##1=y0; }
	
		// stage1:
		y0=x0-y12;
		STAGE(1);
		if(stages>=4){
			x0=y0;
			STAGE(2);
		}
		if(stages>=6){
			x0=y0;
			STAGE(3);
		}
		if(stages>=8){
			x0=y0;
			STAGE(4);
		}
		if(stages>=10){
			x0=y0;
			STAGE(5);
		}
		if(stages>=12){
			x0=y0;
			STAGE(6);
		}
		
#undef STGAE
		
		// apply.
		y0=(int)(((int64_t)y0*(uint64_t)depth)>>32);
		
		*(outBuffer)+=y0>>8;
		outBuffer+=2; // skip the other channel
		
		currentIntensity+=deltaDelayTime;
	}
	
#define STRSTAGE(n) \
	{ channel.y1[n-1]=y##n##1; channel.y2[n-1]=y##n##2; \
	channel.x1[n-1]=x##n##1; channel.x2[n-1]=x##n##2; }
	
	if(stages>=2)
		STRSTAGE(1);
	if(stages>=4)
		STRSTAGE(2);
	if(stages>=6)
		STRSTAGE(3);
	if(stages>=8)
		STRSTAGE(4);
	if(stages>=10)
		STRSTAGE(5);
	if(stages>=12)
		STRSTAGE(6);
	
#undef STRSTAGE
	
	channel.lastIntensity=currentIntensity;
	
	if(samples>leftSamples){
		processOneChannel<stages>(outBuffer, monoBuffer, samples-leftSamples,
						  channel);
	}

}

void TXPhaserFilter::applyStereo
(int32_t *outBuffer, const int32_t *inBuffer, 
 unsigned int samples){
	
	int32_t *monoBuffer=(int32_t *)alloca(samples*4);
	stereoToMono(monoBuffer, inBuffer, samples);
	
	switch(m_parameter.stages){
		case 4:
			processOneChannel<4>(outBuffer, monoBuffer, samples, 
								  m_channel1);
			processOneChannel<4>(outBuffer+1, monoBuffer, samples, 
								  m_channel2);
			break;
		case 6:
			processOneChannel<6>(outBuffer, monoBuffer, samples, 
								  m_channel1);
			processOneChannel<6>(outBuffer+1, monoBuffer, samples, 
								  m_channel2);
			break;
		case 8:
			processOneChannel<8>(outBuffer, monoBuffer, samples, 
								  m_channel1);
			processOneChannel<8>(outBuffer+1, monoBuffer, samples, 
								  m_channel2);
			break;
		case 10:
			processOneChannel<10>(outBuffer, monoBuffer, samples, 
								  m_channel1);
			processOneChannel<10>(outBuffer+1, monoBuffer, samples, 
								  m_channel2);
			break;
		case 12:
			processOneChannel<12>(outBuffer, monoBuffer, samples, 
								  m_channel1);
			processOneChannel<12>(outBuffer+1, monoBuffer, samples, 
								  m_channel2);
			break;
	}
	
	
}

#pragma mark - Serialization

#define ParameterStagesKey "Stages"
#define ParameterDepthKey "Depth"
#define ParameterMinFrequencyKey "MinFrequency"
#define ParameterMaxFrequencyKey "MaxFrequency"
#define ParameterFeedbackKey "Feedback"
#define ParameterLfoFrequencyKey "LfoFrequency"

TPLObject *TXPhaserFilter::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(stages));
        dic->setObject(&*obj, ParameterStagesKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(depth));
        dic->setObject(&*obj, ParameterDepthKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(minFrequency));
        dic->setObject(&*obj, ParameterMinFrequencyKey);
    }
	{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(maxFrequency));
        dic->setObject(&*obj, ParameterMaxFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(feedback));
        dic->setObject(&*obj, ParameterFeedbackKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(lfoFrequency));
        dic->setObject(&*obj, ParameterLfoFrequencyKey);
    }
    return dic;
}

void TXPhaserFilter::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        
		num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterStagesKey));
        if(num)
            stages=num->intValue();
		
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterDepthKey));
        if(num)
            depth=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterLfoFrequencyKey));
        if(num)
			lfoFrequency=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterFeedbackKey));
        if(num)
            feedback=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterMinFrequencyKey));
        if(num)
            minFrequency=num->doubleValue();
		
		num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterMaxFrequencyKey));
        if(num)
            maxFrequency=num->doubleValue();
        
    }
}


TPLObject *TXPhaserFilter::serialize() const{
    return m_parameter.serialize();
    
}

void TXPhaserFilter::deserialize(const TPLObject *obj){
    TXPhaserFilter::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}


