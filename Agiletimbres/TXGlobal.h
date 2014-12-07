//
//  TXGlobal.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include <string>
#include <vector>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

struct TXConfig{
    float sampleRate;
    
    TXConfig(){
        sampleRate=44100.f;
    }
    
};

void TXAddVectorInplace(int32_t *outBuffer,
                        const int32_t *srcBuffer,
                        unsigned int elements);

float TXSqrtApprox(float);
