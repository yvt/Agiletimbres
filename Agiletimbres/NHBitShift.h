//
//  NHBitShift.h
//  NHBoost
//
//  Created by Tomoaki Kawada on 4/10/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "NHBoost.h"

template <typename T, int bits>
void NHBitShiftLeftInplace(T *buffer, unsigned int elements){
	while(elements--){
		*(buffer++)<<=bits;
	}
}

template <typename T, int bits>
void NHBitShiftRightInplace(T *buffer, unsigned int elements){
	while(elements--){
		*(buffer++)>>=bits;
	}
}

template <typename T>
void NHBitShiftLeftInplace(T *buffer, unsigned int elements,
						   unsigned int bits){
	switch(bits){
		case 0: return;
		case 1: NHBitShiftLeftInplace<T,1>(buffer, elements); break;
		case 2: NHBitShiftLeftInplace<T,2>(buffer, elements); break;
		case 3: NHBitShiftLeftInplace<T,3>(buffer, elements); break;
		case 4: NHBitShiftLeftInplace<T,4>(buffer, elements); break;
		case 5: NHBitShiftLeftInplace<T,5>(buffer, elements); break;
		case 6: NHBitShiftLeftInplace<T,6>(buffer, elements); break;
		case 7: NHBitShiftLeftInplace<T,7>(buffer, elements); break;
		case 8: NHBitShiftLeftInplace<T,8>(buffer, elements); break;
		case 9: NHBitShiftLeftInplace<T,9>(buffer, elements); break;
		case 10: NHBitShiftLeftInplace<T,10>(buffer, elements); break;
		case 11: NHBitShiftLeftInplace<T,11>(buffer, elements); break;
		case 12: NHBitShiftLeftInplace<T,12>(buffer, elements); break;
		case 13: NHBitShiftLeftInplace<T,13>(buffer, elements); break;
		case 14: NHBitShiftLeftInplace<T,14>(buffer, elements); break;
		case 15: NHBitShiftLeftInplace<T,15>(buffer, elements); break;
		case 16: NHBitShiftLeftInplace<T,16>(buffer, elements); break;
		case 17: NHBitShiftLeftInplace<T,17>(buffer, elements); break;
		case 18: NHBitShiftLeftInplace<T,18>(buffer, elements); break;
		case 19: NHBitShiftLeftInplace<T,19>(buffer, elements); break;
		case 20: NHBitShiftLeftInplace<T,20>(buffer, elements); break;
		case 21: NHBitShiftLeftInplace<T,21>(buffer, elements); break;
		case 22: NHBitShiftLeftInplace<T,22>(buffer, elements); break;
		case 23: NHBitShiftLeftInplace<T,23>(buffer, elements); break;
		case 24: NHBitShiftLeftInplace<T,24>(buffer, elements); break;
		case 25: NHBitShiftLeftInplace<T,25>(buffer, elements); break;
		case 26: NHBitShiftLeftInplace<T,26>(buffer, elements); break;
		case 27: NHBitShiftLeftInplace<T,27>(buffer, elements); break;
		case 28: NHBitShiftLeftInplace<T,28>(buffer, elements); break;
		case 29: NHBitShiftLeftInplace<T,29>(buffer, elements); break;
		case 30: NHBitShiftLeftInplace<T,30>(buffer, elements); break;
		case 31: NHBitShiftLeftInplace<T,31>(buffer, elements); break;
	}
}

template <typename T>
void NHBitShiftRightInplace(T *buffer, unsigned int elements,
						   unsigned int bits){
	switch(bits){
		case 0: return;
		case 1: NHBitShiftRightInplace<T,1>(buffer, elements); break;
		case 2: NHBitShiftRightInplace<T,2>(buffer, elements); break;
		case 3: NHBitShiftRightInplace<T,3>(buffer, elements); break;
		case 4: NHBitShiftRightInplace<T,4>(buffer, elements); break;
		case 5: NHBitShiftRightInplace<T,5>(buffer, elements); break;
		case 6: NHBitShiftRightInplace<T,6>(buffer, elements); break;
		case 7: NHBitShiftRightInplace<T,7>(buffer, elements); break;
		case 8: NHBitShiftRightInplace<T,8>(buffer, elements); break;
		case 9: NHBitShiftRightInplace<T,9>(buffer, elements); break;
		case 10: NHBitShiftRightInplace<T,10>(buffer, elements); break;
		case 11: NHBitShiftRightInplace<T,11>(buffer, elements); break;
		case 12: NHBitShiftRightInplace<T,12>(buffer, elements); break;
		case 13: NHBitShiftRightInplace<T,13>(buffer, elements); break;
		case 14: NHBitShiftRightInplace<T,14>(buffer, elements); break;
		case 15: NHBitShiftRightInplace<T,15>(buffer, elements); break;
		case 16: NHBitShiftRightInplace<T,16>(buffer, elements); break;
		case 17: NHBitShiftRightInplace<T,17>(buffer, elements); break;
		case 18: NHBitShiftRightInplace<T,18>(buffer, elements); break;
		case 19: NHBitShiftRightInplace<T,19>(buffer, elements); break;
		case 20: NHBitShiftRightInplace<T,20>(buffer, elements); break;
		case 21: NHBitShiftRightInplace<T,21>(buffer, elements); break;
		case 22: NHBitShiftRightInplace<T,22>(buffer, elements); break;
		case 23: NHBitShiftRightInplace<T,23>(buffer, elements); break;
		case 24: NHBitShiftRightInplace<T,24>(buffer, elements); break;
		case 25: NHBitShiftRightInplace<T,25>(buffer, elements); break;
		case 26: NHBitShiftRightInplace<T,26>(buffer, elements); break;
		case 27: NHBitShiftRightInplace<T,27>(buffer, elements); break;
		case 28: NHBitShiftRightInplace<T,28>(buffer, elements); break;
		case 29: NHBitShiftRightInplace<T,29>(buffer, elements); break;
		case 30: NHBitShiftRightInplace<T,30>(buffer, elements); break;
		case 31: NHBitShiftRightInplace<T,31>(buffer, elements); break;
	}
}


