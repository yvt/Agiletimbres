//
//  TXImpulseInstrument.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 12/24/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXInstrument.h"

#include <list>

class TXImpulseInstrument: public TXInstrument{
    
    bool m_impulsed;
    
public:
    
    TXImpulseInstrument(const TXConfig&config);
    virtual ~TXImpulseInstrument();
    
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    static TXFactory *sharedFactory();
    
protected:
    
    virtual void noteOn(int key, int velocity);
    
    virtual void noteOff(int key, int velocity);
    
    virtual void setPitchbend(int millicents);
    
    virtual void allNotesOff();
    virtual void allSoundsOff();
    
    virtual void renderFragmentAdditive(int32_t *stereoOutputBuffer,
                                        unsigned int samples);
    
};
