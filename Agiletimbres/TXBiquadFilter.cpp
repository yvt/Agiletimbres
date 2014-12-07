//
//  TXBiquadFilter.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/17/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "TXBiquadFilter.h"

TXBiquadFilter::TXBiquadFilter(){
    m_coefs[0]=0xffffffff;
    m_coefs[1]=0;
    m_coefs[2]=0;
    m_coefs[3]=0;
    m_coefs[4]=0;
    m_shift=0;
    
    m_leftHistories[0]=0;
    m_leftHistories[1]=0;
    m_leftHistories[2]=0;
    m_leftHistories[3]=0;
    m_rightHistories[0]=0;
    m_rightHistories[1]=0;
    m_rightHistories[2]=0;
    m_rightHistories[3]=0;
}

TXBiquadFilter::TXBiquadFilter(int b0, int b1, int b2,
                               int a1, int a2, int shift){
    m_coefs[0]=b0;
    m_coefs[1]=b1;
    m_coefs[2]=b2;
    m_coefs[3]=a1;
    m_coefs[4]=a2;
    m_shift=shift;
    
    m_leftHistories[0]=0;
    m_leftHistories[1]=0;
    m_leftHistories[2]=0;
    m_leftHistories[3]=0;
    m_rightHistories[0]=0;
    m_rightHistories[1]=0;
    m_rightHistories[2]=0;
    m_rightHistories[3]=0;
    
}

TXBiquadFilter::~TXBiquadFilter(){
    m_leftHistories[0]=0;
    m_leftHistories[1]=0;
    m_leftHistories[2]=0;
    m_leftHistories[3]=0;
    m_rightHistories[0]=0;
    m_rightHistories[1]=0;
    m_rightHistories[2]=0;
    m_rightHistories[3]=0;
}

void TXBiquadFilter::mute(){
    
}

#pragma mark - Apply Mono

void TXBiquadFilter::applyMonoInplace
(int32_t *outBuffer, unsigned int samples){
    register int hist0=m_histories[0];
    register int hist1=m_histories[1];
    register int hist2=m_histories[2];
    register int hist3=m_histories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        int input=*outBuffer;
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
    }
    
    m_histories[0]=hist0;
    m_histories[1]=hist1;
    m_histories[2]=hist2;
    m_histories[3]=hist3;
    
    
}

void TXBiquadFilter::applyMonoOutplace
(int32_t *outBuffer, const int32_t *inBuffer,
 unsigned int samples){
    register int hist0=m_histories[0];
    register int hist1=m_histories[1];
    register int hist2=m_histories[2];
    register int hist3=m_histories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=1<<m_shift;
    
    while(samples--){
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
    }
    
    m_histories[0]=hist0;
    m_histories[1]=hist1;
    m_histories[2]=hist2;
    m_histories[3]=hist3;
    
}

void TXBiquadFilter::applyMonoAdditive
(int32_t *outBuffer, const int32_t *inBuffer,
 unsigned int samples){
    register int hist0=m_histories[0];
    register int hist1=m_histories[1];
    register int hist2=m_histories[2];
    register int hist3=m_histories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)+=hist2;
    }
    
    m_histories[0]=hist0;
    m_histories[1]=hist1;
    m_histories[2]=hist2;
    m_histories[3]=hist3;
}

#pragma mark - Stereo

void TXBiquadFilter::applyStereoLeftInplace
(int32_t *outBuffer, unsigned int samples){
    register int hist0=m_leftHistories[0];
    register int hist1=m_leftHistories[1];
    register int hist2=m_leftHistories[2];
    register int hist3=m_leftHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        int input=*outBuffer;
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
        outBuffer++; // skip right channel
    }
    
    m_leftHistories[0]=hist0;
    m_leftHistories[1]=hist1;
    m_leftHistories[2]=hist2;
    m_leftHistories[3]=hist3;
    
    
}

