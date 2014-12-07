//
//  ATDropdownList.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/19/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATDropdownList.h"
#include "Utils.h"

class ATDropdownList::DropdownView: public twDialog{
    ATDropdownList *m_dropdownList;
    int m_rowHeight;
    int m_drag;
    bool m_hover;
    
    ItemList& items() const{return m_dropdownList->items();}
    twRect rectForItem(int index) const{
        twRect rt=getClientRect();
        rt.x=0; rt.y=m_rowHeight*index;
        rt.h=m_rowHeight;
        return rt;
    }
protected:
    virtual twPoint getStartupPos(const twSize&,
								  const twRect& desktop){
        twRect dropdownListRect=m_dropdownList->getWndRect();
        return twPoint(dropdownListRect.x,
                       dropdownListRect.y+dropdownListRect.h-1);
    }
public:
    DropdownView(ATDropdownList *dl){
        twWndStyle style=getStyle();
        style.border=twBS_panel;
        setStyle(style);
        
        m_dropdownList=dl;
        m_rowHeight=13;
        
        // set size.
        twRect rt;
        rt.x=0; rt.y=0;
        rt.w=dl->getRect().w;
        rt.h=getBorderSize().h+m_rowHeight*(int)items().size();
        setRect(rt);
        
        m_drag=-1;
        m_hover=false;
    }
    
    virtual void clientPaint(const twPaintStruct& p){
        twDC *dc=p.dc;
        twRect rt(twPoint(0,0),getClientRect().size());
        dc->fillRect(tw_curSkin->getWndColor(), rt);
        
        const ItemList& items=this->items();
        for(size_t i=0;i<items.size();i++){
            twColor fgColor=tw_curSkin->getWndTextColor();
            twRect r=rectForItem((int)i);
            if((int)i==m_drag && m_hover){
                dc->fillRect(tw_curSkin->getSelectionColor(), 
                             r);
                fgColor=tw_curSkin->getSelectionTextColor();
            }
            
            getFont()->render(dc, fgColor, twPoint(r.x, r.y+1),
                              items[i]);
        }
        
    }
	virtual bool clientHitTest(const twPoint&) const{
        return true;
    }
	
	virtual void clientMouseDown(const twPoint& pt, twMouseButton mb){
        if(mb!=twMB_left)
            return;
        
        twSetCapture(this);
        
        m_drag=-1;
        
        twRect rt(twPoint(0,0),getClientRect().size());
        if(rt&&pt){
            m_drag=pt.y/m_rowHeight;
            assert(m_drag>=0);
            assert(m_drag<(int)items().size());
            m_hover=true;
            invalidateClientRect(rectForItem(m_drag));
        }
        
        
        
    }
	virtual void clientMouseMove(const twPoint& pt){
        if(m_drag!=-1){
            
            twRect rt(twPoint(0,0),getClientRect().size());
            bool newHover=rt&&pt;
            
            int newDrag=pt.y/m_rowHeight;
            if(newDrag<0)
                newDrag=0;
            if(newDrag>=(int)items().size())
                newDrag=items().size()-1;
            
            if(newHover!=m_hover || newDrag!=m_drag){
                
                invalidateClientRect(rectForItem(m_drag));
                invalidateClientRect(rectForItem(newDrag));
                m_hover=newHover;
                m_drag=newDrag;
            }
            
        }
    }
	virtual void clientMouseUp(const twPoint&, twMouseButton){
        twReleaseCapture();
        if(m_drag!=-1){
            if(m_hover){
                
                m_dropdownList->setSelectedIndex(m_drag);
                endDialog();
                m_dropdownList->sendCmdToParent();
                
                invalidateClientRect(rectForItem(m_drag));
            }
            m_drag=-1;
        }
    }
    
   
    virtual void backgroundTouched(){
        endDialog();
    }
};

ATDropdownList::ATDropdownList(){
    twWndStyle style=getStyle();
    style.border=twBS_panel;
    setStyle(style);
    m_drag=false;
    m_selectedIndex=0;
}

ATDropdownList::~ATDropdownList(){
    
}

void ATDropdownList::clientPaint(const twPaintStruct &p){
    twColor bgColor, fgColor;
    
    bgColor=tw_curSkin->getWndColor();
    fgColor=tw_curSkin->getWndTextColor();
    
    if(m_drag && m_hover){
        bgColor=tw_curSkin->getSelectionColor();
        fgColor=tw_curSkin->getSelectionTextColor();
    }
    
    twDC *dc=p.dc;
    std::wstring str;
    const twFont *font=getFont();
    twRect rt=getClientRect();
    dc->fillRect(bgColor, p.boundRect);
    
    if(m_selectedIndex>=0 && m_selectedIndex<(int)m_items.size()){
        
        str=m_items[m_selectedIndex];
        
        font->render(dc, fgColor, twPoint(0, 0), str);
        
    }
    
    // draw arrow.
    twPoint arrowPt(rt.w-6, (rt.h-3)/2+1);
    dc->fillRect(fgColor, twRect(arrowPt.x-2, arrowPt.y-1,
                                 5, 1));
    dc->fillRect(fgColor, twRect(arrowPt.x-1, arrowPt.y,
                                 3, 1));
    dc->fillRect(fgColor, twRect(arrowPt.x, arrowPt.y+1,
                                 1, 1));
    
    
}

bool ATDropdownList::clientHitTest(const twPoint &pt) const{
    return true;
}

void ATDropdownList::clientMouseDown
(const twPoint &pt, twMouseButton mb){
    if(mb!=twMB_left)
        return;
    
    m_drag=true;
    m_hover=true;
    
    twSetCapture(this);
    
    invalidateClientRect();
    
}

void ATDropdownList::clientMouseMove(const twPoint &pt){
    if(m_drag){
        bool newHover=twRect(twPoint(0,0),getClientRect().size())&&pt;
        if(newHover!=m_hover){
            m_hover=newHover;
            invalidateClientRect();
        }
        
    }
    
    
}

void ATDropdownList::clientMouseUp
(const twPoint & pt, twMouseButton mb){
    if(m_drag){
        m_drag=false;
        twReleaseCapture();
        if(m_hover){
            invalidateClientRect();
            dropdown();
        }
        
    }
}

void ATDropdownList::setSelectedIndex(int index){
    m_selectedIndex=index;
    invalidateClientRect();
}

void ATDropdownList::dropdown(){
    DropdownView dd(this);
    dd.setNeedsDimming(true);
    dd.setDimmerStyle(twDS_normal);
    dd.showModal(NULL);
}
