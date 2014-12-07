//
//  ATSpectralGateFilterEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/22/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXSpectralGateFilter.h"
#include "ATValueEditor.h"

class ATTabView;

class ATSpectralGateFilterEditor: public ATPluginEditor{
    
    TXSpectralGateFilter *m_plugin;
    TXSpectralGateFilter::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_threshold;
    ATValueEditor m_superEnergy;
    ATValueEditor m_subEnergy;
	ATValueEditor m_speed;
    ATValueEditor m_centerFreq;
	ATValueEditor m_bandwidth;
    
    twRect rectForFreqGraph() const;
    
public:
    ATSpectralGateFilterEditor();
    virtual ~ATSpectralGateFilterEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};