void TXBiquadFilter::applyStereoLeftOutplace
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    register int hist0=m_leftHistories[0];
    register int hist1=m_leftHistories[1];
    register int hist2=m_leftHistories[2];
    register int hist3=m_leftHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
        outBuffer++; // skip right channel
        inBuffer++; // skip right channel
    }
    
    m_leftHistories[0]=hist0;
    m_leftHistories[1]=hist1;
    m_leftHistories[2]=hist2;
    m_leftHistories[3]=hist3;
    
    
}

void TXBiquadFilter::applyStereoLeftAdditive
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    register int hist0=m_leftHistories[0];
    register int hist1=m_leftHistories[1];
    register int hist2=m_leftHistories[2];
    register int hist3=m_leftHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)+=hist2;
        outBuffer++; // skip right channel
        inBuffer++; // skip right channel
    }
    
    m_leftHistories[0]=hist0;
    m_leftHistories[1]=hist1;
    m_leftHistories[2]=hist2;
    m_leftHistories[3]=hist3;
    
    
}



void TXBiquadFilter::applyStereoRightInplace
(int32_t *outBuffer, unsigned int samples){
    register int hist0=m_rightHistories[0];
    register int hist1=m_rightHistories[1];
    register int hist2=m_rightHistories[2];
    register int hist3=m_rightHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        
        outBuffer++; // skip left channel
        
        int input=*outBuffer;
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
    }
    
    m_rightHistories[0]=hist0;
    m_rightHistories[1]=hist1;
    m_rightHistories[2]=hist2;
    m_rightHistories[3]=hist3;
    
    
}

void TXBiquadFilter::applyStereoRightOutplace
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    register int hist0=m_rightHistories[0];
    register int hist1=m_rightHistories[1];
    register int hist2=m_rightHistories[2];
    register int hist3=m_rightHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        
        outBuffer++; // skip left channel
        inBuffer++; // skip left channel
        
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)=hist2;
    }
    
    m_rightHistories[0]=hist0;
    m_rightHistories[1]=hist1;
    m_rightHistories[2]=hist2;
    m_rightHistories[3]=hist3;
    
    
}

void TXBiquadFilter::applyStereoRightAdditive
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    register int hist0=m_rightHistories[0];
    register int hist1=m_rightHistories[1];
    register int hist2=m_rightHistories[2];
    register int hist3=m_rightHistories[3];
    register int coef0=m_coefs[0];
    register int coef1=m_coefs[1];
    register int coef2=m_coefs[2];
    register int coef3=m_coefs[3];
    register int coef4=m_coefs[4];
    int shift=m_shift;
    
    while(samples--){
        
        outBuffer++; // skip left channel
        inBuffer++; // skip left channel
        
        int input=*(inBuffer++);
        int64_t temp;
        
        temp=(int64_t)input*(int64_t)coef0;
        temp+=(int64_t)hist0*(int64_t)coef1;
        temp+=(int64_t)hist1*(int64_t)coef2;
        temp+=(int64_t)hist2*(int64_t)coef3;
        temp+=(int64_t)hist3*(int64_t)coef4;
        
        hist1=hist0;
        hist0=input;
        hist3=hist2;
        hist2=((int32_t)((temp<<shift)>>32));
        
        *(outBuffer++)+=hist2;
    }
    
    m_rightHistories[0]=hist0;
    m_rightHistories[1]=hist1;
    m_rightHistories[2]=hist2;
    m_rightHistories[3]=hist3;
    
    
}

void TXBiquadFilter::applyStereoInplace
(int32_t *outBuffer, unsigned int samples){
    applyStereoLeftInplace(outBuffer, samples);
    applyStereoRightInplace(outBuffer, samples);
}

void TXBiquadFilter::applyStereoOutplace
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    applyStereoLeftOutplace(outBuffer, inBuffer, samples);
    applyStereoRightOutplace(outBuffer, inBuffer, samples);
}

