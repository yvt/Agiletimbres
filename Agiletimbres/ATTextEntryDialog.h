//
//  ATTextEntryDialog.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/20/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATTextField;

class ATTextEntryDialog: public twDialog{
    ATTextField *m_textField;
    twButton m_acceptButton;
    twButton m_cancelButton;
    twLabel m_label;
public:
    ATTextEntryDialog();
    virtual ~ATTextEntryDialog();
    
    void setText(const std::wstring&);
    std::wstring text() const;
    
    void setLabel(const std::wstring&);
    
    virtual void command(int);
};
