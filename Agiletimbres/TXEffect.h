//
//  TXEffect.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXPlugin.h"

class TXEffect: public TXPlugin{
public:
    TXEffect();
    virtual ~TXEffect(){}
    
    virtual TXPluginType pluginType() const{
        return TXPluginTypeEffect;
    }
    
    virtual void applyStereo(int32_t *outBuffer,
                             const int32_t *inBuffer,
                             unsigned int samples)=0;
};
