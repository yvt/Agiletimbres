//
//  TXLegacyMidi.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 3/18/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#include "TXLegacyMidi.h"

#include "TXFactory.h"
#include <stdlib.h>
#include <map>
#include "TPList/TPLArray.h"
#include "TPList/TPLDictionary.h"
#include "TPList/TPLNumber.h"
#include "TPList/TPLAutoReleasePtr.h"
#include "TXSineTable.h"
#include <zlib.h>

#include <memory.h>
#ifdef WIN32
// for alloca
#include <malloc.h>
#endif

#pragma mark - Bank

typedef int32_t Int32;
typedef uint32_t UInt32;

class TXLegacyMidi::datafile{
protected:
	struct Item{
		Int32 id;
		long pos;
		UInt32 size;
		UInt32 csize;
		void *wave;
		UInt32 comp;
	};
	struct IndexMap{
		Int32 id;
		UInt32 itemIndex;
		
		bool operator <(const IndexMap& i) const{
			return id<i.id;
		}
	};	
	enum{NameLen=8};
	std::vector<Item> items;
	std::vector<IndexMap> indices;
	/*
	std::map<Int32, long>posdata;
	std::map<Int32, UInt32>sizedata;
	std::map<Int32, UInt32>csizedata;
	std::map<Int32, void *>wavedata;
	std::map<Int32, UInt32>compdata;
	std::vector<void *>waves;
	std::vector<Int32> ids;*/
	char in_buf[16384];
	Int32 *small_data;
	long small_data_size;
	long begin;
	FILE *f;
	long usage;
	virtual void *allocate(size_t size);
	virtual void release(void *);
	int indexForId(Int32);
	
public:
	datafile(FILE *);
	virtual ~datafile();
	virtual void preread(){}
	const void *get_wave(long);
	int32_t get_int(long);
	float get_float(long);
	long get_size(long);
	long get_memory_usage();
	void preload();
	void unload();
	
	static long voice2id(bool drum, int bank, int prog, int split){
		register long id;
		id= ((drum?8:7)+((bank>>6)<<1))*100000000;
		id+=(prog+(((bank>>5)&1)<<8))*     100000;
		id+=(split+((bank&0x1f)<<4))*         100;
		return id;
	}
};

TXLegacyMidi::datafile::datafile(FILE *f){
	if(f==NULL){
		throw std::exception();
	}
	this->f=f;
	Int32 cnt,n;
	long ttl;
	ttl=0;
	
	setvbuf(f, NULL, _IONBF, 0);
	fseek(f, 0, SEEK_SET);
	if(fread(&cnt, sizeof(Int32), 1, f)<1){
		fclose(f);
		throw std::exception();
	}
	
	bool optimizeAdded=false;
	// [0] - small-data optimization
	// [1] - index-map optimization
	uint32_t optimizeAddresses[16]={
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	};
	
	
	// scan for optimization block.
	fseek(f, -4, SEEK_END);
	uint32_t magic;
	fread(&magic, 1, 4, f);
	if(magic==0x4221abbc){
		// optimized
		
		fseek(f, -64, SEEK_END);
		fread(optimizeAddresses, 4, 16, f);
	}
	
	fseek(f, 4, SEEK_SET);
	
	uint32_t *itemsBuf=new uint32_t[cnt*4];
	if(fread(itemsBuf, 16, cnt, f)<cnt){
		delete[] itemsBuf;
		throw std::string("failed to read item descriptors.");
	}
	printf("datafile: %d item descriptors read.\n", (int)cnt);
	
	items.reserve(cnt);
	
	for(n=0;n<cnt;n++){
		Int32 id; UInt32 siz, csiz, comp;
		id=itemsBuf[n*4];
		siz=itemsBuf[n*4+1];
		csiz=itemsBuf[n*4+2];
		comp=itemsBuf[n*4+3];
		
		if(siz==0){
			char exmsg[256];
			sprintf(exmsg, "Invalid siz field [%ld] at 0x%x", (long)id,
					(int)(4+n*16+4));
			delete[] itemsBuf;
			throw std::string(exmsg);
		}
		if(csiz==0){
			char exmsg[256];
			sprintf(exmsg, "Invalid csiz field [%ld] at 0x%x", (long)id,
					(int)(4+n*16+8));
			delete[] itemsBuf;
			throw std::string(exmsg);
		}
		if(comp>3){
			char exmsg[256];
			sprintf(exmsg, "Invalid comp field [%ld] at 0x%x", (long)comp,
					(int)(4+n*16+12));
			delete[] itemsBuf;
			throw std::string(exmsg);
		}
		
		Item item;
		item.size=siz;
		item.csize=csiz;
		item.pos=ttl;
		item.comp=comp;
		item.wave=NULL;
		item.id=id;
		
		items.push_back(item);
		
		
		
		ttl+=siz;
		
	} 
	delete[] itemsBuf;
	begin=ftell(f);
	printf("datafile: %d item descriptors read.\n", (int)cnt);
	printf("datafile: ttl=%d.\n", (int)ttl);
	
	if(optimizeAddresses[1]==0){
		// create optimization.
		fseek(f, 0, SEEK_END);
		optimizeAddresses[1]=ftell(f);
		printf("datafile: creating index-map optimization at 0x%x\n",
			   (int)optimizeAddresses[1]);
		indices.reserve(items.size());
		for(size_t i=0;i<items.size();i++){
			Item& item=items[i];
			IndexMap mp;
			mp.id=item.id;
			mp.itemIndex=i;
			indices.push_back(mp);
		}
		sort(indices.begin(), indices.end());
		puts("datafile: sorted. validating");
		
		for(size_t i=0;i<items.size();i++){
			if(indexForId(items[i].id)!=i){
				printf("datafile: validation failed (%d!=%d) for id %d\n",
					   indexForId(items[i].id), (int)i, (int)items[i].id);
				int i1=indexForId(items[i].id);
				if(i1!=-1){
					Item& item2=items[i1];
					printf("datafile [%d].id=%d\n", (int)i1, (int)item2.id);
					if(item2.id==items[i].id){
						puts("datafile: id duplication. ignored.");
						continue;
					}
				}
				
				throw std::string("internal validation failed.");
			}
		}
		for(size_t i=0;i<indices.size();i++){
			fwrite(&(indices[i].id), 4, 1, f);
			fwrite(&(indices[i].itemIndex), 4, 1, f);
		}
		puts("datafile: index-map optimization created.");
	}else{
		puts("datafile: index-map optimization found.");
		
		fseek(f,optimizeAddresses[1], SEEK_SET);
		uint32_t *indexMapBuf=new uint32_t[items.size()*2];
		fread(indexMapBuf, 8, items.size(), f);
		indices.reserve(items.size());
		for(size_t i=0;i<items.size();i++){
			IndexMap mp;
			mp.id=indexMapBuf[i*2];
			mp.itemIndex=indexMapBuf[i*2+1];
			indices.push_back(mp);
		}
		puts("datafile: index-map optimization read.");
		
	}
	
	
	
	usage=0;
	small_data_size=0;
	for(n=0;n<items.size();n++){
		Item& item=items[n];
		if(item.csize==4){
			small_data_size++;
		}
	}
	if(small_data_size>0){
		
		
		if(optimizeAddresses[0]){
			// optimization found.
			
			puts("datafile: small-data optimization found.");
			
			fseek(f, optimizeAddresses[0], SEEK_SET);
			
			uint32_t readSmallDataCount;
			fread(&readSmallDataCount, 1, 4, f);
			readSmallDataCount^=0x10000000; // VERSION - 2
			if(readSmallDataCount!=small_data_size){
				// strange.
				printf("datafile: warning: count mismatch. (%d!=%d)\n",
					 (int)readSmallDataCount, (int)small_data_size);
				goto needsSmallDataOptimization;
			}
			
			uint32_t *tmp=new uint32_t[small_data_size*2];
			fread(tmp, 8, small_data_size, f);
			
			small_data=(Int32 *)allocate(small_data_size*4);
			
			for(n=0;n<small_data_size;n++){
				small_data[n]=tmp[n*2+1];
				items[tmp[n*2]].wave=small_data+n;
			}
			
			
			printf("datafile: %d small-data entries read.\n", (int)small_data_size);
			
			delete[] tmp;
			
			goto smallDataDone;
			
		}
		
	needsSmallDataOptimization:
		
		small_data=(Int32 *)allocate(small_data_size*4);//new Int32[small_data_size];
		Int32 *ptr=small_data;
		for(n=0;n<cnt;n++){
			Item& item=items[n];
			if(item.csize==4){
				long ps;
				ps=begin+item.pos;
				fseek(f, ps, SEEK_SET);
				
				if(fread(ptr, 4, 1, f)==0){
					fprintf(stderr, "small data preload(%ld) failed\n", (long)item.id);
					fprintf(stderr, "posdata is %ld\n", item.pos);
					throw std::string("bank preload failed.");
				}
				
				item.wave=ptr;
				
				ptr++;
			}
		}
		
		// create optimization.
		fseek(f, 0, SEEK_END);
		optimizeAddresses[0]=ftell(f);
		printf("datafile: creating small-data optimization at 0x%x\n",
			   (int)optimizeAddresses[0]);
		uint32_t hdr=small_data_size;
		hdr^=0x10000000; // VERSION - 2
		fwrite(&hdr, 1, 4, f);
		for(uint32_t n=0;n<cnt;n++){
			Item& item=items[n];
			if(item.csize==4){
				fwrite(&n, 1, 4, f);
				fwrite(item.wave, 1, 4, f);
			}
		}
		
		
		
		optimizeAdded=true;
	}else{
		puts("datafile: no small-data");
		small_data=NULL;
	}
smallDataDone:
	
	usage=small_data_size*4;
	
	if(optimizeAdded){
		// rewrite optimization header.
		puts("datafile: optimization updated.");
		fseek(f, 0, SEEK_END);
		printf("datafile: writing header at 0x%x\n", (int)ftell(f));
		optimizeAddresses[15]=0x4221abbc;
		if(fwrite(optimizeAddresses, 4, 16, f)<16){
			throw std::string("datafile: failed to create optimization.");
		}
		fflush(f);
	}
	
}
void TXLegacyMidi::datafile::preload(){
	for(int n=0;n<items.size();n++)
		get_int(items[n].id);
}
void TXLegacyMidi::datafile::unload(){
	for(size_t i=0;i<items.size();i++){
		Item& item=items[i];
		Int32 *ptr=(Int32 *)item.wave;
		if(ptr>=small_data && ptr<(small_data+small_data_size))
			continue;
		if(!ptr)
			continue;
		release(ptr);
		item.wave=NULL;
	}
	usage=small_data_size*4;
}
// datafile~: îgå`ÉfÅ[É^Ç∆ÇªÇÃèÓïÒÇÉÅÉÇÉäÇ©ÇÁè¡ãéÇµÅAÉtÉ@ÉCÉãÇï¬Ç∂ÇÈÅBÇ≈ÅAéÄÇ ÅB
TXLegacyMidi::datafile::~datafile(){
	unload();
	if(small_data)
		release( small_data);
	//fclose(f);
}

