//
//  ATPhaserFilterEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/3/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//



#pragma once

#include "ATPluginEditor.h"
#include "TXPhaserFilter.h"
#include "ATValueEditor.h"
#include "ATDropdownList.h"

class ATDropdownList;
class ATTabView;

class ATPhaserFilterEditor: public ATPluginEditor{
    
    TXPhaserFilter *m_plugin;
    TXPhaserFilter::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_depth;
    ATDropdownList m_stages;
    ATValueEditor m_frequency;
    ATValueEditor m_feedback;
	ATValueEditor m_centerFrequency;
	ATValueEditor m_range;
    
	twRect rectForDeltaGraph() const;
    
public:
    ATPhaserFilterEditor();
    virtual ~ATPhaserFilterEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};

