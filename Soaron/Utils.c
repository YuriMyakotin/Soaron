/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/


#include "Soaron.h"

unsigned int LogsDisableConsoleOut;

wchar_t * TrimWStr(wchar_t * Str)
{
	wchar_t * retval;
	size_t i;
	i=0;
	retval=Str;
	while(retval[0]==L' ')
		retval+=2;
		
	i=wcslen(retval)-1;
	if (i>=0)
		while(retval[i]==L' ')
		{
			retval[i]=0;
			--i;
		}
	return retval;
}


char * TrimStr(char * Str)
{
	char * retval;
	size_t i;
	i=0;
	retval=Str;
	while(retval[0]==' ')
		retval+=1;
		
	i=strlen(retval)-1;
	if (i>=0)
		while(retval[i]==' ')
		{
			retval[i]=0;
			--i;
		}
	return retval;
}

void StringBufferFreeMem(lpStringBuffer lpStrBuf)
{
	HeapFree(lpStrBuf->hHeap,0,lpStrBuf->lpBuffer);
	lpStrBuf->CurrentSize = 0;
	lpStrBuf->CurrentAllocated = 0;
	lpStrBuf->lpBuffer = NULL;
}

void WStringBufferFreeMem(lpWStringBuffer lpStrBuf)
{
	HeapFree(lpStrBuf->hHeap,0,lpStrBuf->lpBuffer);
	lpStrBuf->CurrentSize = 0;
	lpStrBuf->CurrentAllocated = 0;
	lpStrBuf->lpBuffer = NULL;
}

void DwordBufferFreeMem(lpDwordBuffer lpBuf)
{
	HeapFree(lpBuf->hHeap,0,lpBuf->lpBuffer);
}

void InitStringBuffer(HANDLE hHeap,lpStringBuffer lpStrBuffer)
{
	lpStrBuffer->hHeap=hHeap;
	lpStrBuffer->CurrentSize=0;
	lpStrBuffer->CurrentAllocated=4096;
	lpStrBuffer->lpBuffer=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,4096);
}

void InitWStringBuffer(HANDLE hHeap,lpWStringBuffer lpWStrBuffer)
{
	lpWStrBuffer->hHeap=hHeap;
	lpWStrBuffer->CurrentSize=0;
	lpWStrBuffer->CurrentAllocated=4096;
	lpWStrBuffer->lpBuffer=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,4096);
}

void AllocWStringBuffer(HANDLE hHeap, lpWStringBuffer lpWStrBuffer, size_t size)
{
	lpWStrBuffer->hHeap = hHeap;
	lpWStrBuffer->CurrentSize = size/2;
	lpWStrBuffer->CurrentAllocated = size+256;
	lpWStrBuffer->lpBuffer = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lpWStrBuffer->CurrentAllocated);
}

void InitDwordBuffer(HANDLE hHeap,lpDwordBuffer lpDWBuffer)
{
	lpDWBuffer->hHeap=hHeap;
	lpDWBuffer->CurrentSize=0;
	lpDWBuffer->CurrentAllocated=4096;
	lpDWBuffer->lpBuffer=(unsigned int *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,4096);
}

void AllocDwordBuffer(HANDLE hHeap,lpDwordBuffer lpDWBuffer,size_t Lenght)
{
	lpDWBuffer->hHeap=hHeap;
	lpDWBuffer->CurrentSize=Lenght/4;
	lpDWBuffer->CurrentAllocated=((unsigned int)(Lenght/4096)+1)*4096;
	lpDWBuffer->lpBuffer=(unsigned int *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,lpDWBuffer->CurrentAllocated);
}

void AddStrToBuffer(lpStringBuffer lpStrBuf, const char * lpStrToAdd)
{
	size_t RealSize;

	RealSize=lpStrBuf->CurrentSize+1;
	if ((RealSize+strlen(lpStrToAdd))>=lpStrBuf->CurrentAllocated)
	{	
		lpStrBuf->CurrentAllocated=4096*(((RealSize+strlen(lpStrToAdd))/4096)+1);
		lpStrBuf->lpBuffer=(char *)HeapReAlloc(lpStrBuf->hHeap,HEAP_ZERO_MEMORY,lpStrBuf->lpBuffer,lpStrBuf->CurrentAllocated);
	}
	memcpy(lpStrBuf->lpBuffer+lpStrBuf->CurrentSize,lpStrToAdd,strlen(lpStrToAdd));
	lpStrBuf->CurrentSize+=strlen(lpStrToAdd);
	//strcat(lpStrBuf->lpBuffer,lpStrToAdd);
	return;
	
}

