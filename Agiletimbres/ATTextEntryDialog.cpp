//
//  ATTextEntryDialog.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/20/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATTextEntryDialog.h"
#include "ATTextField.h"

enum{
    IdAccept=1,
    IdCancel
};

ATTextEntryDialog::ATTextEntryDialog(){
    twSize sz(240, 48);
    setRect(twRect(twPoint(0,0),
                   sz+getBorderSize()));
    
    m_textField=new ATTextField();
    m_textField->setParent(this);
    m_textField->setRect(twRect(2, 18, 236, 13));
    m_textField->show();
    
    m_acceptButton.setParent(this);
    m_acceptButton.setRect(twRect(136, 33, 50, 13));
    m_acceptButton.setTitle(L"OK");
    m_acceptButton.setId(IdAccept);
    m_acceptButton.show();
    
    m_cancelButton.setParent(this);
    m_cancelButton.setRect(twRect(188, 33, 50, 13));
    m_cancelButton.setTitle(L"Cancel");
    m_cancelButton.setId(IdCancel);
    m_cancelButton.show();
    
    m_label.setParent(this);
    m_label.setRect(twRect(2, 2, 136, 13));
    m_label.show();
    
}

ATTextEntryDialog::~ATTextEntryDialog(){
    delete m_textField;
}

void ATTextEntryDialog::setText(const std::wstring& text){
    m_textField->setTitle(text);
}

std::wstring ATTextEntryDialog::text() const{
    return m_textField->getTitle();
}

void ATTextEntryDialog::setLabel(const std::wstring &l){
    m_label.setTitle(l);
}

void ATTextEntryDialog::command(int id){
    if(id==IdAccept){
        endDialog(twDR_ok);
    }else if(id==IdCancel){
        endDialog(twDR_cancel);
    }
}
