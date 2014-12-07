//
//  ATInstrumentPane.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATSynthesizer.h"
#include "ATInstrumentPane.h"
#include "ATTabView.h"
#include "ATPluginEditor.h"
#include "ATListView.h"
#include "TXInstrument.h"
#include "TXFactory.h"
#include "TXManager.h"
#include "Utils.h"
#include "main.h"
#include "ATTextEntryDialog.h"
#include "XTBCommondlg.h"
#include "TXLegacyMidi.h"

enum{
    IdPluginsList=1,
    IdPresetsList,
    IdAdd,
    IdSideTab
};

struct ATInstrumentPane::PluginItem{
    TXFactory *factory;
};

struct ATInstrumentPane::PresetItem{
    std::wstring displayName;
    std::wstring path;
    std::wstring lowerPath;
};

ATInstrumentPane::ATInstrumentPane(){
    m_editor=NULL;
    m_activePlugin=NULL;
    m_sideTab=new ATTabView();
    m_sideTab->setId(IdSideTab);
    m_sideTab->setTabCount(2);
    m_sideTab->setParent(this);
    m_sideTab->show();
    
    m_sideTab->setRightMargin(38);
    m_addButton=new twButton();
    m_addButton->setTitle(L"Save");
    m_addButton->setId(IdAdd);
    m_addButton->setParent(m_sideTab);
    {
        twWndStyle style=m_addButton->getStyle();
        style.enable=false;
        m_addButton->setStyle(style);
    }
    m_addButton->show();
    
    m_pluginsList=new ATListView();
    m_pluginsList->setTitle(L"Plugins");
    m_pluginsList->setId(IdPluginsList);
    delete m_sideTab->tabAtIndex(0);
    m_sideTab->setTabAtIndex(0, m_pluginsList);
    
    m_presetsList=new ATListView();
    m_presetsList->setTitle(L"Presets");
    m_presetsList->setId(IdPresetsList);
    delete m_sideTab->tabAtIndex(1);
    m_sideTab->setTabAtIndex(1, m_presetsList);
    
    buildPluginsList();
    buildPresetsList();
    
    createEditor();
    
}

ATInstrumentPane::~ATInstrumentPane(){
    
    delete m_sideTab;
    
    if(m_editor){
        delete m_editor;
    }
    
    // needn't delete m_pluginsList, m_presetsList,
    // m_addButton.
    // they are in m_sideTab.
}

void ATInstrumentPane::setRect(const twRect &rt){
    twWnd::setRect(rt);
    
    m_sideTab->setRect(rectForTab());
    if(m_editor)
        m_editor->setRect(rectForEditor());
    
    twRect r=m_sideTab->getClientRect();
    m_addButton->setRect(twRect(r.w-37, 0,
                                37, 13));
}

twRect ATInstrumentPane::rectForEditor() const{
    twRect rt=getClientRect();
    return twRect(0, 0, 320, 200);
}

twRect ATInstrumentPane::rectForTab() const{
    twRect rt=getClientRect();
    return twRect(321, 0, rt.w-321, rt.h);
}

TXPlugin *ATInstrumentPane::activePlugin() const{
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    TXPlugin *plugin=synth->instrument();
    return plugin;
}

void ATInstrumentPane::setActivePlugin(TXPlugin *plugin){
    assert(dynamic_cast<TXInstrument *>(plugin));
    
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    synth->setInstrument(static_cast<TXInstrument *>(plugin));
}

void ATInstrumentPane::createEditor(){
    

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
    
    selectActivePlugin();
}

void ATInstrumentPane::destroyEditor(){
    if(!m_editor)
        return;
    delete m_editor;
    m_editor=NULL;
    m_activePlugin=NULL;
}

static void legacyMidiProgressProc(float prog){
	XTBSetProgressPercentage(prog);
}

