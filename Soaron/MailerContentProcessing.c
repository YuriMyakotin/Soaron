/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Mailer.h"

typedef struct tagEchomailCheckItem
{
	unsigned int AreaID;
	unsigned int MessageID;
	wchar_t MsgId[127];
	struct tagEchomailCheckItem * lpNext;
} EchomailCheckItem, *lpEchomailCheckItem;

#define REPLY_CODE_ACCEPTED 0
#define REPLY_CODE_DUPE 1
#define REPLY_CODE_AREA_NOT_ALLOWED 2


typedef struct tagEchomailCheckReplyItem
{
	unsigned int MessageID;
	unsigned char ReplyCode;
} EchomailCheckReplyItem, *lpEchomailCheckReplyItem;



BOOL MailerSendNetmail(HSTMT hstmt, lpMailerSessionInfo SI)
{
	lpPackedDataHeader lpMessage;
	lpNetmailMessage lpTmp;
	NetmailOutQueue NOQ;
	MixedBuffer MsgBuff;
	SQLRETURN sqlret;
	unsigned int Size,MessageID;
	NOQ.First = NULL;
	NOQ.Last = NULL;
	

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	sqlret = SQLExecDirectW(hstmt, L"select Netmail.MessageID,FromZone,FromNet,FromNode,FromPoint,ToZone,ToNet,ToNode,ToPoint,CreateTime,FromName,ToName,Subject,MsgId,ReplyTo,MsgText,KillSent,Pvt,FileAttach,Arq,RRq,ReturnReq,Direct,Cfm,recv from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID and NetmailOutbound.ToLinkID=? and Locked=0 order by Netmail.MessageID", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		GetNetmailMessages(hstmt, SI->hHeap, &NOQ);
	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);


	if (NOQ.First == NULL) return FALSE;
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &MessageID, 0, NULL);
	SQLPrepareW(hstmt, L"Update Netmail set Locked=? where MessageID=?", SQL_NTS);

	while (NOQ.First != NULL)

	{
		lpTmp = NOQ.First;
		MessageID = lpTmp->MessageID;
		SQLExecute(hstmt);
		InitMixedBuffer(SI->hHeap, &MsgBuff);
		AddToMixedBuffer(&MsgBuff,&(lpTmp->MessageID), sizeof(unsigned int));
		AddToMixedBuffer(&MsgBuff,&(lpTmp->FromAddr), sizeof(FTNAddr));
		AddToMixedBuffer(&MsgBuff, &(lpTmp->ToAddr), sizeof(FTNAddr));
		AddToMixedBuffer(&MsgBuff, &(lpTmp->flags), sizeof(unsigned int));
		AddToMixedBuffer(&MsgBuff, &(lpTmp->CreateTime), sizeof(SQL_TIMESTAMP_STRUCT));
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->FromName);
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->ToName);
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->Subject);
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->MsgId);
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->ReplyTo);
		AddWStrToMixedBuffer(&MsgBuff, lpTmp->MsgText);
		
		//
		Size = PackData(SI->hHeap, MsgBuff.lpBuffer, MsgBuff.CurrentSize, (char **)&lpMessage, sizeof(PackedDataHeader));
		SetMailerMsgHeader(lpMessage, CMD_NETMAIL_MESSAGE, Size + sizeof(PackedDataHeader));
		lpMessage->OriginalSize = MsgBuff.CurrentSize;
		AddToSendQueue(SI,  QUEUE_ITEM_NETMAIL, (unsigned char *)lpMessage);
		//
		MixedBufferFreeMem(&MsgBuff);
		NOQ.First = NOQ.First->NextMsg;
		HeapFree(SI->hHeap, 0, lpTmp->MsgText);
		if (lpTmp->ReplyTo != NULL) HeapFree(SI->hHeap, 0, lpTmp->ReplyTo);
		if (lpTmp->MsgId != NULL) HeapFree(SI->hHeap, 0, lpTmp->MsgId);
		HeapFree(SI->hHeap, 0, lpTmp->Subject);
		HeapFree(SI->hHeap, 0, lpTmp->ToName);
		HeapFree(SI->hHeap, 0, lpTmp->FromName);
		HeapFree(SI->hHeap, 0, lpTmp);
		

	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	return TRUE;
}

