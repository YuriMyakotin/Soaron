/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

wchar_t * SoftwareName = L"SOARON 1.00";

//global variables, accessible from plugins
GlobalConfig cfg;

CRITICAL_SECTION NetmailRouteCritSect;

HANDLE hHeap;
HKEY FtnKey;

void LoadModules(HSTMT hstmt)
{
	wchar_t LogStr[255];
	wchar_t ModuleName[256];
	wchar_t ModuleFileName[256];
	wchar_t ModuleEventName[256];
	SQLRETURN sqlret;
	unsigned char RobotCode;
	lpModuleInfo lpModInfo;
	unsigned int IdModuleThread;
	HANDLE hModuleThread;

	HMODULE hLib;

	SQLBindCol(hstmt,1,SQL_C_WCHAR,ModuleName,512,NULL);
	SQLBindCol(hstmt,2,SQL_C_WCHAR,ModuleFileName,512,NULL);
	SQLBindCol(hstmt,3,SQL_C_WCHAR,ModuleEventName,512,NULL);
	sqlret=SQLExecDirectW(hstmt,L"select ModuleName,ModuleFileName,EventName from modules",SQL_NTS);

	if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
	{
		sqlret=SQLFetch(hstmt);
		while((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
		{
			lpModInfo=(lpModuleInfo)HeapAlloc(cfg.hMainHeap,0,sizeof(ModuleInfo));
			lpModInfo->ModuleName=(wchar_t *)HeapAlloc(cfg.hMainHeap,HEAP_ZERO_MEMORY,wcslen(ModuleName)*2+2);
			lpModInfo->ModuleFileName=(wchar_t *)HeapAlloc(cfg.hMainHeap,HEAP_ZERO_MEMORY,wcslen(ModuleFileName)*2+2);
			lpModInfo->ModuleEventName=(wchar_t *)HeapAlloc(cfg.hMainHeap,HEAP_ZERO_MEMORY,wcslen(ModuleEventName)*2+2);			
			lstrcpyW(lpModInfo->ModuleName,ModuleName);
			lstrcpyW(lpModInfo->ModuleFileName,ModuleFileName);
			lstrcpyW(lpModInfo->ModuleEventName,ModuleEventName);

			hModuleThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))ModuleThread,lpModInfo,0,&IdModuleThread);

			sqlret=SQLFetch(hstmt);
		}
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt,SQL_UNBIND);
	
	for(RobotCode=0;RobotCode<127;RobotCode++)
	{
		NetmailRobot[RobotCode]=NULL;
	}

	SQLBindCol(hstmt,1,SQL_C_UTINYINT,&RobotCode,0,NULL);
	SQLBindCol(hstmt,2,SQL_C_WCHAR,ModuleName,512,NULL);
	SQLBindCol(hstmt,3,SQL_C_WCHAR,ModuleFileName,512,NULL);
	sqlret=SQLExecDirectW(hstmt,L"select RobotsCode, RobotsName, RobotsFileName from NetmailRobots",SQL_NTS);
	
	if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
	{
		sqlret=SQLFetch(hstmt);
		while((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
		{
			if (RobotCode<127)
			{
				

				hLib=LoadLibraryW(ModuleFileName);
				if(hLib!=NULL)
				{
					NetmailRobot[RobotCode]=(NETMAILROBOTPROC) GetProcAddress(hLib,"NetmailRobotFunc");
					if (NetmailRobot[RobotCode]==NULL)
					{
						wsprintfW(LogStr, L"Loading NetmailRobot %s - Error, invalid module", ModuleName);
						AddLogEntry(LogStr);
						FreeLibrary(hLib);
					}
					else
					{
						wsprintfW(LogStr, L"Loading NetmailRobot %s - Ok.", ModuleName);
						AddLogEntry(LogStr);
					}
				}
				else
				{
					wsprintfW(LogStr, L"Loading NetmailRobot %s - FAIL", ModuleName);
					AddLogEntry(LogStr);
				}
			}
			else
			{
				wsprintfW(LogStr, L"Invalid Robot Number %u", RobotCode );
				AddLogEntry(LogStr);
				
			}


			
			sqlret=SQLFetch(hstmt);
		}
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt,SQL_UNBIND);

}

VOID CALLBACK DailyCallbackProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{
		SetEvent(cfg.hExitEvent);
		return;
		//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	
	SQLExecDirectW(hstmt, L"update links set NextArchiveExt=36, ThisDaySessions=0", SQL_NTS);
		
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	AddLogEntry(L"Daily reset done");
}


int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	unsigned int MyAkaID;
	FTNAddr MyAka;
	
	WSADATA wd;

	HANDLE hThread;
	unsigned int ThreadID;

	



	cfg.hExitEvent=CreateEventW(NULL,TRUE,FALSE,L"Global\\TosserStopEvent");

	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		CloseHandle(cfg.hExitEvent);
		printf("Another running copy detected. Exiting.\n");
		return 1;
	}
	
	hHeap=GetProcessHeap();
	cfg.hMainHeap=hHeap;
		
	if (argc<2)
	{
		printf("Error - no connection string\n");
		return 2;
	}
	wcscpy(cfg.ConnectionString, argv[1]);

	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{

		printf("Error - cannot start Winsock\n");
		return 5;
	}


	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(cfg.henv));
	SQLSetEnvAttr(cfg.henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3_80, 0); 
	SQLSetEnvAttr(cfg.henv,SQL_ATTR_CONNECTION_POOLING,(void*)SQL_CP_ONE_PER_HENV,0);

	cfg.hNetmailOutEvent=CreateEventW(NULL,FALSE,FALSE,L"Global\\NetmailOutEvent");
	cfg.hEchomailTossEvent=CreateEventW(NULL,FALSE,FALSE,L"Global\\EchomailOutEvent");
	cfg.hEchomailOutEvent=CreateEventW(NULL,FALSE,FALSE,L"Global\\EchomailToPktEvent");
	cfg.hPktInEvent=CreateEventW(NULL,FALSE,FALSE,L"Global\\PktInEvent");
	cfg.hLinksUpdateEvent=CreateEventW(NULL,FALSE,FALSE,L"Global\\LinksUpdateEvent");
	cfg.hSchedulerUpdateEvent = CreateEventW(NULL, FALSE, FALSE, L"Global\\SchedulerUpdateEvent");
	cfg.hMailerCallGeneratingEvent = CreateEventW(NULL, FALSE, FALSE, L"Global\\GenerateMailerCallEvent");
	
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc); 
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{

		printf("SQL Connection Error\n");
		return 3;
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


	AddLogEntry(L"Starting...");

	cfg.MyAddr.point=0;

	SQLExecDirectW(hstmt, L"{call sp_startup}", SQL_NTS);

	SQLExecDirectW(hstmt, L"select max(MyAkaID) from MyAka", SQL_NTS);
	sqlret = SQLFetch(hstmt);
	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{
		AddLogEntry(L"Error - no AKA configured");
		return 4;
	}
	SQLGetData(hstmt, 1, SQL_C_ULONG, &cfg.MaxAkaID, 0, NULL);
	SQLCloseCursor(hstmt);
	cfg.MyAkaTable = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(FTNAddr)*(cfg.MaxAkaID + 1));
	
	SQLExecDirectW(hstmt,L"select MyAkaId,Zone,Net,Node,Point from MyAka",SQL_NTS);
	SQLBindCol(hstmt, 1, SQL_C_ULONG, &MyAkaID, 0, NULL);
	SQLBindCol(hstmt, 2, SQL_C_USHORT, &(MyAka.zone), 0, NULL);
	SQLBindCol(hstmt, 3, SQL_C_USHORT, &(MyAka.net), 0, NULL);
	SQLBindCol(hstmt, 4, SQL_C_USHORT, &(MyAka.node), 0, NULL);
	SQLBindCol(hstmt, 5, SQL_C_USHORT, &(MyAka.point), 0, NULL);
	sqlret=SQLFetch(hstmt);
	while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		cfg.MyAkaTable[MyAkaID].FullAddr = MyAka.FullAddr;
		sqlret = SQLFetch(hstmt);
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_UNBIND);

	for (unsigned int i = 0; i <= cfg.MaxAkaID; i++)
	{
		if ((cfg.MyAkaTable[i].FullAddr != 0) && (cfg.MyAkaTable[i].point == 0))
		{
			cfg.MyAddr.FullAddr = cfg.MyAkaTable[i].FullAddr; //first node AKA = main AKA
			goto ok;
		}

	}
	AddLogEntry(L"Error - no node AKA configured");
	return 4;

	ok:

	cfg.SoftwareName = SoftwareName;

	cfg.BadInPktDir=GetString(hHeap,hstmt,L"BadInPktDir");
	cfg.BadOutPktDir = GetString(hHeap, hstmt, L"BadOutPktDir");
	cfg.BinkdPwdCfg = GetString(hHeap, hstmt, L"BinkdPwdCfg");
	cfg.BinkOutboundDir = GetString(hHeap, hstmt, L"BinkOutboundDir");
	cfg.FileboxesDir = GetString(hHeap, hstmt, L"FileboxesDir");
	cfg.InboundDir = GetString(hHeap, hstmt, L"InboundDir");
	cfg.InsecureInPktDir = GetString(hHeap, hstmt, L"InsecureInPktDir");
	cfg.TmailPwdCfg = GetString(hHeap, hstmt, L"TmailPwdCfg");
	cfg.TmpOutboundDir = GetString(hHeap, hstmt, L"TmpOutboundDir");
	cfg.NodelistDir = GetString(hHeap, hstmt, L"NodelistDir");
	cfg.UnzipCommand = GetString(hHeap, hstmt, L"UnzipCommand");
	cfg.ZipCommand = GetString(hHeap, hstmt, L"ZipCommand");
	cfg.SystemName = GetString(hHeap, hstmt, L"SystemName");
	cfg.SysopName = GetString(hHeap, hstmt, L"SysopName");
	cfg.SystemInfo = GetString(hHeap, hstmt, L"SystemInfo");
	cfg.SystemLocation = GetString(hHeap, hstmt, L"SystemLocation");

	

	cfg.RobotsAreaID = GetInt(hstmt, L"RobotsAreaID");

	MailerIdleTimeout = GetInt(hstmt, L"MailerIdleTimeout");
	if (MailerIdleTimeout == 0) MailerIdleTimeout = 60;
	MailerIdleCount = GetInt(hstmt, L"MailerMaxIdleCount");
	if (MailerIdleCount == 0) MailerIdleCount = 5;
	MailerRescanTime = GetInt(hstmt, L"MailerRescanTime");
	if (MailerRescanTime == 0) MailerRescanTime = 10;

	
	DefaultFileFrameSize = GetInt(hstmt, L"DefaultFileFrameSize");
	if ((DefaultFileFrameSize < 1024) || (DefaultFileFrameSize>65536)) DefaultFileFrameSize = 8192;
	
	FilePackMode = GetInt(hstmt, L"FilePackMode");
	if ((FilePackMode == 0) || (FilePackMode > 3)) FilePackMode = 2; //adaptive mode

	FileOverwrite = GetInt(hstmt, L"FileOverwriteEnabled");
	
	UseEncryption = GetInt(hstmt, L"MailerUseEncryption");
	
	AcceptInsecureNetmail = GetInt(hstmt, L"AcceptInsecureNetmail");

	LogsDisableConsoleOut = GetInt(hstmt, L"LogsDisableConsoleOut");

	InitializeCriticalSection(&NetmailRouteCritSect);

	SetConsoleTitleW(cfg.SoftwareName);



