/*
 *  ATTextField.cpp
 *  XTBook
 *
 *  Created by Nexhawks on 2/13/11.
 *  Copyright 2011 Nexhawks. All rights reserved.
 *
 */

#include "ATTextField.h"
#include <tcw/twBaseSkin.h>
#include <assert.h>
#include <tcw/twEvent.h>

class ATTextFieldBlankTimer : public twTimer{
protected:
	ATTextField *textField;
	virtual void fire(){
		textField->blank();
	}
public:
	ATTextFieldBlankTimer(ATTextField *field){
		textField=field;
	}
	virtual ~ATTextFieldBlankTimer(){
	}
};

ATTextField::ATTextField(){
    twWndStyle style=getStyle();
    style.border=twBS_panel;
    setStyle(style);
    
	m_drag=false;
	m_cursorIndex=0;
	m_markIndex=0;
	m_scroll=0;
	m_blankTimer=new ATTextFieldBlankTimer(this);
	m_blankTimer->setInterval(500);
}
ATTextField:: ~ATTextField(){
	delete m_blankTimer;
}


void ATTextField::setRect(const twRect& rt){
	
	twWnd::setRect(rt);
}
void ATTextField::clientPaint(const twPaintStruct& p){
	twDC *dc=p.dc;
	
	dc->fillRect(tw_curSkin->getWndColor(), twRect(twPoint(0, 0), p.bound));
	
	int markStart, markEnd;
	if(m_cursorIndex<m_markIndex){
		markStart=m_cursorIndex;
		markEnd=m_markIndex;
	}else{
		markEnd=m_cursorIndex;
		markStart=m_markIndex;
	}
	
	twRect rt=getClientRect();
	const std::wstring& title=getTitle();
	std::wstring str;
	const twFont *font=getFont();
	int lastPos=-m_scroll;
	
	twColor selectColor=tw_curSkin->getSelectionColor();
	twColor selectTextColor=tw_curSkin->getSelectionTextColor();
	twColor textColor=tw_curSkin->getWndTextColor();
	bool focused=getStatus().focused;
	
	
	
	for(int i=0;i<title.size();i++){
		str=title[i];
		
		twSize siz=font->measure(str);
		
		if(i>=markStart && i<markEnd && focused){
			dc->fillRect(selectColor, twRect(lastPos, 0, siz.w, rt.h));
			font->render(dc, selectTextColor, twPoint(lastPos, 1), str);
		}else{
			font->render(dc, textColor, twPoint(lastPos, 1), str);
		}
		
		lastPos+=siz.w;
	}
	
	if(markStart==markEnd && focused && m_blanking){
		lastPos=positionForCharIndex(markStart);
		lastPos-=m_scroll;
        if(lastPos==0)
            lastPos++;
		dc->fillRect(textColor, twRect(lastPos, 1, 1, rt.h-2));
	}
}
bool ATTextField::clientHitTest(const twPoint&) const{
	return true;
}

void ATTextField::clientMouseDown(const twPoint& pt, twMouseButton button){
	if(button==twMB_left){
		m_cursorIndex=charIndexForPosition(pt.x+m_scroll);
		m_markIndex=m_cursorIndex;
		m_drag=true;
		resetBlank();
		scrollToCursor();
		invalidate();
	}
	twSetCapture(this);
}
void ATTextField::clientMouseMove(const twPoint& pt){
	if(m_drag){
		int oldCursorIndex=m_cursorIndex;
		m_cursorIndex=charIndexForPosition(pt.x+m_scroll);
		if(m_cursorIndex!=oldCursorIndex)
			invalidate();
		scrollToCursor();
	}
}
void ATTextField::clientMouseUp(const twPoint& pt, twMouseButton button){
	if(button==twMB_left){
		m_drag=false;
	}
	twReleaseCapture();
}

void ATTextField::clientKeyDown(const std::wstring& keyName){
	if(keyName.size()==1){
		if(keyName==L" "){
            insertString(keyName);
            sendCmdToParent();
        
		}else{
			
			wchar_t ch=keyName[0];
			
			SDLMod modState=SDL_GetModState();
			if((modState&KMOD_CAPS) || (modState&KMOD_SHIFT)){
				switch(ch){
					case L'a': ch=L'A'; break;
                    case L'b': ch=L'B'; break;
                    case L'c': ch=L'C'; break;
                    case L'd': ch=L'D'; break;
                    case L'e': ch=L'E'; break;
                    case L'f': ch=L'F'; break;
                    case L'g': ch=L'G'; break;
                    case L'h': ch=L'H'; break;
                    case L'i': ch=L'I'; break;
                    case L'j': ch=L'J'; break;
                    case L'k': ch=L'K'; break;
                    case L'l': ch=L'L'; break;
                    case L'm': ch=L'M'; break;
                    case L'n': ch=L'N'; break;
                    case L'o': ch=L'O'; break;
                    case L'p': ch=L'P'; break;
                    case L'q': ch=L'Q'; break;
                    case L'r': ch=L'R'; break;
                    case L's': ch=L'S'; break;
                    case L't': ch=L'T'; break;
                    case L'u': ch=L'U'; break;
                    case L'v': ch=L'V'; break;
                    case L'w': ch=L'W'; break;
                    case L'x': ch=L'X'; break;
                    case L'y': ch=L'Y'; break;
                    case L'z': ch=L'Z'; break;
                        
					case L'-':
						ch=L' ';
						break;
                    case L' ':
						ch=L'-';
						break;
				}
			}
			insertString(std::wstring(&ch, 1));
			sendCmdToParent();
		}
	}else if(keyName==L"BackSpace"){
		backSpace();
	}else if(keyName==L"Right" || keyName==L"KeyPad6"){
		moveRight();
	}else if(keyName==L"Left" || keyName==L"KeyPad4"){
		moveLeft();
	}
}
void ATTextField::clientKeyPress(wchar_t keyChar){
	//printf("0x%x %lc\n", (int)keyChar, keyChar);
}

