//
//  ATChorusFilterEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//


#pragma once

#include "ATPluginEditor.h"
#include "TXChorusFilter.h"
#include "ATValueEditor.h"

class ATTabView;

class ATChorusFilterEditor: public ATPluginEditor{
    
    TXChorusFilter *m_plugin;
    TXChorusFilter::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_minDelayTime;
    ATValueEditor m_spread;
    ATValueEditor m_frequency;
    ATValueEditor m_gain;
    
     twRect rectForDeltaGraph() const;
    
public:
    ATChorusFilterEditor();
    virtual ~ATChorusFilterEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};

