/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

#define PKT_RESULT_FILE_ERROR 0
#define PKT_RESULT_OK 1
#define PKT_RESULT_BAD_DATA 2
#define PKT_RESULT_NOT_TO_MY_AKA 3
#define PKT_RESULT_INSECURE_NETMAIL_OK 4
#define PKT_RESULT_INSECURE_REJECTED 5
#define PKT_RESULT_BAD_PASSWORD 6
#define PKT_RESULT_SQL_FATAL_ERROR -1


SQLHDBC   hdbc;
SQLHSTMT  hstmt;
SQLRETURN sqlret;
HANDLE hHeap;






int TossPkt(wchar_t * FileName, unsigned int FileSize,unsigned int * lpNetmailMsgCount, unsigned int * lpEchomailMsgCount)
{
	HANDLE hPktInFile, hPktInFileMap;
	LPVOID lpInBuf,lpCurrentBufPos;
	lpMsgHeader MsgHdr;
	FTNAddr PktFromAddr,PktToAddr,MsgFromAddr,MsgToAddr;
	char PktPwd[9];

	unsigned int PktFromLinkId,PktToMyAkaId;
	char *DateTimeStr,*toUserName,*fromUserName,*subj,*msgtext, *areaname,*msgid,*reply, *origin;
	StringBuffer PureMsg;
	unsigned int i,j;
	lpEchomailMessage lpEchoMsg;
	lpNetmailMessage lpNetmailMsg;
	char *tmp1,*tmp2;
	unsigned short tmpnode;
	unsigned short seenbynet,pathnet;
	wchar_t LogStr[255];
	BOOL OrigAddrOk;
	int EchoareaCheckResult;
	
	unsigned int PktTossStatus=0;
	
	hPktInFile=CreateFile(FileName,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	if (hPktInFile == INVALID_HANDLE_VALUE) return PKT_RESULT_FILE_ERROR; //файл не открывается

	hPktInFileMap=CreateFileMapping(hPktInFile,NULL,PAGE_READONLY,0,FileSize,NULL);
	if (hPktInFileMap==NULL)
	{
		CloseHandle(hPktInFile);
		return PKT_RESULT_FILE_ERROR;
	}

	lpInBuf=MapViewOfFile(hPktInFileMap,FILE_MAP_READ,0,0,FileSize);
	if (hPktInFileMap==NULL)
	{
		CloseHandle(hPktInFileMap);
		CloseHandle(hPktInFile);
		return PKT_RESULT_FILE_ERROR;
	}
	tmp1=(char *)lpInBuf+FileSize-1;

	if (*tmp1!=0)
	{
		UnmapViewOfFile(lpInBuf);
		CloseHandle(hPktInFileMap);
		CloseHandle(hPktInFile);
		return PKT_RESULT_BAD_DATA;
	}

	if((((lpPktHeader) lpInBuf)->PktVer==2) && (((lpPktHeader) lpInBuf)->cw1H==0) && (((lpPktHeader) lpInBuf)->cw2H==0) && (((lpPktHeader) lpInBuf)->cw1L==1) && (((lpPktHeader) lpInBuf)->cw2L==1))
	{
		lpCurrentBufPos=(unsigned char *)lpInBuf+58;
		
		PktToAddr.zone=((lpPktHeader) lpInBuf)->destZone;
		PktToAddr.net=((lpPktHeader) lpInBuf)->destNet;
		PktToAddr.node=((lpPktHeader) lpInBuf)->destNode;
		PktToAddr.point=((lpPktHeader) lpInBuf)->destPoint;
		PktFromAddr.zone=((lpPktHeader) lpInBuf)->origZone;
		PktFromAddr.net=((lpPktHeader) lpInBuf)->origNet;
		PktFromAddr.node=((lpPktHeader) lpInBuf)->origNode;
		PktFromAddr.point=((lpPktHeader) lpInBuf)->origPoint;
		if ((PktFromAddr.net==65535)&&(PktFromAddr.point!=0)) PktFromAddr.net=((lpPktHeader) lpInBuf)->auxNet;
		
		memset(PktPwd,0,9);
		memcpy(PktPwd,((char *)lpInBuf)+26,8);
		//
		PktFromLinkId=0;
		PktToMyAkaId=0;

		wsprintfW(LogStr,L"%s : %u:%u/%u.%u -> %u:%u/%u.%u",FileName,PktFromAddr.zone,PktFromAddr.net,PktFromAddr.node,PktFromAddr.point,PktToAddr.zone,PktToAddr.net,PktToAddr.node,PktToAddr.point);
		AddLogEntry(LogStr);
		SQLBindParameter(hstmt,1,SQL_PARAM_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&PktTossStatus,0,NULL);
		SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktFromAddr.zone),0,NULL);
		SQLBindParameter(hstmt,3,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktFromAddr.net),0,NULL);
		SQLBindParameter(hstmt,4,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktFromAddr.node),0,NULL);
		SQLBindParameter(hstmt,5,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktFromAddr.point),0,NULL);
		SQLBindParameter(hstmt,6,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktToAddr.zone),0,NULL);
		SQLBindParameter(hstmt,7,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktToAddr.net),0,NULL);
		SQLBindParameter(hstmt,8,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktToAddr.node),0,NULL);
		SQLBindParameter(hstmt,9,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(PktToAddr.point),0,NULL);
		SQLBindParameter(hstmt,10,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,9,0,PktPwd,0,NULL);
		SQLBindParameter(hstmt,11,SQL_PARAM_INPUT_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&PktFromLinkId,0,NULL);
		SQLBindParameter(hstmt,12,SQL_PARAM_INPUT_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&PktToMyAkaId,0,NULL);
		sqlret=SQLExecDirect(hstmt,L"{?=call sp_CheckPktLink(?,?,?,?,?,?,?,?,?,?,?)}",SQL_NTS);
		
		if ((sqlret!=SQL_SUCCESS)&&(sqlret!=SQL_SUCCESS_WITH_INFO))
		{	
			SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
			printf("SQL Error\n");
			PktTossStatus = PKT_RESULT_SQL_FATAL_ERROR;
			goto exitmsg;

		}
		SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
		if ((PktTossStatus == PKT_RESULT_BAD_PASSWORD) || (PktTossStatus == PKT_RESULT_NOT_TO_MY_AKA)) goto exitmsg;
		PktTossStatus = PKT_RESULT_OK;

		while(TRUE)
		{
			MsgHdr=(MsgHeader *)lpCurrentBufPos;
			if (MsgHdr->MsgVer==0)
			{
				if (PktTossStatus == 0) PktTossStatus = PKT_RESULT_OK;
				break;
			}
			if (MsgHdr->MsgVer!=2)
			{
				PktTossStatus = PKT_RESULT_BAD_DATA;
				break;
			}
			
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+14;
			DateTimeStr=(char *)lpCurrentBufPos;
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+strlen(DateTimeStr)+1;
			toUserName=(char *)lpCurrentBufPos;
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+strlen(toUserName)+1;
			fromUserName=(char *)lpCurrentBufPos;
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+strlen(fromUserName)+1;
			subj=(char *)lpCurrentBufPos;
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+strlen(subj)+1;
			msgtext=(char *)lpCurrentBufPos;
			lpCurrentBufPos=(unsigned char *)lpCurrentBufPos+strlen(msgtext)+1;

			InitStringBuffer(hHeap,&PureMsg);
			areaname=NULL;
			msgid=NULL;
			reply=NULL;
			origin=NULL;

			MsgToAddr.point=0;
			MsgToAddr.net=MsgHdr->destNet;
			MsgToAddr.node=MsgHdr->destNode;
			MsgToAddr.zone=((lpPktHeader) lpInBuf)->destZone;
			
			MsgFromAddr.point=0;
			MsgFromAddr.net=MsgHdr->origNet;
			MsgFromAddr.node=MsgHdr->origNode;
			MsgFromAddr.zone=((lpPktHeader) lpInBuf)->origZone;

			i=0;j=0;
			SearchStrEnd:
			if ((msgtext[i]=='\r')||(msgtext[i]==0))
			{
				if ((strncmp(msgtext+j,"AREA:",5)==0)&&(j==0))
				{
					if (PktTossStatus == PKT_RESULT_INSECURE_NETMAIL_OK)
					{
						PktTossStatus = PKT_RESULT_INSECURE_REJECTED;
						goto exitmsg;
					}
					areaname=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,i-j-4);
					memcpy(areaname,msgtext+j+5,i-5-j);
					
					lpEchoMsg=(lpEchomailMessage)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(EchomailMessage));
					OrigAddrOk=FALSE;
					
					InitDwordBuffer(hHeap,&(lpEchoMsg->Path));
					InitDwordBuffer(hHeap,&(lpEchoMsg->SeenBy));
					lpEchoMsg->FromAddr.FullAddr=MsgFromAddr.FullAddr;
					lpEchoMsg->FromLinkID=PktFromLinkId;
					seenbynet=MsgFromAddr.net;
					pathnet=MsgFromAddr.net;
					goto StringComplete;
				}
				else
				{
					if (j==0)
					{
						lpNetmailMsg=(lpNetmailMessage)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(NetmailMessage));
						lpNetmailMsg->pvt=(MsgHdr->attribute&1);
						lpNetmailMsg->fileattach=(MsgHdr->attribute&16);
						lpNetmailMsg->arq=(MsgHdr->attribute&16384);
						lpNetmailMsg->rrq=(MsgHdr->attribute&4096);
						lpNetmailMsg->returnreq=(MsgHdr->attribute&8192);
						lpNetmailMsg->direct=(MsgHdr->attribute&2);
						lpNetmailMsg->cfm=0;
						lpNetmailMsg->recv=1;
						lpNetmailMsg->FromAddr.FullAddr=MsgFromAddr.FullAddr;
						lpNetmailMsg->ToAddr.FullAddr=MsgToAddr.FullAddr;
						lpNetmailMsg->FromLinkID=PktFromLinkId;
					}
					if (strncmp(msgtext+j,"\x01MSGID:",7)==0)
					{
						//копируем msgid
						msgid=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,i-j-7);
						memcpy(msgid,msgtext+8+j,i-8-j);
						goto StringComplete;
					}
																				
					if (strncmp(msgtext+j,"\x01REPLY:",7)==0)
					{
						//reply
						reply=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,i-j-7);
						memcpy(reply,msgtext+8+j,i-8-j);
						goto StringComplete;
					}
					if (areaname==NULL)
					{
						if (strncmp(msgtext+j,"\001FMPT ",6)==0)
						{
							//fmpt
							lpNetmailMsg->FromAddr.point=(unsigned short) strtoul(msgtext+j+6,&tmp1,10);
							goto StringComplete;
						}
						if (strncmp(msgtext+j,"\001TOPT ",6)==0)
						{
							//topt
							lpNetmailMsg->ToAddr.point=(unsigned short) strtoul(msgtext+j+6,&tmp1,10);
							goto StringComplete;
						}
											
						if (strncmp(msgtext+j,"\001INTL ",6)==0)
						{
							//intl
							lpNetmailMsg->ToAddr.zone=(unsigned short) strtoul(msgtext+j+6,&tmp1,10);
							lpNetmailMsg->ToAddr.net=(unsigned short) strtoul(tmp1+1,&tmp2,10);
							lpNetmailMsg->ToAddr.node=(unsigned short) strtoul(tmp2+1,&tmp1,10);
							lpNetmailMsg->FromAddr.zone=(unsigned short) strtoul(tmp1+1,&tmp2,10);
							lpNetmailMsg->FromAddr.net=(unsigned short) strtoul(tmp2+1,&tmp1,10);
							lpNetmailMsg->FromAddr.node=(unsigned short) strtoul(tmp1+1,&tmp2,10);
							goto StringComplete;
						}
										
						if (strncmp(msgtext+j,"\001FLAGS ",7)==0)
						{
							char *tmpflags;
							tmpflags=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,i-j-6);
							memcpy(tmpflags,msgtext+7+j,i-7-j);
							if (strstr(tmpflags,"PVT")!=NULL) lpNetmailMsg->pvt=1;
							if (strstr(tmpflags,"HLD")!=NULL) lpNetmailMsg->direct=1;
							if (strstr(tmpflags,"CRA")!=NULL) lpNetmailMsg->direct=1;
							if (strstr(tmpflags,"DIR")!=NULL) lpNetmailMsg->direct=1;
							if (strstr(tmpflags,"RRQ")!=NULL) lpNetmailMsg->rrq=1;
							if (strstr(tmpflags,"CFM")!=NULL) lpNetmailMsg->cfm=1;
							if (strstr(tmpflags,"FIL")!=NULL) lpNetmailMsg->fileattach=1;
							HeapFree(hHeap,0,tmpflags);
							goto StringComplete;
						}
					}
					else
					{
						if (strncmp(msgtext+j," * Origin:",10)==0)
						{
							//parse origin
							char *beginaddrstr;
							unsigned short q;
							origin=(char *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,i-j-9);
							memcpy(origin,msgtext+10+j,i-10-j);
							beginaddrstr=strrchr(origin,'(');
							if (beginaddrstr!=NULL)
							{
								tmp1=strchr(beginaddrstr,':');
								if (tmp1!=0)
								{
									tmp2=tmp1;
									while(isdigit(*(tmp2-1)))
									{
										--tmp2;
									}
									lpEchoMsg->FromAddr.zone=(unsigned short)atoi(tmp2);
									q=(unsigned short)strtoul(tmp1+1,&tmp2,10);
									if (*tmp2!='/') goto CheckOriginComplete;
									lpEchoMsg->FromAddr.net=q;
									q=(unsigned short)strtoul(tmp1+1,&tmp2,10);
									lpEchoMsg->FromAddr.node=(unsigned short)strtoul(tmp2+1,&tmp1,10);
									if (*tmp1=='.')
										lpEchoMsg->FromAddr.point=(unsigned short)strtoul(tmp1+1,&tmp2,10);
									else
										lpEchoMsg->FromAddr.point=0;
									OrigAddrOk=TRUE;
								}
							}
							CheckOriginComplete:
							if (!OrigAddrOk)
							{
								//get fromaddr from msgid
								if (msgid!=NULL)
								{
									beginaddrstr=msgid;
									tmp1=strchr(beginaddrstr,':');
									if (tmp1!=0)
									{
										tmp2=tmp1;
										while(isdigit(*(tmp2-1)))
										{
											--tmp2;
										}
										lpEchoMsg->FromAddr.zone=(unsigned short)atoi(tmp2);
										q=(unsigned short)strtoul(tmp1+1,&tmp2,10);
										if (*tmp2!='/') goto StringComplete;
										lpEchoMsg->FromAddr.net=q;
										q=(unsigned short)strtoul(tmp1+1,&tmp2,10);
										lpEchoMsg->FromAddr.node=(unsigned short)strtoul(tmp2+1,&tmp1,10);
										if (*tmp1=='.')
											lpEchoMsg->FromAddr.point=(unsigned short)strtoul(tmp1+1,&tmp2,10);
										else
											lpEchoMsg->FromAddr.point=0;
										OrigAddrOk = TRUE;
									}
								}
							}
							HeapFree(hHeap,0,origin);
							goto AddPureString;
						}

							
						if (strncmp(msgtext+j,"\x01PATH: ",7)==0)
						{
							
							//parse path
							
							tmp1=msgtext+j+7;
							NextPathElement:
							tmpnode=(unsigned int)strtoul(tmp1,&tmp2,10);
							if (*tmp2=='/')
							{
								pathnet=tmpnode;
							}
							else
							{
								AddToDwordBuffer(&(lpEchoMsg->Path),pathnet*65536+tmpnode);
								if ((*tmp2=='\r')||(*tmp2==0))
								{	
									
									goto StringComplete;
								}
							}
							tmp1=tmp2+1;
							goto NextPathElement;
						}

						if (strncmp(msgtext+j,"SEEN-BY: ",9)==0)
						{
							//parse seen-by
							tmp1=msgtext+j+9;
							NextSeenByElement:
							tmpnode=(unsigned int)strtoul(tmp1,&tmp2,10);
							if (*tmp2=='/')
							{
								seenbynet=tmpnode;
							}
							else
							{
								AddToDwordBuffer(&(lpEchoMsg->SeenBy),seenbynet*65536+tmpnode);
								if ((*tmp2=='\r')||(*tmp2==0))
								{
									goto StringComplete;
								}
							}
							tmp1=tmp2+1;
							goto NextSeenByElement;
							

						}
					}
				}
				AddPureString:
				AddStr1ToBuffer(&PureMsg,msgtext+j,i+1-j);
				
				



		StringComplete:
				if (i>=strlen(msgtext)) goto SearchStrComplete;
				i++;
				j=i;
				goto SearchStrEnd;
			}
			else 
			{
				i++;
				goto SearchStrEnd;
			}

		SearchStrComplete:;
					


			if (areaname==NULL)
			{ //netmail
				StrToSqlDateTime(&(lpNetmailMsg->CreateTime),DateTimeStr);
				lpNetmailMsg->FromName = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(fromUserName) + 1) * 2);
				lpNetmailMsg->ToName = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(toUserName) + 1) * 2);
				lpNetmailMsg->Subject = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(subj) + 1) * 2);
				lpNetmailMsg->MsgText = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (PureMsg.CurrentSize + 1) * 2);
				MultiByteToWideChar(CP_OEMCP,0,fromUserName,(int)strlen(fromUserName),lpNetmailMsg->FromName,(int)(strlen(fromUserName)+1));
				MultiByteToWideChar(CP_OEMCP, 0, toUserName, (int)strlen(toUserName), lpNetmailMsg->ToName, (int)(strlen(toUserName) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, subj, (int)strlen(subj), lpNetmailMsg->Subject, (int)(strlen(subj) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, PureMsg.lpBuffer, (int)PureMsg.CurrentSize, lpNetmailMsg->MsgText, (int)(PureMsg.CurrentSize + 1));
				StringBufferFreeMem(&PureMsg);
				

				if (msgid!=NULL)
				{
					lpNetmailMsg->MsgId = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(msgid) + 1) * 2);
					MultiByteToWideChar(CP_OEMCP, 0, msgid, (int)strlen(msgid), lpNetmailMsg->MsgId, (int)(strlen(msgid) + 1));
					HeapFree(hHeap,0,msgid);
				}
				
				
				if (reply!=NULL)
				{
					lpNetmailMsg->ReplyTo = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(reply) + 1) * 2);
					MultiByteToWideChar(CP_OEMCP, 0, reply, (int)strlen(reply), lpNetmailMsg->ReplyTo, (int)(strlen(reply) + 1));
					HeapFree(hHeap,0,reply);
				}
				
				{
					//
					BOOL isNetmailAccepted=AddNetmailMessage(hHeap, hstmt, lpNetmailMsg);
					NetmailFreeMem(hHeap, lpNetmailMsg);
					if (!isNetmailAccepted) PktTossStatus = PKT_RESULT_INSECURE_REJECTED;
					//
				}
				++(*lpNetmailMsgCount);
				

			}
			else
			{//echomail
				StrToSqlDateTime(&(lpEchoMsg->CreateTime),DateTimeStr);
				lpEchoMsg->AreaName=(wchar_t *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,(strlen(areaname)+1)*2);
				lpEchoMsg->FromName = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(fromUserName) + 1) * 2);
				lpEchoMsg->ToName = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(toUserName) + 1) * 2);
				lpEchoMsg->Subject = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(subj) + 1) * 2);
				lpEchoMsg->MsgText = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (PureMsg.CurrentSize + 1) * 2);
				MultiByteToWideChar(CP_OEMCP, 0, areaname, (int)strlen(areaname), lpEchoMsg->AreaName, (int)(strlen(areaname) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, fromUserName, (int)strlen(fromUserName), lpEchoMsg->FromName, (int)(strlen(fromUserName) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, toUserName, (int)strlen(toUserName), lpEchoMsg->ToName, (int)(strlen(toUserName) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, subj, (int)strlen(subj), lpEchoMsg->Subject, (int)(strlen(subj) + 1));
				MultiByteToWideChar(CP_OEMCP, 0, PureMsg.lpBuffer, (int)PureMsg.CurrentSize, lpEchoMsg->MsgText, (int)(PureMsg.CurrentSize + 1));
				StringBufferFreeMem(&PureMsg);
				


											
				if (msgid!=NULL)
				{
					lpEchoMsg->MsgId = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(msgid) + 1) * 2);
					MultiByteToWideChar(CP_OEMCP, 0, msgid, (int)strlen(msgid), lpEchoMsg->MsgId, (int)(strlen(msgid) + 1));
					HeapFree(hHeap,0,msgid);

				}
				
						
				if (reply!=NULL)
				{
					lpEchoMsg->ReplyTo = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (strlen(reply) + 1) * 2);
					MultiByteToWideChar(CP_OEMCP, 0, reply, (int)strlen(reply), lpEchoMsg->ReplyTo, (int)(strlen(reply) + 1));
					HeapFree(hHeap,0,reply);

				}
				
				EchoareaCheckResult = CheckEchoArea(hHeap, hstmt, lpEchoMsg->FromLinkID, lpEchoMsg->AreaName, &(lpEchoMsg->EchoAreaID));
				if (EchoareaCheckResult == ECHOAREA_CHECK_SQLERROR) return -1;
				if (EchoareaCheckResult != ECHOAREA_CHECK_WRITE_NOT_ALLOWED)
				{

					if (CheckDupes(hstmt, lpEchoMsg->EchoAreaID, lpEchoMsg->MsgId))
						AddDupeReport(hstmt, lpEchoMsg);
					else
						AddEchomailMessage(hHeap, hstmt, lpEchoMsg);
				}
				
				EchomailFreeMem(hHeap, lpEchoMsg);
				
				++(*lpEchomailMsgCount);
				
			}
		}
	}
	else PktTossStatus = PKT_RESULT_BAD_DATA;

