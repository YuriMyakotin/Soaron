/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"
#include "skein.h"
#include "blowfish.h"

#define PROTO_VER_HI 1
#define PROTO_VER_LO 0

//session status
// 
#define SESS_STAT_SRV_FIRST 0
#define SESS_STAT_SRV_WAITIN_LOGIN 1


#define SESS_STAT_CLIENT_WAITIN_FIRST 10	
#define SESS_STAT_CLIENT_LOGIN_SENT 11


#define SESS_STAT_LINK_OK 20
#define SESS_STAT_ERROR 200


//session result

#define SESS_RESULT_OK 0
#define SESS_RESULT_CANT_CONNECT 1
#define SESS_RESULT_LOGIN_PROBLEM 2
#define SESS_RESULT_NETWORK_ERROR 3
#define SESS_RESULT_DATA_ERROR 4


//queue item type
#define QUEUE_ITEM_COMMAND 0
#define QUEUE_ITEM_NETMAIL 1
#define QUEUE_ITEM_ECHOMAIL 2



//login commands
#define CMD_SRV_FIRST 200
#define CMD_CLIENT_FIRST 201

#define CMD_SRV_OK 203
#define CMD_SRV_INCORRECT_PWD 204
#define CMD_SRV_UNKNOWN_LINK 205
#define CMD_SRV_BUSY 206

//netmail commands
#define CMD_NETMAIL_MESSAGE 210
#define CMD_NETMAIL_MSG_RECEIVED 211
#define CMD_NETMAIL_MSG_REJECTED 212

//echomail commands
#define CMD_ECHOMAIL_CHECK 220
#define CMD_ECHOMAIL_CHECK_REPLY 221
#define CMD_ECHOMAIL_MESSAGE 222
#define CMD_ECHOMAIL_MESSAGE_RECEIDED 223
#define CMD_ECHOMAIL_BATCH_DONE 224

//file commands
#define CMD_FILE_CHECK 230
#define CMD_FILE_ACCEPT 231
#define CMD_FILE_SKIP 232
#define CMD_FILE_DELAY 233
#define CMD_FILE_PART_UNCOMPRESSED 234
#define CMD_FILE_PART_COMPRESSED 235
#define CMD_FILE_RECEIVED 236

//session finishing
#define CMD_IDLE 240
#define CMD_SESSION_CLOSE 241

//data error
#define CMD_INFO_UNPACKING_ERROR 250

//encrypted data 
#define CMD_ENCRYPTED_INFO 199


//file pack mode

#define FILE_PACK_NONE 1
#define FILE_PACK_ADAPTIVE 2
#define FILE_PACK_ALWAYS 3


//file sending status
#define FILE_SENDING_NOTHING 0
#define FILE_SENDING_STARTING 1
#define FILE_SENDING_CHECK_SENT 2
#define FILE_SENDING_IN_PROGRESS 3
#define FILE_SENDING_FINISHED 4

//file receiving status

#define FILE_RECEIVING_NOTHING 0
#define FILE_RECEIVING_IN_PROGRESS 1


__declspec(align(1)) typedef union tagMailerMsgHeader
{
	unsigned int Header;
	__declspec(align(1)) struct
	{
		unsigned char CmdCode;
		unsigned char size_low;
		unsigned char size_med;
		unsigned char size_hi;
	};
	
} MailerMsgHeader, *lpMailerMsgHeader;





__declspec(align(1)) typedef struct tagLoginHeader
{
	MailerMsgHeader HDR;
	unsigned char HASH[32];
	unsigned char ProtocolVersionHi;
	unsigned char ProtocolVersionLo;
	unsigned short NumOfAkas;
	union
	{
			unsigned int Flags;
			struct
			{
				unsigned UseEncryption : 1;
				unsigned AcceptInsecureNetmail : 2;
				unsigned reserved : 29;
			};
	};
	unsigned int PackedContentOriginalSize;
	

} LoginHeader, *lpLoginHeader;


typedef struct tagConfirmationMsg
{
	MailerMsgHeader HDR;
	unsigned int MessageID;
} ConfirmationMsg, *lpConfirmationMsg;


typedef struct tagPackedDataHeader
{
	MailerMsgHeader HDR;
	unsigned int OriginalSize;
	
} PackedDataHeader, *lpPackedDataHeader;

typedef struct tagFileCheckHeader
{
	MailerMsgHeader HDR;
	long long FileSize;
	long long LastChangeTime;

} FileCheckHeader, *lpFileCheckHeader;


typedef struct tagFileAcceptMsg
{
	MailerMsgHeader HDR;
	long long Offset;

} FileAcceptMsg, *lpFileAcceptMsg;

typedef struct tagSendQueueItem
{
	unsigned char * MsgBuf;
	struct tagSendQueueItem * NextItem;
}SendQueueItem, *lpSendQueueItem;


typedef struct tagEncryptionData
{
	blowfish_context_t ctx;
	unsigned long long PrevDataForEncode;
	unsigned long long PrevDataForDecode;
} EncryptionData, *lpEncryptionData;


