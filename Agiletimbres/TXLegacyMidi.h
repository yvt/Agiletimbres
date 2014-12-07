//
//  TXLegacyMidi.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 3/18/12.
//  Copyright (c) 2012 Nexhawks. All rights reserved.
//

#pragma once


#include "TXInstrument.h"

#include <list>
#include <map>

class TPLString;

class TXLegacyMidi: public TXInstrument{
public:
	
	typedef void (*ProgressProc)(float);
	
	struct Parameter{
		int transposeCorase; // semitones
		int transposeFine; // cents
		int polyphonics;
		float volume;
		
		bool isDrum;
		
		int program;
		int bank;
		
		TPLObject *serialize() const;
        void deserialize(const TPLObject *);
		
		Parameter(){
			transposeCorase=0;
			transposeFine=0;
			volume=0.35f;
			isDrum=false;
			program=0;
			bank=0;
			polyphonics=4;
		}
	};
	
	class datafile;
	
private:
    
	struct VolumeEnvelopeState;
	struct FreqEnvelopeState;
	struct VoiceState;
	
	// don't edit this -- corresponding to voice bank value
	enum ChannelConfig{ 
		ChannelConfigWave1=0,
		ChannelConfigStereo,
		ChannelConfigLayered,
		ChannelConfigWave2,
		ChannelConfigVelocitySwitch32,
		ChannelConfigVelocitySwitch64,
		ChannelConfigVelocitySwitch96,
		ChannelConfigVelocitySwitch120,
		ChannelConfigRandom
	};
	struct FilterState;
    struct ChannelState;
    
    typedef std::list<ChannelState> ChannelList;
    
    ChannelList m_channels;
    float m_sampleFreq;
	Parameter m_parameter;
	unsigned int m_gain;
	
	std::map<unsigned int, ChannelList> m_voiceCache;
	std::map<unsigned int, ChannelState> m_drumCache;
	
	void clearCache();
	ChannelList& prepareVoice(int bankMsb, int program);
	ChannelState& prepareDrum(int program, int key);
	
	bool noteOn(int key, int velocity, bool isDrum,
				int bankMsb, int program, ChannelList::iterator&);
    
public:
    
    TXLegacyMidi(const TXConfig&config);
    virtual ~TXLegacyMidi();
    
    virtual TXFactory *factory() const{
        return sharedFactory();
    }
    
    static TXFactory *sharedFactory();
	
	static void setBankFile(FILE *);
	static void setVoicesFile(FILE *);
	void purgeAllSamples();
	static datafile *dataBank();
	
	static std::string nameForVoice(int bankMsb, int program);
	static std::string nameForDrumset(int program);
	static bool isVoiceAvailable(int bankMsb, int program);
	static bool isDrumsetAvailable(int program);
	
	static void setProgressProc(ProgressProc);
	
	const Parameter &parameter() const{
        return m_parameter;
    }
    
    void setParameter(const Parameter&);
	
	virtual TPLObject *serialize() const{
		return m_parameter.serialize();
	}
    virtual void deserialize(const TPLObject *obj);
	
	int currentPolyphonics();
	
	void lastSoundOff();
    
protected:
    
    virtual void noteOn(int key, int velocity);
    
    virtual void noteOff(int key, int velocity);
    
    virtual void setPitchbend(int millicents);
    
    virtual void allNotesOff();
    virtual void allSoundsOff();
    
    virtual void renderFragmentAdditive(int32_t *stereoOutputBuffer,
                                        unsigned int samples);
    
    
    void renderChannelAdditive(int32_t *outBuffer,
                               unsigned int samples,
                               ChannelState& ch);
    
	void setupChannel(ChannelState&);
};

