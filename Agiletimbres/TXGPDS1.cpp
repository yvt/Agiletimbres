//
//  TXGPDS1.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXGPDS1.h"
#include "TXFactory.h"
#include "TXBlepTable.h"
#include "TXSineTable.h"
#include "TXBiquadFilter.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLArray.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"
#include "TPList/TPLString.h"

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif

static TXStandardFactory<TXGPDS1> 
g_sharedFactory("GPDS1 (Hybrid Synth)",
                "com.nexhawks.TXSynthesizer.GPDS1",
                TXPluginTypeInstrument);

TXFactory *TXGPDS1::sharedFactory(){
    return &g_sharedFactory;
}

#pragma mark - Initialization

TXGPDS1::TXGPDS1(const TXConfig& config){
    m_sampleFreq=config.sampleRate;
    m_basePeriodScale=65536.f*m_sampleFreq;
    m_pitchbendScale=0x10000;
    
    {
        Oscilator& osc=m_parameter.osc1;
        osc.waveformType=WaveformTypeSawtooth;
        osc.transposeCoarse=0;
        osc.transposeFine=0;
        osc.fixStart=true;
        osc.volume=.13f;
        osc.sustainLevel=.8f;
        osc.decayTime=2.f;
        osc.releaseTime=.5f;
        osc.lfoDepth=8.f;
        osc.lfoFreq=7.f;
        osc.lfoDelay=0.3f;
    }
    {
        Oscilator& osc=m_parameter.osc2;
        osc=m_parameter.osc1;
        osc.transposeCoarse=0;
        osc.transposeFine=-7;
       // osc.lfoDepth=20.f;
        //osc.lfoFreq=9.f;
    }
    {
        Oscilator& osc=m_parameter.osc3;
        osc=m_parameter.osc1;
        osc.waveformType=WaveformTypeSawtooth;
        osc.transposeCoarse=0;
        osc.transposeFine=3;
        
        //osc.lfoDepth=40.f;
        //osc.lfoFreq=12.f;
        
    }
    
    {
        Filter& flt=m_parameter.flt;
        flt.enable=true;
        flt.filterType=FilterTypeLpf12LC;
        flt.resonance=.4f;
        flt.initialFreq=0.f;
        flt.attackTime=.01f;
        flt.attackFreq=13000.f;
        flt.decayTime=5.f;
        flt.sustainFreq=3000;
        flt.releaseTime=.02f;
    }
    
    m_parameter.polyphonics=4;
}

TXGPDS1::~TXGPDS1(){
    
}

#pragma mark - Synthesizer

#pragma mark - Oscilator

#define BlepDisabled    ((unsigned int)-1)
#define BlepCount 2

struct TXGPDS1::OscilatorState{
    /* oscilator status */
    const Oscilator *oscilator;
    OscilatorState *syncTarget; // master oscilator for oscilator sync
    uint32_t phase;             // 
    uint32_t oldPhase;          // saved for sync
    uint32_t basePeriod;         // per sample
    uint32_t currentPeriod;      // per sample
    uint32_t phaseFactor; // (phase*phaseFactor)>>32=0x10000
    unsigned int bleps[BlepCount];      // 0 - TXBlepTablePeriods*TXBlepTableOversample-1 or
                                // BlepDisabled when disabled
    int blepLevels[BlepCount]; // s20.12
    unsigned int time;          // samples
    int gain;                   // s22.10
    bool off;
    bool dead;
    unsigned int offTime;
    
    /* converted envelope parameters */
    unsigned int attackTime;
    unsigned int decayTime;
    int sustainLevel;
    unsigned int releaseTime;
    
    /* envelope status */
    int lastEnvelope;           // s2.30
    int nextEnvelope;           // s2.30
    int envelopeDelta;          // s2.30 per sample
    
    /* noise generator */
    uint32_t randomStateX;
    uint32_t randomStateY;
    uint32_t randomStateZ;
    uint32_t randomStateW;
    
    /* LFO */
    int lfoDelay;      // samples
    unsigned int lfoSpeed;      // s0.32 per sample 
    unsigned int lfoDepth;      // millicents
    
    /* Spread for Poly and Mono */
    int leftAnalogModulationSamples;
    int analogModulationPeriodSamples;
    int previousAnalogPitch;
    int nextAnalogPitch;
    int maxAnalogPitchSpread;
    bool needsAnalogModulation;
    
    int newAnalogPitch() const{
        int v=((int)rand()<<1)&0xffff;
        int mcents=((int64_t)maxAnalogPitchSpread*(int64_t)(v-0x8000))>>15;
        return (unsigned int)
        (scaleForMillicents(mcents)*(65536.f*256.f));
    }
    
    OscilatorState(){}
    OscilatorState(const Oscilator& osc,
                   const Parameter& param,
                   int key, int velocity,
                   float basePeriodScale,
                   float sampleFreq){
        oscilator=&osc;
        
        int shiftedKey=key+osc.transposeCoarse;
        int cents=osc.transposeFine;
        
        float baseFreq=TXInstrument::frequencyForNote(shiftedKey);
        if(cents!=0)
            baseFreq*=TXInstrument::scaleForCents(cents);
        
        basePeriod=(uint32_t)(basePeriodScale/baseFreq);
        if(basePeriod==0) // too high?
            basePeriod=0x40000000;
        time=0;
        
        needsAnalogModulation=((param.polyphonicsMode==PolyphonicsModePoly||
                                param.polyphonicsMode==PolyphonicsModeMono) &&
                               (param.maxSpread!=0.f));
        if(needsAnalogModulation){
            maxAnalogPitchSpread=(int)(param.maxSpread*1000.f);
            analogModulationPeriodSamples=((int)sampleFreq)>>3;
            leftAnalogModulationSamples=analogModulationPeriodSamples;
            nextAnalogPitch=newAnalogPitch();
            previousAnalogPitch=newAnalogPitch();
        }
        
        if(osc.fixStart)
            phase=0;
        else{
            phase=((uint64_t)(uint32_t)((uint32_t)rand()^((uint32_t)rand()<<17))*(uint64_t)basePeriod)>>32;
            assert(phase<basePeriod);
        }
            
        currentPeriod=basePeriod;
        gain=(int)(osc.volume*(float)((velocity*(velocity+1))>>4));
        syncTarget=NULL;
        dead=(gain==0);
        off=false;
        
        randomStateX=((uint32_t)rand()^((uint32_t)rand()<<15)^((uint32_t)rand()<<30));
        randomStateY=((uint32_t)rand()^((uint32_t)rand()<<15)^((uint32_t)rand()<<30));
        randomStateZ=((uint32_t)rand()^((uint32_t)rand()<<15)^((uint32_t)rand()<<30));
        randomStateW=88675123;
        
        bleps[0]=BlepDisabled;
        bleps[1]=BlepDisabled;
        
        attackTime=(int)(osc.attackTime*sampleFreq);
        decayTime=(int)(osc.decayTime*sampleFreq);
        if(osc.sustainLevel<0.f)
            sustainLevel=0;
        else if(osc.sustainLevel>=.999f)
            sustainLevel=0x3fffffffL;
        else
            sustainLevel=(int)(osc.sustainLevel*(float)0x3fffffffL);
        releaseTime=(int)(osc.releaseTime*sampleFreq);
        
        if(attackTime)
            lastEnvelope=0;
        else
            lastEnvelope=0x3fffffffL;
        nextEnvelope=lastEnvelope;
        
        // initialize LFO.
        if(osc.lfoDepth!=0.f){
            lfoDepth=(unsigned int)(osc.lfoDepth*1000.f);
            lfoDelay=(int)(osc.lfoDelay*sampleFreq);
            lfoSpeed=(unsigned int)(osc.lfoFreq*65536.f*65536.f/sampleFreq);
        }else{
            lfoDepth=0;
        }
    }
    
    int lfoWave(unsigned int phase){
        switch(oscilator->lfoWaveformType){
            case LfoWaveformTypeSine:
            default:
                return TXSineWave(phase);
            case LfoWaveformTypeTriangle:
                phase+=0x40000000;
                phase&=0xffffffff;
                if(phase<0x80000000){
                    return (int)(phase>>15)-32768;
                }else{
                    phase-=0x80000000;
                    return 32768-(int)(phase>>15);
                }
            case LfoWaveformTypeSawtoothUp:
                return 32768-(int)(phase>>16);
            case LfoWaveformTypeSawtoothDown:
                return (int)(phase>>16)-32768;
            case LfoWaveformTypeSquare:
                return (phase<0x80000000)?32767:-32767;
        }
    }
    
