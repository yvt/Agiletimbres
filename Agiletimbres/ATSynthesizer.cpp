//
//  ATSynthesizer.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/11/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "ATSynthesizer.h"
#include "SimpleSound.h"
#include "TXMatrixReverb1.h"
#include "TXChorusFilter.h"
#include "TXSRC.h"
#include "TXBiquadFilter.h"
#include "NHIntegerFFT.h"
#include "TXFFTFilter.h"

#define AT_SAMPLE_RATE  44100


#ifdef WIN32
// for alloca
#include <malloc.h>

// enable iram
#define AT_ENABLE_IRAM  0

#else
// for getttimeofday
#include <sys/time.h>
#endif


#if AT_ENABLE_IRAM
static HINSTANCE g_sharpLib=LoadLibrary(L"SHARPLIB");
#define DEFFUN(rettype, name, plist) typedef rettype(*name ## _t)plist; \
name ## _t name=(name ## _t)GetProcAddress(g_sharpLib, L ##  # name);
DEFFUN(void *, EdMmMapIoSpace, (unsigned long, unsigned long, int));
static int32_t *g_iRam=(int32_t *)EdMmMapIoSpace(0xf8004000,
                                      0xc000, TRUE);
#endif

ATSynthesizer *ATSynthesizer::sharedSynthesizer(){
    static ATSynthesizer *synth=NULL;
    if(!synth)
        synth=new ATSynthesizer();
    return synth;
}

struct ATDCRemover{
	int m_state[2];
	ATDCRemover(){
		m_state[0]=m_state[1]=0;
	}
	
	void applyStereoInplace(int32_t *buffer, unsigned int samples){
		register int state1=m_state[0];
		register int state2=m_state[1];
		
		while(samples--){
			register int tmp;
			
			tmp=*buffer;
			// update LPF.
			state1=(state1<<8)-state1; // state1*=255
			state1+=tmp;
			state1>>=8;
			
			*(buffer++)=tmp-state1;
			
			tmp=*buffer;
			// update LPF.
			state2=(state2<<8)-state2; // state1*=255
			state2+=tmp;
			state2>>=8;
			
			*(buffer++)=tmp-state2;
			
			
		}
		m_state[0]=state1;
		m_state[1]=state2;
	}
};

static TXInstrument * volatile g_currentInstrument=NULL;
static twSDLSemaphore *g_synthesizerSemaphore;  // only while preprocessing
static twSDLSemaphore *g_renderSemaphore;       // rendering, too
static twTicks volatile g_lastRenderTime;
static unsigned int volatile g_lastDelayTime=0;
static float volatile g_lastProcessorLoad=0.f;
static short g_wave[1024];
static TXEffect *g_reverb;
static TXEffect *g_chorus;
static bool g_reverbEnable=true;
static bool g_chorusEnable=true;
static bool g_mute=false;

static ATDCRemover g_dcRemover;

static TXSRC::CICBinaryInterpolator<2, 2, 2> g_interpol[2];
static TXSRC::CICBinaryDecimator<2, 2, 2> g_decim[2];

#ifdef WIN32
static uint64_t g_lastProcessorTime=0;
static DWORD g_lastTime=GetTickCount();

static uint64_t processorTimeMicroseconds(){
    FILETIME ft, a, b, c;
    GetThreadTimes(GetCurrentThread(),
                   &a, &b, &c,
                   &ft);
    uint64_t ft2=*(uint64_t *)&ft;
    return ft2/10;

}

#else

static uint64_t realTimeMicroseconds(){

    struct timeval vl;
    gettimeofday(&vl, NULL);
    
    uint64_t t=vl.tv_usec;
    t+=(uint64_t)vl.tv_sec*1000000ULL;
    return t;

}
#endif

static unsigned int requiredDelayTime(){
    twTicks delayMilliseoncds=twGetTicks()-g_lastRenderTime;
    if((int)delayMilliseoncds<0)
       delayMilliseoncds=0;
    
    unsigned int delayTime=(unsigned int)(delayMilliseoncds*441/10);
    
    if(delayTime<g_lastDelayTime)
        delayTime=g_lastDelayTime;
    g_lastDelayTime=delayTime;
    
    return delayTime;
}

