/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"


NETMAILROBOTPROC NetmailRobot[127];

unsigned int AcceptInsecureNetmail;


lpNetmailMessage CreateNewNetmailMsg(HANDLE hHeap, FTNAddr Addr, HSTMT hstmt)
{
	lpNetmailMessage lpMsg;
	lpMsg=(NetmailMessage *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(NetmailMessage));
	lpMsg->FromAddr.FullAddr=Addr.FullAddr;
	lpMsg->MsgId = (wchar_t *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 70);
	if (Addr.point==0)
		wsprintfW(lpMsg->MsgId,L"%u:%u/%u %08x",Addr.zone,Addr.net,Addr.node,GetMsgIdTime(hstmt));
	else
		wsprintfW(lpMsg->MsgId,L"%u:%u/%u.%u %08x",Addr.zone,Addr.net,Addr.node,Addr.point,GetMsgIdTime(hstmt));

	
	lpMsg->CreateTime=GetSQLTime();

	return lpMsg;

}


void PostReplyMsg(HANDLE hHeap, lpNetmailMessage lpMsg, HSTMT hstmt,wchar_t *FromName, wchar_t *Subj, wchar_t *MsgText,unsigned char isDirect, unsigned char isKillSent)
{
	lpNetmailMessage lpMsgNew;
	lpMsgNew=CreateNewNetmailMsg(hHeap,lpMsg->ToAddr,hstmt);
	lpMsgNew->ToAddr.FullAddr=lpMsg->FromAddr.FullAddr;
	lpMsgNew->killsent=isKillSent;
	lpMsgNew->direct=isDirect;
	lpMsgNew->ReplyTo=(wchar_t *)HeapAlloc(hHeap,0,wcslen(lpMsg->MsgId)*2+2);
	lpMsgNew->FromName=(wchar_t *)HeapAlloc(hHeap,0,wcslen(FromName)*2+2);
	lpMsgNew->ToName=(wchar_t *)HeapAlloc(hHeap,0,wcslen(lpMsg->FromName)*2+2);
	lpMsgNew->Subject=(wchar_t *)HeapAlloc(hHeap,0,wcslen(Subj)*2+2);
	lpMsgNew->MsgText=(wchar_t *)HeapAlloc(hHeap,0,wcslen(MsgText)*2+2);
	lstrcpyW(lpMsgNew->ReplyTo,lpMsg->MsgId);
	lstrcpyW(lpMsgNew->FromName,FromName);
	lstrcpyW(lpMsgNew->ToName,lpMsg->FromName);
	lstrcpyW(lpMsgNew->Subject,Subj);
	lstrcpyW(lpMsgNew->MsgText,MsgText);
	//
	AddNetmailMessage(hHeap, hstmt, lpMsgNew);
	NetmailFreeMem(hHeap, lpMsgNew);
}




void NetmailFreeMem(HANDLE hHeap, lpNetmailMessage lpNetmailMsg)
{
	
	if (lpNetmailMsg->FromName != NULL) HeapFree(hHeap, 0, lpNetmailMsg->FromName);
	if (lpNetmailMsg->ToName != NULL) HeapFree(hHeap, 0, lpNetmailMsg->ToName);
	if (lpNetmailMsg->Subject != NULL) HeapFree(hHeap, 0, lpNetmailMsg->Subject);
	if (lpNetmailMsg->MsgText != NULL) HeapFree(hHeap, 0, lpNetmailMsg->MsgText);
	if (lpNetmailMsg->MsgId != NULL) HeapFree(hHeap, 0, lpNetmailMsg->MsgId);
	if (lpNetmailMsg->ReplyTo != NULL) HeapFree(hHeap, 0, lpNetmailMsg->ReplyTo);
	HeapFree(hHeap, 0, lpNetmailMsg);
}


BOOL AddNetmailMessage(HANDLE hHeap, HSTMT hstmt, lpNetmailMessage lpMsg)
{
SQLRETURN sqlret;
SQLPOINTER pToken;

SQLLEN cb0,cb1,cb2;

unsigned char recv,killsent,pvt,fileattach,arq,rrq,returnreq,direct,cfm,visible;
BOOL isToMyNode;

unsigned int FromLinkId;

unsigned int MessageId;
unsigned int ActionCode;
unsigned int MsgStatus;
wchar_t ViaStr[255];
wchar_t * buf;
size_t bufsize;
SYSTEMTIME st;




recv=lpMsg->recv;
killsent=lpMsg->killsent;
pvt=lpMsg->pvt;
fileattach=lpMsg->fileattach;
arq=lpMsg->arq;
rrq=lpMsg->rrq;
returnreq=lpMsg->returnreq;
direct=lpMsg->direct;

cfm=lpMsg->cfm;



if (recv)
{	
	MsgStatus=0;
	FromLinkId=0;
	ActionCode=0;

	SQLBindParameter(hstmt,1,SQL_PARAM_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&MsgStatus,0,NULL);
	SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.zone),0,NULL);
	SQLBindParameter(hstmt,3,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.net),0,NULL);
	SQLBindParameter(hstmt,4,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.node),0,NULL);
	SQLBindParameter(hstmt,5,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.point),0,NULL);
	SQLBindParameter(hstmt,6,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.zone),0,NULL);
	SQLBindParameter(hstmt,7,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.net),0,NULL);
	SQLBindParameter(hstmt,8,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.node),0,NULL);
	SQLBindParameter(hstmt,9,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.point),0,NULL);
	SQLBindParameter(hstmt,10,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,36,0,lpMsg->ToName,0,NULL);
	SQLBindParameter(hstmt,11,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,74,0,lpMsg->Subject,0,NULL);
	SQLBindParameter(hstmt,12,SQL_PARAM_INPUT_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&FromLinkId,0,NULL);
	SQLBindParameter(hstmt,13,SQL_PARAM_INPUT_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&ActionCode,0,NULL);
	sqlret=SQLExecDirectW(hstmt,L"{?=call sp_CheckNetmailMsg(?,?,?,?,?,?,?,?,?,?,?,?)}",SQL_NTS);
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

	switch(MsgStatus)
	{
		case 0:
			isToMyNode = FALSE;
			visible = 0;
			break;
		case 1:
			isToMyNode = TRUE;
			visible = 1;
			break;
		case 2:
			isToMyNode = TRUE;
			visible = 0;
			break;
		case 3://письмо к роботу не от линка или с неправильным паролем
		case 4://обработка писем к роботам
			{
				
				isToMyNode = TRUE; visible = 1;
				if (NetmailRobot[ActionCode]!=NULL)
				{ 
					if(MsgStatus==3)
					{
						if((NetmailRobot[ActionCode])(lpMsg,FromLinkId,FALSE)) return TRUE;
					}
					else
					{
						if ((NetmailRobot[ActionCode])(lpMsg, FromLinkId, TRUE)) return TRUE;
					}
				}
				break;
			}

		
	}

	if (lpMsg->FromLinkID == 0)
	{
		if (AcceptInsecureNetmail == INSECURE_NETMAIL_REJECT) return FALSE;
		if ((AcceptInsecureNetmail == INSECURE_NETMAIL_ALLOW_TO_ME) && (!isToMyNode)) return FALSE;
	}
}
else if (killsent==0) visible=1;
lpMsg->CreateTime.fraction=0;

