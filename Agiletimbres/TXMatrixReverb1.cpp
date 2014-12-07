//
//  TXMatrixReverb1.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXMatrixReverb1.h"
#include "TXAllpassDelayFilter.h"
#include "TXFactory.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"

#ifdef WIN32
// for alloca
#include <malloc.h>
#endif

struct TXMatrixReverb1Network{
    
    enum{
        NetworkSize=4
    };

    unsigned int delayLengths[NetworkSize];
    int *buffers[NetworkSize];
    unsigned int positions[NetworkSize];
    unsigned int highFrequencyDamp;
    unsigned int gain;
    int filterStates[4];
    
    TXMatrixReverb1Network(unsigned int *lengths){
        for(int i=0;i<NetworkSize;i++){
            delayLengths[i]=lengths[i];
            assert(lengths[i]>0);
            buffers[i]=new int[lengths[i]];
            positions[i]=0;
            memset(buffers[i], 0, lengths[i]*sizeof(int));
        }
        filterStates[0]=0;
        filterStates[1]=0;
        filterStates[2]=0;
        filterStates[3]=0;
    }
    
    ~TXMatrixReverb1Network(){
        for(int i=0;i<NetworkSize;i++)
            delete buffers[i];
    }
    
    void apply(int32_t *outBufferStereo,
               const int32_t *inBufferMono,
               unsigned int samples){
        assert(NetworkSize==4);
        
        // don't make there array because
        // register won't be used and
        // it would be too slow
        int *buffer1=buffers[0];
        int *buffer2=buffers[1];
        int *buffer3=buffers[2];
        int *buffer4=buffers[3];
        unsigned int position1=positions[0];
        unsigned int position2=positions[1];
        unsigned int position3=positions[2];
        unsigned int position4=positions[3];
        unsigned int length1=delayLengths[0];
        unsigned int length2=delayLengths[1];
        unsigned int length3=delayLengths[2];
        unsigned int length4=delayLengths[3];
        unsigned int lpfDamp=this->highFrequencyDamp;
        unsigned int gain=this->gain;
        int filter1=this->filterStates[0];
        int filter2=this->filterStates[1];
        int filter3=this->filterStates[2];
        int filter4=this->filterStates[3];
        
        assert(position1<length1);
        assert(position2<length2);
        assert(position3<length3);
        assert(position4<length4);
        assert(buffer1);
        assert(buffer2);
        assert(buffer3);
        assert(buffer4);
        
        
        while(samples--){
            
            // read delay tap.
            int tap1=buffer1[position1];
            int tap2=buffer2[position2];
            int tap3=buffer3[position3];
            int tap4=buffer4[position4];
            
            // apply lpf for certain tap.
            {
                int delta=tap1-filter1;
                delta=((int64_t)delta*(uint64_t)lpfDamp)>>32;
                filter1+=delta;
                tap1=filter1;
            }
            {
                int delta=tap2-filter2;
                delta=((int64_t)delta*(uint64_t)lpfDamp)>>32;
                filter2+=delta;
                tap2=filter2;
            }
            {
                int delta=tap3-filter3;
                delta=((int64_t)delta*(uint64_t)lpfDamp)>>32;
                filter3+=delta;
                tap3=filter3;
            }
            {
                int delta=tap4-filter4;
                delta=((int64_t)delta*(uint64_t)lpfDamp)>>32;
                filter4+=delta;
                tap4=filter4;
            }
            
            // add input.
            {
                int input=*(inBufferMono++);
                tap1+=input;
                tap2+=input;
            }
            
            // apply gain.
            tap1=((int64_t)tap1*(uint64_t)gain)>>32;
            tap2=((int64_t)tap2*(uint64_t)gain)>>32;
            tap3=((int64_t)tap3*(uint64_t)gain)>>32;
            tap4=((int64_t)tap4*(uint64_t)gain)>>32;
            
            // multiply feedback matrix.
            buffer1[position1]=tap2+tap3;
            buffer2[position2]=-(tap1+tap4);
            /**(outBufferStereo++)+=
            buffer3[position3]=tap1-tap4;
            *(outBufferStereo++)+=
            buffer4[position4]=tap2-tap3;*/
			
			*(outBufferStereo++)+=
            buffer3[position3]+tap1;
            *(outBufferStereo++)+=
            buffer4[position4]-tap2;
			
            buffer3[position3]=tap1-tap4;
            buffer4[position4]=tap2-tap3;
            
            
            
            // advance position.
            
            position1++;
            if(position1==length1)
                position1=0;
            
            position2++;
            if(position2==length2)
                position2=0;
            
            
            position3++;
            if(position3==length3)
                position3=0;
            
            
            position4++;
            if(position4==length4)
                position4=0;
            
        }
        
        positions[0]=position1;
        positions[1]=position2;
        positions[2]=position3;
        positions[3]=position4;
        filterStates[0]=filter1;
        filterStates[1]=filter2;
        filterStates[2]=filter3;
        filterStates[3]=filter4;
    }
    
};