void AddStr1ToBuffer(lpStringBuffer lpStrBuf, const char * lpStrToAdd,int size)
{
	size_t RealSize;

	RealSize=lpStrBuf->CurrentSize+1;
	if ((RealSize+size)>=lpStrBuf->CurrentAllocated)
	{	
		lpStrBuf->CurrentAllocated=4096*(((RealSize+size)/4096)+1);
		lpStrBuf->lpBuffer=(char *)HeapReAlloc(lpStrBuf->hHeap,HEAP_ZERO_MEMORY,lpStrBuf->lpBuffer,lpStrBuf->CurrentAllocated);
	}
	memcpy(lpStrBuf->lpBuffer+lpStrBuf->CurrentSize,lpStrToAdd,size);
	lpStrBuf->CurrentSize+=size;
	//strcat(lpStrBuf->lpBuffer,lpStrToAdd);
	return;
	
}

void AddToDwordBuffer(lpDwordBuffer lpDWBuf, unsigned int DW)
{
	if ((lpDWBuf->CurrentSize*4+4)>=lpDWBuf->CurrentAllocated)
	{
		lpDWBuf->CurrentAllocated+=4096;
		lpDWBuf->lpBuffer=(unsigned int *)HeapReAlloc(lpDWBuf->hHeap,HEAP_ZERO_MEMORY,lpDWBuf->lpBuffer,lpDWBuf->CurrentAllocated);
	}
	(lpDWBuf->lpBuffer)[lpDWBuf->CurrentSize]=DW;
	lpDWBuf->CurrentSize+=1;
	return;
}

BOOL CheckInDwordBuffer(lpDwordBuffer lpDWBuf, unsigned int DW)
{
	unsigned int i;
	for(i=0;i<lpDWBuf->CurrentSize;i++)
		if ((lpDWBuf->lpBuffer)[i]==DW) return TRUE;
	
	return FALSE;    
}

void SortDwordBuffer(lpDwordBuffer lpDWBuf)
{
	unsigned int i,j;
	unsigned int tmp;
	
	for(j=1;j<lpDWBuf->CurrentSize;j++)
	{
		for(i=0;i<(lpDWBuf->CurrentSize-j);i++)
		{
			if ((lpDWBuf->lpBuffer)[i]>(lpDWBuf->lpBuffer)[i+1])
			{
				tmp=(lpDWBuf->lpBuffer)[i];
				(lpDWBuf->lpBuffer)[i]=(lpDWBuf->lpBuffer)[i+1];
				(lpDWBuf->lpBuffer)[i+1]=tmp;
			}
			
		}
	}


}

void AddWStrToBuffer(lpWStringBuffer lpWStrBuf, const wchar_t * lpWStrToAdd)
{
	size_t RealSize;
	wchar_t *lpEndStr;
		
	RealSize=lpWStrBuf->CurrentSize*2+2;
	if ((RealSize+wcslen(lpWStrToAdd)*2)>=lpWStrBuf->CurrentAllocated)
	{	
		lpWStrBuf->CurrentAllocated=4096*(((RealSize+wcslen(lpWStrToAdd)*2)/4096)+1);
		lpWStrBuf->lpBuffer=(wchar_t *)HeapReAlloc(lpWStrBuf->hHeap,HEAP_ZERO_MEMORY,lpWStrBuf->lpBuffer,lpWStrBuf->CurrentAllocated);
	}
	lpEndStr=lpWStrBuf->lpBuffer+lpWStrBuf->CurrentSize;
	memcpy(lpEndStr,lpWStrToAdd,wcslen(lpWStrToAdd)*2);
	lpWStrBuf->CurrentSize+=wcslen(lpWStrToAdd);
	return;
}

unsigned int GetMsgIdTime(HSTMT hStmt)
{
	unsigned int retval;
	SQLBindParameter(hStmt,1,SQL_PARAM_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&retval,0,0);
	SQLExecDirectW(hStmt,L"{?=call sp_GetMsgIdTime}",SQL_NTS);
	SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
	return retval;
}


unsigned int GetPktNumber(HSTMT hStmt)
{
	unsigned int retval;
	SQLBindParameter(hStmt,1,SQL_PARAM_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&retval,0,0);
	SQLExecDirectW(hStmt,L"{?=call sp_GetPktNumber}",SQL_NTS);
	SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
	return retval;
}

unsigned int GetInt(HSTMT hStmt, wchar_t * ParamName)
{
	unsigned int retval = 0;
	SQLRETURN sqlret;
	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 512, 0, ParamName, 0, 0);
	sqlret=SQLExecDirectW(hStmt, L"Select Value from IntTable where ValueName=?", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		
		sqlret = SQLFetch(hStmt);
		if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			
			SQLGetData(hStmt, 1, SQL_C_ULONG, &retval, 0, NULL);
			
		}
		SQLCloseCursor(hStmt);
	}
	SQLFreeStmt(hStmt, SQL_RESET_PARAMS);
	return retval;
}


