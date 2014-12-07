//
//  TXBiquadFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"

class TXBiquadFilter{
    int m_coefs[5];
    union{
        struct{
            int m_histories[4];
        };
        struct{
            int m_leftHistories[4];
            int m_rightHistories[4];
        };
    };
    int m_shift;
    
    void applyStereoLeftInplace(int32_t *, unsigned int samples);
    void applyStereoLeftOutplace(int32_t *, const int32_t *,
                             unsigned int samples);
    void applyStereoLeftAdditive(int32_t *, const int32_t *,
                             unsigned int samples);
    
    void applyStereoRightInplace(int32_t *, unsigned int samples);
    void applyStereoRightOutplace(int32_t *, const int32_t *,
                                 unsigned int samples);
    void applyStereoRightAdditive(int32_t *, const int32_t *,
                                 unsigned int samples);
    
public:
    TXBiquadFilter();
    TXBiquadFilter(int b0, int b1, int b2,
                   int a1, int a2, int shift);
    ~TXBiquadFilter();
    const int *coefficents() const{return m_coefs;}
    int shift() const{return m_shift;}
    
    void applyMonoInplace(int32_t *, unsigned int samples);
    void applyMonoOutplace(int32_t *, const int32_t *,
                           unsigned int samples);
    void applyMonoAdditive(int32_t *, const int32_t *,
                           unsigned int samples);
    
    void applyStereoInplace(int32_t *, unsigned int samples);
    void applyStereoOutplace(int32_t *, const int32_t *,
                             unsigned int samples);
    void applyStereoAdditive(int32_t *, const int32_t *,
                             unsigned int samples);
    
    void mute();
    
    static TXBiquadFilter biShelfFilter(float sampleRate,
                                         float cutoffLow,
                                         float cutoffHigh,
                                         float gainLow,
                                         float gainHigh,
                                         float masterGain);
    static TXBiquadFilter lowShelfFilter(float sampleRate,
                                           float cutoffFreq,
                                           float Q,
                                           float gain);
    static TXBiquadFilter highShelfFilter(float sampleRate,
                                           float cutoffFreq,
                                           float Q,
                                           float gain);
    static TXBiquadFilter peakFilter(float sampleRate,
                                      float cutoffFreq,
                                      float Q,
                                      float gain);
    static TXBiquadFilter allpassFilter(float sampleRate,
                                         float baseFreq,
                                         float Q);
    
};
