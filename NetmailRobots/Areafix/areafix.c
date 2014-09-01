/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

unsigned int StrCount,MsgCount;
WStringBuffer MsgStrBuf;
SQLHDBC  hdbc0,hdbc,hdbc1;
HSTMT hStmt0,hStmt,hStmt1;
lpNetmailMessage lpMsg1;
HANDLE hHeap;

void PutAreaFixMsg(void)
{	
	wchar_t Subj[80];
	if (MsgCount!=0) wsprintfW(Subj,L"Reply from Areafix - part %u",MsgCount+1);
	else lstrcpyW(Subj,L"Reply from Areafix");
	PostReplyMsg(hHeap,lpMsg1, hStmt0,L"Areafix Service", Subj,MsgStrBuf.lpBuffer,1,1);
}

void AddLine(const wchar_t * StrToAdd)
{
	AddWStrToBuffer(&MsgStrBuf,StrToAdd);
	++StrCount;
	if (StrCount>=500)
	{
		PutAreaFixMsg();
		++MsgCount;
		StrCount=0;
		WStringBufferFreeMem(&MsgStrBuf);
		InitWStringBuffer(hHeap,&MsgStrBuf);
	}

}


__declspec(dllexport) BOOL NetmailRobotFunc(lpNetmailMessage lpMsg,int FromLinkId,BOOL isPwdOk)
{

	unsigned int i,j;
	SQLRETURN sqlret;
	wchar_t lpCmdStr[256];
	wchar_t TmpStr[256];
	wchar_t AreaName[256];
	wchar_t r,w,m,p;
	wchar_t * lpTrimmedStr, *lpHelpStr;
	int AreaId;
	int Result;
	BOOL isAdded=FALSE;
	

	SQLLEN cb1,cb2,cb3,cb4,cb5,cb6;
	unsigned char AllowRead,AllowWrite,Mandatory,Passive,GroupAllowRead,GroupAllowWrite;


	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc); 
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hStmt);
	
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc1); 
	sqlret = SQLDriverConnectW(hdbc1, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{	SQLDisconnect(hdbc);
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hStmt1);

	SQLAllocHandle(SQL_HANDLE_DBC,cfg.henv, &hdbc0); 
	sqlret = SQLDriverConnectW(hdbc0, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		SQLDisconnect(hdbc);
		SQLDisconnect(hdbc1);
		SetEvent(cfg.hExitEvent); return FALSE;//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc0, &hStmt0);
	hHeap = HeapCreate(HEAP_NO_SERIALIZE, 8192, 0);

	if (!isPwdOk)
	{
		PostReplyMsg(hHeap,lpMsg, hStmt0,L"Areafix Service", L"Error", L"Hеизвестный линк или неверный пароль\r",0,1);
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt1);
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
		SQLFreeHandle(SQL_HANDLE_STMT,hStmt0);
		SQLDisconnect(hdbc1);
		SQLDisconnect(hdbc);
		SQLDisconnect(hdbc0);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc1);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,hdbc0);
		return FALSE;
	}

	lpMsg1=lpMsg;
	
	InitWStringBuffer(hHeap, &MsgStrBuf);
	StrCount=0;
	MsgCount=0;
	j=0;
	
NextStr:
	i=j;

	while (((lpMsg->MsgText)[i]!=L'\n')&&((lpMsg->MsgText)[i]!=L'\r')&&((lpMsg->MsgText)[i]!=0)&&(i-j<255)) ++i;
	memset(lpCmdStr,0,512);
	memset(TmpStr,0,512);
	
	
	memcpy(lpCmdStr,&((lpMsg->MsgText)[j]),(i-j)*2);

