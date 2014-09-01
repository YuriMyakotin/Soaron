/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

__declspec(align(1)) typedef struct tagExternalSessionInfo
{
	__declspec(align(1)) FTNAddr FtnAddress;
	unsigned char SoftwareCode;
} ExternalSessionInfo, *lpExternalSessionInfo;

DWORD WINAPI ExternalSessionInfoPipesReadThread(LPVOID param)
{
	ExternalSessionInfo BR;
	DWORD cb;
	wchar_t LogStr[255];
	_InterlockedIncrement(&(cfg.ThreadCount));
	HANDLE hPipe = (HANDLE)param;
	ReadFile(hPipe, &BR, sizeof(ExternalSessionInfo), &cb, NULL);
	switch (BR.SoftwareCode)
	{
		case LINK_TYPE_BINKP:wsprintfW(LogStr, L"Binkp session with %u:%u/%u.%u", BR.FtnAddress.zone, BR.FtnAddress.net, BR.FtnAddress.node, BR.FtnAddress.point);
			break;
		case LINK_TYPE_TMAILIP:wsprintfW(LogStr, L"Tmail-IP session with %u:%u/%u.%u", BR.FtnAddress.zone, BR.FtnAddress.net, BR.FtnAddress.node, BR.FtnAddress.point);
			break;
		default: goto check;
	}
		
	AddLogEntry(LogStr);
	check:
	LogSessionAndSendNetmailToLink(&(BR.FtnAddress), BR.SoftwareCode);
		
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	_InterlockedDecrement(&(cfg.ThreadCount));

	return 1;
}

DWORD WINAPI ExternalSessionInfoPipesServerThread(LPVOID param)
{
	HANDLE hPipe,hThread;
	BOOL   fConnected = FALSE;
	unsigned int IdBusyPipesReadThread;
	AddLogEntry(L"External SessionInfo Pipes thread started");

	begin:

	hPipe = CreateNamedPipeW( 
		  L"\\\\.\\pipe\\SoaronExternalSessionInfoPipe",
		  PIPE_ACCESS_INBOUND,       
		  PIPE_TYPE_MESSAGE |       
		  PIPE_READMODE_MESSAGE |   
		  PIPE_WAIT,                
		  PIPE_UNLIMITED_INSTANCES, 
		  0,                  
		  sizeof(ExternalSessionInfo), 
		  0,                  
		  NULL);              

		if (hPipe == INVALID_HANDLE_VALUE) 
		{
			printf("CreateNamedPipe failed, Error=%d \n", GetLastError()); 
			SetEvent(cfg.hExitEvent);
			return 0;
		}
		fConnected=ConnectNamedPipe(hPipe,NULL);
		fConnected = ConnectNamedPipe(hPipe, NULL) ? 
		 TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

		if (fConnected)
		{
			hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))ExternalSessionInfoPipesReadThread, (void *)hPipe, 0, &IdBusyPipesReadThread);
			CloseHandle(hThread);
		}
		else
			CloseHandle(hPipe);

		goto begin;
}

