/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Mailer.h"


unsigned int PackData(HANDLE hHeap, char * UnpackedBuff, unsigned int UnpackedSize, char **PackedBuff,unsigned int offset)
{
	unsigned int PackedSize;
	PackedSize = (unsigned int)(UnpackedSize*1.001) + 12;
	*PackedBuff = HeapAlloc(hHeap, 0, PackedSize+offset);
	compress2((*PackedBuff)+offset, &PackedSize, UnpackedBuff, UnpackedSize, 9);
	*PackedBuff = HeapReAlloc(hHeap, 0, *PackedBuff, PackedSize + offset);
	return PackedSize;
}

BOOL UnpackData(HANDLE hHeap, char * PackedBuff, unsigned int PackedSize, char **UnpackedBuff, unsigned int UnpackedSize)
{
	int res, UnpackedRealSize;
	UnpackedRealSize = UnpackedSize;
	*UnpackedBuff = HeapAlloc(hHeap, 0, UnpackedSize);
	res = uncompress(*UnpackedBuff, &UnpackedRealSize, PackedBuff, PackedSize);
	if (res != Z_OK)
	{
		HeapFree(hHeap, 0, *UnpackedBuff);
		return FALSE;
	}
	else return TRUE;

}



BOOL MailerLogRemoteInfo(lpMailerSessionInfo SI)
{
	wchar_t LogStr[255];
	FTNAddr addr;
	unsigned char * UnpackedBuf;
	wchar_t * lpStr;
	unsigned short i;
	
	unsigned short NumOfAkas;
	unsigned int UnpackedSize;
	unsigned int Size;
	size_t Len;

	Size = GetMailerMsgSize(SI->CurrentRecvBuf);
	NumOfAkas = ((lpLoginHeader)(SI->CurrentRecvBuf))->NumOfAkas;
	UnpackedSize = ((lpLoginHeader)(SI->CurrentRecvBuf))->PackedContentOriginalSize;
	if (NumOfAkas == 0) return FALSE;
	if (UnpackedSize == 0) return FALSE;
	if (Size <= sizeof(LoginHeader)+NumOfAkas*sizeof(FTNAddr)) return FALSE;

	wsprintfW(LogStr, L"Remote using FTNMTP version %u.%u", ((lpLoginHeader)(SI->CurrentRecvBuf))->ProtocolVersionHi, ((lpLoginHeader)(SI->CurrentRecvBuf))->ProtocolVersionLo);
	AddLogEntry(LogStr);


	for (i = 0; i < NumOfAkas; i++)
	{
		memcpy(&addr, (SI->CurrentRecvBuf)+sizeof(LoginHeader) + (i*sizeof(FTNAddr)), sizeof(FTNAddr));
		wsprintfW(LogStr, L"Address: %u:%u/%u.%u", addr.zone, addr.net, addr.node, addr.point);
		AddLogEntry(LogStr);
	}
	
	if (UnpackData(SI->hHeap, (SI->CurrentRecvBuf) + sizeof(LoginHeader) + (NumOfAkas*sizeof(FTNAddr)), Size - (sizeof(LoginHeader) + NumOfAkas*sizeof(FTNAddr)), &UnpackedBuf, UnpackedSize))
	{

		lpStr = (wchar_t *)UnpackedBuf;
		wsprintfW(LogStr, L"SOFTWARE: %s", lpStr);
		AddLogEntry(LogStr);
		Len = wcslen(lpStr);
		lpStr += (Len + 1);

		wsprintfW(LogStr, L"SYSTEM: %s", lpStr);
		AddLogEntry(LogStr);
		Len = wcslen(lpStr);
		lpStr += (Len + 1);
		wsprintfW(LogStr, L"SYSOP: %s", lpStr);

		AddLogEntry(LogStr);
		Len = wcslen(lpStr);
		lpStr += (Len + 1);

		wsprintfW(LogStr, L"LOCATION: %s", lpStr);
		AddLogEntry(LogStr);
		Len = wcslen(lpStr);
		lpStr += (Len + 1);

		wsprintfW(LogStr, L"OTHER INFO: %s", lpStr);
		AddLogEntry(LogStr);

		HeapFree(SI->hHeap, 0, UnpackedBuf);
		return TRUE;
	}
	else return FALSE;


	
}


void GetLoginHash(HSTMT hstmt,unsigned int LinkID,unsigned char * InBuf, unsigned char * OutBuf)
{
	unsigned char TmpBuf[160];
	SQLLEN cb;
	Skein_256_Ctxt_t cc1;

	memcpy(TmpBuf, InBuf, 32);
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &LinkID, 0, NULL);
	SQLExecDirectW(hstmt, L"Select SessionPassword from Links where LinkID=?", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_WCHAR, (TmpBuf + 32), 128, &cb);
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	Skein_256_Init(&cc1, 256);
	Skein_256_Update(&cc1, TmpBuf, cb + 32);
	Skein_256_Final(&cc1, OutBuf);

}

unsigned int GetLinkID(HSTMT hstmt, unsigned short NumOfAkas, unsigned char * Buff)
{
	unsigned int LinkID=0;
	SQLRETURN sqlret;
	unsigned short i;
	FTNAddr addr;
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(addr.zone), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(addr.net), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(addr.node), 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(addr.point), 0, NULL);
	SQLPrepareW(hstmt, L"Select LinkID from Links where Zone=? and Net=? and Node=? and Point=?", SQL_NTS);
	for (i = 0; i < NumOfAkas; i++)
	{
		memcpy(&addr, Buff + sizeof(FTNAddr)*i, sizeof(FTNAddr));
		sqlret = SQLExecute(hstmt);
		if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			sqlret = SQLFetch(hstmt);
			if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
			{
				SQLGetData(hstmt, 1, SQL_C_ULONG, &LinkID, 0, NULL);
				goto close;
				
			}
		}

	}
	close:
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	return LinkID;
}

void GetLinkInfo(HSTMT hstmt, lpMailerSessionInfo SI)
{
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLExecDirectW(hstmt, L"Select Zone,Net,Node,Point,MaxPktSize from Links where LinkID=?", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_USHORT,&(SI->LinkAddr.zone) , 0, NULL);
	SQLGetData(hstmt, 2, SQL_C_USHORT, &(SI->LinkAddr.net), 0, NULL);
	SQLGetData(hstmt, 3, SQL_C_USHORT, &(SI->LinkAddr.node), 0, NULL);
	SQLGetData(hstmt, 4, SQL_C_USHORT, &(SI->LinkAddr.point), 0, NULL);
	SQLGetData(hstmt, 5, SQL_C_ULONG, &(SI->FileFrameSize), 0, NULL);
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_UNBIND);
	if ((SI->FileFrameSize < 1024) || (SI->FileFrameSize>65536)) SI->FileFrameSize = DefaultFileFrameSize;
}

void LogNetworkError(void)
{
	int errcode;
	wchar_t ErrorMsg[230], LogMsg[255];
	errcode=WSAGetLastError(); 
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		ErrorMsg, 230, NULL);
	
	wsprintfW(LogMsg, L"Error %u : %s", errcode, ErrorMsg);
	AddLogEntry(LogMsg);
}

void LogFileError(wchar_t * FileName)
{
	int errcode;
	wchar_t ErrorMsg[230], LogMsg[255];
	errcode = WSAGetLastError();
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		ErrorMsg, 230, NULL);

	wsprintfW(LogMsg, L"Unable to open %s - Error %u : %s", FileName,errcode, ErrorMsg);
	AddLogEntry(LogMsg);
}