static void ATSynthesizerProc(short *out, int samples){
    ATSynthesizer *synth=ATSynthesizer::sharedSynthesizer();
    twTicks ot=twGetTicks();
    synth->render(out, samples);
    ot=twGetTicks()-ot;
    /*
    float renderTime=(float)ot/1000.f;
    float realTime=(float)samples/44100.f;
    
    printf("CPU-Load: %f%c\n", renderTime/realTime*100.f, '%');
    */
}

ATSynthesizer::ATSynthesizer(){
    g_synthesizerSemaphore=new twSDLSemaphore();
    g_renderSemaphore=new twSDLSemaphore();
    
    g_reverb=new TXMatrixReverb1(config());
    g_chorus=new TXChorusFilter(config());
    g_currentInstrument=NULL;
	
    
#if AT_EMBEDDED
    SSStartDSP(ATSynthesizerProc, 1024, AT_SAMPLE_RATE);
#else
    SSStartDSP(ATSynthesizerProc, 256, AT_SAMPLE_RATE);
#endif
}

void ATSynthesizer::setInstrument(TXInstrument *inst){
    twLock lock(g_renderSemaphore);
    g_currentInstrument=inst;
}

TXInstrument *ATSynthesizer::instrument(){
    return g_currentInstrument;
}

twSemaphore *ATSynthesizer::renderSemaphore(){
    return g_renderSemaphore;
}
twSemaphore *ATSynthesizer::synthesizerSemaphore(){
    return g_synthesizerSemaphore;
}

void ATSynthesizer::note(int key, int velocity, int duration){
    if(!g_currentInstrument)
        return;
    twLock lock(g_synthesizerSemaphore);
    g_currentInstrument->noteOn(key, velocity,
                                requiredDelayTime());
    g_currentInstrument->noteOff(key, velocity,
                                requiredDelayTime()+duration*441/10);
    
}

void ATSynthesizer::noteOn(int key, int velocity){
    if(!g_currentInstrument)
        return;
    twLock lock(g_synthesizerSemaphore);
    g_currentInstrument->noteOn(key, velocity,
                                requiredDelayTime());

}
void ATSynthesizer::noteOff(int key, int velocity){
    if(!g_currentInstrument)
        return;
    twLock lock(g_synthesizerSemaphore);
    g_currentInstrument->noteOff(key, velocity,
                                requiredDelayTime());
    
}
void ATSynthesizer::setPitchbend(int millicents){
    if(!g_currentInstrument)
        return;
    twLock lock(g_synthesizerSemaphore);
    g_currentInstrument->setPitchbend(millicents,
                                requiredDelayTime());
}