// get_wave: îgå`ÉfÅ[É^ÇÃÉ|ÉCÉìÉ^Çï‘Ç∑ÅBîgå`ÉfÅ[É^Ç™ì«Ç›çûÇ‹ÇÍÇƒÇ¢Ç»ÇØÇÍÇŒì«Ç›çûÇﬁÅB
static void *align_address(void *bs){
	long addr=(long)bs;
	return (void *)((addr+0xf)&(~0xfL));
}
static const void *align_address(const void *bs){
	long addr=(long)bs;
	return (const void *)((addr+0xf)&(~0xfL));
}

static const int ulaw2pcm[]={
	-32636, -31612, -30588, -29564, -28540, -27516, -26492, -25468, -24444, -23420, -22396, -21372, -20348, -19324, -18300, -17276,
	-16252, -15740, -15228, -14716, -14204, -13692, -13180, -12668, -12156, -11644, -11132, -10620, -10108,  -9596,  -9084,  -8572,
	-8060,  -7804,  -7548,  -7292,  -7036,  -6780,  -6524,  -6268,  -6012,  -5756,  -5500,  -5244,  -4988,  -4732,  -4476,  -4220,
	-3964,  -3836,  -3708,  -3580,  -3452,  -3324,  -3196,  -3068,  -2940,  -2812,  -2684,  -2556,  -2428,  -2300,  -2172,  -2044,
	-1916,  -1852,  -1788,  -1724,  -1660,  -1596,  -1532,  -1468,  -1404,  -1340,  -1276,  -1212,  -1148,  -1084,  -1020,   -956,
	-892,   -860,   -828,   -796,   -764,   -732,   -700,   -668,   -636,   -604,   -572,   -540,   -508,   -476,   -444,   -412,
	-380,   -364,   -348,   -332,   -316,   -300,   -284,   -268,   -252,   -236,   -220,   -204,   -188,   -172,   -156,   -140,
	-124,   -116,   -108,   -100,    -92,    -84,    -76,    -68,    -60,    -52,    -44,    -36,    -28,    -20,    -12,     -4,
	32632,  31608,  30584,  29560,  28536,  27512,  26488,  25464,  24440,  23416,  22392,  21368,  20344,  19320,  18296,  17272,
	16248,  15736,  15224,  14712,  14200,  13688,  13176,  12664,  12152,  11640,  11128,  10616,  10104,   9592,   9080,   8568,
	8056,   7800,   7544,   7288,   7032,   6776,   6520,   6264,   6008,   5752,   5496,   5240,   4984,   4728,   4472,   4216,
	3960,   3832,   3704,   3576,   3448,   3320,   3192,   3064,   2936,   2808,   2680,   2552,   2424,   2296,   2168,   2040,
	1912,   1848,   1784,   1720,   1656,   1592,   1528,   1464,   1400,   1336,   1272,   1208,   1144,   1080,   1016,    952,
	888,    856,    824,    792,    760,    728,    696,    664,    632,    600,    568,    536,    504,    472,    440,    408,
	376,    360,    344,    328,    312,    296,    280,    264,    248,    232,    216,    200,    184,    168,    152,    136,
	120,    112,    104,     96,     88,     80,     72,     64,     56,     48,     40,     32,     24,     16,      8,      0};
static const uint16_t ima_step[89]={
	7,	  8,	9,	 10,   11,	 12,   13,	 14,
	16,	 17,   19,	 21,   23,	 25,   28,	 31,
	34,	 37,   41,	 45,   50,	 55,   60,	 66,
	73,	 80,   88,	 97,  107,	118,  130,	143,
	157,	173,  190,	209,  230,	253,  279,	307,
	337,	371,  408,	449,  494,	544,  598,	658,
	724,	796,  876,	963, 1060, 1166, 1282, 1411,
	1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493,10442,11487,12635,13899,
	15289,16818,18500,20350,22385,24623,27086,29794,
	32767
};


static const int ima_index[8]={
	-1, -1, -1, -1, 2, 4, 6, 8,
};
const void *TXLegacyMidi::datafile::get_wave(long id){
	int index=indexForId(id);
	if(index==-1){
		char buf[256];
		sprintf(buf, "datafile: item with id %ld not found.", id);
		throw std::string(buf);
	}
	
	Item& item=items[index];
	
	void *data;
	
	data=item.wave;
	if(data==NULL){
		
		// small data should have been already read
		assert(item.csize!=4);
		
		long ps;
		preread();
		ps=begin+item.pos;
		fseek(f, ps, SEEK_SET);
		UInt32 siz, real_siz, comp;
		siz=item.size;
		real_siz=item.csize;//siz&0x7fffffffUL;
		comp=item.comp;
		usage+=real_siz;
		
		int16_t *dt;
		dt=(int16_t *)allocate(real_siz);
		
		if(dt==NULL){
			usage-=real_siz;
			throw std::string("Too few memory.");
		}
		memset(dt, 0, real_siz);
		item.wave=dt;
		if(comp==1){ //deflate
			
			z_stream z;
			z.zalloc=Z_NULL;
			z.zfree=Z_NULL;
			z.opaque=Z_NULL;
			inflateInit(&z);
			z.next_out=(Bytef *)dt;
			z.avail_out=real_siz;
			z.avail_in=0;
			//puts("\n* decomp");
			while(1){
				if(z.avail_in==0){
					z.next_in=(Bytef *)in_buf;
					z.avail_in=fread(in_buf, 1, 16384, f);
				}
				int status;
				//printf("decompressing blk: %lu\n", z.avail_out);
				status=inflate(&z, Z_NO_FLUSH);
				if(status==Z_STREAM_END)
					break;
				if(status!=Z_OK){
					inflateEnd(&z);
					fprintf(stderr, "get_wave(%ld) failed (zlib error %s)\n", id,
							(z.msg) ? z.msg : "???");
					fprintf(stderr, "  sizedata: %lu (%lx)\n", (unsigned long)siz, (unsigned long)siz);
					fprintf(stderr, "  real_siz: %lu (%lx)\n", (unsigned long)real_siz, (unsigned long)real_siz);
					fprintf(stderr, "  avail: in %lu, out %lu\n", (unsigned long)z.avail_in, (unsigned long)z.avail_out);
					throw std::string("zlib error.");
				}
				if(z.avail_out==0){
					break;
				}
			}
			inflateEnd(&z);
		}else if(comp==2){ // uLaw
			unsigned char *ulaw;
			ulaw=(unsigned char *)allocate(siz);//new unsigned char[siz];
			if(ulaw==NULL){
			
				throw std::string("Too few memory.");
			
			}
			fread(ulaw, siz, 1, f);
			for(int pos=0;pos<siz;pos++)
				dt[pos]=ulaw2pcm[ulaw[pos]];
			release(ulaw);
		}else if(comp==3){ // IMA ADPCM
			unsigned char *ima;
			ima=(unsigned char *)allocate(siz-4);
			if(ima==NULL){
				throw std::string("Too few memory.");
				
			}
			fread(dt, 4, 1, f);
			fread(ima, siz-4, 1, f);
			register unsigned char *ptr;
			register int step, pred;
			ptr=ima;
			{
				pred=dt[0];
				int delta;
				delta=dt[1]-dt[0];
				if(delta<0)
					delta=-delta;
				if(delta>32767)
					delta=32767;
				step=0;
				while(ima_step[step]<delta){
					step++;
				}
			}
			real_siz>>=1;
			for(int pos=2;pos<real_siz;pos++){
				int dat;
				dat=*ptr;
				if(pos&0x1)
					dat>>=4;
				{
					register int stp;
					stp=ima_step[step];
					step+=ima_index[dat&7];
					if(step<0)
						step=0;
					if(step>88)
						step=88;
					register int diff=stp>>3;
					if(dat&4)
						diff+=stp;
					if(dat&2)
						diff+=stp>>1;
					if(dat&1)
						diff+=stp>>2;
					if(dat&8)
						pred-=diff;
					else
						pred+=diff;
					if(pred<-0x8000)
						pred=-0x8000;
					else if(pred>0x7fff)
						pred=0x7fff;
					dt[pos]=pred;
				}
				//dt[pos]=ulaw2pcm[ulaw[pos]];
				if(pos&0x1)
					ptr++;
			}
			release(ima);
		}else{
			if(fread(dt, real_siz, 1, f)<1){
				fprintf(stderr, "get_wave(%ld) failed\n", (long)id);
				fprintf(stderr, "  sizedata: %lu (%lx)\n", (long)siz, (long)siz);
				fprintf(stderr, "  real_siz: %lu (%lx)\n", (long)real_siz, (long)real_siz);
				throw std::string("TXLegacyMidi: corrupted bank file.");
			}
		}
		
	}
	const void *wd;
	wd=item.wave;
	{
		const Int32 *ptr;
		ptr=(Int32 *)wd;
		if(ptr>=small_data && ptr<(small_data+small_data_size))
			return wd;
	}
	return (wd);
}

