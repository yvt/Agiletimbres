//
//  TXAllpassDelayFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXAllpassDelayFilter.h"

TXAllpassDelayFilter::TXAllpassDelayFilter(){
    m_length=0;
    m_buffer=NULL;
    m_position=0;
     mute();
}

TXAllpassDelayFilter::TXAllpassDelayFilter(const TXAllpassDelayFilter& f){
    m_length=f.length();
    if(f.length()){
        m_buffer=new int32_t[m_length];
    }else{
        m_buffer=NULL;
    }
    m_position=0;
    mute();
}

TXAllpassDelayFilter::TXAllpassDelayFilter
(unsigned int length){
    m_length=length;
    if(m_length)
        m_buffer=new int32_t[m_length];
    else
        m_buffer=NULL;
    m_position=0;
     mute();
}

TXAllpassDelayFilter::TXAllpassDelayFilter
(float sampleRate, float length){
    if(length<0.f)
        length=0.f;
    m_length=(unsigned int)(length*sampleRate);
    if(m_length)
        m_buffer=new int32_t[m_length];
    else
        m_buffer=NULL;
    m_position=0;
    mute();
}

TXAllpassDelayFilter::~TXAllpassDelayFilter(){
    if(m_buffer){
        delete[] m_buffer;
        m_buffer=NULL;
    }
}

void TXAllpassDelayFilter::mute(){
    if(m_buffer)
        memset(m_buffer, 0, m_length*sizeof(int32_t));
}

void TXAllpassDelayFilter::setLength(unsigned int length){
    if(m_length==length)
        return;
    
    if(m_buffer){
        delete[] m_buffer;
        m_buffer=NULL;
    }
    
    m_length=length;
    if(m_length){
        m_buffer=new int32_t[m_length];
        m_position=0;
    }
}

void TXAllpassDelayFilter::applyMonoInplace
(int32_t *outBuffer, unsigned int samples){
    
    if(!m_buffer)
        return;
    
    const unsigned int length=m_length;
    unsigned int position=m_position;
    int32_t *buffer=m_buffer;
    
    assert(position<length);
    
    while(samples--){
        assert(position<length);
        
        int input=*outBuffer;
        int bufValue=buffer[position];
        
        input+=bufValue>>1; // multiply by "gain"
        
        int output=bufValue;
        output-=input>>1; // multiply by "gain"
        
        buffer[position]=input;
        
        *(outBuffer++)=output;
        position++;
        
        if(position==length)
            position=0;
    }
    
    m_position=position;
}


void TXAllpassDelayFilter::applyMonoToSingleStereoAdditive
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    
    if(!m_buffer)
        return;
    
    const unsigned int length=m_length;
    unsigned int position=m_position;
    int32_t *buffer=m_buffer;
    
    assert(position<length);
    
    while(samples--){
        assert(position<length);
        
        int input=*(inBuffer++);
        int bufValue=buffer[position];
        
        input+=bufValue>>1; // multiply by "gain"
        
        int output=bufValue;
        output-=input>>1; // multiply by "gain"
        
        buffer[position]=input;
        
        *(outBuffer++)+=output;
        outBuffer++;
        position++;
        
        if(position==length)
            position=0;
    }
    
    m_position=position;
}