exitmsg:
	UnmapViewOfFile(lpInBuf);
	CloseHandle(hPktInFileMap);
	CloseHandle(hPktInFile);

	return PktTossStatus;
}


DWORD WINAPI PktInTosserThread(LPVOID param)
{

	int WaitTime;
	int result; 

	HANDLE hEvent[2];
	
	WIN32_FIND_DATA FindData;

	HANDLE hPktSearch;

	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	BOOL GoFile;
	BOOL ProcOk;
	unsigned int zipexitcode;
	unsigned int NetmailMsgCount, EchomailMsgCount;
	BOOL isNewEchomail;

	int PktTossStatus;
	wchar_t TmpStr[MAX_PATH];
	wchar_t TmpStr1[MAX_PATH];
	wchar_t TmpStr2[MAX_PATH];
	wchar_t LogStr[255];

	wchar_t *FileExt;




	_InterlockedIncrement(&(cfg.ThreadCount));

	WaitTime=INFINITE;

	StartupInfo.cb=sizeof(STARTUPINFO);
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

	hEvent[0]=cfg.hExitEvent;
	hEvent[1]=cfg.hPktInEvent;
	AddLogEntry(L".PKT importing thread started");
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
		hHeap=HeapCreate(HEAP_NO_SERIALIZE,8192,0);
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

StartTossCycle:
			isNewEchomail = FALSE;
			wsprintfW(TmpStr2, L"%s\\????????.???", cfg.InboundDir);
			hPktSearch=FindFirstFileW(TmpStr2,&FindData);
			if (hPktSearch==INVALID_HANDLE_VALUE) GoFile=FALSE;
			else GoFile=TRUE;

			while(GoFile)
			{
				if ((wcslen(FindData.cFileName)==12)&&((FindData.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY))==0))
				{
					wsprintfW(TmpStr, L"%s\\%s", cfg.InboundDir, FindData.cFileName);
					
					FileExt=FindData.cFileName+9;
					if (lstrcmpiW(FileExt,L"pkt")==0)
					{
						if (FindData.nFileSizeLow<sizeof(PktHeader)) 
						{
							DeleteFileW(TmpStr);
						}
						else
						{
							NetmailMsgCount = 0; EchomailMsgCount = 0;
							PktTossStatus=TossPkt(TmpStr,FindData.nFileSizeLow,&NetmailMsgCount,&EchomailMsgCount);
							if (PktTossStatus == PKT_RESULT_SQL_FATAL_ERROR) goto threadexit;
							if (EchomailMsgCount) isNewEchomail = TRUE;
							if (NetmailMsgCount) SetEvent(cfg.hNetmailOutEvent);
							wsprintfW(LogStr, L"%s -> status: %u, Netmail MSGs: %u, Echomail MSGs: %u", FindData.cFileName, PktTossStatus,NetmailMsgCount,EchomailMsgCount);
							AddLogEntry(LogStr);
							if ((PktTossStatus == PKT_RESULT_OK) || PktTossStatus == PKT_RESULT_INSECURE_NETMAIL_OK)//
							{
								DeleteFileW(TmpStr);

							}
							else 
								if (PktTossStatus == PKT_RESULT_BAD_PASSWORD)
							{
								wsprintfW(TmpStr1, L"%s\\%s", cfg.InsecureInPktDir, FindData.cFileName);
								MoveFileExW(TmpStr, TmpStr1, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
							}
							else
								if (PktTossStatus != PKT_RESULT_FILE_ERROR)
							{
								wsprintfW(TmpStr1, L"%s\\%s", cfg.BadInPktDir, FindData.cFileName);
								MoveFileExW(TmpStr,TmpStr1,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);
							}
						}

					}
					else
					if (((_wcsnicmp( FileExt,L"mo", 2)==0)||(_wcsnicmp( FileExt,L"tu", 2)==0)||(_wcsnicmp( FileExt,L"we", 2)==0)||(_wcsnicmp( FileExt,L"th", 2)==0)||(_wcsnicmp( FileExt,L"fr", 2)==0)||(_wcsnicmp( FileExt,L"sa", 2)==0)||(_wcsnicmp( FileExt,L"su", 2)==0))&&(FindData.nFileSizeLow>64))
					{
						wsprintfW(TmpStr1, L"%s %s", cfg.UnzipCommand, FindData.cFileName);
						ProcOk=CreateProcessW(NULL,TmpStr1,NULL,NULL,FALSE,0,NULL,cfg.InboundDir,&StartupInfo,&ProcInfo);
						if (ProcOk)	
						{
							result=WaitForSingleObject(ProcInfo.hProcess,100000);
							if (result==WAIT_TIMEOUT)
							{
								TerminateProcess(ProcInfo.hProcess,1000);
								zipexitcode=1000;

							}
							else GetExitCodeProcess(ProcInfo.hProcess,(LPDWORD)&zipexitcode);
							
							CloseHandle(ProcInfo.hProcess);
							CloseHandle(ProcInfo.hThread);

							if (zipexitcode==0)
							{
								DeleteFileW(TmpStr);
							}
							else
							{
								wsprintfW(TmpStr1, L"%s\\%s", cfg.BadInPktDir, FindData.cFileName);
								MoveFileExW(TmpStr,TmpStr1,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING);
							}
							FindClose(hPktSearch);
							goto StartTossCycle;
						}
						

					}

				}
				GoFile=FindNextFileW(hPktSearch,&FindData);
			}
			FindClose(hPktSearch);
			
			if (isNewEchomail) SetEvent(cfg.hEchomailTossEvent);
			goto loop;
		}

	}
//

threadexit:

	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}