typedef struct tagMailerSessionInfo
{
	SOCKET Sock;
	HANDLE hHeap;
	HANDLE hWriteEnabledEvent;
	unsigned int LinkID;
	FTNAddr LinkAddr;
	unsigned int FilePackMode;
	unsigned int FileFrameSize;

	unsigned char SessionStatus;
	unsigned char SessionResult;
	unsigned char ProtocolVersionHi;
	unsigned char ProtocolVersionLo;

	BOOL EncryptionEnabled;
	lpEncryptionData EncData;


	lpSendQueueItem CommandsQueueFirst;
	lpSendQueueItem CommandsQueueLast;
	lpSendQueueItem NetmailQueueFirst;
	lpSendQueueItem NetmailQueueLast;
	lpSendQueueItem EchomailQueueFirst;
	lpSendQueueItem EchomailQueueLast;


	unsigned char * CurrentSendBuf;
	unsigned char * CurrentRecvBuf;
	unsigned int SendSize;
	unsigned int RecvSize;
	unsigned int cbAlreadySent;
	unsigned int cbAlreadyRecv;
	
	unsigned int SendingFileStatus;
	unsigned int ReceivingFileStatus;

	HANDLE hSendingFile;
	HANDLE hReceivingFile;

	wchar_t SendingFileName[MAX_PATH];
	wchar_t ReceivingFileName[MAX_PATH];

	__int64 SendingFileSize;
	__int64 ReceivingFileSize;
	__int64 FileAlreadySentBytes;
	__int64 FileAlreadyReceivedBytes;
	
	



	
	unsigned int NetmailSent;
	unsigned int NetmailRcvd;
	unsigned int EchomailSent;
	unsigned int EchomailRcvd;
	unsigned int FilesSent;
	unsigned int FilesRcvd;

	
	unsigned int WaitingTime;
	unsigned int IdleCount;

	unsigned int HASH[32/sizeof(unsigned int)];


} MailerSessionInfo, *lpMailerSessionInfo;


typedef union
{
	struct sockaddr sa;
	struct sockaddr_in sa4;
	struct sockaddr_in6 sa6;
} universal_sa, *lp_universal_sa;




extern lpLoginHeader lpFirstDataToSend;


void AddToSendQueue(lpMailerSessionInfo SI, unsigned int Type, void * Buf);
BOOL GetNextFromSendQueue(HSTMT hstmt, lpMailerSessionInfo SI);

DWORD WINAPI MailerSessionThread(LPVOID param);




__inline unsigned int GetMailerMsgSize(LPVOID Buf)
{
	return ((lpMailerMsgHeader)Buf)->Header >> 8;
}

__inline void SetMailerMsgHeader(LPVOID Buf, unsigned char CmdCode, unsigned int MsgSize)
{
	((lpMailerMsgHeader)Buf)->Header = MsgSize << 8;
	((lpMailerMsgHeader)Buf)->CmdCode = CmdCode;
}

void MailerSendCommand(lpMailerSessionInfo SI, unsigned char CmdCode);
void MailerSendConfirmation(lpMailerSessionInfo SI, unsigned char CmdCode,unsigned int ID);
//Mailerutils

unsigned int PackData(HANDLE hHeap, char * UnpackedBuff, unsigned int UnpackedSize, char **PackedBuff, unsigned int offset);
BOOL UnpackData(HANDLE hHeap, char * PackedBuff, unsigned int PackedSize, char **UnpackedBuff, unsigned int UnpackedSize);

BOOL MailerLogRemoteInfo(lpMailerSessionInfo SI);
void GetLoginHash(HSTMT hstmt, unsigned int LinkID, unsigned char * InBuf, unsigned char * OutBuf);
unsigned int GetLinkID(HSTMT hstmt, unsigned short NumOfAkas, unsigned char * Buff);
void GetLinkInfo(HSTMT hstmt, lpMailerSessionInfo SI);
void LogNetworkError(void);
void LogFileError(wchar_t * FileName);



//MailerContentProcessing
BOOL MailerSendNetmail(HSTMT hstmt, lpMailerSessionInfo SI );
void MailerReceiveNetmail(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerReceiveNetmailConfirmation(HSTMT hstmt, lpMailerSessionInfo SI);
BOOL MailerSendEchomailCheck(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerProcessEchomailCheck(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerProcessEchomailCheckReply(HSTMT hstmt, lpMailerSessionInfo SI);
BOOL MailerSendEchomail(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerReceiveEchomail(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerReceiveEchomailConfirmation(HSTMT hstmt, lpMailerSessionInfo SI);
BOOL MailerGetNextFile(HSTMT hstmt, lpMailerSessionInfo SI);
BOOL MailerSendFileCheck(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerProcessFileCheck(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerFileCheckOk(lpMailerSessionInfo SI);
void MailerFileSendingDone(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerSendFilePart(HSTMT hstmt, lpMailerSessionInfo SI);
void MailerReceiveFilePart(HSTMT hstmt, lpMailerSessionInfo SI);

BOOL FillSendQueue(HSTMT hstmt, lpMailerSessionInfo SI);


//MailerEncryption
void PrepareEncryption(HSTMT hstmt, lpMailerSessionInfo SI);
void EncryptData(lpMailerSessionInfo SI);
void DecryptData(lpMailerSessionInfo SI);