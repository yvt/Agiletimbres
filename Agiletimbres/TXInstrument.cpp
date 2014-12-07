//
//  TXInstrument.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXInstrument.h"
#include <string.h>
#include <assert.h>


static const float g_freqCoarseTable[]={
	261.625565300599, // x 1.0594630943593
	277.182630976872,
	293.664767917409,
	311.126983722084,
	329.627556912875,
	349.228231433011,
	369.994422711643,
	391.99543598176,
	415.304697579959,
	440.,
	466.163761518109,
	493.883301256147
};

static const float g_freqOctaveTable[]={
	1./32.,
	1./16.,
	1./8.,
	1./4.,
	1./2.,
	1.,
	2.,
	4.,
	8.,
	16.,
	32.,
	64.,
	128.
};



static const float g_freqScaleTable1[]={
	// 2^ (0.000833333333333) = 2^(1/12/100)
	1.,
	1.00057778950656,
	1.00115591285383,
	1.00173437023471,
	1.00231316184219,
	1.00289228786939,
	1.00347174850953,
	1.00405154395595,
	1.00463167440209,
	1.00521214004152,
	1.0057929410679,
	1.00637407767502,
	1.00695555005678,
	1.00753735840717,
	1.00811950292033,
	1.00870198379047
};

static const float g_freqScaleTable2[]={
	// 2^(0.013333333333333) = 2^(1/12/100*16)
	1.,
	1.00928480121187,
	1.01865580995729,
	1.02811382665607,
	1.03765965915975,
	1.04729412282063,
	1.05701804056138,
	1.06683224294536,
	1.07673756824752,
	1.08673486252606,
	1.09682497969462,
	1.10700878159531,
	1.11728713807222,
	1.1276609270458,
	1.13813103458782,
	1.14869835499703
};

static const float g_freqScaleTable3[]={
	// 2^(0.213333333333333) = 2^(1/12/100*256)
	1.,
	1.15936379087559,
	1.34412439959342,
	1.558329159321,
	1.80667040158236,
	2.09458824564125,
	2.42838976879009,
	2.81538716806797,
	3.26405793995377,
	3.78423058690237,
	4.38729991877849,
	5.0864766655432,
	5.89707686916438,
	6.83685739411917
};

float TXInstrument::frequencyForNote(int note){
    int oct=0;
	while(note>=12){
		oct++;
		note-=12;
	}
	assert(oct>=0);
	assert(oct<12);
	return g_freqCoarseTable[note]*g_freqOctaveTable[oct];
}

float TXInstrument::scaleForCents(int cents){
    if(cents>=256*16){
		cents=256*16-1;
	}
	
	
	if(cents==0)
		return 1.f;
	else if(cents<0)
		return 1.f/scaleForCents(-cents);
	
	float scale;
	if(cents>=1200){
		int oct=cents/1200;
		cents-=oct*1200;
		oct+=5;
		if(oct<13){
			scale=g_freqOctaveTable[oct];
		}else{
			scale=1.f;
		}
	}else{
		scale=1.f;
	}
	
	assert(cents>=0);
	
	scale*=g_freqScaleTable1[(cents>>0)&0xf];
	
	if(cents>=0x10){
		scale*=g_freqScaleTable2[(cents>>4)&0xf];
	}
	
	if(cents>=0x100){
		scale*=g_freqScaleTable3[(cents>>8)&0xf];
	}
	
	return scale;
}

float TXInstrument::scaleForMillicents(int cents){
    if(cents==0)
        return 1.f;
    
    if(cents<0)
        return 1.f/scaleForMillicents(-cents);
    
    // this may make it faster
    int icents=(int)((unsigned int)cents/1000);
    int fcents=cents-icents*1000;
    
    if(fcents==0)
        return scaleForCents(icents);
    
    float scale1, scale2;
    scale1=scaleForCents(icents);
    scale2=scaleForCents(icents+1);
    
    scale1+=(scale2-scale1)*(float)fcents/1000.f;
    
    return scale1;
}

TXInstrument::TXInstrument(){
    m_currentTime=0;
    m_lastCurrentTime=0;
}

TXInstrument::Event TXInstrument::eventForNoteOn(int key, int velocity){
    Event ev;
    ev.instrument=this;
    ev.type=EventTypeNoteOn;
    ev.noteInfo.key=(short)key;
    ev.noteInfo.velocity=(short)velocity;
    return ev;
}

TXInstrument::Event TXInstrument::eventForNoteOff(int key, int velocity){
    Event ev;
    ev.instrument=this;
    ev.type=EventTypeNoteOff;
    ev.noteInfo.key=(short)key;
    ev.noteInfo.velocity=(short)velocity;
    return ev;
}


