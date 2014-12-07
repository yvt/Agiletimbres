//
//  ATReverbPane.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//


#include "ATSynthesizer.h"
#include "ATReverbPane.h"
#include "ATTabView.h"
#include "ATPluginEditor.h"
#include "ATListView.h"
#include "TXInstrument.h"
#include "TXFactory.h"
#include "TXManager.h"
#include "Utils.h"
#include "main.h"
#include "ATTextEntryDialog.h"

enum{
    IdPresetsList=1,
    IdAdd,
    IdSideTab,
    IdEnable
};


struct ATReverbPane::PresetItem{
    std::wstring displayName;
    std::wstring path;
    std::wstring lowerPath;
};

ATReverbPane::ATReverbPane(){
    m_editor=NULL;
    m_activePlugin=NULL;
    
    m_addButton=new twButton();
    m_addButton->setTitle(L"Save");
    m_addButton->setId(IdAdd);
    m_addButton->setParent(this);
    m_addButton->show();
    
    m_enableButton=new twCheckBox();
    m_enableButton->setTitle(L"Active");
    m_enableButton->setId(IdEnable);
    m_enableButton->setParent(this);
    m_enableButton->setCheckState(pluginEnable());
    m_enableButton->show();
    
    m_presetsList=new ATListView();
    m_presetsList->setId(IdPresetsList);
    m_presetsList->setRect(rectForList());
    m_presetsList->setParent(this);
    m_presetsList->show();
    
    buildPresetsList();
    
    createEditor();
    
}

ATReverbPane::~ATReverbPane(){

    if(m_editor){
        delete m_editor;
    }
    
    // needn't delete m_pluginsList, m_presetsList,
    // m_addButton.
    // they are in m_sideTab.
}

void ATReverbPane::setRect(const twRect &rt){
    twWnd::setRect(rt);
    
    m_presetsList->setRect(rectForList());
    if(m_editor)
        m_editor->setRect(rectForEditor());
    
    twRect r=rectForList();
    m_addButton->setRect(twRect(r.x+r.w-37, r.y-14,
                                37, 13));
    m_enableButton->setRect(twRect(r.x, r.y-14,
                                r.w-37, 13));
}

twRect ATReverbPane::rectForEditor() const{
    twRect rt=getClientRect();
    return twRect(0, 0, 320, 200);
}

twRect ATReverbPane::rectForList() const{
    twRect rt=getClientRect();
    return twRect(321, 14, rt.w-321, rt.h-14);
}

TXPlugin *ATReverbPane::activePlugin() const{
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    TXPlugin *plugin=synth->reverb();
    assert(dynamic_cast<PluginType *>(plugin));
    return plugin;
}

void ATReverbPane::setActivePlugin(TXPlugin *plugin){
    assert(dynamic_cast<PluginType *>(plugin));
    
    //ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    PluginType *oldPlugin=dynamic_cast<PluginType *>(activePlugin());
    assert(oldPlugin);
    
    PluginType::Parameter param=dynamic_cast<PluginType *>
    (plugin)->parameter();
    
    oldPlugin->setParameter(param);
    
    delete plugin;
    
}

bool ATReverbPane::pluginEnable() const{
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    return synth->reverbEnable();
}

void ATReverbPane::setPluginEnable(bool enb){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    synth->setReverbEnable(enb);
}

void ATReverbPane::createEditor(){
    
    
    TXPlugin *plugin=activePlugin();
    TXFactory *factory=plugin->factory();
    
    if(factory!=m_activePlugin){
        if(m_editor)
            delete m_editor;
        
        m_editor=ATPluginEditor::editorForPluginIdentifier
        (factory->identifier());
    }
    m_activePlugin=factory;
    
    m_editor->setSelectedPlugin(plugin);
    
    m_editor->setParent(this);
    m_editor->setRect(rectForEditor());
    m_editor->show();
    
}

void ATReverbPane::destroyEditor(){
    if(!m_editor)
        return;
    delete m_editor;
    m_editor=NULL;
    m_activePlugin=NULL;
}