int TXLegacyMidi::datafile::indexForId(Int32 id){
	std::vector<IndexMap>::iterator it;
	IndexMap i; i.id=id;
	it=std::lower_bound(indices.begin(), indices.end(), i);
	if(it==indices.end() || it->id!=id)
		return -1;
	return it->itemIndex;
}

// get_size: ÉfÅ[É^ÇÃÉTÉCÉYÇï‘Ç∑ÅB
long TXLegacyMidi::datafile::get_size(long id){
	int index=indexForId(id);
	if(index==-1)
		return 0;
	return items[index].csize;
}
// get_int: ílÇï‘Ç∑ÅBílÇ™ì«Ç›çûÇ‹ÇÍÇƒÇ¢Ç»ÇØÇÍÇŒì«Ç›çûÇﬁÅB
int32_t TXLegacyMidi::datafile::get_int(long id){
	return *(int32_t *)get_wave(id);
}
// get_float: ílÇï‘Ç∑ÅBílÇ™ì«Ç›çûÇ‹ÇÍÇƒÇ¢Ç»ÇØÇÍÇŒì«Ç›çûÇﬁÅB
float TXLegacyMidi::datafile::get_float(long id){
	return *(float *)get_wave(id);
}
long TXLegacyMidi::datafile::get_memory_usage(){
	return usage;
}
void *TXLegacyMidi::datafile::allocate(size_t size){
	return malloc(size);
}
void TXLegacyMidi::datafile::release(void *ptr){
	free(ptr);
}

#pragma mark - Data Manager

static TXLegacyMidi::datafile *g_datafile=NULL;
static TXLegacyMidi::datafile *g_voicesfile=NULL;
static TXLegacyMidi::ProgressProc g_progressProc=NULL;

void TXLegacyMidi::setProgressProc(ProgressProc proc){
	g_progressProc=proc;
}

void TXLegacyMidi::setBankFile(FILE *f){
	if(g_datafile)
		delete g_datafile;
	g_datafile=NULL;
	if(f)
		g_datafile=new datafile(f);
}


void TXLegacyMidi::setVoicesFile(FILE *f){
	if(g_voicesfile)
		delete g_voicesfile;
	g_voicesfile=NULL;
	if(f)
		g_voicesfile=new datafile(f);
}

void TXLegacyMidi::purgeAllSamples(){
	if(!g_datafile)
		return;
	allSoundsOff();
	g_datafile->unload();
}

std::string TXLegacyMidi::nameForVoice(int bankMsb, int program){
	int id=256+program+bankMsb*256;
	if(g_voicesfile && g_voicesfile->get_size(id)){
		return (const char *)g_voicesfile->get_wave(id);
	}else{
		char buf[256];
		sprintf(buf, "[%03d:%03d]", bankMsb, program);
		return buf;
	}
}

std::string TXLegacyMidi::nameForDrumset(int program){
	int id=128+program;
	if(g_voicesfile && g_voicesfile->get_size(id)){
		return (const char *)g_voicesfile->get_wave(id);
	}else{
		char buf[256];
		sprintf(buf, "[000:%03d]", program);
		return buf;
	}
}

bool TXLegacyMidi::isVoiceAvailable(int bankMsb, int program){
	long id=datafile::voice2id(false, bankMsb, program, 0);
	return g_datafile->get_size(id)>0;
}
bool TXLegacyMidi::isDrumsetAvailable(int program){
	long id=datafile::voice2id(true, 0, program, 36 /*SNARE*/);
	if(g_datafile->get_size(id+1)>0)
		return true;
	id=datafile::voice2id(true, 0, program, 36 /*KICK*/);
	if(g_datafile->get_size(id+1)>0)
		return true;
	return false;
	
}

#pragma mark - Legacy Midi

static TXStandardFactory<TXLegacyMidi> 
g_sharedFactory("TxMidi Legacy",
                "com.nexhawks.TXSynthesizer.LegacyMidiInstrument",
                TXPluginTypeInstrument);

TXFactory *TXLegacyMidi::sharedFactory(){
    return &g_sharedFactory;
}



#pragma mark - Filter

struct TXLegacyMidi::FilterState{
	uint32_t initialFreq;
	uint32_t attackTime;
	uint32_t attackFreq;
	uint32_t decayTime;
	uint32_t sustainFreq;
	uint32_t resonance;
	
	uint32_t time;
	uint32_t lastFreq;
	uint32_t nextFreq;
	
	struct {
		int charge;
		int current;
	} lc6State1, lc6State2;
	
	FilterState(){
		lc6State1.charge=0;
		lc6State1.current=0;
		lc6State2.charge=0;
		lc6State2.current=0;
		initialFreq=0xffffff;
		attackTime=0;
		attackFreq=0;
		decayTime=0;
		sustainFreq=0xffffff;
		resonance=0;
		
		
	}
	
