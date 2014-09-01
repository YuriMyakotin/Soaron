/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Mailer.h"

unsigned int MailerRescanTime, MailerIdleCount, MailerIdleTimeout;
unsigned int DefaultFileFrameSize, FilePackMode,FileOverwrite;
unsigned int UseEncryption;





void AddToSendQueue(lpMailerSessionInfo SI, unsigned int Type, void * Buf)
{
	lpSendQueueItem lpNewItem;
	lpNewItem = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, sizeof(SendQueueItem));
	lpNewItem->MsgBuf = Buf;
	
	switch (Type)
	{
	case QUEUE_ITEM_COMMAND: if (SI->CommandsQueueLast == NULL) SI->CommandsQueueFirst = lpNewItem;
			else SI->CommandsQueueLast->NextItem = lpNewItem;
			SI->CommandsQueueLast = lpNewItem;
			break;
	case QUEUE_ITEM_NETMAIL:
			if (SI->NetmailQueueLast == NULL) SI->NetmailQueueFirst = lpNewItem;
			else SI->NetmailQueueLast->NextItem = lpNewItem;
			SI->NetmailQueueLast = lpNewItem;
			break;
			
	case QUEUE_ITEM_ECHOMAIL:
			if (SI->EchomailQueueLast == NULL) SI->EchomailQueueFirst = lpNewItem;
			else SI->EchomailQueueLast->NextItem = lpNewItem;
			SI->EchomailQueueLast = lpNewItem;
			break;
			

			
	}
	SetEvent(SI->hWriteEnabledEvent);
}
/*
FALSE очередь пуста
TRUE след. сообщение помещено в SessionInfo
*/

BOOL GetNextFromSendQueue(HSTMT hstmt, lpMailerSessionInfo SI)
{
	lpSendQueueItem lpItem;
	if (SI->CommandsQueueFirst != NULL)
	{
		lpItem = SI->CommandsQueueFirst;
		SI->CommandsQueueFirst = lpItem->NextItem;
		if (SI->CommandsQueueFirst == NULL) SI->CommandsQueueLast = NULL;
		
	}
	else if (SI->NetmailQueueFirst != NULL)
	{
		lpItem = SI->NetmailQueueFirst;
		SI->NetmailQueueFirst = lpItem->NextItem;
		if (SI->NetmailQueueFirst == NULL) SI->NetmailQueueLast = NULL;

	}
	else if (SI->EchomailQueueFirst != NULL)
	{
		lpItem = SI->EchomailQueueFirst;
		SI->EchomailQueueFirst = lpItem->NextItem;
		if (SI->EchomailQueueFirst == NULL) SI->EchomailQueueLast = NULL;

	}
	else goto checkfiles;

	SI->CurrentSendBuf = lpItem->MsgBuf;
	SI->SendSize = GetMailerMsgSize(lpItem->MsgBuf);
	HeapFree(SI->hHeap, 0, lpItem);
	return TRUE;

checkfiles:
	switch (SI->SendingFileStatus)
	{
	case FILE_SENDING_NOTHING:
	case FILE_SENDING_CHECK_SENT:
	case FILE_SENDING_FINISHED:
		return FALSE;
	case FILE_SENDING_STARTING:
		return MailerSendFileCheck(hstmt, SI);
		break;
	case FILE_SENDING_IN_PROGRESS:
		MailerSendFilePart(hstmt, SI);
		break;

	}

	return TRUE;

}


