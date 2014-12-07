//
//  ATListView.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

class ATListView: public twWnd{
public:
    typedef std::vector<std::wstring> ItemList;
private:
    ItemList m_items;
    
    twColor m_tint;
    
    int m_rowHeight;
    int m_scrollBarSize;
    int m_scrollBarMargin;
    int m_scroll;
    int m_maxScroll;
    int m_trackBarHeight;
    int m_selectedIndex;
    
    enum DragMode{
        DragNone=0,
        DragContents,
        DragTrackBar
    } m_drag;
    int m_dragInitialScroll;
    twPoint m_dragInitialMousePos;
    
    twRect rectForContents() const;
    twRect rectForScrollBar() const;
    twRect rectForTrackBar() const;
    twRect rectForItemAtIndex(size_t) const;
    size_t indexForItemAtPoint(const twPoint&) const;
    bool isRowSelectable(size_t) const;
    
    void invalidateItem(size_t);
    
    void computeScroll();
    
public:
    ATListView();
    virtual ~ATListView();
    
    virtual void clientPaint(const twPaintStruct& p);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
    
	virtual void setRect(const twRect&);
    
    ItemList& items(){return m_items;}
    const ItemList& items() const{return m_items;}
    
    void reloadData();
    
    void setScroll(int);
    void setSelectedIndex(int);
    
    int selectedIndex() const{return m_selectedIndex;}
    void scrollToItem(size_t);
};

