/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/


#include "Soaron.h"



void WriteConfigs(void)
{
	HANDLE hTmpHeap;

	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;

	unsigned short zone, net, node, point;

	char SessionPassword[60];

	char Ip[128];
	unsigned char Dialout;
	SQLLEN cb1;
	DWORD cb2;

	char TmpStr[512];

	StringBuffer BinkdCfgBuf, TmailPwdBuf;
	HANDLE hBinkdFile, hTmailFile;

	if ((cfg.TmailPwdCfg == NULL) && (cfg.BinkdPwdCfg == NULL)) return;


	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	hTmpHeap = HeapCreate(HEAP_NO_SERIALIZE, 65536, 0);



	sqlret = SQLExecDirectW(hstmt, L"select zone, net, node, point,sessionpassword,ip,dialout from links order by zone,net,node,point", SQL_NTS);
	SQLBindCol(hstmt, 1, SQL_C_USHORT, &zone, 0, NULL);
	SQLBindCol(hstmt, 2, SQL_C_USHORT, &net, 0, NULL);
	SQLBindCol(hstmt, 3, SQL_C_USHORT, &node, 0, NULL);
	SQLBindCol(hstmt, 4, SQL_C_USHORT, &point, 0, NULL);
	SQLBindCol(hstmt, 5, SQL_C_CHAR, SessionPassword, 60, NULL);
	SQLBindCol(hstmt, 6, SQL_C_CHAR, Ip, 128, &cb1);
	SQLBindCol(hstmt, 7, SQL_C_BIT, &Dialout, 0, NULL);


	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{

		InitStringBuffer(hTmpHeap, &BinkdCfgBuf);

		if (cfg.TmailPwdCfg != NULL) InitStringBuffer(hTmpHeap, &TmailPwdBuf);
			

		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{

			if ((cb1 == SQL_NULL_DATA) || (cb1 == 0) || (Dialout == 0))
			{
				wsprintfA(TmpStr, "node %u:%u/%u.%u  - %s \n", zone, net, node, point, SessionPassword);
				AddStrToBuffer(&BinkdCfgBuf, TmpStr);
			}
			else
			{
				wsprintfA(TmpStr, "node %u:%u/%u.%u  %s  %s \n", zone, net, node, point, Ip, SessionPassword);
				AddStrToBuffer(&BinkdCfgBuf, TmpStr);
			}
			if (cfg.TmailPwdCfg != NULL)
			{
				wsprintfA(TmpStr, "   %u:%u/%u.%u  %s\n", zone, net, node, point, SessionPassword);
				AddStrToBuffer(&TmailPwdBuf, TmpStr);
			}

			sqlret = SQLFetch(hstmt);
		}



		hBinkdFile = CreateFileW(cfg.BinkdPwdCfg, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		WriteFile(hBinkdFile, BinkdCfgBuf.lpBuffer, (DWORD)BinkdCfgBuf.CurrentSize, &cb2, NULL);
		CloseHandle(hBinkdFile);
		StringBufferFreeMem(&BinkdCfgBuf);

		if (cfg.TmailPwdCfg != NULL)
		{
			hTmailFile = CreateFileW(cfg.TmailPwdCfg, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(hTmailFile, TmailPwdBuf.lpBuffer, (DWORD)TmailPwdBuf.CurrentSize, &cb2, NULL);
			CloseHandle(hTmailFile);
			StringBufferFreeMem(&TmailPwdBuf);
		}
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_UNBIND);


	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	HeapDestroy(hTmpHeap);
	return;
}



void ParseFidoAddrStr(char * Str, signed short * zone, signed short * net, signed short * node)
{
	char * zonestr, *netstr, *nodestr;

	signed short tmp;
	char * TmpStr, *TmpStr1;
	TmpStr = Str;

	TmpStr1 = strchr(TmpStr, ':');
	if (TmpStr1 != NULL)
	{
		TmpStr1[0] = 0;
		zonestr = TmpStr;
		TmpStr = TmpStr1 + 1;
	}
	else zonestr = NULL;

	TmpStr1 = strchr(TmpStr, '/');
	if (TmpStr1 != NULL)
	{
		TmpStr1[0] = 0;
		netstr = TmpStr;
		TmpStr = TmpStr1 + 1;
	}
	else netstr = NULL;

	nodestr = TmpStr;



	if (zonestr != NULL)
	{
		if (zonestr[0] == '*') 	*zone = -1;
		else
		{
			tmp = atoi(zonestr);
			*zone = tmp;
		}
	}
	if (netstr != NULL)
	{
		if (netstr[0] == '*') 	*net = -1;
		else
		{
			tmp = atoi(netstr);
			*net = tmp;
		}
	}
	if (nodestr[0] == '*') 	*node = -1;
	else
	{
		tmp = atoi(nodestr);
		*node = tmp;
	}

}


void ImportRoutingFile(HSTMT hStmt, int FileType)
{
	//filetype: 0: rou, 1: tru
	HANDLE hFile, hFileMap, hSearch;
	WIN32_FIND_DATA fd;
	BOOL GoFile;

	char * lpFileView;

	wchar_t FileNameTemplate[MAX_PATH], FileName[MAX_PATH];


	char TmpStr[80];
	char FidoAddrStr[24];
	char * TrimmedStr, *TrimmedStr1;
	unsigned int i, j;
	size_t Len;

	signed short zone1, zone2, net1, net2, node1, node2;
	BOOL is2partroute, isNextPartExists;

	SQLRETURN SqlRet;

	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &zone1, 0, NULL);
	SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &net1, 0, NULL);
	SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &node1, 0, NULL);
	SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &zone2, 0, NULL);
	SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &net2, 0, NULL);
	SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_SMALLINT, 0, 0, &node2, 0, NULL);

	switch (FileType)
	{
	case 0:
	{
			  wsprintfW(FileNameTemplate, L"%s\\*.rou", cfg.NodelistDir);
			  SQLPrepareW(hStmt, L"INSERT INTO NetmailRouTable VALUES(?,?,?,?,?,?,0)", SQL_NTS);
			  break;
	}
	case 1:
	{
			  wsprintfW(FileNameTemplate, L"%s\\*.tru", cfg.NodelistDir);
			  SQLPrepareW(hStmt, L"INSERT INTO NetmailTruTable VALUES(?,?,?,?,?,?,0)", SQL_NTS);
			  break;
	}
	}

	hSearch = FindFirstFileW(FileNameTemplate, &fd);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		GoFile = TRUE;
		while (GoFile)
		{
			wsprintfW(FileName, L"%s\\%s", cfg.NodelistDir, fd.cFileName);
			hFile = CreateFileW(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				hFileMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, fd.nFileSizeHigh, fd.nFileSizeLow, NULL);
				lpFileView = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, fd.nFileSizeLow);
				Len = 0;
				for (i = 0; i < fd.nFileSizeLow; i++)
				{
					if ((*(lpFileView + i) == '\r') || (*(lpFileView + i) == '\n'))
					{//
						if (Len>2)
						{
							memcpy(TmpStr, lpFileView + i - Len, Len);
							TmpStr[Len] = 0;
							TrimmedStr = TrimStr(TmpStr);
							Len = strlen(TrimmedStr);
							for (j = 0; j < Len; j++)
							{
								if (TrimmedStr[j] == '\t') TrimmedStr[j] = ' ';
							}

							is2partroute = FALSE;
							//process str
							if (TrimmedStr[0] != ';')
							{
								if (TrimmedStr[0] == '>')
								{
									is2partroute = TRUE;
									++TrimmedStr;
								}
								zone1 = cfg.MyAddr.zone;
								//parse first addr

								TrimmedStr1 = strchr(TrimmedStr, ' ');
								if (TrimmedStr1 != NULL)
								{
									TrimmedStr1[0] = 0;
									strcpy_s(FidoAddrStr, 24, TrimmedStr);
									ParseFidoAddrStr(FidoAddrStr, &zone1, &net1, &node1);
									TrimmedStr = TrimStr(TrimmedStr1 + 1);
									zone2 = zone1;
									net2 = net1;
									isNextPartExists = TRUE;

									while (isNextPartExists)
									{
										TrimmedStr1 = strchr(TrimmedStr, ' ');
										if (TrimmedStr1 == NULL)
										{
											isNextPartExists = FALSE;
											strcpy_s(FidoAddrStr, 24, TrimmedStr);
										}
										else
										{
											TrimmedStr1[0] = 0;
											strcpy_s(FidoAddrStr, 24, TrimmedStr);
											TrimmedStr = TrimStr(TrimmedStr1 + 1);
										}
										ParseFidoAddrStr(FidoAddrStr, &zone2, &net2, &node2);

										SqlRet = SQLExecute(hStmt);
										if ((SqlRet != SQL_SUCCESS) && (SqlRet != SQL_SUCCESS_WITH_INFO))
										{
											SQLCancel(hStmt);

										}

										if ((FileType == 0) && is2partroute)
										{
											zone1 = zone2;
											net1 = net2;
											node1 = node2;
											is2partroute = FALSE;
										}
									}
								}
							}



						}//

						Len = 0;
					}
					else ++Len;

				}


				UnmapViewOfFile(lpFileView);
				CloseHandle(hFileMap);
			}
			CloseHandle(hFile);
			GoFile = FindNextFileW(hSearch, &fd);
		}

		FindClose(hSearch);
	}



	SQLFreeStmt(hStmt, SQL_RESET_PARAMS);

}


void MakeRoute()
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

	SQLExecDirectW(hstmt, L"TRUNCATE TABLE NetmailRouTable", SQL_NTS);
	SQLExecDirectW(hstmt, L"TRUNCATE TABLE NetmailTruTable", SQL_NTS);

	ImportRoutingFile(hstmt, 0);
	ImportRoutingFile(hstmt, 1);
	//

	EnterCriticalSection(&NetmailRouteCritSect);
	SQLExecDirectW(hstmt, L"execute sp_MakeNetmailRouting", SQL_NTS);
	//
	LeaveCriticalSection(&NetmailRouteCritSect);

	//
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SetEvent(cfg.hNetmailOutEvent);


}








DWORD WINAPI ConfigsThread(LPVOID param)
{
	int result; 
	
	HANDLE hEvent[2];
	InterlockedIncrement(&(cfg.ThreadCount));


	hEvent[0]=cfg.hExitEvent;

	hEvent[1]=cfg.hLinksUpdateEvent;
	AddLogEntry(L"Configs thread started");

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
			WriteConfigs();
			MakeRoute();
			AddLogEntry(L"Links configuration updated");
			goto loop;
		}

	}
	

threadexit:
	InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}