    void calcCurrentPeriod(unsigned int samples){
        if(!basePeriod)
            return;
        unsigned int newPeriod=basePeriod;
        assert(phase<currentPeriod);
        
        // TODO: LFO...
        //newPeriod+=((rand()%1000)-500)*100;
        /*if(syncTarget){
            int tmp=time>>2;
            tmp&=0xffff;
            if(tmp>0x8000)
                tmp=0x10000-tmp;
            newPeriod>>=2;
            newPeriod+=(((uint64_t)newPeriod*(uint64_t)tmp)>>15)*3;
        }*/
        
        if(needsAnalogModulation){
            uint32_t per=leftAnalogModulationSamples;
            per=(uint32_t)(((uint64_t)per<<32)/analogModulationPeriodSamples);
            
            int mods=nextAnalogPitch;
            int diff=previousAnalogPitch-nextAnalogPitch;
            diff=(int)((((int64_t)diff)*(uint64_t)per)>>32);
            mods+=diff;
            
            newPeriod=((uint64_t)newPeriod*(uint64_t)mods)>>24;
            
            leftAnalogModulationSamples-=samples;
            while(leftAnalogModulationSamples<0){
                previousAnalogPitch=nextAnalogPitch;
                nextAnalogPitch=newAnalogPitch();
                leftAnalogModulationSamples+=analogModulationPeriodSamples;
            }
        }
        
        if(lfoDepth>10){
            if(lfoDelay<0 || time>=lfoDelay){
                uint32_t lfoPhase=(time-lfoDelay)*lfoSpeed;
                int val=lfoWave(lfoPhase);
                if(oscilator->lfoPeriodBase){
                    unsigned int minPeriodScale=(unsigned int)
                    (scaleForMillicents(-lfoDepth)*(65536.f*256.f));
                    int periodScaleDelta=(int)(0x1000000 - minPeriodScale);
                    unsigned int lfoScale=0x1000000;
                    lfoScale+=((int64_t)periodScaleDelta*(int64_t)val)>>15;
                    newPeriod=((uint64_t)newPeriod*(uint64_t)lfoScale)>>24;
                }else{
                    int lfoMCents=((int64_t)lfoDepth*(int64_t)val)>>15;
                    unsigned int lfoScale=(unsigned int)
                    (scaleForMillicents(lfoMCents)*(65536.f*256.f));
                    newPeriod=((uint64_t)newPeriod*(uint64_t)lfoScale)>>24;
                }
            }
        }
        if(newPeriod<0x1000)
            newPeriod=0x1000;
        
        // scale phase.
        if(newPeriod!=currentPeriod){
            uint64_t finalRatio=(((uint64_t)newPeriod<<32)/currentPeriod);
            phase=((uint64_t)phase*(uint64_t)finalRatio)>>32;
            if(phase>=newPeriod)
                phase=newPeriod-1;
        }
        currentPeriod=newPeriod;
        phaseFactor=(uint32_t)((uint64_t)0x1000000000000ULL/(uint64_t)currentPeriod);
        
        // save for oscilator sync.
        oldPhase=phase;
        assert(phase<currentPeriod);
    }
    
    void calcEnvelope(unsigned int samples){
        lastEnvelope=nextEnvelope;
        
        unsigned int envelope;
        time+=samples;
        
        // attack/sustain
        if(time<attackTime){
            envelope=(time<<10)/attackTime;
            if(envelope>1023)
                envelope=1023;
            
            unsigned int tmp=envelope;
            for(int i=1;i<oscilator->attackPower;i++)
                envelope=(envelope*tmp)>>10;
            
            envelope<<=20;
        }else if(time<attackTime+decayTime){
            unsigned int level, levelTmp;
            level=1024-((time-attackTime)<<10)/decayTime;
            levelTmp=level;
            for(int i=1;i<oscilator->decayPower;i++)
                level=(level*levelTmp)>>10;
            
            envelope=sustainLevel;
            envelope+=((uint64_t)(0x3fffffff-envelope)*(uint64_t)level)>>10;
        }else{
            envelope=sustainLevel;
            if(sustainLevel==0)
                dead=true;
        }
        
        // release.
        if(off){
            unsigned int timeSinceOff=time-offTime;
            if(timeSinceOff>=releaseTime){
                envelope=0;
                dead=true;
            }else{
                unsigned int level, levelTmp;
                level=1024-(timeSinceOff<<10)/releaseTime;
                levelTmp=level;
                for(int i=1;i<oscilator->releasePower;i++)
                    level=(level*levelTmp)>>10;
               
                
                envelope=((uint64_t)envelope*(uint64_t)level)>>10;
            }
        }
        
        nextEnvelope=envelope;
        envelopeDelta=(nextEnvelope-lastEnvelope)/(int)samples;
        
        //time+=samples;
    }
    
    static unsigned int countOfFrontZeroBits(uint32_t value){
        unsigned int count=0;
        while (value<0x80000000) {
            value<<=1;
            count++;
        }
        return count;
    }
    
#define addBlep(blepScale, blepPhase) \
do{ \
    if(blepPhase>=0x10000) break; \
    if(blep1==BlepDisabled){ \
        blep1=(blepPhase)>>(16-TXBlepTableOversampleShift); \
        blepLevels[0]=blepScale; \
        assert(blep1<TXBlepTableOversample); \
        break; \
    } \
    if(blep2==BlepDisabled){ \
        blep2=(blepPhase)>>(16-TXBlepTableOversampleShift); \
        blepLevels[1]=blepScale;  \
        assert(blep2<TXBlepTableOversample); \
        break; \
    } \
    blep1=BlepDisabled; \
}while(true);
    
    void renderAdditiveUniformSquare(int32_t *out,
                               unsigned int samples){
        unsigned int leftSamples=samples;
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register uint32_t halfPeriod=period>>1;
        const int16_t *blepTable=TXBlepTable;
        unsigned int blep1=bleps[0];
        unsigned int blep2=bleps[1];
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        int bias=-32768;
        
        assert(currentPhase<period);
        
        if(oscilator->waveformType==WaveformTypePulse4){
            halfPeriod>>=1; bias>>=1;
        }else if(oscilator->waveformType==WaveformTypePulse8){
            halfPeriod>>=2; bias>>=2;
        }else if(oscilator->waveformType==WaveformTypePulse16){
            halfPeriod>>=3; bias>>=3;
        }
        
        if(halfPeriod<0x20000)
            halfPeriod=0x20000;
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // square
            if(currentPhase<halfPeriod){
                currentPhase+=0x10000;
                if(currentPhase<halfPeriod){
                    outSample=bias+65536;
                }else{
                    outSample=bias;
                    addBlep((4)<<12, currentPhase-halfPeriod);
                }
            }else{
                currentPhase+=0x10000;
                if(currentPhase>=period){
                    outSample=bias+65536;
                    currentPhase-=period;
                     addBlep((-4)<<12, currentPhase);
                    
                }else{
                    outSample=bias;
                }
            }
            
            if(blep1!=BlepDisabled){
                assert(blep1<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep1]*blepLevels[0])>>12;
                blep1+=TXBlepTableOversample;
                if(blep1>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep1=BlepDisabled;
                }
            }
            