void MailerReceiveNetmail(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int Size,UnpackedBuffSize,MessageID;
	NetmailMessage NetmailMsg;
	char * UnpackedBuff;
	char * CurrentBuffPos;
	wchar_t * lpStr;
	size_t Len;

	Size = GetMailerMsgSize(SI->CurrentRecvBuf);
	UnpackedBuffSize = ((lpPackedDataHeader)(SI->CurrentRecvBuf))->OriginalSize;
	
	

	if (!UnpackData(SI->hHeap, (SI->CurrentRecvBuf) + sizeof(PackedDataHeader), Size - sizeof(PackedDataHeader), &UnpackedBuff, UnpackedBuffSize))
	{
		SI->SessionStatus = SESS_STAT_ERROR;
		MailerSendCommand(SI, CMD_INFO_UNPACKING_ERROR);
		AddLogEntry(L"Error: incorrect packed data received");
		return;
	}
	CurrentBuffPos = UnpackedBuff;
	
	NetmailMsg.FromLinkID = SI->LinkID;

	memcpy(&MessageID, CurrentBuffPos, sizeof(unsigned int));
	memcpy(&NetmailMsg.MessageID, CurrentBuffPos, sizeof(unsigned int));
	CurrentBuffPos += sizeof(unsigned int);
	
	memcpy(&NetmailMsg.FromAddr, CurrentBuffPos, sizeof(FTNAddr));
	CurrentBuffPos += sizeof(FTNAddr);

	memcpy(&NetmailMsg.ToAddr, CurrentBuffPos, sizeof(FTNAddr));
	CurrentBuffPos += sizeof(FTNAddr);

	memcpy(&NetmailMsg.flags, CurrentBuffPos, sizeof(unsigned int));
	CurrentBuffPos += sizeof(unsigned int);

	memcpy(&NetmailMsg.CreateTime, CurrentBuffPos, sizeof(SQL_TIMESTAMP_STRUCT));
	CurrentBuffPos += sizeof(SQL_TIMESTAMP_STRUCT);

	lpStr = (wchar_t *)CurrentBuffPos;
	NetmailMsg.FromName = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);
	
	NetmailMsg.ToName = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);

	NetmailMsg.Subject = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);

	Len = wcslen(lpStr);
	if (Len == 0)
		NetmailMsg.MsgId = NULL;
	else NetmailMsg.MsgId = lpStr;
	lpStr += (Len + 1);

	Len = wcslen(lpStr);
	if (Len == 0)
		NetmailMsg.ReplyTo = NULL;
	else NetmailMsg.ReplyTo = lpStr;
	lpStr += (Len + 1);

	NetmailMsg.MsgText = lpStr;
	if (AddNetmailMessage(SI->hHeap, hstmt, &NetmailMsg))
	{
		MailerSendConfirmation(SI, CMD_NETMAIL_MSG_RECEIVED, MessageID);
		++(SI->NetmailRcvd);
	}
	else 
		MailerSendConfirmation(SI, CMD_NETMAIL_MSG_REJECTED, MessageID);
	HeapFree(SI->hHeap, 0, UnpackedBuff);
	return;
}

void MailerReceiveNetmailConfirmation(HSTMT hstmt, lpMailerSessionInfo SI)
{
	
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(((lpConfirmationMsg)(SI->CurrentRecvBuf))->MessageID), 0, NULL);
	SQLExecDirectW(hstmt, L"{call sp_NetmailMessageSent(?)}", SQL_NTS);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	++(SI->NetmailSent);
}


BOOL MailerSendEchomailCheck(HSTMT hstmt, lpMailerSessionInfo SI)
{
	lpEchomailCheckItem lpFirst, lpLast, lpCurrent;
	SQLRETURN sqlret;
	wchar_t AreaName[80];
	MixedBuffer UnpackedBuff;
	lpPackedDataHeader lpMessage;
	unsigned int Size;
	unsigned int PrevAreaID = 0;
	unsigned int NullID = 0;
	
	

	lpFirst = NULL; 
	lpLast = NULL;
	
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	sqlret = SQLExecDirectW(hstmt, L"select EchoAreas.AreaID,EchoMessages.MessageID,EchoMessages.MsgId from Echoareas,EchoMessages,Outbound where Outbound.ToLink=? and Outbound.Status=0 and Outbound.MessageId=EchoMessages.MessageID and EchoAreas.AreaId=EchoMessages.AreaId order by EchoAreas.AreaName,EchoMessages.MessageID", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			lpCurrent = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, sizeof(EchomailCheckItem));
			SQLGetData(hstmt, 1, SQL_C_ULONG, &(lpCurrent->AreaID), 0, NULL);
			SQLGetData(hstmt, 2, SQL_C_ULONG, &(lpCurrent->MessageID), 0, NULL);
			SQLGetData(hstmt, 3, SQL_C_WCHAR, &(lpCurrent->MsgId), 254, NULL);
			
			if (lpFirst == NULL) lpFirst = lpCurrent;
			else lpLast->lpNext = lpCurrent;

			lpLast = lpCurrent;
			sqlret = SQLFetch(hstmt);
		}
		SQLCloseCursor(hstmt);
	}
	
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	if (lpFirst == NULL) return FALSE;
	InitMixedBuffer(SI->hHeap, &UnpackedBuff);

	while (lpFirst != NULL)
	{
		
		lpCurrent = lpFirst;
		if (lpCurrent->AreaID != PrevAreaID)
		{
			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpCurrent->AreaID), 0, NULL);
			SQLExecDirectW(hstmt, L"Select AreaName from EchoAreas where AreaId=?", SQL_NTS);
			SQLFetch(hstmt);
			SQLGetData(hstmt, 1, SQL_C_WCHAR, AreaName, 160, NULL);
			SQLCloseCursor(hstmt);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
			if (PrevAreaID != 0) //first string
			{
				AddToMixedBuffer(&UnpackedBuff, &NullID, sizeof(unsigned int));
			}
			AddWStrToMixedBuffer(&UnpackedBuff, AreaName);
			PrevAreaID = lpCurrent->AreaID;
		}
		AddToMixedBuffer(&UnpackedBuff, &(lpCurrent->MessageID), sizeof(unsigned int));
		AddWStrToMixedBuffer(&UnpackedBuff, (wchar_t *)&(lpCurrent->MsgId));
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(lpCurrent->MessageID), 0, NULL);
		SQLExecDirectW(hstmt, L"Update Outbound set Status=1 where ToLink=? and MessageId=?", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

		lpFirst = lpCurrent->lpNext;
		HeapFree(SI->hHeap, 0, lpCurrent);
	}

	Size = PackData(SI->hHeap, UnpackedBuff.lpBuffer, UnpackedBuff.CurrentSize, (char **)&lpMessage, sizeof(PackedDataHeader));
	SetMailerMsgHeader(lpMessage, CMD_ECHOMAIL_CHECK, Size + sizeof(PackedDataHeader));
	lpMessage->OriginalSize = UnpackedBuff.CurrentSize;
	AddToSendQueue(SI, QUEUE_ITEM_ECHOMAIL, (unsigned char *)lpMessage);
	MixedBufferFreeMem(&UnpackedBuff);

	return TRUE;

}

