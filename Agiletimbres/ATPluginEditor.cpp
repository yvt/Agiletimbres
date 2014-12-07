//
//  ATPluginEditor.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/18/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATPluginEditor.h"
#include "ATGPDS1Editor.h"
#include "ATMatrixReverb1Editor.h"
#include "TXFactory.h"
#include "ATChorusFilterEditor.h"
#include "ATLegacyMidiEditor.h"
#include "ATOverdriveFilterEditor.h"
#include "ATPhaserFilterEditor.h"
#include "ATDecimatorFilterEditor.h"
#include "ATSpectralGateFilterEditor.h"

void ATPluginEditor::clientPaint(const twPaintStruct &p){
    twDC *dc=p.dc;
    const twFont *font=getFont();
    
    std::wstring str=L"No editor";
    twSize siz=font->measure(str);
    
    twRect rt=p.boundRect;
    
    font->render(dc, twRGB(192, 192, 192), 
                 twPoint((rt.w-siz.w)/2,
                         (rt.h-siz.h)/2), 
                 str);
    
}

ATPluginEditor *ATPluginEditor::editorForPluginIdentifier
(const std::string & str){
    
    if(str==TXGPDS1::sharedFactory()->identifier()){
        return new ATGPDS1Editor();
    }else if(str==TXMatrixReverb1::sharedFactory()->identifier()){
        return new ATMatrixReverb1Editor();
    }else if(str==TXChorusFilter::sharedFactory()->identifier()){
        return new ATChorusFilterEditor();
    }else if(str==TXLegacyMidi::sharedFactory()->identifier()){
        return new ATLegacyMidiEditor();
    }else if(str==TXOverdriveFilter::sharedFactory()->identifier()){
        return new ATOverdriveFilterEditor();
    }else if(str==TXPhaserFilter::sharedFactory()->identifier()){
        return new ATPhaserFilterEditor();
    }else if(str==TXDecimatorFilter::sharedFactory()->identifier()){
        return new ATDecimatorFilterEditor();
    }else if(str==TXSpectralGateFilter::sharedFactory()->identifier()){
        return new ATSpectralGateFilterEditor();
    }
    
    // fall back.
    return new ATPluginEditor();
}
