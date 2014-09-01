/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Soaron.h"

typedef struct tagTossQueue
{
	unsigned int MessageId;
	unsigned int AreaId;
	unsigned int FromLinkId;
	DwordBuffer Path;
	DwordBuffer SeenBy;
	struct tagTossQueue * lpNext;

} TossQueue,*lpTossQueue;



DWORD WINAPI EchomailTossThread(LPVOID param)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	HANDLE hHeap;
	int WaitTime;
	int result; 
	HANDLE hEvent[2];
	// 

	lpTossQueue FirstTossData,PrevTossData,CurrentTossData;

	unsigned int MessageId,AreaId,AreaId1,FromLinkId,MyAkaId;
	SQLLEN cb,cb0,cb1,cb2;
	unsigned int NumOfLinks;
	unsigned int LinkId,LinkType;
	FTNAddr LinkAddr;
	unsigned int tmp;
	unsigned char recv;
	unsigned int * LinksIdTable;
	unsigned int * LinksTypeTable;
	lpFTNAddr LinksAddrTable;
	unsigned int i,j;
	unsigned char IgnoreSeenby;
	BOOL LinksDataAllocated = FALSE;

	_InterlockedIncrement(&(cfg.ThreadCount));
	WaitTime=INFINITE;

	hEvent[0]=cfg.hExitEvent;
	hEvent[1]=cfg.hEchomailTossEvent;
	AddLogEntry(L"Echomail tossing thread started");
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
		hHeap=HeapCreate(HEAP_NO_SERIALIZE,32768,0);
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
			FirstTossData=NULL;
			PrevTossData=NULL;
			
			sqlret=SQLExecDirectW(hstmt,L"Select MessageID,AreaID,FromLinkId,recv,Path,Seenby from EchoMessages where scanned=0 order by AreaId,MessageId",SQL_NTS);
			
			if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
			{
				

				SQLBindCol(hstmt,1,SQL_C_ULONG,&MessageId,0,NULL);
				SQLBindCol(hstmt,2,SQL_C_ULONG,&AreaId,0,NULL);
				SQLBindCol(hstmt,3,SQL_C_ULONG,&FromLinkId,0,&cb0);
				SQLGetData(hstmt,4,SQL_C_BIT,&recv,0,NULL);

				sqlret=SQLFetch(hstmt);
				while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
				{
					//
					CurrentTossData=(lpTossQueue)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(TossQueue));
					if (FirstTossData==NULL)
					{
						FirstTossData=CurrentTossData;
					}
					else
					{
						PrevTossData->lpNext=CurrentTossData;
					}
				
					PrevTossData=CurrentTossData;

					CurrentTossData->MessageId=MessageId;
					CurrentTossData->AreaId=AreaId;
					CurrentTossData->FromLinkId=FromLinkId;
					
					
					//
					SQLGetData(hstmt,5,SQL_C_BINARY,&tmp,0,&cb);
					if (cb!=SQL_NULL_DATA)
					{
						AllocDwordBuffer(hHeap,&(CurrentTossData->Path),cb);
						SQLGetData(hstmt,5,SQL_C_BINARY,CurrentTossData->Path.lpBuffer,cb,NULL);

					}
					else
					{
						InitDwordBuffer(hHeap,&(CurrentTossData->Path));
					}

					SQLGetData(hstmt,6,SQL_C_BINARY,&tmp,0,&cb);
					
					if (cb!=SQL_NULL_DATA)
					{
						AllocDwordBuffer(hHeap,&(CurrentTossData->SeenBy),cb);
						SQLGetData(hstmt,6,SQL_C_BINARY,CurrentTossData->SeenBy.lpBuffer,cb,NULL);

					}
					else
					{
						InitDwordBuffer(hHeap,&(CurrentTossData->SeenBy));
								
					}

					//
						sqlret=SQLFetch(hstmt);
				}
				
				SQLFreeStmt(hstmt,SQL_UNBIND);
				SQLCloseCursor(hstmt);
					//read complete
				
				//now toss
				while (FirstTossData!=NULL)
				{
					MessageId=FirstTossData->MessageId;
					AreaId=FirstTossData->AreaId;
					FromLinkId=FirstTossData->FromLinkId;
														
					AreaId1=0; NumOfLinks=0;
					if (AreaId!=AreaId1)
					{
						AreaId1=AreaId;
						if ((NumOfLinks != 0) && (LinksDataAllocated))
						{
							HeapFree(hHeap,0,LinksAddrTable);
							HeapFree(hHeap, 0, LinksTypeTable);
							HeapFree(hHeap,0,LinksIdTable);
							LinksDataAllocated = FALSE;

						}

						SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&AreaId,0,NULL);
						SQLExecDirectW(hstmt,L"select count(links.linkid) from links,arealinks where links.linkid=arealinks.linkid and Links.PassiveLink=0 and arealinks.AllowRead=1 and arealinks.Passive=0 and arealinks.AreaId=?",SQL_NTS);
						SQLFetch(hstmt);
						SQLGetData(hstmt,1,SQL_C_ULONG,&NumOfLinks,0,NULL);
						
						SQLCloseCursor(hstmt);
						sqlret = SQLExecDirectW(hstmt, L"select UseAka,IgnoreSeenby from EchoAreas where AreaId=?", SQL_NTS);

						SQLFetch(hstmt);
						SQLGetData(hstmt, 1, SQL_C_ULONG, &MyAkaId, 0, NULL);
						SQLGetData(hstmt, 2, SQL_C_BIT, &IgnoreSeenby, 0, NULL);
						SQLCloseCursor(hstmt);


						if (NumOfLinks!=0)
						{
							LinksIdTable=(unsigned int *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(LinkId)*NumOfLinks);
							LinksTypeTable = (unsigned int *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(LinkType)*NumOfLinks);
							LinksAddrTable=(lpFTNAddr)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(FTNAddr)*NumOfLinks);
							LinksDataAllocated = TRUE;

							sqlret=SQLExecDirectW(hstmt,L"select links.linkid, links.zone,links.net,links.node,links.point,links.LinkType from links,arealinks where links.linkid=arealinks.linkid and Links.PassiveLink=0 and arealinks.AllowRead=1 and arealinks.Passive=0 and arealinks.AreaId=?",SQL_NTS);
							if ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
							{	
								SQLBindCol(hstmt,1,SQL_C_ULONG,&LinkId,0,NULL);
								SQLBindCol(hstmt,2,SQL_C_USHORT,&(LinkAddr.zone),0,NULL);
								SQLBindCol(hstmt,3,SQL_C_USHORT,&(LinkAddr.net),0,NULL);
								SQLBindCol(hstmt,4,SQL_C_USHORT,&(LinkAddr.node),0,NULL);
								SQLBindCol(hstmt,5,SQL_C_USHORT,&(LinkAddr.point),0,NULL);
								SQLBindCol(hstmt, 6, SQL_C_ULONG, &LinkType, 0, NULL);
								i=0;
								sqlret=SQLFetch(hstmt);
								while ((sqlret==SQL_SUCCESS)||(sqlret==SQL_SUCCESS_WITH_INFO))
								{
								//считать линки
									LinksIdTable[i]=LinkId;
									LinksTypeTable[i] = LinkType;
									LinksAddrTable[i].FullAddr=LinkAddr.FullAddr;
									++i;
									sqlret=SQLFetch(hstmt);
								}
								SQLFreeStmt(hstmt,SQL_UNBIND);
								SQLCloseCursor(hstmt);
							}
							
						}

						
						SQLFreeStmt(hstmt,SQL_RESET_PARAMS);

					}
					
					SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&LinkId,0,NULL);
					SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&MessageId,0,NULL);
					SQLPrepareW(hstmt,L"insert into Outbound values(?,?,0)",SQL_NTS);

					for(i=0;i<NumOfLinks;i++)
					{
						if (LinksIdTable[i]!=FromLinkId)
						{
							LinkId=LinksIdTable[i];
							if (LinksAddrTable[i].point==0)
							{
								//check path
								if (CheckInDwordBuffer(&(FirstTossData->Path), LinksAddrTable[i].net * 65536 + LinksAddrTable[i].node) == FALSE)
								{

									//check seenby
									if (CheckInDwordBuffer(&(FirstTossData->SeenBy), LinksAddrTable[i].net * 65536 + LinksAddrTable[i].node) == FALSE)
									{
										SQLExecute(hstmt);
										AddToDwordBuffer(&(FirstTossData->SeenBy), LinksAddrTable[i].net * 65536 + LinksAddrTable[i].node);
									}
									else
										if ((IgnoreSeenby != 0) || (LinksTypeTable[i] == 3)) SQLExecute(hstmt); //ignore seenby if flag for area set or ftnmtp link
								}
							}
							else 
							{//point
								
								SQLExecute(hstmt);
							}
						}
						
					}
					SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
					


					if (cfg.MyAkaTable[MyAkaId].point == 0) AddToDwordBuffer(&(FirstTossData->Path), cfg.MyAkaTable[MyAkaId].net * 65536 + cfg.MyAkaTable[MyAkaId].node);//дописываем себя в путь
					for (j = 0; j <= cfg.MaxAkaID; j++)
					{
						if ((cfg.MyAkaTable[j].FullAddr != 0) && (cfg.MyAkaTable[j].point == 0))
						if (CheckInDwordBuffer(&(FirstTossData->SeenBy), cfg.MyAkaTable[j].net * 65536 + cfg.MyAkaTable[j].node) == FALSE)
							AddToDwordBuffer(&(FirstTossData->SeenBy), cfg.MyAkaTable[j].net * 65536 + cfg.MyAkaTable[j].node);
			
						
					}

					SortDwordBuffer(&(FirstTossData->SeenBy));
					//обновить path и seenby в базе
					//
					SQLBindParameter(hstmt,1,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_LONGVARBINARY,60000,0,FirstTossData->Path.lpBuffer,0,&cb1);
					cb1=FirstTossData->Path.CurrentSize*4;
		
					SQLBindParameter(hstmt,2,SQL_PARAM_INPUT,SQL_C_BINARY,SQL_LONGVARBINARY,60000,0,FirstTossData->SeenBy.lpBuffer,0,&cb2);
					cb2=FirstTossData->SeenBy.CurrentSize*4;
					SQLBindParameter(hstmt,3,SQL_PARAM_INPUT,SQL_C_ULONG,SQL_INTEGER,0,0,&MessageId,0,NULL);
														
					SQLExecDirectW(hstmt,L"Update EchoMessages set Path=?, SeenBy=?, scanned=1 where MessageID=?",SQL_NTS);
					
					SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
					
					DwordBufferFreeMem(&(FirstTossData->SeenBy));
					DwordBufferFreeMem(&(FirstTossData->Path));
					
					CurrentTossData=FirstTossData->lpNext;
					HeapFree(hHeap,0,FirstTossData);
					FirstTossData=CurrentTossData;
				}
				
				SQLFreeStmt(hstmt,SQL_UNBIND);
				SQLCloseCursor(hstmt);
				if (LinksDataAllocated)
				{
					HeapFree(hHeap, 0, LinksAddrTable);
					HeapFree(hHeap, 0, LinksTypeTable);
					HeapFree(hHeap, 0, LinksIdTable);
					LinksDataAllocated = FALSE;
				}
				SQLExecDirectW(hstmt, L"update Outbound set Status=2 from EchoMessages where Outbound.MessageId=EchoMessages.MessageID and ISNULL(MsgId,'')=''", SQL_NTS);
			}
		}
	}
	SetEvent(cfg.hEchomailOutEvent);
	SetEvent(cfg.hMailerCallGeneratingEvent);
	goto loop;

	threadexit:
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}