SQL_TIMESTAMP_STRUCT GetSQLTime(void)
{
	SQL_TIMESTAMP_STRUCT retval;
	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);
	retval.year=SysTime.wYear;
	retval.month=SysTime.wMonth;
	retval.day=SysTime.wDay;
	retval.hour=SysTime.wHour;
	retval.minute=SysTime.wMinute;
	retval.second=SysTime.wSecond;
	

	return retval;
}

void StrToSqlDateTime(SQL_TIMESTAMP_STRUCT *result,char *str)
{
	char *tmp1;
	result->day=(unsigned short)strtoul(str,&tmp1,10);
	result->year=(unsigned short)strtoul(str+7,&tmp1,10);
	if (result->year<90) result->year+=2000;
	else result->year+=1900;
	result->hour=(unsigned short)strtoul(str+11,&tmp1,10);
	result->minute=(unsigned short)strtoul(str+14,&tmp1,10);
	result->second=(unsigned short)strtoul(str+17,&tmp1,10);
	if (_memicmp(str+3,"Jan",3)==0)
	{
		result->month=1;
		goto complete;
	}
	if (_memicmp(str+3,"Feb",3)==0)
	{
		result->month=2;
		goto complete;
	}
	if (_memicmp(str+3,"Mar",3)==0)
	{
		result->month=3;
		goto complete;
	}
	if (_memicmp(str+3,"Apr",3)==0)
	{
		result->month=4;
		goto complete;
	}
	if (_memicmp(str+3,"May",3)==0)
	{
		result->month=5;
		goto complete;
	}
	if (_memicmp(str+3,"Jun",3)==0)
	{
		result->month=6;
		goto complete;
	}
	if (_memicmp(str+3,"Jul",3)==0)
	{
		result->month=7;
		goto complete;
	}
	if (_memicmp(str+3,"Aug",3)==0)
	{
		result->month=8;
		goto complete;
	}
	if (_memicmp(str+3,"Sep",3)==0)
	{
		result->month=9;
		goto complete;
	}
	if (_memicmp(str+3,"Oct",3)==0)
	{
		result->month=10;
		goto complete;
	}
	if (_memicmp(str+3,"Nov",3)==0)
	{
		result->month=11;
		goto complete;
	}
	if (_memicmp(str+3,"Dec",3)==0)
	{
		result->month=12;
		goto complete;
	}

	complete:
	result->fraction=0;
}

wchar_t * GetBigString(HANDLE hHeap, HSTMT hstmt,wchar_t *StrName)
{
	wchar_t * buf=NULL;
	wchar_t tmpbuff;
	SQLLEN size;
	SQLRETURN sqlret;

	SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,512,0,StrName,0,NULL);
	SQLExecDirectW(hstmt,L"Select StrValue from BigStringTable where StrName=?",SQL_NTS);
	sqlret=SQLFetch(hstmt);
	if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
	{
		SQLGetData(hstmt,1,SQL_C_WCHAR,&tmpbuff,0,&size);
		buf=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,size+2);
		SQLGetData(hstmt,1,SQL_C_WCHAR,buf,size+2,NULL);
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

	return buf;
}

wchar_t * GetString(HANDLE hHeap, HSTMT hstmt, wchar_t * StrName)
{
	wchar_t * buf=NULL;
	wchar_t tmpbuff;
	SQLLEN size;
	SQLRETURN sqlret;

	SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,512,0,StrName,0,NULL);
	sqlret=SQLExecDirectW(hstmt, L"Select StrValue from StringTable where StrName=?", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		sqlret = SQLFetch(hstmt);
		if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{

			SQLGetData(hstmt, 1, SQL_C_WCHAR, &tmpbuff, 0, &size);
			buf = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size + 2);
			SQLGetData(hstmt, 1, SQL_C_WCHAR, buf, size + 2, NULL);

		}
		SQLCloseCursor(hstmt);
	}
	SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

	return buf;
}

