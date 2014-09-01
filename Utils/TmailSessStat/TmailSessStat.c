/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include <windows.h>


__declspec(align(1)) typedef struct tagFTNAddr
{
	
		WORD zone;
		WORD net;
		WORD node;
		WORD point;

} FTNAddr, *lpFTNAddr;


__declspec(align(1)) typedef struct tagBsyRequest
{
	
		FTNAddr FtnAddress;
		BYTE SoftwareCode;
		
} SessionInfo, *lpSessionInfo;


int main(int argc, char *argv[], char *envp[])
{

int z,ne,no,p;
char *tmp1;

SessionInfo SI;
DWORD cb;
HANDLE hPipe;


	tmp1=argv[1];
	z=atoi(tmp1);
	tmp1=strchr(tmp1,':')+1;
	ne=atoi(tmp1);
	tmp1=strchr(tmp1,'/')+1;
	no=atoi(tmp1);
	tmp1=strchr(tmp1,'.')+1;
	p=atoi(tmp1);


	SI.SoftwareCode = 2;
	SI.FtnAddress.zone = z;
	SI.FtnAddress.net = ne;
	SI.FtnAddress.node = no;
	SI.FtnAddress.point = p;


	hPipe = CreateFileW(L"\\\\.\\pipe\\SoaronExternalSessionInfoPipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		WriteFile(hPipe, &SI, sizeof(SessionInfo), &cb, NULL);
		CloseHandle(hPipe);

	}
	

	return 0;
}

