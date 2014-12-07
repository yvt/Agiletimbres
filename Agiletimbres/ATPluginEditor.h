//
//  ATPluginEditor.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

#include "TXPlugin.h"

class ATPluginEditor: public twWnd{
public:
    ATPluginEditor(){}
    virtual ~ATPluginEditor(){}
    
    virtual void clientPaint(const twPaintStruct& p);
    
    virtual void setSelectedPlugin(TXPlugin *){}
    
    static ATPluginEditor *editorForPluginIdentifier(const std::string&);
};
