//
//  TXManager.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"

class TXPlugin;
class TXFactory;



class TXManager{
    
    
protected:
    TXManager();
    ~TXManager();
public:
    
    typedef std::vector<TXFactory *> PluginList;
    
    static TXManager *sharedManager();
    
    const PluginList& allPlugins() const;
    TXFactory *factoryForIdentifier(const std::string&) const;
    
    TXPlugin *pluginWithStdFileStream(FILE *, const TXConfig&) const;
    void serializePluginToStdFileStream(FILE *, TXPlugin *) const;
    
    void registerPlugin(TXFactory *);
};
