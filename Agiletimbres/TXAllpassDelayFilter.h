//
//  TXAllpassDelayFilter.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"

class TXAllpassDelayFilter{
    
    unsigned int m_length;
    unsigned int m_position;
    int32_t *m_buffer;
    
public:
    
    TXAllpassDelayFilter();
    TXAllpassDelayFilter(const TXAllpassDelayFilter&);
    TXAllpassDelayFilter(unsigned int length);
    TXAllpassDelayFilter(float sampleRate,
                         float length);
    
    ~TXAllpassDelayFilter();
    
    void setLength(unsigned int length);
    unsigned int length() const{return m_length;}
    
    void applyMonoInplace(int32_t *outBUffer,
                          unsigned int samples);
    
    void applyMonoToSingleStereoAdditive(int32_t *outBuffer,const int32_t *inBuffer,
                          unsigned int samples);
    
    void mute();
    
};
