//
//  ATLegacyMidiEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXLegacyMidi.h"

class ATTabView;

class ATLegacyMidiEditor: public ATPluginEditor{
    class VoiceEditor;
    class RoutingEditor;
    
    ATTabView *m_tabView;
    
    VoiceEditor *m_voiceEditor;
    RoutingEditor *m_routingEditor;
    
    TXLegacyMidi *m_plugin;
    TXLegacyMidi::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    twRect rectForTabView() const;
public:
    ATLegacyMidiEditor();
    virtual ~ATLegacyMidiEditor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};

