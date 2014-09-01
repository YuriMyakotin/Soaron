/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#define __UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S


#include <windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <process.h>  
#include "zlib.h"


#if defined(MAINFILE)
#define SOARON_API __declspec(dllexport)
#else
#define SOARON_API __declspec(dllimport)
#endif

#define ECHOAREA_CHECK_SQLERROR -1 // - ошибка SQL, выход
#define ECHOAREA_CHECK_OK 0 // - все ок
#define ECHOAREA_CHECK_NEW_AREA_CREATED 1 //- создана новая эха
#define ECHOAREA_CHECK_WRITE_NOT_ALLOWED 2 //- пользователь не имеет права создавать эху / писать в нее.
#define ECHOAREA_CHECK_SUBSCRIBED 3 //- подписан на уже существующую эху

#define INSECURE_NETMAIL_REJECT 0
#define INSECURE_NETMAIL_ALLOW_TO_ME 1
#define INSECURE_NETMAIL_ALLOW_ALL 2

#define LINK_TYPE_BINKP 1
#define LINK_TYPE_TMAILIP 2
#define LINK_TYPE_FTNMTP 3


typedef struct {
	HANDLE hHeap;
	size_t CurrentAllocated;
	size_t CurrentSize;
	char * lpBuffer;
	} StringBuffer, *lpStringBuffer;

typedef struct {
	HANDLE hHeap;
	size_t CurrentAllocated;
	size_t CurrentSize;
	wchar_t * lpBuffer;
	} WStringBuffer, *lpWStringBuffer;

typedef struct {
	HANDLE hHeap;
	size_t CurrentAllocated;
	size_t CurrentSize;
	unsigned int * lpBuffer;
	} DwordBuffer, *lpDwordBuffer;

typedef struct {
	HANDLE hHeap;
	unsigned int CurrentAllocated;
	unsigned int CurrentSize;
	char * lpBuffer;
} MixedBuffer, *lpMixedBuffer;


__declspec(align(1)) typedef struct {
	unsigned short origNode;
	unsigned short destNode;
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short baud;
	unsigned short PktVer;
	unsigned short origNet;
	unsigned short destNet;
	unsigned char ProductCodeL;
	unsigned char RevisionH;
	char password[8];
	unsigned short origZoneQ;
	unsigned short destZoneQ;
	unsigned short auxNet;
	unsigned char cw1H;
	unsigned char cw1L;
	unsigned char ProductCodeH;
	unsigned char RevisionL;
	unsigned char cw2L;
	unsigned char cw2H;	
	unsigned short origZone;
	unsigned short destZone;
	unsigned short origPoint;
	unsigned short destPoint;
	unsigned char reserved[4];
	} PktHeader,*lpPktHeader;


__declspec(align(1)) typedef struct {
	unsigned short MsgVer;
	unsigned short origNode;
	unsigned short destNode;
	unsigned short origNet;
	unsigned short destNet;
	unsigned short attribute;
	unsigned short cost;
	} MsgHeader,*lpMsgHeader;
	
	


	
	
__declspec(align(1)) typedef union
{
	unsigned __int64 FullAddr;
	__declspec(align(1)) struct
	{
		WORD zone;
		WORD net;
		WORD node;
		WORD point;
	};
} FTNAddr, *lpFTNAddr;





__declspec(align(1)) typedef struct tagMsgHdr
{
	char FromName[36];
	char ToName[36];
	char Subj[72];
	char DateTime[20];
	unsigned short timesRead;
	unsigned short DestNode;
	unsigned short OrigNode;
	unsigned short cost;
	unsigned short OrigNet;
	unsigned short DestNet;
	unsigned short DestZone;
	unsigned short OrigZone;
	unsigned short DestPoint;
	unsigned short OrigPoint;
	unsigned short ReplyTo;
	unsigned short Attribute;
	unsigned short NextReply;
} MsgHdr,*lpMsgHdr;




typedef struct tagNetmailMessage
{
	
	unsigned int FromLinkID;
	unsigned int MessageID;
	
	wchar_t * FromName;
	wchar_t * ToName;
	FTNAddr FromAddr;
	FTNAddr ToAddr;
	SQL_TIMESTAMP_STRUCT CreateTime;
	wchar_t * Subject;
	wchar_t * MsgId;
	wchar_t * ReplyTo;
	wchar_t * MsgText;
	union
	{
		unsigned int flags;
		struct
		{
			unsigned recv:1;
			unsigned killsent:1;
			unsigned pvt:1;
			unsigned fileattach:1;
			unsigned arq:1;
			unsigned rrq:1;
			unsigned returnreq:1;
			unsigned direct:1;
			unsigned cfm:1;
			unsigned Reserved : 23;
			
		};
	};

	struct tagNetmailMessage *NextMsg;
} NetmailMessage, *lpNetmailMessage;