void MailerProcessEchomailCheck(HSTMT hstmt, lpMailerSessionInfo SI )
{
	MixedBuffer Buff;
	EchomailCheckReplyItem ReplyItem;
	unsigned int AreaID;
	unsigned int Size,UnpackedBuffSize;
	int EchoareaCheckResult;
	size_t Len;
	char * CurrentBuffPos;
	char * UnpackedBuff;
	wchar_t * AreaName;
	wchar_t * MsgId;
	BOOL isSkipCheck;


	Size = GetMailerMsgSize(SI->CurrentRecvBuf);
	UnpackedBuffSize = ((lpPackedDataHeader)(SI->CurrentRecvBuf))->OriginalSize;
	
	if (!UnpackData(SI->hHeap, (SI->CurrentRecvBuf) + sizeof(PackedDataHeader), Size - sizeof(PackedDataHeader), &UnpackedBuff, UnpackedBuffSize))
	{
		SI->SessionStatus = SESS_STAT_ERROR;
		MailerSendCommand(SI, CMD_INFO_UNPACKING_ERROR);
		AddLogEntry(L"Error: incorrect packed data received");
		return;
	}
	CurrentBuffPos = UnpackedBuff;

	InitMixedBuffer(SI->hHeap, &Buff);
	Buff.CurrentSize = sizeof(MailerMsgHeader);

	//
NewArea:

	AreaName = (wchar_t *)CurrentBuffPos;
	Len = wcslen(AreaName);
	CurrentBuffPos += (Len + 1)*sizeof(wchar_t);
	AreaID = 0;
	EchoareaCheckResult = CheckEchoArea(SI->hHeap, hstmt, SI->LinkID, AreaName, &AreaID);

	if (EchoareaCheckResult == ECHOAREA_CHECK_SQLERROR)
	{
		//fatal error
		printf("SQL Error!\n");
		SetEvent(cfg.hExitEvent);
		return;
	}
	isSkipCheck = FALSE;
NextMessage:
	if ((CurrentBuffPos - UnpackedBuff) >= UnpackedBuffSize) goto done;
	ReplyItem.MessageID = *((unsigned int *)CurrentBuffPos);
	CurrentBuffPos += sizeof(unsigned int);
	if (ReplyItem.MessageID == 0) goto NewArea;
	MsgId = (wchar_t *)CurrentBuffPos;
	Len = wcslen(MsgId);
	CurrentBuffPos += (Len + 1)*sizeof(wchar_t);
	if (EchoareaCheckResult == ECHOAREA_CHECK_WRITE_NOT_ALLOWED)
	{
		if (!isSkipCheck)
		{
			ReplyItem.ReplyCode = REPLY_CODE_AREA_NOT_ALLOWED;
			AddToMixedBuffer(&Buff, &ReplyItem, sizeof(EchomailCheckReplyItem));
			isSkipCheck = TRUE;
		}
	}
	else
	{
		if (CheckDupes(hstmt, AreaID, MsgId)) ReplyItem.ReplyCode = REPLY_CODE_DUPE;
		else ReplyItem.ReplyCode = REPLY_CODE_ACCEPTED;
	
		AddToMixedBuffer(&Buff, &ReplyItem, sizeof(EchomailCheckReplyItem));
	}
	goto NextMessage;


	

done:
	HeapFree(SI->hHeap, 0, UnpackedBuff);
	SetMailerMsgHeader(Buff.lpBuffer, CMD_ECHOMAIL_CHECK_REPLY, Buff.CurrentSize);
	AddToSendQueue(SI, QUEUE_ITEM_ECHOMAIL, Buff.lpBuffer);
}

void MailerProcessEchomailCheckReply(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int i, NumOfReplyElements, MessageID;
	EchomailCheckReplyItem * lpReplyItem;
	NumOfReplyElements = (GetMailerMsgSize(SI->CurrentRecvBuf) -sizeof(MailerMsgHeader)) / sizeof(EchomailCheckReplyItem);
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &MessageID, 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);

	for (i = 0; i < NumOfReplyElements; i++)
	{
		lpReplyItem = (lpEchomailCheckReplyItem)((SI->CurrentRecvBuf) + sizeof(MailerMsgHeader) + i*sizeof(EchomailCheckReplyItem));
		MessageID = lpReplyItem->MessageID;
		switch (lpReplyItem->ReplyCode)
		{
		case REPLY_CODE_ACCEPTED:
			SQLExecDirectW(hstmt, L"update Outbound set Status=2 where MessageId=? and ToLink=?", SQL_NTS);
			break;
		case REPLY_CODE_DUPE:
			SQLExecDirectW(hstmt, L"delete from Outbound where MessageId=? and ToLink=?", SQL_NTS);
			break;
		case REPLY_CODE_AREA_NOT_ALLOWED:
			{
				wchar_t AreaName[80];
				wchar_t LogStr[255];
				SQLExecDirectW(hstmt, L"{call sp_PassiveUnacceptedAreaLink(?,?)}", SQL_NTS);
				SQLExecDirectW(hstmt, L"select Echoareas.AreaName from EchoAreas,Echomessages where EchoAreas.AreaID=Echomessages.AreaID and Echomessages.MessageID=?", SQL_NTS);
				SQLFetch(hstmt); 
				SQLGetData(hstmt, 1, SQL_C_WCHAR, AreaName, 160, NULL);
				SQLCloseCursor(hstmt);
				wsprintfW(LogStr, L"WARNING: Remote side not accepting messages in %s echoarea, making subscription passive", AreaName);
				AddLogEntry(LogStr);
			}

		}

	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
}