	void setInitialFreq(float sampleFreq, float value){
		float halfSampleFreq=sampleFreq*.5f;
		float freqScale=(65536.f*256.f)/halfSampleFreq;
		if(value<0.f) value=0.f;
		if(value>halfSampleFreq) value=halfSampleFreq;
		initialFreq=(uint32_t)(value*freqScale);
	}
	void setAttackTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		attackTime=(uint32_t)(value*sampleFreq);
	}
	void setAttackFreq(float sampleFreq, float value){
		float halfSampleFreq=sampleFreq*.5f;
		float freqScale=(65536.f*256.f)/halfSampleFreq;
		if(value<0.f) value=0.f;
		if(value>halfSampleFreq) value=halfSampleFreq;
		attackFreq=(uint32_t)(value*freqScale);
	}
	void setDecayTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		decayTime=(uint32_t)(value*sampleFreq);
	}
	void setSustainFreq(float sampleFreq, float value){
		float halfSampleFreq=sampleFreq*.5f;
		float freqScale=(65536.f*256.f)/halfSampleFreq;
		if(value<0.f) value=0.f;
		if(value>halfSampleFreq) value=halfSampleFreq;
		sustainFreq=(uint32_t)(value*freqScale);
	}
	void setResonance(float value){
		if(value<0.f) value=0.f;
		if(value>.999f) value=.999f;
		resonance=(uint32_t)(value*(256.f*65536.f));
	}
	
	void calcEnvelope(unsigned int samples){
        time+=samples;
        lastFreq=nextFreq;
        
        if(time<attackTime){
            nextFreq=initialFreq;
            int delta=(int)attackFreq-(int)initialFreq;
            delta=int((int64_t)delta*(int64_t)time/(int64_t)attackTime);
            nextFreq+=delta;
        }else if(time<attackTime+decayTime){
            nextFreq=attackFreq;
            int delta=(int)sustainFreq-(int)attackFreq;
            delta=int((int64_t)delta*(int64_t)(time-attackTime)/(int64_t)decayTime);
            nextFreq+=delta;
        }else{
            nextFreq=sustainFreq;
        }
        
        
        if(nextFreq>0x80000000UL)
            nextFreq=0;
        
        if(nextFreq>0xffffff)
            nextFreq=0xffffff;
        if(nextFreq<0x80000)
            nextFreq=0x80000;
        
    }
	
	template<bool stereo, bool additive, bool muteSecondChannel>
	void apply(int32_t *out,
			   const int32_t *src1,
			   const int32_t *src2,
			   unsigned int samples){
        unsigned int freq=lastFreq;
        int freqDelta=((int)nextFreq-(int)lastFreq)/(int)samples;
        int charge1=lc6State1.charge;
        int current1=lc6State1.current;
		int charge2=lc6State2.charge;
        int current2=lc6State2.current;
        
        unsigned int reso=resonance<<8;
		
        while(samples--){
            
            
            assert(freq<=0xffffff);
            unsigned int actualFreq=freq<<7;
            // scale freq so that resonance is enhanced.
            actualFreq+=((uint64_t)actualFreq*(uint64_t)reso)>>32;
			
			
			
			int input=*(src1++);
			if(additive){
				input+=*(src2++);
			}
			input<<=8;
            
            current1=((int64_t)current1*(uint64_t)reso)>>32;
            current1+=((int64_t)((input-charge1)<<1)*(uint64_t)actualFreq)>>32;
            charge1+=((int64_t)(current1<<1)*(uint64_t)actualFreq)>>32;
            
            *(out++)+=charge1>>8;
           
			if(muteSecondChannel){
				out++;
			}else{
				if(stereo){
					input=*(src2++)<<8;
					
					current2=((int64_t)current2*(uint64_t)reso)>>32;
					current2+=((int64_t)((input-charge2)<<1)*(uint64_t)actualFreq)>>32;
					charge2+=((int64_t)(current2<<1)*(uint64_t)actualFreq)>>32;
					
					*(out++)+=charge2>>8;
				}else{
					*(out++)+=charge1>>8;
				}
			}
            
            
            freq+=freqDelta;
        }
        
        lc6State1.charge=charge1;
        lc6State1.current=current1;
        
		lc6State2.charge=charge2;
        lc6State2.current=current2;
        
    }
	
	void setup(){
		time=0;
		nextFreq=initialFreq;
		if(nextFreq>0xffffff)
			nextFreq=0xffffff;
	}
};

#pragma mark - Volume Envelope

struct TXLegacyMidi::VolumeEnvelopeState{
	uint32_t time;
	bool off:1;
	bool alive:1; // false = no more sound
	unsigned int velocity: 30;
	uint32_t offTime;
	
	uint32_t attackTime;
	uint32_t decayTime;
	uint32_t sustainLevel; // 0x10000 = 100%
	uint32_t releaseTime;
	uint32_t gain; // 0x10000 = 100%
	bool smoothRelease;
	bool smoothDecay;
	
	uint32_t nextGain;
	uint32_t lastGain;
	int32_t deltaGain;
	
	uint32_t synthGain;
	
	VolumeEnvelopeState(){
		time=0;
		off=false;
		alive=true;
		offTime=0;
		attackTime=0;
		decayTime=0;
		sustainLevel=0x10000;
		releaseTime=0;
		gain=0x10000;
		synthGain=0x10000;
		velocity=127;
	}
	
	void setAttackTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		attackTime=(uint32_t)(value*sampleFreq);
	}
	void setDecayTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		decayTime=(uint32_t)(value*sampleFreq);
	}
	void setSustainLevel(float value){
		if(value<0.f) value=0.f;
		if(value>16.f) value=16.f;
		sustainLevel=(uint32_t)(value*65536.f);
	}
	void setReleaseTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		releaseTime=(uint32_t)(value*sampleFreq);
	}
	void setGain(float value){
		if(value<0.f) value=0.f;
		if(value>16.f) value=16.f;
		gain=(uint32_t)(value*65536.f);
	}
	
	// returns current gain. 0x10000 = 100%
	uint32_t currentGain() { 
		if(!alive)
			return false;
		
		uint32_t value;
		if(time<attackTime){
			value=(uint32_t)(((uint64_t)time<<16)/(uint64_t)attackTime);
		}else if(time<attackTime+decayTime){
			value=(uint32_t)(((uint64_t)(time-attackTime)<<16)/
							 (uint64_t)decayTime);
			if(smoothDecay){
				value=0x10000-value;
				value=(value*value)>>16;
				value=(value*value)>>16;
				value=0x10000-value;
			}
			int32_t delta=(int32_t)sustainLevel-0x10000;
			value=0x10000+(((int64_t)value*(int64_t)delta)>>16);
		}else{
			value=sustainLevel;
		}
		if(off){
			if(offTime<releaseTime){
				uint32_t per;
				per=(uint32_t)(((uint64_t)offTime<<16)/(uint64_t)releaseTime);
				per=0x10000-per;
				if(smoothRelease)
					per=(per*per)>>16;
				value=(((uint64_t)value*(uint64_t)per)>>16);
			}else{
				value=0;
				alive=false;
				return value;
			}
		}
		
		value=(((uint64_t)value*(uint64_t)gain)>>16);
		value=(((uint64_t)value*(uint64_t)synthGain)>>16);
		
		value=(((uint64_t)value*(uint64_t)velocity)>>7);
		value=(((uint64_t)value*(uint64_t)(velocity+1))>>7);
		
		return value;
	}
	
	void advanceTime(int delta){
		time+=delta;
		if(off)
			offTime+=delta;
	}
	
	void setup(int vel){
		nextGain=currentGain();
		velocity=vel;
	}
	
	void calcEnvelope(unsigned int samples){
		advanceTime(samples);
		lastGain=nextGain;
		
		nextGain=currentGain();
		deltaGain=nextGain-lastGain;
	}
	
	void applyInplace(int32_t *out,
			   unsigned int samples){
        unsigned int gain=lastGain;
        int delta=(int)deltaGain/(int)samples;
        
        while(samples--){
			int32_t value=*out;
			
			value=(int32_t)(((int64_t)value*(int64_t)gain)>>16);
			*(out++)=value;
			
			gain+=delta;
        }
        
    }
};

#pragma mark - Frequency Envelope

struct TXLegacyMidi::FreqEnvelopeState{
	uint32_t time;
	uint32_t legatoShift; // millicents
	
	uint32_t riseTime;
	int32_t initialPitch; // millicents
	uint32_t legatoTime;
	uint32_t lfoDelay;
	int32_t lfoDepth; // millicents
	uint32_t lfoSpeed; // phase/sample (2pi=0x100000000)
	
	FreqEnvelopeState(){
		time=0;
		legatoShift=0;
		
		riseTime=0;
		initialPitch=-120;
		legatoTime=0;
		lfoDelay=0;
		lfoDepth=0;
		lfoSpeed=0;
	}
	FreqEnvelopeState(float sampleFreq){
		time=0;
		legatoShift=0;
		
		riseTime=0;
		initialPitch=-120;
		legatoTime=0;
		lfoDelay=0;
		lfoDepth=0;
		lfoSpeed=0;
		
		setLfoDelay(sampleFreq, .13f);
		setLfoFreq(sampleFreq, 5.f);
	}
	
	// returns in millicents
	int32_t currentShift(){
		int32_t shift=0;
		if(time<riseTime){
			uint32_t per=(uint32_t)(((uint64_t)time<<16)/(uint64_t)riseTime);
			per=65536-per;
			shift+=(int32_t)(((int64_t)initialPitch*(int64_t)per)>>16);
		}
		if(time<legatoTime){
			uint32_t per=(uint32_t)(((uint64_t)time<<16)/(uint64_t)legatoTime);
			per=65536-per;
			shift+=(int32_t)(((int64_t)legatoShift*(int64_t)per)>>16);
		}
		if(time>lfoDelay && lfoDepth>0){
			uint32_t lfoTime=time-lfoDelay;
			uint32_t phase=lfoTime*lfoSpeed;
			int32_t wave=TXSineWave(phase);
			shift+=(int32_t)(((int64_t)lfoDepth*(int64_t)wave)>>15);
		}
		return shift;
	}
	
	void setRiseTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		riseTime=(uint32_t)(value*sampleFreq);
	}
	void setInitialPitch(float value){
		if(value<-65536.f) value=-65536.f;
		if(value>65536.f) value=65536.f;
		initialPitch=(uint32_t)(value*1000.f);
	}
	void setLegatoTime(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		legatoTime=(uint32_t)(value*sampleFreq);
	}
	void setLfoDelay(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>60.f) value=60.f;
		lfoDelay=(uint32_t)(value*sampleFreq);
	}
	void setLfoDepth(float value){
		if(value<-65536.f) value=-65536.f;
		if(value>65536.f) value=65536.f;
		lfoDepth=(uint32_t)(value*1000.f);
	}
	void setLfoFreq(float sampleFreq, float value){
		if(value<0.f) value=0.f;
		if(value>100.f) value=100.f;
		lfoSpeed=(uint32_t)(4294967296.f/sampleFreq*value);
	}
	
	void setup(){
		// currently nothing to do
	}
};

