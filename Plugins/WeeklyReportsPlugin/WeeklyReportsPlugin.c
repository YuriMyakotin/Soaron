/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/



#include "Soaron.h"

SQLHDBC  hdbc;
HSTMT hstmt;
SQLRETURN sqlret;
SQLLEN cb;
HANDLE hHeap;
WStringBuffer Buf;

void PostRules(void)
{
	unsigned int AreaId;
	wchar_t FromName[36], ToName[36], Subj[74];
	wchar_t tmp;
	SQLHDBC  hdbc1;
	HSTMT hstmt1;
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc1);
	SQLDriverConnectW(hdbc1, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt1);

	sqlret = SQLExecDirectW(hstmt, L"SELECT Areaid, FromName, ToName, Subject,RulesText From RulesBase", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLBindCol(hstmt, 1, SQL_C_ULONG, &AreaId, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_WCHAR, FromName, 72, NULL);
		SQLBindCol(hstmt, 3, SQL_C_WCHAR, ToName, 72, NULL);
		SQLBindCol(hstmt, 4, SQL_C_WCHAR, Subj, 148, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{

			SQLGetData(hstmt, 5, SQL_C_WCHAR, &tmp, 0, &cb);
			AllocWStringBuffer(hHeap, &Buf, cb-2);
			SQLGetData(hstmt, 5, SQL_C_WCHAR, Buf.lpBuffer, cb, NULL);
			PostEchomailMessage(hstmt1, AreaId, FromName, ToName, Subj, &Buf);
			WStringBufferFreeMem(&Buf);
			sqlret = SQLFetch(hstmt);
		}


		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	SQLFreeStmt(hstmt,SQL_CLOSE);

	SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);
	SQLDisconnect(hdbc1);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);
	;
}

void PostOutboundStat(void)
{
	unsigned short zone, net, node;
	unsigned int OutboundSize;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t DirStr[MAX_PATH],TmpStr[80],TmpStr1[20],TmpStr2[30];
	SYSTEMTIME st;
	ULARGE_INTEGER ft_curr, ft_prev;
	FILETIME ft;
	InitWStringBuffer(hHeap,&Buf);
	AddWStrToBuffer(&Buf, L"\r                      Bytes in outbound            Outbound Age          \r==========================================================================\r");

	//

	sqlret = SQLExecDirectW(hstmt, L"SELECT Zone,Net,Node from Links where Point=0 and LinkType<3 and PassiveLink=0 order by Zone,Net,Node", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLBindCol(hstmt, 1, SQL_C_USHORT, &zone, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_USHORT, &net, 0, NULL);
		SQLBindCol(hstmt, 3, SQL_C_USHORT, &node, 0, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{

			wsprintfW(DirStr, L"%s\\%u.%u.%u.0\\*.*", cfg.FileboxesDir, zone, net, node);
			wsprintfW(TmpStr1, L"%u:%u/%u", zone, net, node);
			//
			OutboundSize = 0;
			
			hFind = FindFirstFileW(DirStr, &fd);
			if (hFind == INVALID_HANDLE_VALUE) goto nextlink;

			ft_prev.HighPart = fd.ftCreationTime.dwHighDateTime;
			ft_prev.LowPart = fd.ftCreationTime.dwLowDateTime;
			FileTimeToLocalFileTime(&(fd.ftCreationTime), &ft);
		nextfile:
			{
				OutboundSize += fd.nFileSizeLow;
				ft_curr.HighPart = fd.ftCreationTime.dwHighDateTime;
				ft_curr.LowPart = fd.ftCreationTime.dwLowDateTime;
				if (ft_curr.QuadPart < ft_prev.QuadPart)
				{
					ft_prev.QuadPart = ft_curr.QuadPart;
					FileTimeToLocalFileTime(&(fd.ftCreationTime), &ft);
				}

			}
			if (FindNextFileW(hFind, &fd)) goto nextfile;

			FindClose(hFind);
			if (OutboundSize != 0)
			{
				FileTimeToSystemTime(&ft, &st);
				wsprintfW(TmpStr2, L"%02u-%02u-%04u %02u:%02u", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
				wsprintfW(TmpStr, L"|%-18s | %17u | %30s |\r", TmpStr1, OutboundSize, TmpStr2);
				AddWStrToBuffer(&Buf, TmpStr);
			}
			nextlink:
			sqlret = SQLFetch(hstmt);
		}
		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	SQLFreeStmt(hstmt, SQL_CLOSE);
	AddWStrToBuffer(&Buf, L"==========================================================================\r");

	//
	PostEchomailMessage(hstmt, cfg.RobotsAreaID, cfg.SoftwareName, L"All", L"Outbound statistics", &Buf);
	WStringBufferFreeMem(&Buf);
	
}

void PostWeeklyEchoStat(void)
{
	wchar_t AreaName[256], TmpStr[80];
	unsigned int MessagesCount;
	
	InitWStringBuffer(hHeap, &Buf);
	AddWStrToBuffer(&Buf, L"\r       Area Name                                        Messages         \r==========================================================================\r");
	sqlret = SQLExecDirectW(hstmt, L"Select EchoAreas.Areaname,WeeklyEchoStat.Messages from WeeklyEchoStat,EchoAreas where WeeklyEchoStat.AreaID=EchoAreas.AreaID order by WeeklyEchoStat.Messages desc,EchoAreas.Areaname asc", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLBindCol(hstmt, 1, SQL_C_WCHAR, AreaName, 256, NULL);
		SQLBindCol(hstmt, 2, SQL_C_ULONG, &MessagesCount, 0, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			wsprintfW(TmpStr, L"|%-50s | %18u |\r", AreaName, MessagesCount);
			AddWStrToBuffer(&Buf, TmpStr);
			sqlret = SQLFetch(hstmt);
		}
		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	SQLFreeStmt(hstmt, SQL_CLOSE);

	AddWStrToBuffer(&Buf, L"==========================================================================\r\r\r Total number of messages on this week :  ");
	
	SQLExecDirectW(hstmt, L"Select isnull(sum(Messages),0) from WeeklyEchoStat", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_ULONG, &MessagesCount, 0, NULL);
	SQLCloseCursor(hstmt);
	wsprintfW(TmpStr, L"%u\r", MessagesCount);
	AddWStrToBuffer(&Buf, TmpStr);
	PostEchomailMessage(hstmt, cfg.RobotsAreaID, cfg.SoftwareName, L"All", L"Weekly echomail statistics", &Buf);
	WStringBufferFreeMem(&Buf);
	SQLExecDirectW(hstmt, L"Delete from WeeklyEchoStat", SQL_NTS);
}


__declspec(dllexport) BOOL SoaronModuleFunc(void)
{
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	hHeap = HeapCreate(HEAP_NO_SERIALIZE, 65536, 0);

	PostRules();
	PostOutboundStat();
	PostWeeklyEchoStat();
	SetEvent(cfg.hEchomailTossEvent);


	HeapDestroy(hHeap);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	return TRUE;
}