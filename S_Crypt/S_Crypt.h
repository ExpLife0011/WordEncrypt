#include <fltKernel.h>
#include <Wdm.h>
#include <ntddk.h>
#include <ntifs.h>
#include <Intsafe.h>
#include <Time.h>

#include "S_File.h"
#include "S_CryptVer.h"
#include "S_GlobalFunc.h"
#include "S_Common.h"
#include "S_Ctx.h"
#include "S_Cache.h"

//#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#define MINISPY_PORT_NAME			    L"\\ShareMiniPort"
#define DELAY_ONE_MILLISECOND            1
#define DEFAULT_VOLUME_NAME_LENGTH 64
#define PROCESS_NAME_LENGTH 128

#define SYSTEM_PROCESS   1
#define NORMAL_PROCESS   0
#define EXCEL_PROCESS    2
#define POWERPNT_PROCESS 3
#define ACAD_PROCESS 4
#define DELPHI7_PROCESS 5
#define WINWORD_PROCESS    6
#define WPS_PROCESS    7
#define EXPLORER_PROCESS 8

/*************************************************************************
    Pool Tags
*************************************************************************/
#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'ppBS'

/*************************************************************************
    Local structures
*************************************************************************/
#define MIN_SECTOR_SIZE 0x200

#define FILE_EXT_LENGTH 16

//
//  This is a context structure that is used to pass state from our
//  pre-operation callback to our post-operation callback.
//
typedef struct _PRE_2_POST_CONTEXT {
    PSTREAM_CONTEXT pStreamCtx ;

    //
    //  Since the post-operation parameters always receive the "original"
    //  parameters passed to the operation, we need to pass our new destination
    //  buffer to our post operation routine so we can free it.
    //

    PVOID SwappedBuffer;

} PRE_2_POST_CONTEXT, *PPRE_2_POST_CONTEXT;

NPAGED_LOOKASIDE_LIST Pre2PostContextList;

/*************************************************************************
    Message Struct
*************************************************************************/
//App Message Struct

#define IOCTL_SET_PROCESS_INFO  0x00000001
#define IOCTL_SET_FILTER_INFO  0x00000002
#define IOCTL_SET_AESKEY_INFO  0x00000003
#define IOCTL_DEL_PROCESS_INFO  0x00000004

#define ERR_AESKEY_UNINIT           ((ULONG)0xE0000001L)

//统一的消息头uSendType消息类型，uFlag扩展标识
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

typedef struct _FILTER_PROCESS_NODE {
    LIST_ENTRY list_entry;
    PMSG_SEND_SET_PROCESS_INFO ProcessInfo;
} FILTER_PROCESS_NODE , *PFILTER_PROCESS_NODE ;

/*************************************************************************
    Prototypes
*************************************************************************/

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS
S_Unload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

VOID
CleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    );

FLT_PREOP_CALLBACK_STATUS
S_PreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );  

FLT_POSTOP_CALLBACK_STATUS
S_PostCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
S_PreWrite (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );  

FLT_POSTOP_CALLBACK_STATUS
S_PostWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
S_PreRead (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );  

