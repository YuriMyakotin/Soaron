/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/


#include "Soaron.h"


__declspec(dllexport) BOOL SoaronModuleFunc(void)
{
	SQLHDBC  hdbc;
	HSTMT hstmt;
	SQLRETURN sqlret;
	SQLLEN cb;
	HANDLE hHeap;
	WStringBuffer Buf;
	unsigned int * Path;
	unsigned char tmp;
	int i;
	BOOL DupesExists=FALSE;
	unsigned short PathNet,PathNode,PrevNet;
	wchar_t AreaName[256];
	wchar_t Subj[256];
	wchar_t MsgId[128];
	wchar_t FromName[128];
	wchar_t ToName[128];
	wchar_t TmpStr[80];
	SQL_TIMESTAMP_STRUCT CTime,RTime1,RTime2;
	unsigned short zone, net, node, point;
	
	
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	hHeap = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);
	InitWStringBuffer(hHeap, &Buf);
	

	sqlret = SQLExecDirectW(hstmt, L"select EchoAreas.AreaName,links.Zone, Links.Net, Links.Node, Links.Point, DupesReport.ReceiveTime, DupesReport.CreateTime, DupesReport.FromName, DupesReport.ToName, DupesReport.Subject, DupesReport.MsgId, EchoMessages.ReceiveTime,DupesReport.Path, EchoMessages.Path  from DupesReport, EchoMessages, links, EchoAreas where DupesReport.OriginalMessageId = EchoMessages.MessageID and DupesReport.FromLinkId = Links.LinkID and DupesReport.AreaId = EchoAreas.AreaId", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLBindCol(hstmt, 1, SQL_C_WCHAR, AreaName, 512, NULL);
		SQLBindCol(hstmt, 2, SQL_C_USHORT, &zone, 0, NULL);
		SQLBindCol(hstmt, 3, SQL_C_USHORT, &net, 0, NULL);
		SQLBindCol(hstmt, 4, SQL_C_USHORT, &node, 0, NULL);
		SQLBindCol(hstmt, 5, SQL_C_USHORT, &point, 0, NULL);
		SQLBindCol(hstmt, 6, SQL_C_TYPE_TIMESTAMP, &RTime1, 0, NULL);
		SQLBindCol(hstmt, 7, SQL_C_TYPE_TIMESTAMP, &CTime, 0, NULL);
		SQLBindCol(hstmt, 8, SQL_C_WCHAR, FromName, 256, NULL);
		SQLBindCol(hstmt, 9, SQL_C_WCHAR, ToName, 256, NULL);
		SQLBindCol(hstmt, 10, SQL_C_WCHAR, Subj, 512, NULL);
		SQLBindCol(hstmt, 11, SQL_C_WCHAR, MsgId, 256, NULL);
		SQLBindCol(hstmt, 12, SQL_C_TYPE_TIMESTAMP, &RTime2, 0, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			DupesExists = TRUE;
			//
			AddWStrToBuffer(&Buf, L"Duplicate message in area ");
			AddWStrToBuffer(&Buf, AreaName);
			AddWStrToBuffer(&Buf, L":\rFrom: ");
			AddWStrToBuffer(&Buf, FromName);
			AddWStrToBuffer(&Buf, L"  To: ");
			AddWStrToBuffer(&Buf, ToName);
			wsprintfW(TmpStr, L"  %02u-%02u-%04u %02u:%02u:%02u",CTime.day,CTime.month,CTime.year,CTime.hour,CTime.minute,CTime.second);
			AddWStrToBuffer(&Buf, TmpStr);
			AddWStrToBuffer(&Buf, L"\rSubj: ");
			AddWStrToBuffer(&Buf, Subj);
			AddWStrToBuffer(&Buf, L"\rMSGID: ");
			AddWStrToBuffer(&Buf, MsgId);
			AddWStrToBuffer(&Buf, L"\rReceived from: ");
			wsprintfW(TmpStr, L"%u:%u/%u.%u", zone, net, node, point);
			AddWStrToBuffer(&Buf, TmpStr);
			AddWStrToBuffer(&Buf, L" in ");
			wsprintfW(TmpStr, L"%02u-%02u-%04u %02u:%02u:%02u", RTime1.day, RTime1.month, RTime1.year, RTime1.hour, RTime1.minute, RTime1.second);
			AddWStrToBuffer(&Buf, TmpStr);
			AddWStrToBuffer(&Buf, L"\rPATH:");
			
			SQLGetData(hstmt, 13, SQL_C_BINARY, &tmp, 0, &cb);
			Path = HeapAlloc(hHeap, 0, cb);
			SQLGetData(hstmt, 13, SQL_C_BINARY, Path, cb, NULL);
			PrevNet = 0;
			//
			for (i = 0; i < cb / 4; i++)
			{
				PathNet = Path[i] / 65536;
				PathNode = Path[i] % 65536;
				if (PathNet == PrevNet) wsprintfW(TmpStr, L" %u", PathNode);
				else
				{
					PrevNet = PathNet;
					wsprintfW(TmpStr, L" %u/%u", PathNet, PathNode);
				}
				AddWStrToBuffer(&Buf, TmpStr);
			}
			HeapFree(hHeap, 0, Path);
			AddWStrToBuffer(&Buf, L"\rOriginal message: received in ");
			wsprintfW(TmpStr, L"%02u-%02u-%04u %02u:%02u:%02u", RTime2.day, RTime2.month, RTime2.year, RTime2.hour, RTime2.minute, RTime2.second);
			AddWStrToBuffer(&Buf, TmpStr);
			AddWStrToBuffer(&Buf, L"\rPATH:");
			SQLGetData(hstmt, 14, SQL_C_BINARY, &tmp, 0, &cb);
			Path = HeapAlloc(hHeap, 0, cb);
			SQLGetData(hstmt, 14, SQL_C_BINARY, Path, cb, NULL);
			PrevNet = 0;
			//
			for (i = 0; i < (cb / 4)-1; i++)
			{
				PathNet = Path[i] / 65536;
				PathNode = Path[i] % 65536;
				if (PathNet == PrevNet) wsprintfW(TmpStr, L" %u", PathNode);
				else
				{
					PrevNet = PathNet;
					wsprintfW(TmpStr, L" %u/%u", PathNet, PathNode);
				}
				AddWStrToBuffer(&Buf, TmpStr);
			}
			HeapFree(hHeap, 0, Path);

			AddWStrToBuffer(&Buf, L"\r\r\r");
			sqlret = SQLFetch(hstmt);
		}


		
		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	SQLFreeStmt(hstmt, SQL_CLOSE);

	if (DupesExists)
	{
		PostEchomailMessage(hstmt, cfg.RobotsAreaID, cfg.SoftwareName, L"All", L"Dupes detected", &Buf);
		SetEvent(cfg.hEchomailTossEvent);
		AddLogEntry(L"Dupes report posted");
		SQLExecDirectW(hstmt, L"Delete from DupesReport", SQL_NTS);
	}
	WStringBufferFreeMem(&Buf);

	HeapDestroy(hHeap);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	return TRUE;
}