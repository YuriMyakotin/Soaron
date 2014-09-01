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


	HANDLE hFile, hFileMap, hSearch;
	char * buf;
	char * currentbufpos;
	char str[255], SearchFileName[MAX_PATH],filename[MAX_PATH];
	 

	size_t i, j, len;
	unsigned short zone, net, node, hubnode;
	char sysopname[255];
	unsigned char hub, hold, down, pvt, host, region, zc;

	WIN32_FIND_DATAA FindData;
	FILETIME ft;


	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	wsprintfA(SearchFileName, "%S\\nodelist.???", cfg.NodelistDir);
	
	ft.dwHighDateTime = 0; ft.dwLowDateTime = 0;

	hSearch = FindFirstFileA(SearchFileName, &FindData);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		AddLogEntry(L"Error: nodelist file not found");
		goto sqlclose;
	}
SearchLoop:
	if ((FindData.ftCreationTime.dwHighDateTime > ft.dwHighDateTime) || ((FindData.ftCreationTime.dwHighDateTime == ft.dwHighDateTime) && (FindData.ftCreationTime.dwLowDateTime > ft.dwLowDateTime)))
	{
		ft.dwHighDateTime = FindData.ftCreationTime.dwHighDateTime;
		ft.dwLowDateTime = FindData.ftCreationTime.dwLowDateTime;
		wsprintfA(filename, "%S\\%s", cfg.NodelistDir, FindData.cFileName);
	}

	if (FindNextFileA(hSearch, &FindData)) goto SearchLoop;



	hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	FindClose(hSearch);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AddLogEntry(L"Nodelist file open error");
		goto sqlclose;
	}
	hFileMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, GetFileSize(hFile, NULL), NULL);
	buf = (char *)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, GetFileSize(hFile, NULL));
	currentbufpos = buf;
	SQLExecDirectA(hstmt, (SQLCHAR *)"delete from nodelist", SQL_NTS);
	SQLPrepareA(hstmt, (SQLCHAR *)"insert into nodelist values(?,?,?,?,?,?,?,?,?,?,?,?,0)", SQL_NTS);
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &zone, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &net, 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &node, 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, sysopname, 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &hub, 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &hold, 0, NULL);
	SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &down, 0, NULL);
	SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &pvt, 0, NULL);
	SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &host, 0, NULL);
	SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &region, 0, NULL);
	SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &zc, 0, NULL);
	SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &hubnode, 0, NULL);

	zone = 0;
	net = 0;
	node = 0;
	hubnode = 0;
	len = 0;


loop:
	if (currentbufpos[0] == '\x1A') goto exit;
	if ((currentbufpos[0] == '\x0D') && (currentbufpos[1] == '\x0A')) goto newline;
	str[len] = currentbufpos[0];
	currentbufpos++;
	len++;

	goto loop;


newline:
	str[len] = 0;
	currentbufpos += 2;
	if (str[0] == ';') goto nextstr;
	hub = 0;
	hold = 0;
	down = 0;
	pvt = 0;
	host = 0;
	region = 0;
	zc = 0;
	for (i = 0; i<255; i++) sysopname[i] = 0;
	i = 1;
	if (str[0] == ',') goto simplenode;
	i = strchr(str, ',') + 1 - str;
	if (_memicmp(str, "hub", i - 1) == 0)
	{
		hub = 255;
		node = atoi(str + i);
		hubnode = node;

		goto addnode;

	}
	if (_memicmp(str, "hold", i - 1) == 0)
	{
		hold = 255;
		goto simplenode;

	}
	if (_memicmp(str, "down", i - 1) == 0)
	{
		down = 255;
		goto simplenode;

	}
	if (_memicmp(str, "pvt", i - 1) == 0)
	{
		pvt = 255;
		goto simplenode;

	}
	if (_memicmp(str, "host", i - 1) == 0)
	{
		host = 255;
		node = 0;
		hubnode = 0;
		net = atoi(str + i);
		goto addnode;

	}
	if (_memicmp(str, "region", i - 1) == 0)
	{
		region = 255;
		node = 0;
		hubnode = 0;

		net = atoi(str + i);
		goto addnode;

	}
	if (_memicmp(str, "zone", i - 1) == 0)
	{
		zc = 255;
		node = 0;
		net = 0;
		hubnode = 0;
		zone = atoi(str + i);
		goto addnode;

	}




simplenode:
	node = atoi(str + i);

addnode:
	j = 0;
	i = strchr(1 + strchr(1 + strchr(1 + strchr(str, ','), ','), ','), ',') + 1 - str;
	while (str[i] != ',')
	{
		sysopname[j] = str[i];
		i++;
		j++;
	}
	sysopname[j] = 0;


	SQLExecute(hstmt);

nextstr:
	len = 0;
	goto loop;

exit:
	UnmapViewOfFile((void *)buf);
	CloseHandle(hFileMap);
	CloseHandle(hFile);

	{
		wchar_t LogStr[255];
		wsprintfW(LogStr, L"Import %S complete", filename);
		AddLogEntry(LogStr);
	}

	


sqlclose:



	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);

	return TRUE;
}