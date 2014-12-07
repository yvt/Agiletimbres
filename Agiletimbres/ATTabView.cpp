//
//  ATTabView.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/14/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATTabView.h"

ATTabView::ATTabView(){
    m_drag=-1;
    m_selectedTabIndex=0;
    m_rightMargin=0;
}

ATTabView::~ATTabView(){
    // no need to delete tabs because
    // twWnd automatically does for children
}

void ATTabView::setTabCount(size_t count){
    while(count>m_tabs.size()){
        twWnd *tab=new twWnd();
        tab->setRect(rectForTab());
        tab->setParent(this);
        tab->setTitle(L"Untitled");
        m_tabs.push_back(tab);
        if((int)m_tabs.size()-1==m_selectedTabIndex)
            tab->show();
    }
    while(count<m_tabs.size()){
        twWnd *tab=m_tabs.back();
        delete tab;
        m_tabs.pop_back();
    }
}

size_t ATTabView::tabCount() const{
    return m_tabs.size();
}

twWnd *ATTabView::tabAtIndex(size_t i) const{
    return m_tabs[i];
}

void ATTabView::setTabAtIndex(size_t i, twWnd *tab){
    m_tabs[i]=tab;
    tab->setRect(rectForTab());
    tab->setParent(this);
    if((int)i==m_selectedTabIndex)
        tab->show();
    else
        tab->hide();
}

twRect ATTabView::rectForTab() const{
    twRect r=getClientRect();
    return twRect(0, 14, r.w, r.h-14);
}
twRect ATTabView::rectForTabBar() const{
    twRect r=getClientRect();
    return twRect(0, 0, r.w-m_rightMargin, 13);
}
twRect ATTabView::rectForTabInTabBar(size_t i) const{
    twRect r=rectForTabBar();
    r.x=(r.w*i)/m_tabs.size();
    r.w=(r.w*(i+1))/m_tabs.size()-r.x;
    return r;
}

void ATTabView::clientPaint(const twPaintStruct &p){
    twDC *dc=p.dc;
    if(rectForTabBar() && p.paintRect){
        
        twRect r;
        const twFont *f=getFont();
        
        for(size_t i=0;i<m_tabs.size();i++){
            r=rectForTabInTabBar(i);
            
            twColor bgColor=twRGB(192, 192, 192);
            twColor fgColor=0;
            if((int)i==m_drag && m_hover){
                bgColor=tw_curSkin->getSelectionColor();
                fgColor=tw_curSkin->getSelectionTextColor();
            }else if(m_selectedTabIndex==(int)i){
                bgColor=tw_curSkin->getSelectionColor();
                fgColor=tw_curSkin->getSelectionTextColor();
            }
            
            if(i==0){
                dc->fillRect(bgColor, twRect(r.x+r.h/2,
                                             r.y,
                                             r.w-r.h/2,
                                             r.h));
                dc->fillEllipse(bgColor, twRect(r.x, r.y,
                                                r.h-1,
                                                r.h-1));
            }else if(i==m_tabs.size()-1){
                dc->fillRect(bgColor, twRect(r.x,
                                             r.y,
                                             r.w-r.h/2,
                                             r.h));
                dc->fillEllipse(bgColor, twRect(r.x+r.w-r.h, r.y,
                                                r.h-1,
                                                r.h-1));
            }else{
                dc->fillRect(bgColor, r);  
            }
            
            
            std::wstring title=m_tabs[i]->getTitle();
            twSize sz=f->measure(title);
            
            f->render(dc, fgColor, 
                      twPoint(r.x+(r.w-sz.w)/2,
                              r.y+(r.h-sz.h)/2),
                      title);
        }
        
    }
}

bool ATTabView::clientHitTest(const twPoint &) const{
    return true;
}

void ATTabView::clientMouseDown(const twPoint &p, twMouseButton mb){
    if(mb!=twMB_left)
        return;
    
    m_drag=-1;
    twSetCapture(this);
    
    if(rectForTabBar() && p){
        
        for(size_t i=0;i<m_tabs.size();i++){
            twRect r=rectForTabInTabBar(i);
            if(r&&p){
                
                m_drag=(int)i;
                m_hover=true;
                invalidateClientRect(r);
                
            }
        }
        
    }
}

void ATTabView::clientMouseMove(const twPoint &p){
    if(m_drag!=-1){
        twRect r=rectForTabInTabBar((size_t)m_drag);
        bool newHover=(r&&p);
        if(newHover!=m_hover){
            m_hover=newHover;
            invalidateClientRect(r);
        }
    }
}

void ATTabView::clientMouseUp(const twPoint &p, twMouseButton){
    twReleaseCapture();
    if(m_drag!=-1){
        if(m_hover){
            
            setSelectedTabIndex(m_drag);
            m_drag=-1;
            sendCmdToParent();
        }
        m_drag=-1;
    }
}

void ATTabView::setSelectedTabIndex(int i){
    if(i==m_selectedTabIndex)
        return;
    invalidateClientRect(rectForTabInTabBar((size_t)m_selectedTabIndex));
    m_tabs[m_selectedTabIndex]->hide();
    m_selectedTabIndex=i;
    m_tabs[m_selectedTabIndex]->show();
    invalidateClientRect(rectForTabInTabBar((size_t)m_selectedTabIndex));
}

void ATTabView::setRect(const twRect &rt){
    twWnd::setRect(rt);
    for(size_t i=0;i<m_tabs.size();i++)
        m_tabs[i]->setRect(rectForTab());
    
}

void ATTabView::setRightMargin(int rm){
     invalidateClientRect(rectForTabBar());
    m_rightMargin=rm;
    invalidateClientRect(rectForTabBar());
}