//добавить проверку на слишком длинную строку
	if (_memicmp(lpCmdStr,L" * Origin",18)==0)  goto EndAnalyse; //origin
	if (memcmp(lpCmdStr,L"---",6)==0)  goto EndAnalyse; //tearline

	if ((_memicmp(lpCmdStr,L"%all",8)==0)||(_memicmp(lpCmdStr,L"+%all",10)==0))    
		lstrcpyW(lpCmdStr,L"+*");
	else
		if ((_memicmp(lpCmdStr,L"-%all",10)==0)||(_memicmp(lpCmdStr,L"%-all",10)==0))    
		lstrcpyW(lpCmdStr,L"-*");



	lpTrimmedStr=TrimWStr(lpCmdStr);
	//анализируем команды
	if (lpTrimmedStr[0]==1) goto EndAnalyse; //кладжи
	if (lstrlenW(lpTrimmedStr)==0) goto EndAnalyse;
	if (lpTrimmedStr[0]==L'%') 
	{
		if (_memicmp(lpTrimmedStr,L"%list",10)==0) //list
		{
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLBindCol(hStmt,1,SQL_C_WCHAR,AreaName,510,NULL);
			SQLBindCol(hStmt,2,SQL_C_BIT,&AllowRead,0,&cb1);
			SQLBindCol(hStmt,3,SQL_C_BIT,&AllowWrite,0,&cb2);
			SQLBindCol(hStmt,4,SQL_C_BIT,&Mandatory,0,&cb3);
			SQLBindCol(hStmt,5,SQL_C_BIT,&Passive,0,&cb4);
			SQLBindCol(hStmt,6,SQL_C_BIT,&GroupAllowRead,0,&cb5);
			SQLBindCol(hStmt,7,SQL_C_BIT,&GroupAllowWrite,0,&cb6);
			SQLExecDirectW(hStmt,L"select echoareas.areaname, arealinks.allowread,arealinks.allowwrite,arealinks.mandatory,arealinks.passive, grouppermissions.allowread,grouppermissions.allowwrite  from echoareas left outer join arealinks on echoareas.areaid=arealinks.areaid and arealinks.linkid=? left outer join grouppermissions on echoareas.areagroup=grouppermissions.areagroup and grouppermissions.linkid=?  order by areaname",SQL_NTS);
			sqlret=SQLFetch(hStmt);
			while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				if ((Passive)&&(cb4!=SQL_NULL_DATA)) p=L'P';
				else p=L'-';

				if ((Mandatory)&&(cb3!=SQL_NULL_DATA))
				{
					m=L'M';
					if((AllowRead)&&(cb1!=SQL_NULL_DATA))
						r=L'R';
					else
						r=L'x';
					if((AllowWrite)&&(cb2!=SQL_NULL_DATA))
						w=L'W';
					else
						w=L'x';
				}
				else
				{
					m=L'-';
					if((AllowRead)&&(cb1!=SQL_NULL_DATA))
						r=L'R';
					else
						if((GroupAllowRead)&&(cb5!=SQL_NULL_DATA))
							r=L'-';
						else
							r=L'x';
					if((AllowWrite)&&(cb2!=SQL_NULL_DATA))
						w=L'W';
					else
						if((GroupAllowRead)&&(cb6!=SQL_NULL_DATA))
							w=L'-';
						else
							w=L'x';
				}
				
				wsprintfW(TmpStr,L" %c%c%c%c  %s\r",r,w,m,p,AreaName);
				AddLine (TmpStr);
				sqlret=SQLFetch(hStmt);
			}
		
											
			SQLCloseCursor(hStmt);
			SQLFreeStmt(hStmt,SQL_UNBIND);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
			AddLine(L"\r\r");

		}
		else
		if (_memicmp(lpTrimmedStr,L"%query",12)==0) //query
		{	
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLBindCol(hStmt,1,SQL_C_WCHAR,AreaName,510,NULL);
			SQLBindCol(hStmt,2,SQL_C_BIT,&AllowRead,0,NULL);
			SQLBindCol(hStmt,3,SQL_C_BIT,&AllowWrite,0,NULL);
			SQLBindCol(hStmt,4,SQL_C_BIT,&Mandatory,0,NULL);
			SQLBindCol(hStmt,5,SQL_C_BIT,&Passive,0,NULL);
			SQLExecDirectW(hStmt,L"select echoareas.areaname,arealinks.allowread,arealinks.allowwrite,arealinks.mandatory,arealinks.passive from echoareas,arealinks where echoareas.areaid=arealinks.areaid and linkid=? order by areaname",SQL_NTS);
			sqlret=SQLFetch(hStmt);
			while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				if (Passive) p=L'P';
				else p=L'-';

				if (Mandatory)
				{
					m=L'M';
					if(AllowRead)
						r=L'R';
					else
						r=L'x';
					if(AllowWrite)
						w=L'W';
					else
						w=L'x';
				}
				else
				{
					m=L'-';
					if(AllowRead)
						r=L'R';
					else
						r=L'-';
			
					if(AllowWrite)
						w=L'W';
					else
						w=L'-';
				}
				
				wsprintfW(TmpStr,L" %c%c%c%c  %s\r",r,w,m,p,AreaName);
				AddLine (TmpStr);
				sqlret=SQLFetch(hStmt);
			}
			AddLine(L"\r\r");
			SQLCloseCursor(hStmt);
			SQLFreeStmt(hStmt,SQL_UNBIND);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
		}
		else 
		if ((_memicmp(lpTrimmedStr,L"%pause",12)==0)||(_memicmp(lpTrimmedStr,L"%passive",16)==0)) //passive
		{
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLExecDirectW(hStmt,L"update arealinks set passive=1 where LinkId=?",SQL_NTS);
			SQLExecDirectW(hStmt,L"update links set passive=1, netmaildirect=0 where LinkId=?",SQL_NTS);
			SQLExecDirectW(hStmt,L"Delete from NewAreaLinks where LinkId=?",SQL_NTS);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
			AddLine(L"Линк и подписка переведены в пассивное состояние\r");


		}
		else
		if ((_memicmp(lpTrimmedStr,L"%active",14)==0)||(_memicmp(lpTrimmedStr,L"%resume",14)==0)) //resume
		{
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLExecDirectW(hStmt,L"update arealinks set passive=0 where LinkId=?",SQL_NTS);
			SQLExecDirectW(hStmt,L"update links set passive=0 where LinkId=?",SQL_NTS);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
			AddLine(L"Линк и подписка возвращены в активное состояние\r");

		}
		else if(_memicmp(lpTrimmedStr,L"%help",10)==0)//help
		{		
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			lpHelpStr = GetBigString(hHeap, hStmt, L"AreafixHelp");
			AddLine(lpHelpStr);
			AddLine(L"\r\r");
			HeapFree(hHeap, 0, lpHelpStr);
			
		}

		else if(_memicmp(lpTrimmedStr,L"%rescan ",16)==0)
		{
			wchar_t * tmp1;
			unsigned int NumOfDays;
			
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			lpTrimmedStr+=8;
			tmp1=wcschr(lpTrimmedStr,L' ');
			if(tmp1!=NULL)
			{
				(wchar_t)(*tmp1)=0;
				++tmp1;
				NumOfDays=_wtoi(tmp1);
				if (NumOfDays==0)
				{
					AddLine(L"Некорректный формат команды RESCAN\r");
				}
				else
				{
					wsprintfW(TmpStr,L"Рескан сообщений за последние %u дней:\r",NumOfDays);
					AddLine(TmpStr);
					
					for(cb1=0;cb1<lstrlenW(lpTrimmedStr);cb1++)
					if (lpTrimmedStr[cb1]==L'*') lpTrimmedStr[cb1]=L'%'; //замена для SQL оператора Like

					SQLPrepareW(hStmt1,L"{call sp_RescanEchoArea(?,?,?,?)}",SQL_NTS);
					SQLBindParameter(hStmt1,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
					SQLBindParameter(hStmt1,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&AreaId,0,NULL);
					SQLBindParameter(hStmt1,3,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&NumOfDays,0,NULL);
					SQLBindParameter(hStmt1,4,SQL_PARAM_INPUT_OUTPUT,SQL_C_LONG,SQL_INTEGER,0,0,&Result,0,NULL);

					SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,510,0,lpTrimmedStr,0,NULL);
					SQLBindCol(hStmt,1,SQL_C_ULONG,&AreaId,0,NULL);
					SQLBindCol(hStmt,2,SQL_C_WCHAR,AreaName,512,NULL);
					SQLExecDirectW(hStmt,L"select areaid,areaname from echoareas where areaname like ? order by areaname",SQL_NTS);
					sqlret=SQLFetch(hStmt);
					while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
					{
						sqlret=SQLExecute(hStmt1);
						wsprintfW(TmpStr,L"%s - отослано %u сообщений\r",AreaName,Result);
						AddLine(TmpStr);
						if (Result!=0) isAdded=TRUE;
						sqlret=SQLFetch(hStmt);
					}
					SQLCloseCursor(hStmt);
					SQLFreeStmt(hStmt,SQL_UNBIND);
					SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
					SQLFreeStmt(hStmt1,SQL_RESET_PARAMS);
					SQLExecDirectW(hStmt, L"update Outbound set Status=2 from EchoMessages where Outbound.MessageId=EchoMessages.MessageID and ISNULL(MsgId,'')=''", SQL_NTS);

				}
				
			}

			else
			{
				AddLine(L"Некорректный формат команды RESCAN\r");
				//некорректная строка
			}
			

		}
		else if(_memicmp(lpTrimmedStr,L"%ping",10)==0)
		{
			SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
			AddLine(TmpStr);
			wsprintfW(TmpStr,L"Ping reply: %02u-%02u-%04u  %02u:%02u:%02u.%u\r",st.wDay,st.wMonth,st.wYear,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
			AddLine(TmpStr);

		}
		else
		{
			wsprintfW(TmpStr,L">Неизвестная команда %s\r",lpTrimmedStr);
			AddLine(TmpStr);

		}
		
		
		
	}
	else
	{
		wsprintfW(TmpStr,L">%s\r",lpTrimmedStr);
		AddLine(TmpStr);

		for(cb1=0;cb1<lstrlenW(lpTrimmedStr);cb1++)
					if (lpTrimmedStr[cb1]==L'*') lpTrimmedStr[cb1]=L'%'; //замена для SQL оператора Like
		if (lpTrimmedStr[0]==L'-')//отписка
		{
			lpTrimmedStr+=1;
		

			SQLPrepareW(hStmt1,L"{call UnsubscribeEchoArea(?,?,?)}",SQL_NTS);
			SQLBindParameter(hStmt1,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLBindParameter(hStmt1,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&AreaId,0,NULL);
			SQLBindParameter(hStmt1,3,SQL_PARAM_INPUT_OUTPUT,SQL_C_LONG,SQL_INTEGER,0,0,&Result,0,NULL);


			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,510,0,lpTrimmedStr,0,NULL);
			SQLBindCol(hStmt,1,SQL_C_ULONG,&AreaId,0,NULL);
			SQLBindCol(hStmt,2,SQL_C_WCHAR,AreaName,512,NULL);
			SQLExecDirectW(hStmt,L"select areaid,areaname from echoareas where areaname like ? order by areaname",SQL_NTS);
			sqlret=SQLFetch(hStmt);
			
			if((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
				{
					
					SQLExecute(hStmt1);
					switch(Result)
					{
						case 0:	
							{ 
								wsprintfW(TmpStr,L"Вы не были подписаны на эху %s\r",AreaName);
								AddLine(TmpStr);
								break;
							}
						case 1:
							{
								wsprintfW(TmpStr,L"Эха %s отписана\r",AreaName);
								AddLine(TmpStr);

								break;
							}
						case 2:
							{
								wsprintfW(TmpStr,L"Отписка от эхи %s невозможна\r",AreaName);
								AddLine(TmpStr);
								break;
							}

					}

					sqlret=SQLFetch(hStmt);
				}
			}
			else
			{
				wsprintfW(TmpStr,L"Эха %s не найдена\r",lpTrimmedStr);
				AddLine(TmpStr);
			}

			SQLCloseCursor(hStmt);
			SQLFreeStmt(hStmt,SQL_UNBIND);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
			SQLFreeStmt(hStmt1,SQL_RESET_PARAMS);


		}
		else //подписка
		{
			if (lpTrimmedStr[0]==L'+') lpTrimmedStr+=1;

			
			SQLPrepareW(hStmt1,L"{call SubscribeEchoArea(?,?,?)}",SQL_NTS);
			SQLBindParameter(hStmt1,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
			SQLBindParameter(hStmt1,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&AreaId,0,NULL);
			SQLBindParameter(hStmt1,3,SQL_PARAM_INPUT_OUTPUT,SQL_C_LONG,SQL_INTEGER,0,0,&Result,0,NULL);
	
			SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,510,0,lpTrimmedStr,0,NULL);
			SQLBindCol(hStmt,1,SQL_C_ULONG,&AreaId,0,NULL);
			SQLBindCol(hStmt,2,SQL_C_WCHAR,AreaName,510,NULL);
			sqlret=SQLExecDirectW(hStmt,L"select areaid,areaname from echoareas where areaname like ? order by areaname",SQL_NTS);
			
			sqlret=SQLFetch(hStmt);
			if((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
				{
					sqlret=SQLExecute(hStmt1);
					
					switch(Result)
					{
						case 0:	
							{ 
								wsprintfW(TmpStr,L"Вы уже подписаны на эху %s\r",AreaName);
								AddLine(TmpStr);
								break;
							}
						case 1:
							{
								wsprintfW(TmpStr,L"Эха %s успешно подписана\r",AreaName);
								AddLine(TmpStr);
								break;
							}
						case 2:
							{
								wsprintfW(TmpStr,L"Нет прав на подписку на эху %s\r",AreaName);
								AddLine(TmpStr);
								break;
							}
					}


					sqlret=SQLFetch(hStmt);
				}
			}
			else
			{
				wsprintfW(TmpStr,L"Эха %s не найдена\r",lpTrimmedStr);
				AddLine(TmpStr);
			}
			SQLCloseCursor(hStmt);
			SQLFreeStmt(hStmt,SQL_UNBIND);
			SQLFreeStmt(hStmt,SQL_RESET_PARAMS);
			SQLFreeStmt(hStmt1,SQL_RESET_PARAMS);

		}
		
	}

	//следующая строка
EndAnalyse:
	if ((lpMsg->MsgText)[i]!=0)
	{	
		j=i+1;
		goto NextStr;
	}

//endMsg
	if(StrCount!=0) PutAreaFixMsg();
	WStringBufferFreeMem(&MsgStrBuf);

	HeapDestroy(hHeap);
	SQLFreeHandle(SQL_HANDLE_STMT,hStmt1);
	SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT,hStmt0);
	SQLDisconnect(hdbc1);
	SQLDisconnect(hdbc);
	SQLDisconnect(hdbc0);
	SQLFreeHandle(SQL_HANDLE_DBC,hdbc1);
	SQLFreeHandle(SQL_HANDLE_DBC,hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC,hdbc0);

	if (isAdded) SetEvent(cfg.hEchomailOutEvent);

	return TRUE;
}