BOOL MailerSendEchomail(HSTMT hstmt, lpMailerSessionInfo SI)
{
	MixedBuffer UnpackedBuff;
	lpPackedDataHeader lpMessage;
	unsigned int Size;
	SQLRETURN sqlret;
	SQLLEN cb1, cb2, cb3, cb4,cb5;
	wchar_t tmp;
	unsigned int tmpSize;
	
	unsigned int MessageID;
	wchar_t AreaName[80];
	FTNAddr FromAddr;
	wchar_t FromName[36];
	wchar_t ToName[36];
	SQL_TIMESTAMP_STRUCT CreateTime;
	wchar_t Subject[74];
	wchar_t MsgId[127], ReplyTo[127];
	unsigned int UnpackedMsgTextSize;
	char * BinaryData;
	wchar_t * UnpackedMsgText;

	BOOL isHaveSomethingToSend = FALSE;
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	sqlret = SQLExecDirectW(hstmt, L"SELECT EchoMessages.MessageID,EchoAreas.AreaName,FromName,ToName,FromZone,FromNet,FromNode,FromPoint,CreateTime,Subject,MsgId,ReplyTo,OriginalSize,MsgText,Path,SeenBy FROM EchoMessages,EchoAreas,Outbound where EchoAreas.AreaId=EchoMessages.AreaId and EchoMessages.MessageID=OutBound.MessageId and OutBound.ToLink=? and Outbound.Status=2 order by EchoAreas.AreaName,EchoMessages.MessageID", SQL_NTS);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		//
		SQLBindCol(hstmt, 1, SQL_C_ULONG, &MessageID, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_WCHAR, AreaName, 160, NULL);
		SQLBindCol(hstmt, 3, SQL_C_WCHAR, FromName, 72, NULL);
		SQLBindCol(hstmt, 4, SQL_C_WCHAR, ToName, 72, NULL);
		SQLBindCol(hstmt, 5, SQL_C_USHORT, &(FromAddr.zone), 0, NULL);
		SQLBindCol(hstmt, 6, SQL_C_USHORT, &(FromAddr.net), 0, NULL);
		SQLBindCol(hstmt, 7, SQL_C_USHORT, &(FromAddr.node), 0, NULL);
		SQLBindCol(hstmt, 8, SQL_C_USHORT, &(FromAddr.point), 0, NULL);
		SQLBindCol(hstmt, 9, SQL_C_TYPE_TIMESTAMP, &CreateTime, 0, NULL);
		SQLBindCol(hstmt, 10, SQL_C_WCHAR, Subject, 148, NULL);
		SQLBindCol(hstmt, 11, SQL_C_WCHAR, MsgId, 254, &cb1);
		SQLBindCol(hstmt, 12, SQL_C_WCHAR, ReplyTo, 254, &cb2);
		SQLBindCol(hstmt, 13, SQL_C_ULONG, &UnpackedMsgTextSize, 0, NULL);

		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			isHaveSomethingToSend = TRUE;
			InitMixedBuffer(SI->hHeap,&UnpackedBuff);
			AddToMixedBuffer(&UnpackedBuff, &MessageID, sizeof(unsigned int));
			AddToMixedBuffer(&UnpackedBuff, &FromAddr, sizeof(FTNAddr));
			AddToMixedBuffer(&UnpackedBuff, &CreateTime, sizeof(SQL_TIMESTAMP_STRUCT));
			AddWStrToMixedBuffer(&UnpackedBuff, AreaName);
			AddWStrToMixedBuffer(&UnpackedBuff, FromName);
			AddWStrToMixedBuffer(&UnpackedBuff, ToName);
			AddWStrToMixedBuffer(&UnpackedBuff, Subject);
			if (cb1==SQL_NULL_DATA)
				AddWStrToMixedBuffer(&UnpackedBuff, NULL);
			else 
				AddWStrToMixedBuffer(&UnpackedBuff, MsgId);

			if (cb2 == SQL_NULL_DATA)
				AddWStrToMixedBuffer(&UnpackedBuff, NULL);
			else
				AddWStrToMixedBuffer(&UnpackedBuff, ReplyTo);

			//msgtext
			SQLGetData(hstmt, 14, SQL_C_BINARY, &tmp, 0, &cb3);
			UnpackedMsgText = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, UnpackedMsgTextSize);
			BinaryData = HeapAlloc(SI->hHeap, 0, cb3);
			SQLGetData(hstmt, 14, SQL_C_BINARY, BinaryData, cb3, 0);
			uncompress((unsigned char *)UnpackedMsgText, &UnpackedMsgTextSize, BinaryData, (unsigned int)cb3);
			HeapFree(SI->hHeap, 0, BinaryData);

			AddWStrToMixedBuffer(&UnpackedBuff, UnpackedMsgText);
			HeapFree(SI->hHeap, 0, UnpackedMsgText);
			//path
			SQLGetData(hstmt, 15, SQL_C_BINARY, &tmp, 0, &cb4);
			tmpSize = (unsigned int)cb4;
			BinaryData = HeapAlloc(SI->hHeap, 0, cb4);
			SQLGetData(hstmt, 15, SQL_C_BINARY, BinaryData, cb4, 0);
			AddToMixedBuffer(&UnpackedBuff, &tmpSize, sizeof(unsigned int));
			AddToMixedBuffer(&UnpackedBuff, BinaryData, tmpSize);
			HeapFree(SI->hHeap, 0, BinaryData);

			//seen-by
			SQLGetData(hstmt, 16, SQL_C_BINARY, &tmp, 0, &cb5);
			tmpSize = (unsigned int)cb5;
			BinaryData = HeapAlloc(SI->hHeap, 0, cb5);
			SQLGetData(hstmt, 16, SQL_C_BINARY, BinaryData, cb5, 0);
			AddToMixedBuffer(&UnpackedBuff, &tmpSize, sizeof(unsigned int));
			AddToMixedBuffer(&UnpackedBuff, BinaryData, tmpSize);
			HeapFree(SI->hHeap, 0, BinaryData);

			//
			Size = PackData(SI->hHeap, UnpackedBuff.lpBuffer, UnpackedBuff.CurrentSize, (char **)&lpMessage, sizeof(PackedDataHeader));
			SetMailerMsgHeader(lpMessage, CMD_ECHOMAIL_MESSAGE, Size + sizeof(PackedDataHeader));
			lpMessage->OriginalSize = UnpackedBuff.CurrentSize;
			AddToSendQueue(SI, QUEUE_ITEM_ECHOMAIL, (unsigned char *)lpMessage);

			//
			MixedBufferFreeMem(&UnpackedBuff);
			sqlret = SQLFetch(hstmt);
		}
		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		SQLExecDirectW(hstmt, L"Update Outbound set Status=3 where ToLink=? and Status=2", SQL_NTS);
		
	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	if (isHaveSomethingToSend) MailerSendCommand(SI, CMD_ECHOMAIL_BATCH_DONE);
	return isHaveSomethingToSend;

}