            if(blep2!=BlepDisabled){
                assert(blep2<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep2]*blepLevels[1])>>12;
                blep2+=TXBlepTableOversample;
                if(blep2>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep2=BlepDisabled;
                }
            }
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        bleps[0]=blep1;
        bleps[1]=blep2;
        
    }
    
    void renderAdditiveUniformSawtooth(int32_t *out,
                                     unsigned int samples){
        unsigned int leftSamples=samples;
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register const uint32_t factor=phaseFactor;
        const int16_t *blepTable=TXBlepTable;
        unsigned int blep1=bleps[0];
        unsigned int blep2=bleps[1];
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        
        assert(currentPhase<period);
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // sawtooth
            currentPhase+=0x10000;
            if(currentPhase>=period){
                currentPhase-=period;
                addBlep((4)<<12, currentPhase);
            }
            outSample=((uint64_t)currentPhase*(uint64_t)factor)>>32;
            outSample-=32768;
            
            if(blep1!=BlepDisabled){
                assert(blep1<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep1]*blepLevels[0])>>12;
                blep1+=TXBlepTableOversample;
                if(blep1>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep1=BlepDisabled;
                }
            }
            
            if(blep2!=BlepDisabled){
                assert(blep2<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep2]*blepLevels[1])>>12;
                blep2+=TXBlepTableOversample;
                if(blep2>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep2=BlepDisabled;
                }
            }
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        bleps[0]=blep1;
        bleps[1]=blep2;
        
    }
    
    void renderAdditiveUniformSine(int32_t *out,
                                       unsigned int samples){
        unsigned int leftSamples=samples;
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register const uint32_t factor=phaseFactor;
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        const int16_t *sineTable=TXSineTable;
        
        assert(currentPhase<period);
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // sine
            currentPhase+=0x10000;
            if(currentPhase>=period){
                currentPhase-=period;
            }
            outSample=((uint64_t)currentPhase*(uint64_t)factor)>>32;
            
            {
                int32_t outSample2, per;
                // scale to sine table.
                per=outSample&((1<<(16-(TXSineTableSizeShift+2)))-1);
                outSample>>=(16-(TXSineTableSizeShift+2));
                
                assert(outSample<TXSineTableSize*4);
                assert(outSample>=0);
                assert(per>=0);
                assert(per<(1<<(16-(TXSineTableSizeShift+2))));
                
                if(outSample&(2<<TXSineTableSizeShift)){
                    assert(outSample>=(2<<TXSineTableSizeShift));
                    assert(outSample<(4<<TXSineTableSizeShift));
                    
                    outSample-=(2<<TXSineTableSizeShift);
                    
                    // 0 ... pi
                    if(outSample&(1<<TXSineTableSizeShift)){
                        assert(outSample>=(1<<TXSineTableSizeShift));
                        // (1/2)pi ... pi
                        outSample=(2<<TXSineTableSizeShift)-outSample;
                        outSample2=outSample-1;
                    }else{
                        assert(outSample<(1<<TXSineTableSizeShift));
                        // 0 ... (1/2)pi
                        outSample2=outSample+1;
                    }
                    
                    
                    
                    assert(outSample<=TXSineTableSize);
                    outSample=sineTable[outSample];
                    outSample2=sineTable[outSample2];
                    outSample+=((outSample2-outSample)*per)>>
                    (16-(TXSineTableSizeShift+2));
                    outSample=-outSample;
                }else{
                    assert(outSample>=0);
                    assert(outSample<(2<<TXSineTableSizeShift));
                    
                    // 0 ... pi
                    if(outSample&(1<<TXSineTableSizeShift)){
                        assert(outSample>=(1<<TXSineTableSizeShift));
                        // (1/2)pi ... pi
                        outSample=(2<<TXSineTableSizeShift)-outSample;
                        outSample2=outSample-1;
                    }else{
                        assert(outSample<(1<<TXSineTableSizeShift));
                        // 0 ... (1/2)pi
                        outSample2=outSample+1;
                    }
                    
                    assert(outSample<=TXSineTableSize);
                    outSample=sineTable[outSample];
                    outSample2=sineTable[outSample2];
                    outSample+=((outSample2-outSample)*per)>>
                    (16-(TXSineTableSizeShift+2));
                }
            }
            
            //printf("%d\n", outSample);
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        
    }
    
    void renderAdditiveUniform(int32_t *out,
                               unsigned int samples){
        if(currentPeriod<65536*4){
            renderAdditiveUniformSine(out, samples);
            return;
        }
        switch(oscilator->waveformType){
            case WaveformTypeSquare:
            case WaveformTypePulse4:
            case WaveformTypePulse8:
            case WaveformTypePulse16:
                renderAdditiveUniformSquare(out, samples);
                break;
            case WaveformTypeSawtooth:
                renderAdditiveUniformSawtooth(out, samples);
                break;
            case WaveformTypeSine:
                renderAdditiveUniformSine(out, samples);
                break;
            case WaveformTypeNoise:
                assert(false);
                break;
        }
    }
    
    void renderAdditiveSyncSquare(int32_t *out,
                                     unsigned int samples){
        unsigned int leftSamples=samples;
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register uint32_t halfPeriod=period>>1;
        register uint32_t masterPhase=syncTarget->oldPhase;
        register const uint32_t masterPeriod=syncTarget->currentPeriod;
        const int16_t *blepTable=TXBlepTable;
        unsigned int blep1=bleps[0];
        unsigned int blep2=bleps[1];
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        
        assert(currentPhase<period);
        assert(masterPhase<masterPeriod);
        
        if(oscilator->waveformType==WaveformTypePulse4){
            halfPeriod>>=1;
        }else if(oscilator->waveformType==WaveformTypePulse8){
            halfPeriod>>=2;
        }else if(oscilator->waveformType==WaveformTypePulse16){
            halfPeriod>>=3;
        }
        
        if(halfPeriod<0x20000)
            halfPeriod=0x20000;
        /*
        printf("%d: %d %d %d %d\n", basePeriod, period, masterPeriod,
               phase, masterPhase);*/
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // square
            if(currentPhase<halfPeriod){
                currentPhase+=0x10000;
                if(currentPhase<halfPeriod){
                    outSample=32768;
                    
                    masterPhase+=0x10000;
                    if(masterPhase>=masterPeriod){
                        // syncing... but no change in waveform.
                        masterPhase-=masterPeriod;
                        currentPhase=masterPhase;
                    }
                    
                }else{
                    masterPhase+=0x10000;
                    
                    if(masterPhase>=masterPeriod){
                        
                        // conflicting sync.
                        masterPhase-=masterPeriod;
                        
                        unsigned int leak1=currentPhase-halfPeriod;
                        
                        if(leak1<masterPhase){
                            // slave trigger is earlier.
                            addBlep((4)<<12, leak1);
                            
                            // sync transistion happens.
                            addBlep((-4)<<12, masterPhase);
                        }else{
                            // master trigger is earlier.
                        }
                        
                        currentPhase=masterPhase;
                        
                        outSample=32768;
                        
                    }else{
                        
                        outSample=~32768;
                        addBlep((4)<<12, currentPhase-halfPeriod);
                        
                    }
                }
            }else{
                currentPhase+=0x10000;
                if(currentPhase>=period){
                    
                    masterPhase+=0x10000;
                    if(masterPhase>=masterPeriod){
                        
                        // conflicting.
                        
                        masterPhase-=masterPeriod;
                        unsigned int leak=currentPhase-period;
                        
                        if(leak<masterPhase){
                            // slave trigger is earlier.
                            addBlep((-4)<<12, leak);
                        }else{
                            // master trigger is earlier.
                            addBlep((-4)<<12, masterPhase);
                        }
                        
                        outSample=32768;
                    
                        currentPhase=masterPhase;
                        
                    }else{
                        
                        outSample=32768;
                        currentPhase-=period;
                        addBlep((-4)<<12, currentPhase);
                        
                    }
                    
                }else{
                    
                    masterPhase+=0x10000;
                    
                    if(masterPhase>=masterPeriod){
                        // syncing...
                        masterPhase-=masterPeriod;
                        outSample=32768;
                        currentPhase=masterPhase;
                        addBlep((-4)<<12, currentPhase);
                    }else{
                        outSample=~32768;
                    }
                    
                }
            }
            
            if(blep1!=BlepDisabled){
                assert(blep1<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep1]*blepLevels[0])>>12;
                blep1+=TXBlepTableOversample;
                if(blep1>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep1=BlepDisabled;
                }
            }
            
            if(blep2!=BlepDisabled){
                assert(blep2<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep2]*blepLevels[1])>>12;
                blep2+=TXBlepTableOversample;
                if(blep2>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep2=BlepDisabled;
                }
            }
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        bleps[0]=blep1;
        bleps[1]=blep2;
        
    }

    
    void renderAdditiveSyncSawtooth(int32_t *out,
                                       unsigned int samples){
        unsigned int leftSamples=samples;
        //unsigned int shift=countOfFrontZeroBits(currentSpeed);
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register uint32_t masterPhase=syncTarget->oldPhase;
        register const uint32_t masterPeriod=syncTarget->currentPeriod;
        register const uint32_t factor=phaseFactor;
        const int16_t *blepTable=TXBlepTable;
        unsigned int blep1=bleps[0];
        unsigned int blep2=bleps[1];
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        
        assert(currentPhase<period);
        assert(masterPhase<masterPeriod);
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // synced sawtooth
            currentPhase+=0x10000;
            if(currentPhase>=period){
                unsigned int leakedPhase1=currentPeriod-period;
                
                masterPhase+=0x10000;
                if(masterPhase>=masterPeriod){
                    
                    // conflicting! (both osc were triggered)
                    
                    unsigned int leakedPhase2=masterPhase-masterPeriod;
                    
                    if(leakedPhase2<leakedPhase1){
                        // master first
                        currentPhase=leakedPhase2;
                    }else{
                        // slave(this) first
                        currentPhase=leakedPhase1;
                    }
                    
                    masterPhase=leakedPhase2;
                    
                }else{
                
                    currentPhase=leakedPhase1;
                    
                }
                
                // reduce code size
                outSample=(4<<12);
                goto aboutToAddBlep;
                /* addBlep((4)<<12, currentPhase); */
            }else{
                
                masterPhase+=0x10000;
                if(masterPhase>=masterPeriod){
                    
                    // syncing...
                    masterPhase-=masterPeriod;
                    
                    // compute slave osc'S value before syncing in order to
                    // generate ripple.
                    outSample=((uint64_t)currentPhase*(uint64_t)factor)>>32;
                    
                    currentPhase=masterPhase;
                    
                    // generate ripple
                    outSample>>=2;
                aboutToAddBlep:
                    addBlep(outSample, currentPhase);
                    
                }
                
            }
            outSample=((uint64_t)currentPhase*(uint64_t)factor)>>32;
            outSample-=32768;
            
            if(blep1!=BlepDisabled){
                assert(blep1<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep1]*blepLevels[0])>>12;
                blep1+=TXBlepTableOversample;
                if(blep1>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep1=BlepDisabled;
                }
            }
            
            if(blep2!=BlepDisabled){
                assert(blep2<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep2]*blepLevels[1])>>12;
                blep2+=TXBlepTableOversample;
                if(blep2>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep2=BlepDisabled;
                }
            }
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        bleps[0]=blep1;
        bleps[1]=blep2;
        
    }
    
    void renderAdditiveSyncSine(int32_t *out,
                                   unsigned int samples){
        unsigned int leftSamples=samples;
        register uint32_t currentPhase=phase;
        register const uint32_t period=currentPeriod;
        register const uint32_t factor=phaseFactor;
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        const int16_t *blepTable=TXBlepTable;
        unsigned int blep1=bleps[0];
        unsigned int blep2=bleps[1];
        const int16_t *sineTable=TXSineTable;
        register uint32_t masterPhase=syncTarget->oldPhase;
        register const uint32_t masterPeriod=syncTarget->currentPeriod;
        
        assert(currentPhase<period);
         assert(masterPhase<masterPeriod);
        
        while(leftSamples--){
            
            int32_t outSample;
            
            // sine
            currentPhase+=0x10000;
            if(currentPhase>=period){
                currentPhase-=period;
            }
            outSample=((uint64_t)currentPhase*(uint64_t)factor)>>32;
            
            {
                int32_t outSample2, per;
                // scale to sine table.
                per=outSample&((1<<(16-(TXSineTableSizeShift+2)))-1);
                outSample>>=(16-(TXSineTableSizeShift+2));
                
                assert(outSample<TXSineTableSize*4);
                assert(outSample>=0);
                assert(per>=0);
                assert(per<(1<<(16-(TXSineTableSizeShift+2))));
                
                if(outSample&(2<<TXSineTableSizeShift)){
                    assert(outSample>=(2<<TXSineTableSizeShift));
                    assert(outSample<(4<<TXSineTableSizeShift));
                    
                    outSample-=(2<<TXSineTableSizeShift);
                    
                    // 0 ... pi
                    if(outSample&(1<<TXSineTableSizeShift)){
                        assert(outSample>=(1<<TXSineTableSizeShift));
                        // (1/2)pi ... pi
                        outSample=(2<<TXSineTableSizeShift)-outSample;
                        outSample2=outSample-1;
                    }else{
                        assert(outSample<(1<<TXSineTableSizeShift));
                        // 0 ... (1/2)pi
                        outSample2=outSample+1;
                    }
                    
                    
                    
                    assert(outSample<=TXSineTableSize);
                    outSample=sineTable[outSample];
                    outSample2=sineTable[outSample2];
                    outSample+=((outSample2-outSample)*per)>>
                    (16-(TXSineTableSizeShift+2));
                    outSample=-outSample;
                }else{
                    assert(outSample>=0);
                    assert(outSample<(2<<TXSineTableSizeShift));
                    
                    // 0 ... pi
                    if(outSample&(1<<TXSineTableSizeShift)){
                        assert(outSample>=(1<<TXSineTableSizeShift));
                        // (1/2)pi ... pi
                        outSample=(2<<TXSineTableSizeShift)-outSample;
                        outSample2=outSample-1;
                    }else{
                        assert(outSample<(1<<TXSineTableSizeShift));
                        // 0 ... (1/2)pi
                        outSample2=outSample+1;
                    }
                    
                    assert(outSample<=TXSineTableSize);
                    outSample=sineTable[outSample];
                    outSample2=sineTable[outSample2];
                    outSample+=((outSample2-outSample)*per)>>
                    (16-(TXSineTableSizeShift+2));
                }
            }
            
            // master oscilator.
            masterPhase+=0x10000;
            if(masterPhase>=masterPeriod){
                // syncing...
                masterPhase-=masterPeriod;
                
                currentPhase=masterPhase;
                
                // generate ripple
                outSample=outSample>>2;
                addBlep(outSample, currentPhase);
                
                outSample=0;
            }
            
            if(blep1!=BlepDisabled){
                assert(blep1<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep1]*blepLevels[0])>>12;
                blep1+=TXBlepTableOversample;
                if(blep1>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep1=BlepDisabled;
                }
            }
            
            if(blep2!=BlepDisabled){
                assert(blep2<TXBlepTableOversample*TXBlepTablePeriods);
                outSample+=((int)blepTable[blep2]*blepLevels[1])>>12;
                blep2+=TXBlepTableOversample;
                if(blep2>=TXBlepTableOversample*TXBlepTablePeriods){
                    blep2=BlepDisabled;
                }
            }
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        assert(currentPhase<period);
        phase=currentPhase;
        
        bleps[0]=blep1;
        bleps[1]=blep2;
        
    }
    
    void renderAdditiveSync(int32_t *out,
                               unsigned int samples){
        if(!syncTarget->basePeriod)
            return;
        switch(oscilator->waveformType){
            case WaveformTypeSquare:
            case WaveformTypePulse4:
            case WaveformTypePulse8:
            case WaveformTypePulse16:
                renderAdditiveSyncSquare(out, samples);
                break;
            case WaveformTypeSawtooth:
                renderAdditiveSyncSawtooth(out, samples);
                break;
            case WaveformTypeSine:
                renderAdditiveSyncSine(out, samples);
                break;
            case WaveformTypeNoise:
                assert(false);
                break;
        }
    }
    
    void renderAdditiveSilence(int32_t *out,
                               unsigned int samples){
        uint32_t currentPhase=phase;
        uint32_t period=currentPeriod;
        assert(currentPhase<period);
        while(samples--){
            currentPhase+=65536;
            while(currentPhase>=currentPeriod){
                currentPhase-=currentPeriod;
            }
        }
        phase=currentPhase;
        assert(currentPhase<period);
    }
    
    /* Marsaglia (2003 July). 
       “Xorshift RNGs”Journal of Statistical Software Vol. 8 
       (Issue  14). */
    void renderAdditiveNoise(int32_t *out,
                             unsigned int samples){
        
        // forward phase for sync oscilator.
        renderAdditiveSilence(out, samples);
        
        unsigned int leftSamples=samples;
        int envelope=((int64_t)lastEnvelope*(int64_t)gain)>>10;
        int deltaEnvelope=((int64_t)envelopeDelta*(int64_t)gain)>>10;
        
        uint32_t x=randomStateX;
        uint32_t y=randomStateY;
        uint32_t z=randomStateZ;
        uint32_t w=randomStateW;
        
        while(leftSamples--){
            
            int32_t outSample;
            
            uint32_t t=x^(x<<11);
            x=y; y=z; z=w;
            w=(w^(w>>19))^(t^(t>>8));
            outSample=(w>>16);
            outSample-=32768;
            
            outSample=((int64_t)(outSample<<2)*(int64_t)envelope)>>32;
            
            envelope+=deltaEnvelope;
            
            *out+=outSample;
            out++;
        }
        
        randomStateX=x;
        randomStateY=y;
        randomStateZ=z;
        randomStateW=w;
        
        
    }
    
    
    void renderAdditive(int32_t *out,
                        unsigned int samples){
        if(!basePeriod)
            return;
        calcEnvelope(samples);
        if(basePeriod<65536*2 ||
           currentPeriod<65536*2 || gain<2){
            renderAdditiveSilence(out, samples);
        }else if(oscilator->waveformType==WaveformTypeNoise){
            renderAdditiveNoise(out, samples);
        }else if(syncTarget){
            renderAdditiveSync(out, samples);
        }else{
            renderAdditiveUniform(out, samples);
        }
        
    }
};

