//
//  TXSRC.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 4/1/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once

#include "TXGlobal.h"

namespace TXSRC{
	
	class IntInterpolator{
	public:
		virtual void process(int32_t *output,
					 const int32_t *input,
					 size_t inSamples)=0;
		virtual ~IntInterpolator(){}
	};
	
	class IntDecimator{
	public:
		virtual void process(int32_t *output,
							 const int32_t *input,
							 size_t outSamples)=0;
		virtual ~IntDecimator(){}
	};

	// convert nHz -> n*(2^Power)Hz. 
	template<int Power, int OutputStride, int InputStride>
	class CICBinaryInterpolator: public IntInterpolator{
		enum{
			N=1<<Power
		};
		int accum[2];
		int comb[2];
	public:
		void mute(){
			for(int i=0;i<2;i++) accum[i]=0;
			for(int i=0;i<2;i++) comb[i]=0;
		}
		CICBinaryInterpolator(){
			mute();
		}
		virtual void process(int32_t *output,
					 const int32_t *input,
					 size_t inSamples){
			register int accum1=accum[0], accum2=accum[1];
			register int comb1=comb[0], comb2=comb[1];
			
			while(inSamples--){
				// do comb filter.
				register int tmp=*(input)<<8, tmp2;
				input+=InputStride;
				
				// comb stage 1
				tmp2=tmp-comb1; comb1=tmp;
				// comb stage 2
				tmp=tmp2-comb2; comb2=tmp2;
				
				// upsampling loop
				for(int i=0;i<N;i++){
					
					// integrator stage 1
					accum1+=tmp;
					// integrator stage 2
					accum2+=accum1>>Power;
					
					*(output)=(accum2>>Power)>>8;
					output+=OutputStride;
					
					accum1-=(accum1+(1<<8))>>9;
					accum2-=(accum2+(1<<8))>>9;
					
				}
				
				
				
			}
			
			accum[0]=accum1; accum[1]=accum2;
			comb[0]=comb1; comb[1]=comb2;
		}
	};
	
	// convert nHz <- n*(2^Power)Hz. 
	template<int Power, int OutputStride, int InputStride>
	class CICBinaryDecimator: public IntDecimator{
		enum{
			N=1<<Power
		};
		int accum[2];
		int comb[2];
	public:
		void mute(){
			for(int i=0;i<2;i++) accum[i]=0;
			for(int i=0;i<2;i++) comb[i]=0;
		}
		CICBinaryDecimator(){
			mute();
		}
		virtual void process(int32_t *output,
					 const int32_t *input,
					 size_t outSamples){
			register int accum1=accum[0], accum2=accum[1];
			register int comb1=comb[0], comb2=comb[1];
			
			while(outSamples--){
				
				
				// do comb filter.
				register int tmp, tmp2;
				
				tmp=accum2>>Power;
				
				// comb stage 1
				tmp2=tmp-comb1; comb1=tmp;
				// comb stage 2
				//tmp2>>=Power;
				tmp=tmp2-comb2; comb2=tmp2;
				
				*(output)=(tmp);
				output+=OutputStride;
				
				// downsampling loop
				for(int i=0;i<N;i++){
					
					tmp=*(input);
					input+=InputStride;
					
					// integrator stage 1
					accum1+=tmp;
					// integrator stage 2
					accum2+=accum1>>Power;
					
					
					accum1-=(accum1+(1<<7))>>8;
					accum2-=(accum2+(1<<7))>>8;
					
				}
				
				
			}
			
			accum[0]=accum1; accum[1]=accum2;
			comb[0]=comb1; comb[1]=comb2;
		}
	};

	
};
