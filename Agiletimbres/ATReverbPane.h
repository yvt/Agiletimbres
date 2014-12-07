//
//  ATReverbPane.h
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
class TXMatrixReverb1;

class ATReverbPane: public twWnd{
    
    typedef TXMatrixReverb1 PluginType;
    
    struct PresetItem;
    
    typedef std::vector<PresetItem> PresetList;
    
    PresetList m_presets;
    
    TXFactory *m_activePlugin;
    ATPluginEditor *m_editor;
    ATListView *m_presetsList;
    twButton *m_addButton;
    twCheckBox *m_enableButton;
    
    twRect rectForEditor() const;
    twRect rectForList() const;
    
    void createEditor();
    void destroyEditor();
    void buildPresetsList();
    
    PresetItem presetFolderItemWithTitle(const std::wstring&) const;
    PresetItem presetItemForPath(const std::wstring&) const;
    bool isPresetPathValid(const std::wstring&) const;
    
    TXPlugin *activePlugin() const;
    void setActivePlugin(TXPlugin *);
    
    bool pluginEnable() const;
    void setPluginEnable(bool);
    
    void selectPresetWithPath(const std::wstring&);
    
public:
    ATReverbPane();
    virtual ~ATReverbPane();
    
	virtual void setRect(const twRect&);
	virtual void command(int);
    
};
