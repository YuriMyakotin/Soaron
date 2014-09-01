/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"


void PostEchomailMessage(HSTMT hstmt,unsigned int EchoAreaID, wchar_t * FromName, wchar_t * ToName, wchar_t * Subj, lpWStringBuffer lpMsgTextBuf)
{
	

	SQLRETURN sqlret;
	SQLLEN cb;
	unsigned int MyAkaId;
	wchar_t TmpStr[160];
	wchar_t MsgId[128];

	int OriginalSize, PackedSize;
	unsigned char * PackedBuf;


	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &EchoAreaID, 0, NULL);
	sqlret = SQLExecDirectW(hstmt, L"select UseAka from EchoAreas where EchoAreas.AreaId=?", SQL_NTS);

	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_ULONG, &MyAkaId, 0, NULL);

	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	

	
	
	if (cfg.MyAkaTable[MyAkaId].point == 0)
	{
		wsprintfW(MsgId, L"%u:%u/%u %08x", cfg.MyAkaTable[MyAkaId].zone, cfg.MyAkaTable[MyAkaId].net, cfg.MyAkaTable[MyAkaId].node, GetMsgIdTime(hstmt));
		wsprintfW(TmpStr, L"\r--- %s\r * Origin: %s (%u:%u/%u)\r", cfg.SoftwareName, cfg.SystemName, cfg.MyAkaTable[MyAkaId].zone, cfg.MyAkaTable[MyAkaId].net, cfg.MyAkaTable[MyAkaId].node);
	}
	else
	{
		wsprintfW(MsgId, L"%u:%u/%u.%u %08x", cfg.MyAkaTable[MyAkaId].zone, cfg.MyAkaTable[MyAkaId].net, cfg.MyAkaTable[MyAkaId].node, cfg.MyAkaTable[MyAkaId].point, GetMsgIdTime(hstmt));
		wsprintfW(TmpStr, L"\r--- %s\r * Origin: %s (%u:%u/%u.%u)\r", cfg.SoftwareName, cfg.SystemName, cfg.MyAkaTable[MyAkaId].zone, cfg.MyAkaTable[MyAkaId].net, cfg.MyAkaTable[MyAkaId].node, cfg.MyAkaTable[MyAkaId].point);
	}
	
	AddWStrToBuffer(lpMsgTextBuf, TmpStr);
	
	//
	OriginalSize =(int) wcslen(lpMsgTextBuf->lpBuffer) * 2 + 2;
	PackedSize = (int)(OriginalSize*1.001) + 12;
	PackedBuf = HeapAlloc(lpMsgTextBuf->hHeap, 0, PackedSize);
	compress2(PackedBuf, &PackedSize, (Bytef *)(lpMsgTextBuf->lpBuffer), OriginalSize, 9);

	//post message
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &EchoAreaID, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 36, 0, FromName, 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 36, 0, ToName, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(cfg.MyAkaTable[MyAkaId].zone), 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(cfg.MyAkaTable[MyAkaId].net), 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(cfg.MyAkaTable[MyAkaId].node), 0, NULL);
	SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(cfg.MyAkaTable[MyAkaId].point), 0, NULL);

	SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 74, 0, Subj, 0, NULL);
	SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, MsgId, 0, NULL);
	SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &OriginalSize, 0, NULL);
	SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 10000000, 0, PackedBuf, 0, &cb);
	cb = PackedSize;

	sqlret = SQLExecDirectW(hstmt, L"{call sp_PostEchomailMessage(?,?,?,?,?,?,?,?,?,?,?)}", SQL_NTS);

	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{
		printf("SQL Error\n");
		SetEvent(cfg.hExitEvent);
		
		//fatal error
	}
	


	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	HeapFree(lpMsgTextBuf->hHeap, 0, PackedBuf);
	;
}