typedef struct tagEchoMessage
{
	wchar_t * AreaName;
	unsigned int EchoAreaID;
	unsigned int FromLinkID;
	wchar_t * FromName;
	wchar_t * ToName;
	FTNAddr FromAddr;
	SQL_TIMESTAMP_STRUCT CreateTime;
	wchar_t * Subject;
	wchar_t * MsgId;
	wchar_t * ReplyTo;
	wchar_t * MsgText;
	DwordBuffer Path;
	DwordBuffer SeenBy;
} EchomailMessage, *lpEchomailMessage;



typedef struct tagGlobalConfig
{
	FTNAddr MyAddr;
	unsigned int ThreadCount;
	SQLHENV henv;
	wchar_t ConnectionString[256];
	
	HANDLE hMainHeap;


	
	HANDLE hExitEvent;
	HANDLE hThreadEndEvent;

	HANDLE hLinksUpdateEvent;

	HANDLE hPktInEvent;
	HANDLE hNetmailOutEvent;
	
	HANDLE hEchomailTossEvent;
	HANDLE hEchomailOutEvent;
	HANDLE hSchedulerUpdateEvent;

	HANDLE hMailerCallGeneratingEvent;


	wchar_t * BadInPktDir;
	wchar_t * BadOutPktDir;
	wchar_t * BinkdPwdCfg;
	wchar_t * BinkOutboundDir;
	wchar_t * FileboxesDir;
	wchar_t * InboundDir;
	wchar_t * InsecureInPktDir;
	wchar_t * TmailPwdCfg;
	wchar_t * TmpOutboundDir;
	wchar_t * NodelistDir;
	wchar_t * UnzipCommand;
	wchar_t * ZipCommand;

	wchar_t * SoftwareName;
	wchar_t * SystemName;
	wchar_t * SysopName;
	wchar_t * SystemInfo;
	wchar_t * SystemLocation;

	unsigned int MaxAkaID;
	FTNAddr * MyAkaTable;

	unsigned int RobotsAreaID;

	//

	
} GlobalConfig, *lpGlobalConfig;

typedef BOOL (*NETMAILROBOTPROC)(lpNetmailMessage,unsigned int,BOOL); 

typedef struct tagModuleInfo
{
	wchar_t * ModuleName;
	wchar_t * ModuleFileName;
	wchar_t * ModuleEventName;
} ModuleInfo, *lpModuleInfo;



typedef struct tagNetmailOutQueue
{
	lpNetmailMessage First;
	lpNetmailMessage Last;

} NetmailOutQueue, *lpNetmailOutQueue;


//глобальные переменные
extern SOARON_API GlobalConfig cfg;

extern NETMAILROBOTPROC NetmailRobot[127];




extern CRITICAL_SECTION NetmailRouteCritSect;

//mailer global variables

extern unsigned int MailerRescanTime, MailerIdleCount,MailerIdleTimeout;
extern unsigned int DefaultFileFrameSize, FilePackMode,FileOverwrite;
extern unsigned int UseEncryption;
extern unsigned int AcceptInsecureNetmail;

extern unsigned int LogsDisableConsoleOut;


//util.c
SOARON_API wchar_t * TrimWStr(wchar_t * Str);
SOARON_API char * TrimStr(char * Str);
SOARON_API void StringBufferFreeMem(lpStringBuffer lpStrBuf);
SOARON_API void WStringBufferFreeMem(lpWStringBuffer lpStrBuf);
SOARON_API void DwordBufferFreeMem(lpDwordBuffer lpBuf);
SOARON_API void InitStringBuffer(HANDLE hHeap,lpStringBuffer lpStrBuffer);
SOARON_API void InitWStringBuffer(HANDLE hHeap,lpWStringBuffer lpStrBuffer);
SOARON_API void AllocWStringBuffer(HANDLE hHeap, lpWStringBuffer lpWStrBuffer, size_t size);
SOARON_API void InitDwordBuffer(HANDLE hHeap,lpDwordBuffer lpDWBuffer);
SOARON_API void AllocDwordBuffer(HANDLE hHeap,lpDwordBuffer lpDWBuffer,size_t Lenght);
SOARON_API void AddStrToBuffer(lpStringBuffer lpStrBuf, const char * StrToAdd);
SOARON_API void AddStr1ToBuffer(lpStringBuffer lpStrBuf, const char * lpStrToAdd,int size);
SOARON_API void AddWStrToBuffer(lpWStringBuffer lpWStrBuf, const wchar_t * WStrToAdd);
SOARON_API void AddToDwordBuffer(lpDwordBuffer lpDWBuf, unsigned int DW);
SOARON_API BOOL CheckInDwordBuffer(lpDwordBuffer lpDWBuf, unsigned int DW);
SOARON_API void SortDwordBuffer(lpDwordBuffer lpDWBuf);
SOARON_API unsigned int GetMsgIdTime(HSTMT hStmt);
SOARON_API unsigned int GetPktNumber(HSTMT hStmt);
SOARON_API unsigned int GetInt(HSTMT hStmt, wchar_t * ParamName);
SOARON_API SQL_TIMESTAMP_STRUCT GetSQLTime(void);
SOARON_API void StrToSqlDateTime(SQL_TIMESTAMP_STRUCT *result,char *str);
SOARON_API wchar_t * GetBigString(HANDLE hHeap, HSTMT hstmt,wchar_t *StrName);
SOARON_API wchar_t * GetString(HANDLE hHeap, HSTMT hstmt,wchar_t *StrName);
SOARON_API void TimeToMessageStr(char * Str, SQL_TIMESTAMP_STRUCT * st);
SOARON_API void AddLogEntry(wchar_t * Message);

