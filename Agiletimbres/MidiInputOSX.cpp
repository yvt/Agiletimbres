//
//  MidiInputOSX.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 12/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "MidiInputOSX.h"

#if USE_MIDIINPUT_OSX


#include <CoreMIDI/MIDIServices.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include "ATSynthesizer.h"
#include "TXInstrument.h"

#define MAX_EXBUF 20
#define BUFF_SIZE 512


typedef uint32_t uint32;
typedef uint8_t uint8;

static unsigned int InNum;

#define PMBUFF_SIZE 8192
#define EXBUFF_SIZE 512
static char    sysexbuffer[EXBUFF_SIZE];

/*********************************************************************/
#define TmStreamBufNum 102400
typedef struct {
	uint32  current;
	uint32  next;
	uint8   buf[TmStreamBufNum];
}TmStream;

TmStream  tmstream;
pthread_mutex_t tmstream_mutex = PTHREAD_MUTEX_INITIALIZER;


static void TmQueue( int port, const MIDIPacket *p)
{
	int  used;
	int  room;
	
	
	pthread_mutex_lock(&tmstream_mutex);
	
	used = tmstream.next - tmstream.current;
	if( used <0 ){ used+=TmStreamBufNum; }
	room = TmStreamBufNum - used;
	
	if(p->length>=256){
		printf("too big packet\n"); // and do nothing
		
	}else if(room < p->length +3){
		printf("full buffer\n"); //and do nothing
		fflush(stdout);
	}else{  //go!
		int i;
		
		tmstream.buf[tmstream.next] = port; 
		tmstream.next++; tmstream.next %= TmStreamBufNum;
		
		tmstream.buf[tmstream.next] = p->length;
		tmstream.next++; tmstream.next %= TmStreamBufNum;
		
		for( i=0; i<p->length; i++ ){
			tmstream.buf[tmstream.next] = p->data[i];
			tmstream.next++; tmstream.next %= TmStreamBufNum;
		}
	}
	
	pthread_mutex_unlock(&tmstream_mutex);
}

static int TmDequeue( int *portp, uint8 buf[256])
{
	int     used;
	int     room;
	int     len=0, i;
	
	pthread_mutex_lock(&tmstream_mutex);
	
	used = tmstream.next - tmstream.current;
	if( used <0 ){ used+=TmStreamBufNum; }
	room = TmStreamBufNum - used;
	
	if( used == 0 ){
		len = 0;
		goto finally;
	}
	
	*portp = tmstream.buf[tmstream.current];
	tmstream.current++; tmstream.current %= TmStreamBufNum;
	
	len = tmstream.buf[tmstream.current];
	tmstream.current++; tmstream.current %= TmStreamBufNum;
	
	for( i=0; i<len; i++){
		buf[i] = tmstream.buf[tmstream.current];
		tmstream.current++; tmstream.current %= TmStreamBufNum;
	}
	
finally:
	pthread_mutex_unlock(&tmstream_mutex);
	return len;
}


template <int device>
static void	MyReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
	unsigned int j;
	
	MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	// remove const (!)
	for ( j = 0; j < pktlist->numPackets; ++j) {
		
		TmQueue( device, packet );
		packet = MIDIPacketNext(packet);
	}
	
}



static inline uint32 ev2message( uint32 len, uint8 buf[])
{
	switch(len){
			
		case 0:
			return 0;
			
		case 1:
			return buf[0];
			
		case 2:
			return buf[0]+(buf[1]<<8);
			break;
			
		case 3:
			return buf[0]+(buf[1]<<8)+(buf[2]<<16);
			break;
			
		default:
			return buf[0];
	}
}

static ATSynthesizer *synthesizer(){
    return ATSynthesizer::sharedSynthesizer();
}

static int rtsyn_play_one_data(int dev, int32_t msg){
	if((msg&0xff)==0xf0)
		return 1;
	unsigned char status, data1, data2, ch;
	status=msg&0xff;
	data1=(msg>>8)&0xff;
	data2=(msg>>16)&0xff;
	ch=status&0xf;
	ch+=dev<<4;
	switch(status>>4){
		case 0x9:
			if(data2==0)
				goto isoff;
            synthesizer()->noteOn(data1, data2);
			break;
		case 0x8:
		isoff:
            synthesizer()->noteOff(data1, 0);
			break;
		case 0xa:
			// after touch
			break;
		case 0xb:
			//sfx::midi->control(ch, data1, data2);
			break;
		case 0xc:
			//sfx::midi->program(ch, data1);
			break;
		case 0xe:
            int val;
            val=(((long)data1)+(((long)data2)<<7))-8192;
            synthesizer()->setPitchbend(val);
			break;
		case 0xf:
			if(ch==0xf){
                synthesizer()->instrument()->allNotesOff(0);
			}
			break;
	}
	return 0;
}
static int rtsyn_play_one_sysex(uint8 *buf, int len){
	//printf("sysex %s\n", len);
	//sfx::midi->sysex(buf, len);
	return 0;
}
static int rtsyn_play_some_data (void)
{
	int played;	
	int port=0;
	long pmlength;
	uint8  buf[256];
	
	
	played=0;
	do{
		//for(port=0;port<rtsyn_portnumber;port++){
		
		do{
			pmlength = TmDequeue(&port, buf);
			if(pmlength<0) goto pmerror;
			if(pmlength==0){
				break;
			}
			//printf("get:%02x \n", buf[0]);
			played=~0;
			if( 1==rtsyn_play_one_data (port, ev2message(pmlength,buf)) ){	
				rtsyn_play_one_sysex(buf,pmlength );
			}
			
		}while(pmlength>0);
		//}
		//troiaezu
		usleep(100);
		//  break;
	}while(1);
	
	return played;
pmerror:
	//Pm_Terminate();
	//ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return 0;
}
static int workerThread(void *){
    rtsyn_play_some_data();;
    return 0;
}
void initMidiInputOSX(){
    
	OSStatus result;
	MIDIClientRef client = NULL;
	
	
	result = MIDIClientCreate( CFSTR("OSXAgiletimbres"), NULL, 0, 
							  &client );
	MIDIEndpointRef outDest[4] = {NULL,NULL,NULL,NULL};
	
	
	MIDIDestinationCreate(  client, CFSTR("TX Synthesizer"), MyReadProc<0>, 
						  (void*)0, &outDest[0] );

    
    SDL_CreateThread(workerThread, NULL);
    
    
}


#endif
