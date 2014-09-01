/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include <windows.h>


int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	HANDLE hEvent;
	
	hEvent = CreateEventW(NULL,FALSE,FALSE, argv[1]);
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}
	return 0;
}