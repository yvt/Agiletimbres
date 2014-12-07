//
//  TXSineTable.c
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/13/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include <assert.h>
#include "TXSineTable.h"

short TXSineTable[]={
    0, /* 0 */
    804, /* 1 */
    1607, /* 2 */
    2410, /* 3 */
    3211, /* 4 */
    4011, /* 5 */
    4807, /* 6 */
    5601, /* 7 */
    6392, /* 8 */
    7179, /* 9 */
    7961, /* 10 */
    8739, /* 11 */
    9511, /* 12 */
    10278, /* 13 */
    11038, /* 14 */
    11792, /* 15 */
    12539, /* 16 */
    13278, /* 17 */
    14009, /* 18 */
    14732, /* 19 */
    15446, /* 20 */
    16150, /* 21 */
    16845, /* 22 */
    17530, /* 23 */
    18204, /* 24 */
    18867, /* 25 */
    19519, /* 26 */
    20159, /* 27 */
    20787, /* 28 */
    21402, /* 29 */
    22004, /* 30 */
    22594, /* 31 */
    23169, /* 32 */
    23731, /* 33 */
    24278, /* 34 */
    24811, /* 35 */
    25329, /* 36 */
    25831, /* 37 */
    26318, /* 38 */
    26789, /* 39 */
    27244, /* 40 */
    27683, /* 41 */
    28105, /* 42 */
    28510, /* 43 */
    28897, /* 44 */
    29268, /* 45 */
    29621, /* 46 */
    29955, /* 47 */
    30272, /* 48 */
    30571, /* 49 */
    30851, /* 50 */
    31113, /* 51 */
    31356, /* 52 */
    31580, /* 53 */
    31785, /* 54 */
    31970, /* 55 */
    32137, /* 56 */
    32284, /* 57 */
    32412, /* 58 */
    32520, /* 59 */
    32609, /* 60 */
    32678, /* 61 */
    32727, /* 62 */
    32757, /* 63 */
    32767, /* 64 */
};

int TXSineWave(uint32_t phase){
    int32_t outSample=phase>>16;
    // TODO: 32-bit precision phase
    {
        int32_t outSample2, per;
        // scale to sine table.
        per=outSample&((1<<(16-(TXSineTableSizeShift+2)))-1);
        outSample>>=(16-(TXSineTableSizeShift+2));
        
        assert(outSample<TXSineTableSize*4);
        assert(outSample>=0);
        assert(per>=0);
        assert(per<(1<<(16-(TXSineTableSizeShift+2))));
        
        if(outSample&(2<<TXSineTableSizeShift)){
            assert(outSample>=(2<<TXSineTableSizeShift));
            assert(outSample<(4<<TXSineTableSizeShift));
            
            outSample-=(2<<TXSineTableSizeShift);
            
            // 0 ... pi
            if(outSample&(1<<TXSineTableSizeShift)){
                assert(outSample>=(1<<TXSineTableSizeShift));
                // (1/2)pi ... pi
                outSample=(2<<TXSineTableSizeShift)-outSample;
                outSample2=outSample-1;
            }else{
                assert(outSample<(1<<TXSineTableSizeShift));
                // 0 ... (1/2)pi
                outSample2=outSample+1;
            }
            
            
            
            assert(outSample<=TXSineTableSize);
            outSample=TXSineTable[outSample];
            outSample2=TXSineTable[outSample2];
            outSample+=((outSample2-outSample)*per)>>
            (16-(TXSineTableSizeShift+2));
            outSample=-outSample;
        }else{
            assert(outSample>=0);
            assert(outSample<(2<<TXSineTableSizeShift));
            
            // 0 ... pi
            if(outSample&(1<<TXSineTableSizeShift)){
                assert(outSample>=(1<<TXSineTableSizeShift));
                // (1/2)pi ... pi
                outSample=(2<<TXSineTableSizeShift)-outSample;
                outSample2=outSample-1;
            }else{
                assert(outSample<(1<<TXSineTableSizeShift));
                // 0 ... (1/2)pi
                outSample2=outSample+1;
            }
            
            assert(outSample<=TXSineTableSize);
            outSample=TXSineTable[outSample];
            outSample2=TXSineTable[outSample2];
            outSample+=((outSample2-outSample)*per)>>
            (16-(TXSineTableSizeShift+2));
        }
    }
    return outSample;
}

