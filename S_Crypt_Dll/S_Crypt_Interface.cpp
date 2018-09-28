// S_Crypt_Interface.cpp : 定义 DLL 应用程序的入口点。
//

#include "stdafx.h"
#include <fltUser.h>
#include <Psapi.h>
#include <atlcoll.h>

#define IOCTL_SET_PROCESS_INFO  0x00000001
#define IOCTL_SET_FILTER_INFO  0x00000002
#define IOCTL_SET_AESKEY_INFO  0x00000003
#define IOCTL_DEL_PROCESS_INFO  0x00000004

#define NPMINI_PORT_NAME       L"\\ShareMiniPort"
#define PROCESS_NAME_LENGTH 128

typedef struct _MSG_SEND_TYPE{
	ULONG uSendType ; 
	ULONG uFlag;
}MSG_SEND_TYPE,*PMSG_SEND_TYPE ;
typedef struct _PROCESS_INFO{
	ULONG    ProcessID;
	BOOLEAN bMonitor ;
	BOOLEAN bPorcessValid;
	CHAR ProcessName[PROCESS_NAME_LENGTH];
}PROCESS_INFO,*PPROCESS_INFO; 
typedef struct _MSG_SEND_SET_PROCESS_INFO{
	MSG_SEND_TYPE sSendType ;
	PROCESS_INFO sProcInfo ;
}MSG_SEND_SET_PROCESS_INFO,*PMSG_SEND_SET_PROCESS_INFO ;

#pragma comment(lib,"fltLib.lib")
#pragma comment(lib,"psapi.lib")

HANDLE g_hPort = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
    return TRUE;
}
int __stdcall  StartProtect()
{
	DWORD hResult = FilterConnectCommunicationPort( 
		NPMINI_PORT_NAME, 
		0, 
		NULL, 
		0, 
		NULL, 
		&g_hPort );

	if (hResult != S_OK) {
		return hResult;
	}

	DWORD bytesReturned = 0;
	MSG_SEND_SET_PROCESS_INFO commandMessage;


	commandMessage.sSendType.uFlag = 1;
	commandMessage.sSendType.uSendType = IOCTL_SET_FILTER_INFO;

	hResult = FilterSendMessage(
		g_hPort,
		&commandMessage,
		sizeof(MSG_SEND_SET_PROCESS_INFO),//Command = b
		NULL,
		NULL,
		&bytesReturned );

	if (hResult != S_OK) {
		return hResult;
	}

	return 0;	

}
int __stdcall  StopProtect()
{
	DWORD hResult = FilterConnectCommunicationPort( 
		NPMINI_PORT_NAME, 
		0, 
		NULL, 
		0, 
		NULL, 
		&g_hPort );

	if (hResult != S_OK) {
		return hResult;
	}

	DWORD bytesReturned = 0;
	MSG_SEND_SET_PROCESS_INFO commandMessage;

	commandMessage.sSendType.uFlag = 0;
	commandMessage.sSendType.uSendType = IOCTL_SET_FILTER_INFO;

	hResult = FilterSendMessage(
		g_hPort,
		&commandMessage,
		sizeof(MSG_SEND_SET_PROCESS_INFO),//Command = b
		NULL,
		NULL,
		&bytesReturned );

	if (hResult != S_OK) {
		return hResult;
	}

	return 0;	

}


int __stdcall  AddSecretProc(CString ProcName)
{
	
	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	MSG_SEND_SET_PROCESS_INFO commandMessage;

	strcpy(commandMessage.sProcInfo.ProcessName,ProcName.GetBuffer());
	commandMessage.sSendType.uSendType = IOCTL_SET_PROCESS_INFO;

	hResult = FilterSendMessage(
		g_hPort,
		&commandMessage,
		sizeof(MSG_SEND_SET_PROCESS_INFO),
		NULL,
		NULL,
		&bytesReturned );

	if (hResult != S_OK) {
		return hResult;
	}

	return 0;	
}


int __stdcall  DelSecretProc()
{

	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	MSG_SEND_SET_PROCESS_INFO commandMessage;
	commandMessage.sSendType.uSendType = IOCTL_DEL_PROCESS_INFO;

	hResult = FilterSendMessage(
		g_hPort,
		&commandMessage,
		sizeof(MSG_SEND_SET_PROCESS_INFO),//Command = b
		NULL,
		NULL,
		&bytesReturned );

	if (hResult != S_OK) {
		return hResult;
	}


	return 0;	

}