void ATReverbPane::command(int index){
    if(index==IdPresetsList){
        if(m_presetsList->selectedIndex()==-1)
            return;
        
        PresetItem& item=m_presets[m_presetsList->selectedIndex()];
        if(item.path.empty())
            return;
        TXManager *manager=TXManager::sharedManager();
        ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
        FILE *f=NHOpenStd(item.path, L"rb");
        if(!f){
            twMsgBox(NULL, L"Cannot open \""+item.path+L"\".",
                     twMBB_ok, L"Error", true);
            return;
        }
        
        TXPlugin *plugin;
        try{
            
            plugin=manager->pluginWithStdFileStream(f, synth->config());
            fclose(f);
        }catch(...){
            fclose(f);
            twMsgBox(NULL, L"Failed to read \""+item.path+L"\".",
                     twMBB_ok, L"Error", true);
            return;
        }
        
        assert(plugin->factory());
        
        //TXPlugin *oldPlugin=activePlugin();
        TXPlugin *newPlugin=plugin;
        
        setActivePlugin(newPlugin);
        
        createEditor();
        
        //delete oldPlugin;
        
    }else if(index==IdAdd){
        ATTextEntryDialog dlg;
        dlg.setNeedsDimming(true);
        dlg.setLabel(L"Type a name for the preset:");
        dlg.setTitle(L"Add Preset");
        if(dlg.showModal(NULL)==twDR_ok){
            std::wstring savePath;
            TXPlugin *plugin=activePlugin();
            TXFactory *factory=plugin->factory();
            savePath=ATUserPluginSettingsDirForPlugin(factory->identifier());
            TXManager *manager=TXManager::sharedManager();
            
            NHMakeDirectory(savePath);
            savePath+=NHPathSeparator;
            savePath+=dlg.text();
            savePath+=L".plist";
            
            FILE *f=NHOpenStd(savePath, L"wb");
            if(!f){
                twMsgBox(NULL, L"Cannot open \""+savePath+L"\".",
                         twMBB_ok, L"Error", true);
                return;
            }
            
            manager->serializePluginToStdFileStream(f, plugin);
            
            fclose(f);
            
            buildPresetsList();
            selectPresetWithPath(savePath);
        }
    }else if(index==IdEnable){
        setPluginEnable(m_enableButton->getCheckState());
    }
}


void ATReverbPane::buildPresetsList(){
    std::vector<std::wstring> paths;
    std::string plugin=activePlugin()->factory()->identifier();
    m_presets.clear();
    
    // user presets.
    paths=NHScanDirectory(ATUserPluginSettingsDirForPlugin(plugin));
    m_presets.push_back(presetFolderItemWithTitle(L"Library"));
    for(size_t i=0;i<paths.size();i++){
        const std::wstring& path=paths[i];
        if(!isPresetPathValid(path))
            continue;
        m_presets.push_back(presetItemForPath(path));
    }
    
    // builtin presets.
    paths=NHScanDirectory(ATBuiltinPluginSettingsDirForPlugin(plugin));
    m_presets.push_back(presetFolderItemWithTitle(L"Built-in"));
    for(size_t i=0;i<paths.size();i++){
        const std::wstring& path=paths[i];
        if(!isPresetPathValid(path))
            continue;
        m_presets.push_back(presetItemForPath(path));
    }
    
    
    
    ATListView::ItemList& items=m_presetsList->items();
    items.resize(m_presets.size());
    
    for(size_t i=0;i<m_presets.size();i++){
        PresetItem& item=m_presets[i];
        items[i]=item.displayName;
    }
    
    m_presetsList->reloadData();
}

void ATReverbPane::selectPresetWithPath(const std::wstring &path){
    std::wstring lowerPath=NHToLower(path);
    for(size_t i=0;i<m_presets.size();i++){
        PresetItem& pi=m_presets[i];
        if(pi.lowerPath==lowerPath){
            m_presetsList->setSelectedIndex(i);
            m_presetsList->scrollToItem(i);
        }
    }
}

ATReverbPane::PresetItem ATReverbPane::presetFolderItemWithTitle(const std::wstring &title) const{
    PresetItem item;
    item.displayName=L"\x01"+title;
    return item;
}

ATReverbPane::PresetItem ATReverbPane::presetItemForPath(const std::wstring &path) const{
    std::wstring fn=NHLastPathComponent(path);
    PresetItem item;
    item.path=path;
    item.lowerPath=NHToLower(path);
    
    size_t pos=fn.find(L'.');
    if(pos==std::wstring::npos)
        pos=fn.size();
    item.displayName=L" "+fn.substr(0, pos);
    return item;
}

bool ATReverbPane::isPresetPathValid(const std::wstring &path) const{
    std::wstring fn=NHLastPathComponent(path);
    if(fn[0]==L'.')
        return false;
    if(fn.size()<6)
        return false;
    if(fn[fn.size()-1]!=L't' && fn[fn.size()-1]!=L'T')
        return false;
    if(fn[fn.size()-2]!=L's' && fn[fn.size()-2]!=L'S')
        return false;
    if(fn[fn.size()-3]!=L'i' && fn[fn.size()-3]!=L'I')
        return false;
    if(fn[fn.size()-4]!=L'l' && fn[fn.size()-4]!=L'L')
        return false;
    if(fn[fn.size()-5]!=L'p' && fn[fn.size()-5]!=L'P')
        return false;
    if(fn[fn.size()-6]!=L'.')
        return false;
    
    return true;
}
