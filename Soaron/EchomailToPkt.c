/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

typedef struct tagLinkInfo
{
	FTNAddr LinkAddr;
	unsigned int MaxPktSize;
	size_t CurrentSize;
	HANDLE hFile;
	char PktPwd[9];

} LinkInfo,*lpLinkInfo;

lpLinkInfo LinksInfo;
unsigned int NumOfLinks;

LPVOID lpPackedMsgBuffer;
SIZE_T PackedMsgBufferRealSize,PackedMsgBufferAllocSize;

typedef struct tagOutboundQueue
{
	unsigned int LinkId;
	unsigned int MessageId;
	struct tagOutboundQueue * lpNext;

} OutboundQueue,*lpOutboundQueue;

const wchar_t lastsymbol[]=L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const wchar_t ChangeSymbol[]=L"GHIJKLMNOPQRSTUVWXYZ";

__inline BOOL CheckFile(wchar_t * FileName, WIN32_FIND_DATA * lpFindData)
{
	HANDLE hSearch;

	hSearch=FindFirstFileW(FileName,lpFindData);
	if (hSearch==INVALID_HANDLE_VALUE) return FALSE;
	
	FindClose(hSearch);
	return TRUE;

}

__inline void MakeFileName(wchar_t * Filename,wchar_t * DirName,wchar_t * DayName, unsigned short zone,unsigned short net,unsigned short node,unsigned short point,int ArchiveNum)
{
	size_t strLen;
	int letterpos;
	int ChangeLetter;
	ChangeLetter=ArchiveNum/36;
	if(point==0)
	{
		swprintf_s(Filename,MAX_PATH,L"%s\\%u.%u.%u.%u\\%04hX%04hX.%s%c",DirName,zone,net,node,point,cfg.MyAddr.net-net,cfg.MyAddr.node-node,DayName,lastsymbol[ArchiveNum%36]);
	}
	else
	{
		swprintf_s(Filename,MAX_PATH,L"%s\\%u.%u.%u.%u\\%04hX%04hX.%s%c",DirName,zone,net,node,point,cfg.MyAddr.node,point,DayName,lastsymbol[ArchiveNum%36]);
	}
	if (ChangeLetter>0)
	{
		strLen=wcslen(Filename);
		letterpos=ChangeLetter/20;
		Filename[strLen+letterpos-12]=ChangeSymbol[ChangeLetter%20];

	}
}

__inline void MakeTmpFileName(wchar_t * Filename,wchar_t * DirName,wchar_t * DayName, unsigned short net,unsigned short node,unsigned short point,int ArchiveNum)
{
	size_t strLen;
	int letterpos;
	int ChangeLetter;
	ChangeLetter=ArchiveNum/36;
	if(point==0)
	{
		swprintf_s(Filename,MAX_PATH,L"%s\\%04hX%04hX.%s%c",DirName,cfg.MyAddr.net-net,cfg.MyAddr.node-node,DayName,lastsymbol[ArchiveNum%36]);
	}
	else
	{
		swprintf_s(Filename,MAX_PATH,L"%s\\%04hX%04hX.%s%c",DirName,cfg.MyAddr.node,point,DayName,lastsymbol[ArchiveNum%36]);
	}
	if (ChangeLetter>0)
	{
		strLen=wcslen(Filename);
		letterpos=ChangeLetter/20;
		Filename[strLen+letterpos-12]=ChangeSymbol[ChangeLetter%20];

	}
}


