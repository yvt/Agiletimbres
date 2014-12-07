//
//  TXInstrument.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXPlugin.h"
#include <stdint.h>
#include <vector>
#include <queue>

class TXInstrument: public TXPlugin{
public:
    
    TXInstrument();
    virtual ~TXInstrument(){}
    
    virtual TXPluginType pluginType() const{
        return TXPluginTypeInstrument;
    }
    
protected:
    
    virtual void noteOn(int key, int velocity)=0;
    
    virtual void noteOff(int key, int velocity)=0;
    
    virtual void setPitchbend(int millicents)=0;
    
    virtual void renderFragmentAdditive(int32_t *stereoOutputBuffer,
                                       unsigned int samples)=0;
    
    virtual void allNotesOff()=0;
    virtual void allSoundsOff()=0;
    
    
private:
    
    enum EventType{
        EventTypeNoteOn=0,
        EventTypeNoteOff,
        EventTypePitchbend,
        EventTypeAllNotesOff,
        EventTypeAllSoundsOff
    };
    
    struct Event{
        TXInstrument *instrument;
        EventType type;
        unsigned int time;
        union{
            struct{
                short key;
                short velocity;
            } noteInfo;
            struct{
                int millicents;
            } pitchbendInfo;
        };
        bool operator <(const Event& ev) const;
    };
    
    friend struct Event;
    
    std::priority_queue<Event> m_pendingEvents;
    std::vector<Event> m_processedEvents;
    unsigned int m_currentTime;
    unsigned int m_lastCurrentTime;
    
    Event eventForNoteOn(int key, int velocity);
    Event eventForNoteOff(int key, int velocity);
    Event eventForPitchbend(int millicents);
    Event eventForAllNotesOff();
    Event eventForAllSoundsOff();
    
    void processEvent(const Event&);
    
public:
    
    void noteOn(int key, int velocity,
                unsigned int delayInSamples);
    void noteOff(int key, int velocity,
                 unsigned int delayInSamples);
    void setPitchbend(int millicents,
                      unsigned int delayInSamples);
    void allNotesOff(unsigned int delayInSamples);
    void allSoundsOff(unsigned int delayInSamples);
    
    void preprocess(unsigned int samples);
    void renderAdditive(int32_t *stereoOutputBuffer,
                        unsigned int samples);
    
    /*
     *  Render Manager Usage (Renderer Thread):
     *
     *   1. Lock the semaphore
     *   2. preprocess(samples)
     *   3. Unlock the semaphore
     *   4. Update delay timer
     *   5. renderAddtive(buffer, samples)
     *
     */
    
    
    static float frequencyForNote(int);
    static float scaleForCents(int);
    static float scaleForMillicents(int);
    
};