//init plugins

	LoadModules(hstmt);
	

	SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);

	{
		//daily timer for reset NextArchiveExt
		int timeval;
		SYSTEMTIME st;
		HANDLE hTimer;
		GetLocalTime(&st);
		timeval = 86400 - (st.wHour * 3600 + st.wMinute * 60 + st.wSecond);
		CreateTimerQueueTimer(&hTimer, NULL, (WAITORTIMERCALLBACK)DailyCallbackProc, (PVOID)NULL, timeval * 1000, 86400000, WT_EXECUTEDEFAULT);
		if (hTimer == INVALID_HANDLE_VALUE) printf("Timer set error!\n");
	}
	

	hThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))NetmailOutThread,NULL,0,&ThreadID);
	CloseHandle(hThread);
	hThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))EchomailOutThread,NULL,0,&ThreadID);
	CloseHandle(hThread);

	hThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))ConfigsThread,NULL,0,&ThreadID);
	CloseHandle(hThread);
	
	hThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))PktInTosserThread,NULL,0,&ThreadID);
	CloseHandle(hThread);
	
	hThread=(HANDLE)_beginthreadex(NULL,0,(unsigned int (__stdcall *)(void *))EchomailTossThread,NULL,0,&ThreadID);
	CloseHandle(hThread);

	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))ExternalSessionInfoPipesServerThread, NULL, 0, &ThreadID);
	CloseHandle(hThread);

	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))SchedulerThread, NULL, 0, &ThreadID);
	CloseHandle(hThread);

	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))TcpServerMainThread, NULL, 0, &ThreadID);
	CloseHandle(hThread);

	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))MailerCallGeneratingThread, NULL, 0, &ThreadID);
	CloseHandle(hThread);


	WaitForSingleObject(cfg.hExitEvent,INFINITE);
	
	while(cfg.ThreadCount>0)
	{
		
		WaitForSingleObject(cfg.hThreadEndEvent,INFINITE);
		
	}
	WSACleanup();
	return 0;
}

