//
//  main.cpp
//  Agiletimbres
//
//  Created by Tomoaki Kawada on 11/10/11.
//  Copyright (c) 2011 Nexhawks. All rights reserved.
//

#include "StdAfx.h"
#include "Utils.h"
#include "NHHardRoundSkin.h"
#include "ATMainWindow.h"
#include "NHTcwHelvetica.h"
#include "ATSynthesizer.h"
#include "TXTestInstrument.h"
#include "TXGPDS1.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "main.h"
#include "MidiInputOSX.h"
#include "TXLegacyMidi.h"
#include "ATSplashWindow.h"

std::wstring ATGetAppDir(){
#ifdef WIN32
    wchar_t buf[512];
	// retrive the path of the application.
	GetModuleFileName(NULL, buf, 512);
	
	std::wstring str=buf;
	// find the last path separator.
	std::wstring::size_type pos=MIN(str.rfind(L"/"), str.rfind(L"\\"));
	
	if(pos==std::string::npos)
		// ... only to find the base name. no directory infomation available.
		return L"";
	else
		// retrive the directory name.
		return str.substr(0, pos);
#else
    return L".";
#endif
}

std::wstring ATVersion(){
    return L"0.0.2";
}

std::wstring ATResourcesDir(){
    return ATGetAppDir()+NHPathSeparator L"Resources";
}

std::wstring ATUserLibraryDir(){
    std::wstring path;
#ifdef WIN32
    path=ATGetAppDir()+NHPathSeparator L"Library";
#elif defined(__MACOSX__)
    path=twM2W(getenv("HOME"));
    path+=NHPathSeparator 
    L"/Library/Application Support/Agiletimbres";
#else
    path=twM2W(getenv("HOME"));
    path+=NHPathSeparator 
    L"/.Agiletimbres";
#endif
    NHMakeDirectory(path);
    return path;
}

std::wstring ATBuiltinPluginSettingsDir(){
    return ATResourcesDir()+NHPathSeparator L"Plugin Settings";
}

std::wstring ATBuiltinPluginSettingsDirForPlugin(const std::string& identifier){
    return ATBuiltinPluginSettingsDir()+NHPathSeparator+
    twM2W(identifier);
}

std::wstring ATBuiltinSampleFilesDir(){
    return ATResourcesDir()+NHPathSeparator L"Sample Files";
}

std::wstring ATBuiltinSampleFilesDirForPlugin(const std::string& identifier){
    return ATBuiltinSampleFilesDir()+NHPathSeparator+
    twM2W(identifier);
}

std::wstring ATUserPluginSettingsDir(){
    std::wstring path;
    path=ATUserLibraryDir()+NHPathSeparator L"Plugin Settings";
    NHMakeDirectory(path);
    return path;
}

std::wstring ATUserPluginSettingsDirForPlugin(const std::string&identifier){
    return ATUserPluginSettingsDir()+NHPathSeparator+
    twM2W(identifier);
}

#ifdef WIN32

typedef PVOID WINAPI (*AddVectoredExceptionHandlerFunc)(ULONG FirstHandler,
														LPTOP_LEVEL_EXCEPTION_FILTER VectoredHandler
														);


/** convert the win32 error code to string, and throw it as XTBWin32Exception. */
static void XTBThrowWin32Exception(DWORD code){
	
	throw code;
	
	// throw the exception.
	//throw SCException(SCFormatStd(L"Win32 Exception 0x%08x.", (int)code));
}

int XTBHandleWin32Exception(struct _EXCEPTION_RECORD* ExceptionRecord, void *, struct _CONTEXT*, struct _DISPATCHER_CONTEXT*){
	DWORD code=ExceptionRecord->ExceptionCode;
	XTBThrowWin32Exception(code);
	return 0;
}

static LONG XTBUnhandledWin32ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo){
	XTBThrowWin32Exception(ExceptionInfo->ExceptionRecord->ExceptionCode);
	return 0;
}

#endif

twApp *twCreateApp(){
	return new twApp();
}

twEvent *twCreateEvent(){
	return new twSDLEvent();//WGSDLEvent();
}

extern void twThreadStarted();
void twThreadStarted(){
	
}