void EchomailFreeMem(HANDLE hHeap, lpEchomailMessage lpEchomailMsg)
{
	HeapFree(hHeap, 0, lpEchomailMsg->FromName);
	HeapFree(hHeap, 0, lpEchomailMsg->ToName);
	HeapFree(hHeap, 0, lpEchomailMsg->Subject);
	if (lpEchomailMsg->MsgText != NULL) HeapFree(hHeap, 0, lpEchomailMsg->MsgText);
	if (lpEchomailMsg->MsgId != NULL) HeapFree(hHeap, 0, lpEchomailMsg->MsgId);
	if (lpEchomailMsg->ReplyTo != NULL) HeapFree(hHeap, 0, lpEchomailMsg->ReplyTo);
	if (lpEchomailMsg->AreaName != NULL) HeapFree(hHeap, 0, lpEchomailMsg->AreaName);
	if (lpEchomailMsg->SeenBy.lpBuffer != NULL) HeapFree(hHeap, 0, lpEchomailMsg->SeenBy.lpBuffer);
	if (lpEchomailMsg->Path.lpBuffer != NULL) HeapFree(hHeap, 0, lpEchomailMsg->Path.lpBuffer);
	HeapFree(hHeap, 0, lpEchomailMsg);
}


/*
-1 - ошибка SQL, выход
0 - все ок
1 - создана новая эха
2 - пользователь не имеет права создавать эху/писать в нее.
3 - подписан на уже существующую эху
*/

int CheckEchoArea(HANDLE hHeap, HSTMT hstmt, unsigned int FromLinkID, wchar_t * AreaName, unsigned int * lpEchoAreaID)
{
	int MsgStatus;
	SQLRETURN sqlret;


	SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &MsgStatus, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(FromLinkID), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 80, 0, AreaName, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, lpEchoAreaID, 0, NULL);
	sqlret = SQLExecDirectW(hstmt, L"{?=call  sp_CheckEchomailMessage(?,?,?)}", SQL_NTS);
	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{

		printf("SQL Error\n");
		SetEvent(cfg.hExitEvent);
		return ECHOAREA_CHECK_SQLERROR;
		//fatal error
	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	if ((MsgStatus == ECHOAREA_CHECK_NEW_AREA_CREATED) && (cfg.RobotsAreaID != 0))
	{
		//анонс новой эхи
		wchar_t Subj[72];
		wchar_t TmpStr[80];
		WStringBuffer Buf;
		unsigned short zone, net, node, point;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(FromLinkID), 0, NULL);
		SQLExecDirectW(hstmt, L"select Zone,Net,Node, Point from Links where LinkID=?", SQL_NTS);
		SQLFetch(hstmt);
		SQLGetData(hstmt, 1, SQL_C_USHORT, &zone, 0, NULL);
		SQLGetData(hstmt, 2, SQL_C_USHORT, &net, 0, NULL);
		SQLGetData(hstmt, 3, SQL_C_USHORT, &node, 0, NULL);
		SQLGetData(hstmt, 4, SQL_C_USHORT, &point, 0, NULL);
		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

		InitWStringBuffer(hHeap, &Buf);

		wsprintfW(Subj, L"New echoarea on %u:%u/%u", cfg.MyAddr.zone, cfg.MyAddr.net, cfg.MyAddr.node);
		wsprintfW(TmpStr, L"Echoarea %s created by %u:%u/%u.%u", AreaName, zone, net, node, point);
		AddLogEntry(TmpStr);
		AddWStrToBuffer(&Buf, TmpStr);
		AddWStrToBuffer(&Buf, L"\r");

		PostEchomailMessage(hstmt, cfg.RobotsAreaID, cfg.SoftwareName, L"All", Subj, &Buf);

		WStringBufferFreeMem(&Buf);
	}


	return MsgStatus;
}




/* 0 - все ок, не 0 = дуп */
int CheckDupes(HSTMT hstmt, unsigned int EchoAreaID, wchar_t * MsgId)
{
	
	int MsgStatus = 0;
	if (MsgId != NULL)
	{

		//проверка на дупы
	
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(EchoAreaID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, MsgId, 0, NULL);
		SQLExecDirectW(hstmt, L"select count(*) from DupeBase where AreaId=? and MsgId=?", SQL_NTS);
		SQLFetch(hstmt);
		SQLGetData(hstmt, 1, SQL_C_ULONG, &MsgStatus, 0, NULL);
		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	}
	return MsgStatus;

}