/*
0 ок, буфер освобожден
1 отосланы не все сообщения
2 ошибка
3 буфер заполнен, отправка на паузе
*/
int MailerSend(lpMailerSessionInfo SI)
{
	
	int BytesSent;
	int ErrCode;
	if (SI->cbAlreadySent == 0)
	{
		if (SI->EncryptionEnabled)
		{
			switch (((lpMailerMsgHeader)(SI->CurrentSendBuf))->CmdCode)
			{
				case CMD_NETMAIL_MESSAGE:
				case CMD_NETMAIL_MSG_RECEIVED:
				case CMD_ECHOMAIL_CHECK:
				case CMD_ECHOMAIL_CHECK_REPLY:
				case CMD_ECHOMAIL_MESSAGE:
				case CMD_ECHOMAIL_MESSAGE_RECEIDED:
				case CMD_FILE_CHECK:
				case CMD_FILE_ACCEPT:
				case CMD_FILE_PART_UNCOMPRESSED:
				case CMD_FILE_PART_COMPRESSED:
					EncryptData(SI);
					
			}
		}
		BytesSent = send(SI->Sock, SI->CurrentSendBuf, SI->SendSize, 0);
	}
	else
	{
		BytesSent = send(SI->Sock, (SI->CurrentSendBuf) + SI->cbAlreadySent, SI->SendSize-SI->cbAlreadySent, 0);
	}//
	if (BytesSent == SOCKET_ERROR)
	{
		ErrCode = WSAGetLastError();
		
		if (ErrCode == WSAEWOULDBLOCK)
		{
			ResetEvent(SI->hWriteEnabledEvent);
			return 3;
		}
		else
		{
			LogNetworkError();
			return 2;
		}
			
	}
	else 	
	if (BytesSent == (SI->SendSize-SI->cbAlreadySent))
	{
		HeapFree(SI->hHeap, 0, SI->CurrentSendBuf);
		SI->CurrentSendBuf = NULL;
		SI->cbAlreadySent = 0;
		SI->SendSize = 0;
		return 0;
	}
	else
	{
		ResetEvent(SI->hWriteEnabledEvent);
		SI->cbAlreadySent += BytesSent;
		return 1;
	}
	




	return 0;
}

void MailerSendCommand(lpMailerSessionInfo SI, unsigned char CmdCode)
{
	char * buf;
	buf = HeapAlloc(SI->hHeap, 0, sizeof(MailerMsgHeader));
	SetMailerMsgHeader(buf, CmdCode, sizeof(MailerMsgHeader));
	AddToSendQueue(SI, QUEUE_ITEM_COMMAND, buf);
}

void MailerSendConfirmation(lpMailerSessionInfo SI, unsigned char CmdCode, unsigned int ID)
{
	lpConfirmationMsg buf;
	buf = HeapAlloc(SI->hHeap, 0, sizeof(ConfirmationMsg));
	SetMailerMsgHeader(buf, CmdCode, sizeof(ConfirmationMsg));
	buf->MessageID = ID;
	AddToSendQueue(SI, QUEUE_ITEM_COMMAND, buf);
}