FLT_POSTOP_CALLBACK_STATUS
S_PostRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
S_PostReadWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
S_PreQueryInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
S_PostQueryInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
S_PreSetInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
S_PreDirCtrl (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
S_PostDirCtrl (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PreClose (
		  __inout PFLT_CALLBACK_DATA Data,
		  __in PCFLT_RELATED_OBJECTS FltObjects,
		  __deref_out_opt PVOID *CompletionContext
		  );

FLT_POSTOP_CALLBACK_STATUS
S_PostDirCtrlWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
S_MiniMessage (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(IS_utBufferSize) PVOID IS_utBuffer,
    __in ULONG IS_utBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

NTSTATUS
S_MiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID
S_MiniDisconnect(
    __in_opt PVOID ConnectionCookie
    );

//写入统一的文件头到新建文件
NTSTATUS WriteFileFlag(
    PFILE_OBJECT FileObject ,
    PCFLT_RELATED_OBJECTS FltObjects,
    PFILE_FLAG FileFlag
    );
//读取指定长度的文件头
NTSTATUS ReadFileFlag(
    PFILE_OBJECT FileObject ,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID FileFlag,
    ULONG ReadLength
    );
//获取文件信息
NTSTATUS 
File_GetFileStandardInfoNew(
	__in  PFILE_OBJECT FileObject,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileAllocationSize,
	__in PLARGE_INTEGER FileSize,
	__in PBOOLEAN bDirectory
	);
            
//  Assign text sections for each routine.
#ifdef ALLOC_PRAGMA
	#pragma alloc_text(INIT, DriverEntry)
	#pragma alloc_text(PAGE, S_Unload)
	#pragma alloc_text(PAGE, CleanupContext)
	#pragma alloc_text(PAGE, S_PreCreate)
	#pragma alloc_text(PAGE, S_PostCreate)
	#pragma alloc_text(PAGE, S_PreWrite)
       #pragma alloc_text(PAGE, S_PostWrite)
	#pragma alloc_text(PAGE, S_PreRead)
	#pragma alloc_text(PAGE, S_PostRead)
	#pragma alloc_text(PAGE, S_PostReadWhenSafe)
	#pragma alloc_text(PAGE, S_PreQueryInfo)
	#pragma alloc_text(PAGE, S_PostQueryInfo)    
	#pragma alloc_text(PAGE, S_PreSetInfo)    
	#pragma alloc_text(PAGE, S_PreDirCtrl)    
	#pragma alloc_text(PAGE, S_PostDirCtrl)    
	#pragma alloc_text(PAGE, S_PostDirCtrlWhenSafe)    
	#pragma alloc_text(PAGE, PreClose)   
	#pragma alloc_text(PAGE, S_MiniMessage)
	#pragma alloc_text(PAGE, S_MiniConnect)
	#pragma alloc_text(PAGE, S_MiniDisconnect)
#endif    

//  operation registration
const FLT_OPERATION_REGISTRATION Callbacks[] = {
    {IRP_MJ_CREATE, FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO, 
            S_PreCreate, S_PostCreate, NULL},
    //由于在IRP_MJ_CLOSE里面清理缓存，会造成蓝屏，所以这里不不再处理PreClose
    { IRP_MJ_READ,              0, S_PreRead,      S_PostRead }, 
    { IRP_MJ_WRITE,              0, S_PreWrite,      S_PostWrite }, 
    { IRP_MJ_QUERY_INFORMATION, 0, S_PreQueryInfo, S_PostQueryInfo },
    { IRP_MJ_SET_INFORMATION,   0, S_PreSetInfo,   NULL },
    { IRP_MJ_DIRECTORY_CONTROL, 0, S_PreDirCtrl,   S_PostDirCtrl },
	{ IRP_MJ_CLOSE,   0, 	PreClose,     NULL },
    { IRP_MJ_OPERATION_END }
};

CONST FLT_CONTEXT_REGISTRATION Context_Array[] = {

       { FLT_STREAM_CONTEXT, 0, CleanupContext, STREAM_CONTEXT_SIZE,    STREAM_CONTEXT_TAG },

	{ FLT_CONTEXT_END }
};

//  This defines what we want to filter with FltMgr
const FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                               //  Flags

    Context_Array,                         //  Context
    Callbacks,                                  //  Operation callbacks

    S_Unload,                            //  MiniFilterUnload

    NULL,                                         //  InstanceSetup
    NULL,                                         //  InstanceQueryTeardown
    NULL,                                         //  InstanceTeardownStart
    NULL,                                         //  InstanceTeardownComplete

    NULL,                                         //  GenerateFileName
    NULL,                                         //  GenerateDestinationFileName
    NULL                                          //  NormalizeNameComponent

};

