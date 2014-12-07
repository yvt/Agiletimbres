//
//  NHIntegerFFT.h
//  NHBoost
//
//  Created by Tomoaki Kawada on 4/10/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "NHBoost.h"
#include "NHBitShift.h"

extern uint32_t NHIntegerFFTRotationTable[][2];
#define _NHIFMultiply(a, b) ((int32_t)(((int64_t)(a)*(int64_t)(b))>>FractionBits))
#define _NFIFScale(a, scale) ((int32_t)(((int64_t)(a)*(uint64_t)(scale))>>32))

template<bool IsForward, int FractionBits, int phaseFlg>
void NHIntegerFFTPhase(unsigned int bits, int32_t *x, int32_t *y,
					   unsigned int phase, 
					   unsigned int l1, unsigned int l2,
					   unsigned int nn){
	uint32_t c1=NHIntegerFFTRotationTable[phase][0];
	uint32_t c2=NHIntegerFFTRotationTable[phase][1];
	
	int32_t u1=(1<<FractionBits), u2=0;
	
	for(unsigned int j=0;j<l1;j++){
		for(unsigned int i=j;i<nn;i+=l2){
			unsigned int i1=i+l1;
			int32_t t1=_NHIFMultiply(u1, x[i1]);
			int32_t t2=_NHIFMultiply(u1, y[i1]);
			t1-=_NHIFMultiply(u2, y[i1]);
			t2+=_NHIFMultiply(u2, x[i1]);
			x[i1]=x[i]-t1;
			y[i1]=y[i]-t2;
			x[i]+=t1;
			y[i]+=t2;
		}
		
		if(phaseFlg==0){
			// c1=-1.0, c2=0
			u2=-u2; u1=-u1;
		}else if(phaseFlg==1){
			// c1=0, c2=(IsForward?-1:1)
			if(IsForward){
				int32_t z=u2;
				u2=-u1; u1=z;
			}else{
				int32_t z=-u2;
				u2=u1; u1=z;
			}
		}else{
			if(IsForward){
				// negated c2
				int32_t z=_NFIFScale(u1, c1);
				z+=_NFIFScale(u2, c2);
				u2=_NFIFScale(u2, c1);
				u2-=_NFIFScale(u1, c2);
				u1=z;
			}else{
				int32_t z=_NFIFScale(u1, c1);
				z-=_NFIFScale(u2, c2);
				u2=_NFIFScale(u2, c1);
				u2+=_NFIFScale(u1, c2);
				u1=z;
			}
		}
	}
	
	
	
}


template<bool IsForward, int FractionBits, int phaseFlg>
void NHIntegerFFTPackedPhase(unsigned int bits, int32_t *x,
					   unsigned int phase, 
					   unsigned int l1, unsigned int l2,
					   unsigned int nn){
	uint32_t c1=NHIntegerFFTRotationTable[phase][0];
	uint32_t c2=NHIntegerFFTRotationTable[phase][1];
	
	int32_t u1=(1<<FractionBits), u2=0;
	
	for(unsigned int j=0;j<l1;j++){
		for(unsigned int i=j;i<nn;i+=l2){
			unsigned int i1=i+l1;
			int32_t t1=_NHIFMultiply(u1, x[i1*2]);
			int32_t t2=_NHIFMultiply(u1, x[i1*2+1]);
			t1-=_NHIFMultiply(u2, x[i1*2+1]);
			t2+=_NHIFMultiply(u2, x[i1*2]);
			x[i1*2]=x[i*2]-t1;
			x[i1*2+1]=x[i*2+1]-t2;
			x[i*2]+=t1;
			x[i*2+1]+=t2;
		}
		
		if(phaseFlg==0){
			// c1=-1.0, c2=0
			u2=-u2; u1=-u1;
		}else if(phaseFlg==1){
			// c1=0, c2=(IsForward?-1:1)
			if(IsForward){
				int32_t z=u2;
				u2=-u1; u1=z;
			}else{
				int32_t z=-u2;
				u2=u1; u1=z;
			}
		}else{
			if(IsForward){
				// negated c2
				int32_t z=_NFIFScale(u1, c1);
				z+=_NFIFScale(u2, c2);
				u2=_NFIFScale(u2, c1);
				u2-=_NFIFScale(u1, c2);
				u1=z;
			}else{
				int32_t z=_NFIFScale(u1, c1);
				z-=_NFIFScale(u2, c2);
				u2=_NFIFScale(u2, c1);
				u2+=_NFIFScale(u1, c2);
				u1=z;
			}
		}
	}
	
	
	
}