DWORD WINAPI MailerSessionThread(LPVOID param)
{

	lpMailerSessionInfo SI;
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	HANDLE hEvent[3];
	DWORD EventNum;
	wchar_t LogStr[255];
	WSANETWORKEVENTS NE;
	int SendResult,cbRecv;
	unsigned char *buf;
	MailerMsgHeader RecvHDR;
	unsigned int MsgSize;
	
	unsigned int IdleTime = MailerIdleTimeout;


	SI = (lpMailerSessionInfo)param;
	_InterlockedIncrement(&(cfg.ThreadCount));

	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{
		printf("SQL ERROR\n");
		SetEvent(cfg.hExitEvent);
		goto exit;
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	SI->hHeap = HeapCreate(HEAP_NO_SERIALIZE, 65536, 0);
	
	if (SI->SessionStatus == SESS_STAT_SRV_FIRST)
	{//server
		wchar_t AddrName[50];
		
		universal_sa sa;
		int namelen = sizeof(universal_sa);
		getpeername(SI->Sock, &sa.sa,&namelen);
		if (sa.sa.sa_family != AF_INET6)
			InetNtopW(AF_INET, &sa.sa4.sin_addr, AddrName, 50);
		else InetNtopW(AF_INET6, &sa.sa6.sin6_addr, AddrName, 50);
		
		wsprintfW(LogStr, L"Incoming call from %s", AddrName);
		AddLogEntry(LogStr);
		MsgSize = GetMailerMsgSize(lpFirstDataToSend);
		buf = HeapAlloc(SI->hHeap, 0, MsgSize);
		memcpy(buf, lpFirstDataToSend, MsgSize);
		
		for (int i = 0; i < 32 / sizeof(unsigned int); i++)
		{
			rand_s(&(SI->HASH[i]));//
		}
		memcpy((unsigned char *)&(((lpLoginHeader)buf)->HASH), &(SI->HASH), 32);


		AddToSendQueue(SI, QUEUE_ITEM_COMMAND, buf);
		SI->SessionStatus = SESS_STAT_SRV_WAITIN_LOGIN;

	}
	else
	{//connect client
		unsigned short port;
		wchar_t Ip[256];
		universal_sa sa;
		DWORD v6only = 0;
		int addr_len = sizeof(universal_sa);
		ADDRINFOW *result = NULL;
		int namelen;
		wchar_t AddrName[50];
		

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLExecDirectW(hstmt, L"Update Links set isCalling=1 where LinkID=?", SQL_NTS);
		SQLExecDirectW(hstmt, L"Select ip,ipport from Links where LinkID=?", SQL_NTS);
		SQLFetch(hstmt);
		SQLGetData(hstmt, 1, SQL_C_WCHAR, Ip, 512, NULL);
		SQLGetData(hstmt, 2, SQL_C_USHORT, &port, 0, NULL);

		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
		if (port == 0) port = 8502;
		GetLinkInfo(hstmt, SI);
		
		wsprintfW(LogStr, L"Calling %u:%u/%u.%u (%s)", SI->LinkAddr.zone, SI->LinkAddr.net, SI->LinkAddr.node, SI->LinkAddr.point, Ip);
		AddLogEntry(LogStr);

		if (GetAddrInfoW(Ip, NULL, NULL, &result) != 0)
		{
			LogNetworkError();
			SI->SessionResult = SESS_RESULT_CANT_CONNECT;
			goto close;
		}
		
		memset(&sa, 0, sizeof(universal_sa));
		if (result->ai_family == AF_INET) namelen = sizeof(struct sockaddr_in);
		else namelen = sizeof(struct sockaddr_in6);
		
		memcpy(&sa, result->ai_addr,namelen );


		FreeAddrInfoW(result);
		


		if (sa.sa.sa_family != AF_INET6)
		{
			InetNtopW(AF_INET, &(sa.sa4.sin_addr), AddrName, 50);
			sa.sa4.sin_port = htons(port);
			SI->Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		}
		else
		{
			InetNtopW(AF_INET6, &(sa.sa6.sin6_addr), AddrName, 50);
			sa.sa6.sin6_port = htons(port);
			SI->Sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			setsockopt(SI->Sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&v6only, sizeof(v6only));
			
		}
		
		if (connect(SI->Sock,&sa.sa,namelen)==0)
		{ //connected ok
			

			wsprintfW(LogStr, L"Connected to %s", AddrName);
			AddLogEntry(LogStr);
			SI->SessionStatus = SESS_STAT_CLIENT_WAITIN_FIRST;
			//
		}
		else
		{//error
			LogNetworkError();
			SI->SessionResult = SESS_RESULT_CANT_CONNECT;
			goto close;
		}

	}
	SI->hWriteEnabledEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	hEvent[0] = cfg.hExitEvent;
	hEvent[2] = SI->hWriteEnabledEvent;
	

	hEvent[1] = WSACreateEvent();
	WSAEventSelect(SI->Sock, hEvent[1], FD_WRITE | FD_READ | FD_CLOSE);
//

	
//
cycle:
	EventNum = WSAWaitForMultipleEvents(3, hEvent, FALSE, IdleTime*1000, FALSE);
	if (EventNum != WAIT_TIMEOUT)
	{
		SI->WaitingTime = 0;
	}
	switch (EventNum)
	{
		case WAIT_OBJECT_0:
			goto close;
		case WAIT_OBJECT_0 + 1:
		{
			WSAEnumNetworkEvents(SI->Sock, hEvent[1], &NE);
			if (NE.lNetworkEvents&FD_CLOSE) goto close;
			if (NE.lNetworkEvents&FD_WRITE)
			{
				if (NE.iErrorCode[FD_WRITE_BIT] != 0)
				{
					wsprintfW(LogStr, L"Network error %u", NE.iErrorCode[FD_ACCEPT_BIT]);
					AddLogEntry(LogStr);
					goto close;
				}
				
				SetEvent(SI->hWriteEnabledEvent);
				
			}
			if (NE.lNetworkEvents&FD_READ) //read
			{
				if (NE.iErrorCode[FD_READ_BIT] != 0)
				{
					wsprintfW(LogStr, L"Network error %u", NE.iErrorCode[FD_ACCEPT_BIT]);
					AddLogEntry(LogStr);
					goto close;
				}
				if (SI->cbAlreadyRecv < 4)
				{
					cbRecv = recv(SI->Sock, (unsigned char *)&RecvHDR + SI->cbAlreadyRecv, 4 - SI->cbAlreadyRecv, 0);
					if (cbRecv == SOCKET_ERROR) goto close;
					SI->cbAlreadyRecv += cbRecv;
					if (SI->cbAlreadyRecv < 4) goto cycle;
					if (RecvHDR.CmdCode < 199) goto close; //incorrect command;
					
					SI->RecvSize = RecvHDR.Header >> 8;
					

					if (SI->RecvSize>1000000)  goto close; //incorrect size;

					SI->CurrentRecvBuf = HeapAlloc(SI->hHeap, 0, SI->RecvSize);
					memcpy(SI->CurrentRecvBuf, &RecvHDR, sizeof(RecvHDR));
					
				}
				if (SI->RecvSize == 4) goto process;

				cbRecv = recv(SI->Sock, (unsigned char *)SI->CurrentRecvBuf + SI->cbAlreadyRecv, SI->RecvSize - SI->cbAlreadyRecv, 0);
				if (cbRecv == SOCKET_ERROR) goto close;
				SI->cbAlreadyRecv += cbRecv;
				if (SI->cbAlreadyRecv<SI->RecvSize) goto cycle;
				//process message
			process:
				if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode==CMD_SESSION_CLOSE) goto close;
				if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_INFO_UNPACKING_ERROR)
				{
					AddLogEntry(L"Other side received incorrect packed data");
					SI->SessionResult = SESS_RESULT_DATA_ERROR;
					goto close;
				}
				switch (SI->SessionStatus)
				{
				case SESS_STAT_SRV_WAITIN_LOGIN:
						switch (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode)
						{
							case CMD_CLIENT_FIRST:
							{
								unsigned char HASH[32];

								if (!MailerLogRemoteInfo(SI))
								//if (!LogSessionInfo(SI->hHeap, SI->RecvSize - sizeof(LoginHeader), ((lpLoginHeader)(SI->CurrentRecvBuf))->NumOfAkas, ((lpLoginHeader)(SI->CurrentRecvBuf))->PackedContentOriginalSize, SI->CurrentRecvBuf + sizeof(LoginHeader)))
								{
									SI->SessionStatus = SESS_STAT_ERROR;
									MailerSendCommand(SI,  CMD_INFO_UNPACKING_ERROR);
									AddLogEntry(L"Error: incorrect packed data received");
									break;
								};

								SI->LinkID=GetLinkID(hstmt, ((lpLoginHeader)(SI->CurrentRecvBuf))->NumOfAkas, SI->CurrentRecvBuf + sizeof(LoginHeader));
								if (SI->LinkID==0)
								{
									SI->SessionStatus = SESS_STAT_ERROR;
									AddLogEntry(L"Error: unknown link");
									MailerSendCommand(SI,  CMD_SRV_UNKNOWN_LINK);
									break;
								}
								GetLoginHash(hstmt, SI->LinkID,(unsigned char *)&(SI->HASH), HASH);
								if (memcmp(HASH, (unsigned char *)&(((lpLoginHeader)(SI->CurrentRecvBuf))->HASH), 32)!=0)
								{
									SI->SessionStatus = SESS_STAT_ERROR;
									AddLogEntry(L"Error: incorrect password");
									MailerSendCommand(SI,  CMD_SRV_INCORRECT_PWD);
									
								}
								else
								{
									unsigned char isBusy;
									SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
									SQLExecDirectW(hstmt, L"select isBusy from Links where LinkID=?", SQL_NTS);
									SQLFetch(hstmt);
									SQLGetData(hstmt, 1, SQL_C_BIT, &isBusy, 0, NULL);
									SQLCloseCursor(hstmt);
									if (isBusy)
									{
										SQLFreeStmt(hstmt, SQL_UNBIND);
										AddLogEntry(L"Already have active session");
										MailerSendCommand(SI,  CMD_SRV_BUSY);
										SI->LinkID = 0;
										break;
									}

									SQLExecDirectW(hstmt, L"update Links set isBusy=1,LastSessionType=3, LastSessionTime=Getdate(), TotalSessions=TotalSessions+1, ThisDaySessions=ThisDaySessions+1 where LinkID=?", SQL_NTS);
									SQLExecDirectW(hstmt, L"update FileOutbound set Delayed=0 where LinkID=?", SQL_NTS);
									SQLFreeStmt(hstmt, SQL_UNBIND);
									IdleTime = MailerRescanTime;


									SI->SessionStatus = SESS_STAT_LINK_OK;
									AddLogEntry(L"Password protected session");
									if ((UseEncryption != 0) && (((lpLoginHeader)(SI->CurrentRecvBuf))->UseEncryption != 0))
									{
										SI->EncryptionEnabled = TRUE;
										AddLogEntry(L"Using data encryption");
										PrepareEncryption(hstmt, SI);
									}



									MailerSendCommand(SI,  CMD_SRV_OK);
									GetLinkInfo(hstmt, SI);
									FillSendQueue(hstmt, SI);
								}
								break;
							}
							case CMD_NETMAIL_MESSAGE:
								MailerReceiveNetmail(hstmt, SI);
								break;
							
							//

							default: goto invalidcommand;
						}

						break;
					

					case SESS_STAT_CLIENT_WAITIN_FIRST:
						switch (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode)
						{
						case CMD_SRV_FIRST:
							//;
							{
								unsigned int LinkID1;
								if (!MailerLogRemoteInfo(SI))
								{
									SI->SessionStatus = SESS_STAT_ERROR;
									MailerSendCommand(SI,  CMD_INFO_UNPACKING_ERROR);
									AddLogEntry(L"Error: incorrect packed data received");
									break;
								};
								LinkID1 = GetLinkID(hstmt, ((lpLoginHeader)(SI->CurrentRecvBuf))->NumOfAkas, SI->CurrentRecvBuf + sizeof(LoginHeader));
								if (LinkID1 != SI->LinkID)
								{
									AddLogEntry(L"Error: no common AKAs");
									goto close;
								}
								MsgSize = GetMailerMsgSize(lpFirstDataToSend);
								buf = HeapAlloc(SI->hHeap, 0, MsgSize);
															
								memcpy(buf, lpFirstDataToSend, MsgSize);
								((lpMailerMsgHeader)buf)->CmdCode = CMD_CLIENT_FIRST;
								
								memcpy(&(SI->HASH), (unsigned char *)&(((lpLoginHeader)(SI->CurrentRecvBuf))->HASH), 32);
								GetLoginHash(hstmt, SI->LinkID, (unsigned char *)&(((lpLoginHeader)(SI->CurrentRecvBuf))->HASH), (unsigned char *)&(((lpLoginHeader)(buf))->HASH));
								
								if ((UseEncryption != 0) && (((lpLoginHeader)(SI->CurrentRecvBuf))->UseEncryption != 0))
									SI->EncryptionEnabled = TRUE;
															
								
								//
								
								AddToSendQueue(SI,  QUEUE_ITEM_COMMAND, buf);
							
								SI->SessionStatus = SESS_STAT_CLIENT_LOGIN_SENT;
								break;
							}

							default: goto invalidcommand;
						}
						break;

					case SESS_STAT_CLIENT_LOGIN_SENT:
						switch (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode)
						{
							case CMD_SRV_OK:
								SI->SessionStatus = SESS_STAT_LINK_OK;
								
								SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
								SQLExecDirectW(hstmt, L"update Links set isBusy=1, Dialout=0, LastSessionType=3, LastSessionTime=Getdate(), TotalSessions=TotalSessions+1, ThisDaySessions=ThisDaySessions+1 where LinkID=?", SQL_NTS);
								SQLExecDirectW(hstmt, L"update FileOutbound set Delayed=0 where LinkID=?", SQL_NTS);
								SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
								IdleTime = MailerRescanTime;
								AddLogEntry(L"Password protected session");
								if (SI->EncryptionEnabled)
								{
									AddLogEntry(L"Using data encryption");
									PrepareEncryption(hstmt, SI);
								}
								FillSendQueue(hstmt, SI);
								//send data
								break;
							case CMD_SRV_INCORRECT_PWD:
								AddLogEntry(L"Error: invalid password");
								SI->SessionResult = SESS_RESULT_LOGIN_PROBLEM;
								goto close;
							case CMD_SRV_UNKNOWN_LINK:
								AddLogEntry(L"Error: unknown link");
								SI->SessionResult = SESS_RESULT_LOGIN_PROBLEM;
								goto close;
							case CMD_SRV_BUSY:
								AddLogEntry(L"Error: already have active session");
								SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
								SQLExecDirectW(hstmt, L"Update Links set isCalling=0 where LinkID=?", SQL_NTS);
								SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
								SI->LinkID = 0;
								goto close;

							default: goto invalidcommand;
						}
						break;
					case SESS_STAT_LINK_OK:

						if (SI->EncryptionEnabled)
						{
							
							if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode == CMD_ENCRYPTED_INFO)
								DecryptData(SI);
							
						}

						if (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode != CMD_IDLE) SI->IdleCount = 0;

						switch (((lpMailerMsgHeader)(SI->CurrentRecvBuf))->CmdCode)
						{
						
							//netmail commands
						case CMD_NETMAIL_MESSAGE:
							MailerReceiveNetmail(hstmt, SI);
							break;
						case CMD_NETMAIL_MSG_RECEIVED:
							MailerReceiveNetmailConfirmation(hstmt, SI);
							break;

							//echomail commands
						case CMD_ECHOMAIL_CHECK:
							MailerProcessEchomailCheck(hstmt, SI);
							break;
						case CMD_ECHOMAIL_CHECK_REPLY:
							MailerProcessEchomailCheckReply(hstmt, SI);
							MailerSendEchomail(hstmt, SI);
							break;
						case CMD_ECHOMAIL_MESSAGE:
							MailerReceiveEchomail(hstmt, SI);
							break;
						case CMD_ECHOMAIL_MESSAGE_RECEIDED:
							MailerReceiveEchomailConfirmation(hstmt,SI);
							break;

						case CMD_ECHOMAIL_BATCH_DONE:
							SetEvent(cfg.hEchomailTossEvent);
							break;

							//file commands
						case CMD_FILE_CHECK:
							MailerProcessFileCheck(hstmt, SI);
							break;
						case CMD_FILE_ACCEPT:
							MailerFileCheckOk(SI);
							break;
						case CMD_FILE_SKIP:
						case CMD_FILE_DELAY:
						case CMD_FILE_RECEIVED:
							MailerFileSendingDone(hstmt, SI);
							break;


						case CMD_FILE_PART_UNCOMPRESSED:
						case CMD_FILE_PART_COMPRESSED:
							MailerReceiveFilePart(hstmt, SI);
							break;

							//session finishing
						case CMD_IDLE:
							//recheck
							if (!FillSendQueue(hstmt, SI))
							{
								++SI->IdleCount;
								if (SI->IdleCount >= MailerIdleCount)
									MailerSendCommand(SI,  CMD_SESSION_CLOSE);
							}
							
							break;



						default: goto invalidcommand;
							
						}
						break;
					case SESS_STAT_ERROR: goto invalidcommand;

				}

				




				//
				
				SI->cbAlreadyRecv = 0;
				SI->RecvSize = 0;
				HeapFree(SI->hHeap, 0, SI->CurrentRecvBuf);
			}

			break;

			
		}
		case WAIT_OBJECT_0 + 2:
		{
			if (SI->CurrentSendBuf != NULL) SendResult = MailerSend(SI); //finish sending message
			else if (GetNextFromSendQueue(hstmt,SI)) SendResult = MailerSend(SI); //send next
			else
			{//queue empty
				ResetEvent(SI->hWriteEnabledEvent);
				break;
			};
			if (SendResult == 2)
			{
				goto close;
			}
			break;
			//write;
		}
		case WAIT_TIMEOUT:
		{
			if (SI->SessionStatus != SESS_STAT_LINK_OK)
			{
				AddLogEntry(L"Timeout");
				goto close;
			}
			else
			{
				SI->WaitingTime += IdleTime;
				if (SI->WaitingTime >= MailerIdleTimeout)
				{
					AddLogEntry(L"Timeout");
					goto close;
				}
				
				if (!FillSendQueue(hstmt, SI)) MailerSendCommand(SI,  CMD_IDLE);

			}
							 				
		}
	}
	
	
	goto cycle;




