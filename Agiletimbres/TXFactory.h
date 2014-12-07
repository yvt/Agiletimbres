//
//  TXFactory.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"
#include "TXPlugin.h"
#include <string>

class TXFactory{

public:
    TXFactory(){}
    virtual ~TXFactory(){}
    
    virtual std::string displayName() const=0;
    virtual std::string identifier() const=0;
    virtual TXPluginType pluginType() const=0;
    virtual TXPlugin *createInstance(const TXConfig&) const=0;
    
    
};

template<typename T>
class TXStandardFactory: public TXFactory{
    std::string m_displayName;
    std::string m_identifier;
    TXPluginType m_type;
public:
    TXStandardFactory(const std::string& displayName,
                      const std::string& identifier,
                      TXPluginType type):
    m_displayName(displayName),
    m_identifier(identifier),
    m_type(type){
        
    }
    
    virtual std::string displayName() const{
        return m_displayName;
    }
    virtual std::string identifier() const{
        return m_identifier;
    }
    virtual TXPluginType pluginType() const{
        return m_type;
    }
    virtual TXPlugin *createInstance(const TXConfig& config) const{
        return new T(config);
    }
};
