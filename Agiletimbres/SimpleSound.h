#ifndef _SSOUND_H
#define _SSOUND_H
#if defined(WIN32) || defined(_WINCE_VER)
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif
	
	typedef void (*SSDSPProc)(short *, int samples);
	
	void SSStartDSP(SSDSPProc, int samples, int freq=44100);
	void SSStopDSP();
	
	typedef int SSChunk;
	typedef int SSVolume;
	typedef int SSChunkFlags;
	typedef int SSSound;
	typedef int SSSpeed;
	
	void SSSynthProc(short *, int);
	
	void SSInitSynth();
	void SSExitSynth();
	
#define SS_INVALID_CHUNK	((SSChunk)(-1))
#define SS_INVALID_SOUND	((SSSound)(-1))
#define SS_MAX_VOL			((SSVolume)16384)
#define SSC_DEFAULT			((SSChunkFlags)0x00000000)
#define SSC_LOOPED			((SSChunkFlags)0x00000001)
#define SSS_DEFAULT			((SSSpeed)0x1000)
	
	SSChunk SSLoadChunkFromFile(const char *);
#if defined(WIN32) || defined(_WINCE_VER)
	SSChunk SSLoadChunkFromFileA(LPCSTR);
	SSChunk SSLoadChunkFromFileW(LPCWSTR);
	SSChunk SSLoadChunkFromResource(HINSTANCE, LPCTSTR);
	SSChunk SSLoadChunkFromResourceRaw(HINSTANCE, LPCTSTR, int);
#endif
	SSChunk SSLoadChunkStaticData(const void *);
	SSChunk SSCreateChunk(int bytes);
	void SSMakeChunkStereo(SSChunk);
	void SSLoopRange(SSChunk, int, int);
	void *SSGetData(SSChunk);
	
	SSSound SSPlayChunk(SSChunk, SSChunkFlags flg=SSC_DEFAULT, 
						SSVolume left=SS_MAX_VOL, SSVolume right=SS_MAX_VOL);
	
	SSSound SSPlayChunk2(SSChunk, SSChunkFlags flg=SSC_DEFAULT, 
						 SSVolume left=SS_MAX_VOL, SSVolume right=SS_MAX_VOL,
						 SSSpeed speed=SSS_DEFAULT);
	
	void SSStopSound(SSSound);
	void SSStopChunk(SSChunk);
	void SSFadeSound(SSSound, SSVolume /* (per sec) */);
	void SSFadeChunk(SSChunk, SSVolume /* (per sec) */);
	void SSStopAll();
	
	int SSIsChunkPlaying(SSChunk);
	int SSIsSoundPlaying(SSSound);
	
	void SSSelfTest();
	
#ifdef __cplusplus
};
#endif

#endif