void TXBiquadFilter::applyStereoAdditive
(int32_t *outBuffer, const int32_t *inBuffer, unsigned int samples){
    applyStereoLeftAdditive(outBuffer, inBuffer, samples);
    applyStereoRightAdditive(outBuffer, inBuffer, samples);
}

#pragma mark - Design

static const int32_t cordic_circular_gain = 0xb2458939;
static const uint32_t atan_table[] = {
	0x1fffffff, /* +0.785398163 (or pi/4) */
	0x12e4051d, /* +0.463647609 */
	0x09fb385b, /* +0.244978663 */
	0x051111d4, /* +0.124354995 */
	0x028b0d43, /* +0.062418810 */
	0x0145d7e1, /* +0.031239833 */
	0x00a2f61e, /* +0.015623729 */
	0x00517c55, /* +0.007812341 */
	0x0028be53, /* +0.003906230 */
	0x00145f2e, /* +0.001953123 */
	0x000a2f98, /* +0.000976562 */
	0x000517cc, /* +0.000488281 */
	0x00028be6, /* +0.000244141 */
	0x000145f3, /* +0.000122070 */
	0x0000a2f9, /* +0.000061035 */
	0x0000517c, /* +0.000030518 */
	0x000028be, /* +0.000015259 */
	0x0000145f, /* +0.000007629 */
	0x00000a2f, /* +0.000003815 */
	0x00000517, /* +0.000001907 */
	0x0000028b, /* +0.000000954 */
	0x00000145, /* +0.000000477 */
	0x000000a2, /* +0.000000238 */
	0x00000051, /* +0.000000119 */
	0x00000028, /* +0.000000060 */
	0x00000014, /* +0.000000030 */
	0x0000000a, /* +0.000000015 */
	0x00000005, /* +0.000000007 */
	0x00000002, /* +0.000000004 */
	0x00000001, /* +0.000000002 */
	0x00000000, /* +0.000000001 */
	0x00000000, /* +0.000000000 */
};


#define FRACMUL(x, y) (int32_t) (((((int64_t) (x)) * ((int64_t) (y))) >> 31))
#define FRACMUL_SHL(x, y, z) \
((int32_t)(((((int64_t) (x)) * ((int64_t) (y))) >> (31 - (z)))))
#define DIV64(x, y, z) (int32_t)(((int64_t)(x) << (z))/(y))

static int32_t fsincos(uint32_t phase, int32_t *cos) {
    int32_t x, x1, y, y1;
    uint32_t z, z1;
    int i;
    
    /* Setup initial vector */
    x = cordic_circular_gain;
    y = 0;
    z = phase;
    
    /* The phase has to be somewhere between 0..pi for this to work right */
    if (z < 0xffffffff / 4) {
        /* z in first quadrant, z += pi/2 to correct */
        x = -x;
        z += 0xffffffff / 4;
    } else if (z < 3 * (0xffffffff / 4)) {
        /* z in third quadrant, z -= pi/2 to correct */
        z -= 0xffffffff / 4;
    } else {
        /* z in fourth quadrant, z -= 3pi/2 to correct */
        x = -x;
        z -= 3 * (0xffffffff / 4);
    }
    
    /* Each iteration adds roughly 1-bit of extra precision */
    for (i = 0; i < 31; i++) {
        x1 = x >> i;
        y1 = y >> i;
        z1 = atan_table[i];
        
        /* Decided which direction to rotate vector. Pivot point is pi/2 */
        if (z >= 0xffffffff / 4) {
            x -= y1;
            y += x1;
            z -= z1;
        } else {
            x += y1;
            y -= x1;
            z += z1;
        }
    }
    
    *cos = x;
    
    return y;
}


static int integerGain(float gain){
    if(gain>127.f)
        gain=127.f;
    if(gain<0.f)
        gain=0.f;
    return (int)(gain*16777216.0f);
}

