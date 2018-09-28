#include <ntddk.h>
#include <tdikrnl.h>
#include <ntstrsafe.h>
#include <WinDef.h>
#include <tchar.h>

#define NT_PROCNAMELEN  16

#define STATUS_DELETE_FAILED 1
#define STATUS_CHAIN_FAILED 2
#define STATUS_FILE_FAILED 3


#define STATUS_CREATECHAINHEAD_FAILED 0x00000100
#define STATUS_CHAINHEAD_NULL 0x00000101
#define STATUS_DELCHAIN_FAILED 0x00000102
#define STATUS_CHAINTABLE_NULL 0x00000103

#define PROCESS_NAME_LENGTH 128
typedef struct _PROC_INFO {
	
	INT Index;//�ڵ���� �����ڴ��е�ʶ��,��ͷ�ڵ�����������¼����������ڵ������
	struct _PROCESS_INFO* Next;
	CHAR ProcessName[PROCESS_NAME_LENGTH];//��������
	
} PROC_INFO, *PPROC_INFO;

ULONG DecideProcessNameOffsetInEPROCESS(VOID);
PCHAR GetCurrentProcessName(PCHAR ProcessName);

//CHAIN OPERATION
DWORD CreateHead();// ��������ͷ
VOID InsertInfo(PROC_INFO* NodeData);//�������в���ڵ�����
VOID DeleteInfo(int Index);//��������ɾ���ڵ�����
VOID FreeChain();//�ͷ�����
PROC_INFO* SearchChain(INT Index);//����ĳ���ڵ��Index�򣬷���ĳ���ڵ�ĵ�ַ��֮��Ϳɸ��������ַ����ر�־λ���޸���

//��������ж��Ƿ���ӹ��������
PPROC_INFO SearchByName(PUNICODE_STRING Name);//����ĳ���ڵ��Name�򣬷���ĳ���ڵ�ĵ�ַ
//������д���ļ�

DWORD GetProcessNameOffset();
BOOL GetProcessName(PCHAR* ImageFileName, DWORD dwProcessNameOffset);
PROC_INFO* CompareString(PCHAR ImageFileName);

