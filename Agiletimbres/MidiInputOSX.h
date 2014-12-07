//
//  MidiInputOSX.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 12/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"

#ifdef __MACOSX__
#define USE_MIDIINPUT_OSX   1
#endif

#if USE_MIDIINPUT_OSX

void initMidiInputOSX();

#endif
