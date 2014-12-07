//
//  TXSineTable.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/13/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif
    
#define TXSineTableSize 64
#define TXSineTableSizeShift 6
    
    extern short TXSineTable[];
    
    int TXSineWave(uint32_t);
    
#ifdef __cplusplus
};
#endif
