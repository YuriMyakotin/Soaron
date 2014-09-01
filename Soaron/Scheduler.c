/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/


#include "Soaron.h"
/*
Schedule types:

1: Repeating every day, every [Period] seconds
2: Once per Day, in [Starttime]
3: Weekly, every week in [day] day of week (sunday=0)
 


*/

#define SCHEDULE_EVERY_NNNN_SECONDS 1
#define SCHEDULE_DAILY 2
#define SCHEDULE_WEEKLY 3


unsigned int NumOfTimers;
HANDLE TimerQueue;

LPWSTR * EventNames;


VOID CALLBACK TimerCallbackProc(PVOID lpParameter,BOOLEAN TimerOrWaitFired)
{
	HANDLE hEvent;
	hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, EventNames[(unsigned int)lpParameter]);
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}

}




void CreateTimers(void)
{
	SQLHDBC   hdbc;
	SQLHSTMT  hstmt;
	SQLRETURN sqlret;
	SQLLEN cb;
	wchar_t tmp;
	unsigned char SchedType, Day;
	unsigned int Period;
	SQL_TIME_STRUCT Starttime;
	int timeval;
	unsigned int i;
	SYSTEMTIME st;
	HANDLE hTimer;
	
	SQLAllocHandle(SQL_HANDLE_DBC, cfg.henv, &hdbc);
	sqlret = SQLDriverConnectW(hdbc, NULL, cfg.ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if (sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO)
	{

		SetEvent(cfg.hExitEvent);
		return;
		//fatal error
	}
	SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	SQLExecDirectW(hstmt, L"Select count(*) from Schedule where Enabled<>0", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_ULONG, &NumOfTimers, 0, NULL);
	SQLCloseCursor(hstmt);
	if (NumOfTimers != 0)
	{
		GetLocalTime(&st);
		TimerQueue = CreateTimerQueue();
		EventNames = HeapAlloc(cfg.hMainHeap, HEAP_ZERO_MEMORY, NumOfTimers*sizeof(LPWSTR));

		i = 0;
		SQLExecDirectW(hstmt, L"Select ScheduleType,Day,Starttime,Period,EventObjectName from Schedule where Enabled<>0", SQL_NTS);
		SQLBindCol(hstmt, 1, SQL_C_UTINYINT, &SchedType, 0, NULL);
		SQLBindCol(hstmt, 2, SQL_C_UTINYINT, &Day, 0, NULL);
		SQLBindCol(hstmt, 3, SQL_C_TIME, &Starttime, 0, NULL);
		SQLBindCol(hstmt, 4, SQL_C_ULONG, &Period, 0, NULL);
		sqlret = SQLFetch(hstmt);
		while ((sqlret == SQL_SUCCESS) || (sqlret == SQL_SUCCESS_WITH_INFO))
		{
			//
			SQLGetData(hstmt, 5, SQL_C_WCHAR, &tmp, 0, &cb);
			EventNames[i] = HeapAlloc(cfg.hMainHeap, 0, cb);
			SQLGetData(hstmt, 5, SQL_C_WCHAR, EventNames[i], 512, NULL);
			switch (SchedType)
			{
			case SCHEDULE_EVERY_NNNN_SECONDS:
				timeval = ((Starttime.hour * 3600 + Starttime.minute * 60 + Starttime.second) % Period) - ((st.wHour * 3600 + st.wMinute * 60 + st.wSecond) % Period);
				if (timeval < 0) timeval += Period;
				break;
			case SCHEDULE_DAILY:
				Period = 86400;
				timeval = (Starttime.hour * 3600 + Starttime.minute * 60 + Starttime.second) - (st.wHour * 3600 + st.wMinute * 60 + st.wSecond);
				if (timeval < 0) timeval += Period;
				break;
			case SCHEDULE_WEEKLY:
				Period = 604800;
				Day %= 7;
				timeval = (Day * 86400 + Starttime.hour * 3600 + Starttime.minute * 60 + Starttime.second) - (st.wDayOfWeek * 86400+st.wHour * 3600 + st.wMinute * 60 + st.wSecond);
				if (timeval < 0) timeval += Period;
				break;

			default: goto next; //invalid values
			}
			CreateTimerQueueTimer(&hTimer, TimerQueue, (WAITORTIMERCALLBACK)TimerCallbackProc, (PVOID)i, timeval * 1000, Period * 1000, WT_EXECUTEINTIMERTHREAD);

			//
			next:
			sqlret = SQLFetch(hstmt);
			++i;
		}
		SQLCloseCursor(hstmt);
		SQLFreeStmt(hstmt, SQL_UNBIND);
	}
	else TimerQueue = INVALID_HANDLE_VALUE;
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
}

void DeleteTimers(void)
{
	if (TimerQueue != INVALID_HANDLE_VALUE)
	{

		DeleteTimerQueueEx(TimerQueue, NULL);
		for (unsigned int i = 0; i < NumOfTimers; i++) HeapFree(cfg.hMainHeap, 0, EventNames[i]);
		HeapFree(cfg.hMainHeap, 0, EventNames);

	}
}

DWORD WINAPI SchedulerThread(LPVOID param)
{
	int result;

	HANDLE hEvent[2];
	_InterlockedIncrement(&(cfg.ThreadCount));


	hEvent[0] = cfg.hExitEvent;

	hEvent[1] = cfg.hSchedulerUpdateEvent;
	AddLogEntry(L"Scheduler thread started");
	CreateTimers();

loop:
	result = WaitForMultipleObjects(2, hEvent, FALSE, INFINITE);
	switch (result)
	{
	case (WAIT_OBJECT_0) :
	{
		DeleteTimers();
		goto threadexit;
	}

	case (WAIT_OBJECT_0 + 1) :
	{
		DeleteTimers();
		CreateTimers();
		goto loop;
	}

	}


threadexit:
	_InterlockedDecrement(&(cfg.ThreadCount));
	SetEvent(cfg.hThreadEndEvent);
	return 0;
}