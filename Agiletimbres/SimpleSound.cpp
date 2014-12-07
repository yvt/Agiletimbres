//
//  SimpleSound.cpp


#include "StdAfx.h"
#ifndef SS_SDL
#include <windows.h>
#endif
#include <stdio.h>
#ifndef SS_SDL
#include <MmSystem.h>
#else
#include <SDL.h>
#endif
#include "SimpleSound.h"
#include <assert.h>

#ifndef SS_SDL
static HWAVEOUT waveout=NULL;
static WAVEFORMATEX wf;
static WAVEHDR hdrs[2];
static SSDSPProc prc;


static int *waves[2];
static int sampsize;

#else


#endif

static uint32_t lastSynthTime=0;

static volatile bool running=false;

#define _T(s) L ## s

static uint32_t getTicks(){
#ifndef SS_SDL
	return GetTickCount();
#else
	return SDL_GetTicks();
#endif
}

extern "C" void SSSelfTest(){
	SSInitSynth();
	SSStartDSP(SSSynthProc, 4096, 36000);
	SSChunk ch;
	ch=SSCreateChunk(65536);
	short *dt=(short *)SSGetData(ch);
	int n;
	for(n=0;n<32768;n++)
		dt[n]=(n&1)?32767:-32768;
	SSPlayChunk(ch, SSC_LOOPED, 16384, 8192);
#ifdef SS_SDL
	char buf[256];
	gets(buf);
#else
	MessageBox(NULL, _T("Click OK to stop."), NULL, MB_OK);
#endif
	SSStopDSP();
	SSExitSynth();
}

#ifndef SS_SDL

static HANDLE volatile g_synthThread;
static HANDLE volatile g_synthMutex;
static volatile int g_nextId=-1;
static volatile int g_nextId2=-1;

// SharpLib.dll imports.
static HINSTANCE g_sharpLib=LoadLibrary(L"SHARPLIB");
#define DEFFUN(rettype, name, plist) typedef rettype(*name ## _t)plist; \
name ## _t name=(name ## _t)GetProcAddress(g_sharpLib, L ##  # name);
DEFFUN(int, SHDicToolsInit, (HWND));

#pragma mark - Synthesize

static void Prepare(int id, bool unprep=true){
	
	memset(&hdrs[id], 0, sizeof(WAVEHDR));
	hdrs[id].lpData=(LPSTR)waves[id];
	hdrs[id].dwBufferLength=sampsize*4;
	hdrs[id].dwFlags=0;
	waveOutPrepareHeader(waveout, &hdrs[id], sizeof(WAVEHDR));
	
}
static void CALLBACK SynthProc(HWAVEOUT hwo,
							   UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2){
	
	if(uMsg==WOM_DONE){
		if(!running)
			return;
		
		
		
		WAVEHDR *hdr=(WAVEHDR *)dwParam1;
		int id;
		id=((hdr==&hdrs[1])?1:0);/*
								  Prepare(1-id);
								  waveOutWrite(waveout, &hdrs[1-id], sizeof(WAVEHDR));
								  (*prc)((short *)waves[id], sampsize);*/
		
        if(g_nextId==-1){
            g_nextId=id;
        }else if(g_nextId2==-1){
            g_nextId2=id;
        }else{
            //puts("buffer underrun!");
            while(g_nextId2!=-1){
                ReleaseSemaphore(g_synthMutex, 1, NULL);
                Sleep(10);
            }
			g_nextId2=id;
			return;
        }
		
		ReleaseSemaphore(g_synthMutex, 1, NULL);
		
		
	}
}