void MailerReceiveEchomail(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int Size, UnpackedBuffSize, MessageID;
	EchomailMessage EchomailMsg;
	char * UnpackedBuff;
	char * CurrentBuffPos;
	wchar_t * lpStr;
	size_t Len;

	Size = GetMailerMsgSize(SI->CurrentRecvBuf);
	UnpackedBuffSize = ((lpPackedDataHeader)(SI->CurrentRecvBuf))->OriginalSize;



	if (!UnpackData(SI->hHeap, (SI->CurrentRecvBuf) + sizeof(PackedDataHeader), Size - sizeof(PackedDataHeader), &UnpackedBuff, UnpackedBuffSize))
	{
		SI->SessionStatus = SESS_STAT_ERROR;
		MailerSendCommand(SI, CMD_INFO_UNPACKING_ERROR);
		AddLogEntry(L"Error: incorrect packed data received");
		return;
	}
	CurrentBuffPos = UnpackedBuff;
	memset(&EchomailMsg, 0, sizeof(EchomailMessage));
	
	EchomailMsg.FromLinkID = SI->LinkID;
	
	memcpy(&MessageID, CurrentBuffPos, sizeof(unsigned int));
	CurrentBuffPos += sizeof(unsigned int);

	memcpy(&EchomailMsg.FromAddr, CurrentBuffPos, sizeof(FTNAddr));
	CurrentBuffPos += sizeof(FTNAddr);

	memcpy(&EchomailMsg.CreateTime, CurrentBuffPos, sizeof(SQL_TIMESTAMP_STRUCT));
	CurrentBuffPos += sizeof(SQL_TIMESTAMP_STRUCT);
	
	lpStr = (wchar_t *)CurrentBuffPos;
	EchomailMsg.AreaName = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);
	
	EchomailMsg.FromName = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);

	EchomailMsg.ToName = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);

	EchomailMsg.Subject = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);

	Len = wcslen(lpStr);
	if (Len == 0)
		EchomailMsg.MsgId = NULL;
	else EchomailMsg.MsgId = lpStr;
	lpStr += (Len + 1);

	Len = wcslen(lpStr);
	if (Len == 0)
		EchomailMsg.ReplyTo = NULL;
	else EchomailMsg.ReplyTo = lpStr;
	lpStr += (Len + 1);


	EchomailMsg.MsgText = lpStr;
	Len = wcslen(lpStr);
	lpStr += (Len + 1);
	
	CurrentBuffPos =(char *) lpStr;

	memcpy(&Size, CurrentBuffPos, sizeof(unsigned int));
	EchomailMsg.Path.CurrentSize = Size / sizeof(unsigned int);
	CurrentBuffPos += sizeof(unsigned int);
	EchomailMsg.Path.lpBuffer =(unsigned int *) CurrentBuffPos;
	CurrentBuffPos += Size;

	memcpy(&Size, CurrentBuffPos, sizeof(unsigned int));
	EchomailMsg.SeenBy.CurrentSize = Size / sizeof(unsigned int);
	CurrentBuffPos += sizeof(unsigned int);
	EchomailMsg.SeenBy.lpBuffer = (unsigned int *)CurrentBuffPos;
	

	//
	//
	if (EchomailMsg.MsgId == NULL)
	{
		int EchoareaCheckResult;
		EchoareaCheckResult = CheckEchoArea(SI->hHeap, hstmt, SI->LinkID, EchomailMsg.AreaName, &(EchomailMsg.EchoAreaID));
		if (EchoareaCheckResult == ECHOAREA_CHECK_WRITE_NOT_ALLOWED) goto done;
	}
	else
	{
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 80, 0, EchomailMsg.AreaName, 0, NULL);
		SQLExecDirectW(hstmt, L"SELECT AreaId from EchoAreas WHERE AreaName=?", SQL_NTS);
		SQLFetch(hstmt);
		SQLGetData(hstmt, 1, SQL_C_ULONG, &(EchomailMsg.EchoAreaID), 0, NULL);
		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_UNBIND);
		if (CheckDupes(hstmt, EchomailMsg.EchoAreaID, EchomailMsg.MsgId)) goto done;
			

	}




	if (!AddEchomailMessage(SI->hHeap, hstmt, &EchomailMsg))
	{
		
			//fatal error
			printf("SQL Error!\n");
			SetEvent(cfg.hExitEvent);
			return;
		
	}
	;
done:
	//


	HeapFree(SI->hHeap, 0, UnpackedBuff);
	MailerSendConfirmation(SI, CMD_ECHOMAIL_MESSAGE_RECEIDED, MessageID);
	++(SI->EchomailRcvd);
	return;


}


