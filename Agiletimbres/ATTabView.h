//
//  ATTabView.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/14/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATTabView: public twWnd{
    std::vector<twWnd *> m_tabs;
    int m_drag;
    bool m_hover;
    int m_selectedTabIndex;
    int m_rightMargin;
    
    twRect rectForTab() const;
    twRect rectForTabBar() const;
    twRect rectForTabInTabBar(size_t) const;
public:
    ATTabView();
    virtual ~ATTabView();
    
    void setTabCount(size_t);
    size_t tabCount() const;
    
    twWnd *tabAtIndex(size_t) const;
    void setTabAtIndex(size_t,
                       twWnd *);
    
    void setRightMargin(int);
    int rightMargin() const{return m_rightMargin;}
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
    
	virtual void setRect(const twRect&);
    
    int selectedTabIndex() const{return m_selectedTabIndex;}
    void setSelectedTabIndex(int);
};