#pragma mark - Filter

struct TXGPDS1::FilterState{
    
    const Filter *filter;
    
    unsigned int initialFreq;   // s8.24
    unsigned int resonance;     // s8.24
    unsigned int attackTime;    // samples
    unsigned int attackFreq;    // s8.24
    unsigned int decayTime;     // samples
    unsigned int sustainFreq;   // s8.24
    unsigned int releaseTime;   // samples
    unsigned int releaseFreq;   // s8.24
    
    unsigned int time;
    bool off;
    unsigned int offTime;
    
    unsigned int lastFreq;
    unsigned int nextFreq;
    
    union{
        struct{
            int in[4];
            int out[4];
        } moogState;
        struct{
            int charge;
            int current;
        } lc6State;
        struct{
            int charge[2];
            int current[2];
        } lc12State;
		struct{
			int vLp, vBp, vHp;
		} sidState;
    };
    
    FilterState(){}
    FilterState(const Filter& flt,
                int key, int velocity,
                float sampleFreq){
        filter=&flt;
        
        if(!flt.enable)
            return;
        
        float timeScale=sampleFreq;
        float freqScale=(2.f*65536.f*256.f)/sampleFreq;
        
        initialFreq=(unsigned int)(flt.initialFreq*freqScale);
        attackFreq=(unsigned int)(flt.attackFreq*freqScale);
        sustainFreq=(unsigned int)(flt.sustainFreq*freqScale);
        releaseFreq=(unsigned int)(flt.releaseFreq*freqScale);
        resonance=(unsigned int)(flt.resonance*65536.f*256.f);
        attackTime=(unsigned int)(flt.attackTime*timeScale);
        decayTime=(unsigned int)(flt.decayTime*timeScale);
        releaseTime=(unsigned int)(flt.releaseTime*timeScale);
        
        if(flt.filterType==FilterTypeLpf24Moog){
            resonance+=1<<22;
            resonance+=1<<21;
            
        }else if(flt.filterType==FilterTypeHpf24Moog){
            resonance+=1<<22;
            resonance+=1<<21;
            
        }else if(flt.filterType==FilterTypeLpf12LC){
            resonance-=resonance>>3;
            
        }else if(flt.filterType==FilterTypeHpf12LC){
            resonance-=resonance>>3;
            
        }
        
        nextFreq=initialFreq;
        time=0;
        off=false;
        if(nextFreq>0xffffff)
            nextFreq=0xffffff;
        if(nextFreq<0x80000)
            nextFreq=0x80000;
        if(resonance>0xff0fff)
            resonance=0xff0fff;
        
        if(flt.filterType==FilterTypeLpf24Moog){
            moogState.in[0]=0;
            moogState.in[1]=0;
            moogState.in[2]=0;
            moogState.in[3]=0;
            moogState.out[0]=0;
            moogState.out[1]=0;
            moogState.out[2]=0;
            moogState.out[3]=0;
        }else if(flt.filterType==FilterTypeLpf6LC){
            lc6State.charge=0;
            lc6State.current=0;
        }else if(flt.filterType==FilterTypeLpf12LC){
            lc12State.charge[0]=0;
            lc12State.current[0]=0;
            lc12State.charge[1]=0;
            lc12State.current[1]=0;
        }else if(flt.filterType==FilterTypeHpf24Moog){
            moogState.in[0]=0;
            moogState.in[1]=0;
            moogState.in[2]=0;
            moogState.in[3]=0;
            moogState.out[0]=0;
            moogState.out[1]=0;
            moogState.out[2]=0;
            moogState.out[3]=0;
        }else if(flt.filterType==FilterTypeHpf6LC){
            lc6State.charge=0;
            lc6State.current=0;
        }else if(flt.filterType==FilterTypeHpf12LC){
            lc12State.charge[0]=0;
            lc12State.current[0]=0;
            lc12State.charge[1]=0;
            lc12State.current[1]=0;
        }else if(flt.filterType==FilterTypeLpf12SID ||
				 flt.filterType==FilterTypeHpf12SID ||
				 flt.filterType==FilterTypeBpf6SID ||
				 flt.filterType==FilterTypeNotch6SID){
            sidState.vLp=0;
			sidState.vBp=0;
			sidState.vHp=0;
        }
        
        
    }
    
    void calcEnvelope(unsigned int samples){
        time+=samples;
        lastFreq=nextFreq;
        
        if(time<attackTime){
            nextFreq=initialFreq;
            int delta=(int)attackFreq-(int)initialFreq;
            delta=int((int64_t)delta*(int64_t)time/(int64_t)attackTime);
            nextFreq+=delta;
        }else if(time<attackTime+decayTime){
            nextFreq=attackFreq;
            int delta=(int)sustainFreq-(int)attackFreq;
            delta=int((int64_t)delta*(int64_t)(time-attackTime)/(int64_t)decayTime);
            nextFreq+=delta;
        }else{
            nextFreq=sustainFreq;
        }
        
        if(off){
            if(time-offTime<releaseTime){
                int delta=(int)releaseFreq-(int)sustainFreq;
                delta=int((int64_t)delta*(int64_t)(time-offTime)/(int64_t)releaseTime);
                nextFreq+=delta;
            }else{
                nextFreq+=(int)releaseFreq-(int)sustainFreq;
            }
        }
        
        if(nextFreq>0x80000000UL)
            nextFreq=0;
        
        if(nextFreq>0xffffff)
            nextFreq=0xffffff;
        if(nextFreq<0x80000)
            nextFreq=0x80000;
        
    }
    
