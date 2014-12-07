//
//  ATChorusPane.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATTabView;
class ATPluginEditor;
class ATListView;
class TXFactory;
class TXChorusFilter;

class ATChorusPane: public twWnd{
    
    typedef TXEffect PluginType;
    
	struct PluginItem;
    struct PresetItem;
    
    typedef std::vector<PresetItem> PresetList;
	typedef std::vector<PluginItem> PluginList;
    
    PresetList m_presets;
	PluginList m_plugins;
    
    TXFactory *m_activePlugin;
    ATPluginEditor *m_editor;
	ATTabView *m_sideTab;
	ATListView *m_pluginsList;
    ATListView *m_presetsList;
    twButton *m_addButton;
    twCheckBox *m_enableButton;
	
	std::wstring m_workerError;
    
    twRect rectForEditor() const;
    twRect rectForList() const;
    
    void createEditor();
    void destroyEditor();
	void buildPluginsList();
	void selectActivePlugin();
    void buildPresetsList();
	
	void loadPluginWorker();
    
    PresetItem presetFolderItemWithTitle(const std::wstring&) const;
    PresetItem presetItemForPath(const std::wstring&) const;
    bool isPresetPathValid(const std::wstring&) const;
    
    TXPlugin *activePlugin() const;
    void setActivePlugin(TXPlugin *);
    
    bool pluginEnable() const;
    void setPluginEnable(bool);
    
    void selectPresetWithPath(const std::wstring&);
    
public:
    ATChorusPane();
    virtual ~ATChorusPane();
    
	virtual void setRect(const twRect&);
	virtual void command(int);
    
};