#pragma mark - Layer (Voice)

struct TXLegacyMidi::VoiceState{
	const int16_t *wave;
	long waveId;
	uint32_t loopStart, loopEnd;
	float baseFreq; int baseKey;
	
	float baseSpeed; // without actual key or freqEnvelope
	float speed; // without freqEnvelope
	uint32_t actualSpeed; // with all
	uint32_t lastActualSpeed;
	
	VolumeEnvelopeState volumeEnvelope;
	FreqEnvelopeState freqEnvelope;
	uint32_t highPos;
	uint32_t lowPos;
	
	int transposeMillicents;
	
	bool alive;
	VoiceState(){
		wave=NULL; waveId=-1;
		loopStart=0; loopEnd=0;
		highPos=0; lowPos=0;
		alive=true;
		transposeMillicents=0;
	}
	
	uint32_t currentActualSpeed(){
		int32_t shift=freqEnvelope.currentShift();
		float fSpeed=speed;
		shift+=transposeMillicents;
		
		fSpeed*=TXInstrument::scaleForMillicents(shift);
		
		assert(fSpeed>=0.f);
		
		return (uint32_t)(fSpeed);
	}
	
	void setup(int key, int velocity, bool isDrum){
		if(!alive)
			return;
		if(isDrum){
			// already scaled for drum
			speed=baseSpeed;
		}else{
			speed=baseSpeed;
			speed*=TXInstrument::scaleForCents((key-baseKey)*100);
		}
		
		volumeEnvelope.setup(velocity);
		freqEnvelope.setup();
		
		actualSpeed=currentActualSpeed();
	}
	
	void triggerOff(){
		if(!alive)
			return;
		volumeEnvelope.off=true;
	}
	
	template<bool looped>
	void renderWave(int32_t *out, unsigned int samples){
		register unsigned int nowActualSpeed=lastActualSpeed;
		register int deltaActualSpeed=actualSpeed-lastActualSpeed;
		deltaActualSpeed/=(int)samples;
		register const int16_t *wav=wave;
		register unsigned int hPos=highPos;
		register unsigned int lPos=lowPos;
		register const unsigned int loop1=loopStart;
		register const unsigned int loop2=loopEnd;
		
		assert(hPos<loop2);
		assert(lPos<0x10000);
		
		// rendering core
		while(samples--){
			
			// linear interpolation.
			register int value1=wav[hPos];
			register int value2=hPos+1;
			if(looped){
				if(value2>=loop2){
					// wrapped.
					value2=loop1;
				}
				value2=wav[value2];
			}else{
				if(value2>=loop2){
					// ended.
					value2=0;
				}else{
					value2=wav[value2];
				}
			}
			
			value1*=(0x10000-(int)lPos);
			value2*=(	     (int)lPos);
			value1>>=16; value2>>=16;
			
			*(out++)=value1+value2;
			
			// advance sampling cursor.
			lPos+=nowActualSpeed;
			hPos+=(lPos>>16);
			lPos&=0xffffU;
			
			if(looped){
				while(hPos>=loop2){
					hPos-=loop2;
					hPos+=loop1;
				}
			}else{
				if(hPos>=loop2){
					// no more samples.
					alive=false;
					break;
				}
			}
			
			nowActualSpeed+=deltaActualSpeed;
		}
		
		// ???
		actualSpeed=nowActualSpeed;
		
		// zero fill
		if((int)samples>0){
			while(samples--){
				*(out++)=0;
			}
		}
		
		highPos=hPos;
		lowPos=lPos;
		
	}
	
	void render(int32_t *out, unsigned int samples){
		assert(alive);
		
		// first, render waveform.
		
		// calc freq envelope.
		lastActualSpeed=actualSpeed;
		freqEnvelope.time+=samples;
		actualSpeed=currentActualSpeed();
		
		// render waveform.
		if(loopStart<loopEnd){
			renderWave<true>(out, samples);
		}else {
			renderWave<false>(out, samples);
		}
		
		// apply volume envelope.
		volumeEnvelope.calcEnvelope(samples);
		volumeEnvelope.applyInplace(out, samples);
		
		if(!volumeEnvelope.alive)
			alive=false;
	}
};

#pragma mark - Channel

struct TXLegacyMidi::ChannelState{
	unsigned int bankMsb: 7;
	bool isAlive: 1;
	unsigned int program: 7;
	bool isDrum: 1;
	unsigned int velocity: 7;
	bool isOff:1;
	unsigned int key: 7;
	bool isValid:1;
	
	uint32_t voiceId;
	
	VoiceState voice1;
	VoiceState voice2;
	ChannelConfig cfg;
	FilterState filter;
	bool useFourVoice: 7;
	bool isSlave:1;
	unsigned int fourVoiceBankMsb:8;
	unsigned int fourVoiceProgram:8;
	unsigned int maxKey:8;
	ChannelList::iterator fourVoiceChannel;
	
	ChannelState(){
		isOff=false;
		isSlave=false;
		isAlive=true;
	}
	
	void setup(int key, int velocity, bool isDrum){
		this->key=key;
		this->velocity=velocity;
		voice1.setup(key, velocity, isDrum);
		voice2.setup(key, velocity, isDrum);
		if(cfg==ChannelConfigVelocitySwitch32){
			cfg=((velocity>=32)?ChannelConfigWave2:
				 ChannelConfigWave1);
		}else if(cfg==ChannelConfigVelocitySwitch64){
			cfg=((velocity>=64)?ChannelConfigWave2:
				 ChannelConfigWave1);
		}else if(cfg==ChannelConfigVelocitySwitch96){
			cfg=((velocity>=96)?ChannelConfigWave2:
				 ChannelConfigWave1);
		}else if(cfg==ChannelConfigVelocitySwitch120){
			cfg=((velocity>=120)?ChannelConfigWave2:
				 ChannelConfigWave1);
		}else if(cfg==ChannelConfigRandom){
			cfg=((rand()&0x100)?ChannelConfigWave2:
				 ChannelConfigWave1);
		}
		
		filter.setup();
	}
	
	void triggerOff(){
		isOff=true;
		voice1.triggerOff();
		voice2.triggerOff();
	}
};

enum{
	VoiceInfoOffsetChannelConfig=0,
	VoiceInfoOffsetMaxKey=1,
	VoiceInfoOffsetVoice1WaveId=2,
	VoiceInfoOffsetVoice2WaveId=3,
	VoiceInfoOffsetVoice1VolumeAttackTime=4,
	VoiceInfoOffsetVoice1VolumeDecayTime=5,
	VoiceInfoOffsetVoice1VolumeSustainLevel=6,
	VoiceInfoOffsetVoice1VolumeReleaseTime=7,
	VoiceInfoOffsetVoice1VolumeGain=8,
	VoiceInfoOffsetVoice2VolumeAttackTime=9,
	VoiceInfoOffsetVoice2VolumeDecayTime=10,
	VoiceInfoOffsetVoice2VolumeSustainLevel=11,
	VoiceInfoOffsetVoice2VolumeReleaseTime=12,
	VoiceInfoOffsetVoice2VolumeGain=13,
	VoiceInfoOffsetFilterInitialFreq=14,
	VoiceInfoOffsetFilterAttackTime=15,
	VoiceInfoOffsetFilterAttackFreq=16,
	VoiceInfoOffsetFilterDecayTime=17,
	VoiceInfoOffsetFilterSustainFreq=18,
	VoiceInfoOffsetFilterResonance=31,
	VoiceInfoOffsetVoice1LfoDepth=19,
	VoiceInfoOffsetVoice1LfoDelay=20,
	VoiceInfoOffsetVoice1LfoFreq=21,
	VoiceInfoOffsetVoice1FreqScale=22,
	VoiceInfoOffsetVoice2FreqScale=23,
	VoiceInfoOffsetVoice1SmoothRelease=24,
	VoiceInfoOffsetVoice1SmoothSustain=25,
	VoiceInfoOffsetVoice2SmoothRelease=26,
	VoiceInfoOffsetVoice2SmoothSustain=27,
	VoiceInfoOffsetVoice2LfoDepth=28,
	VoiceInfoOffsetVoice2LfoDelay=29,
	VoiceInfoOffsetVoice2LfoFreq=30,
	VoiceInfoOffsetVoice1FreqRiseTime=32,
	VoiceInfoOffsetVoice2FreqRiseTime=33,
	VoiceInfoOffsetVoice1InitialPitch=34,
	VoiceInfoOffsetVoice2InitialPitch=35,
	VoiceInfoOffsetEffect=36,
	VoiceInfoOffsetUseFourVoice=37,
	VoiceInfoOffsetFourVoiceProgram=38,
	VoiceInfoOffsetFourVoiceBankMSB=39,
	VoiceInfoOffsetVoice1LegatoTime=40,
	VoiceInfoOffsetVoice2LegatoTime=41,
	
};