//
invalidcommand:

	AddLogEntry(L"Invalid command received");

close:
	
	shutdown(SI->Sock, SD_BOTH);
	closesocket(SI->Sock);
	
	CloseHandle(hEvent[1]);
	CloseHandle(SI->hWriteEnabledEvent);
	//
	if (SI->LinkID != 0)
	{
		if (SI->SendingFileStatus != FILE_SENDING_NOTHING) CloseHandle(SI->hSendingFile);
		if (SI->ReceivingFileStatus == FILE_RECEIVING_IN_PROGRESS) CloseHandle(SI->hSendingFile);

		switch (SI->SessionResult)
		{
		case SESS_RESULT_OK:
			wsprintfW(LogStr, L"Session with %u:%u/%u.%u completed. Sent/received: Netmail %u/%u, Echomail %u/%u, Files %u/%u", SI->LinkAddr.zone, SI->LinkAddr.net, SI->LinkAddr.node, SI->LinkAddr.point, SI->NetmailSent, SI->NetmailRcvd, SI->EchomailSent, SI->EchomailRcvd, SI->FilesSent, SI->FilesRcvd);
			break;
		case SESS_RESULT_CANT_CONNECT:
		case SESS_RESULT_LOGIN_PROBLEM:
			wsprintfW(LogStr,L"Session closed");
			break;
		case SESS_RESULT_NETWORK_ERROR:
		case SESS_RESULT_DATA_ERROR:
			wsprintfW(LogStr, L"Session with %u:%u/%u.%u closed with error. Sent/received: Netmail %u/%u, Echomail %u/%u, Files %u/%u", SI->LinkAddr.zone, SI->LinkAddr.net, SI->LinkAddr.node, SI->LinkAddr.point, SI->NetmailSent, SI->NetmailRcvd, SI->EchomailSent, SI->EchomailRcvd, SI->FilesSent, SI->FilesRcvd);
			break;
		}
		AddLogEntry(LogStr);
		

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_UTINYINT, SQL_TINYINT, 0, 0, &(SI->SessionResult), 0, NULL);
		SQLExecDirectW(hstmt, L"{call sp_SetCallSuccessStatus(?,?)}", SQL_NTS);
		SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	}
	else 
		AddLogEntry(L"Session closed");
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);

exit:
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	HeapDestroy(SI->hHeap);
	HeapFree(GetProcessHeap(), 0, param);
	_InterlockedDecrement(&(cfg.ThreadCount));
	return 0;


}