template<bool IsForward, int FractionBits>
void NHIntegerFFT(unsigned int bits, int32_t *x, int32_t *y){
	unsigned int nn=1<<bits;
	
	// do the bit reversal.
	{
		unsigned int j=0;
		for(unsigned int i=0;i<nn-1;i++){
			if(i<j){
				int32_t tx=x[i], ty=y[i];
				x[i]=x[j]; y[i]=y[j];
				x[j]=tx; y[j]=ty;
			}
			unsigned int k=nn>>1;
			while(k<=j){
				j-=k; k>>=1;
			}
			j+=k;
		}
	}
	
	// compute FFT.
	{
		unsigned int l1, l2;
		l2=1;
		
		l1=l2; l2<<=1;
		NHIntegerFFTPhase<IsForward, FractionBits, 0>(bits, x, y,
													  0, l1, l2, 
													  nn);
		if(bits>1){
			l1=l2; l2<<=1;
			NHIntegerFFTPhase<IsForward, FractionBits, 1>(bits, x, y,
														  1, l1, l2, 
														  nn);
		}
		
		for(unsigned int l=2;l<bits;l++){
			l1=l2; l2<<=1;
			NHIntegerFFTPhase<IsForward, FractionBits, 2>(bits, x, y,
														  l, l1, l2, 
														  nn);
		}
	}
	
}


template<bool IsForward, int FractionBits>
void NHIntegerFFTPacked(unsigned int bits, int32_t *x){
	unsigned int nn=1<<bits;
	
	// do the bit reversal.
	{
		unsigned int j=0;
		for(unsigned int i=0;i<nn-1;i++){
			if(i<j){
				int32_t tx=x[i*2], ty=x[i*2+1];
				x[i*2]=x[j*2]; x[i*2+1]=x[j*2+1];
				x[j*2]=tx; x[j*2+1]=ty;
			}
			unsigned int k=nn>>1;
			while(k<=j){
				j-=k; k>>=1;
			}
			j+=k;
		}
	}
	
	// compute FFT.
	{
		unsigned int l1, l2;
		l2=1;
		
		l1=l2; l2<<=1;
		NHIntegerFFTPackedPhase<IsForward, FractionBits, 0>(bits, x,
													  0, l1, l2, 
													  nn);
		if(bits>1){
			l1=l2; l2<<=1;
			NHIntegerFFTPackedPhase<IsForward, FractionBits, 1>(bits, x,
														  1, l1, l2, 
														  nn);
		}
		
		for(unsigned int l=2;l<bits;l++){
			l1=l2; l2<<=1;
			NHIntegerFFTPackedPhase<IsForward, FractionBits, 2>(bits, x,
														  l, l1, l2, 
														  nn);
		}
	}
	
}


template<int FractionBits>
void NHIntegerFFTForward(unsigned int bits, int32_t *x, int32_t *y){
	NHIntegerFFT<true, FractionBits>(bits, x, y);
}

template<int FractionBits>
void NHIntegerFFTBackward(unsigned int bits, int32_t *x, int32_t *y){
	NHIntegerFFT<false, FractionBits>(bits, x, y);
}


template<int FractionBits>
void NHIntegerFFTPackedForward(unsigned int bits, int32_t *x){
	NHIntegerFFTPacked<true, FractionBits>(bits, x);
}

template<int FractionBits>
void NHIntegerFFTPackedBackward(unsigned int bits, int32_t *x){
	NHIntegerFFTPacked<false, FractionBits>(bits, x);
}