enum{
	WaveInfoOffsetWave=0,
	WaveInfoOffsetLoopBegin=1,
	WaveInfoOffsetLoopEnd=2,
	WaveInfoOffsetBaseSampleFreq=3,
	WaveInfoOffsetBaseKey=4
};

TXLegacyMidi::TXLegacyMidi(const TXConfig& config){
	if(!g_datafile)
		throw std::string("TxMidi bank is not loaded.");
    m_sampleFreq=config.sampleRate;
    //m_baseSpeedScale=65536.f*65536.f/m_sampleFreq;
    //m_pitchbendScale=0x10000;
	m_gain=(unsigned int)(m_parameter.volume*65536.f);
	prepareVoice(0, 0);
}

TXLegacyMidi::~TXLegacyMidi(){
    
}

TXLegacyMidi::datafile *TXLegacyMidi::dataBank(){
	return g_datafile;
}

static float nextRandom(){
    return (float)rand()/(float)RAND_MAX;
}

bool TXLegacyMidi::noteOn(int key, int velocity, bool isDrum,
			int bankMsb, int program, ChannelList::iterator& out){
	ChannelState newState;
	if(isDrum){
		newState=prepareDrum(program, key);
		if(!newState.isValid){
			return false;
		}
		
		// four layer drum voice is not supported.
		
	}else{
		ChannelList& list=prepareVoice(bankMsb, program);
		// find appropriate "split"
		ChannelList::iterator it;
		for(it=list.begin();it!=list.end();it++){
			ChannelState& state=*it;
			if(state.maxKey>=key+m_parameter.transposeCorase ||
			   state.maxKey>=127){
				// found!
				newState=state;
				break;
			}
		}
		
		if(it==list.end()){
			// this shouldn't happen...
			return false;
		}
		
		// four layer.
		if(newState.useFourVoice){
			ChannelList::iterator newLayer;
			if(noteOn(key, velocity, isDrum,
					  newState.fourVoiceBankMsb,
					  newState.fourVoiceProgram,
					  newLayer)){
				newState.fourVoiceChannel=newLayer;
				newLayer->isSlave=true;
			}else{
				// failed.
				newState.useFourVoice=false;
			}
		}
	}
	
	newState.voice1.volumeEnvelope.synthGain=m_gain;
	newState.voice2.volumeEnvelope.synthGain=m_gain;
	newState.voice1.transposeMillicents=m_parameter.transposeFine*1000+
	m_parameter.transposeCorase*100000;
	newState.voice2.transposeMillicents=m_parameter.transposeFine*1000+
	m_parameter.transposeCorase*100000;
	newState.setup(key, velocity, isDrum);
	
	// have to push to front, so that
	// noteOff can handle a four layered channel properly
	m_channels.push_front(newState);
	
	out=m_channels.begin();
	
	return true;
}

int TXLegacyMidi::currentPolyphonics(){
	int poly=0;
	for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
		if(it->isSlave)
			continue;
		if(!it->isAlive)
			continue;
		poly++;
	}
	return poly;
}

void TXLegacyMidi::lastSoundOff(){
	for(ChannelList::reverse_iterator it=m_channels.rbegin();
        it!=m_channels.rend();it++){
		if(it->isSlave)
			continue;
		if(!it->isAlive)
			continue;
		it->isAlive=false;
		for(ChannelList::reverse_iterator it2=m_channels.rbegin();
			it!=it2;it2++){
			it2->isAlive=false;
		}
		return;
	}
}

void TXLegacyMidi::noteOn(int key, int velocity){
	ChannelList::iterator outChannel;
	if(currentPolyphonics()>=m_parameter.polyphonics)
		lastSoundOff();
	noteOn(key, velocity, m_parameter.isDrum,
		   m_parameter.bank, m_parameter.program, outChannel);
}

void TXLegacyMidi::noteOff(int key, int velocity){
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        ChannelState& ch=*it;
        if(ch.key==key && !ch.isOff){
            // found!
            bool hasFourLayer=ch.useFourVoice;
			ch.triggerOff();
			if(hasFourLayer){
				noteOff(key, velocity);
			}
			return;
        }
    }
}