static uint32_t integerCutoff(float sampleRate,
                              float cutoffFreq){
    float nyquistFreq=sampleRate*.5f;
    if(cutoffFreq>=nyquistFreq)
        cutoffFreq=nyquistFreq;
    
    return (uint32_t)(cutoffFreq*(float)0x80000000UL
                      /nyquistFreq);
}

static void filter_shelf_coefs(unsigned long cutoff, float A, bool low, int32_t *c){
    int32_t sin, cos;
    int32_t b0, b1, a0, a1; /* s3.28 */
    const long g = integerGain(TXSqrtApprox(A)) << 4; /* 10^(db/40), s3.28 */
    
    sin = fsincos(cutoff/2, &cos);
    if (low) {
        const int32_t sin_div_g = DIV64(sin, g, 25);
        cos >>= 3;
        b0 = FRACMUL(sin, g) + cos;   /* 0.25 .. 4.10 */
        b1 = FRACMUL(sin, g) - cos;   /* -1 .. 3.98 */
        a0 = sin_div_g + cos;         /* 0.25 .. 4.10 */
        a1 = sin_div_g - cos;         /* -1 .. 3.98 */
    } else {
        const int32_t cos_div_g = DIV64(cos, g, 25);
        sin >>= 3;
        b0 = sin + FRACMUL(cos, g);   /* 0.25 .. 4.10 */
        b1 = sin - FRACMUL(cos, g);   /* -3.98 .. 1 */
        a0 = sin + cos_div_g;         /* 0.25 .. 4.10 */
        a1 = sin - cos_div_g;         /* -3.98 .. 1 */
    }
    
    const int32_t rcp_a0 = DIV64(1, a0, 57); /* 0.24 .. 3.98, s2.29 */
    *c++ = FRACMUL_SHL(b0, rcp_a0, 1);       /* 0.063 .. 15.85 */
    *c++ = FRACMUL_SHL(b1, rcp_a0, 1);       /* -15.85 .. 15.85 */
    *c++ = -FRACMUL_SHL(a1, rcp_a0, 1);      /* -1 .. 1 */
}

TXBiquadFilter TXBiquadFilter::biShelfFilter
(float sampleRate,
 float cutoffLow,
 float cutoffHigh,
 float gainLow,
 float gainHigh,
 float masterGain){
    const long g = integerGain(masterGain) << 7; /* 10^(db/20), s0.31 */
    int32_t c_ls[3], c_hs[3];
    filter_shelf_coefs(integerCutoff(sampleRate, cutoffLow), 
                       gainLow, true, c_ls);
    filter_shelf_coefs(integerCutoff(sampleRate, cutoffHigh), 
                       gainHigh, false, c_hs);
    c_ls[0] = FRACMUL(g, c_ls[0]);
    c_ls[1] = FRACMUL(g, c_ls[1]);
    
    /* now we cascade the two first order filters to one second order filter
     * which can be used by eq_filter(). these resulting coefficients have a
     * really wide numerical range, so we use a fixed point format which will
     * work for the selected cutoff frequencies (in dsp.c) only.
     */
    const int32_t b0 = c_ls[0], b1 = c_ls[1], b2 = c_hs[0], b3 = c_hs[1];
    const int32_t a0 = c_ls[2], a1 = c_hs[2];
    
    return TXBiquadFilter(FRACMUL_SHL(b0, b2, 4),
                          FRACMUL_SHL(b0, b3, 4) + FRACMUL_SHL(b1, b2, 4),
                          FRACMUL_SHL(b1, b3, 4),
                          a0 + a1,
                          -FRACMUL_SHL(a0, a1, 4),
                          5);
}