extern "C" int main (int argc,char * argv[])
{
#ifdef AT_EMBEDDED
    
	if(wcsstr(GetCommandLine(), L"Opened")==0){
		
		// need to open another instance because
		// SHARP Simulator waits for this app to exit.
		
		wchar_t buf[512];
		// retrive the path of the application.
		GetModuleFileName(NULL, buf, 512);
		
		SHELLEXECUTEINFO info;
		
		memset(&info, 0, sizeof(info));
		
		info.cbSize=sizeof(SHELLEXECUTEINFO);
		info.lpFile=buf;
		info.lpParameters=L"Opened";
		
		ShellExecuteEx(&info);
		
		return 0;
	}
	
	if(FindWindow(NULL, L"Nexhawks Agiletimbres")){
		Sleep(1000);
		ShowWindow(FindWindow(NULL, L"Nexhawks Agiletimbres"), SW_SHOW);
		SetForegroundWindow(FindWindow(NULL, L"Nexhawks Agiletimbres"));
		SetActiveWindow(FindWindow(NULL, L"Nexhawks Agiletimbres"));
		return 0;
	}
#endif
    
    // open the log file for XTBLog to output to.
#ifdef WIN32
	NHReOpenStd((ATGetAppDir()+L"/stdout.txt").c_str(), L"w", stdout);
#endif
	//setvbuf(stdout, NULL, _IONBF,0);
	puts("Agiletembres started.");
    
#ifdef WIN32
	
	// register the exception handler.
	
	// in Windows CE, SetUnhandledExceptionFilter is undefined.
	// so let's use AddVectoredExceptionHandler that exists in only Windows CE Embedded 6.
	AddVectoredExceptionHandlerFunc AddVectoredExceptionHandler;
	AddVectoredExceptionHandler=(AddVectoredExceptionHandlerFunc)
	GetProcAddress(LoadLibrary(L"COREDLL"),
				   L"AddVectoredExceptionHandler");
	if(AddVectoredExceptionHandler){
		(*AddVectoredExceptionHandler)(1, XTBUnhandledWin32ExceptionFilter);
	}else{
		MessageBox(NULL, L"AddVectoredExceptionHandler failed.", NULL,
				   MB_ICONERROR);
	}
#endif
    
    // create data dircetory.
    ATUserLibraryDir();
    ATUserPluginSettingsDir();
	
	// initialize TCW.
	twInitStruct initStruct;
	initStruct.title=L"Nexhawks Agiletimbres";
	initStruct.scrSize=twSize(480, 320);
	initStruct.fullScreen=false;
	initStruct.resizable=false;
#ifdef AT_EMBEDDED
    initStruct.fullScreen=true;
#endif
	twInit(initStruct);
	atexit(twExit);
    
	
	tw_curSkin=new NHHardRoundSkin();
    
	tw_defFont=new NHTcwHelvetica();
    
	try{
        
		
		
	}catch(const std::exception& ex){
		std::wstring str=twM2W(ex.what());
		twMsgBox(NULL, str, twMBB_ok, L"Error", true);
		
	}catch(...){
		twMsgBox(NULL, L"Unknown exception. Exiting.", twMBB_ok, L"Error", true);
		
		
	}
	
	
	
	SDL_EnableKeyRepeat(0, 0);
	
#if USE_MIDIINPUT_OSX
    initMidiInputOSX();
#endif

    try{
		// later, ATSplashWindow calls initializeMain().
		twSetDesktop(new ATSplashWindow());
        tw_event->mainLoop();
    }catch(const std::exception& ex){
        std::wstring str=twM2W(ex.what());
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }
#ifdef WIN32
    catch(DWORD ex){
        std::wstring str=NHFormatStd(L"Win32 Exception 0x%08X. Aborting.", (int)ex);
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }
#endif
    catch(...){
        twMsgBox(NULL, L"Unknown exception. Exiting.", twMBB_ok, L"Error", true);
        
        
    }

	

    
    return 0;
}

void initializeMain(){
	try{
        // load txmidi.tbl
		std::wstring tablePath=ATBuiltinSampleFilesDirForPlugin("com.nexhawks.TXSynthesizer.LegacyMidiInstrument");
		tablePath+=NHPathSeparator;
		tablePath+=L"txmidis.tbl";
		FILE *f=NHOpenStd(tablePath, L"ab+");
		if(f){
			TXLegacyMidi::setBankFile(f);
		}
		
		if(!f){
			tablePath=ATBuiltinSampleFilesDirForPlugin("com.nexhawks.TXSynthesizer.LegacyMidiInstrument");
			tablePath+=NHPathSeparator;
			tablePath+=L"txmidi.tbl";
			FILE *f=NHOpenStd(tablePath, L"ab+");
			if(f){
				TXLegacyMidi::setBankFile(f);
			}
		}
		
		if(!f){
			tablePath=ATBuiltinSampleFilesDirForPlugin("com.nexhawks.TXSynthesizer.LegacyMidiInstrument");
			tablePath+=NHPathSeparator;
			tablePath+=L"txmidii.tbl";
			FILE *f=NHOpenStd(tablePath, L"ab+");
			if(f){
				TXLegacyMidi::setBankFile(f);
			}
		}
		
		// load voices.tbl
		std::wstring voicePath=ATBuiltinSampleFilesDirForPlugin("com.nexhawks.TXSynthesizer.LegacyMidiInstrument");
		voicePath+=NHPathSeparator;
		voicePath+=L"voices.tbl";
		f=NHOpenStd(voicePath, L"ab+");
		if(f){
			TXLegacyMidi::setVoicesFile(f);
		}
		
		// load synthesizer.
		ATSynthesizer::sharedSynthesizer()->
		setInstrument(new TXGPDS1
					  (ATSynthesizer::sharedSynthesizer()->
					   config()));
		
		
		twSetDesktop(new ATMainWindow());
    }catch(const std::exception& ex){
        std::wstring str=twM2W(ex.what());
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }catch(const std::string& ex){
        std::wstring str=twM2W(ex);
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }catch(const std::wstring& ex){
        std::wstring str=(ex);
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }
#ifdef WIN32
    catch(DWORD ex){
        std::wstring str=NHFormatStd(L"Win32 Exception 0x%08X. Aborting.", (int)ex);
        twMsgBox(NULL, str, twMBB_ok, L"Error", true);
        
    }
#endif
    catch(...){
        twMsgBox(NULL, L"Unknown exception. Exiting.", twMBB_ok, L"Error", true);
        
        
    }
	
}

bool ATIsCertainlySlowDevice(){
#if defined(WIN32) && AT_EMBEDDED
	static bool *res=NULL;
	if(!res){
		res=new bool;
		HMODULE md=LoadLibrary(L"SHARPLIB");
		if(md){
			void *prc=(void *)GetProcAddress(md, L"Disp_SCUC_TTF");
			if(prc)
				*res=false;
			else
				*res=true;
		}else{
			*res=false;
		}
	}
	return *res;
#else
	return false;
#endif
}

