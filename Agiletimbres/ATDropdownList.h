//
//  ATDropdownList.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATDropdownList: public twWnd{
public:
    typedef std::vector<std::wstring> ItemList;
private:
    class DropdownView;
    bool m_drag;
    bool m_hover;
    ItemList m_items;
    int m_selectedIndex;
public:
    ATDropdownList();
    virtual ~ATDropdownList();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
    
    ItemList& items(){return m_items;}
    const ItemList& items() const{return m_items;}
    
    virtual void setSelectedIndex(int);
    int selectedIndex() const{return m_selectedIndex;}
    
    virtual void dropdown();
};