void TimeToMessageStr(char * Str, SQL_TIMESTAMP_STRUCT * st)
{
	unsigned int year = st->year % 100;
	switch (st->month)
	{
	case 1: wsprintfA(Str, "%02u Jan %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 2: wsprintfA(Str, "%02u Feb %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 3: wsprintfA(Str, "%02u Mar %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 4: wsprintfA(Str, "%02u Apr %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 5: wsprintfA(Str, "%02u May %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 6: wsprintfA(Str, "%02u Jun %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 7: wsprintfA(Str, "%02u Jul %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 8: wsprintfA(Str, "%02u Aug %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 9: wsprintfA(Str, "%02u Sep %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 10: wsprintfA(Str, "%02u Oct %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 11: wsprintfA(Str, "%02u Nov %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	case 12: wsprintfA(Str, "%02u Dec %02u  %02u:%02u:%02u", st->day, year, st->hour, st->minute, st->second); break;
	}
}



void WritePktHeader(HANDLE hFile, lpFTNAddr lpFromAka, lpFTNAddr lpToAddr, char * Pwd)
{
	PktHeader PktHdr;
	SYSTEMTIME SysTime;
	DWORD cbFile;
	memset(&PktHdr, 0, sizeof(PktHeader));
	GetLocalTime(&SysTime);
	PktHdr.origZone = lpFromAka->zone;
	PktHdr.origZoneQ = lpFromAka->zone;
	PktHdr.origNode = lpFromAka->node;
	PktHdr.destZone = lpToAddr->zone;
	PktHdr.destZoneQ = lpToAddr->zone;
	PktHdr.destNet = lpToAddr->net;
	PktHdr.destNode = lpToAddr->node;
	PktHdr.destPoint = lpToAddr->point;
	if (lpFromAka->point == 0)
	{
		
		PktHdr.origNet = lpFromAka->net;
		
	}
	else
	{
		PktHdr.origNet = 65535;
		PktHdr.auxNet = lpFromAka->net;
		PktHdr.origPoint = lpFromAka->point;
	}
	PktHdr.year = SysTime.wYear;
	PktHdr.month = SysTime.wMonth;
	PktHdr.day = SysTime.wDay;
	PktHdr.hour = SysTime.wHour;
	PktHdr.minute = SysTime.wMinute;
	PktHdr.second = SysTime.wSecond;

	PktHdr.PktVer = 2;
	PktHdr.cw1H = 0;
	PktHdr.cw2H = 0;
	PktHdr.cw1L = 1;
	PktHdr.cw2L = 1;

	memcpy(&(PktHdr.password), Pwd,8);
	


	WriteFile(hFile, &PktHdr, sizeof(PktHeader), &cbFile, NULL);
}

void ClosePktFile(HANDLE hFile)
{
	DWORD cbFile;
	const unsigned short EndSymbs = 0;
	WriteFile(hFile, &EndSymbs, 2, &cbFile, NULL);
	CloseHandle(hFile);
}

wchar_t * GetFileName(wchar_t * FullName)
{
	wchar_t * result;
	result = wcsrchr(FullName, L'\\');
	if (result == NULL) return FullName;
	else return result+1;
}

void AddLogEntry(wchar_t * Message)
{
	DWORD ThreadID = GetCurrentThreadId();
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SYSTEMTIME st;

	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ThreadID, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, Message, 0, NULL);
	SQLExecDirectW(hstmt, L"INSERT INTO Logs values(?,GetDate(),?)", SQL_NTS);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	if (!LogsDisableConsoleOut)
	{
		GetLocalTime(&st);
		wprintf(L"%02u:%02u:%02u [%05u] %s\n", st.wHour, st.wMinute, st.wSecond, ThreadID, Message);
	}


}

void InitMixedBuffer(HANDLE hHeap, lpMixedBuffer lpBuff)
{
	lpBuff->hHeap = hHeap;
	lpBuff->CurrentSize = 0;
	lpBuff->CurrentAllocated = 1024;
	lpBuff->lpBuffer = (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);

}

void AddToMixedBuffer(lpMixedBuffer lpBuff, void * Data, unsigned int DataSize)
{

	if (lpBuff->CurrentSize + DataSize >= lpBuff->CurrentAllocated)
	{
		lpBuff->CurrentAllocated = 1024 * (((lpBuff->CurrentSize + DataSize) / 1024) + 1);
		lpBuff->lpBuffer = HeapReAlloc(lpBuff->hHeap, HEAP_ZERO_MEMORY, lpBuff->lpBuffer, lpBuff->CurrentAllocated);
	}

	memcpy(lpBuff->lpBuffer + lpBuff->CurrentSize, Data, DataSize);
	lpBuff->CurrentSize += DataSize;
	return;

}




void MixedBufferFreeMem(lpMixedBuffer lpBuff)
{
	HeapFree(lpBuff->hHeap, 0, lpBuff->lpBuffer);
	lpBuff->CurrentSize = 0;
	lpBuff->CurrentAllocated = 0;
	lpBuff->lpBuffer = NULL;
}