struct TXMatrixReverb1Reflections{
    enum{
        bufferSize=4096,
        tapCount=8
    };
    
    int32_t history[bufferSize];
    unsigned int position;
    unsigned int delays[tapCount];
    
    TXMatrixReverb1Reflections(float sampleRate){
        static const float delayTimes[]={
            512.f/44100.f, 1143.f/44100.f, 
            1537.f/44100.f, 1848.f/44100.f,
            411.f/44100.f, 1197.f/44100.f, 
            1639.f/44100.f, 1933.f/44100.f
        };
        memset(history, 0, sizeof(history));
        
        for(size_t i=0;i<tapCount;i++){
            unsigned int tm=(unsigned int)(delayTimes[i]*sampleRate*2.f);
            delays[i]=tm;
        }
        
        position=0;
    }
    
    void applyAdditive(int32_t *outBufferStereo,
               const int32_t *inBufferMono,
               size_t samples){
        
        int32_t *historyBuffer=history;
        const register size_t bufferSizeMask=bufferSize-1;
        register unsigned int currentPosition=position;
        unsigned int delay1=delays[0];
        unsigned int delay2=delays[1];
        unsigned int delay3=delays[2];
        unsigned int delay4=delays[3];
        unsigned int delay5=delays[4];
        unsigned int delay6=delays[5];
        unsigned int delay7=delays[6];
        unsigned int delay8=delays[7];
        
        while(samples--){
            
            {
                register int32_t out=*(outBufferStereo);
                register int tmp;
                
                tmp=historyBuffer[(currentPosition+delay1)&bufferSizeMask];
                out+=tmp;
                
                tmp=historyBuffer[(currentPosition+delay2)&bufferSizeMask];
                out+=tmp;
                
                tmp=historyBuffer[(currentPosition+delay3)&bufferSizeMask];
                out+=tmp-(tmp>>2);
                
                tmp=historyBuffer[(currentPosition+delay4)&bufferSizeMask];
                out+=tmp>>1;
                
                *(outBufferStereo++)=out;
            }
            
            {
                register int32_t out=*(outBufferStereo);
                register int tmp;
                
                tmp=historyBuffer[(currentPosition+delay5)&bufferSizeMask];
                out+=tmp;
                
                tmp=historyBuffer[(currentPosition+delay6)&bufferSizeMask];
                out+=tmp;
                
                tmp=historyBuffer[(currentPosition+delay7)&bufferSizeMask];
                out+=tmp-(tmp>>2);
                
                tmp=historyBuffer[(currentPosition+delay8)&bufferSizeMask];
                out+=tmp>>1;
                
                *(outBufferStereo++)=out;
            }
            
            
            historyBuffer[currentPosition]=*(inBufferMono++);
            
            if(currentPosition==0)
                currentPosition=bufferSize;
            currentPosition--;
        }
        
        position=currentPosition;
        delays[0]=delay1;
        delays[1]=delay2;
        delays[2]=delay3;
        delays[3]=delay4;
        delays[4]=delay5;
        delays[5]=delay6;
        delays[6]=delay7;
        delays[7]=delay8;
        
    }
};

TXMatrixReverb1::TXMatrixReverb1(const TXConfig& config):
m_config(config){
    
    m_parameter.inGain=.5f;
    m_parameter.reverbTime=2.3f;
    m_parameter.highFrequencyDamp=.5f;
    m_parameter.earlyReflectionGain=.5f;
    
    // feedback networks.
    static const float delayTimes[TXMatrixReverb1Network::NetworkSize]={
        4029.f / 44100.f,
        4351.f / 44100.f,
        5843.f / 44100.f,
        6361.f / 44100.f
    };
    unsigned int delayLengths[TXMatrixReverb1Network::NetworkSize];
    
    for(int i=0;i<TXMatrixReverb1Network::NetworkSize;i++){
        float secs=delayTimes[i];
        unsigned int samples;
        samples=(unsigned int)(secs*config.sampleRate);
        //samples>>=1;
        
        // make it odd
        if((samples&1)==0)
            samples++;
        
        delayLengths[i]=samples;
    }
    
    m_network=new TXMatrixReverb1Network(delayLengths);
    m_reflections=new TXMatrixReverb1Reflections(config.sampleRate);
    
    // allpass filters.
    static const float allpassTimes[AllpassFiltersCount]={
        1433.f / 44100.f,
        1493.f / 44100.f,
        1571.f / 44100.f,
        1621.f / 44100.f,
        
        2709.f / 44100.f, 
        3831.f / 44100.f,
        2877.f / 44100.f,
        4098.f / 44100.f
    };
    
    for(int i=0;i<AllpassFiltersCount;i++){
        m_allpassFilters[i]=new TXAllpassDelayFilter
        (config.sampleRate, allpassTimes[i]*.5f);
    }
    
    updateParameter();
}

#pragma mark - TXMatrixReverb1


static TXStandardFactory<TXMatrixReverb1> 
g_sharedFactory("MatrixReverb1",
                "com.nexhawks.TXSynthesizer.MatrixReverb1",
                TXPluginTypeEffect);

