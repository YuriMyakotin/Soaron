/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"
DWORD WINAPI ModuleThread(LPVOID param)
{
	
	typedef BOOL (*MODULEPROC)(void); 


	HMODULE hLib;
	MODULEPROC ModuleFunc;
	wchar_t LogStr[255];

	HANDLE hEvent[2];
	int result;


	_InterlockedIncrement(&(cfg.ThreadCount));
	hEvent[0]=cfg.hExitEvent;
	hEvent[1]=CreateEvent(NULL,FALSE,FALSE,((lpModuleInfo)param)->ModuleEventName);
	wsprintfW(LogStr,L"%s Thread started",((lpModuleInfo)param)->ModuleName);
	AddLogEntry(LogStr);

loop:
	result=WaitForMultipleObjects(2,hEvent,FALSE,INFINITE);
	switch(result)
	{
		case (WAIT_OBJECT_0):
		{
			goto threadexit;
		}
		case (WAIT_OBJECT_0+1):
		{
			

			hLib=LoadLibrary(((lpModuleInfo)param)->ModuleFileName);
			if(hLib!=NULL)
			{
				ModuleFunc=(MODULEPROC) GetProcAddress(hLib,"SoaronModuleFunc");
				if (ModuleFunc==NULL)
				{
					wsprintfW(LogStr, L"Loading Module %s - Invalid Module", ((lpModuleInfo)param)->ModuleName);
					AddLogEntry(LogStr);
					
				}
				else
				{
					wsprintfW(LogStr, L"Loading Module %s - OK", ((lpModuleInfo)param)->ModuleName);
					AddLogEntry(LogStr);
					ModuleFunc();
				}
				FreeLibrary(hLib);

			}
			else
			{
				wsprintfW(LogStr, L"Loading Module %s - FAIL", ((lpModuleInfo)param)->ModuleName);
				AddLogEntry(LogStr);
			}


			goto loop;
		}
	}
	threadexit:

	
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}