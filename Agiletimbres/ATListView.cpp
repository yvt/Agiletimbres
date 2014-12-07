//
//  ATListView.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATListView.h"
#include <tcw/twBaseSkin.h>

ATListView::ATListView(){
    m_rowHeight=17;
    m_scrollBarSize=10;
    m_scroll=0;
    m_maxScroll=0;
    m_scrollBarMargin=1;
    m_tint=twRGB(192, 192, 192);
    m_selectedIndex=-1;
}

ATListView::~ATListView(){
    
}

#pragma mark - Geometry

twRect ATListView::rectForContents() const{
    twRect cliRect=getClientRect();
    return twRect(0, 0, cliRect.w-m_scrollBarSize-
                  m_scrollBarMargin, cliRect.h);
}

twRect ATListView::rectForScrollBar() const{
    twRect cliRect=getClientRect();
    return twRect(cliRect.w-m_scrollBarSize, 0,
                  m_scrollBarSize, cliRect.h);
}

twRect ATListView::rectForTrackBar() const{
    twRect scrollBarRect=rectForScrollBar();
    twRect rt;
    rt.x=scrollBarRect.x;
    rt.y=scrollBarRect.y;
    if(m_maxScroll>0)
        rt.y+=(scrollBarRect.h-m_trackBarHeight)*
        m_scroll/m_maxScroll;
    rt.w=scrollBarRect.w;
    rt.h=m_trackBarHeight;
    return rt;
}

twRect ATListView::rectForItemAtIndex(size_t index) const{
    twRect contentsRect=rectForContents();
    twRect rt;
    rt.x=contentsRect.x;
    rt.y=contentsRect.y+(index*m_rowHeight)-m_scroll;
    rt.w=contentsRect.w;
    rt.h=m_rowHeight;
    return rt;
}

size_t ATListView::indexForItemAtPoint(const twPoint &pt) const{
    assert(m_items.size()>0);
    
    int contentsY=pt.y+m_scroll-rectForContents().y;
    if(contentsY<0)
        return 0;
    
    size_t index=(size_t)(contentsY/m_rowHeight);
    if(index>=m_items.size())
        return m_items.size()-1;
    return index;
}

void ATListView::computeScroll(){
    int itemsHeight=m_items.size()*m_rowHeight;
    twRect contentsRect=rectForContents();
    twRect scrollBarRect=rectForScrollBar();
    int oldMaxScroll=m_maxScroll;
    int oldScroll=m_scroll;
    
    if(contentsRect.h<=0)
        return;
    
    if(itemsHeight<=contentsRect.h){
        m_maxScroll=0;
    }else{
        m_maxScroll=itemsHeight-contentsRect.h;
    }
    
    if(m_scroll<0){
        m_scroll=0;
    }
    
    if(m_scroll>m_maxScroll)
        m_scroll=m_maxScroll;
    
    if(m_maxScroll>0)
        m_trackBarHeight=(contentsRect.h*scrollBarRect.h)/itemsHeight;
    else
        m_trackBarHeight=contentsRect.h;
    
    if(m_scroll!=oldScroll ||
       m_maxScroll!=oldMaxScroll)
        invalidateClientRect();
    
}

void ATListView::setScroll(int scroll){
    if(scroll<0)
        scroll=0;
    if(scroll>m_maxScroll)
        scroll=m_maxScroll;
    if(scroll!=m_scroll){
        m_scroll=scroll;
        invalidateClientRect();
    }
}

void ATListView::reloadData(){
    computeScroll();
    
    if(m_selectedIndex!=-1){
        if((size_t)m_selectedIndex>=m_items.size())
            m_selectedIndex=-1;
    }
    
    invalidateClientRect();
}

bool ATListView::isRowSelectable(size_t i) const{
    if(i>=m_items.size())
        return false;
    const std::wstring& text=m_items[i];
    if(text.size()==0)
        return true;
    if(text[0]==L'\x001')
        return false;
    return true;
}

#pragma mark - Renderer

