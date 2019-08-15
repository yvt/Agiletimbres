//
//  TXGPDS1.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include <list>
#include "TXInstrument.h"

class TPLString;

class TXGPDS1: public TXInstrument{
    
    
    
public:
    
    struct OscilatorState;
    struct FilterState;
    struct Channel;
    
    enum WaveformType{
        WaveformTypeSine=0,
        WaveformTypeSquare,
        WaveformTypePulse4,
        WaveformTypePulse8,
        WaveformTypePulse16,
        WaveformTypeSawtooth,
        WaveformTypeNoise
    };
    
    enum LfoWaveformType{
        LfoWaveformTypeSine=0,
        LfoWaveformTypeSquare,
        LfoWaveformTypeSawtoothUp,
        LfoWaveformTypeSawtoothDown,
        LfoWaveformTypeTriangle
    };
    
    struct Oscilator{
        WaveformType waveformType;
        int syncTarget;
        int transposeCoarse;// semitones
        int transposeFine;  // cents
        bool fixStart;
        float volume;
        
        float attackTime;
        int attackPower;
        float decayTime;
        int decayPower;
        float sustainLevel;
        float releaseTime;
        int releasePower;
        
        LfoWaveformType lfoWaveformType;
        float lfoDepth;
        float lfoFreq;
        float lfoDelay;
        bool lfoPeriodBase;
        
        Oscilator(){
            waveformType=WaveformTypeSquare;
            syncTarget=0;
            transposeCoarse=0;
            transposeFine=0;
            fixStart=false;
            volume=.33f;
            
            attackTime=0.f;
            attackPower=1;
            decayTime=1.f;
            decayPower=6;
            sustainLevel=0.5f;
            releaseTime=1.f;
            releasePower=6;
            
            lfoWaveformType=LfoWaveformTypeSine;
            lfoDepth=0.f;
            lfoFreq=5.f;
            lfoDelay=.2f;
            lfoPeriodBase=false;
        }
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
        
        TPLString *waveformTypeName() const;
        void setWaveformTypeName(const TPLString *);
        
        TPLString *lfoWaveformTypeName() const;
        void setLfoWaveformTypeName(const TPLString *);
    };
    
    enum FilterType{
        FilterTypeLpf6LC,
        FilterTypeLpf12LC,
		FilterTypeLpf12SID,
        FilterTypeLpf24Moog,
        FilterTypeHpf6LC,
        FilterTypeHpf12LC,
		FilterTypeHpf12SID,
        FilterTypeHpf24Moog,
		FilterTypeBpf6SID,
		FilterTypeNotch6SID
    };
    
    struct Filter{
        bool enable;
        
        FilterType filterType;
        
        
        float resonance;
        
        float initialFreq;
        float attackTime;
        float attackFreq;
        float decayTime;
        float sustainFreq;
        float releaseFreq;
        float releaseTime;
        
        
        Filter(){
            enable=true;
            filterType=FilterTypeLpf24Moog;
            resonance=0.8f;
            initialFreq=20000.f;
            attackTime=.8f;
            attackFreq=10.f;
            decayTime=10.f;
            sustainFreq=80.f;
            releaseFreq=80.f;
            releaseTime=5.f;
        }
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
        
        TPLString *filterTypeName() const;
        void setFilterTypeName(const TPLString *);
    };
    
    enum PolyphonicsMode{
        PolyphonicsModePoly=0,
        PolyphonicsModePolyUnison,
        PolyphonicsModeMono
    };
    
    struct Parameter{
        Oscilator osc1;
        Oscilator osc2;
        Oscilator osc3;
        Filter flt;
        int polyphonics;
        PolyphonicsMode polyphonicsMode;
        float maxSpread;
        
        Parameter(){
            polyphonics=8;
            polyphonicsMode=PolyphonicsModePoly;
            maxSpread=0.f;
        }
        
        TPLObject *serialize() const;
        void deserialize(const TPLObject *);
        
        TPLString *polyphonicsModeName() const;
        void setPolyphonicsModeName(const TPLString *);
    };
    
    TXGPDS1(const TXConfig&config);
    virtual ~TXGPDS1();
    
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    static TXFactory *sharedFactory();
    
    const Parameter &parameter() const{
        return m_parameter;
    }
    
    void setParameter(const Parameter&);
    
    virtual TPLObject *serialize() const;
    virtual void deserialize(const TPLObject *);
    
   
    
protected:
    
    virtual void noteOn(int key, int velocity);
    
    virtual void noteOff(int key, int velocity);
    
    virtual void setPitchbend(int millicents);
    
    virtual void allNotesOff();
    virtual void allSoundsOff();
    
    virtual void renderFragmentAdditive(int32_t *stereoOutputBuffer,
                                        unsigned int samples);
private:
    
    
    typedef std::list<Channel> ChannelList;
    
    ChannelList m_channels;
    float m_sampleFreq;
    float m_basePeriodScale;
    uint32_t m_pitchbendScale;
    
    Parameter m_parameter;
    
    void renderChannelAdditive(int32_t *outBuffer,
                               unsigned int samples,
                               Channel& ch);
    
    void purgeOldestChannel();
    
     int maxPolyphonics();
    
};
