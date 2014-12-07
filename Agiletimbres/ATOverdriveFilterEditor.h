//
//  ATOverdriveFilterEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXOverdriveFilter.h"
#include "ATValueEditor.h"

class ATTabView;

class ATOverdriveFilterEditor: public ATPluginEditor{
    
    TXOverdriveFilter *m_plugin;
    TXOverdriveFilter::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_mix;
    ATValueEditor m_tone;
    ATValueEditor m_drive;
    ATValueEditor m_gain;
    
    twRect rectForGraph() const;
    
public:
    ATOverdriveFilterEditor();
    virtual ~ATOverdriveFilterEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};