TXFactory *TXMatrixReverb1::sharedFactory(){
    return &g_sharedFactory;
}

TXMatrixReverb1::~TXMatrixReverb1(){
    for(int i=0;i<4;i++)
        delete m_allpassFilters[i];
    delete m_network;
    delete m_reflections;
}

void TXMatrixReverb1::setParameter(const TXMatrixReverb1::Parameter &param){
    m_parameter=param;
    updateParameter();
}

static unsigned int integerValue(float v){
    if(v<0.f)
        v=0.f;
    if(v>.99999f)
        v=.99999f;
    return (unsigned int)(v*65536.f*65536.f);
}

void TXMatrixReverb1::updateParameter(){
    const float referenceDelayTime=5843.f/44100.f ;
    
    // reverb time (RT60)
    float gain;
    gain=powf(1e-3f, referenceDelayTime/m_parameter.reverbTime);
    assert(gain<1.f);
    
    // normalize network matrix
    gain/=sqrtf(2.f);
    
    m_network->gain=integerValue(gain);
    
    // high freq. damp
    m_network->highFrequencyDamp=integerValue((m_parameter.highFrequencyDamp));
    
    // others.
    m_inGain=integerValue(m_parameter.inGain);
    m_erGain=integerValue(m_parameter.earlyReflectionGain)>>1;
}

static void convertToMono(int32_t *outBuffer,
                          const int32_t *inBuffer,
                          unsigned int samples,
                          unsigned int inGain){
    while(samples--){
        int32_t value=*(inBuffer++);
        value+=*(inBuffer++);
        value=((int64_t)value*(uint64_t)inGain)>>32;
        *(outBuffer++)=value>>1;
    }
}

static void applyGain(int32_t *outBuffer,
                      unsigned int samples,
                      unsigned int inGain){
    while(samples--){
        int32_t value=*outBuffer;
        value=((int64_t)value*(uint64_t)inGain)>>32;
        *(outBuffer++)=value;
    }
}

void TXMatrixReverb1::applyStereo
(int32_t *outBuffer, const int32_t *inBuffer, 
 unsigned int samples){
    int32_t *monoBuffer=(int32_t *)alloca(samples*4);
    int32_t *monoBuffer2=(int32_t *)alloca(samples*4);
    
    convertToMono(monoBuffer, 
                  inBuffer, samples,
                  m_inGain);
    
    memcpy(monoBuffer2, monoBuffer,
           samples*4);
    
    for(int i=0;i<6;i++){
        m_allpassFilters[i]->applyMonoInplace(monoBuffer,
                                              samples);
    }
    
    m_network->apply(outBuffer, 
                     monoBuffer, 
                     samples);
    
    applyGain(monoBuffer2, samples, m_erGain);
    /*
    m_allpassFilters[4]->applyMonoInplace(monoBuffer2, samples);
    m_allpassFilters[5]->applyMonoInplace(monoBuffer2, samples);*/
    
    m_reflections->applyAdditive(outBuffer, monoBuffer2, samples);
    
    /*
    memcpy(monoBuffer2, monoBuffer,
           samples*4);
    
    m_allpassFilters[4]->applyMonoInplace(monoBuffer, samples);
    m_allpassFilters[5]->applyMonoToSingleStereoAdditive(outBuffer, monoBuffer, samples);
    
    m_allpassFilters[6]->applyMonoInplace(monoBuffer2, samples);
    m_allpassFilters[7]->applyMonoToSingleStereoAdditive(outBuffer+1, monoBuffer2, samples);*/
    
    /*
    while(samples--){
        int32_t v=*(monoBuffer++);
        *(outBuffer++)=v;
        *(outBuffer++)=v;
        
    }*/
    
}

#pragma mark - Serialization

#define ParameterInGainKey "InGain"
#define ParameterReverbTimeKey "ReverbTime"
#define ParameterHighFrequencyDampKey "HighFrequencyDamp"
#define ParameterEarlyReflectionGainKey "EarlyReflectionGain"

TPLObject *TXMatrixReverb1::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(inGain));
        dic->setObject(&*obj, ParameterInGainKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(reverbTime));
        dic->setObject(&*obj, ParameterReverbTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(highFrequencyDamp));
        dic->setObject(&*obj, ParameterHighFrequencyDampKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(earlyReflectionGain));
        dic->setObject(&*obj, ParameterEarlyReflectionGainKey);
    }
    return dic;
}

void TXMatrixReverb1::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterInGainKey));
        if(num)
            inGain=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterReverbTimeKey));
        if(num)
            reverbTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterHighFrequencyDampKey));
        if(num)
            highFrequencyDamp=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(ParameterEarlyReflectionGainKey));
        if(num)
            earlyReflectionGain=num->doubleValue();
        
    }
}


TPLObject *TXMatrixReverb1::serialize() const{
    return m_parameter.serialize();
    
}

void TXMatrixReverb1::deserialize(const TPLObject *obj){
    TXMatrixReverb1::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}