    template<bool secondChannel>
    void applyLpf12LCAdditive(int32_t *out,
                             const int32_t *src,
                             unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int charge1=lc12State.charge[0];
        int current1=lc12State.current[0];
        int charge2=lc12State.charge[1];
        int current2=lc12State.current[1];
        
        unsigned int reso=resonance<<8;
        while(samples--){
            
            int input=*(src++)<<8;
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<7;
            // scale freq so that resonance is enhanced.
            actualFreq+=((uint64_t)actualFreq*(uint64_t)reso)>>32;
            
            current1=((int64_t)current1*(uint64_t)reso)>>32;
            current1+=((int64_t)((input-charge1)<<1)*(uint64_t)actualFreq)>>32;
            charge1+=((int64_t)(current1<<1)*(uint64_t)actualFreq)>>32;
            
            current2=((int64_t)current2*(uint64_t)reso)>>32;
            current2+=((int64_t)((charge1-charge2)<<1)*(uint64_t)actualFreq)>>32;
            charge2+=((int64_t)(current2<<1)*(uint64_t)actualFreq)>>32;
            
            *(out++)+=charge2>>8;
            if(secondChannel)
                *(out++)+=charge2>>8;
            else
                out++;
            
            
            freq+=freqDelta;
        }
        
        lc12State.charge[0]=charge1;
        lc12State.current[0]=current1;
        
        lc12State.charge[1]=charge2;
        lc12State.current[1]=current2;
        
    }
    
    template<bool secondChannel>
    void applyLpf6LCAdditive(int32_t *out,
                                const int32_t *src,
                                unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int charge=lc6State.charge;
        int current=lc6State.current;
        
        unsigned int reso=resonance<<8;
        while(samples--){
            
            int input=*(src++)<<8;
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<7;
            // scale freq so that resonance is enhanced.
            actualFreq+=((uint64_t)actualFreq*(uint64_t)reso)>>32;
            
            current=((int64_t)current*(uint64_t)reso)>>32;
            current+=((int64_t)((input-charge)<<1)*(uint64_t)actualFreq)>>32;
            charge+=((int64_t)(current<<1)*(uint64_t)actualFreq)>>32;
            
            *(out++)+=charge>>8;
            if(secondChannel)
                *(out++)+=charge>>8;
            else
                out++;
            
            
            freq+=freqDelta;
        }
        
        lc6State.charge=charge;
        lc6State.current=current;
        
    }
    
    template<bool secondChannel>
    void applyHpf6LCAdditive(int32_t *out,
                             const int32_t *src,
                             unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int charge=lc6State.charge;
        int current=lc6State.current;
        
        unsigned int reso=resonance<<8;
        while(samples--){
            
            int input=*(src++)<<8;
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<7;
            // scale freq so that resonance is enhanced.
            actualFreq+=((uint64_t)actualFreq*(uint64_t)reso)>>32;
            
            current=((int64_t)current*(uint64_t)reso)>>32;
            current+=((int64_t)((input-charge)<<1)*(uint64_t)actualFreq)>>32;
            charge+=((int64_t)(current<<1)*(uint64_t)actualFreq)>>32;
            
            *(out++)+=current>>8;
            if(secondChannel)
                *(out++)+=current>>8;
            else
                out++;
            
            
            freq+=freqDelta;
        }
        
        lc6State.charge=charge;
        lc6State.current=current;
        
    }
    
    template<bool secondChannel>
    void applyHpf12LCAdditive(int32_t *out,
                             const int32_t *src,
                             unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int charge1=lc12State.charge[0];
        int current1=lc12State.current[0];
        int charge2=lc12State.charge[1];
        int current2=lc12State.current[1];
        
        unsigned int reso=resonance<<8;
        while(samples--){
            
            int input=*(src++)<<8;
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<7;
            // scale freq so that resonance is enhanced.
            actualFreq+=((uint64_t)actualFreq*(uint64_t)reso)>>32;
            
            current1=((int64_t)current1*(uint64_t)reso)>>32;
            current1+=((int64_t)((input-charge1)<<1)*(uint64_t)actualFreq)>>32;
            charge1+=((int64_t)(current1<<1)*(uint64_t)actualFreq)>>32;
            
            current2=((int64_t)current2*(uint64_t)reso)>>32;
            current2+=((int64_t)((current1-charge2)<<1)*(uint64_t)actualFreq)>>32;
            charge2+=((int64_t)(current2<<1)*(uint64_t)actualFreq)>>32;
            
            *(out++)+=current2>>8;
            if(secondChannel)
                *(out++)+=current2>>8;
            else
                out++;
            
            
            freq+=freqDelta;
        }
        
        lc12State.charge[0]=charge1;
        lc12State.current[0]=current1;
        
        lc12State.charge[1]=charge2;
        lc12State.current[1]=current2;
        
    }

    
    template<bool secondChannel>
    void applyLpf24MoogAdditive(int32_t *out,
                                const int32_t *src,
                                unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int in1=moogState.in[0];
        int in2=moogState.in[1];
        int in3=moogState.in[2];
        int in4=moogState.in[3];
        int out1=moogState.out[0];
        int out2=moogState.out[1];
        int out3=moogState.out[2];
        int out4=moogState.out[3];
        unsigned int reso=resonance<<8;
        while(samples--){
            
            int input=*(src++)<<8;
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<8;
            input+=((int64_t)(input<<2)*(uint64_t)reso)>>32;
            /*
            if(actualFreq>0x1000000) // clamp in [0, fs/2]
                actualFreq=0x1000000;
            if(actualFreq<0x150000)
                actualFreq=0x150000;*/
            
            // do moog filter
            // actualFreq+=actualFreq>>3; // *1.125
            
            unsigned int powerFreq=actualFreq;
            powerFreq=((uint64_t)powerFreq*(uint64_t)powerFreq)>>32;
            
            unsigned int fb=(0xffffffff-(powerFreq>>1));
            fb=((uint64_t)fb*(uint64_t)reso)>>32;
            
            unsigned int quatFreq=powerFreq;
            quatFreq=((uint64_t)quatFreq*(uint64_t)quatFreq)>>32;
            
            // hard limit
            if(out4>0x1000000)
                out4=0x1000000;
            if(out4<-0x1000000)
                out4=-0x1000000;
            
            input-=((int64_t)(out4<<2)*(int64_t)fb)>>32;
            input=(input>>2)+(input>>3);
            input=((int64_t)input*(int64_t)quatFreq)>>32;
            
            out1-=((int64_t)out1*(uint64_t)actualFreq)>>32;
            out1+=input;
            out1+=in1>>2;
            in1=input;
            
            out2-=((int64_t)out2*(uint64_t)actualFreq)>>32;
            out2+=out1;
            out2+=in2>>2;
            in2=out1;
            
            out3-=((int64_t)out3*(uint64_t)actualFreq)>>32;
            out3+=out2;
            out3+=in3>>2;
            in3=out2;
            
            out4-=((int64_t)out4*(uint64_t)actualFreq)>>32;
            out4+=out3;
            out4+=in4>>2;
            in4=out3;
            
            *(out++)+=out4>>8;
            if(secondChannel)
            *(out++)+=out4>>8;
            else
                out++;
            
            out1-=out1>>7;
            out2-=out2>>7;
            out3-=out3>>7;
            out4-=out4>>7;
            
            
            freq+=freqDelta;
        }
        
        moogState.in[0]=in1;
        moogState.in[1]=in2;
        moogState.in[2]=in3;
        moogState.in[3]=in4;
        moogState.out[0]=out1;
        moogState.out[1]=out2;
        moogState.out[2]=out3;
        moogState.out[3]=out4;
        
    }
	
	inline uint32_t SID6581Distortion(unsigned int freq, int dist){
		if(dist>=524288)dist=524288-1;
		if(dist>=0)
			freq+=dist<<9;
		if(freq>0xffffff)
			freq=0xffffff;
		return freq<<8;
		
		
		uint32_t base=70642790+freq*6;
		
		// offset
		dist<<=5;
		dist+=256*1024;
		dist+=0x10000;
		dist-=freq>>6;
		//dist<<=1;
		//dist=(int32_t)(((int64_t)dist*(uint64_t)resonance)>>24);
		
		if(dist<=0) return base;
		
		uint32_t distu=dist;
		
		if(distu>=524288)distu=524288-1; // avoid overflow.
		distu<<=13;
		
		uint32_t del=77369598U+freq*233U;
		
		uint32_t d=base+(uint32_t)(((uint64_t)del*(uint64_t)distu)>>32);
		//printf("%u %u %u\n", freq, distu, d);
		if(d>=0x40000000)d=0x40000000-1;
		d<<=2;
		return d;
	}
	
	
	
	/* based on Antii Lankila's 6581 distortion
	 * http://bel.fi/~alankila/c64-sw/index-cpp.html */
	template<bool secondChannel,
	bool flagLp, bool flagBp, bool flagHp>
    void applySID6581Additive(int32_t *out,
							 const int32_t *src,
							 unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int vLp=sidState.vLp;
		int vBp=sidState.vBp;
		int vHp=sidState.vHp;
        
        unsigned int reso=resonance*209U;
		
		reso=0xffffffff-reso;
		//reso=0xffffffff;
        while(samples--){
            
            int input=*(src++);
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq;
            
			int vf=0;
			int vi=input;
			if(vi<-0x40000) vi=-0x40000;
			if(vi>0x40000)vi=0x40000;
			vi<<=1;
			
			if(flagLp)
				vf+=vLp;
			if(flagBp)
				vf+=vBp;
			if(flagHp)
				vf+=vHp;
			
			if(flagBp){
				int d=vi>>1;
				d+=vHp+vLp;
				d-=(int32_t)(((int64_t)vBp*(uint64_t)reso)>>32);
				vf-=d>>1;
				vBp+=(vf-vBp)>>12;
			}
			if(flagLp)
				vLp+=(vf-vLp)>>12;
			if(flagHp)
				vHp+=(vf-vHp)>>12;
			
			if(vf>3200000){
				vf-=(vf-3200000)>>1;
			}
			
			{
				uint32_t w0=SID6581Distortion(actualFreq, 
											  vBp);
				vLp-=(int32_t)(((int64_t)vBp*(uint64_t)w0)>>32);
			}
			{
				uint32_t w0=SID6581Distortion(actualFreq, 
											  vHp);
				vBp-=(int32_t)(((int64_t)vHp*(uint64_t)w0)>>32);
			}
			{
				vHp=(int32_t)(((int64_t)vBp*(uint64_t)reso)>>32);
				vHp-=vLp>>1;
				vHp-=vi>>1;
			}
			
			vf>>=1;
			*(out++)+=vf;
            if(secondChannel)
                *(out++)+=vf;
            else
                out++;
            
            freq+=freqDelta;
        }
        
        sidState.vLp=vLp;
		sidState.vBp=vBp;
		sidState.vHp=vHp;
		
        
    }

    
    template<bool secondChannel>
    void applyAdditive(int32_t *out,
                       const int32_t *src,
                       unsigned int samples){
        assert(filter->enable);
        calcEnvelope(samples);
        
        if(filter->filterType==FilterTypeLpf24Moog){
            applyLpf24MoogAdditive<secondChannel>(out, src, samples);
        }else if(filter->filterType==FilterTypeLpf6LC){
            applyLpf6LCAdditive<secondChannel>(out, src, samples);
        }else if(filter->filterType==FilterTypeLpf12LC){
            applyLpf12LCAdditive<secondChannel>(out, src, samples);
        }else if(filter->filterType==FilterTypeHpf6LC){
            applyHpf6LCAdditive<secondChannel>(out, src, samples);
        }else if(filter->filterType==FilterTypeHpf12LC){
            applyHpf12LCAdditive<secondChannel>(out, src, samples);
        }else if(filter->filterType==FilterTypeLpf12SID){
			applySID6581Additive<secondChannel, 
			true, false, false>(out, src, samples);
		}else if(filter->filterType==FilterTypeHpf12SID){
			applySID6581Additive<secondChannel, 
			false, false, true>(out, src, samples);
		}else if(filter->filterType==FilterTypeBpf6SID){
			applySID6581Additive<secondChannel, 
			false, true, false>(out, src, samples);
		}else if(filter->filterType==FilterTypeNotch6SID){
			applySID6581Additive<secondChannel, 
			true, false, true>(out, src, samples);
		}
		
		else{
            assert(false);
        }
    }
};