void MailerReceiveEchomailConfirmation(HSTMT hstmt, lpMailerSessionInfo SI)
{
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(((lpConfirmationMsg)(SI->CurrentRecvBuf))->MessageID), 0, NULL);
	SQLExecDirectW(hstmt, L"DELETE FROM Outbound where ToLink=? and MessageId=?", SQL_NTS);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	++(SI->EchomailSent);
}

BOOL MailerGetNextFile(HSTMT hstmt, lpMailerSessionInfo SI)
{
	BOOL HaveFilesToSent = FALSE;
	SQLRETURN sqlret;
	

	if (SI->SendingFileStatus != FILE_SENDING_NOTHING) return TRUE;
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLExecDirectW(hstmt, L"SELECT FileName from FileOutbound where LinkID=? and Delayed=0 order by id", SQL_NTS);
	sqlret = SQLFetch(hstmt);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		wchar_t LogStr[255];
		HaveFilesToSent = TRUE;
		SQLGetData(hstmt, 1, SQL_C_WCHAR, &(SI->SendingFileName), 520, NULL);
		SI->SendingFileStatus = FILE_SENDING_STARTING;
		SI->FilePackMode = FilePackMode;
		SetEvent(SI->hWriteEnabledEvent);
		wsprintfW(LogStr, L"Sending %s", SI->SendingFileName);
		AddLogEntry(LogStr);
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	return HaveFilesToSent;
}