int ATTextField::positionForCharIndex(int index) const{
	const std::wstring& title=getTitle();
	if(index<0)
		index=0;
	if(index>title.size())
		index=title.size();
	const twFont *font=getFont();
	std::wstring str=title.substr(0, index);
	twSize size;
	size=font->measure(str);
	return size.w;
}
int ATTextField::charIndexForPosition(int position) const{
	const std::wstring& title=getTitle();
	std::wstring str;
	const twFont *font=getFont();
	int lastPos=0;
	for(int i=0;i<title.size();i++){
		str=title[i];
		
		twSize siz=font->measure(str);
		if(position<lastPos+siz.w/2)
			return i;
		
		lastPos+=siz.w;
	}
	return title.size();
}
void ATTextField::scrollToCursor(){
	scrollToCharIndex(m_cursorIndex);
}
void ATTextField::scrollToPosition(int pos){
	const twRect &rt=getClientRect();
	if(pos<m_scroll){
		m_scroll=pos;
		invalidate();
	}
	if(pos>m_scroll+rt.w-2){
		m_scroll=pos-rt.w+2;
		invalidate();
	}
	
	int textWidth=positionForCharIndex(getTitle().size());
	if(textWidth+2>=rt.w){
		// may need scroll.
		if(m_scroll>textWidth-rt.w+2){
			m_scroll=textWidth-rt.w+2;
			invalidate();
		}
	}else{
		if(m_scroll!=0){
			m_scroll=0;
			invalidate();
		}
	}
	
}
void ATTextField::scrollToCharIndex(int index){
	int position;
	position=positionForCharIndex(index);
	scrollToPosition(position);
}
void ATTextField::insertString(const std::wstring& str){
	int markStart, markEnd;
	const std::wstring& title=getTitle();
	if(m_cursorIndex<m_markIndex){
		markStart=m_cursorIndex;
		markEnd=m_markIndex;
	}else{
		markEnd=m_cursorIndex;
		markStart=m_markIndex;
	}
	assert(markStart>=0);
	assert(markStart<=title.size());
	assert(markEnd>=0);
	assert(markEnd<=title.size());
	
	
	std::wstring newTitle;
	newTitle=title.substr(0, markStart)+str+title.substr(markEnd);
	m_cursorIndex=markStart+str.size();
	m_markIndex=m_cursorIndex;
	setTitle(newTitle);
	scrollToCursor();
	resetBlank();
}

void ATTextField::backSpace(){
	if(m_cursorIndex!=m_markIndex){
		insertString(L"");
		sendCmdToParent();
	}else{
		if(m_markIndex>0){
			m_markIndex--;
			insertString(L"");
			sendCmdToParent();
		}
	}
	resetBlank();
	scrollToCursor();
}

void ATTextField::moveLeft(){
	if(m_cursorIndex!=m_markIndex){
		if(m_cursorIndex>m_markIndex){
			m_cursorIndex=m_markIndex;
		}else{
			m_markIndex=m_cursorIndex;
		}
		invalidate();
	}else{
		if(m_markIndex>0){
			m_markIndex--;
			m_cursorIndex=m_markIndex;
			invalidate();
		}
	}
	resetBlank();
	scrollToCursor();
}	

void ATTextField::moveRight(){
	if(m_cursorIndex!=m_markIndex){
		if(m_cursorIndex<m_markIndex){
			m_cursorIndex=m_markIndex;
		}else{
			m_markIndex=m_cursorIndex;
		}
		invalidate();
	}else{
		const std::wstring& title=getTitle();
		if(m_markIndex<title.size()){
			m_markIndex++;
			m_cursorIndex=m_markIndex;
			invalidate();
		}
	}
	resetBlank();
	scrollToCursor();
}	

void ATTextField::clientEnter(){
	m_blankTimer->addToEvent(tw_event);
	m_blanking=true;
}
void ATTextField::clientLeave(){
	m_blankTimer->invalidate();
}


void ATTextField::blank(){
	m_blanking=!m_blanking;
	invalidate();
}

void ATTextField::resetBlank(){
	m_blanking=true;
	if(m_blankTimer->isValid()){
		m_blankTimer->invalidate();
		m_blankTimer->addToEvent(tw_event);
	}
	invalidate();
}
void ATTextField::selectAll(){
	m_markIndex=0;
	m_cursorIndex=getTitle().size();
	scrollToCursor();
	resetBlank();
	invalidate();
}
void ATTextField::selectLast(){
	m_markIndex=getTitle().size();
	m_cursorIndex=getTitle().size();
	scrollToCursor();
	resetBlank();
	invalidate();
}
void ATTextField::removeAllContents(){
	setTitle(L"");
	m_markIndex=0;
	m_cursorIndex=0;
	scrollToCursor();
	resetBlank();
	invalidate();
}