TXBiquadFilter TXBiquadFilter::lowShelfFilter
(float sampleRate,
 float cutoffFreq,
 float Q,
 float gain){
    uint32_t cutoff;
    cutoff=integerCutoff(sampleRate, cutoffFreq);
    
    int Qi=(int)(Q*10.f);
    gain=TXSqrtApprox(gain);
    gain=TXSqrtApprox(gain);
    
    int32_t cs;
    const int32_t one = 1 << 25; /* s6.25 */
    const int32_t sqrtA = integerGain(gain) << 2; /* 10^(db/80), s5.26 */
    const int32_t A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const int32_t alpha = fsincos(cutoff, &cs)/(2*Qi)*10 >> 1; /* s1.30 */
    const int32_t ap1 = (A >> 4) + one;
    const int32_t am1 = (A >> 4) - one;
    const int32_t twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;
    
    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 - FRACMUL(am1, cs) + twosqrtalpha, 2);
    /* [-16 .. 63.4] */
    b1 = FRACMUL_SHL(A, am1 - FRACMUL(ap1, cs), 3);
    /* [0 .. 31.7] */
    b2 = FRACMUL_SHL(A, ap1 - FRACMUL(am1, cs) - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 + FRACMUL(am1, cs) + twosqrtalpha;
    /* [-16 .. 4] */
    a1 = -2*((am1 + FRACMUL(ap1, cs)));
    /* [0 .. 8] */
    a2 = ap1 + FRACMUL(am1, cs) - twosqrtalpha;
    
    /* [0.1 .. 1.99] */
    const int32_t rcp_a0 = DIV64(1, a0, 55);    /* s1.30 */
    
    return TXBiquadFilter(FRACMUL_SHL(b0, rcp_a0, 2),
                          FRACMUL_SHL(b1, rcp_a0, 2),
                          FRACMUL_SHL(b2, rcp_a0, 2),
                          FRACMUL_SHL(-a1, rcp_a0, 2),
                          FRACMUL_SHL(-a2, rcp_a0, 2),
                          6);
}

TXBiquadFilter TXBiquadFilter::highShelfFilter
(float sampleRate,
 float cutoffFreq,
 float Q,
 float gain){
    uint32_t cutoff;
    cutoff=integerCutoff(sampleRate, cutoffFreq);
    
    int Qi=(int)(Q*10.f);
    gain=TXSqrtApprox(gain);
    gain=TXSqrtApprox(gain);
    
    int32_t cs;
    const int32_t one = 1 << 25; /* s6.25 */
    const int32_t sqrtA = integerGain(gain) << 2; /* 10^(db/80), s5.26 */
    const int32_t A = FRACMUL_SHL(sqrtA, sqrtA, 8); /* s2.29 */
    const int32_t alpha = fsincos(cutoff, &cs)/(2*Qi)*10 >> 1; /* s1.30 */
    const int32_t ap1 = (A >> 4) + one;
    const int32_t am1 = (A >> 4) - one;
    const int32_t twosqrtalpha = 2*FRACMUL(sqrtA, alpha);
    int32_t a0, a1, a2; /* these are all s6.25 format */
    int32_t b0, b1, b2;
    
    /* [0.1 .. 40] */
    b0 = FRACMUL_SHL(A, ap1 + FRACMUL(am1, cs) + twosqrtalpha, 2);
    /* [-63.5 .. 16] */
    b1 = -FRACMUL_SHL(A, am1 + FRACMUL(ap1, cs), 3);
    /* [0 .. 32] */
    b2 = FRACMUL_SHL(A, ap1 + FRACMUL(am1, cs) - twosqrtalpha, 2);
    /* [0.5 .. 10] */
    a0 = ap1 - FRACMUL(am1, cs) + twosqrtalpha;
    /* [-4 .. 16] */
    a1 = 2*((am1 - FRACMUL(ap1, cs)));
    /* [0 .. 8] */
    a2 = ap1 - FRACMUL(am1, cs) - twosqrtalpha;
    
    /* [0.1 .. 1.99] */
    const int32_t rcp_a0 = DIV64(1, a0, 55);    /* s1.30 */
    return TXBiquadFilter(FRACMUL_SHL(b0, rcp_a0, 2),
                          FRACMUL_SHL(b1, rcp_a0, 2),
                          FRACMUL_SHL(b2, rcp_a0, 2),
                          FRACMUL_SHL(-a1, rcp_a0, 2),
                          FRACMUL_SHL(-a2, rcp_a0, 2),
                          6);
    
}

