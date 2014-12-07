//
//  TXChorusFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXChorusFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"

static TXStandardFactory<TXChorusFilter> 
g_sharedFactory("Flanger",
                "com.nexhawks.TXSynthesizer.Chorus",
                TXPluginTypeEffect);

TXFactory *TXChorusFilter::sharedFactory(){
    return &g_sharedFactory;
}

TXChorusFilter::TXChorusFilter(const TXConfig& c):
m_config(c){
    
    assert((BufferSamples&(BufferSamples-1))==0);
    
    m_buffer=new int32_t[BufferSamples];
    m_period=0;
    
    m_parameter.baseDelayTime=.01f;
    m_parameter.deltaDelayTime=.005f;
    m_parameter.frequency=.25f;
    m_parameter.inGain=1.f;
    setParameter(m_parameter);
    
    mute();
}

TXChorusFilter::~TXChorusFilter(){
    delete[] m_buffer;
}

float TXChorusFilter::maxDelayTime() const{
    return (float)(BufferSamples-1)/m_config.sampleRate;
}

static unsigned int integerValue(float v){
    if(v<0.f)
        v=0.f;
    if(v>.99999f)
        v=.99999f;
    return (unsigned int)(v*65536.f*65536.f);
}

void TXChorusFilter::setParameter(const TXChorusFilter::Parameter &param){
    m_parameter=param;
    
    if(m_parameter.deltaDelayTime<0.f)
        m_parameter.deltaDelayTime=0;
    
    // modify delay time so that
    // it doesn't go beyond the buffer size.
    if(m_parameter.baseDelayTime+
       m_parameter.deltaDelayTime>maxDelayTime()){
        float ratio=maxDelayTime();
        ratio/=m_parameter.baseDelayTime+
        m_parameter.deltaDelayTime;
        m_parameter.baseDelayTime*=ratio;
        m_parameter.deltaDelayTime*=ratio;
    }
    
    assert(m_parameter.frequency>0.f);
    float periodTime=1.f/m_parameter.frequency;
    
    m_minDelayTime=(int)((m_parameter.baseDelayTime)*
                         m_config.sampleRate*65536.f);
    m_maxDelayTime=(int)((m_parameter.baseDelayTime+
                          m_parameter.deltaDelayTime)*
                         m_config.sampleRate*65536.f);
    
    if((int)(periodTime*m_config.sampleRate)!=m_period){
        // reset LFO oscilator.
        m_period=(int)(periodTime*m_config.sampleRate);
        m_halfPeriod=m_period>>1;
        m_lfoPosition=0;
        m_lastDelayTime=m_minDelayTime;
    }
    
    m_inGain=integerValue(m_parameter.inGain);
}

void TXChorusFilter::mute(){
    m_bufferPosition=0;
    memset(m_buffer, 0, BufferSamples*sizeof(int32_t));
}

void TXChorusFilter::applyStereo
(int32_t *outBuffer, const int32_t *inBuffer, 
 unsigned int samples){
    unsigned int leftSamples;
    unsigned int nextDelayTime;
    
     assert(m_lfoPosition<m_period);
    
    if(m_lfoPosition<m_halfPeriod){
        // raising.
        leftSamples=m_halfPeriod-m_lfoPosition;
        nextDelayTime=m_maxDelayTime;
    }else{
        // lowering.
        leftSamples=m_period-m_lfoPosition;
        nextDelayTime=m_minDelayTime;
    }
    
    int deltaDelayTime=nextDelayTime;
    deltaDelayTime-=m_lastDelayTime;
    deltaDelayTime/=(int)leftSamples;
    if(leftSamples>samples){
        leftSamples=samples;
    }
    
    unsigned int currentDelayTime=m_lastDelayTime;
    
    m_lfoPosition+=leftSamples;
    assert(m_lfoPosition<=m_period);
    if(m_lfoPosition==m_period){
        m_lfoPosition=0;
    }
    
    // start effector.
    register int32_t * const buffer=m_buffer;
    register unsigned int bufferPosition=m_bufferPosition;
    register const unsigned int inGain=m_inGain;
    const unsigned int subtracted=(m_minDelayTime+m_maxDelayTime)>>16;
    
    assert(bufferPosition<BufferSamples);

    
    // don't do while(samples--) here!
    // samples and leftSamples are used later.
    for(unsigned int i=0;i<leftSamples;i++){
        
        assert(bufferPosition<BufferSamples);
        
        
        int input=*(inBuffer++);
        input+=*(inBuffer++);
        input>>=1;
        input=((int64_t)input*(uint64_t)inGain)>>32;
        buffer[bufferPosition]=input;
        
        int fracPos=currentDelayTime>>8;
        fracPos&=0xff;
        
        // tap 1
        unsigned int tapPos1=bufferPosition;
        tapPos1-=currentDelayTime>>16;
        tapPos1&=(BufferSamples-1);
        
        int tap1=buffer[tapPos1];
        int tap1b=buffer[tapPos1?(tapPos1-1):(BufferSamples-1)];
        tap1+=((tap1b-tap1)*fracPos)>>8;
        *(outBuffer++)+=tap1;
        
        // tap 2
        unsigned int tapPos2=bufferPosition;
        tapPos2+=(currentDelayTime>>16)-subtracted;
        tapPos2&=(BufferSamples-1);
        
        int tap2=buffer[tapPos2];
        int tap2b=buffer[(tapPos2+1)&(BufferSamples-1)];
        tap2+=((tap2b-tap2)*fracPos)>>8;
        *(outBuffer++)+=tap2;
        
        
        
        
        currentDelayTime+=deltaDelayTime;
        
        bufferPosition++;
        if(bufferPosition==BufferSamples)
            bufferPosition=0;
        
        assert(bufferPosition<BufferSamples);
        
    }
    
    
    // save effector's state.
    m_bufferPosition=bufferPosition;
    m_lastDelayTime=currentDelayTime;
    
    if(samples>leftSamples){
        applyStereo
        (outBuffer, 
         inBuffer, 
         samples-leftSamples);
    }
}

#pragma mark - Serialization

#define ParameterInGainKey "InGain"
#define ParameterBaseDelayTimeKey "BaseDelayTime"
#define ParameterDeltaDelayTimeKey "DeltaDelayTime"
#define ParameterFrequencyKey "Frequency"

TPLObject *TXChorusFilter::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(inGain));
        dic->setObject(&*obj, ParameterInGainKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(baseDelayTime));
        dic->setObject(&*obj, ParameterBaseDelayTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(deltaDelayTime));
        dic->setObject(&*obj, ParameterDeltaDelayTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(frequency));
        dic->setObject(&*obj, ParameterFrequencyKey);
    }
    return dic;
}

void TXChorusFilter::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterInGainKey));
        if(num)
            inGain=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterBaseDelayTimeKey));
        if(num)
            baseDelayTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterDeltaDelayTimeKey));
        if(num)
            deltaDelayTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterFrequencyKey));
        if(num)
            frequency=num->doubleValue();
        
    }
}


TPLObject *TXChorusFilter::serialize() const{
    return m_parameter.serialize();
    
}

void TXChorusFilter::deserialize(const TPLObject *obj){
    TXChorusFilter::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}



