/*
* 
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/



#include "Soaron.h"


void GetNetmailMessages(HSTMT hstmt, HANDLE hHeap, lpNetmailOutQueue lpNMOQ)
{
	SQLRETURN sqlret;
	SQLLEN cb;
	lpNetmailMessage lpNetmailMsg;
	
	wchar_t tmp;
	unsigned char tmpbit;



	
	sqlret=SQLFetch(hstmt);
	while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
	{
		lpNetmailMsg=(lpNetmailMessage)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(NetmailMessage));
		SQLGetData(hstmt,1,SQL_C_ULONG,&(lpNetmailMsg->MessageID),0,NULL);
		SQLGetData(hstmt,2,SQL_C_USHORT,&(lpNetmailMsg->FromAddr.zone),0,NULL);
		SQLGetData(hstmt,3,SQL_C_USHORT,&(lpNetmailMsg->FromAddr.net),0,NULL);
		SQLGetData(hstmt,4,SQL_C_USHORT,&(lpNetmailMsg->FromAddr.node),0,NULL);
		SQLGetData(hstmt,5,SQL_C_USHORT,&(lpNetmailMsg->FromAddr.point),0,NULL);
		SQLGetData(hstmt,6,SQL_C_USHORT,&(lpNetmailMsg->ToAddr.zone),0,NULL);
		SQLGetData(hstmt,7,SQL_C_USHORT,&(lpNetmailMsg->ToAddr.net),0,NULL);
		SQLGetData(hstmt,8,SQL_C_USHORT,&(lpNetmailMsg->ToAddr.node),0,NULL);
		SQLGetData(hstmt,9,SQL_C_USHORT,&(lpNetmailMsg->ToAddr.point),0,NULL);
		SQLGetData(hstmt,10,SQL_C_TYPE_TIMESTAMP,&(lpNetmailMsg->CreateTime),0,NULL);
		
		SQLGetData(hstmt,11,SQL_C_WCHAR,&tmp,0,&cb);
		lpNetmailMsg->FromName=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
		SQLGetData(hstmt,11,SQL_C_WCHAR,lpNetmailMsg->FromName,cb+2,NULL);
					
		SQLGetData(hstmt,12,SQL_C_WCHAR,&tmp,0,&cb);
		lpNetmailMsg->ToName=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
		SQLGetData(hstmt,12,SQL_C_WCHAR,lpNetmailMsg->ToName,cb+2,NULL);
					
		SQLGetData(hstmt,13,SQL_C_WCHAR,&tmp,0,&cb);
		lpNetmailMsg->Subject=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
		SQLGetData(hstmt,13,SQL_C_WCHAR,lpNetmailMsg->Subject,cb+2,NULL);
					
		SQLGetData(hstmt,14,SQL_C_WCHAR,&tmp,0,&cb);
		if (cb!=SQL_NULL_DATA) 
		{
			lpNetmailMsg->MsgId=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
			SQLGetData(hstmt,14,SQL_C_WCHAR,lpNetmailMsg->MsgId,cb+2,NULL);
		}

		SQLGetData(hstmt,15,SQL_C_WCHAR,&tmp,0,&cb);
		if (cb!=SQL_NULL_DATA) 
		{
			lpNetmailMsg->ReplyTo=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
			SQLGetData(hstmt,15,SQL_C_WCHAR,lpNetmailMsg->ReplyTo,cb+2,NULL);
		}
					
		SQLGetData(hstmt,16,SQL_C_WCHAR,&tmp,0,&cb);
		lpNetmailMsg->MsgText=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cb+2);
		SQLGetData(hstmt,16,SQL_C_WCHAR,lpNetmailMsg->MsgText,cb+2,NULL);
					
		SQLGetData(hstmt,17,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->killsent=1;

		SQLGetData(hstmt,18,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->pvt=1;

		SQLGetData(hstmt,19,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->fileattach=1;
					
		SQLGetData(hstmt,20,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->arq=1;
					
		SQLGetData(hstmt,21,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->rrq=1;
					
		SQLGetData(hstmt,22,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->returnreq=1;

		SQLGetData(hstmt,23,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->direct=1;

		SQLGetData(hstmt,24,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->cfm=1;
					
		SQLGetData(hstmt,25,SQL_C_BIT,&tmpbit,0,NULL);
		if (tmpbit!=0) lpNetmailMsg->recv=1;

		if (lpNMOQ->First == NULL)
		{
			lpNMOQ->First = lpNetmailMsg;
		}
		else
		{
			lpNMOQ->Last->NextMsg = lpNetmailMsg;
		}
		lpNMOQ->Last = lpNetmailMsg;

		sqlret=SQLFetch(hstmt);

	}
	SQLCloseCursor(hstmt);
}


void WriteNetmailMessage(HANDLE hPktFile, HANDLE hHeap, lpNetmailMessage lpNetmailMsg)
{
	StringBuffer StrBuf;
	char TmpStr[80];
	MsgHeader MsgHdr;
	DWORD cbfile;
	char * MsgTxt;
	size_t cb;


	InitStringBuffer(hHeap, &StrBuf);
	//header

	MsgHdr.MsgVer = 2;
	MsgHdr.origNet = lpNetmailMsg->FromAddr.net;
	MsgHdr.origNode = lpNetmailMsg->FromAddr.node;
	MsgHdr.destNet = lpNetmailMsg->ToAddr.net;
	MsgHdr.destNode = lpNetmailMsg->ToAddr.node;
	MsgHdr.cost = 0;
	MsgHdr.attribute = 0;
	if (lpNetmailMsg->pvt) MsgHdr.attribute |= 1;
	if (lpNetmailMsg->fileattach) MsgHdr.attribute |= 16;
	if (lpNetmailMsg->rrq) MsgHdr.attribute |= 4096;
	if (lpNetmailMsg->returnreq) MsgHdr.attribute |= 8192;
	if (lpNetmailMsg->arq) MsgHdr.attribute |= 16384;
	WriteFile(hPktFile, &MsgHdr, sizeof(MsgHeader), &cbfile, NULL);
	//
	TimeToMessageStr(TmpStr, &(lpNetmailMsg->CreateTime));

	WriteFile(hPktFile, TmpStr, 20, &cbfile, NULL);
	//
	WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->ToName, -1, TmpStr, 36, NULL, NULL);
	WriteFile(hPktFile, TmpStr, (DWORD)(strlen(TmpStr) + 1), &cbfile, NULL);

	WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->FromName, -1, TmpStr, 36, NULL, NULL);
	WriteFile(hPktFile, TmpStr, (DWORD)(strlen(TmpStr) + 1), &cbfile, NULL);

	WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->Subject, -1, TmpStr, 72, NULL, NULL);
	WriteFile(hPktFile, TmpStr, (DWORD)(strlen(TmpStr) + 1), &cbfile, NULL);
	//
	if ((lpNetmailMsg->pvt) || (lpNetmailMsg->cfm) || (lpNetmailMsg->fileattach) || (lpNetmailMsg->rrq))
	{
		wsprintfA(TmpStr, "\01FLAGS");
		if (lpNetmailMsg->pvt) lstrcatA(TmpStr, " PVT");
		if (lpNetmailMsg->killsent) lstrcatA(TmpStr, " K/S"); else lstrcatA(TmpStr, " SNT");
		if (lpNetmailMsg->cfm) lstrcatA(TmpStr, " CFM");
		if (lpNetmailMsg->fileattach) lstrcatA(TmpStr, " FIL");
		if (lpNetmailMsg->rrq) lstrcatA(TmpStr, " RRQ");
		

		lstrcatA(TmpStr, "\r");
		AddStrToBuffer(&StrBuf, TmpStr);
	}
	//
	wsprintfA(TmpStr, "\01INTL %u:%u/%u %u:%u/%u\r", lpNetmailMsg->ToAddr.zone, lpNetmailMsg->ToAddr.net, lpNetmailMsg->ToAddr.node, lpNetmailMsg->FromAddr.zone, lpNetmailMsg->FromAddr.net, lpNetmailMsg->FromAddr.node);
	AddStrToBuffer(&StrBuf, TmpStr);
	//fmpt
	if (lpNetmailMsg->FromAddr.point != 0)
	{
		wsprintfA(TmpStr, "\01FMPT %u\r", lpNetmailMsg->FromAddr.point);
		AddStrToBuffer(&StrBuf, TmpStr);
	}
	//topt
	if (lpNetmailMsg->ToAddr.point != 0)
	{
		wsprintfA(TmpStr, "\01TOPT %u\r", lpNetmailMsg->ToAddr.point);
		AddStrToBuffer(&StrBuf, TmpStr);
	}
	//MSGID
	if (lpNetmailMsg->MsgId != NULL)
	{
		AddStrToBuffer(&StrBuf, "\01MSGID: ");
		WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->MsgId, -1, TmpStr, 72, NULL, NULL);
		lstrcatA(TmpStr, "\r");
		AddStrToBuffer(&StrBuf, TmpStr);
	}
	if (lpNetmailMsg->ReplyTo != NULL)
	{
		AddStrToBuffer(&StrBuf, "\01REPLY: ");
		WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->ReplyTo, -1, TmpStr, 72, NULL, NULL);
		lstrcatA(TmpStr, "\r");
		AddStrToBuffer(&StrBuf, TmpStr);
	}


	//msgtext
	cb = wcslen(lpNetmailMsg->MsgText) + 1;
	MsgTxt = (char *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cb);
	WideCharToMultiByte(CP_OEMCP, 0, lpNetmailMsg->MsgText, -1, MsgTxt, (int)cb, NULL, NULL);
	AddStrToBuffer(&StrBuf, MsgTxt);
	HeapFree(hHeap, 0, MsgTxt);
	
	
	WriteFile(hPktFile, StrBuf.lpBuffer, (DWORD)(StrBuf.CurrentSize + 1), &cbfile, NULL);
	
	StringBufferFreeMem(&StrBuf);
	
}


DWORD WINAPI NetmailOutThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	SQLLEN cb;
	HANDLE hHeap;

	FTNAddr LastAddr;
	HANDLE hPktFile;
	wchar_t tmpFileName[MAX_PATH], finalPktFileName[MAX_PATH],FileboxDirName[MAX_PATH];
	wchar_t LogStr[255];
	unsigned int PktNumber;
	char PktPwd[9];

	NetmailOutQueue NDOQ;



	int WaitTime;
	int result; 

	HANDLE hEvent[2];
	
	InterlockedIncrement(&(cfg.ThreadCount));

	WaitTime=INFINITE;


	hEvent[0]=cfg.hExitEvent;
	hEvent[1]=cfg.hNetmailOutEvent;
	AddLogEntry(L"Netmail out thread started");

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
		hHeap=HeapCreate(HEAP_NO_SERIALIZE,16384,0);
		SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc); 
		sqlret=SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
		if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
		{		
			SetEvent(cfg.hExitEvent);
			goto threadexit;
			//fatal error
		}
		SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
		
		WaitTime=10000;
	}

	switch(result)
	{
	case (WAIT_OBJECT_0):
		{
			goto threadexit;
		}
	case (WAIT_OBJECT_0+1):
		{
			
			NDOQ.First=NULL;
			NDOQ.Last=NULL;
			
			EnterCriticalSection(&NetmailRouteCritSect);
			SQLExecDirectW(hstmt, L"execute sp_DirectNetmail", SQL_NTS); // обработать директный нетмейл для использующих FTN Service линков
			LeaveCriticalSection(&NetmailRouteCritSect);

			sqlret=SQLExecDirectW(hstmt,L"select MessageID,FromZone,FromNet,FromNode,FromPoint,ToZone,ToNet,ToNode,ToPoint,CreateTime,FromName,ToName,Subject,MsgId,ReplyTo,MsgText,KillSent,Pvt,FileAttach,Arq,RRq,ReturnReq,Direct,Cfm,recv from Netmail where direct=1 and sent=0 and Locked=0 order by ToZone,ToNet,ToNode,MessageID",SQL_NTS);
			if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				GetNetmailMessages(hstmt,hHeap,&NDOQ);
			}	
			
			sqlret=SQLExecDirectW(hstmt,L"select MessageID,FromZone,FromNet,FromNode,FromPoint,ToZone,ToNet,ToNode,ToPoint,CreateTime,FromName,ToName,Subject,MsgId,ReplyTo,MsgText,KillSent,Pvt,FileAttach,Arq,RRq,ReturnReq,Direct,Cfm,recv from Netmail,MyAka where direct=0 and sent=0 and Locked=0 and MyAka.Point=0 and Netmail.ToPoint<>0 and Netmail.ToZone=MyAka.Zone and Netmail.ToNet=MyAka.Net and Netmail.ToNode=MyAka.Node order by ToPoint,MessageID",SQL_NTS);
			if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				GetNetmailMessages(hstmt, hHeap, &NDOQ);
			}	
			
			sqlret = SQLExecDirectW(hstmt, L"select MessageID,FromZone,FromNet,FromNode,FromPoint,ToZone,ToNet,ToNode,ToPoint,CreateTime,FromName,ToName,Subject,MsgId,ReplyTo,MsgText,KillSent,Pvt,FileAttach,Arq,RRq,ReturnReq,Direct,Cfm,recv from Netmail,Links where direct=0 and sent=0 and Locked=0 and Netmail.ToZone=Links.Zone and Netmail.ToNet=Links.Net and Netmail.ToNode=Links.Node and Links.NetmailDirect<>0 and Links.LinkType=2 order by Links.LinkID,MessageID", SQL_NTS);
			if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
			{
				GetNetmailMessages(hstmt, hHeap, &NDOQ);
			}

			
			LastAddr.FullAddr=0;
			hPktFile=INVALID_HANDLE_VALUE;
			
			while (NDOQ.First!=NULL)
			{
				lpNetmailMessage lpTmp;
				wsprintfW(LogStr,L"Direct netmail From %u:%u/%u.%u To %u:%u/%u.%u",NDOQ.First->FromAddr.zone,NDOQ.First->FromAddr.net,NDOQ.First->FromAddr.node,NDOQ.First->FromAddr.point,NDOQ.First->ToAddr.zone,NDOQ.First->ToAddr.net,NDOQ.First->ToAddr.node,NDOQ.First->ToAddr.point);
				AddLogEntry(LogStr);
				
				if (LastAddr.FullAddr!=NDOQ.First->ToAddr.FullAddr)
				{
					if(LastAddr.FullAddr!=0)
					{
						ClosePktFile(hPktFile);
						CreateDirectoryW(FileboxDirName,NULL);
						MoveFileExW(tmpFileName,finalPktFileName,MOVEFILE_COPY_ALLOWED);
						
					}
					LastAddr.FullAddr=NDOQ.First->ToAddr.FullAddr;
					//create file
					PktNumber=GetPktNumber(hstmt);
					wsprintfW(FileboxDirName, L"%s\\%u.%u.%u.%u", cfg.FileboxesDir, NDOQ.First->ToAddr.zone, NDOQ.First->ToAddr.net, NDOQ.First->ToAddr.node, NDOQ.First->ToAddr.point);
					wsprintfW(tmpFileName, L"%s\\%08X.NETMAIL", cfg.TmpOutboundDir, PktNumber);
					wsprintfW(finalPktFileName,L"%s\\%08X.PKT",FileboxDirName,PktNumber);
					hPktFile=CreateFileW(tmpFileName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);
					
					
					
					memset(PktPwd, 0, 9);
					SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(NDOQ.First->ToAddr.zone), 0, NULL);
					SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(NDOQ.First->ToAddr.net), 0, NULL);
					SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(NDOQ.First->ToAddr.node), 0, NULL);
					SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(NDOQ.First->ToAddr.point), 0, NULL);
					sqlret = SQLExecDirectW(hstmt, L"Select PktPassword from Links where Zone=? and Net=? and Node=? and Point=?", SQL_NTS);
					if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
					{
						sqlret = SQLFetch(hstmt);
						if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
						{
							SQLGetData(hstmt, 1, SQL_C_CHAR, PktPwd, 9, &cb);
						}
					}
					SQLCloseCursor(hstmt);
					SQLFreeStmt(hstmt, SQL_RESET_PARAMS);



					WritePktHeader(hPktFile,&(cfg.MyAddr),&(NDOQ.First->ToAddr),PktPwd); 
					
				}
				//
				WriteNetmailMessage(hPktFile,hHeap,NDOQ.First);
				//
				
				SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&(NDOQ.First->MessageID),0,NULL);
				sqlret=SQLExecDirectW(hstmt,L"{call sp_NetmailMessageSent(?)}",SQL_NTS);
				SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

				//
				lpTmp=NDOQ.First->NextMsg;
				HeapFree(hHeap,0,NDOQ.First->MsgText);
				if (NDOQ.First->ReplyTo!=NULL) HeapFree(hHeap,0,NDOQ.First->ReplyTo);
				if (NDOQ.First->MsgId!=NULL) HeapFree(hHeap,0,NDOQ.First->MsgId);
				HeapFree(hHeap,0,NDOQ.First->Subject);
				HeapFree(hHeap,0,NDOQ.First->ToName);
				HeapFree(hHeap,0,NDOQ.First->FromName);
				HeapFree(hHeap,0,NDOQ.First);
				NDOQ.First=lpTmp;
			}

			if (hPktFile!=INVALID_HANDLE_VALUE)
			{
				ClosePktFile(hPktFile);
				CreateDirectoryW(FileboxDirName,NULL);
				MoveFileExW(tmpFileName,finalPktFileName,MOVEFILE_COPY_ALLOWED);
			}

			
			//
			EnterCriticalSection(&NetmailRouteCritSect);
					
			SQLExecDirectW(hstmt, L"EXECUTE sp_RouteNetmail",SQL_NTS);
			
			LeaveCriticalSection(&NetmailRouteCritSect);
			
			SetEvent(cfg.hMailerCallGeneratingEvent);

			//make polls
			sqlret = SQLExecDirectW(hstmt, L"select Zone,Net,Node from Links,NetmailOutbound,Netmail where Links.LinkID=NetmailOutbound.ToLinkID and Netmail.MessageID=NetmailOutbound.MessageID and Netmail.Locked=0 and Links.DialOut<>0 and Links.LinkType<=2 and Links.Point=0", SQL_NTS);
			if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
			{
				unsigned short zone, net, node;
				SQLBindCol(hstmt, 1, SQL_C_USHORT, &zone, 0, NULL);
				SQLBindCol(hstmt, 2, SQL_C_USHORT, &net, 0, NULL);
				SQLBindCol(hstmt, 3, SQL_C_USHORT, &node, 0, NULL);
				
				sqlret = SQLFetch(hstmt);
				while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
				{
					if (zone == cfg.MyAddr.zone)
					{
						wsprintfW(LogStr,L"Creating poll to %u:%u/%u", zone, net, node);
						AddLogEntry(LogStr);
												
						swprintf_s(tmpFileName, MAX_PATH, L"%s\\%04hX%04hX.CLO", cfg.BinkOutboundDir, net, node);
						hPktFile = CreateFileW(tmpFileName, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
						CloseHandle(hPktFile);
						//
					}
					sqlret = SQLFetch(hstmt);
				}
				SQLCloseCursor(hstmt);
				SQLFreeStmt(hstmt, SQL_UNBIND);
			}

	
		}
	}
	goto loop;

threadexit:
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}

void LogSessionAndSendNetmailToLink(lpFTNAddr lpLinkAddr, unsigned char SoftwareCode)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	SQLLEN cb;

	HANDLE hHeap;
	HANDLE hPktFile;
	
	wchar_t tmpPktFileName[MAX_PATH], finalPktFileName[MAX_PATH], FileboxDirName[MAX_PATH];
	wchar_t LogStr[255];
	unsigned int PktNumber;

	unsigned int LinkID;
	NetmailOutQueue NOQ;
	char PktPwd[9];



	hHeap = HeapCreate(HEAP_NO_SERIALIZE, 16384, 0);
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent);
		return;
		//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &LinkID, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpLinkAddr->zone), 0, NULL);
	SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpLinkAddr->net), 0, NULL);
	SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpLinkAddr->node), 0, NULL);
	SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_SMALLINT, 0, 0, &(lpLinkAddr->point), 0, NULL);
	SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_UTINYINT , SQL_TINYINT, 0, 0, &SoftwareCode, 0, NULL);
	SQLExecDirectW(hstmt, L"{?=call sp_GetLinkIdForNetmailRouting(?,?,?,?,?)}", SQL_NTS);

	

	NOQ.First = NULL;
	NOQ.Last = NULL;

	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	if (SoftwareCode != 1) goto exit;

	if (LinkID != 0)
	{
		//netmail out//
		memset(PktPwd, 0, 9);
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &LinkID, 0, NULL);
		sqlret = SQLExecDirectW(hstmt, L"select PktPassword from Links where LinkID=?", SQL_NTS);//
		if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			sqlret = SQLFetch(hstmt);
			if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
			{
				SQLGetData(hstmt, 1, SQL_C_CHAR, PktPwd, 9, &cb);
			}
		}
		SQLCloseCursor(hstmt);
		
		EnterCriticalSection(&NetmailRouteCritSect);
		sqlret = SQLExecDirectW(hstmt, L"select Netmail.MessageID,FromZone,FromNet,FromNode,FromPoint,ToZone,ToNet,ToNode,ToPoint,CreateTime,FromName,ToName,Subject,MsgId,ReplyTo,MsgText,KillSent,Pvt,FileAttach,Arq,RRq,ReturnReq,Direct,Cfm,recv from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID and NetmailOutbound.ToLinkID=? order by Netmail.MessageID", SQL_NTS);//
		if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			GetNetmailMessages(hstmt, hHeap, &NOQ);
		}
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

		if (NOQ.First != NULL)
		{

			PktNumber = GetPktNumber(hstmt);
			wsprintfW(FileboxDirName, L"%s\\%u.%u.%u.0", cfg.FileboxesDir, lpLinkAddr->zone, lpLinkAddr->net, lpLinkAddr->node);
			wsprintfW(tmpPktFileName, L"%s\\%08X.NETMAIL", cfg.TmpOutboundDir, PktNumber);
			wsprintfW(finalPktFileName, L"%s\\%08X.PKT", FileboxDirName, PktNumber);
			hPktFile = CreateFileW(tmpPktFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
			
			WritePktHeader(hPktFile, &(cfg.MyAddr),lpLinkAddr, PktPwd); 
			//
			while (NOQ.First != NULL)
			{
				lpNetmailMessage lpTmp;
				//
				wsprintfW(LogStr, L"Dynamic netmail %u:%u/%u.%u -> %u:%u/%u.%u thru %u:%u/%u", NOQ.First->FromAddr.zone, NOQ.First->FromAddr.net, NOQ.First->FromAddr.node, NOQ.First->FromAddr.point, NOQ.First->ToAddr.zone, NOQ.First->ToAddr.net, NOQ.First->ToAddr.node, NOQ.First->ToAddr.point, lpLinkAddr->zone, lpLinkAddr->net, lpLinkAddr->node);
				AddLogEntry(LogStr);
				
				WriteNetmailMessage(hPktFile, hHeap, NOQ.First);

				SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(NOQ.First->MessageID), 0, NULL);
				sqlret = SQLExecDirectW(hstmt, L"{call sp_NetmailMessageSent(?)}", SQL_NTS);
				SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
				//
				lpTmp = NOQ.First->NextMsg;
				HeapFree(hHeap, 0, NOQ.First->MsgText);
				if (NOQ.First->ReplyTo != NULL) HeapFree(hHeap, 0, NOQ.First->ReplyTo);
				if (NOQ.First->MsgId != NULL) HeapFree(hHeap, 0, NOQ.First->MsgId);
				HeapFree(hHeap, 0, NOQ.First->Subject);
				HeapFree(hHeap, 0, NOQ.First->ToName);
				HeapFree(hHeap, 0, NOQ.First->FromName);
				HeapFree(hHeap, 0, NOQ.First);
				NOQ.First = lpTmp;
			}
			if (hPktFile != INVALID_HANDLE_VALUE)
			{
				ClosePktFile(hPktFile);
				CreateDirectoryW(FileboxDirName, NULL);
				MoveFileExW(tmpPktFileName, finalPktFileName, MOVEFILE_COPY_ALLOWED);
			}
			//
		}
		LeaveCriticalSection(&NetmailRouteCritSect);
	}

	exit:
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	HeapDestroy(hHeap);
}