TXInstrument::Event TXInstrument::eventForPitchbend(int millicents){
    Event ev;
    ev.instrument=this;
    ev.type=EventTypePitchbend;
    ev.pitchbendInfo.millicents=millicents;
    return ev;
}

TXInstrument::Event TXInstrument::eventForAllNotesOff(){
    Event ev;
    ev.instrument=this;
    ev.type=EventTypeAllNotesOff;
    return ev;
}

TXInstrument::Event TXInstrument::eventForAllSoundsOff(){
    Event ev;
    ev.instrument=this;
    ev.type=EventTypeAllSoundsOff;
    return ev;
}

bool TXInstrument::Event::operator<(const TXInstrument::Event &ev) const{
    unsigned int currentTime=instrument->m_lastCurrentTime;
    unsigned int time1=time-currentTime;
    unsigned int time2=ev.time-currentTime;
    if(time1==time2){
        if(type==EventTypeNoteOn && ev.type==EventTypeNoteOff){
            time2++;
        }
        if(type==EventTypeNoteOff && ev.type==EventTypeNoteOn){
            time1++;
        }
        
    }
    return time1>time2; // inversed
}

void TXInstrument::noteOn(int key, int velocity,
                          unsigned int delayInSamples){
    Event event=eventForNoteOn(key, velocity);
    event.time=m_lastCurrentTime+delayInSamples;
    m_pendingEvents.push(event);
   // printf("add event: note on(%d, %d)\n", key, velocity);
}

void TXInstrument::noteOff(int key, int velocity,
                          unsigned int delayInSamples){
    Event event=eventForNoteOff(key, velocity);
    event.time=m_lastCurrentTime+delayInSamples;
    m_pendingEvents.push(event);
}

void TXInstrument::setPitchbend(int millicents,
                          unsigned int delayInSamples){
    Event event=eventForPitchbend(millicents);
    event.time=m_lastCurrentTime+delayInSamples;
    m_pendingEvents.push(event);
}

void TXInstrument::allNotesOff(unsigned int delayInSamples){
    Event event=eventForAllNotesOff();
    event.time=m_lastCurrentTime+delayInSamples;
    m_pendingEvents.push(event);
}

void TXInstrument::allSoundsOff(unsigned int delayInSamples){
    Event event=eventForAllSoundsOff();
    event.time=m_lastCurrentTime+delayInSamples;
    m_pendingEvents.push(event);
}


void TXInstrument::processEvent(const TXInstrument::Event & ev){
    switch (ev.type) {
        case EventTypeNoteOn:
            // printf("process event: note on(%d, %d)\n", ev.noteInfo.key, ev.noteInfo.velocity);
            noteOn(ev.noteInfo.key,
                   ev.noteInfo.velocity);
            break;
        case EventTypeNoteOff:
            noteOff(ev.noteInfo.key,
                   ev.noteInfo.velocity);
            break;
        case EventTypePitchbend:
            setPitchbend(ev.pitchbendInfo.millicents);
            break;
        case EventTypeAllNotesOff:
            allNotesOff();
            break;
        case EventTypeAllSoundsOff:
            allSoundsOff();
            break;
    }
}

void TXInstrument::preprocess(unsigned int samples){
    m_processedEvents.clear();
    while(!m_pendingEvents.empty()){
        const Event& event=m_pendingEvents.top();
        unsigned int time=event.time-m_currentTime;
        assert((int)time>=0);
        if((int)time<0){
            processEvent(event);
            m_pendingEvents.pop();
            continue;
        }
        if(time>=samples)
            break;
       
        m_processedEvents.push_back(event);
        m_pendingEvents.pop();
    }
    
    m_lastCurrentTime=m_currentTime+samples;
}

void TXInstrument::renderAdditive(int32_t *stereoOutputBuffer, unsigned int samples){
    size_t pos=0, nextPos;
    while(samples){
        nextPos=pos;
        while(nextPos<m_processedEvents.size()){
            if(m_processedEvents[nextPos].time!=m_currentTime)
                break;
            nextPos++;
        }
        
        unsigned int nextDuration=samples;
        if(nextPos<m_processedEvents.size()){
            const Event& event=m_processedEvents[nextPos];
            unsigned int dur=event.time-m_currentTime;
            if(dur<nextDuration)
                nextDuration=dur;
        }
        
        while(pos<nextPos){
            processEvent(m_processedEvents[pos++]);
        }
        
        renderFragmentAdditive(stereoOutputBuffer,
                               nextDuration);
        
        samples-=nextDuration;
        stereoOutputBuffer+=nextDuration*2;
        m_currentTime+=nextDuration;
        
    }
    
    assert(m_currentTime=m_lastCurrentTime);
    assert(pos>=m_processedEvents.size());
}






