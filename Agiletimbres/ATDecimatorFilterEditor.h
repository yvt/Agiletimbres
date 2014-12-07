//
//  ATDecimatorFilterEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/4/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXDecimatorFilter.h"
#include "ATValueEditor.h"

class ATTabView;

class ATDecimatorFilterEditor: public ATPluginEditor{
    
    TXDecimatorFilter *m_plugin;
    TXDecimatorFilter::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_mix;
    ATValueEditor m_drive;
    ATValueEditor m_gain;
	ATValueEditor m_bits;
	ATValueEditor m_downsample;
    
    twRect rectForGraph() const;
    
public:
    ATDecimatorFilterEditor();
    virtual ~ATDecimatorFilterEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};