void AddDupeReport(HSTMT hstmt, lpEchomailMessage lpMsg)
{
	SQLLEN cb1;
	if (cfg.RobotsAreaID == 0) return;
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->EchoAreaID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->FromLinkID), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 36, 0, lpMsg->FromName, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 36, 0, lpMsg->ToName, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 19, 0, &(lpMsg->CreateTime), 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 74, 0, lpMsg->Subject, 0, NULL);
	SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, lpMsg->MsgId, 0, NULL);
	SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 60000, 0, lpMsg->Path.lpBuffer, 0, &cb1);
	cb1 = lpMsg->Path.CurrentSize * 4;

	SQLExecDirectW(hstmt, L"{call  sp_AddDupeReport(?,?,?,?,?,?,?,?)}", SQL_NTS);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

}


BOOL AddEchomailMessage(HANDLE hHeap,HSTMT hstmt,lpEchomailMessage lpMsg)
{
	
	SQLLEN cb0, cb1, cb2, cb3, cb4;
	SQLRETURN sqlret;
	int OriginalSize, PackedSize;
	unsigned char * PackedBuf;
	OriginalSize = (int)(wcslen(lpMsg->MsgText) * 2 + 2);
	PackedSize = (int)(OriginalSize*1.001) + 12;
	PackedBuf = HeapAlloc(hHeap, 0, PackedSize);
	compress2(PackedBuf, &PackedSize, (Bytef *)(lpMsg->MsgText), OriginalSize, 9);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->EchoAreaID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->FromLinkID), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 72, 0, lpMsg->FromName, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 72, 0, lpMsg->ToName, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.zone), 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.net), 0, NULL);
	SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.node), 0, NULL);
	SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.point), 0, NULL);
	SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 19, 0, &(lpMsg->CreateTime), 0, NULL);
	SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 144, 0, lpMsg->Subject, 0, NULL);

	if (lpMsg->MsgId == NULL)
	{
		SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, NULL, 0, &cb0);
		cb0 = SQL_NULL_DATA;
	}
	else 	SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, lpMsg->MsgId, 0, NULL);
	if (lpMsg->ReplyTo == NULL)
	{
		SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, NULL, 0, &cb1);
		cb1 = SQL_NULL_DATA;
	}
	else SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, lpMsg->ReplyTo, 0, NULL);

	SQLBindParameter(hstmt, 13, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &OriginalSize, 0, NULL);

	SQLBindParameter(hstmt, 14, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 10000000, 0, PackedBuf, 0, &cb2);
	cb2 = PackedSize;

	SQLBindParameter(hstmt, 15, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 60000, 0, lpMsg->Path.lpBuffer, 0, &cb3);
	cb3 = lpMsg->Path.CurrentSize * 4;

	SQLBindParameter(hstmt, 16, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 60000, 0, lpMsg->SeenBy.lpBuffer, 0, &cb4);
	cb4 = lpMsg->SeenBy.CurrentSize * 4;

	sqlret = SQLExecDirectW(hstmt, L"{call sp_Add_Echomail_Message(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}", SQL_NTS);
	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		printf("SQL Error\n");
		return FALSE;
	}


	HeapFree(hHeap, 0, PackedBuf);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	return TRUE;
	
}


