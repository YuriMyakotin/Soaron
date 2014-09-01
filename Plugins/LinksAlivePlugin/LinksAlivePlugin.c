/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"


typedef struct tagLinksCheckInfo
{
	unsigned int LinkID;
	FTNAddr LinkAddr;
	SOCKET s;
	struct sockaddr_in sa;
} LinksCheckInfo, *lpLinksCheckInfo;

__declspec(dllexport) BOOL SoaronModuleFunc(void)
{
	SQLHDBC  hdbc;
	HSTMT hstmt;
	unsigned int NumOfLinks, LinkID;
	WSADATA wd;
	SQLRETURN sqlret;
	HANDLE hHeap;
	FTNAddr LinkAddr;
	wchar_t Ip[256];

	lpLinksCheckInfo LinksTable;
	WSAEVENT * EventsTable;
	WSANETWORKEVENTS evt;
	unsigned int i, j;
	int res;

	ADDRINFOW *result = NULL;
	ADDRINFOW hints;
	wchar_t LogStr[255];
	memset(&hints, 0, sizeof(hints));

	LinkAddr.point = 0;
	NumOfLinks = 0;
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	AddLogEntry(L"Checking binkp links alive");
	//get links info
	SQLExecDirectW(hstmt, L"update Links set DialOut=0 where datediff(minute,LastSessionTime,GetDate())>40 and LinkType=1", SQL_NTS);
	sqlret = SQLExecDirectW(hstmt, L"select count(*) from Links where dialout=0 and passivelink=0 and LinkType=1 and isnull(ip,'')<>''", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret = SQL_SUCCESS_WITH_INFO))
	{
		sqlret = SQLFetch(hstmt);
		if ((sqlret == SQL_SUCCESS) || (sqlret = SQL_SUCCESS_WITH_INFO))
		{
			SQLGetData(hstmt, 1, SQL_C_ULONG, &NumOfLinks, 0, NULL);
		}
	}
	SQLCloseCursor(hstmt);
	if (NumOfLinks == 0) goto exit;
	hHeap = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);
	LinksTable = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, NumOfLinks*sizeof(LinksCheckInfo));
	EventsTable = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(WSAEVENT)*NumOfLinks);
	i = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	WSAStartup(MAKEWORD(2, 2), &wd);
	sqlret = SQLExecDirectW(hstmt, L"select LinkID,Zone,Net,Node, Ip from Links where dialout=0 and passivelink=0 and LinkType=1 and isnull(ip,'')<>''", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret = SQL_SUCCESS_WITH_INFO))
	{
		
		SQLBindCol(hstmt, 1, SQL_C_ULONG, &LinkID, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_USHORT, &(LinkAddr.zone), 0, NULL);
		SQLBindCol(hstmt, 3, SQL_C_USHORT, &(LinkAddr.net), 0, NULL);
		SQLBindCol(hstmt, 4, SQL_C_USHORT, &(LinkAddr.node), 0, NULL);
		SQLBindCol(hstmt, 5, SQL_C_WCHAR, Ip, 512, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			//


			res = GetAddrInfoW(Ip, L"24554", &hints, &result);
			if (res == 0)
			{

				memcpy(&(LinksTable[i].sa), result->ai_addr, sizeof(struct sockaddr_in));
				//				printf("%u  %u %S %u.%u.%u.%u\n", i,LinkID, Ip, LinksTable[i].sa.sin_addr.S_un.S_un_b.s_b1, LinksTable[i].sa.sin_addr.S_un.S_un_b.s_b2, LinksTable[i].sa.sin_addr.S_un.S_un_b.s_b3, LinksTable[i].sa.sin_addr.S_un.S_un_b.s_b4);

				LinksTable[i].LinkID = LinkID;
				LinksTable[i].LinkAddr.FullAddr = LinkAddr.FullAddr;
				LinksTable[i].s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				EventsTable[i] = WSACreateEvent();
				WSAEventSelect(LinksTable[i].s, EventsTable[i], FD_CONNECT);
				FreeAddrInfoW(result);
				++i;
			}
			else
			{
				wsprintfW(LogStr,L"%u:%u/%u: Ip address %s cannot be resolved", LinkAddr.zone, LinkAddr.net, LinkAddr.node,Ip);
				AddLogEntry(LogStr);
			}


			sqlret = SQLFetch(hstmt);

		}

	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_UNBIND);
	NumOfLinks = i;
	for (i = 0; i < NumOfLinks; i++)
	{
		connect(LinksTable[i].s, (struct sockaddr *)&(LinksTable[i].sa), sizeof(struct sockaddr_in));
	}
	j = 0;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &LinkID, 0, NULL);
	SQLPrepareW(hstmt, L"Update Links set DialOut=1 where LinkID=?", SQL_NTS);

	while (j < NumOfLinks)
	{
		res = WSAWaitForMultipleEvents(NumOfLinks, EventsTable, FALSE, WSA_INFINITE, FALSE);
		WSAEnumNetworkEvents(LinksTable[res].s, EventsTable[res], &evt);
		shutdown(LinksTable[res].s, SD_BOTH);
		closesocket(LinksTable[res].s);
		if (evt.iErrorCode[FD_CONNECT_BIT] == 0)
		{
			wsprintfW(LogStr, L"%u:%u/%u: OK", LinksTable[res].LinkAddr.zone, LinksTable[res].LinkAddr.net, LinksTable[res].LinkAddr.node);
			LinkID = LinksTable[res].LinkID;
			SQLExecute(hstmt);
		}
		else
		{
			switch (evt.iErrorCode[FD_CONNECT_BIT])
			{
				case WSAECONNREFUSED:
					wsprintfW(LogStr, L"%u:%u/%u: Connection refused", LinksTable[res].LinkAddr.zone, LinksTable[res].LinkAddr.net, LinksTable[res].LinkAddr.node);
					break;
				case WSAETIMEDOUT:
					wsprintfW(LogStr, L"%u:%u/%u: Connection timed out", LinksTable[res].LinkAddr.zone, LinksTable[res].LinkAddr.net, LinksTable[res].LinkAddr.node);
					break;
				default:wsprintfW(LogStr, L"%u:%u/%u: Unknown connection error", LinksTable[res].LinkAddr.zone, LinksTable[res].LinkAddr.net, LinksTable[res].LinkAddr.node);

			
			}

		}
		AddLogEntry(LogStr);
		++j;
	}
	//

	for (j = 0; j < NumOfLinks; j++)
	{
		WSACloseEvent(EventsTable[j]);
	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	//
	WSACleanup();
	HeapDestroy(hHeap);
exit:
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	AddLogEntry(L"Links alive check done");
	SetEvent(cfg.hLinksUpdateEvent);
	return TRUE;
}