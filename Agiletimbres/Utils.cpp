//
//  Utils.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include <string.h>
#include "Utils.h"
#include <tcw/twStrConv.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/dirent.h>
#include <dirent.h>
#endif

int NHIntWithString(const std::wstring& str){
	char buf[256];
	wcstombs(buf, str.c_str(), 256);
	return atoi(buf);
}
std::wstring NHStringWithInt(int iv){
	wchar_t buf[256];
#ifdef WIN32
    swprintf(buf, L"%d", iv);
#else
	swprintf(buf, 256, L"%d", iv);
#endif
    
	return buf;
}


std::wstring NHFormatStd(const wchar_t *format, ...){
	va_list list;
	wchar_t buf[4096];
	va_start(list, format);
#ifdef WIN32
	vswprintf(buf, format, list);
#else
    vswprintf(buf, 256, format, list);
#endif
	va_end(list);
	return buf;
}

std::wstring NHVarArgFormat(const std::wstring& format, va_list list){
	std::vector<const wchar_t *> args;
	const wchar_t *arg;
	
	while((arg=va_arg(list, const wchar_t *))!=NULL){
		args.push_back(arg);
	}
	
	if(args.empty())
		return format;
	
	std::wstring outStr;
	std::wstring::size_type pos=0;
	outStr.reserve(format.size());
	while(pos<format.size()){
		std::wstring::size_type newPos=format.find(L'{', pos);
		if(newPos==std::wstring::npos){
			outStr.append(format, pos, format.size()-pos);
			break;
		}
		outStr.append(format, pos, newPos-pos);
		pos=newPos+1;
		newPos=format.find(L'}', pos);
		if(newPos==std::wstring::npos)
			newPos=format.size();
		
		int argId=NHIntWithString(format.substr(pos, newPos-pos));
		pos=newPos+1;
		if(argId<0 || argId>=(int)args.size()){
			// over range.
			outStr+=L'{';
			outStr+=NHStringWithInt(argId);
			outStr+=L'}';
			continue;
		}
		
		arg=args[argId];
		if(arg)
			outStr+=arg;
		else
			outStr+=L"(null)";
		
	}
	return outStr;
}

std::wstring NHFormat(const std::wstring& format, ...){
	va_list list;
	va_start(list, format);
	std::wstring str=NHVarArgFormat(format, list);
	va_end(list);
	return str;
}

FILE *NHOpenStd(const std::wstring& path,
                const std::wstring& fileMode){
#ifdef WIN32
    return _wfopen(path.c_str(),
                   fileMode.c_str());
#else
    return fopen(twW2M(path).c_str(),
                 twW2M(fileMode).c_str());
#endif
}


FILE *NHReOpenStd(const std::wstring& path,
                const std::wstring& fileMode,
                  FILE *f){
#ifdef WIN32
    return _wfreopen(path.c_str(),
                   fileMode.c_str(),f);
#else
    return freopen(twW2M(path).c_str(),
                 twW2M(fileMode).c_str(),f);
#endif
}

#ifdef WIN32
std::vector<std::wstring> NHScanDirectory(const std::wstring& path){
	WIN32_FIND_DATA fd;
	HANDLE h;
	std::vector<std::wstring>  ret;
    std::wstring filePath;
	
	// open the Win32 find handle.
	h=FindFirstFileExW((path+L"\\*").c_str(),
					   FindExInfoStandard, &fd,
					   FindExSearchNameMatch,
					   NULL, 0);
	
	if(h==INVALID_HANDLE_VALUE){
		// couldn't open. return the empty vector.
		return ret;
	}
	
	do{
		// is it a directory?
		if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			// "." and ".." mustn't be included.
			if(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")){
				filePath=path+L"\\";
				filePath+=fd.cFileName;
				ret.push_back(filePath);
			}
		}else{
			// usual file.
			filePath=path+L"\\";
			filePath+=fd.cFileName;
			ret.push_back(filePath);
		}
		
		// iterate!
	}while(FindNextFile(h, &fd));
	
	// close the handle.
	FindClose(h);
	
	return ret;
}
#else
std::vector<std::wstring> NHScanDirectory(const std::wstring& path){
	// open the directory.
    std::string utf8Path=twW2M(path);
	DIR *dir=opendir(utf8Path.c_str());
	struct dirent *ent;
	
	std::vector<std::wstring>  ret;
    std::string filePath;
	
	// if couldn't open the directory, return the empty vector.
	if(!dir)
		return ret;
	
	// read an entry.
	while((ent=readdir(dir))){
		if(ent->d_name[0]=='.')
			continue;
		
		// make it full-path.
		filePath=utf8Path;
		filePath+="/";
		filePath+=ent->d_name;
		
		// add to the result vector.
		ret.push_back(twM2W(filePath));
	}
	
	// close the directory.
	closedir(dir);
	
	return ret;
}
#endif

#ifdef WIN32
void NHMakeDirectory(const std::wstring& path){
    CreateDirectory(path.c_str(), NULL);
}
#else
void NHMakeDirectory(const std::wstring& path){
    mkdir(twW2M(path).c_str(), 0766);
}
#endif

std::wstring NHLastPathComponent(const std::wstring& path){
    std::wstring::size_type pos;
    pos=path.rfind(NHPathSeparator);
    if(pos==std::wstring::npos)
        return path;
    return path.substr(pos+1);
}

static inline wchar_t easytolower(wchar_t in){
	if(in<=L'Z' && in>=L'A')
		return in-(L'Z'-L'z');
	return in;
} 

std::wstring NHToLower(const std::wstring& str){
	std::wstring result=str;
	std::transform(result.begin(), result.end(), result.begin(), easytolower);
	return result;
}