void ATListView::clientPaint(const twPaintStruct &p){
    twDC *dc=p.dc;
    twRect rt=p.boundRect;
    
    // first, render contents.
    if(rectForContents() && p.paintRect){
        twRect oldClipRect=dc->getClipRect();
        twRect contentsRect=rectForContents();
        const twFont *font=getFont();
        
        assert(m_scroll>=0);
        size_t firstVisibleRow=(size_t)(m_scroll/m_rowHeight);
        size_t i=firstVisibleRow;
        
        dc->addClipRect(contentsRect);
        dc->fillRect(m_tint, contentsRect);
        
        // draw row lines.
        for(int i=m_rowHeight-1-(m_scroll%m_rowHeight);
            i<contentsRect.h;i+=m_rowHeight){
            dc->fillRect(0x7f7f7f,
                         twRect(contentsRect.x,
                                contentsRect.y+i,
                                contentsRect.w,
                                1));
        }
        
        while(i<m_items.size()){
            twRect itemRect=rectForItemAtIndex(i);
            
            if(itemRect.y>contentsRect.y+contentsRect.h)
                break;
            
            if(!(itemRect&&p.paintRect)){
                i++;
                continue;
            }
            
            bool selected=(m_selectedIndex==(int)i);
            twColor textColor=twRGB(0, 0, 0);
            bool selectable=isRowSelectable(i);
            
            if(!selectable){
                selected=false;
                textColor=twRGB(64, 64, 64);
            }
            
            if(selected){
                // fill the item avoiding row line.
                dc->fillRect(tw_curSkin->getSelectionColor(), 
                             twRect(itemRect.x, itemRect.y,
                                    itemRect.w, itemRect.h-1));
                
                textColor=tw_curSkin->getSelectionTextColor();
            }
            
            const std::wstring& text=m_items[i];
            twSize textSize=font->measure(text);
            
            twPoint pt;
            pt.x=itemRect.x+8;
            pt.y=itemRect.y+(itemRect.h-1-textSize.h)/2+1;
            
            font->render(dc, textColor, pt, text);
            
            i++;
        }
        
        // cutoff corners.
        {
            twRect r=contentsRect;
            dc->fillRect(0, twRect(r.x, r.y,
                                   3, 1));
            dc->fillRect(0, twRect(r.x, r.y,
                                   1, 3));
            
            dc->fillRect(0, twRect(r.x+r.w-3, r.y,
                                   3, 1));
            dc->fillRect(0, twRect(r.x+r.w-1, r.y,
                                   1, 3));
            
            dc->fillRect(0, twRect(r.x, r.y+r.h-1,
                                   3, 1));
            dc->fillRect(0, twRect(r.x, r.y+r.h-3,
                                   1, 3));
            
            dc->fillRect(0, twRect(r.x+r.w-3, r.y+r.h-1,
                                   3, 1));
            dc->fillRect(0, twRect(r.x+r.w-1, r.y+r.h-3,
                                   1, 3));
        }
        
        dc->setClipRect(oldClipRect);
    }
    
    // and, draw scroll bar.
    {
        twRect r=rectForTrackBar();
        dc->fillRect(m_tint, r);
        
        // cutoff corners.
        dc->fillRect(0, twRect(r.x, r.y,
                               3, 1));
        dc->fillRect(0, twRect(r.x, r.y,
                               1, 3));
        
        dc->fillRect(0, twRect(r.x+r.w-3, r.y,
                               3, 1));
        dc->fillRect(0, twRect(r.x+r.w-1, r.y,
                               1, 3));
        
        dc->fillRect(0, twRect(r.x, r.y+r.h-1,
                               3, 1));
        dc->fillRect(0, twRect(r.x, r.y+r.h-3,
                               1, 3));
        
        dc->fillRect(0, twRect(r.x+r.w-3, r.y+r.h-1,
                               3, 1));
        dc->fillRect(0, twRect(r.x+r.w-1, r.y+r.h-3,
                               1, 3));
    }
    
}

#pragma mark - Responder

bool ATListView::clientHitTest(const twPoint &) const{
    return true;
}

void ATListView::clientMouseDown(const twPoint &p,
                                 twMouseButton mb){
    if(mb!=twMB_left)
        return;
    
    twSetCapture(this);
    if(rectForContents()&&p){
        if(m_items.size()==0)
            return;
        
        if((p.y+m_scroll)<m_rowHeight*(int)m_items.size()){
            size_t i=indexForItemAtPoint(p);
            if(!isRowSelectable(i))
                return;
            setSelectedIndex(i);
            m_drag=DragContents;
        }
    }else if(rectForScrollBar()&&p){
        if(rectForTrackBar()&&p){
            if(m_maxScroll==0)
                return;
            m_dragInitialScroll=m_scroll;
            m_dragInitialMousePos=p;
            m_drag=DragTrackBar;
        }else if(p.y<rectForTrackBar().y){
            setScroll(m_scroll-rectForContents().h);
        }else{
            setScroll(m_scroll+rectForContents().h);
        }
    }
}

void ATListView::clientMouseMove(const twPoint &p){
    if(m_drag==DragContents){
        size_t i=indexForItemAtPoint(p);
        if(!isRowSelectable(i))
            return;
        setSelectedIndex(i);
    }else if(m_drag==DragTrackBar){
        int delta=p.y-m_dragInitialMousePos.y;
        int range=rectForScrollBar().h-m_trackBarHeight;
        delta=(delta*m_maxScroll)/range;
        delta+=m_dragInitialScroll;
        setScroll(delta);
    }
}

void ATListView::clientMouseUp(const twPoint &p,
                               twMouseButton mb){
    twReleaseCapture();
    if(m_drag==DragContents){
        sendCmdToParent();
    }
    m_drag=DragNone;
}

void ATListView::setRect(const twRect &rt){
    twWnd::setRect(rt);
    computeScroll();
}

void ATListView::setSelectedIndex(int index){
    if(index==m_selectedIndex)
        return;
    if(m_selectedIndex!=-1)
        invalidateItem((size_t)m_selectedIndex);
    
    m_selectedIndex=index;
    
    if(m_selectedIndex!=-1)
        invalidateItem((size_t)m_selectedIndex);
}

void ATListView::invalidateItem(size_t index){
    assert(index<m_items.size());
    invalidateClientRect(rectForItemAtIndex(index));
}
void ATListView::scrollToItem(size_t item){
    twRect rt=rectForItemAtIndex(item);
    twRect contentsRect=rectForContents();
    if(rt.y<0){
        m_scroll+=rt.y;
        invalidateClientRect();
    }else if(rt.y+rt.h>contentsRect.h){
        m_scroll+=rt.y+rt.h-contentsRect.h;
        invalidateClientRect();
    }
    
}