static void DSPThread(void *){
	CeSetThreadPriority(GetCurrentThread(),
						170);
	//puts("thread start");
	while(running){
		if(WaitForSingleObject(g_synthMutex, 200)==WAIT_TIMEOUT){
            if(running){
                // restart waveout.
                waveOutReset(waveout);
				
				Sleep(50);
                
                MMRESULT res;
                
                Prepare(0, false);
                Prepare(1, false);
                
                memset(waves[0], 0, sampsize*4);
                if((res=waveOutWrite(waveout, &hdrs[0], sizeof(WAVEHDR)))){
                    printf("waveOutWrite returned: 0x%08x\n", (int)res);
                }
                
                memset(waves[1], 0, sampsize*4);
                if((res=waveOutWrite(waveout, &hdrs[1], sizeof(WAVEHDR)))){
                    printf("waveOutWrite returned: 0x%08x\n", (int)res);
                }
            }
        }
		
		if(!running)
			return;
        
        if(g_nextId!=-1){
            int id=g_nextId;
            
            memset(waves[id], 0, sampsize*4);
            (*prc)((short *)waves[id], sampsize);
            
            MMRESULT res;
            if((res=waveOutWrite(waveout, &hdrs[id], sizeof(WAVEHDR)))){
                printf("waveOutWrite returned: 0x%08x\n", (int)res);
            }
            
            g_nextId=-1;
        }
		
        if(g_nextId2!=-1){
            int id=g_nextId2;
            
            memset(waves[id], 0, sampsize*4);
            (*prc)((short *)waves[id], sampsize);
            
            MMRESULT res;
            if((res=waveOutWrite(waveout, &hdrs[id], sizeof(WAVEHDR)))){
                printf("waveOutWrite returned: 0x%08x\n", (int)res);
            }
            
            g_nextId2=-1;
        }
		
	}
	
	//puts("ended");
}

#define WAVE_NOMIXER 0x00000080

#pragma mark - Sleep Management