void TXLegacyMidi::setupChannel(TXLegacyMidi::ChannelState &channel){
	
	channel.voice1.wave=NULL;
	channel.voice2.wave=NULL;
	channel.voice1.waveId=-1;
	channel.voice2.waveId=-1;
	channel.isValid=true;
	
	long baseId=-1;
	datafile *bank=dataBank();
	
	if(channel.isDrum){
		// drum voice.
		baseId=datafile::voice2id(true, 0, 
								  channel.program, channel.key);
		if(bank->get_size(baseId+VoiceInfoOffsetChannelConfig)==0){
			// fall back to program 0 (STANDARD)...
			baseId=datafile::voice2id(true, 0, 
									  channel.program, channel.key);
			if(bank->get_size(baseId+VoiceInfoOffsetChannelConfig)==0){
				// voice not found.
				channel.isValid=false;
				return;
			}
		}
	}else{
		// normal voice.
		baseId=datafile::voice2id(false, channel.bankMsb,
								  channel.program, 0);
		if(bank->get_size(baseId+VoiceInfoOffsetMaxKey)==0){
			// fall back to bank MSB 0...
			baseId=datafile::voice2id(false, 0,
									  channel.program, 0);
			channel.bankMsb=0;
		}
		if(bank->get_size(baseId+VoiceInfoOffsetMaxKey)==0){
			// fall back to program 0 (Grand Piano)...
			baseId=datafile::voice2id(false, 0,
									  0, 0);
			channel.program=0;
		}
		if(bank->get_size(baseId+VoiceInfoOffsetMaxKey)==0){
			// normal voice is unavailable.
			channel.isValid=false;
			return;
		}
		
		// find appropriate "split".
		int splitId;
		int channelKey=channel.key;
		for(splitId=1;splitId<16;splitId++){
			if(bank->get_size(baseId+VoiceInfoOffsetMaxKey)==0){
				// rejected. (what is this?)
				channel.isValid=false;
				return;
			}
			if(channelKey<=bank->get_int(baseId+VoiceInfoOffsetMaxKey)){
				// found.
				break;
			}
			
			// next split.
			baseId+=100;
		}
		
		if(splitId==16){
			// split not found.
			channel.isValid=false;
			return;
		}
		
				
	}
	
	channel.cfg=(ChannelConfig)
	bank->get_int(baseId+VoiceInfoOffsetChannelConfig);
	
	channel.maxKey=bank->get_int(baseId+VoiceInfoOffsetMaxKey);
	if(bank->get_size(baseId+VoiceInfoOffsetUseFourVoice)>=4){
		channel.useFourVoice=bank->get_int
		(baseId+VoiceInfoOffsetUseFourVoice);
		if(channel.useFourVoice){
			channel.fourVoiceProgram=bank->get_int
			(baseId+VoiceInfoOffsetFourVoiceProgram);
			channel.fourVoiceBankMsb=bank->get_int
			(baseId+VoiceInfoOffsetFourVoiceBankMSB);
		}
	}else{
		channel.useFourVoice=false;
	}
	
	// load operator 1
	if(bank->get_size(baseId+VoiceInfoOffsetVoice1WaveId)>=4){
		VoiceState& voice=channel.voice1;
		voice=VoiceState();
		voice.waveId=bank->get_int(baseId+VoiceInfoOffsetVoice1WaveId);
		voice.wave=(const int16_t *)bank->get_wave(voice.waveId+WaveInfoOffsetWave);
		if(bank->get_size(voice.waveId+WaveInfoOffsetLoopBegin)>=4){
			voice.loopStart=bank->get_int(voice.waveId+WaveInfoOffsetLoopBegin);
			voice.loopEnd=bank->get_int(voice.waveId+WaveInfoOffsetLoopEnd);
		}else{
			voice.loopStart=bank->get_size(voice.waveId+WaveInfoOffsetWave)/2;
			voice.loopStart-=64;
			voice.loopEnd=voice.loopStart;
		}
		voice.baseFreq=bank->get_float(voice.waveId+WaveInfoOffsetBaseSampleFreq);
		voice.baseKey=bank->get_int(voice.waveId+WaveInfoOffsetBaseKey);
		if(channel.isDrum){
			voice.baseKey=60-voice.baseKey;
		}
		voice.baseSpeed=voice.baseFreq*65536.f/m_sampleFreq;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1FreqScale)>=4){
			voice.baseSpeed*=bank->get_float(baseId+VoiceInfoOffsetVoice1FreqScale);
		}
		
		VolumeEnvelopeState& vol=voice.volumeEnvelope;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1VolumeAttackTime)>=4){
			vol.setAttackTime(m_sampleFreq, bank->get_float
							  (baseId+VoiceInfoOffsetVoice1VolumeAttackTime));
			vol.setDecayTime(m_sampleFreq, bank->get_float
							  (baseId+VoiceInfoOffsetVoice1VolumeDecayTime));
			vol.setSustainLevel(bank->get_float
							  (baseId+VoiceInfoOffsetVoice1VolumeSustainLevel));
			vol.setReleaseTime(m_sampleFreq, bank->get_float
							  (baseId+VoiceInfoOffsetVoice1VolumeReleaseTime));
			vol.setGain(bank->get_float
							  (baseId+VoiceInfoOffsetVoice1VolumeGain));
			if(bank->get_size(baseId+VoiceInfoOffsetVoice1SmoothRelease)>=4){
				if(bank->get_int(baseId+VoiceInfoOffsetVoice1SmoothRelease)){
					vol.smoothRelease=true;
				}
			}
			if(bank->get_size(baseId+VoiceInfoOffsetVoice1SmoothSustain)>=4){
				if(bank->get_int(baseId+VoiceInfoOffsetVoice1SmoothSustain)){
					vol.smoothRelease=true;
				}
			}
		}
		
		FreqEnvelopeState& freq=voice.freqEnvelope;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1LfoDepth)>=4){
			freq.setLfoDepth(bank->get_float
							 (baseId+VoiceInfoOffsetVoice1LfoDepth));
			freq.setLfoDelay(m_sampleFreq, bank->get_float
							   (baseId+VoiceInfoOffsetVoice1LfoDelay));
			freq.setLfoFreq(m_sampleFreq, bank->get_float
							   (baseId+VoiceInfoOffsetVoice1LfoFreq));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1FreqRiseTime)>=4){
			freq.setRiseTime(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetVoice1FreqRiseTime));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1LegatoTime)>=4){
			freq.setLegatoTime(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetVoice1LegatoTime));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice1InitialPitch)>=4){
			freq.setInitialPitch(bank->get_float
							 (baseId+VoiceInfoOffsetVoice1InitialPitch));
		}						   
	}else{
		channel.voice1.alive=false;
	}

	channel.voice2=channel.voice1;

	// load operator 2
	if(bank->get_size(baseId+VoiceInfoOffsetVoice2WaveId)>=4){
		VoiceState& voice=channel.voice2;
		voice.waveId=bank->get_int(baseId+VoiceInfoOffsetVoice2WaveId);
		voice.wave=(const int16_t *)bank->get_wave(voice.waveId+WaveInfoOffsetWave);
		if(bank->get_size(voice.waveId+WaveInfoOffsetLoopBegin)>=4){
			voice.loopStart=bank->get_int(voice.waveId+WaveInfoOffsetLoopBegin);
			voice.loopEnd=bank->get_int(voice.waveId+WaveInfoOffsetLoopEnd);
		}else{
			voice.loopStart=bank->get_size(voice.waveId+WaveInfoOffsetWave)/2;
			voice.loopStart-=64;
			voice.loopEnd=voice.loopStart;
		}
		voice.baseFreq=bank->get_float(voice.waveId+WaveInfoOffsetBaseSampleFreq);
		voice.baseKey=bank->get_int(voice.waveId+WaveInfoOffsetBaseKey);
		voice.baseSpeed=voice.baseFreq*65536.f/m_sampleFreq;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2FreqScale)>=4){
			voice.baseSpeed*=bank->get_float(baseId+VoiceInfoOffsetVoice2FreqScale);
		}
		if(channel.isDrum){
			voice.baseSpeed*=TXInstrument::scaleForCents(voice.baseKey*100);
		}
		
		VolumeEnvelopeState& vol=voice.volumeEnvelope;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2VolumeAttackTime)>=4){
			vol.setAttackTime(m_sampleFreq, bank->get_float
							  (baseId+VoiceInfoOffsetVoice2VolumeAttackTime));
			vol.setDecayTime(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetVoice2VolumeDecayTime));
			vol.setSustainLevel(bank->get_float
								(baseId+VoiceInfoOffsetVoice2VolumeSustainLevel));
			vol.setReleaseTime(m_sampleFreq, bank->get_float
							   (baseId+VoiceInfoOffsetVoice2VolumeReleaseTime));
			vol.setGain(bank->get_float
						(baseId+VoiceInfoOffsetVoice2VolumeGain));
			if(bank->get_size(baseId+VoiceInfoOffsetVoice2SmoothRelease)>=4){
				if(bank->get_int(baseId+VoiceInfoOffsetVoice2SmoothRelease)){
					vol.smoothRelease=true;
				}
			}
			if(bank->get_size(baseId+VoiceInfoOffsetVoice2SmoothSustain)>=4){
				if(bank->get_int(baseId+VoiceInfoOffsetVoice2SmoothSustain)){
					vol.smoothRelease=true;
				}
			}
		}
		
		FreqEnvelopeState& freq=voice.freqEnvelope;
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2LfoDepth)>=4){
			freq.setLfoDepth(bank->get_float
							 (baseId+VoiceInfoOffsetVoice2LfoDepth));
			freq.setLfoDelay(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetVoice2LfoDelay));
			freq.setLfoFreq(m_sampleFreq, bank->get_float
							(baseId+VoiceInfoOffsetVoice2LfoFreq));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2FreqRiseTime)>=4){
			freq.setRiseTime(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetVoice2FreqRiseTime));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2LegatoTime)>=4){
			freq.setLegatoTime(m_sampleFreq, bank->get_float
							   (baseId+VoiceInfoOffsetVoice2LegatoTime));
		}
		
		if(bank->get_size(baseId+VoiceInfoOffsetVoice2InitialPitch)>=4){
			freq.setInitialPitch(bank->get_float
								 (baseId+VoiceInfoOffsetVoice2InitialPitch));
		}						   
	}else{
		channel.voice2.alive=false;
	}
	
	
	FilterState& filter=channel.filter;
	filter=FilterState();
	if(bank->get_size(baseId+VoiceInfoOffsetFilterInitialFreq)>=4){
		filter.setInitialFreq(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetFilterInitialFreq));
		filter.setAttackTime(m_sampleFreq, bank->get_float
						  (baseId+VoiceInfoOffsetFilterAttackTime));
		filter.setAttackFreq(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetFilterAttackFreq));
		filter.setDecayTime(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetFilterDecayTime));
		filter.setSustainFreq(m_sampleFreq, bank->get_float
							 (baseId+VoiceInfoOffsetFilterSustainFreq));
		filter.resonance=0;
		if(bank->get_size(baseId+VoiceInfoOffsetFilterResonance)>=4){
			filter.setResonance(bank->get_float
								 (baseId+VoiceInfoOffsetFilterResonance));
		}
	}
	
}

void TXLegacyMidi::allNotesOff(){
    m_channels.clear();
}

void TXLegacyMidi::allSoundsOff(){
    m_channels.clear();
}

void TXLegacyMidi::setPitchbend(int millicents){
    //m_pitchbendScale=(uint32_t)(65536.f*scaleForCents(millicents/1000));
}

void TXLegacyMidi::renderFragmentAdditive(int32_t *out,
                                              unsigned int samples){
    for(ChannelList::iterator it=m_channels.begin();
        it!=m_channels.end();it++){
        ChannelState& ch=*it;
        renderChannelAdditive(out, samples, ch);
    }
	
	// purge dead channels
    ChannelList::iterator it, it2;
    it=m_channels.begin();
    while(it!=m_channels.end()){
        it2=it; it2++;
        if(!it->isAlive){
            
            m_channels.erase(it);
        }
        it=it2;
    }
}



