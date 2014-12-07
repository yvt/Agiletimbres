//
//  TXGlobal.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/12/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXGlobal.h"

void TXAddVectorInplace(int32_t *outBuffer,
                        const int32_t *srcBuffer,
                        unsigned int elements){
    while(elements--){
        *(outBuffer++)+=*(srcBuffer++);
    }
}

float TXSqrtApprox(float z)
{
    union
    {
        int tmp;
        float f;
    } u;
    
    u.f = z;
    
    /*
     * To justify the following code, prove that
     *
     * ((((val_int / 2^m) - b) / 2) + b) * 2^m = ((val_int - 2^m) / 2) + ((b + 1) / 2) * 2^m)
     *
     * where
     *
     * val_int = u.tmp
     * b = exponent bias
     * m = number of mantissa bits
     *
     * .
     */
    
    u.tmp -= 1 << 23; /* Subtract 2^m. */
    u.tmp >>= 1; /* Divide by 2. */
    u.tmp += 1 << 29; /* Add ((b + 1) / 2) * 2^m. */
    
    return u.f;
}
