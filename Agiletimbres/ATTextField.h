/*
 *  ATTextField.h
 *  XTBook
 *
 *  Created by Nexhawks on 2/13/11.
 *  Copyright 2011 Nexhawks. All rights reserved.
 *
 */

#include "StdAfx.h"


class ATTextField: public twWnd{
	friend class ATTextFieldBlankTimer;
protected:
	int m_scroll;
	int m_cursorIndex;
	int m_markIndex;
	bool m_drag;
	bool m_blanking;
	twTimer *m_blankTimer;
	int m_inputBegin;
	int m_inputLength;
	
	void blank();
public:
	ATTextField();
	virtual ~ATTextField();
	
	virtual void setRect(const twRect&);
	virtual void clientPaint(const twPaintStruct&);
	virtual bool clientHitTest(const twPoint&) const;
	
	virtual void clientKeyDown(const std::wstring&);
	virtual void clientKeyPress(wchar_t);
	
	virtual void clientMouseDown(const twPoint&, twMouseButton);
	virtual void clientMouseMove(const twPoint&);
	virtual void clientMouseUp(const twPoint&, twMouseButton);
	
	virtual void clientEnter();
	virtual void clientLeave();
	
	virtual int positionForCharIndex(int) const;
	virtual int charIndexForPosition(int) const;
	virtual void scrollToPosition(int);
	virtual void scrollToCharIndex(int);
	virtual void scrollToCursor();
	virtual void insertString(const std::wstring&);
	virtual void backSpace();
	virtual void moveLeft();
	virtual void moveRight();
	virtual void selectAll();
	virtual void selectLast();
	virtual void removeAllContents();
	
	virtual void resetBlank();
	
};