#pragma mark - Management

#include "TXAllpassDelayFilter.h"

enum ChannelMode{
    ChannelModeStereo,
    ChannelModeLeft,
    ChannelModeRight
};

struct TXGPDS1::Channel{
    OscilatorState osc1;
    OscilatorState osc2;
    OscilatorState osc3;
    FilterState flt;
    int velocity;
    int key;
    bool off;
    bool dead;
    unsigned int time;
    ChannelMode mode;
};

void TXGPDS1::purgeOldestChannel(){
    if(m_channels.empty())
        return;
    
    unsigned int lastTime=m_channels.begin()->time;
    ChannelList::iterator it2=m_channels.begin();
    
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        if(it->time>lastTime){
            lastTime=it->time;
            it2=it;
        }
    }
    
    m_channels.erase(it2);
}

int TXGPDS1::maxPolyphonics(){
    if(m_parameter.polyphonicsMode==PolyphonicsModeMono ||
       m_parameter.polyphonicsMode==PolyphonicsModePolyUnison){
        return ((m_parameter.polyphonics+1)>>1)<<1;
    }else{
        return m_parameter.polyphonics;
    }
}

void TXGPDS1::noteOn(int key, int velocity){
    
    if(m_parameter.polyphonicsMode==PolyphonicsModeMono){
        allSoundsOff();
        
        int polys=maxPolyphonics();
        for(int i=0;i<polys;i++){
            Channel channel;
            
            channel.osc1=OscilatorState(m_parameter.osc1,
                                        m_parameter,
                                        key, velocity,
                                        m_basePeriodScale,
                                        m_sampleFreq);
            channel.osc2=OscilatorState(m_parameter.osc2,
                                        m_parameter,
                                        key, velocity,
                                        m_basePeriodScale,
                                        m_sampleFreq);
            channel.osc3=OscilatorState(m_parameter.osc3,
                                        m_parameter,
                                        key, velocity,
                                        m_basePeriodScale,
                                        m_sampleFreq);
            
            
            channel.flt=FilterState(m_parameter.flt,
                                    key,velocity,
                                    m_sampleFreq);
            
            channel.velocity=velocity;
            channel.key=key;
            channel.dead=false;
            channel.off=false;
            channel.time=0;
            channel.mode=((i&1)?ChannelModeLeft:
                          ChannelModeRight);
            
            m_channels.push_back(channel);
        }
        
    }else if(m_parameter.polyphonicsMode==PolyphonicsModePoly){
        
        if(m_channels.size()>=maxPolyphonics())
            purgeOldestChannel();
        
        Channel channel;
        
        channel.osc1=OscilatorState(m_parameter.osc1,
                                    m_parameter,
                                    key, velocity,
                                    m_basePeriodScale,
                                    m_sampleFreq);
        channel.osc2=OscilatorState(m_parameter.osc2,
                                    m_parameter,
                                    key, velocity,
                                    m_basePeriodScale,
                                    m_sampleFreq);
        channel.osc3=OscilatorState(m_parameter.osc3,
                                    m_parameter,
                                    key, velocity,
                                    m_basePeriodScale,
                                    m_sampleFreq);
        
        
        channel.flt=FilterState(m_parameter.flt,
                                key,velocity,
                                m_sampleFreq);
        
        channel.velocity=velocity;
        channel.key=key;
        channel.dead=false;
        channel.off=false;
        channel.time=0;
        channel.mode=ChannelModeStereo;
        
        m_channels.push_back(channel);
        
    }else if(m_parameter.polyphonicsMode==PolyphonicsModePolyUnison){
        if(m_channels.size()>=maxPolyphonics())
            purgeOldestChannel();
        if(m_channels.size()+1>=maxPolyphonics())
            purgeOldestChannel();
        
        int spreadMillicents=(int)(m_parameter.maxSpread*1000.f);
        float scale1=scaleForMillicents(spreadMillicents);
        float scale2=1.f/scale1; // inverted
        
        for(int i=0;i<2;i++){
            Channel channel;
            float basePeriodScale=m_basePeriodScale;
            basePeriodScale*=(i?scale1:scale2);
            
            channel.osc1=OscilatorState(m_parameter.osc1,
                                        m_parameter,
                                        key, velocity,
                                        basePeriodScale,
                                        m_sampleFreq);
            channel.osc2=OscilatorState(m_parameter.osc2,
                                        m_parameter,
                                        key, velocity,
                                        basePeriodScale,
                                        m_sampleFreq);
            channel.osc3=OscilatorState(m_parameter.osc3,
                                        m_parameter,
                                        key, velocity,
                                        basePeriodScale,
                                        m_sampleFreq);
            
            
            channel.flt=FilterState(m_parameter.flt,
                                    key,velocity,
                                    m_sampleFreq);
            
            channel.velocity=velocity;
            channel.key=key;
            channel.dead=false;
            channel.off=false;
            channel.time=0;
            channel.mode=(i?ChannelModeLeft:
                          ChannelModeRight);
            
            m_channels.push_back(channel);
        }
        
    }
    
   
    
    //Channel& ch=channel;
}

void TXGPDS1::noteOff(int key, int velocity){
    
    if(m_parameter.polyphonicsMode==PolyphonicsModePoly){
        
        for(ChannelList::iterator it=m_channels.begin();
            it!=m_channels.end();it++){
            Channel& ch=*it;
            if(ch.key==key && !ch.off){
                // found!
                ch.off=true;
                return;
            }
        }
        
    }else if(m_parameter.polyphonicsMode==PolyphonicsModePolyUnison){
        
        int left=2; // find left and right channel.
        for(ChannelList::iterator it=m_channels.begin();
            it!=m_channels.end();it++){
            Channel& ch=*it;
            if(ch.key==key && !ch.off){
                // found!
                ch.off=true;
                left--;
                if(left<=0)
                    break;
            }
        }
        
    }else if(m_parameter.polyphonicsMode==PolyphonicsModeMono){
        if(m_channels.empty())
            return;
        if(m_channels.begin()->key!=key)
            return;
        allNotesOff();
        
    }
}

void TXGPDS1::setPitchbend(int millicents){
    m_pitchbendScale=(uint32_t)(65536.f*scaleForMillicents(-millicents));
}

void TXGPDS1::allNotesOff(){
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        Channel& ch=*it;
        ch.off=true;
    }
}

void TXGPDS1::allSoundsOff(){
    m_channels.clear();
}




void TXGPDS1::renderChannelAdditive(int32_t *outBuffer,
                                    unsigned int samples,
                                    TXGPDS1::Channel &ch){
    
    int32_t *waveBuffer=(int32_t *)alloca(samples*4);
    memset(waveBuffer, 0, samples*4);
    ch.time+=samples;
    if(ch.off){
        if(!ch.osc1.off){
            ch.osc1.off=true; 
            ch.osc1.offTime=ch.osc1.time;
        }
        if(!ch.osc2.off){
            ch.osc2.off=true; 
            ch.osc2.offTime=ch.osc2.time;
        }
        if(!ch.osc3.off){
            ch.osc3.off=true; 
            ch.osc3.offTime=ch.osc3.time;
        }
        if(!ch.flt.off){
            ch.flt.off=true;
            ch.flt.offTime=ch.flt.time;
        }
    }
    switch(m_parameter.osc1.syncTarget){
        case 1:
            ch.osc1.syncTarget=&ch.osc1;
            break;
        case 2:
            ch.osc1.syncTarget=&ch.osc2;
            break;
        case 3:
            ch.osc1.syncTarget=&ch.osc3;
            break;
        default:
            ch.osc1.syncTarget=NULL;
    }
    switch(m_parameter.osc2.syncTarget){
        case 1:
            ch.osc2.syncTarget=&ch.osc1;
            break;
        case 2:
            ch.osc2.syncTarget=&ch.osc2;
            break;
        case 3:
            ch.osc2.syncTarget=&ch.osc3;
            break;
        default:
            ch.osc2.syncTarget=NULL;
    }
    switch(m_parameter.osc3.syncTarget){
        case 1:
            ch.osc3.syncTarget=&ch.osc1;
            break;
        case 2:
            ch.osc3.syncTarget=&ch.osc2;
            break;
        case 3:
            ch.osc3.syncTarget=&ch.osc3;
            break;
        default:
            ch.osc3.syncTarget=NULL;
    }
    
    ch.osc1.calcCurrentPeriod(samples);
    ch.osc2.calcCurrentPeriod(samples);
    ch.osc3.calcCurrentPeriod(samples);
    
    ch.osc1.renderAdditive(waveBuffer, samples);
    ch.osc2.renderAdditive(waveBuffer, samples);
    ch.osc3.renderAdditive(waveBuffer, samples);
    
    switch(ch.mode){
        case ChannelModeStereo:
            if(m_parameter.flt.enable){
                ch.flt.applyAdditive<true>(outBuffer, waveBuffer, 
                                           samples);
            }else{
                while(samples--){
                    int32_t wave=*(waveBuffer++);
                    *(outBuffer++)+=wave;
                    *(outBuffer++)+=wave;
                }
            }
            break;
        case ChannelModeRight:
            if(m_parameter.flt.enable){
                ch.flt.applyAdditive<false>(outBuffer, waveBuffer, 
                                           samples);
            }else{
                while(samples--){
                    int32_t wave=*(waveBuffer++);
                    *(outBuffer++)+=wave;
                    outBuffer++;
                }
            }
            break;
        case ChannelModeLeft:
            if(m_parameter.flt.enable){
                ch.flt.applyAdditive<false>(outBuffer+1, waveBuffer, 
                                            samples);
            }else{
                while(samples--){
                    int32_t wave=*(waveBuffer++);
                    outBuffer++;
                    *(outBuffer++)+=wave;
                }
            }
            break;
    }
    
    
    if(ch.osc1.dead && ch.osc2.dead && ch.osc3.dead)
        ch.dead=true;
}