void ATSynthesizer::render(int16_t *outBuffer,
                           unsigned int samples){

    
#ifdef AT_ENABLE_IRAM
    bool useIRam=false;
#endif
    int32_t *renderBuffer;
    
#ifndef WIN32
    uint64_t startTime=realTimeMicroseconds();
#endif
    
#if AT_ENABLE_IRAM
    if(samples*8<=0xc000){
        renderBuffer=g_iRam;
    }else
#endif
    {
        renderBuffer=(int32_t *)alloca(samples*8);
    }
    
    memset(renderBuffer, 0,
           samples*8);
	
	if(!g_mute){
    
		if(g_currentInstrument){
			twLock lock(g_renderSemaphore);
			
			
			
			
			for(int i=0;i<4;i++){
				{
					twLock lock(g_synthesizerSemaphore);
					g_currentInstrument->preprocess(samples/4);
					
					g_lastRenderTime=twGetTicks();
					if(g_lastDelayTime<samples/4)
						g_lastDelayTime=0;
					else
						g_lastDelayTime-=samples/4;
				}
				g_currentInstrument->renderAdditive(renderBuffer+(samples*i/4)*2,
													samples/4);
			}
			/*
			g_currentInstrument->preprocess(samples);
			g_currentInstrument->renderAdditive(renderBuffer,
												samples);*/
			
		}
		if(g_chorusEnable){
			twLock lock(g_renderSemaphore);
			g_chorus->applyStereo(renderBuffer,
										  renderBuffer,
										  samples);
		}
		if(g_reverbEnable){
			twLock lock(g_renderSemaphore);
			g_reverb->applyStereo(renderBuffer,
										  renderBuffer, samples);
		}
		
	} // (!g_mute)
	
	
	g_dcRemover.applyStereoInplace(renderBuffer, samples);
	/*
	int32_t *rb2=(int32_t *)alloca(samples*4);
	int32_t *rb3=(int32_t *)alloca(samples*4);
	for(int i=0;i<samples;i++){
		rb2[i]=renderBuffer[i*2]<<8;
		rb3[i]=renderBuffer[i*2+1]<<8;
	}
	assert(samples==256);
	for(int i=0;i<16;i++){
	NHIntegerFFTForward<16>(8, rb2, rb3);

		NHIntegerFFTBackward<16>(8, rb2, rb3);}
	for(int i=0;i<samples;i++){
		renderBuffer[i*2]=rb2[i]>>8;
		renderBuffer[i*2+1]=rb3[i]>>8;
	}*/
	
    /*
	int32_t *renderBuffer2;
	renderBuffer2=(int32_t *)alloca((samples<<2)*8);
	g_interpol[0].process(renderBuffer2, renderBuffer, samples);
	g_interpol[1].process(renderBuffer2+1, renderBuffer+1, samples);
	g_decim[0].process(renderBuffer, renderBuffer2, samples);
	g_decim[1].process(renderBuffer+1, renderBuffer2+1, samples);*/
        
    // clip.
    unsigned int samples2=samples*2;
    for(unsigned int i=0;i<samples2;i++){
        
        int32_t v=renderBuffer[i];
        
        if(v<-32768)
            v=-32768;
        if(v>32767)
            v=32767;
        
        /*
        v>>=1;
        
        if(v<-32768)
            v=-32768;
        if(v>32767)
            v=32767;
        
        if(v>0){
            v=32767-v;
            v=(v*v)>>15;
            v=32767-v;
        }else{
            v=v+32768;
            v=(v*v)>>15;
            v=v-32768;
        }
        */
        int16_t v16=(int16_t)v;
        outBuffer[i]=v16;
        
    }
    
#ifdef WIN32
    
    if((GetTickCount()-g_lastTime)>500){
        uint64_t takenTime=processorTimeMicroseconds()-g_lastProcessorTime;
        uint64_t realTime=(GetTickCount()-g_lastTime)*1000;
        g_lastProcessorLoad=(float)takenTime*100.f/(float)realTime;
        if(g_lastProcessorLoad>100.f)
            g_lastProcessorLoad=100.f;
        
        g_lastTime=GetTickCount();
        g_lastProcessorTime=processorTimeMicroseconds();
    }
    
#else
    
    unsigned int takenTime=(unsigned int)
    (realTimeMicroseconds()-startTime);
    
    unsigned int realAudioTime=
    (unsigned int)((uint64_t)samples*1000000ULL
                   /(uint64_t)AT_SAMPLE_RATE);
    
    
    g_lastProcessorLoad=(float)takenTime*100.f/(float)realAudioTime;
    if(g_lastProcessorLoad>100.f)
        g_lastProcessorLoad=100.f;
#endif
}

TXConfig ATSynthesizer::config(){
    TXConfig cfg;
    cfg.sampleRate=AT_SAMPLE_RATE;
    return cfg;
}

float ATSynthesizer::processorLoad(){
    return g_lastProcessorLoad;
}

short *ATSynthesizer::waveform(){
  return g_wave;
}

TXEffect *ATSynthesizer::reverb(){
    return g_reverb;
}

TXEffect *ATSynthesizer::chorus(){
    return g_chorus;
}

void ATSynthesizer::setChorus(TXEffect *fx){
	twLock lock(g_renderSemaphore);
	g_chorus=fx;
}

void ATSynthesizer::setReverbEnable(bool enb){
    g_reverbEnable=enb;
}

bool ATSynthesizer::reverbEnable(){
    return g_reverbEnable;
}

void ATSynthesizer::setChorusEnable(bool enb){
    g_chorusEnable=enb;
}

bool ATSynthesizer::chorusEnable(){
    return g_chorusEnable;
}


void ATSynthesizer::setMute(bool m){
	g_mute=m;
}