// in real = N points.
// out complex = (N/2)+1 points.
template<int FractionBits>
void NHIntegerRealFFTForward(unsigned int bits, int32_t *inReal,
							 int32_t *outX, int32_t *outY){
	NHIntegerFFTPackedForward<FractionBits>(bits-1, inReal);
	
	int size=1<<bits;
	int halfSize=size>>1;
	outX[0]=(inReal[0]+inReal[1])>>1;
	outY[0]=0;
	outX[halfSize]=(inReal[0]-inReal[1])>>1;
	outY[halfSize]=0;
	
	uint32_t c1=NHIntegerFFTRotationTable[bits-1][0];
	uint32_t c2=NHIntegerFFTRotationTable[bits-1][1];
	
	int32_t u1=0;
	int32_t u2=-(1<<FractionBits);
	
	int quatSize=halfSize>>1;
	
	for(int k=1;k<=quatSize;k++){
		
		int32_t z=_NFIFScale(u1, c1);
		z+=_NFIFScale(u2, c2);
		u2=_NFIFScale(u2, c1);
		u2-=_NFIFScale(u1, c2);
		u1=z;
		
		int kx=inReal[k*2]>>1, ky=inReal[k*2+1]>>1;
		int nx=inReal[(halfSize-k)*2]>>1, ny=-(inReal[(halfSize-k)*2+1]>>1);
		int f1x=kx+nx, f1y=ky+ny;
		int f2x=kx-nx, f2y=ky-ny;
		int tx=_NHIFMultiply(f2x, u1)-_NHIFMultiply(f2y, u2);
		int ty=_NHIFMultiply(f2x, u2)+_NHIFMultiply(f2y, u1);
		
		outX[k]=(f1x+tx)>>1;
		outY[k]=(f1y+ty)>>1;
		outX[halfSize-k]=(f1x-tx)>>1;
		outY[halfSize-k]=(ty-f1y)>>1;
	}
}

// in real = N points.
// out complex = (N/2)+1 points.
template<int FractionBits>
void NHIntegerRealFFTBackward(unsigned int bits, int32_t *outReal,
							 const int32_t *inX, const int32_t *inY){
	int size=1<<bits;
	int halfSize=size>>1;
	
	outReal[0]=(inX[0]+inX[halfSize])>>1;
	outReal[1]=(inX[0]-inX[halfSize])>>1;
	
	uint32_t c1=NHIntegerFFTRotationTable[bits-1][0];
	uint32_t c2=NHIntegerFFTRotationTable[bits-1][1];
	
	int32_t u1=0;
	int32_t u2=(1<<FractionBits);
	
	int quatSize=halfSize>>1;
	
	for(int k=1;k<=quatSize;k++){
		
		int32_t z=_NFIFScale(u1, c1);
		z-=_NFIFScale(u2, c2);
		u2=_NFIFScale(u2, c1);
		u2+=_NFIFScale(u1, c2);
		u1=z;
		
		int32_t fx=inX[k]>>1;
		int32_t fy=inY[k]>>1;
		int32_t nx=inX[halfSize-k]>>1;
		int32_t ny=-(inY[halfSize-k]>>1);
		
		int32_t ex=fx+nx;
		int32_t ey=fy+ny;
		int32_t mx=fx-nx;
		int32_t my=fy-ny;
		
		int ox=_NHIFMultiply(mx, u1)-_NHIFMultiply(my, u2);
		int oy=_NHIFMultiply(mx, u2)+_NHIFMultiply(my, u1);
		
		
		outReal[k*2]=ex+ox;
		outReal[k*2+1]=ey+oy;
		outReal[(halfSize-k)*2]=ex-ox;
		outReal[(halfSize-k)*2+1]=oy-ey;
		
	}
	
	NHIntegerFFTPackedBackward<FractionBits>(bits-1, outReal);
	
}

#undef _NHIFMultiply
#undef _NFIFScale


