//
//  main.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/20/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include "StdAfx.h"
#include "Utils.h"

std::wstring ATVersion();
std::wstring ATGetAppDir();         // /
std::wstring ATResourcesDir();      // /Resources (immutable)
std::wstring ATUserLibraryDir();    // /Library (mutable)

std::wstring ATBuiltinPluginSettingsDir();
std::wstring ATBuiltinPluginSettingsDirForPlugin(const std::string&);
    // /Resources/Plugin Settings (immutable)
std::wstring ATUserPluginSettingsDir();
std::wstring ATUserPluginSettingsDirForPlugin(const std::string&);
    // /Library/Plugin Settings (mutable)

std::wstring ATBuiltinSampleFilesDir();
std::wstring ATBuiltinSampleFilesDirForPlugin(const std::string&); // immutable

void initializeMain();
bool ATIsCertainlySlowDevice();