void PackEchoMessages(HSTMT hstmt)
{
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	SQLRETURN sqlret;
	
	int result; 

	BOOL isUseCurrent, isNumChanged,isBusy,isExists;  //isUseCurrent 0 надо в любом случае создавать новый файл, 1 - не надо
	
	unsigned short zone,net,node,point;
	unsigned int LinkID;
	HANDLE hSearch,hFile,hFile1,hFileMap;
	WIN32_FIND_DATA FindData,FileInfoData;
	wchar_t Filename[MAX_PATH];
	wchar_t Filename1[MAX_PATH];
	wchar_t TmpStr[MAX_PATH];
	wchar_t CmdLine[MAX_PATH];
	wchar_t LogStr[255];
	wchar_t DayName[3];
	lpPktHeader lpPkt;
	unsigned int MaxArchiveSize,ArchiveNum;
	SYSTEMTIME Time;

	StartupInfo.cb=sizeof(StartupInfo);
	StartupInfo.cbReserved2=0;
	StartupInfo.dwFillAttribute=0;
	StartupInfo.dwFlags=0;
	StartupInfo.dwX=0;
	StartupInfo.dwXCountChars=80;
	StartupInfo.dwXSize=0;
	StartupInfo.dwY=0;
	StartupInfo.dwYCountChars=25;
	StartupInfo.dwYSize=0;
	StartupInfo.hStdError=NULL;
	StartupInfo.hStdInput=NULL;
	StartupInfo.hStdOutput=NULL;
	StartupInfo.lpDesktop=NULL;
	StartupInfo.lpReserved=NULL;
	StartupInfo.lpReserved2=NULL;
	StartupInfo.lpTitle=NULL;
	StartupInfo.wShowWindow=0;

	wsprintfW(TmpStr, L"%s\\*.PKT", cfg.TmpOutboundDir);
			hSearch=FindFirstFileW(TmpStr,&FindData);
			
			if(hSearch!=INVALID_HANDLE_VALUE)
			{
				nextoutfile:
				wsprintfW(Filename, L"%s\\%s", cfg.TmpOutboundDir, &(FindData.cFileName));
			
				hFile=CreateFileW(Filename,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
				if (hFile==INVALID_HANDLE_VALUE) goto nextout;
				
				hFileMap=CreateFileMappingW(hFile,NULL,PAGE_READONLY,0,sizeof(PktHeader),NULL);
				lpPkt=(lpPktHeader)MapViewOfFile(hFileMap,FILE_MAP_READ,0,0,sizeof(PktHeader));
				zone=lpPkt->destZone;
				net=lpPkt->destNet;
				node=lpPkt->destNode;
				point=lpPkt->destPoint;
				UnmapViewOfFile(lpPkt);
				CloseHandle(hFileMap);
				CloseHandle(hFile);

				SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&zone,0,NULL);
				SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&net,0,NULL);
				SQLBindParameter(hstmt,3,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&node,0,NULL);
				SQLBindParameter(hstmt,4,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&point,0,NULL);

				SQLExecDirectW(hstmt,L"Select LinkID,MaxArchiveSize,NextArchiveExt from links where zone=? and net=? and node=? and point=?",SQL_NTS);
				sqlret=SQLFetch(hstmt);
				if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
				{
					SQLGetData(hstmt,1,SQL_C_ULONG,&LinkID,0,NULL);
					SQLGetData(hstmt,2,SQL_C_ULONG,&MaxArchiveSize,0,NULL);
					SQLGetData(hstmt,3,SQL_C_ULONG,&ArchiveNum,0,NULL);
					SQLCloseCursor(hstmt);
					SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
				}
				else
				{
					//линк не наш, в бэды
					SQLCloseCursor(hstmt);
					SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
					wsprintfW(LogStr,L"Address %u:%u/%u.%u unknown",zone,net,node,point);
					AddLogEntry(LogStr);
					wsprintfW(TmpStr, L"%s\\%s", cfg.BadOutPktDir, &(FindData.cFileName));
					MoveFileW(Filename,TmpStr);
					goto nextout;

				}

					//проверка на бизи
				if(point==0)
				{
					swprintf_s(TmpStr, MAX_PATH, L"%s\\%04hX%04hX.bsy", cfg.BinkOutboundDir, net, node);
				}
				else
				{
					swprintf_s(TmpStr, MAX_PATH, L"%s\\%04hX%04hX.pnt\\0000%04hX.bsy", cfg.BinkOutboundDir, net, node, point);
				}

				hFile=CreateFileW(TmpStr,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);


				if (hFile!=INVALID_HANDLE_VALUE)
				{
					
					CloseHandle(hFile);
					isBusy=TRUE;
					
				}
				else
				{
					isBusy=FALSE;// бизи нету
					hFile1=CreateFile(TmpStr,GENERIC_READ,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,NULL); //создаем бизи на время паковки
				}


				

				wsprintfW(TmpStr, L"%s\\%s", cfg.TmpOutboundDir, &(FindData.cFileName));

				isNumChanged=FALSE;
				

				//check dir
				wsprintfW(Filename, L"%s\\%u.%u.%u.%u", cfg.FileboxesDir, zone, net, node, point);
				CreateDirectoryW(Filename,NULL);
				
				//get day
				GetLocalTime(&Time);
				switch(Time.wDayOfWeek)
				{
					case 0: lstrcpyW(DayName,L"SU");break;
					case 1: lstrcpyW(DayName,L"MO");break;
					case 2: lstrcpyW(DayName,L"TU");break;
					case 3: lstrcpyW(DayName,L"WE");break;
					case 4: lstrcpyW(DayName,L"TH");break;
					case 5: lstrcpyW(DayName,L"FR");break;
					case 6: lstrcpyW(DayName,L"SA");break;
				}
				
				


newarch:
				if (ArchiveNum>=36)
				{
					//проверяем, не свободно ли имя без изменений
					MakeFileName(Filename, cfg.FileboxesDir, DayName, zone, net, node, point, ArchiveNum % 36);
					isExists=CheckFile(Filename,&FileInfoData);
					if (!isExists)
					{
						isNumChanged=TRUE;
						ArchiveNum=ArchiveNum%36;
						isUseCurrent=FALSE;
						goto packfile;

					}
				

				}

				



				MakeFileName(Filename, cfg.FileboxesDir, DayName, zone, net, node, point, ArchiveNum);
				
				isExists=CheckFile(Filename,&FileInfoData);
				if (!isExists)
				{
					if (isNumChanged)
					{
						isUseCurrent=FALSE;
						goto packfile;
					}
					else
					{
						isNumChanged=TRUE;
						++ArchiveNum;
						goto newarch;
					}
				}
			
				if (isBusy) 
				{
					//бизи, не допаковываем
					isNumChanged=TRUE;
					++ArchiveNum;
					goto newarch;
				}

				if (FileInfoData.nFileSizeLow>=MaxArchiveSize)
				{
					//слишком большой архив, не допаковываем
					isNumChanged=TRUE;
					++ArchiveNum;
					goto newarch;
				}
				else
				{
					isUseCurrent=1;
				}
			
				//все ок, пакуем
packfile:


				wsprintfW(LogStr,L"%u bytes to %u:%u/%u.%u",FindData.nFileSizeLow,zone,net,node,point);
				AddLogEntry(LogStr);
				if (isNumChanged)
				{

					SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&ArchiveNum,0,NULL);
					SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&LinkID,0,NULL);
					SQLExecDirectW(hstmt,L"update Links set NextArchiveExt=? where LinkID=?",SQL_NTS);
					SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
				}
				if (isUseCurrent) 
				{
					wsprintfW(CmdLine, L"%s %s %s", cfg.ZipCommand, Filename, TmpStr);
				}
				else
				{
					MakeTmpFileName(Filename1, cfg.TmpOutboundDir, DayName, net, node, point, ArchiveNum);
					wsprintfW(CmdLine, L"%s %s %s", cfg.ZipCommand, Filename1, TmpStr);
				}
								
				CreateProcessW(NULL, CmdLine, NULL, NULL, FALSE, 0, NULL, cfg.TmpOutboundDir, &StartupInfo, &ProcInfo);
				result=WaitForSingleObject(ProcInfo.hProcess,30000);
				if (result==WAIT_TIMEOUT)
				{
					TerminateProcess(ProcInfo.hProcess,0);
					//
				}
				CloseHandle(ProcInfo.hProcess);
				CloseHandle(ProcInfo.hThread);
				
				if (!isUseCurrent) 
				{
					//перемещаем в файлбокс
					MoveFileExW(Filename1,Filename,MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH );
				}

				if (isBusy==0) CloseHandle(hFile1); //убираем бизи
	
				nextout:
				if (FindNextFileW(hSearch,&FindData)!=0) goto nextoutfile;
			}
			FindClose(hSearch);
}