void TXLegacyMidi::renderChannelAdditive(int32_t *outBuffer,
                                             unsigned int samples,
                                             TXLegacyMidi::ChannelState &ch){
	if(!ch.isAlive)
		return;
	ch.voice1.volumeEnvelope.synthGain=m_gain;
	ch.voice2.volumeEnvelope.synthGain=m_gain;
	ch.voice1.transposeMillicents=m_parameter.transposeFine*1000+
	m_parameter.transposeCorase*100000;
	ch.voice2.transposeMillicents=m_parameter.transposeFine*1000+
	m_parameter.transposeCorase*100000;
	if(ch.cfg==ChannelConfigWave1){
		int32_t *tempBuf=(int32_t *)alloca(samples*4);
		
		if(ch.voice1.alive)
			ch.voice1.render(tempBuf, samples);
		else
			memset(tempBuf, 0, samples*4);
		
		ch.filter.calcEnvelope(samples);
		ch.filter.apply<false, false, false>
		(outBuffer, tempBuf, NULL, samples);
		
		ch.isAlive=ch.voice1.alive;
		
	}else if(ch.cfg==ChannelConfigWave2){
		int32_t *tempBuf=(int32_t *)alloca(samples*4);
		
		ch.filter.calcEnvelope(samples);
		
		if(ch.voice2.alive){
			ch.voice2.render(tempBuf, samples);
			ch.filter.apply<false, false, false>
			(outBuffer, tempBuf, NULL, samples);
		}
		
		ch.isAlive=ch.voice2.alive;
	}else if(ch.cfg==ChannelConfigLayered){
		int32_t *tempBuf1=(int32_t *)alloca(samples*4);
		int32_t *tempBuf2=(int32_t *)alloca(samples*4);
		
		ch.filter.calcEnvelope(samples);
		
		if(ch.voice1.alive){
			ch.voice1.render(tempBuf1, samples);
			if(ch.voice2.alive){
				ch.voice2.render(tempBuf2, samples);
				ch.filter.apply<false, true, false>
				(outBuffer, tempBuf1, tempBuf2, samples);
			}else{
				ch.filter.apply<false, false, false>
				(outBuffer, tempBuf1, NULL, samples);
			}
		}else if(ch.voice2.alive){
			ch.voice2.render(tempBuf2, samples);
			ch.filter.apply<false, false, false>
			(outBuffer, tempBuf2, NULL, samples);
		}else{
			// mute
		}
		
		ch.isAlive=ch.voice1.alive||ch.voice2.alive;
	}else if(ch.cfg==ChannelConfigStereo){
		int32_t *tempBuf1=(int32_t *)alloca(samples*4);
		int32_t *tempBuf2=(int32_t *)alloca(samples*4);
		
		ch.filter.calcEnvelope(samples);
		
		if(ch.voice1.alive){
			ch.voice1.render(tempBuf1, samples);
			if(ch.voice2.alive){
				ch.voice2.render(tempBuf2, samples);
				ch.filter.apply<true, false, false>
				(outBuffer, tempBuf1, tempBuf2, samples);
			}else{
				ch.filter.apply<false, false, true>
				(outBuffer, tempBuf1, NULL, samples);
			}
		}else if(ch.voice2.alive){
			ch.voice2.render(tempBuf2, samples);
			ch.filter.apply<false, false, true>
			(outBuffer+1, tempBuf2, NULL, samples);
		}else{
			// mute
		}
		
		ch.isAlive=ch.voice1.alive||ch.voice2.alive;
	}
	
	
	/*
    
    uint32_t phase1=ch.phase1;
    uint32_t phase2=ch.phase2;
    uint32_t speed1=ch.baseSpeed1;
    uint32_t speed2=ch.baseSpeed2;
    int vol=ch.velocity;
    
    // apply pitchbend.
    speed1=((uint64_t)speed1*m_pitchbendScale)>>16;
    speed2=((uint64_t)speed2*m_pitchbendScale)>>16;
    
    while(samples--){
        
        *(outBuffer++)+=(readWaveform(phase1)*vol)>>9>>2;
        *(outBuffer++)+=(readWaveform(phase2)*vol)>>9>>2;
        
        phase1+=speed1;
        phase2+=speed2;
        
    }
    
    ch.phase1=phase1;
    ch.phase2=phase2;
    */
}

#pragma mark - Cache

void TXLegacyMidi::clearCache(){
	m_voiceCache.clear();
	m_drumCache.clear();
}

static unsigned int voiceCacheId(int bankMsb, int program){
	return program|(bankMsb<<7);
}

TXLegacyMidi::ChannelList& TXLegacyMidi::prepareVoice(int bankMsb, int program){
	// check for cache existence.
	if(m_voiceCache.find(voiceCacheId(bankMsb, program))!=m_voiceCache.end()){
		ChannelList& l=m_voiceCache.find(voiceCacheId(bankMsb, program))->second;
		if(l.empty()){
			// invalid
			if(bankMsb==0){
				if(program==0){
					// invalid voice
					return l;
				}else{
					// fall back to program 0
					return prepareVoice(0, 0);
				}
			}else{
				// fall back to bank MSB 0
				return prepareVoice(0, program);
			}
		}else{
			return l;
		}
	}
	
	ChannelList list;
	ChannelState state;
	state.bankMsb=bankMsb;
	state.program=program;
	state.isDrum=false;
	state.velocity=127;
	state.key=0;
	
	setupChannel(state);
	
	if(!state.isValid){
		// invalid voice
		m_voiceCache[voiceCacheId(bankMsb, program)]=list;
		return m_voiceCache[voiceCacheId(bankMsb, program)];
	}
	
	if(state.program!=program || state.bankMsb!=bankMsb){
		// fall backed.
		m_voiceCache[voiceCacheId(bankMsb, program)]=list;
		
		if(bankMsb==0){
			return prepareVoice(0, 0);
		}else{
			return prepareVoice(0, program);
		}
	}
	
	list.push_back(state);
	
	while(state.maxKey<127){
		if(g_progressProc){
			(*g_progressProc)((float)state.maxKey/127.f);
		}
		state.key=state.maxKey+1;
		setupChannel(state);
		assert(state.isValid);
		list.push_back(state);
	}
	
	if(g_progressProc){
		(*g_progressProc)(1.f);
	}
	
	m_voiceCache[voiceCacheId(bankMsb, program)]=list;
	return m_voiceCache[voiceCacheId(bankMsb, program)];
	
}

static unsigned int drumCacheId(int program, int key){
	return key|(program<<7);
}

TXLegacyMidi::ChannelState& TXLegacyMidi::prepareDrum(int program, int key){
	// check for cache existence.
	if(m_drumCache.find(drumCacheId(program, key))!=m_drumCache.end()){
		ChannelState& l=m_drumCache.find(drumCacheId(program, key))->second;
		if(!l.isValid){
			// invalid
			if(program==0){
				// invalid voice
				return l;
			}else{
				// fall back to STANDARD
				return prepareDrum(0, key);
			}
		}else{
			return l;
		}
	}
	
	ChannelState state;
	state.bankMsb=0;
	state.program=program;
	state.isDrum=true;
	state.velocity=127;
	state.key=key;
	
	setupChannel(state);
	
	if(!state.isValid){
		m_drumCache[drumCacheId(program, key)]=state;
		return m_drumCache[drumCacheId(program, key)];
	}
	
	if(state.program!=program){
		// fall backed.
		state.isValid=false;
		m_drumCache[drumCacheId(program, key)]=state;
		return prepareDrum(0, key);
	}
	
	m_drumCache[drumCacheId(program, key)]=state;
	return m_drumCache[drumCacheId(program, key)];
}

#pragma mark - Settings Parameters

#define TransposeCoraseKey "TransposeCorase"
#define TransposeFineKey "TransposeFine"
#define VolumeKey "Volume"
#define IsDrumKey "IsDrum"
#define ProgramKey "ProgramNumber"
#define BankKey "BankMSB"
#define PolyphonicsKey "Polyphonics"

TPLObject *TXLegacyMidi::Parameter::serialize() const{
	TPLDictionary *dic=new TPLDictionary();
	
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(transposeCorase));
		dic->setObject(value, TransposeCoraseKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(transposeFine));
		dic->setObject(value, TransposeFineKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(volume));
		dic->setObject(value, VolumeKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(isDrum));
		dic->setObject(value, IsDrumKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(program));
		dic->setObject(value, ProgramKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(bank));
		dic->setObject(value, BankKey);
	}
	{
		TPLAutoReleasePtr<TPLNumber> value(new TPLNumber(polyphonics));
		dic->setObject(value, PolyphonicsKey);
	}
	return dic;
}

void TXLegacyMidi::Parameter::deserialize(const TPLObject *obj){
	const TPLDictionary *dic=dynamic_cast<const TPLDictionary *>(obj);
	if(!dic)
		return;
	
	const TPLNumber *number;
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(TransposeCoraseKey)))){
		transposeCorase=number->intValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(TransposeFineKey)))){
		transposeFine=number->intValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(VolumeKey)))){
		volume=number->doubleValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(IsDrumKey)))){
		isDrum=number->boolValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(ProgramKey)))){
		program=number->intValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(BankKey)))){
		bank=number->intValue();
	}
	
	if((number=dynamic_cast<const TPLNumber *>(dic->objectForKey(PolyphonicsKey)))){
		polyphonics=number->intValue();
	}
	
}

void TXLegacyMidi::setParameter(const TXLegacyMidi::Parameter &newParam){
	bool reload=false;
	if(m_parameter.isDrum!=newParam.isDrum ||
	   m_parameter.program!=newParam.program ||
	   m_parameter.bank!=newParam.bank){
		reload=true;
	}
	if(m_parameter.polyphonics!=newParam.polyphonics){
		allSoundsOff();
	}
	m_parameter=newParam;
	m_gain=(unsigned int)(m_parameter.volume*65536.f);
	if(reload){
		clearCache();
		if(m_parameter.isDrum){
			for(int i=0;i<128;i++){
				if(g_progressProc){
					(*g_progressProc)((float)i/127.f);
				}
				if(!prepareDrum(m_parameter.program, i).isValid){
					prepareDrum(0, i);
				}
			}
			if(g_progressProc){
				(*g_progressProc)(1.f);
			}
		}else{
			prepareVoice(m_parameter.bank, m_parameter.program);
		}
	}
}


void TXLegacyMidi::deserialize(const TPLObject *obj){
	Parameter param=m_parameter;
	param.deserialize(obj);
	setParameter(param);
}
