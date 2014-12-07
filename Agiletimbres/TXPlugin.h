//
//  TXPlugin.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"

class TPLObject;
class TXFactory;

enum TXPluginType{
    TXPluginTypeInstrument=0,
    TXPluginTypeEffect
};

class TXPlugin{
public:
    TXPlugin(){}
    virtual ~TXPlugin(){}
    
    virtual TXPluginType pluginType() const=0;
    
    virtual TPLObject *serialize() const{return NULL;}
    virtual void deserialize(const TPLObject *){}
    
    virtual TXFactory *factory() const=0;
};
