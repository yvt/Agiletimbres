/*
 *  pack.cpp
 *  PSPMusic
 *
 *  Created by tcpp on 09/01/10.
 *  Copyright 2009 tcpps.
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <stdint.h>
using namespace std;
#include <zlib.h>
#include <set>
#define BUF_SIZE	65536
unsigned char in_buf[65536];
unsigned char out_buf[65536];
char buf[65536];
char compbuf[65536];
char buf4[65536];
char buf5[65536];
char outs[65536];
bool noComp=false;
bool maxComp=false;
bool ulawComp=false;
int comment=0;
static inline unsigned char pcm2ulaw(short val){
	
	val>>=2;
	if(val>8158)
		val=8158;
	if(val<-8159)
		val=-8159;
	if(val<=-4064)
		return 0x00+((val+8159)>>8);
	else if(val<=-2016)
		return 0x10+((val+4063)>>7);
	else if(val<=-992)
		return 0x20+((val+2015)>>6);
	else if(val<=-480)
		return 0x30+((val+991)>>5);
	else if(val<=-224)
		return 0x40+((val+479)>>4);
	else if(val<=-96)
		return 0x50+((val+223)>>3);
	else if(val<=-32)
		return 0x60+((val+95)>>2);
	else if(val<=-2)
		return 0x70+((val+31)>>1);
	else if(val==-1)
		return 0x7f;
	else if(val==0)
		return 0xff;
	else if(val<=30)
		return 0xf0+((30-val)>>1);
	else if(val<=94)
		return 0xe0+((94-val)>>2);
	else if(val<=222)
		return 0xd0+((222-val)>>3);
	else if(val<=478)
		return 0xc0+((478-val)>>4);
	else if(val<=990)
		return 0xb0+((990-val)>>5);
	else if(val<=2014)
		return 0xa0+((2014-val)>>6);
	else if(val<=4062)
		return 0x90+((4062-val)>>7);
	else
		return 0x80+((8158-val)>>8);
}
static const uint16_t ima_step[89] =
{
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


static const int ima_index[8] =
{
	-1, -1, -1, -1, 2, 4, 6, 8,
};
vector<int> ids;
vector<string> fns;
int ind;
map<string, int> alias;
set<int> excludes;
void read_file(FILE *f){
	char *buf2, *buf3;
	int n;
	while(fgets(buf, 65536, f)){
		if(buf[0]=='#' || buf[0]==0 || buf[0]==' ' || buf[0]==13 || buf[0]==10)
			continue;
		while(buf[strlen(buf)-1]==13 || buf[strlen(buf)-1]==10)
			buf[strlen(buf)-1]=0;
		if(buf[0]=='@'){
			if(buf[1]=='#'){
				if(!strcasecmp(buf, "@#no-compression")){
					noComp=true;
				}
				if(!strcasecmp(buf, "@#max-compression")){
					maxComp=true;
				}
				if(!strcasecmp(buf, "@#ulaw-compression")){
					ulawComp=true;
				}
				if(!strcasecmp(buf, "@#begin-comment")){
					comment++;
				}
				if(!strcasecmp(buf, "@#end-comment")){
					comment--;
				}
				char buf2[256];
				strcpy(buf2, buf);
				buf2[10]=0;
				if(!strcasecmp(buf2, "@#exclude ")){
					excludes.insert(atoi(((const char *)buf)+10));
				}	
			}else{
				FILE *f2;
				f2=fopen(buf+1, "r");
				read_file(f2);
				fclose(f2);
				continue;
			}
		}
		if(comment)
			continue;
		strcpy(buf4, buf);
		
		string dat;
		int id;
		id=0;
		buf3=buf;
		bool instr;
		instr=false;
		for(n=0;buf[n]!=0;n++){
			if(buf[n]==' ' && !instr){
				buf2=&buf[n];
				buf2++;
				dat=((string)buf2);
				buf[n]=0;
				printf("* id add: %s\n", buf3);
				if(buf3[0]<'0' || buf3[0]>'9') // alias
					id+=alias[(string)buf3];
				else
					id+=atol(buf3);
				
				buf3=&buf[n];
				buf3++;
			}else if(buf[n]=='='){
				buf2=&buf[n];
				buf2++;
				
				buf[n]=0;
				id=atol(buf2);
				dat=((string)buf3);
				alias[dat]=id;
				id=-1;
				break;
			}
			if(buf[n]=='"')
				instr=true;
		}
		if(id==-1 || id==0)
			continue;
		
		puts(buf4);
		if(excludes.find(id)!=excludes.end()){
			printf("EXCLUDE: %d\n", id);
			continue;
		}
		printf("  [%ld] %ld - %s\n", ind, id, dat.c_str());
		ind++;
		
		fns.push_back(dat);
		ids.push_back(id);
		printf("  added\n");
	}
}
int main(int argc, char **argv){
	FILE *f, *f2;
	
	
	
	char *buf2, *buf3;
	
	int n, i;
	unsigned int j;
	ind=0;
	f=fopen(argv[1], "r"); 
	fgets(outs, 65536, f);
	while(outs[strlen(outs)-1]==13 || outs[strlen(outs)-1]==10)
		outs[strlen(outs)-1]=0;
	for(n=0;outs[n];n++)
		printf("%d ", (int)outs[n]);
	printf("\n");
	read_file(f);
	fclose(f);
	//printf("ind: %d\n", find(ids.begin(), ids.end(), -2)-ids.begin());
	//strcpy(outs, fns[find(ids.begin(), ids.end(), -2)-ids.begin()].c_str());
	//outs[strlen(outs)-1]=0;
	printf("filename=%s\n", outs);
	f=fopen(outs, "wb");
	fpos_t next, next2, tpos;
	
	n=ids.size(); 
	fwrite(&n, 1, 4, f);
	fgetpos(f, &next);
	next2=next+ids.size()*16;
	printf("%ld items\n", ids.size());
	for(n=0;n<ids.size();n++){
		fsetpos(f, &next);
		printf("%ld : ", ids[n]);
		printf("%s...", fns[n].c_str());
		if(((fns[n].c_str()[0]>='0' && fns[n].c_str()[0]<='9') || (fns[n].c_str()[0]=='-')) && (
																								fns[n].c_str()[fns[n].size()-1]=='f')){
			float value;
			value=atof(fns[n].c_str());
			printf("float value %f\n",value);
			
			fsetpos(f, &next2);
			fwrite(&value, sizeof(float), 1, f);
			fgetpos(f, &next2);
			fsetpos(f, &next);
			j=sizeof(int);
			fwrite(&ids[n], 1, 4, f);
			fwrite(&j, 1, 4, f);
			fwrite(&j, 1, 4, f);
			j=0; fwrite(&j, 1, 4, f);
		}else if((fns[n].c_str()[0]>='0' && fns[n].c_str()[0]<='9') || (fns[n].c_str()[0]=='-')){
			int32_t value;
			value=atol(fns[n].c_str());
			printf("integer value %d\n",value);
			
			fsetpos(f, &next2);
			fwrite(&value, sizeof(int32_t), 1, f);
			fgetpos(f, &next2);
			fsetpos(f, &next);
			j=sizeof(int32_t);
			fwrite(&ids[n], 1, 4, f);
			fwrite(&j, 1, 4, f);
			fwrite(&j, 1, 4, f);
			j=0; fwrite(&j, 1, 4, f);
		}else if(fns[n].c_str()[0]=='"'){
			strcpy(buf5, fns[n].c_str()+1);
			buf5[strlen(buf5)-1]=0;
			printf("string value %s\n", buf5);
			j=strlen(buf5)+1;
			fsetpos(f, &next2);
			fwrite(buf5, j, 1, f);
			fgetpos(f, &next2);
			fsetpos(f, &next);
			fwrite(&ids[n], 1, 4, f);
			fwrite(&j, 1, 4, f);
			fwrite(&j, 1, 4, f);
			j=0; fwrite(&j, 1, 4, f);
		}else{
			printf("file\n");
			// fn=fns[n];
			char fnbuf[256];
			const char *fmt;
			strcpy(fnbuf, fns[n].c_str());
			fmt="";
			for(i=0;fnbuf[i]!=0;i++)
				if(fnbuf[i]=='@'){
					fnbuf[i]=0;
					fmt=fnbuf;
					fmt+=i+1;
					
				}
			
			f2=fopen(fnbuf, "rb");
			if(!f2){
				fprintf(stderr, "FATAL: cannot open %s\n", fnbuf);
				return 1;
			}
			fread(buf, 1, 4, f2);
			buf[5]=0;
			if(buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F'){
				tpos=44;
				printf("  riff ");
				fsetpos(f2, &tpos);
			}else{
				printf("  unknown ");
				tpos=0;
				fsetpos(f2, &tpos);
			}
			fsetpos(f, &next2);
			i=fread(buf, 1, 65536, f2);
			fsetpos(f2, &tpos);
			if(noComp)
				fmt="";
			if(maxComp)
				fmt="imaadpcm";
			if(ulawComp)
				fmt="ulaw";
			if(strcmp(fmt, "imaadpcm")==0){
				puts("  IMA ADPCM compressing...");
				j=0;
				unsigned char *compbuf2=(unsigned char *)compbuf;
				int16_t *buf_int=(int16_t *)buf;
				int step, pred;
				int wrote=0;
				
				{
					i=fread(buf, 1, 4, f2);
					j+=i;
					pred=buf_int[0];
					int delta;
					delta=buf_int[1]-buf_int[0];
					if(delta<0)
						delta=-delta;
					if(delta>32767)
						delta=32767;
					step=0;
					while(ima_step[step]<delta){
						step++;
					}
					fwrite(buf, i, 1, f);
					wrote+=i;
				}
				
				while(1){
					i=fread(buf, 1, 65536, f2);
					j+=i;
					if(i==0)
						break;
					unsigned char *ptr=compbuf2;
					i>>=1;
					for(int k=0;k<i;k++){
						int vl;
						vl=buf_int[k];
						
						int delta;
						unsigned int value;
						delta=vl-pred;
						if(delta>=0)
							value=0;
						else{
							value=8;
							delta=-delta;
						}
						int stp=ima_step[step];
						int diff=stp>>3;
						if(delta>stp){
							value|=4;
							delta-=stp;
							diff+=stp;
						}
						stp>>=1;
						if(delta>stp){
							value|=2;
							delta-=stp;
							diff+=stp;
						}
						stp>>=1;
						if(delta>stp){
							value|=1;
							delta-=stp;
							diff+=stp;
						}
						if(value&8)
							pred-=diff;
						else
							pred+=diff;
						if(pred<-0x8000)
							pred=-0x8000;
						if(pred>0x7fff)
							pred=0x7fff;
						step+=ima_index[value&7];
						if(step<0)
							step=0;
						else if(step>88)
							step=88;
						if(k&0x1)
							*ptr|=value<<4;
						else
							*ptr=value;
						//*ptr=pcm2ulaw(buf_int[k]);
						if(k&0x1)
							ptr++;
					}
					if(i&0x1)
						j+=2;
					i=(i+1)>>1;
					wrote+=i;
					fwrite(compbuf2, i, 1, f);
				}
				fgetpos(f, &next2);
				fsetpos(f, &next);
				printf("  read %ld bytes\n", j);
				printf("  wrote %ld bytes\n", wrote);
				fwrite(&ids[n], 1, 4, f);
				fwrite(&wrote, 1, 4, f);
				fwrite(&j, 1, 4, f);
				j=3; fwrite(&j, 1, 4, f); // ima adpcm
			}else if(strcmp(fmt, "ulaw")==0){
				puts("  u-Law compressing...");
				j=0;
				unsigned char *compbuf2=(unsigned char *)compbuf;
				int16_t *buf_int=(int16_t *)buf;
				while(1){
					i=fread(buf, 1, 65536, f2);
					j+=i;
					if(i==0)
						break;
					unsigned char *ptr=compbuf2;
					i>>=1;
					for(int k=0;k<i;k++){
						*ptr=pcm2ulaw(buf_int[k]);
						ptr++;
					}
					fwrite(compbuf2, i, 1, f);
				}
				fgetpos(f, &next2);
				fsetpos(f, &next);
				int wrote=j>>1;
				printf("  read %ld bytes\n", j);
				printf("  wrote %ld bytes\n", wrote);
				fwrite(&ids[n], 1, 4, f);
				fwrite(&wrote, 1, 4, f);
				fwrite(&j, 1, 4, f);
				j=2; fwrite(&j, 1, 4, f); //uLaw
				
			}else if(i>=65536 || strcmp(fmt, "deflate")==0){
				//compress
				int wrote=0;
				puts("  compressing...");
				j=0;
				z_stream z;
				z.zalloc=Z_NULL;
				z.zfree=Z_NULL;
				z.opaque=Z_NULL;
				deflateInit(&z, 9);
				z.avail_in=0;
				z.next_out=out_buf;
				z.avail_out=BUF_SIZE;
				while(1){
					int flush;
					flush=Z_NO_FLUSH;
					if(z.avail_in==0){
						i=fread(in_buf, 1, BUF_SIZE, f2);
						j+=i;
						z.next_in=in_buf;
						z.avail_in=i;
						if(i==0)
							flush=Z_FINISH;
					}
					if(deflate(&z, flush)==Z_STREAM_END)
						break;
					if(z.avail_out==0){
						fwrite(out_buf, BUF_SIZE, 1, f);
						z.next_out=out_buf;
						z.avail_out=BUF_SIZE;
						wrote+=BUF_SIZE;
					}
				}
				int cnt;
				if((cnt=BUF_SIZE-z.avail_out)>0){
					fwrite(out_buf, cnt, 1, f);
					wrote+=cnt;
				}
				fgetpos(f, &next2);
				fsetpos(f, &next);
				
				printf("  read %ld bytes\n", j);
				printf("  wrote %ld bytes\n", wrote);
				fwrite(&ids[n], 1, 4, f);
				//j|=0x80000000UL;
				fwrite(&wrote, 1, 4, f);
				fwrite(&j, 1, 4, f);
				j=1; fwrite(&j, 1, 4, f);
				deflateEnd(&z);
			}else{	
				j=0;
				while(1){
					i=fread(buf, 1, 65536, f2);
					j+=i;
					if(i==0)
						break;
					fwrite(buf, i, 1, f);
				}
				fgetpos(f, &next2);
				fsetpos(f, &next);
				printf("  read %ld bytes\n", j);
				fwrite(&ids[n], 1, 4, f);
				fwrite(&j, 1, 4, f);
				fwrite(&j, 1, 4, f);
				j=0; fwrite(&j, 1, 4, f);
			}
			fclose(f2);
			puts("");
			
		}
		next+=16;
		fflush(stdout);
	}
	fclose(f);
}