/*
DWORD WINAPI EchomailTosserThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;

	
	int WaitTime;
	int WaitResult; 
	
	SQLLEN cb0,cb1,cb2,cb3,cb4;


	HANDLE WaitEvents[2];
	lpEchomailMessage lpMsg;

	int MsgStatus;


	
	_InterlockedIncrement(&(cfg.ThreadCount));

	WaitEvents[0]=cfg.hExitEvent;
	WaitEvents[1]=EchomailQueue.hNewEchomailMsgEvent;

	WaitTime=INFINITE;
	AddLogEntry(L"Echomail processing thread started");

WaitForNewMsg:

	WaitResult=WaitForMultipleObjects(2,WaitEvents,FALSE,WaitTime);
	if (WaitResult==WAIT_TIMEOUT)
	{
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
		WaitTime=INFINITE;
		HeapCompact(hEchomailHeap,0);

		goto WaitForNewMsg;
	}

	if (WaitTime==INFINITE)
	{
		SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc); 
		sqlret=SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
		if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
		{
			SetEvent(cfg.hExitEvent);
			goto threadexit;
			//fatal error
		}
		SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
		WaitTime=5000;
	}
	switch(WaitResult)
	{
		case WAIT_OBJECT_0: goto threadexit;
		case (WAIT_OBJECT_0+1): goto StartTossCycle;
	}

StartTossCycle:
//
lpMsg=GetEchomailFromQueue();
if (lpMsg==NULL) goto CheckMoreMail;

if (lpMsg->EchoAreaID != 0) goto AddMessage; //Уже проверено, мессага поступила через прямой линк с другим испльзующим FTN узлом
MsgStatus = GetEchoMsgStatus(hstmt, lpMsg);
if (MsgStatus == -1) goto threadexit;

if (MsgStatus == 2) goto DelMsg;
	
MsgStatus = CheckDupes(hstmt, lpMsg);
if (MsgStatus != 0) goto DelMsg;
		
AddMessage:

{
	//add echomail message
	int OriginalSize, PackedSize;
	unsigned char * PackedBuf;
	OriginalSize = (int)(wcslen(lpMsg->MsgText) * 2 + 2);
	PackedSize = (int)(OriginalSize*1.001) + 12;
	PackedBuf = HeapAlloc(hEchomailHeap, 0, PackedSize);
	compress2(PackedBuf, &PackedSize, (Bytef *)(lpMsg->MsgText), OriginalSize, 9);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->EchoAreaID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpMsg->FromLinkID), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 72, 0, lpMsg->FromName, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 72, 0, lpMsg->ToName, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.zone), 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.net), 0, NULL);
	SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.node), 0, NULL);
	SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpMsg->FromAddr.point), 0, NULL);
	SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 19, 0, &(lpMsg->CreateTime), 0, NULL);
	SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 144, 0, lpMsg->Subject, 0, NULL);

	if (lpMsg->MsgId == NULL)
	{
		SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, NULL, 0, &cb0);
		cb0 = SQL_NULL_DATA;
	}
	else 	SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, lpMsg->MsgId, 0, NULL);

	if (lpMsg->ReplyTo == NULL)
	{
		SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, NULL, 0, &cb1);
		cb1 = SQL_NULL_DATA;
	}
	else SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 127, 0, lpMsg->ReplyTo, 0, NULL);

	SQLBindParameter(hstmt, 13, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &OriginalSize, 0, NULL);

	
	SQLBindParameter(hstmt, 14, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 10000000, 0, PackedBuf, 0, &cb2);
	cb2 = PackedSize;

	SQLBindParameter(hstmt, 15, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 60000, 0, lpMsg->Path.lpBuffer, 0, &cb3);
	cb3 = lpMsg->Path.CurrentSize * 4;

	SQLBindParameter(hstmt, 16, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, 60000, 0, lpMsg->SeenBy.lpBuffer, 0, &cb4);
	cb4 = lpMsg->SeenBy.CurrentSize * 4;

	sqlret = SQLExecDirectW(hstmt, L"{call sp_Add_Echomail_Message(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}", SQL_NTS);
	if ((sqlret != SQL_SUCCESS) && (sqlret != SQL_SUCCESS_WITH_INFO))
	{
		
		printf("SQL Error\n");
		SetEvent(cfg.hExitEvent);
		goto threadexit;
	}
	

	HeapFree(hEchomailHeap, 0, PackedBuf);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
}
	





DelMsg:
EchomailFreeMem(lpMsg);

CheckMoreMail:
	EnterCriticalSection(&(EchomailQueue.CritSect));
	if (EchomailQueue.First==NULL)
	{
		ResetEvent(EchomailQueue.hNewEchomailMsgEvent);
		SetEvent(cfg.hEchomailTossEvent);
	}
	LeaveCriticalSection(&(EchomailQueue.CritSect));
	goto WaitForNewMsg;

threadexit: 
	
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;

}
*/