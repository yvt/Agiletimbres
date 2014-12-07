//
//  Utils.h
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <string>

#ifdef WIN32
#define NHPathSeparator L"\\"
#else
#define NHPathSeparator L"/"
#endif

int NHIntWithString(const std::wstring& str);
std::wstring NHStringWithInt(int iv);
std::wstring NHFormatStd(const wchar_t *format, ...);
std::wstring NHVarArgFormat(const std::wstring& format, va_list list);
std::wstring NHFormat(const std::wstring& format, ...);

FILE *NHOpenStd(const std::wstring& path,
                const std::wstring& fileMode);

FILE *NHReOpenStd(const std::wstring& path,
                const std::wstring& fileMode,
                  FILE *f);

std::vector<std::wstring> NHScanDirectory
(const std::wstring& path);

std::wstring NHLastPathComponent(const std::wstring&);

void NHMakeDirectory(const std::wstring& path);

std::wstring NHToLower(const std::wstring& str);