void TXGPDS1::renderFragmentAdditive(int32_t *out,
                                     unsigned int samples){
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        Channel& ch=*it;
        renderChannelAdditive(out, samples, ch);
    }
    
    // purge dead channels
    ChannelList::iterator it, it2;
    it=m_channels.begin();
    while(it!=m_channels.end()){
        it2=it; it2++;
        if(it->dead){
            
            m_channels.erase(it);
        }
        it=it2;
    }
}

#pragma mark - Setting Parameter

void TXGPDS1::setParameter(const TXGPDS1::Parameter &newParam){
    bool shouldReset=false;
    
    if(m_parameter.flt.filterType!=newParam.flt.filterType){
        shouldReset=true;
    }
    if(m_parameter.polyphonicsMode!=newParam.polyphonicsMode){
        shouldReset=true;
    }
    if(m_parameter.polyphonics!=newParam.polyphonics){
        shouldReset=true;
    }
    
    if(shouldReset){
        m_channels.clear();
    }
    
    m_parameter=newParam;
    
}

#define OscilatorWaveformTypeKey "WaveformType"
#define OscilatorWaveformTypeSineName "Sine"
#define OscilatorWaveformTypeSquareName "Square"
#define OscilatorWaveformTypePulse4Name "Pulse 1/4"
#define OscilatorWaveformTypePulse8Name "Pulse 1/8"
#define OscilatorWaveformTypePulse16Name "Pulse 1/16"
#define OscilatorWaveformTypeSawtoothName "Sawtooth"
#define OscilatorWaveformTypeNoiseName "Noise"
#define OscilatorLfoWaveformTypeKey "LfoWaveformType"
#define OscilatorLfoWaveformTypeSineName "Sine"
#define OscilatorLfoWaveformTypeSquareName "Square"
#define OscilatorLfoWaveformTypeSawtoothUpName "SawtoothUp"
#define OscilatorLfoWaveformTypeSawtoothDownName "SawtoothDown"
#define OscilatorLfoWaveformTypeTriangleName "Triangle"
#define OscilatorSyncTargetKey "SyncTarget"
#define OscilatorTransposeCoarseKey "TransposeCoarse"
#define OscilatorTransposeFineKey "TransposeFine"
#define OscilatorFixStartKey "FixStart"
#define OscilatorVolumeKey "Volume"
#define OscilatorAttackTimeKey "AttackTime"
#define OscilatorAttackPowerKey "AttackPower"
#define OscilatorDecayTimeKey "DecayTime"
#define OscilatorDecayPowerKey "DecayPower"
#define OscilatorSustainLevelKey "SustainLevel"
#define OscilatorReleaseTimeKey "ReleaseTime"
#define OscilatorReleasePowerKey "ReleasePower"
#define OscilatorLfoDepthKey "LfoDepth"
#define OscilatorLfoFrequencyKey "LfoFrequency"
#define OscilatorLfoDelayKey "LfoDelay"
#define OscilatorLfoPeriodBaseKey "IsLfoPeriodBase"


TPLString *TXGPDS1::Oscilator::waveformTypeName() const{
    switch(waveformType){
        default:
        case WaveformTypeSine:
            return new TPLString(OscilatorWaveformTypeSineName);
        case WaveformTypeSquare:
            return new TPLString(OscilatorWaveformTypeSquareName);
        case WaveformTypePulse4:
            return new TPLString(OscilatorWaveformTypePulse4Name);
        case WaveformTypePulse8:
            return new TPLString(OscilatorWaveformTypePulse8Name);
        case WaveformTypePulse16:
            return new TPLString(OscilatorWaveformTypePulse16Name);
        case WaveformTypeSawtooth:
            return new TPLString(OscilatorWaveformTypeSawtoothName);
        case WaveformTypeNoise:
            return new TPLString(OscilatorWaveformTypeNoiseName);
            
    }
}

void TXGPDS1::Oscilator::setWaveformTypeName(const TPLString *name){
    if(name->isEqualToUTF8String(OscilatorWaveformTypeSineName))
        waveformType=WaveformTypeSine;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypeSquareName))
        waveformType=WaveformTypeSquare;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypePulse4Name))
        waveformType=WaveformTypePulse4;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypePulse8Name))
        waveformType=WaveformTypePulse8;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypePulse16Name))
        waveformType=WaveformTypePulse16;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypeSawtoothName))
        waveformType=WaveformTypeSawtooth;
    else if(name->isEqualToUTF8String(OscilatorWaveformTypeNoiseName))
        waveformType=WaveformTypeNoise;
    
}


TPLString *TXGPDS1::Oscilator::lfoWaveformTypeName() const{
    switch(lfoWaveformType){
        default:
        case LfoWaveformTypeSine:
            return new TPLString(OscilatorLfoWaveformTypeSineName);
        case LfoWaveformTypeSquare:
            return new TPLString(OscilatorLfoWaveformTypeSquareName);
        case LfoWaveformTypeSawtoothUp:
            return new TPLString(OscilatorLfoWaveformTypeSawtoothUpName);
        case LfoWaveformTypeSawtoothDown:
            return new TPLString(OscilatorLfoWaveformTypeSawtoothDownName);
        case LfoWaveformTypeTriangle:
            return new TPLString(OscilatorLfoWaveformTypeTriangleName);
            
    }
}

void TXGPDS1::Oscilator::setLfoWaveformTypeName(const TPLString *name){
    if(name->isEqualToUTF8String(OscilatorLfoWaveformTypeSineName))
        lfoWaveformType=LfoWaveformTypeSine;
    else if(name->isEqualToUTF8String(OscilatorLfoWaveformTypeSquareName))
        lfoWaveformType=LfoWaveformTypeSquare;
    else if(name->isEqualToUTF8String(OscilatorLfoWaveformTypeSawtoothUpName))
        lfoWaveformType=LfoWaveformTypeSawtoothUp;
    else if(name->isEqualToUTF8String(OscilatorLfoWaveformTypeSawtoothDownName))
        lfoWaveformType=LfoWaveformTypeSawtoothDown;
    else if(name->isEqualToUTF8String(OscilatorLfoWaveformTypeTriangleName))
        lfoWaveformType=LfoWaveformTypeTriangle;
    
}



TPLObject *TXGPDS1::Oscilator::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(waveformTypeName());
        dic->setObject(&*obj, OscilatorWaveformTypeKey);
    }
    if(syncTarget){
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(syncTarget));
        dic->setObject(&*obj, OscilatorSyncTargetKey);
    }else{
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(false));
        dic->setObject(&*obj, OscilatorSyncTargetKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(transposeCoarse));
        dic->setObject(&*obj, OscilatorTransposeCoarseKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(transposeFine));
        dic->setObject(&*obj, OscilatorTransposeFineKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(fixStart));
        dic->setObject(&*obj, OscilatorFixStartKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(volume));
        dic->setObject(&*obj, OscilatorVolumeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(attackTime));
        dic->setObject(&*obj, OscilatorAttackTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(attackPower));
        dic->setObject(&*obj, OscilatorAttackPowerKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(decayTime));
        dic->setObject(&*obj, OscilatorDecayTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(decayPower));
        dic->setObject(&*obj, OscilatorDecayPowerKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(sustainLevel));
        dic->setObject(&*obj, OscilatorSustainLevelKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(releaseTime));
        dic->setObject(&*obj, OscilatorReleaseTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(releasePower));
        dic->setObject(&*obj, OscilatorReleasePowerKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(lfoWaveformTypeName());
        dic->setObject(&*obj, OscilatorLfoWaveformTypeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(lfoDepth));
        dic->setObject(&*obj, OscilatorLfoDepthKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(lfoFreq));
        dic->setObject(&*obj, OscilatorLfoFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(lfoDelay));
        dic->setObject(&*obj, OscilatorLfoDelayKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(lfoPeriodBase));
        dic->setObject(&*obj, OscilatorLfoPeriodBaseKey);
    }
    return dic;
}

