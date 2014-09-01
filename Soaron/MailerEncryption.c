/*
*
* (c) Yuri Myakotin, 2001-1014
*
* Revision 1.00 - initial release
*
*/

#include "Mailer.h"

void PrepareEncryption(HSTMT hstmt, lpMailerSessionInfo SI)
{
	unsigned char TmpBuf[160];
	unsigned long long HASH[8];
	SQLLEN cb;
	sph_keccak512_context cc;

	
	SI->EncData = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, sizeof(EncryptionData));
	
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(SI->LinkID), 0, NULL);
	SQLExecDirectW(hstmt, L"Select SessionPassword from Links where LinkID=?", SQL_NTS);
	SQLFetch(hstmt);
	SQLGetData(hstmt, 1, SQL_C_WCHAR, (TmpBuf), 128, &cb);
	SQLCloseCursor(hstmt);
	SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
	memcpy(TmpBuf + cb, SI->HASH, 32);
	sph_keccak512_init(&cc);
	sph_keccak512(&cc, TmpBuf, cb + 32);
	sph_keccak512_close(&cc, HASH);

	blowfish_initiate(&(SI->EncData->ctx), HASH, 56);
	SI->EncData->PrevDataForDecode = HASH[7];
	SI->EncData->PrevDataForEncode = HASH[7];


}

void EncryptData(lpMailerSessionInfo SI)
{
	unsigned int Size;
	unsigned int i;
	unsigned char * buf;
	ULARGE_INTEGER DataToEncrypt;
	
	
	Size = GetMailerMsgSize(SI->CurrentSendBuf);
	if ((Size % 8) != 0)
	{
		Size = ((Size / 8) + 1) * 8;
		SI->CurrentSendBuf = HeapReAlloc(SI->hHeap, HEAP_ZERO_MEMORY, SI->CurrentSendBuf, Size);
	}

	buf = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, Size + sizeof(MailerMsgHeader));
	for (i = 0; i < Size / 8; i++)
	{
		DataToEncrypt.QuadPart = ((unsigned long long *)(SI->CurrentSendBuf))[i];
		DataToEncrypt.QuadPart ^= SI->EncData->PrevDataForEncode;
		blowfish_encryptblock(&(SI->EncData->ctx), &(DataToEncrypt.HighPart), &(DataToEncrypt.LowPart));
		SI->EncData->PrevDataForEncode = DataToEncrypt.QuadPart;
		((unsigned long long *)(buf + sizeof(MailerMsgHeader)))[i] = DataToEncrypt.QuadPart;
	}
	
	SetMailerMsgHeader(buf, CMD_ENCRYPTED_INFO, Size + sizeof(MailerMsgHeader));
	HeapFree(SI->hHeap, 0, SI->CurrentSendBuf);
	SI->CurrentSendBuf = buf;
	SI->SendSize = Size + sizeof(MailerMsgHeader);
	

}

void DecryptData(lpMailerSessionInfo SI)
{
	unsigned int Size;
	unsigned int i;
	unsigned char * buf;
	unsigned char * encbuf;
	ULARGE_INTEGER DataToDecrypt;
	Size = GetMailerMsgSize(SI->CurrentRecvBuf);
	buf = HeapAlloc(SI->hHeap, HEAP_ZERO_MEMORY, Size - sizeof(MailerMsgHeader));
	encbuf = (SI->CurrentRecvBuf) + sizeof(MailerMsgHeader);
	for (i = 0; i < (Size - sizeof(MailerMsgHeader)) / 8; i++)
	{
		DataToDecrypt.QuadPart = ((unsigned long long *)encbuf)[i];
		blowfish_decryptblock(&(SI->EncData->ctx), &(DataToDecrypt.HighPart), &(DataToDecrypt.LowPart));
		DataToDecrypt.QuadPart ^= SI->EncData->PrevDataForDecode;
		SI->EncData->PrevDataForDecode = ((unsigned long long *)encbuf)[i];
		((unsigned long long *)(buf))[i] = DataToDecrypt.QuadPart;

	}
	HeapFree(SI->hHeap, 0, SI->CurrentRecvBuf);
	SI->CurrentRecvBuf = buf;
	
}