DWORD WINAPI EchomailOutThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;

	HANDLE hHeap;

	int WaitTime;
	int result; 

	HANDLE hEvent[2];
	SQLLEN cb,cb1,cb2;

	wchar_t tmp;
	
	FTNAddr LinkAddr;
	unsigned int MaxPktSize;

	unsigned int LinkId,MessageId,MessageId1;
	

	lpOutboundQueue FirstData,PrevData,CurrentData;

	unsigned int i;
	size_t strl;
	unsigned short Net,Net1,Node;
	char TmpStr1[13];
	char Pwd[9];
	DWORD cbFile;
	const unsigned short EndSymbs=0;
	wchar_t Filename[MAX_PATH];
	unsigned int PktNumber;
	unsigned int UseAkaId;

	_InterlockedIncrement(&(cfg.ThreadCount));
	WaitTime=INFINITE;

	hEvent[0]=cfg.hExitEvent;
	hEvent[1]=cfg.hEchomailOutEvent;
	AddLogEntry(L"Echomail to .PKT thread started");

loop:
	result=WaitForMultipleObjects(2,hEvent,FALSE,WaitTime);

	if (result==WAIT_TIMEOUT)
	{
		
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
		SQLDisconnect(hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
		WaitTime=INFINITE;
		HeapDestroy(hHeap);
		goto loop;

	}
	if (WaitTime==INFINITE)
	{
		hHeap=HeapCreate(HEAP_NO_SERIALIZE,65536,0);
		SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc); 
		sqlret=SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
		if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
		{	
			printf("SQL ERROR\n");
			SetEvent(cfg.hExitEvent);
			goto threadexit;
			//fatal error
		}
		
		SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
		
		WaitTime=10000;

		//fill links table
		SQLExecDirectW(hstmt,L"select max(linkid) from links",SQL_NTS);
		SQLFetch(hstmt);
		SQLGetData(hstmt,1,SQL_C_ULONG,&NumOfLinks,0,NULL);
		SQLCloseCursor(hstmt);

		LinksInfo=(lpLinkInfo)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,(NumOfLinks+1)*sizeof(LinkInfo));
		memset(Pwd, 0, 9);
		sqlret=SQLExecDirectW(hstmt,L"select LinkId,Zone,Net,Node,Point,MaxPktSize,PktPassword from links order by LinkId",SQL_NTS);
		if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
		{	
			SQLBindCol(hstmt,1,SQL_C_ULONG,&LinkId,0,NULL);
			SQLBindCol(hstmt,2,SQL_C_USHORT,&(LinkAddr.zone),0,NULL);
			SQLBindCol(hstmt,3,SQL_C_USHORT,&(LinkAddr.net),0,NULL);
			SQLBindCol(hstmt,4,SQL_C_USHORT,&(LinkAddr.node),0,NULL);
			SQLBindCol(hstmt,5,SQL_C_USHORT,&(LinkAddr.point),0,NULL);
			SQLBindCol(hstmt,6,SQL_C_ULONG,&MaxPktSize,0,NULL);
			SQLBindCol(hstmt, 7, SQL_C_CHAR, &Pwd, 9, &cb2);
			
			sqlret=SQLFetch(hstmt);
			while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				
				LinksInfo[LinkId].LinkAddr.FullAddr=LinkAddr.FullAddr;
				LinksInfo[LinkId].MaxPktSize=MaxPktSize;
				memcpy(&(LinksInfo[LinkId].PktPwd),Pwd,8);
				memset(Pwd, 0, 9);
				sqlret=SQLFetch(hstmt);
			}
		//
		}
		SQLFreeStmt(hstmt,SQL_UNBIND);
		SQLCloseCursor(hstmt);

	}

	switch (result)
	{
	case (WAIT_OBJECT_0) :
	{
							 goto threadexit;
	}
	case (WAIT_OBJECT_0 + 1) :
	{
			for (UseAkaId = 0; UseAkaId <= cfg.MaxAkaID; UseAkaId++)
			if (cfg.MyAkaTable[UseAkaId].FullAddr != 0)
			{
				FirstData = NULL; PrevData = NULL;
				SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &UseAkaId, 0, NULL);
				sqlret = SQLExecDirectW(hstmt, L"Select Outbound.ToLink,Outbound.MessageID from Outbound,EchoMessages,EchoAreas,Links where EchoMessages.MessageID=Outbound.MessageID and Links.LinkID=Outbound.ToLink and Links.LinkType<=2 and EchoMessages.AreaID=EchoAreas.AreaID and EchoAreas.UseAka=? order by EchoMessages.AreaId,Outbound.MessageId", SQL_NTS);
				if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
				{
					SQLBindCol(hstmt, 1, SQL_C_ULONG, &LinkId, 0, NULL);
					SQLBindCol(hstmt, 2, SQL_C_ULONG, &MessageId, 0, NULL);
					sqlret = SQLFetch(hstmt);
					while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
					{
						CurrentData = (lpOutboundQueue)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OutboundQueue));
						if (FirstData == NULL) FirstData = CurrentData;
						else PrevData->lpNext = CurrentData;
							
						PrevData = CurrentData;
						CurrentData->LinkId = LinkId;
						CurrentData->MessageId = MessageId;
						sqlret = SQLFetch(hstmt);
					}
					SQLFreeStmt(hstmt, SQL_UNBIND);
					SQLCloseCursor(hstmt);
				}
				SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
				MessageId1 = 0;


				while (FirstData != NULL)
				{
					MessageId = FirstData->MessageId;
					LinkId = FirstData->LinkId;
					if (MessageId != MessageId1)
					{
						if (MessageId1 != 0) HeapFree(hHeap, 0, lpPackedMsgBuffer);	// очистить буфер предыдущего сообщения 
						MessageId1 = MessageId;
						
						SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &MessageId, 0, NULL);
						sqlret = SQLExecDirectW(hstmt, L"select EchoMessages.OriginalSize,EchoMessages.MsgText, EchoMessages.CreateTime, EchoMessages.ToName, EchoMessages.FromName, EchoMessages.Subject,  EchoAreas.AreaName, EchoMessages.MsgId, EchoMessages.ReplyTo, EchoMessages.SeenBy, EchoMessages.Path from EchoMessages,EchoAreas where EchoMessages.AreaId=EchoAreas.AreaId and EchoMessages.MessageId=?", SQL_NTS);
						if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
						{
							sqlret = SQLFetch(hstmt);
							if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
							{
								SQL_TIMESTAMP_STRUCT CreateTime;
								char * CurrentStrBuffPosition;
								char TmpStr[128];
								wchar_t TmpStrW[128];
								char * MsgTxtBuff;
								wchar_t * MsgTxtBuffW;
								StringBuffer StrBuff;
								LPVOID BinaryBuff;
								int OriginalSize, PackedSize;
								unsigned char * PackedBuff;

								SQLGetData(hstmt, 1, SQL_C_LONG, &OriginalSize, 0, NULL);


								SQLGetData(hstmt, 2, SQL_C_BINARY, &tmp, 0, &cb);
								PackedSize =(int) cb;

								PackedMsgBufferAllocSize = OriginalSize + 4096;
								lpPackedMsgBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, PackedMsgBufferAllocSize);
								MsgTxtBuffW = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, OriginalSize + 2);
								PackedBuff = HeapAlloc(hHeap, 0, PackedSize);
								SQLGetData(hstmt, 2, SQL_C_BINARY, PackedBuff, PackedSize, 0);

								uncompress((Bytef *)MsgTxtBuffW, &OriginalSize, PackedBuff, PackedSize);
								HeapFree(hHeap, 0, PackedBuff);

								MsgTxtBuff = (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (OriginalSize / 2) + 1);
								WideCharToMultiByte(CP_OEMCP, 0, MsgTxtBuffW, -1, MsgTxtBuff, (int)((OriginalSize / 2) + 1), NULL, NULL);
								HeapFree(hHeap, 0, MsgTxtBuffW);

								PackedMsgBufferRealSize = sizeof(MsgHeader);
								((lpMsgHeader)lpPackedMsgBuffer)->MsgVer = 2;
								((lpMsgHeader)lpPackedMsgBuffer)->attribute = 8;
								//
								((lpMsgHeader)lpPackedMsgBuffer)->origNet = cfg.MyAkaTable[UseAkaId].net;
								((lpMsgHeader)lpPackedMsgBuffer)->origNode = cfg.MyAkaTable[UseAkaId].node;


								CurrentStrBuffPosition = (char *)lpPackedMsgBuffer + PackedMsgBufferRealSize;
								SQLGetData(hstmt, 3, SQL_C_TYPE_TIMESTAMP, &CreateTime, 0, NULL);
								TimeToMessageStr(CurrentStrBuffPosition, &CreateTime);
								PackedMsgBufferRealSize += 20;

								//to user
								CurrentStrBuffPosition = (char *)lpPackedMsgBuffer + PackedMsgBufferRealSize;
								SQLGetData(hstmt, 4, SQL_C_WCHAR, TmpStrW, 72, NULL);
								WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 36, NULL, NULL);
								memcpy(CurrentStrBuffPosition, TmpStr, strlen(TmpStr) + 1);
								PackedMsgBufferRealSize += (strlen(TmpStr) + 1);

								//from user
								CurrentStrBuffPosition = (char *)lpPackedMsgBuffer + PackedMsgBufferRealSize;
								SQLGetData(hstmt, 5, SQL_C_WCHAR, TmpStrW, 72, NULL);
								WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 80, NULL, NULL);
								memcpy(CurrentStrBuffPosition, TmpStr, strlen(TmpStr) + 1);
								PackedMsgBufferRealSize += (strlen(TmpStr) + 1);

								//Subj
								CurrentStrBuffPosition = (char *)lpPackedMsgBuffer + PackedMsgBufferRealSize;
								SQLGetData(hstmt, 6, SQL_C_WCHAR, TmpStrW, 144, NULL);
								WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 80, NULL, NULL);
								memcpy(CurrentStrBuffPosition, TmpStr, strlen(TmpStr) + 1);
								PackedMsgBufferRealSize += (strlen(TmpStr) + 1);
								
								//areaname

								InitStringBuffer(hHeap, &StrBuff);
								AddStrToBuffer(&StrBuff, "AREA:");
								SQLGetData(hstmt, 7, SQL_C_WCHAR, TmpStrW, 160, NULL);
								WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 80, NULL, NULL);
								AddStrToBuffer(&StrBuff, TmpStr);
								AddStrToBuffer(&StrBuff, "\r");

								//msgid
								SQLGetData(hstmt, 8, SQL_C_WCHAR, &tmp, 0, &cb1);
								if (cb1 != SQL_NULL_DATA)
								{
									AddStrToBuffer(&StrBuff, "\01MSGID: ");
									SQLGetData(hstmt, 8, SQL_C_WCHAR, TmpStrW, 256, NULL);
									WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 128, NULL, NULL);
									AddStrToBuffer(&StrBuff, TmpStr);
									AddStrToBuffer(&StrBuff, "\r");
								}
								
								//replyto
								SQLGetData(hstmt, 9, SQL_C_WCHAR, &tmp, 0, &cb1);
								if (cb1 != SQL_NULL_DATA)
								{
									AddStrToBuffer(&StrBuff, "\01REPLY: ");
									SQLGetData(hstmt, 9, SQL_C_WCHAR, TmpStrW, 256, NULL);
									WideCharToMultiByte(CP_OEMCP, 0, TmpStrW, -1, TmpStr, 128, NULL, NULL);
									AddStrToBuffer(&StrBuff, TmpStr);
									AddStrToBuffer(&StrBuff, "\r");
								}


								//msgtext
								AddStrToBuffer(&StrBuff, MsgTxtBuff);
								HeapFree(hHeap, 0, MsgTxtBuff);
								//seenby

								SQLGetData(hstmt, 10, SQL_C_BINARY, &tmp, 0, &cb1);
								BinaryBuff = HeapAlloc(hHeap, 0, cb1);
								SQLGetData(hstmt, 10, SQL_C_BINARY, BinaryBuff, cb1, 0);
								strl = 0;
								for (i = 0; i < cb1 / 4; i++)
								{
									memcpy(&Node, (char *)BinaryBuff + i * 4, 2);
									memcpy(&Net, (char *)BinaryBuff + i * 4 + 2, 2);
									if (strl == 0)
									{
										AddStrToBuffer(&StrBuff, "SEEN-BY:");
										wsprintfA(TmpStr1, " %u/%u", Net, Node);
										Net1 = Net;
										strl = 8;
									}
									else
									{
										if (Net != Net1)
										{
											wsprintfA(TmpStr1, " %u/%u", Net, Node);
											Net1 = Net;
										}
										else
										{
											wsprintfA(TmpStr1, " %u", Node);
										}
									}
									if (strlen(TmpStr1) + strl < 78)
									{
										strl += strlen(TmpStr1);
										AddStrToBuffer(&StrBuff, TmpStr1);
									}
									else
									{
										AddStrToBuffer(&StrBuff, "\rSEEN-BY:");
										wsprintfA(TmpStr1, " %u/%u", Net, Node);
										AddStrToBuffer(&StrBuff, TmpStr1);
										Net1 = Net;
										strl = 8 + strlen(TmpStr1);
									}

								}
								AddStrToBuffer(&StrBuff, "\r");
								HeapFree(hHeap, 0, BinaryBuff);
								
								//path
								SQLGetData(hstmt, 11, SQL_C_BINARY, &tmp, 0, &cb1);
								BinaryBuff = HeapAlloc(hHeap, 0, cb1);
								SQLGetData(hstmt, 11, SQL_C_BINARY, BinaryBuff, cb1, 0);
								strl = 0;
								for (i = 0; i < cb1 / 4; i++)
								{
									memcpy(&Node, (char *)BinaryBuff + i * 4, 2);
									memcpy(&Net, (char *)BinaryBuff + i * 4 + 2, 2);
									if (strl == 0)
									{
										AddStrToBuffer(&StrBuff, "\01PATH:");
										wsprintfA(TmpStr1, " %u/%u", Net, Node);
										Net1 = Net;
										strl = 6;
									}
									else
									{
										if (Net != Net1)
										{
											wsprintfA(TmpStr1, " %u/%u", Net, Node);
											Net1 = Net;
										}
										else
										{
											wsprintfA(TmpStr1, " %u", Node);
										}
									}
									if (strlen(TmpStr1) + strl < 78)
									{
										strl += strlen(TmpStr1);
										AddStrToBuffer(&StrBuff, TmpStr1);
									}
									else
									{
										AddStrToBuffer(&StrBuff, "\r\01PATH:");
										wsprintfA(TmpStr1, " %u/%u", Net, Node);
										AddStrToBuffer(&StrBuff, TmpStr1);
										Net1 = Net;
										strl = 6 + strlen(TmpStr1);
									}
								}
								AddStrToBuffer(&StrBuff, "\r");
								HeapFree(hHeap, 0, BinaryBuff);


								CurrentStrBuffPosition = (char *)lpPackedMsgBuffer + PackedMsgBufferRealSize;
								memcpy(CurrentStrBuffPosition, StrBuff.lpBuffer, StrBuff.CurrentSize + 1);
								PackedMsgBufferRealSize += (StrBuff.CurrentSize + 1);
								StringBufferFreeMem(&StrBuff);

							}
						}
						SQLFreeStmt(hstmt, SQL_UNBIND);
						SQLCloseCursor(hstmt);
						SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
					}
					if (LinksInfo[LinkId].LinkAddr.zone == 0) goto FileError; //ошибка таблицы линков
					//записать уже имеющееся сообщение на очередного линка
					if (LinksInfo[LinkId].CurrentSize == 0)
					{
						//create file
						PktNumber = GetPktNumber(hstmt);

						wsprintfW(Filename, L"%s\\%08X.PKT", cfg.TmpOutboundDir, PktNumber);
						LinksInfo[LinkId].hFile = CreateFileW(Filename, GENERIC_ALL, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

						if (LinksInfo[LinkId].hFile == INVALID_HANDLE_VALUE) goto FileError;

						WritePktHeader(LinksInfo[LinkId].hFile, &(cfg.MyAkaTable[UseAkaId]), &(LinksInfo[LinkId].LinkAddr), (char *)&(LinksInfo[LinkId].PktPwd));
						
						LinksInfo[LinkId].CurrentSize = sizeof(PktHeader);
					}
					
					((lpMsgHeader)lpPackedMsgBuffer)->destNet = LinksInfo[LinkId].LinkAddr.net;
					((lpMsgHeader)lpPackedMsgBuffer)->destNode = LinksInfo[LinkId].LinkAddr.node;
					WriteFile(LinksInfo[LinkId].hFile, lpPackedMsgBuffer, (DWORD)PackedMsgBufferRealSize, &cbFile, NULL);
					LinksInfo[LinkId].CurrentSize += PackedMsgBufferRealSize;
					if (LinksInfo[LinkId].CurrentSize >= LinksInfo[LinkId].MaxPktSize)
					{
						//close
						ClosePktFile(LinksInfo[LinkId].hFile);
						LinksInfo[LinkId].CurrentSize = 0;
					}

										 //удалить запись из Outbound
					SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &LinkId, 0, NULL);
					SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &MessageId, 0, NULL);
					SQLExecDirectW(hstmt, L"delete from Outbound where ToLink=? and MessageId=?", SQL_NTS);
					SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
FileError:
					CurrentData = FirstData->lpNext;
					HeapFree(hHeap, 0, FirstData);
					FirstData = CurrentData;
				}
				
			}
		}
		if (MessageId1 != 0) 	HeapFree(hHeap, 0, lpPackedMsgBuffer);
		
		//close files
		for (LinkId = 1; LinkId <= NumOfLinks; LinkId++)
			if (LinksInfo[LinkId].CurrentSize != 0)
			{
				ClosePktFile(LinksInfo[LinkId].hFile);
				LinksInfo[LinkId].CurrentSize = 0;
			}
		
	}
	
	PackEchoMessages(hstmt);
	goto loop;

threadexit:
	
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}