void TXGPDS1::Oscilator::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        TPLString *str;
        
        str=dynamic_cast<TPLString *>(dic->objectForKey(OscilatorWaveformTypeKey));
        if(str)
            setWaveformTypeName(str);
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(OscilatorSyncTargetKey));
        if(num)
            syncTarget=num->intValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(OscilatorTransposeCoarseKey));
        if(num)
            transposeCoarse=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(OscilatorTransposeFineKey));
        if(num)
            transposeFine=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(OscilatorFixStartKey));
        if(num)
            fixStart=num->boolValue();
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(OscilatorVolumeKey));
        if(num)
            volume=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorAttackTimeKey));
        if(num)
            attackTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorAttackPowerKey));
        if(num)
            attackPower=num->intValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorDecayTimeKey));
        if(num)
            decayTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorDecayPowerKey));
        if(num)
            decayPower=num->intValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorSustainLevelKey));
        if(num)
            sustainLevel=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorReleaseTimeKey));
        if(num)
            releaseTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorReleasePowerKey));
        if(num)
            releasePower=num->intValue();
        
        str=dynamic_cast<TPLString *>(dic->objectForKey(OscilatorLfoWaveformTypeKey));
        if(str)
            setLfoWaveformTypeName(str);
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorLfoDepthKey));
        if(num)
            lfoDepth=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorLfoFrequencyKey));
        if(num)
            lfoFreq=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorLfoDelayKey));
        if(num)
            lfoDelay=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(OscilatorLfoPeriodBaseKey));
        if(num)
            lfoPeriodBase=num->boolValue();
    }
}

#define FilterEnableKey "Enable"
#define FilterTypeKey "Type"
#define FilterTypeLpf6LCName "Lpf6LC"
#define FilterTypeLpf12LCName "Lpf12LC"
#define FilterTypeLpf12SIDName "Lpf12SID"
#define FilterTypeLpf24MoogName "Lpf24Moog"
#define FilterTypeHpf6LCName "Hpf6LC"
#define FilterTypeHpf12LCName "Hpf12LC"
#define FilterTypeHpf12SIDName "Hpf12SID"
#define FilterTypeHpf24MoogName "Hpf24Moog"
#define FilterTypeBpf6SIDName "Bpf6SID"
#define FilterTypeNotch6SIDName "Notch6SID"
#define FilterResonanceKey "Resonance"
#define FilterInitialFrequencyKey "InitialFrequency"
#define FilterAttackTimeKey "AttackTime"
#define FilterAttackFrequencyKey "AttackFrequency"
#define FilterDecayTimeKey "DecayTime"
#define FilterSustainFrequencyKey "SustainFrequency"
#define FilterReleaseFrequencyKey "ReleaseFrequency"
#define FilterReleaseTimeKey "ReleaseTime"

TPLString *TXGPDS1::Filter::filterTypeName() const{
    switch(filterType){
        default:
        case FilterTypeLpf6LC:
            return new TPLString(FilterTypeLpf6LCName);
        case FilterTypeLpf12LC:
            return new TPLString(FilterTypeLpf12LCName);
        case FilterTypeLpf12SID:
            return new TPLString(FilterTypeLpf12SIDName);
        case FilterTypeLpf24Moog:
            return new TPLString(FilterTypeLpf24MoogName);
        case FilterTypeHpf6LC:
            return new TPLString(FilterTypeHpf6LCName);
        case FilterTypeHpf12LC:
            return new TPLString(FilterTypeHpf12LCName);
        case FilterTypeHpf12SID:
            return new TPLString(FilterTypeHpf12SIDName);
        case FilterTypeHpf24Moog:
            return new TPLString(FilterTypeHpf24MoogName);
        case FilterTypeBpf6SID:
            return new TPLString(FilterTypeBpf6SIDName);
        case FilterTypeNotch6SID:
            return new TPLString(FilterTypeNotch6SIDName);
    }
}

void TXGPDS1::Filter::setFilterTypeName(const TPLString *name){
    if(name->isEqualToUTF8String(FilterTypeLpf6LCName))
        filterType=FilterTypeLpf6LC;
    else if(name->isEqualToUTF8String(FilterTypeLpf12LCName))
        filterType=FilterTypeLpf12LC;
    else if(name->isEqualToUTF8String(FilterTypeLpf12SIDName))
        filterType=FilterTypeLpf12SID;
    else if(name->isEqualToUTF8String(FilterTypeLpf24MoogName))
        filterType=FilterTypeLpf24Moog;
    else if(name->isEqualToUTF8String(FilterTypeHpf6LCName))
        filterType=FilterTypeHpf6LC;
    else if(name->isEqualToUTF8String(FilterTypeHpf12LCName))
        filterType=FilterTypeHpf12LC;
    else if(name->isEqualToUTF8String(FilterTypeHpf12SIDName))
        filterType=FilterTypeHpf12SID;
    else if(name->isEqualToUTF8String(FilterTypeHpf24MoogName))
        filterType=FilterTypeHpf24Moog;
    else if(name->isEqualToUTF8String(FilterTypeBpf6SIDName))
        filterType=FilterTypeBpf6SID;
	else if(name->isEqualToUTF8String(FilterTypeNotch6SIDName))
        filterType=FilterTypeNotch6SID;
}

TPLObject *TXGPDS1::Filter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(enable));
        dic->setObject(&*obj, FilterEnableKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(filterTypeName());
        dic->setObject(&*obj, FilterTypeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(resonance));
        dic->setObject(&*obj, FilterResonanceKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(initialFreq));
        dic->setObject(&*obj, FilterInitialFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(attackTime));
        dic->setObject(&*obj, FilterAttackTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(attackFreq));
        dic->setObject(&*obj, FilterAttackFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(decayTime));
        dic->setObject(&*obj, FilterDecayTimeKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(sustainFreq));
        dic->setObject(&*obj, FilterSustainFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(releaseFreq));
        dic->setObject(&*obj, FilterReleaseFrequencyKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(releaseTime));
        dic->setObject(&*obj, FilterReleaseTimeKey);
    }
    return dic;
}

void TXGPDS1::Filter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        TPLNumber *num;
        TPLString *str;
        
        num=dynamic_cast<TPLNumber *>(dic->objectForKey(FilterEnableKey));
        if(num)
            enable=num->boolValue();
        
        str=dynamic_cast<TPLString *>(dic->objectForKey(FilterTypeKey));
        if(str)
            setFilterTypeName(str);
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterInitialFrequencyKey));
        if(num)
            initialFreq=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterAttackTimeKey));
        if(num)
            attackTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterAttackFrequencyKey));
        if(num)
            attackFreq=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterDecayTimeKey));
        if(num)
            decayTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterSustainFrequencyKey));
        if(num)
            sustainFreq=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterReleaseFrequencyKey));
        if(num)
            releaseFreq=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterReleaseTimeKey));
        if(num)
            releaseTime=num->doubleValue();
        
        num=dynamic_cast<TPLNumber *>
        (dic->objectForKey(FilterResonanceKey));
        if(num)
            resonance=num->doubleValue();
        
    }
}

#define Oscilator1Key "Oscilator1"
#define Oscilator2Key "Oscilator2"
#define Oscilator3Key "Oscilator3"
#define FilterKey "Filter"
#define PolyphonicsKey "Polyphonics"
#define MaxSpreadKey "MaxSpread"
#define PolyphonicsModeKey "PolyphonicsMode"
#define PolyphonicsModePolyName "Poly"
#define PolyphonicsModePolyUnisonName "Poly Unison"
#define PolyphonicsModeMonoName "Mono"


TPLString *TXGPDS1::Parameter::polyphonicsModeName() const{
    switch(polyphonicsMode){
        default:
        case PolyphonicsModePoly:
            return new TPLString(PolyphonicsModePolyName);
        case PolyphonicsModePolyUnison:
            return new TPLString(PolyphonicsModePolyUnisonName);
        case PolyphonicsModeMono:
            return new TPLString(PolyphonicsModeMonoName);
    }
}

void TXGPDS1::Parameter::setPolyphonicsModeName(const TPLString *name){
    if(name->isEqualToUTF8String(PolyphonicsModePolyName))
        polyphonicsMode=PolyphonicsModePoly;
    else if(name->isEqualToUTF8String(PolyphonicsModePolyUnisonName))
        polyphonicsMode=PolyphonicsModePolyUnison;
    else if(name->isEqualToUTF8String(PolyphonicsModeMonoName))
        polyphonicsMode=PolyphonicsModeMono;
    
}


TPLObject *TXGPDS1::Parameter::serialize() const{
    TPLDictionary *dic=new TPLDictionary();
    {
        TPLAutoReleasePtr<TPLObject> obj(osc1.serialize());
        dic->setObject(&*obj, Oscilator1Key);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(osc2.serialize());
        dic->setObject(&*obj, Oscilator2Key);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(osc3.serialize());
        dic->setObject(&*obj, Oscilator3Key);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(flt.serialize());
        dic->setObject(&*obj, FilterKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(polyphonics));
        dic->setObject(&*obj, PolyphonicsKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(new TPLNumber(maxSpread));
        dic->setObject(&*obj, MaxSpreadKey);
    }
    {
        TPLAutoReleasePtr<TPLObject> obj(polyphonicsModeName());
        dic->setObject(&*obj, PolyphonicsModeKey);
    }
    return dic;
}

void TXGPDS1::Parameter::deserialize(const TPLObject *obj){
    const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
    if(dic){
        osc1.deserialize(dic->objectForKey(Oscilator1Key));
        osc2.deserialize(dic->objectForKey(Oscilator2Key));
        osc3.deserialize(dic->objectForKey(Oscilator3Key));
        flt.deserialize(dic->objectForKey(FilterKey));
        
        {
            TPLNumber *num=dynamic_cast<TPLNumber *>(dic->objectForKey(PolyphonicsKey));
            if(num){
                polyphonics=num->intValue();
            }
        }
        
        {
            TPLNumber *num=dynamic_cast<TPLNumber *>(dic->objectForKey(MaxSpreadKey));
            if(num){
                maxSpread=num->doubleValue();
            }
        }
        
        {
            TPLString *v=dynamic_cast<TPLString *>(dic->objectForKey(PolyphonicsModeKey));
            if(v){
                setPolyphonicsModeName(v);
            }
        }
        
        
    }
}

TPLObject *TXGPDS1::serialize() const{
    return m_parameter.serialize();
    
}

void TXGPDS1::deserialize(const TPLObject *obj){
    TXGPDS1::Parameter param;
    param.deserialize(obj);
    setParameter(param);
}