SOARON_API void InitMixedBuffer(HANDLE hHeap, lpMixedBuffer lpBuff);
SOARON_API void AddToMixedBuffer(lpMixedBuffer lpBuff, void * Data, unsigned int DataSize);

SOARON_API void MixedBufferFreeMem(lpMixedBuffer lpBuff);
__inline void AddWStrToMixedBuffer(lpMixedBuffer lpBuff, const wchar_t * lpWStrToAdd)
{
	if (lpWStrToAdd != NULL) AddToMixedBuffer(lpBuff, (char *)lpWStrToAdd, (unsigned int)((wcslen(lpWStrToAdd) + 1)*sizeof(wchar_t)));
	else
	{
		wchar_t N = 0;
		AddToMixedBuffer(lpBuff, &N, sizeof(wchar_t));

	}
}

void WritePktHeader(HANDLE hFile, lpFTNAddr lpFromAka, lpFTNAddr lpToAddr, char * Pwd);
void ClosePktFile(HANDLE hFile);
wchar_t * GetFileName(wchar_t * FullName);




//netmailin.c
SOARON_API lpNetmailMessage CreateNewNetmailMsg(HANDLE hHeap, FTNAddr Addr, HSTMT hstmt);
SOARON_API void PostReplyMsg(HANDLE hHeap, lpNetmailMessage lpMsg, HSTMT hstmt, wchar_t *FromName, wchar_t *Subj, wchar_t *MsgText, unsigned char isDirect, unsigned char isKillSent);
SOARON_API void NetmailFreeMem(HANDLE hHeap, lpNetmailMessage lpNetmailMsg);
SOARON_API BOOL AddNetmailMessage(HANDLE hHeap, HSTMT hstmt, lpNetmailMessage lpMsg);

//echomailin.c

SOARON_API void PostEchomailMessage(HSTMT hstmt, unsigned int EchoAreaID, wchar_t * FromName, wchar_t * ToName, wchar_t * Subj, lpWStringBuffer lpMsgTextBuf);
void EchomailFreeMem(HANDLE hHeap, lpEchomailMessage lpEchomailMsg);
int CheckDupes(HSTMT hstmt, unsigned int EchoAreaID, wchar_t * MsgId);
void AddDupeReport(HSTMT hstmt, lpEchomailMessage lpMsg);
BOOL AddEchomailMessage(HANDLE hHeap, HSTMT hstmt, lpEchomailMessage lpMsg);
int CheckEchoArea(HANDLE hHeap, HSTMT hstmt, unsigned int FromLinkID, wchar_t * AreaName, unsigned int * lpEchoAreaID);

//PktInTosser
DWORD WINAPI PktInTosserThread(LPVOID param);

//ConfigsThread
DWORD WINAPI ConfigsThread(LPVOID param);

//NetmailOut
void GetNetmailMessages(HSTMT hstmt, HANDLE hHeap, lpNetmailOutQueue lpNMOQ);
DWORD WINAPI NetmailOutThread(LPVOID param);
void LogSessionAndSendNetmailToLink(lpFTNAddr lpLinkAddr, unsigned char SoftwareCode);

//ModuleThread
DWORD WINAPI ModuleThread(LPVOID param);

// echomail out
DWORD WINAPI EchomailTossThread(LPVOID param);
DWORD WINAPI EchomailOutThread(LPVOID param);

//external sessions info
DWORD WINAPI ExternalSessionInfoPipesServerThread(LPVOID param);

//scheduler
DWORD WINAPI SchedulerThread(LPVOID param);

//server.c

DWORD WINAPI TcpServerMainThread(LPVOID param);
DWORD WINAPI MailerCallGeneratingThread(LPVOID param);