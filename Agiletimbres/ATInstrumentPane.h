//
//  ATInstrumentPane.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATTabView;
class ATPluginEditor;
class ATListView;
class TXFactory;

class ATInstrumentPane: public twWnd{
    
    struct PluginItem;
    struct PresetItem;
    
    typedef std::vector<PluginItem> PluginList;
    typedef std::vector<PresetItem> PresetList;
    
    PluginList m_plugins;
    PresetList m_presets;
    
    TXFactory *m_activePlugin;
    ATPluginEditor *m_editor;
    ATTabView *m_sideTab;
    ATListView *m_pluginsList;
    ATListView *m_presetsList;
    twButton *m_addButton;
	
	std::wstring m_workerError;
    
    twRect rectForEditor() const;
    twRect rectForTab() const;
    
    void createEditor();
    void destroyEditor();
    void buildPluginsList();
    void buildPresetsList();
    
    PresetItem presetFolderItemWithTitle(const std::wstring&) const;
    PresetItem presetItemForPath(const std::wstring&) const;
    bool isPresetPathValid(const std::wstring&) const;
    
    void selectActivePlugin();
    
    TXPlugin *activePlugin() const;
    void setActivePlugin(TXPlugin *);
    
    void selectPresetWithPath(const std::wstring&);
	
	void loadPluginWorker();
    
public:
    ATInstrumentPane();
    virtual ~ATInstrumentPane();
    
	virtual void setRect(const twRect&);
	virtual void command(int);
    
};