BOOL MailerSendFileCheck(HSTMT hstmt, lpMailerSessionInfo SI)
{
	MixedBuffer Buff;
	FileCheckHeader FCH;
	FILETIME ft;
	

	SI->hSendingFile = CreateFileW(SI->SendingFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (SI->hSendingFile == INVALID_HANDLE_VALUE)
	{
		//
		LogFileError(SI->SendingFileName);
		SI->SendingFileStatus = FILE_SENDING_NOTHING;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, SI->SendingFileName, 0, NULL);
		SQLExecDirectW(hstmt, L"UPDATE FileOutbound SET Delayed=1 where LinkID=? and FileName=?", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		//
		return MailerGetNextFile(hstmt, SI);
	}

	GetFileSizeEx(SI->hSendingFile, (PLARGE_INTEGER) &(FCH.FileSize));
	SI->SendingFileSize = FCH.FileSize;
	GetFileTime(SI->hSendingFile, NULL, NULL, &ft);
	FCH.LastChangeTime = ft.dwLowDateTime + (((long long)(ft.dwHighDateTime)) << 32);
	InitMixedBuffer(SI->hHeap, &Buff);
	AddToMixedBuffer(&Buff, &FCH, sizeof(FileCheckHeader));
	AddWStrToMixedBuffer(&Buff, GetFileName(SI->SendingFileName));
	SetMailerMsgHeader(Buff.lpBuffer, CMD_FILE_CHECK, Buff.CurrentSize);
	SI->CurrentSendBuf = Buff.lpBuffer;
	SI->SendSize = Buff.CurrentSize;
	SI->SendingFileStatus = FILE_SENDING_CHECK_SENT;
	return TRUE;
}

void MailerProcessFileCheck(HSTMT hstmt, lpMailerSessionInfo SI)
{
	ULARGE_INTEGER DiskFreeSpace;
	wchar_t * FileName;
	SQLRETURN sqlret;
	LARGE_INTEGER AlreadyReceivedBytes;
	long long FileSize,FileLastChangedTime;
	wchar_t FullFileName[MAX_PATH];
	wchar_t LogStr[255];
	BOOL isCreateNew = TRUE;
	lpFileAcceptMsg buf;
	if (SI->ReceivingFileStatus == FILE_RECEIVING_IN_PROGRESS)
	{
		CloseHandle(SI->hReceivingFile);
		SI->ReceivingFileStatus = FILE_RECEIVING_NOTHING;
	}

	GetDiskFreeSpaceExW(cfg.InboundDir, &DiskFreeSpace, NULL, NULL);
	
	FileName = (wchar_t *)(SI->CurrentRecvBuf + sizeof(FileCheckHeader));
	wsprintfW(LogStr, L"Receiving %s (%I64u bytes)", FileName, ((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize);
	AddLogEntry(LogStr);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, FileName, 0, NULL);
	SQLExecDirectW(hstmt, L"SELECT FileSize,FileLastChangedTime from PartiallyReceivedFiles where LinkID=? and FileName=?", SQL_NTS);
	sqlret = SQLFetch(hstmt);
	if ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
	{
		SQLGetData(hstmt, 1, SQL_C_SBIGINT, &FileSize, 0, NULL);
		SQLGetData(hstmt, 2, SQL_C_SBIGINT, &FileLastChangedTime, 0, NULL);
		isCreateNew = FALSE;
			
	}
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

	if (isCreateNew)
	{
		WIN32_FIND_DATAW fd;
		HANDLE hFind;
		AlreadyReceivedBytes.QuadPart = 0;
		//check if exists
		wsprintfW(FullFileName, L"%s\\%s", cfg.InboundDir, FileName);
		hFind=FindFirstFileW(FullFileName, &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			//file exists
			FindClose(hFind);
			if (((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize == (fd.nFileSizeLow + (((long long)(fd.nFileSizeHigh)) << 32)))
			{
				//same size
				AddLogEntry(L"File already exists, sending SKIP command");
				MailerSendCommand(SI, CMD_FILE_SKIP);
				return;
			}

			
		}
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, FileName, 0, NULL);
		SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &(((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize), 0, NULL);
		SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &(((lpFileCheckHeader)(SI->CurrentRecvBuf))->LastChangeTime), 0, NULL);
		SQLExecDirectW(hstmt, L"INSERT INTO PartiallyReceivedFiles VALUES(?,?,?,?)", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		FileSize = ((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize;
		

	}
	
	wsprintfW(FullFileName, L"%s\\%s.PARTIAL.%u_%u_%u_%u", cfg.InboundDir, FileName,SI->LinkAddr.zone,SI->LinkAddr.net,SI->LinkAddr.node,SI->LinkAddr.point);
	SI->hReceivingFile = CreateFileW(FullFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	if (SI->hReceivingFile == INVALID_HANDLE_VALUE)
	{
		LogFileError(FullFileName);
		AddLogEntry(L"File creating error, delayed");
		MailerSendCommand(SI, CMD_FILE_DELAY);
		return;
	}

	if (!isCreateNew)
	{

		if ((FileSize != ((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize) || (FileLastChangedTime != ((lpFileCheckHeader)(SI->CurrentRecvBuf))->LastChangeTime))
		{
			AlreadyReceivedBytes.QuadPart = 0;
			SetFilePointerEx(SI->hReceivingFile, AlreadyReceivedBytes, NULL, FILE_BEGIN);
			SetEndOfFile(SI->hReceivingFile);
			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &(((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize), 0, NULL);
			SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &(((lpFileCheckHeader)(SI->CurrentRecvBuf))->LastChangeTime), 0, NULL);
			SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
			SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, FileName, 0, NULL);
			SQLExecDirectW(hstmt, L"UPDATE PartiallyReceivedFiles set FileSize=?, FileLastChangedTime=? where LinkID=? and FileName=?", SQL_NTS);
			SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		}
		else
		{
			GetFileSizeEx(SI->hReceivingFile, &AlreadyReceivedBytes);
			SetFilePointerEx(SI->hReceivingFile, AlreadyReceivedBytes, NULL, FILE_BEGIN);

		}
		//
	}

	if ((unsigned long long)(((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize - AlreadyReceivedBytes.QuadPart) >= DiskFreeSpace.QuadPart)
	{
			AddLogEntry(L"Not enough free space, file receiving delayed");
			MailerSendCommand(SI, CMD_FILE_DELAY);
			return;
	}
	
	buf = HeapAlloc(SI->hHeap, 0, sizeof(FileAcceptMsg));
	buf->Offset = AlreadyReceivedBytes.QuadPart;
	SetMailerMsgHeader(buf, CMD_FILE_ACCEPT, sizeof(FileAcceptMsg));
	AddToSendQueue(SI, QUEUE_ITEM_COMMAND, (unsigned char *)buf);
	SI->ReceivingFileSize = ((lpFileCheckHeader)(SI->CurrentRecvBuf))->FileSize;
	SI->FileAlreadyReceivedBytes = AlreadyReceivedBytes.QuadPart;
	SI->ReceivingFileStatus = FILE_RECEIVING_IN_PROGRESS;
	wcscpy_s(SI->ReceivingFileName,MAX_PATH, FileName);

}

void MailerFileCheckOk(lpMailerSessionInfo SI)
{
	wchar_t LogStr[255];
	LARGE_INTEGER Offset;
	SI->SendingFileStatus = FILE_SENDING_IN_PROGRESS;
	SI->FileAlreadySentBytes = ((lpFileAcceptMsg)(SI->CurrentRecvBuf))->Offset;
	Offset.QuadPart = ((lpFileAcceptMsg)(SI->CurrentRecvBuf))->Offset;
	SetFilePointerEx(SI->hSendingFile, Offset, NULL, FILE_BEGIN);
	wsprintfW(LogStr, L"Sending from %I64u (%I64u bytes remaining)", SI->FileAlreadySentBytes, SI->SendingFileSize - SI->FileAlreadySentBytes);
	AddLogEntry(LogStr);
	SetEvent(SI->hWriteEnabledEvent);
}

void MailerFileSendingDone(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int isDelete=0;
	SI->SendingFileStatus = FILE_SENDING_NOTHING;
	CloseHandle(SI->hSendingFile);

	if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode==CMD_FILE_DELAY)
	{
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, SI->SendingFileName, 0, NULL);
		SQLExecDirectW(hstmt, L"UPDATE FileOutbound SET Delayed=1 where LinkID=? and FileName=?", SQL_NTS);
		AddLogEntry(L"File sending delayed by remote");
	}
	else
	{
		if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_FILE_SKIP)
			AddLogEntry(L"File skipped by remote");
		else
		{
			AddLogEntry(L"File sending complete");
			++(SI->FilesSent);
		}
		SQLBindParameter(hstmt, 1, SQL_PARAM_OUTPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &isDelete, 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, SI->SendingFileName, 0, NULL);
		SQLExecDirectW(hstmt, L"{?=call  sp_FileSent(?,?)}", SQL_NTS);
		if (isDelete != 0)
		{
			DeleteFileW(SI->SendingFileName);
		}
		//
	}
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	FillSendQueue(hstmt, SI);
}

void MailerSendFilePart(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int PartSize;
	long long RemainingBytes;
	char * buf;
	lpPackedDataHeader PackedMessage;
	unsigned int PackedSize;
	DWORD cb;

	RemainingBytes = (SI->SendingFileSize) - (SI->FileAlreadySentBytes);
	if (RemainingBytes <= SI->FileFrameSize)
	{
		PartSize = (unsigned int)RemainingBytes;
		SI->SendingFileStatus = FILE_SENDING_FINISHED;

	}
	else PartSize = SI->FileFrameSize;

	buf = HeapAlloc(SI->hHeap, 0, PartSize + sizeof(MailerMsgHeader));
	if (!ReadFile(SI->hSendingFile, buf + sizeof(MailerMsgHeader), PartSize, &cb, NULL))
	{
		HeapFree(SI->hHeap, 0, buf);
		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, SI->SendingFileName, 0, NULL);
		SQLExecDirectW(hstmt, L"update FileOutbound SET Delayed=1 where LinkID=? and FileName=?", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		LogFileError(SI->SendingFileName);
		AddLogEntry(L"Sending delayed");

		SI->SendingFileStatus = FILE_SENDING_NOTHING;
		CloseHandle(SI->hSendingFile);
		FillSendQueue(hstmt, SI);
		return;

	}
	SI->FileAlreadySentBytes += PartSize;

	if (SI->FilePackMode != FILE_PACK_NONE) goto pack;
send_unpacked:
	SetMailerMsgHeader(buf, CMD_FILE_PART_UNCOMPRESSED, PartSize + sizeof(MailerMsgHeader));
	SI->CurrentSendBuf = buf;
	SI->SendSize = PartSize + sizeof(MailerMsgHeader);
	return;


pack:
	PackedSize = PackData(SI->hHeap, buf + sizeof(MailerMsgHeader), PartSize, (char **)&PackedMessage, sizeof(PackedDataHeader));
	if (SI->FilePackMode == FILE_PACK_ADAPTIVE)
	{
		
		if (PartSize*100 <= PackedSize*105)
		{
			// no need pack
			SI->FilePackMode = FILE_PACK_NONE;
			HeapFree(SI->hHeap, 0, PackedMessage);
			goto send_unpacked;
		}
		else SI->FilePackMode = FILE_PACK_ALWAYS;

	}
	HeapFree(SI->hHeap, 0, buf);
	PackedMessage->OriginalSize = PartSize;
	SetMailerMsgHeader(PackedMessage, CMD_FILE_PART_COMPRESSED, PackedSize + sizeof(PackedDataHeader));
	SI->CurrentSendBuf =(char *) PackedMessage;
	SI->SendSize = PackedSize + sizeof(PackedDataHeader);

}

void MailerReceiveFilePart(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned int Size,PackedSize;
	char * buf ;
	DWORD cb;
	if (SI->ReceivingFileStatus != FILE_RECEIVING_IN_PROGRESS) return;

	if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_FILE_PART_COMPRESSED)
	{
		
		PackedSize = GetMailerMsgSize(SI->CurrentRecvBuf) - sizeof(PackedDataHeader);
		Size = ((lpPackedDataHeader)(SI->CurrentRecvBuf))->OriginalSize;
		if (!UnpackData(SI->hHeap, (SI->CurrentRecvBuf) + sizeof(PackedDataHeader), PackedSize, &buf, Size))
		{
			SI->SessionStatus = SESS_STAT_ERROR;
			MailerSendCommand(SI, CMD_INFO_UNPACKING_ERROR);
			AddLogEntry(L"Error: incorrect packed data received");
			return;
		}
		//
	}
	else
	{
		buf = SI->CurrentRecvBuf + sizeof(MailerMsgHeader);
		Size = GetMailerMsgSize(SI->CurrentRecvBuf) - sizeof(MailerMsgHeader);
	}

	if (!WriteFile(SI->hReceivingFile, buf, Size, &cb, NULL))
	{
		LogFileError(SI->ReceivingFileName);
		AddLogEntry(L"File receiving delayed");

		SI->ReceivingFileStatus = FILE_RECEIVING_NOTHING;
		CloseHandle(SI->hReceivingFile);
		MailerSendCommand(SI, CMD_FILE_DELAY);
		if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_FILE_PART_COMPRESSED)
			HeapFree(SI->hHeap, 0, buf);
		return;
	}
	//
	if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_FILE_PART_COMPRESSED)
		HeapFree(SI->hHeap, 0, buf);
	SI->FileAlreadyReceivedBytes += Size;
	if (SI->FileAlreadyReceivedBytes >= SI->ReceivingFileSize) 
	{
		wchar_t FileNameTmp[MAX_PATH], FileNameFinal[MAX_PATH];
		int i=0;
		AddLogEntry(L"File receiving complete");
		SI->ReceivingFileStatus = FILE_RECEIVING_NOTHING;
		CloseHandle(SI->hReceivingFile);

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 520, 0, SI->ReceivingFileName, 0, NULL);
		SQLExecDirectW(hstmt, L"DELETE FROM PartiallyReceivedFiles where LinkID=? and FileName=?", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		wsprintfW(FileNameTmp, L"%s\\%s.PARTIAL.%u_%u_%u_%u", cfg.InboundDir, SI->ReceivingFileName, SI->LinkAddr.zone, SI->LinkAddr.net, SI->LinkAddr.node, SI->LinkAddr.point);
		wsprintfW(FileNameFinal, L"%s\\%s", cfg.InboundDir, SI->ReceivingFileName);
		if (FileOverwrite != 0)
		{
			MoveFileExW(FileNameTmp, FileNameFinal, MOVEFILE_REPLACE_EXISTING);
		}
		else
		{
			while (!MoveFileExW(FileNameTmp, FileNameFinal, 0))
			{
				i++;
				wsprintfW(FileNameFinal, L"%s\\%s.%u", cfg.InboundDir, SI->ReceivingFileName,i);
			}
		}

		MailerSendCommand(SI, CMD_FILE_RECEIVED);
		++(SI->FilesRcvd);

	}
}

BOOL FillSendQueue(HSTMT hstmt, lpMailerSessionInfo SI)
{
	BOOL isHaveSomethingToSend = FALSE;

	if (MailerSendNetmail(hstmt, SI)) isHaveSomethingToSend = TRUE;
	//
	if (MailerSendEchomailCheck(hstmt, SI)) isHaveSomethingToSend = TRUE;

	if (MailerSendEchomail(hstmt, SI)) isHaveSomethingToSend = TRUE;
	
	if (MailerGetNextFile(hstmt, SI)) isHaveSomethingToSend = TRUE;
	//

	return isHaveSomethingToSend;
}

