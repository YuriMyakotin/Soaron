/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/


#include "Mailer.h"



lpLoginHeader lpFirstDataToSend;

void MakeFirstPackedDataTemplate(HANDLE hHeap)
{
	MixedBuffer UnpackedBuff;
	unsigned int MsgSize,PackedBuffSize,i;
	char * CurrentBuffPos;
	unsigned short MyAkasCount = 0;

	for (i = 0; i <= cfg.MaxAkaID; i++)
	{
		if (cfg.MyAkaTable[i].FullAddr != 0) ++MyAkasCount;
	}


	InitMixedBuffer(hHeap, &UnpackedBuff);
	AddWStrToMixedBuffer(&UnpackedBuff, cfg.SoftwareName);
	AddWStrToMixedBuffer(&UnpackedBuff, cfg.SystemName);
	AddWStrToMixedBuffer(&UnpackedBuff, cfg.SysopName);
	AddWStrToMixedBuffer(&UnpackedBuff, cfg.SystemLocation);
	AddWStrToMixedBuffer(&UnpackedBuff, cfg.SystemInfo);
	PackedBuffSize = PackData(hHeap, UnpackedBuff.lpBuffer, UnpackedBuff.CurrentSize, (char **)&lpFirstDataToSend, sizeof(LoginHeader) + sizeof(FTNAddr)*MyAkasCount);

	MsgSize = sizeof(LoginHeader) + sizeof(FTNAddr)*MyAkasCount + PackedBuffSize;
	SetMailerMsgHeader(lpFirstDataToSend, CMD_SRV_FIRST, MsgSize);

	lpFirstDataToSend->NumOfAkas = MyAkasCount;
	lpFirstDataToSend->ProtocolVersionHi = PROTO_VER_HI;
	lpFirstDataToSend->ProtocolVersionLo= PROTO_VER_LO;
	lpFirstDataToSend->Flags = 0;
	lpFirstDataToSend->UseEncryption = UseEncryption;
	lpFirstDataToSend->AcceptInsecureNetmail = AcceptInsecureNetmail;
	lpFirstDataToSend->PackedContentOriginalSize = UnpackedBuff.CurrentSize;
	
	CurrentBuffPos = (unsigned char *)(lpFirstDataToSend)+sizeof(LoginHeader);
	for (i = 0; i <= cfg.MaxAkaID; i++)
	{
		if (cfg.MyAkaTable[i].FullAddr != 0)
		{
			memcpy(CurrentBuffPos, &(cfg.MyAkaTable[i]), sizeof(FTNAddr));
			CurrentBuffPos += sizeof(FTNAddr);
		};
	}
//	memcpy(CurrentBuffPos, PackedBuff, PackedBuffSize);
//	HeapFree(hHeap, 0, PackedBuff);
	MixedBufferFreeMem(&UnpackedBuff);

}





DWORD WINAPI TcpServerMainThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;

	HANDLE hHeap,hThread;
	
	SOCKET ListenSocket[64];
	unsigned int SocketsCount,MaxSocketsCount,Id;
	WSAEVENT EventsTable[65];
	WSANETWORKEVENTS NE;
	DWORD EventNum;
	lpMailerSessionInfo SI;
	//int sa_len;
	//struct sockaddr accepted_sa;
	
	
	_InterlockedIncrement(&(cfg.ThreadCount));
	hHeap = GetProcessHeap();
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		printf("SQL ERROR\n");
		SetEvent(cfg.hExitEvent);
		goto exit;
		//fatal error
	}

	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	
	MakeFirstPackedDataTemplate(hHeap);
	
	SQLExecDirectW(hstmt, L"Select count(*) from ListenPorts", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_ULONG, &MaxSocketsCount, 0, NULL);


	SQLCloseCursor(hstmt);

	if (MaxSocketsCount == 0)
	{ 
		
		ADDRINFO Hints, *AddrInfo, *AI;
		SOCKET s;
		DWORD v6only = 0;
		int bindresult;
		char *pn = "8502";
		wchar_t AddrName[50];
		wchar_t LogStr[255];
		
		
		memset(&Hints, 0, sizeof(Hints));
		Hints.ai_family = PF_UNSPEC;
		Hints.ai_socktype = SOCK_STREAM;
		Hints.ai_flags = AI_PASSIVE;

		getaddrinfo(NULL, pn,&Hints, &AddrInfo);
		SocketsCount = 0;
		AI = AddrInfo;
		while (AI != NULL)
		{
			

			if (AI->ai_family == AF_INET6)
			{
				struct sockaddr_in6 * lpsa;
				lpsa = (struct sockaddr_in6 *)AI->ai_addr;
				s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
				if (s == INVALID_SOCKET)
				{
					LogNetworkError();
					break;
				}
				setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&v6only, sizeof(v6only));
				lpsa->sin6_port = htons(8502);
				InetNtopW(AF_INET6, &(lpsa->sin6_addr), AddrName, 50);
				bindresult = bind(s, (struct sockaddr *)lpsa, sizeof(struct sockaddr_in6));
				
			}
			else
			{
				struct sockaddr_in * lpsa;
				lpsa = (struct sockaddr_in *)AI->ai_addr;
				s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (s == INVALID_SOCKET)
				{
					LogNetworkError();
					break;
				}
				lpsa->sin_port = htons(8502);
				InetNtopW(AF_INET, &(lpsa->sin_addr), AddrName, 50);
				bindresult = bind(s, (struct sockaddr*)lpsa, sizeof(struct sockaddr_in));
			}


			if (bindresult==0)
			{
				wsprintfW(LogStr, L"Listen on %s (default port 8502)", AddrName);
				AddLogEntry(LogStr);
				ListenSocket[SocketsCount] = s;
				EventsTable[SocketsCount+1] = WSACreateEvent();
				WSAEventSelect(ListenSocket[SocketsCount], EventsTable[SocketsCount+1], FD_ACCEPT);
				listen(ListenSocket[SocketsCount], SOMAXCONN);
				++SocketsCount;

			}
			else
			{
				LogNetworkError();
				closesocket(s);
			};

			AI = AI->ai_next;
		}
		freeaddrinfo(AddrInfo);

		if (SocketsCount == 0)
		{
			AddLogEntry(L"Can't bind to any interface, server thread terminating"); 
			goto exit;
		}

	}
	else
	{
		wchar_t LogStr[255];
		wchar_t Addr[50];
		unsigned char isV6;
		universal_sa sa;

		int sz; 
		SocketsCount = 0;
		sqlret=SQLExecDirectW(hstmt, L"Select IpAddr,isIPV6 from ListenPorts", SQL_NTS);
		SQLBindCol(hstmt, 1, SQL_C_WCHAR, Addr, 100, NULL);
		SQLBindCol(hstmt, 2, SQL_C_BIT, &isV6, 0, NULL);

		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			if (isV6)
			{
				ListenSocket[SocketsCount] = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
				sa.sa6.sin6_family = AF_INET6;
				sz= sizeof(struct sockaddr_in6);
				WSAStringToAddressW(Addr, AF_INET6, NULL, &sa.sa, &sz);
			}
			else
			{
				ListenSocket[SocketsCount] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				sa.sa4.sin_family = AF_INET;
				sz = sizeof(struct sockaddr_in6);
				WSAStringToAddressW(Addr, AF_INET, NULL, &sa.sa, &sz);
			}
			if (bind(ListenSocket[SocketsCount], &sa.sa, sz) == 0)
			{
				EventsTable[SocketsCount + 1] = WSACreateEvent();
				WSAEventSelect(ListenSocket[SocketsCount], EventsTable[SocketsCount + 1], FD_ACCEPT);
				listen(ListenSocket[SocketsCount], SOMAXCONN);
				wsprintfW(LogStr, L"Listen on %s", Addr);
				AddLogEntry(LogStr);
				++SocketsCount;
			}
			else
			{
				LogNetworkError();
				closesocket(ListenSocket[SocketsCount]);
				
			}

			sqlret = SQLFetch(hstmt);
		}

	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_UNBIND);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);

	EventsTable[0] = cfg.hExitEvent;