GetLocalTime(&st);

wsprintfW(ViaStr, L"\01Via %s %u:%u/%u, %02u-%02u-%04u %02u:%02u:%02u\r", cfg.SoftwareName, cfg.MyAddr.zone, cfg.MyAddr.net, cfg.MyAddr.node, st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

buf= HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (wcslen(lpMsg->MsgText) + wcslen(ViaStr) + 1) * 2);
wcscpy(buf, lpMsg->MsgText);
wcscat(buf, ViaStr);
bufsize = wcslen(buf);

MessageId=0;
SQLBindParameter(hstmt,1,SQL_PARAM_OUTPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&MessageId,0,NULL);
SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.zone),0,NULL);
SQLBindParameter(hstmt,3,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.net),0,NULL);
SQLBindParameter(hstmt,4,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.node),0,NULL);
SQLBindParameter(hstmt,5,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->FromAddr.point),0,NULL);
SQLBindParameter(hstmt,6,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.zone),0,NULL);
SQLBindParameter(hstmt,7,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.net),0,NULL);
SQLBindParameter(hstmt,8,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.node),0,NULL);
SQLBindParameter(hstmt,9,SQL_PARAM_INPUT,SQL_C_USHORT,SQL_SMALLINT,0,0,&(lpMsg->ToAddr.point),0,NULL);
SQLBindParameter(hstmt,10,SQL_PARAM_INPUT,SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,19,0,&(lpMsg->CreateTime),0,NULL);

SQLBindParameter(hstmt,11,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,36,0,lpMsg->FromName,0,NULL);
SQLBindParameter(hstmt,12,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,36,0,lpMsg->ToName,0,NULL);
SQLBindParameter(hstmt,13,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,74,0,lpMsg->Subject,0,NULL);
	
if (lpMsg->MsgId==NULL)
{
	SQLBindParameter(hstmt,14,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,127,0,NULL,0,&cb0);
	cb0=SQL_NULL_DATA;
}
else 	SQLBindParameter(hstmt,14,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,127,0,lpMsg->MsgId,0,NULL);

if (lpMsg->ReplyTo==NULL)
{
	SQLBindParameter(hstmt,15,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,127,0,NULL,0,&cb1);
	cb1=SQL_NULL_DATA;
}
else SQLBindParameter(hstmt,15,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WVARCHAR,127,0,lpMsg->ReplyTo,0,NULL);

SQLBindParameter(hstmt,16,SQL_PARAM_INPUT,SQL_C_WCHAR,SQL_WLONGVARCHAR,bufsize,0,(SQLPOINTER) 25,0,&cb2);
cb2=SQL_LEN_DATA_AT_EXEC((int)bufsize*2);

SQLBindParameter(hstmt,17,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&killsent,0,NULL);
SQLBindParameter(hstmt,18,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&pvt,0,NULL);
SQLBindParameter(hstmt,19,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&fileattach,0,NULL);
SQLBindParameter(hstmt,20,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&arq,0,NULL);
SQLBindParameter(hstmt,21,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&rrq,0,NULL);
SQLBindParameter(hstmt,22,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&returnreq,0,NULL);
SQLBindParameter(hstmt,23,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&direct,0,NULL);
SQLBindParameter(hstmt,24,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&cfm,0,NULL);
SQLBindParameter(hstmt,25,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&visible,0,NULL);
SQLBindParameter(hstmt,26,SQL_PARAM_INPUT,SQL_C_BIT,SQL_BIT,0,0,&recv,0,NULL);
sqlret=SQLExecDirectW(hstmt,L"{?=call sp_Add_Netmail_Message(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}",SQL_NTS);
if (sqlret==SQL_NEED_DATA)
{
	sqlret=SQLParamData(hstmt,&pToken);
	if (sqlret==SQL_NEED_DATA) SQLPutData(hstmt,buf,bufsize*2);
	sqlret=SQLParamData(hstmt,&pToken);
}


SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

HeapFree(hHeap, 0, buf);

return TRUE;
}