#define MessageWindowClass L"SimpleSoundMessageWindowClass"
static HWND g_msgWindow=NULL;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    
    if(message==RegisterWindowMessage(L"EdPowerOnOff")){
        if(wParam==0x0001){
            // power off
            
            // save prc.
            SSDSPProc oldPrc=prc;
            if(prc)
                SSStopDSP();
            prc=oldPrc;
        }else if(wParam==0x0002){
            // power on
            if(prc)
                SSStartDSP(prc, sampsize, wf.nSamplesPerSec);
        }
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void createMsgWindow(){
    if(g_msgWindow)
        return;
    WNDCLASS wc;
	
	wc.style			= CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc		= WndProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= 0;
    wc.hInstance		= GetModuleHandle(NULL);
    wc.hIcon			= NULL;
    wc.hCursor			= 0;
    wc.hbrBackground	= (HBRUSH) 4;
    wc.lpszMenuName		= 0;
    wc.lpszClassName	= MessageWindowClass;
	
	RegisterClass(&wc);

    g_msgWindow=CreateWindow(MessageWindowClass, MessageWindowClass, 
                        0, -1, -1, 1, 1,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

}

#pragma mark - Exports

extern "C" void SSStartDSP(SSDSPProc p, int samples, int freq){
	prc=p;
    if(waveout)
        return;
	MMRESULT res;
	wf.cbSize=sizeof(WAVEFORMATEX);
	wf.nSamplesPerSec=freq;
	wf.wBitsPerSample=16;
	wf.nChannels=2;
	
	wf.nBlockAlign=(wf.wBitsPerSample*wf.nChannels)>>3;
	wf.nAvgBytesPerSec=wf.nSamplesPerSec*wf.nBlockAlign;
	
	wf.wFormatTag=WAVE_FORMAT_PCM;
    
    g_synthMutex=NULL;
    
    createMsgWindow();
    if(SHDicToolsInit)
        SHDicToolsInit(g_msgWindow);
    
    // don't use message window for callback.
	res=waveOutOpen(&waveout, 0, &wf, (DWORD)SynthProc, 0, CALLBACK_FUNCTION|
                    WAVE_NOMIXER|WAVE_FORMAT_DIRECT);
    
	if(res){
		TCHAR bf[256];
		waveOutGetErrorText(res, bf, 255);
		MessageBox(GetForegroundWindow(), bf, NULL, MB_ICONERROR);
		return;
	}
	if((res=waveOutReset(waveout))){
		TCHAR bf[256];
		waveOutGetErrorText(res, bf, 255);
		MessageBox(GetForegroundWindow(), bf, NULL, MB_ICONERROR);
		return;
	}

	waves[0]=(int *)GlobalAlloc(GPTR, samples*4);
	waves[1]=(int *)GlobalAlloc(GPTR, samples*4);
	sampsize=samples;
    
	memset(waves[0], 0, sampsize*4);
	//(*prc)((short *)waves[0], sampsize);
   
    
	Prepare(0, false);
    
	
	if((res=waveOutWrite(waveout, &hdrs[0], sizeof(WAVEHDR)))){
		TCHAR bf[256];
		waveOutGetErrorText(res, bf, 255);
		MessageBox(GetForegroundWindow(), bf, NULL, MB_ICONERROR);
		return;
	}
	running=true;
	
	Sleep(10);
    
    memset(waves[1], 0, sampsize*4);
	(*prc)((short *)waves[1], sampsize);
    Prepare(1, false);
	
	if((res=waveOutWrite(waveout, &hdrs[1], sizeof(WAVEHDR)))){
		TCHAR bf[256];
		waveOutGetErrorText(res, bf, 255);
		MessageBox(GetForegroundWindow(), bf, NULL, MB_ICONERROR);
		return;
	}
	
	DWORD threadId;
    
    g_synthMutex=CreateSemaphore(NULL, 0, 1, NULL);
	
	g_synthThread=CreateThread(NULL, 0,
							   (LPTHREAD_START_ROUTINE)DSPThread,
							   NULL,
							   0, 
							   &threadId);
	assert(g_synthThread);
	assert(g_synthMutex);
	
}

extern "C" void SSStopDSP(){
	ReleaseMutex(g_synthMutex);
	running=false;
	waveOutReset(waveout);
	Sleep(100);
	//waveOutUnprepareHeader(waveout, &hdrs[0], sizeof(WAVEHDR));
	//waveOutUnprepareHeader(waveout, &hdrs[1], sizeof(WAVEHDR));
	
	waveOutClose(waveout);
	Sleep(100);
	
	GlobalFree((HGLOBAL)waves[0]);
	GlobalFree((HGLOBAL)waves[1]);
	CloseHandle(g_synthMutex);
	CloseHandle(g_synthThread);
	waveout=NULL;
    prc=NULL;
}

#else

#pragma mark - SDL Port

static void SynthProc(void *userdata, Uint8 *stream, int len){
	SSDSPProc p=(SSDSPProc)userdata;
	(*p)((short *)stream, len/4);
}

extern "C" void SSStartDSP(SSDSPProc p, int samples, int freq){
	SDL_AudioSpec spec;
	spec.format=AUDIO_S16SYS;
	spec.freq=freq;
	spec.channels=2;
	spec.samples=samples;
	spec.callback=SynthProc;
	spec.userdata=(void *)p;
    SDL_InitSubSystem(SDL_INIT_AUDIO);
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
}
extern "C" void SSStopDSP(){
	SDL_CloseAudio();
}

#endif

#pragma mark - SimpleSound Synthesizer

struct Chunk{
	short *wave;
	bool stereo;
	int samples;
	int loopbegin;
	bool is_res;
};

#define max_chunks	256

static Chunk chunks[max_chunks];

struct Channel{
	SSChunk c;
	int pos;
	bool looped;
	bool sounding;
	SSSound id;
	int lvol, rvol;
	int speed;
	int volspeed;
	int delay;
};

#define max_channels 256

static Channel chans[max_channels];
static SSSound next_sound=0xdeadbeef;

extern "C" void SSInitSynth(){
	int n;
	for(n=0;n<max_chunks;n++){
		chunks[n].wave=NULL;
		chunks[n].is_res=false;
	}
	for(n=0;n<max_channels;n++){
		chans[n].sounding=false;
	}
	
}

extern "C" void SSExitSynth(){
	int n;
	for(n=0;n<max_chunks;n++)
		if(chunks[n].wave){
			if(!chunks[n].is_res)
				delete[] chunks[n].wave;
			chunks[n].wave=NULL;
		}
}


static SSChunk FindFreeChunk(){
	int n;
	for(n=0;n<max_chunks;n++)
		if(!chunks[n].wave)
			return (SSChunk)(n);
	return SS_INVALID_CHUNK;
}
#if defined(WIN32) || defined(_WINCE_VER)
extern "C" SSChunk SSLoadChunkFromFileA(LPCSTR str){
	return SSLoadChunkFromFile(str);
}
extern "C" SSChunk SSLoadChunkFromFileW(LPCWSTR str){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;
	FILE *f=_wfopen(str, L"rb");
	if(f==NULL)
		return SS_INVALID_CHUNK;
	int siz;
	fread(&siz, 1, 4, f);
	Chunk ch;
	ch.wave=new short[siz>>1];
	if(ch.wave==NULL){
		fclose(f);
		return SS_INVALID_CHUNK;
	}
	ch.samples=siz>>1;
	ch.stereo=false;
	ch.is_res=false;
	ch.loopbegin=0;
	
	chunks[fre]=ch;
	fread(ch.wave, 2, siz>>1, f);
	fclose(f);
	return fre;
}

extern "C" SSChunk SSLoadChunkFromResource(HINSTANCE inst, LPCTSTR name){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;
	HRSRC hr;
	HGLOBAL gb;
	hr=FindResource(inst, name, _T("WAVE"));
	gb=LoadResource(inst, hr);
	void *ptr;
	ptr=LockResource(gb);
	if(ptr==NULL)
		return SS_INVALID_CHUNK;
	int siz;
	siz=*((int *)ptr);
	
	
	Chunk ch;
	ch.wave=(short *)ptr+2;
	ch.samples=(siz)>>1;
	ch.stereo=false;
	ch.is_res=true;
	ch.loopbegin=0;
	
	chunks[fre]=ch;
	return fre;
}
extern "C" SSChunk SSLoadChunkFromResourceRaw(HINSTANCE inst, LPCTSTR name, int size){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;
	HRSRC hr;
	HGLOBAL gb;
	hr=FindResource(inst, name, _T("WAVE"));
	gb=LoadResource(inst, hr);
	void *ptr;
	ptr=LockResource(gb);
	if(ptr==NULL)
		return SS_INVALID_CHUNK;
	int siz;
	siz=size;
	
	
	Chunk ch;
	ch.wave=(short *)ptr;
	ch.samples=(siz)>>1;
	ch.stereo=false;
	ch.is_res=true;
	ch.loopbegin=0;
	
	chunks[fre]=ch;
	return fre;
}
#endif
extern "C" SSChunk SSLoadChunkStaticData(const void *ptr){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;

	if(ptr==NULL)
		return SS_INVALID_CHUNK;
	int siz;
	siz=*((int *)ptr);
	
	
	Chunk ch;
	ch.wave=(short *)ptr+2;
	ch.samples=(siz)>>1;
	ch.stereo=false;
	ch.is_res=true;
	ch.loopbegin=0;
	
	chunks[fre]=ch;
	return fre;
}
extern "C" SSChunk SSLoadChunkFromFile(const char *str){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;
	FILE *f=fopen(str, "rb");
	if(f==NULL)
		return SS_INVALID_CHUNK;
	int siz;
	fread(&siz, 1, 4, f);
	assert(siz<(1<<22));
	Chunk ch;
	ch.wave=new short[siz>>1];
	if(ch.wave==NULL){
		fclose(f);
		return SS_INVALID_CHUNK;
	}
	ch.samples=siz>>1;
	ch.stereo=false;
	ch.is_res=false;
	ch.loopbegin=0;
	
	chunks[fre]=ch;
	fread(ch.wave, 2, siz>>1, f);
	fclose(f);
	return fre;
}

extern "C" SSChunk SSCreateChunk(int bytes){
	SSChunk fre=FindFreeChunk();
	if(fre==SS_INVALID_CHUNK)
		return SS_INVALID_CHUNK;
	Chunk ch;
	ch.wave=new short[bytes>>1];
	ch.samples=bytes>>1;
	ch.stereo=false;
	ch.is_res=false;
	chunks[fre]=ch;
	return fre;
}
extern "C" void *SSGetData(SSChunk ck){
	if(ck==SS_INVALID_CHUNK)
		return NULL;
	return chunks[ck].wave;
}
extern "C" void SSMakeChunkStereo(SSChunk ck){
	if(ck==SS_INVALID_CHUNK)
		return;
	Chunk *ch=&chunks[ck];
	if(ch->stereo)
		return;
	ch->stereo=true;
	ch->samples>>=1;
}
extern "C" void SSLoopRange(SSChunk ck, int begin, int end){
	if(ck==SS_INVALID_CHUNK)
		return;
	Chunk *ch=&chunks[ck];
	ch->loopbegin=begin;
	ch->samples=end;
}
static int FindFreeChannel(){
	int n;
	for(n=0;n<max_channels;n++)
		if(!chans[n].sounding)
			return n;
	return -1;
}

extern "C" SSSound SSPlayChunk(SSChunk ck, SSChunkFlags flg, 
							   SSVolume left, SSVolume right){
	return SSPlayChunk2(ck, flg, left, right);
}
extern "C" SSSound SSPlayChunk2(SSChunk ck, SSChunkFlags flg, 
								SSVolume left, SSVolume right,
								SSSpeed speed){
	if(ck==SS_INVALID_CHUNK)
		return SS_INVALID_SOUND;
	int chan=FindFreeChannel();
	if(chan==-1)
		return SS_INVALID_SOUND;
	Channel& ch=chans[chan];
	ch.c=ck;
	ch.id=next_sound;
	next_sound++;
	ch.looped=(flg&SSC_LOOPED);
	ch.lvol=left;
	ch.pos=0;
	ch.rvol=right;
	ch.speed=speed;
	ch.sounding=true;
	ch.volspeed=0;
	ch.delay=0;
	if(!ch.looped){
		int delay;
		delay=getTicks()-lastSynthTime;
		ch.delay=delay*22;
	}
	
	return ch.id;
}

extern "C" void SSStopSound(SSSound snd){
	int n;
	for(n=0;n<max_channels;n++)
		if(chans[n].id==snd)
			chans[n].sounding=false;
}
extern "C" void SSStopChunk(SSChunk c){
	int n;
	for(n=0;n<max_channels;n++)
		if(chans[n].c==c)
			chans[n].sounding=false;
}
extern "C" void SSFadeSound(SSSound snd, SSVolume speed){
	int n;
	for(n=0;n<max_channels;n++)
		if(chans[n].id==snd)
			chans[n].volspeed=speed;
}
extern "C" void SSFadeChunk(SSChunk c, SSVolume speed){
	int n;
	for(n=0;n<max_channels;n++)
		if(chans[n].c==c)
			chans[n].volspeed=speed;
}
extern "C" int SSIsChunkPlaying(SSChunk c){
	int n, cnt=0;
	for(n=0;n<max_channels;n++)
		if(chans[n].c==c && chans[n].sounding)
			cnt++;
	return cnt;
}
extern "C" int SSIsSoundPlaying(SSSound snd){
	int n, cnt=0;
	for(n=0;n<max_channels;n++)
		if(chans[n].id==snd && chans[n].sounding)
			cnt++;
	return cnt;
}
extern "C" void SSStopAll(){
	int n;
	for(n=0;n<max_channels;n++)
		chans[n].sounding=false;
}

static int synth_buf[65536];

extern "C" void SSSynthProc(short *samp, int samples){
	memset(synth_buf, 0, samples*8);
	lastSynthTime=getTicks();
	int n;
	register int i;
	for(n=0;n<max_channels;n++){
		Channel& ch=chans[n];
		if(ch.sounding==false)
			continue;
		Chunk cnk=chunks[ch.c];
		register short *dat=cnk.wave;
		register int pos=ch.pos;
		register int cnt=cnk.samples;
		register int vol1=ch.lvol;
		register int vol2=ch.rvol;
		
		int startPos=0;
		if(ch.delay){
			startPos=ch.delay;
			if(startPos>samples){
				startPos=samples;
			}
			ch.delay-=startPos;
			if(ch.delay>0)
				continue;
		}
		
		if(ch.speed!=SSS_DEFAULT && cnt<500000){
			// non44100hz speed
			register int speed=ch.speed;
			cnt<<=12;
			cnk.loopbegin<<=12;
			pos<<=4;
			if(ch.looped){
				
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[((pos>>12)<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[((pos>>12)<<1)+1]*vol2)>>14;
						pos+=speed;
						while(pos>=cnt)
							pos=pos-cnt+cnk.loopbegin;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos>>12]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos>>12]*vol2)>>14;
						pos+=speed;
						while(pos>=cnt)
							pos=pos-cnt+cnk.loopbegin;
					}
				}
			}else{
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[((pos>>12)<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[((pos>>12)<<1)+1]*vol2)>>14;
						pos+=speed;
						if(pos>=cnt)
							break;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos>>12]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos>>12]*vol2)>>14;
						pos+=speed;
						if(pos>=cnt)
							break;
					}
				}
				if(pos>=cnt){
					chans[n].sounding=false;
					continue;
				}
			}
			cnk.loopbegin>>=12;
			pos>>=4;
		}else if(ch.speed!=SSS_DEFAULT){
			// non44100hz speed, but too long
			register int speed=ch.speed;
			cnt<<=8;
			cnk.loopbegin<<=8;
			speed>>=4;
			if(ch.looped){
				
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[((pos>>8)<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[((pos>>8)<<1)+1]*vol2)>>14;
						pos+=speed;
						while(pos>=cnt)
							pos=pos-cnt+cnk.loopbegin;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos>>8]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos>>8]*vol2)>>14;
						pos+=speed;
						while(pos>=cnt)
							pos=pos-cnt+cnk.loopbegin;
					}
				}
			}else{
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[((pos>>8)<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[((pos>>8)<<1)+1]*vol2)>>14;
						pos+=speed;
						if(pos>=cnt)
							break;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos>>8]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos>>8]*vol2)>>14;
						pos+=speed;
						if(pos>=cnt)
							break;
					}
				}
				if(pos>=cnt){
					chans[n].sounding=false;
					continue;
				}
			}
			cnk.loopbegin>>=12;
			
		}else{
			// 44100hz - original speeed
			pos>>=8;
			if(ch.looped){
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[(pos<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[(pos<<1)+1]*vol2)>>14;
						pos++;
						if(pos==cnt)
							pos=cnk.loopbegin;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos]*vol2)>>14;
						pos++;
						if(pos==cnt)
							pos=cnk.loopbegin;
					}
				}
			}else{
				if(cnk.stereo){
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[(pos<<1)+0]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[(pos<<1)+1]*vol2)>>14;
						pos++;
						if(pos==cnt)
							break;
					}
				}else{
					for(i=startPos;i<samples;i++){
						synth_buf[(i<<1)+0]+=((int)dat[pos]*vol1)>>14;
						synth_buf[(i<<1)+1]+=((int)dat[pos]*vol2)>>14;
						pos++;
						if(pos==cnt)
							break;
					}
				}
				if(pos==cnt){
					chans[n].sounding=false;
					continue;
				}
			}
			pos<<=8;
		}
		ch.lvol-=(ch.volspeed>>1)*(samples>>1)/11025;
		ch.rvol-=(ch.volspeed>>1)*(samples>>1)/11025;
		if(ch.lvol<0)
			ch.lvol=0;
		if(ch.rvol<0)
			ch.rvol=0;
		if(ch.lvol==0 && ch.rvol==0)
			ch.sounding=false;
		chans[n].pos=pos;
		
	}
	n=samples<<1;
	for(i=0;i<n;i++){
		int dt=synth_buf[i];
		if(dt<-32768)
			dt=-32768;
		if(dt>32767)
			dt=32767;
		samp[i]=dt;
	}
}
