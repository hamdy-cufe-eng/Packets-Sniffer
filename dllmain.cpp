// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#include <stdarg.h> 
#include <TlHelp32.h>
#include <vector>
#include <stdint.h>
#include <psapi.h>
#include <string.h>
#include <process.h>
#include <algorithm>
#include <random>
#include <iphlpapi.h>
#include <cctype>
#include <locale>
#include <intrin.h>																								// Required to display the hex properly
#include "detours.h"
#include <iomanip>
																								// Version 3.0 use for this hook. Be sure to include the library and includes to your project in visual studio
// Detours: https://www.microsoft.com/en-us/research/project/detours/

#pragma comment(lib,"detours.lib")																						// Need to include this so we can use Detours
#pragma comment(lib,"ws2_32.lib")																						// Required to hook Send and Recv since they both reside in this library


extern "C" {																											// Pointers to the original functions
	int (WINAPI* originalSend)(SOCKET s, const char* buf, int len, int flags) = send;									// https://msdn.microsoft.com/en-us/library/windows/desktop/ms740149(v=vs.85).aspx
	int (WINAPI* originalRecv)(SOCKET s, char* buf, int len, int flags) = recv;											// https://msdn.microsoft.com/en-us/library/windows/desktop/ms740121(v=vs.85).aspx
}
HMODULE hModule;

std::ofstream sendLog;
std::ofstream recvLog;

int WINAPI newSend(SOCKET s, char* buf, int len, int flags)																// Dumps each buffer to a new line in the "send.txt" file in the games directory
{
	sendLog.open("send.txt", std::ios::app);
	DWORD thiz = (DWORD)_ReturnAddress();
	sendLog << "Return Address :: 0x" << std::hex << thiz << std::endl;// Opens a handle to the send file
	for (int i = 0; i < len; i++) {																						// For each byte:
		sendLog << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)(unsigned char)buf[i] << " ";			//		Log the hex of the byte with a width of 2 (leading 0 added if necessary) and a space after to separate bytes
	}
	sendLog << std::endl;																								// Add a newline to the text file, indicating the end of this request
	sendLog.close();																									// Close the text file
	return originalSend(s, buf, len, flags);																			// Send the buffer to the original send function
}

int WINAPI newRecv(SOCKET s, char* buf, int len, int flags)																// Dumps each buffer to a new line in the "recv.txt" file in the games directory
{
	len = originalRecv(s, buf, len, flags);																				// Send the request with a pointer to the buffer for recv to store the response 
	recvLog.open("recv.txt", std::ios::app);																			// Opens a handle to the recv file
	DWORD thiz = (DWORD)_ReturnAddress();
	recvLog << "Return Address :: 0x" << std::hex << thiz << std::endl;
	for (int i = 0; i < len; i++) {																	

		recvLog << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)buf[i] << " ";						//		Log the hex of the byte with a width of 2 (leading 0 added if necessary) and a space after to separate bytes
	}
	recvLog << std::endl;																								// Add a newline to the text file, indicating the end of this request
	recvLog.close();																									// Close the text file

	return len;																											// Returns the output from the original recv call
}


void hook() {																											// Basic detours
	DisableThreadLibraryCalls(hModule);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)originalSend, newSend);
	DetourAttach(&(PVOID&)originalRecv, newRecv);
	DetourTransactionCommit();
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hook, NULL, 0, NULL);
		// g_utils->CheckDllActivated(hModule);
		// CheckForInvalidName();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}