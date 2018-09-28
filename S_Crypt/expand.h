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
	
	INT Index;//节点序号 用于内存中的识别,表头节点的这个变量记录了整个链表节点的总数
	struct _PROCESS_INFO* Next;
	CHAR ProcessName[PROCESS_NAME_LENGTH];//进程名称
	
} PROC_INFO, *PPROC_INFO;

ULONG DecideProcessNameOffsetInEPROCESS(VOID);
PCHAR GetCurrentProcessName(PCHAR ProcessName);

//CHAIN OPERATION
DWORD CreateHead();// 创建链表头
VOID InsertInfo(PROC_INFO* NodeData);//向链表中插入节点数据
VOID DeleteInfo(int Index);//从链表中删除节点数据
VOID FreeChain();//释放链表
PROC_INFO* SearchChain(INT Index);//根据某个节点的Index域，返回某个节点的地址，之后就可根据这个地址做相关标志位的修改了

//这个函数判断是否添加过这个进程
PPROC_INFO SearchByName(PUNICODE_STRING Name);//根据某个节点的Name域，返回某个节点的地址
//将链表写入文件

DWORD GetProcessNameOffset();
BOOL GetProcessName(PCHAR* ImageFileName, DWORD dwProcessNameOffset);
PROC_INFO* CompareString(PCHAR ImageFileName);

