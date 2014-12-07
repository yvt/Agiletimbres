//
//  ATMatrixReverb1Editor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/25/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXMatrixReverb1.h"
#include "ATValueEditor.h"

class ATTabView;

class ATMatrixReverb1Editor: public ATPluginEditor{
    
    TXMatrixReverb1 *m_plugin;
    TXMatrixReverb1::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    ATValueEditor m_reverbTime;
    ATValueEditor m_gain;
    ATValueEditor m_highFreqDamp;
    ATValueEditor m_er;
    
    twRect rectForFreqGraph() const;
    
public:
    ATMatrixReverb1Editor();
    virtual ~ATMatrixReverb1Editor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};
