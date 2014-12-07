//
//  TXManager.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXManager.h"
#include "TXFactory.h"
#include "TXPlugin.h"
#include "TPList/TPLException.h"
#include "TPList/TPLPropertyListSerialization.h"
#include "TPList/TPLString.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLAutoReleasePtr.h"

#include "TXGPDS1.h"
#include "TXMatrixReverb1.h"
#include "TXChorusFilter.h"
#include "TXTestInstrument.h"
#include "TXImpulseInstrument.h"
#include "TXLegacyMidi.h"
#include "TXOverdriveFilter.h"
#include "TXPhaserFilter.h"
#include "TXDecimatorFilter.h"
#include "TXSpectralGateFilter.h"

static TXManager *g_sharedManager=NULL;

TXManager *TXManager::sharedManager(){
    if(!g_sharedManager)
        g_sharedManager=new TXManager();
    return g_sharedManager;
}

TXManager::TXManager(){
    registerPlugin(TXTestInstrument::sharedFactory());
    registerPlugin(TXImpulseInstrument::sharedFactory());
    registerPlugin(TXGPDS1::sharedFactory());
    registerPlugin(TXMatrixReverb1::sharedFactory());
    registerPlugin(TXChorusFilter::sharedFactory());
	registerPlugin(TXLegacyMidi::sharedFactory());
	registerPlugin(TXOverdriveFilter::sharedFactory());
	registerPlugin(TXPhaserFilter::sharedFactory());
	registerPlugin(TXDecimatorFilter::sharedFactory());
	registerPlugin(TXSpectralGateFilter::sharedFactory());
}
TXManager::~TXManager(){}

#pragma mark - Plugins List

static TXManager::PluginList g_allPlguins;

const TXManager::PluginList& TXManager::allPlugins() const{
    return g_allPlguins;
}

TXFactory *TXManager::factoryForIdentifier(const std::string &identifier) const{
    const PluginList& plugins=allPlugins();
    PluginList::const_iterator it;
    for(it=plugins.begin();it!=plugins.end();it++){
        TXFactory *factory=*it;
        if(factory->identifier()==identifier)
            return factory;
    }
    return NULL;
}

void TXManager::registerPlugin(TXFactory *factory){
    if(factoryForIdentifier(factory->identifier())){
        // already exists
        return;
    }
    g_allPlguins.push_back(factory);
}

#pragma mark - Serialization

#define TXSettingsPlguinIdentifierKey "TXSettingsPlguinIdentifier"
#define TXSettingsPluginSettingsKey "TXSettingsPluginSettings"

TXPlugin *TXManager::pluginWithStdFileStream(FILE *stream,
                                             const TXConfig &config) const{
    TPLAutoReleasePtr<TPLObject> obj
    (TPLPropertyListSerialization::propertyListWithStream(stream));
    if(!dynamic_cast<TPLDictionary *>(&(*obj))){
        throw TPLException("Root element of property list for TX plugin must be a dictionary.");
    }
    
    TPLDictionary *dic=dynamic_cast<TPLDictionary *>(&(*obj));
    
    std::string identifier;
    
    {
        TPLString *value=dynamic_cast<TPLString *>
        (dic->objectForKey(TXSettingsPlguinIdentifierKey));
        if(!value){
             throw TPLException("Root element of property list for TX plugin must contain key \""
                                TXSettingsPlguinIdentifierKey
                                "\".");
        }
        identifier=value->UTF8String();
    }
    
    TPLObject *settings;
    settings=dic->objectForKey(TXSettingsPluginSettingsKey);
    
    TXFactory *factory=factoryForIdentifier(identifier);
    if(!factory){
        throw TPLException(("Plugin with identifier \""+identifier
                           +"\" not found.").c_str());
    }
    
    
    TXPlugin *plugin=factory->createInstance(config);
    
    try{
        if(settings)
            plugin->deserialize(settings);
    }catch(...){
        delete plugin;
        throw;
    }
    
    return plugin;
    
}

void TXManager::serializePluginToStdFileStream(FILE *stream,
                                               TXPlugin *plugin) const{
    TPLAutoReleasePtr<TPLDictionary> dic
    (new TPLDictionary());
    
    TXFactory *factory=plugin->factory();
    
    {
        TPLAutoReleasePtr<TPLObject> value
        (new TPLString(factory->identifier().c_str()));
        dic->setObject(value, TXSettingsPlguinIdentifierKey);
    }
    
    {
        TPLObject *obj=plugin->serialize();
        if(obj){
            TPLAutoReleasePtr<TPLObject> value
            (obj);
            dic->setObject(value, TXSettingsPluginSettingsKey);
        }
    }
    
    TPLPropertyListSerialization::writePropertyList(dic, stream);
}