loop:
	
	EventNum=WSAWaitForMultipleEvents(SocketsCount + 1, EventsTable, FALSE, INFINITE,FALSE);
	if (EventNum == 0) goto exit;
	WSAEnumNetworkEvents(ListenSocket[EventNum - 1], EventsTable[EventNum], &NE);
	if (NE.iErrorCode[FD_ACCEPT_BIT] != 0)
	{
		LogNetworkError();
		goto loop;
	}
	
	SI = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(MailerSessionInfo));
	SI->Sock = accept(ListenSocket[EventNum - 1], NULL, NULL);
	if (SI->Sock == INVALID_SOCKET)
	{
		LogNetworkError();
		HeapFree(hHeap, 0, SI);
		goto loop;
	}
	
	SI->SessionStatus = SESS_STAT_SRV_FIRST;
		
	hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))MailerSessionThread, (void *)SI, 0, &Id);
	CloseHandle(hThread);
	
	goto loop;


exit:
	_InterlockedDecrement(&(cfg.ThreadCount));
	return 0;
}

DWORD WINAPI MailerCallGeneratingThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;

	HANDLE hHeap, hThread;
	int result;
	unsigned int LinkID, cnt, Id;
	unsigned char isDialout;
	lpMailerSessionInfo SI;

	HANDLE hEvent[2];

	_InterlockedIncrement(&(cfg.ThreadCount));
	hHeap = GetProcessHeap();


	hEvent[0] = cfg.hExitEvent;
	hEvent[1] = cfg.hMailerCallGeneratingEvent;
	AddLogEntry(L"Mailer calls generating thread started");

loop:
	result = WaitForMultipleObjects(2, hEvent, FALSE, 30000);
	if (result == 0) goto threadexit;

	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		printf("SQL ERROR\n");
		SetEvent(cfg.hExitEvent);
		goto threadexit;
		//fatal error
	}

	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	sqlret = SQLExecDirectW(hstmt, L"select linkid, dialout, ((select count(Netmail.MessageId) from NetmailOutbound,Netmail where NetmailOutbound.ToLinkID=links.linkID and NetmailOutbound.MessageID=Netmail.MessageID and Netmail.Locked=0)+(select count(MessageId) from Outbound where Outbound.ToLink=links.linkID)+(select count(Id) from FileOutbound where FileOutbound.LinkID=links.linkID and FileOutbound.Delayed=0)) as C from Links where isnull(Links.IP,'')<>'' and Links.isBusy=0 and Links.isCalling=0 and Links.LinkType=3 and Links.HoldUntil<GetDate()", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLBindCol(hstmt, 1, SQL_C_ULONG, &LinkID, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_BIT, &isDialout, 0, NULL);
		SQLBindCol(hstmt, 3, SQL_C_ULONG, &cnt, 0, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			if ((cnt != 0) || (isDialout != 0))
			{
				SI = (lpMailerSessionInfo)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(MailerSessionInfo));
				SI->LinkID = LinkID;
				SI->SessionStatus = SESS_STAT_CLIENT_WAITIN_FIRST;
				hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))MailerSessionThread, (void *)SI, 0, &Id);
				CloseHandle(hThread);
			}
			sqlret = SQLFetch(hstmt);
		}
		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	SQLCloseCursor(hstmt);



	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	goto loop;
threadexit:

	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;


}