void ATInstrumentPane::loadPluginWorker(){
	TXFactory *factory=m_plugins[m_pluginsList->selectedIndex()].factory;
	
	m_workerError.clear();
	
	try{
		
		ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
		TXPlugin *oldPlugin=activePlugin();
		TXPlugin *newPlugin=NULL;
		
		XTBSetProgressText(L"Loading "+twM2W(factory->displayName()));
		
		if(factory->identifier()==TXLegacyMidi::sharedFactory()->identifier()){
			TXLegacyMidi::setProgressProc(legacyMidiProgressProc);
		}
		
		newPlugin=factory->createInstance(synth->config());
        
		setActivePlugin(newPlugin);
		
		createEditor();
		m_presetsList->setSelectedIndex(-1);
		m_presetsList->scrollToItem(0);
		buildPresetsList();
		
		delete oldPlugin;
		
		
	}catch(const std::wstring& err){
		m_workerError=L"Failed to load the plugin:\n\n"+
		err;
		return;
	}catch(const std::string& err){
		m_workerError=L"Failed to load the plugin:\n\n"+
		twM2W(err);
		return;
	}catch(...){
		m_workerError=L"Failed to load the plugin.";
		
		return;
	}
}

void ATInstrumentPane::command(int index){
    if(index==IdPluginsList){
        if(m_pluginsList->selectedIndex()==-1)
            return;
        
		ATSynthesizer::sharedSynthesizer()->setMute(true);
        XTBInvokeWithProgressOverlay(new twNoArgumentMemberFunctionInvocation<ATInstrumentPane>
									 (this, &ATInstrumentPane::loadPluginWorker)); 
		ATSynthesizer::sharedSynthesizer()->setMute(false);
		if(!m_workerError.empty()){
			twMsgBox(NULL, m_workerError,
					 twMBB_ok, L"Error", true);
		}
        
    }else if(index==IdSideTab){
        {
            twWndStyle style=m_addButton->getStyle();
            style.enable=(m_sideTab->selectedTabIndex()==1);
            m_addButton->setStyle(style);
        }
    }else if(index==IdPresetsList){
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
        
        
        TXPlugin *oldPlugin=activePlugin();
        TXPlugin *newPlugin=plugin;
        
        setActivePlugin(newPlugin);
        
        createEditor();
        if(newPlugin->factory()!=oldPlugin->factory()){
            m_presetsList->setSelectedIndex(-1);
            m_presetsList->scrollToItem(0);
            buildPresetsList();
            
        }
        
        delete oldPlugin;
        
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
    }
}

void ATInstrumentPane::buildPluginsList(){
    m_plugins.clear();
    
    const TXManager::PluginList& allPlugins=
    TXManager::sharedManager()->allPlugins();
    
    for(TXManager::PluginList::const_iterator it=
        allPlugins.begin();it!=allPlugins.end();it++){
        TXFactory *factory=*it;
        
        // pick up only instruments
        if(factory->pluginType()!=TXPluginTypeInstrument)
            continue;
        
        PluginItem item;
        item.factory=factory;
        
        m_plugins.push_back(item);
    }
    
    ATListView::ItemList& items=m_pluginsList->items();
    items.resize(m_plugins.size());
    
    for(size_t i=0;i<m_plugins.size();i++){
        PluginItem& item=m_plugins[i];
        TXFactory *factory=item.factory;
        
        std::wstring title;
        title=twM2W(factory->displayName());
        
        items[i]=title;
    }
    
    m_pluginsList->reloadData();
    
}

void ATInstrumentPane::selectActivePlugin(){
    TXPlugin *plugin=activePlugin();
    TXFactory *factory=plugin->factory();
    for(size_t i=0;i<m_plugins.size();i++){
        if(factory==m_plugins[i].factory){
            m_pluginsList->scrollToItem(i);
            m_pluginsList->setSelectedIndex((int)i);
        }
    }
   
}

void ATInstrumentPane::buildPresetsList(){
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

void ATInstrumentPane::selectPresetWithPath(const std::wstring &path){
    std::wstring lowerPath=NHToLower(path);
    for(size_t i=0;i<m_presets.size();i++){
        PresetItem& pi=m_presets[i];
        if(pi.lowerPath==lowerPath){
            m_presetsList->setSelectedIndex(i);
            m_presetsList->scrollToItem(i);
        }
    }
}

ATInstrumentPane::PresetItem ATInstrumentPane::presetFolderItemWithTitle(const std::wstring &title) const{
    PresetItem item;
    item.displayName=L"\x01"+title;
    return item;
}

ATInstrumentPane::PresetItem ATInstrumentPane::presetItemForPath(const std::wstring &path) const{
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

bool ATInstrumentPane::isPresetPathValid(const std::wstring &path) const{
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
