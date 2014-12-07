//
//  ATGPDS1Editor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "ATPluginEditor.h"
#include "TXGPDS1.h"

class ATTabView;

class ATGPDS1Editor: public ATPluginEditor{
    class OscilatorEditor;
    class FilterEditor;
    class RoutingEditor;
    
    ATTabView *m_tabView;
    
    OscilatorEditor *m_oscilatorEditors[3];
    FilterEditor *m_filterEditor;
    RoutingEditor *m_routingEditor;
    
    TXGPDS1 *m_plugin;
    TXGPDS1::Parameter m_parameter;
    
    void loadParameters(); // load from m_parameter
    void setParameters();  // set m_parameter to m_plugin
    
    twRect rectForTabView() const;
public:
    ATGPDS1Editor();
    virtual ~ATGPDS1Editor();
    
    virtual void setSelectedPlugin(TXPlugin *);
    virtual void clientPaint(const twPaintStruct& p);
    virtual void command(int);
};