TXBiquadFilter TXBiquadFilter::peakFilter
(float sampleRate,
 float cutoffFreq,
 float Q,
 float gain){
    uint32_t cutoff;
    cutoff=integerCutoff(sampleRate, cutoffFreq);
    
    int Qi=(int)(Q*10.f);
    gain=TXSqrtApprox(gain);
    
    int32_t cs;
    const int32_t one = 1 << 28; /* s3.28 */
    const int32_t A = integerGain(gain) << 5; /* 10^(db/40), s2.29 */
    const int32_t alpha = fsincos(cutoff, &cs)/(2*Qi)*10 >> 1; /* s1.30 */
    int32_t a0, a1, a2; /* these are all s3.28 format */
    int32_t b0, b1, b2;
    const int32_t alphadivA = DIV64(alpha, A, 27);
    
    /* possible numerical ranges are in comments by each coef */
    b0 = one + FRACMUL(alpha, A);     /* [1 .. 5] */
    b1 = a1 = -2*(cs >> 3);           /* [-2 .. 2] */
    b2 = one - FRACMUL(alpha, A);     /* [-3 .. 1] */
    a0 = one + alphadivA;             /* [1 .. 5] */
    a2 = one - alphadivA;             /* [-3 .. 1] */
    
    /* range of this is roughly [0.2 .. 1], but we'll never hit 1 completely */
    const int32_t rcp_a0 = DIV64(1, a0, 59); /* s0.31 */

    return TXBiquadFilter(FRACMUL(b0, rcp_a0),
                          FRACMUL(b1, rcp_a0),
                          FRACMUL(b2, rcp_a0),
                          FRACMUL(-a1, rcp_a0),
                          FRACMUL(-a2, rcp_a0),
                          4);
}

TXBiquadFilter TXBiquadFilter::allpassFilter
(float sampleRate,
 float cutoffFreq,
 float Q){
    uint32_t cutoff;
    cutoff=integerCutoff(sampleRate, cutoffFreq);
    
    int Qi=(int)(Q*10.f);
    
    int32_t cs;
    const int32_t one = 1 << 28; /* s3.28 */
    const int32_t A = 0x1000000UL << 5; /* 10^(db/40), s2.29 */
    const int32_t alpha = fsincos(cutoff, &cs)/(2*Qi)*10 >> 1; /* s1.30 */
    int32_t a0, a1, a2; /* these are all s3.28 format */
    int32_t b0, b1, b2;
    const int32_t alphadivA = DIV64(alpha, A, 27);
    
    /* possible numerical ranges are in comments by each coef */
    b0 = one - FRACMUL(alpha, A);     /* [1 .. 5] */
    b1 = a1 = -2*(cs >> 3);           /* [-2 .. 2] */
    b2 = one + FRACMUL(alpha, A);     /* [-3 .. 1] */
    a0 = one + alphadivA;             /* [1 .. 5] */
    a2 = one - alphadivA;             /* [-3 .. 1] */
    
    /* range of this is roughly [0.2 .. 1], but we'll never hit 1 completely */
    const int32_t rcp_a0 = DIV64(1, a0, 59); /* s0.31 */
    
    return TXBiquadFilter(FRACMUL(b0, rcp_a0),
                          FRACMUL(b1, rcp_a0),
                          FRACMUL(b2, rcp_a0),
                          FRACMUL(-a1, rcp_a0),
                          FRACMUL(-a2, rcp_a0),
                          4);
}

