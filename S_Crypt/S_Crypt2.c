#include "S_Crypt.h"
#include "expand.h"
//---------------------------------------------------------------------------
//变量区
//---------------------------------------------------------------------------

#define FILEFLAG_POOL_TAG1 'AAAA'
#define FILEFLAG_POOL_TAG2 'BBBB'
extern ULONG ProcessNameOffset;
extern PPROC_INFO ChainHead;

PFLT_PORT 	gServerPort;
PFLT_PORT     gClientPort;

PFLT_FILTER gFilterHandle;

LIST_ENTRY g_ProcessListHead ;
KSPIN_LOCK g_ProcessListLock ;

//调用驱动的应用进程结构
PEPROCESS UserProcess;
BOOLEAN CanFilter = FALSE;

//LIST_ENTRY s_List_SecProc;
//KSPIN_LOCK s_Lock;

//--------------------------------------------------------------------------
//实现区PsLookupProcessByProcessId
//--------------------------------------------------------------------------
KSPIN_LOCK	g_SpinLockForWriteFileFlag;
NTSTATUS WriteFileFlag(
    PFILE_OBJECT FileObject ,
    PCFLT_RELATED_OBJECTS FltObjects,
    PFILE_FLAG FileFlag
    )
{
    LARGE_INTEGER ByteOffset = {0} ;
    ULONG BytesWritten = 0 ;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	//KIRQL irql;
	//KeAcquireSpinLock(&g_SpinLockForWriteFileFlag, &irql);

    if (NULL == FileFlag)
        return status;

	try 
	{
		RtlZeroMemory(FileFlag,FILE_FLAG_LENGTH);
		//g_psFileFlag为初始化的通用头部，这里可以自己定义一些头部的值
		RtlCopyMemory(FileFlag, g_psFileFlag, FILE_FLAG_LENGTH);
		FileFlag->FileFlagInfo.CryptIndex = CRYPT_INDEX_BIT;
	     
		ByteOffset.QuadPart = 0;
		status = File_ReadWriteFile(
        		IRP_MJ_WRITE, 
        		FltObjects->Instance, 
        		FileObject, 
        		&ByteOffset, 
        		FILE_FLAG_LENGTH, 
        		FileFlag, 
        		&BytesWritten,
        		FLTFL_IO_OPERATION_NON_CACHED|FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET) ;
	}finally {
		//if (NULL != FileFlag)
			//ExFreePool(FileFlag);
	}
	//KeReleaseSpinLock(&g_SpinLockForWriteFileFlag, irql);
    return status;
}

KSPIN_LOCK	g_SpinLockForReadFileFlag;
NTSTATUS ReadFileFlag(
    PFILE_OBJECT FileObject ,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID FileFlag,
    ULONG ReadLength
    )
{
    LARGE_INTEGER ByteOffset = {0} ;
    ULONG BytesRead = 0 ;
     NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	//KIRQL irql;
	//KeAcquireSpinLock(&g_SpinLockForReadFileFlag, &irql);

    if (NULL == FileFlag)
        return STATUS_UNSUCCESSFUL;

	try 
	{
		ByteOffset.QuadPart = 0;
		status = File_ReadWriteFile(IRP_MJ_READ, 
				  FltObjects->Instance, 
				  FileObject, 
				  &ByteOffset, 
				  ReadLength, 
				  FileFlag, 
				  &BytesRead,
				  FLTFL_IO_OPERATION_NON_CACHED|FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET) ;
	}finally {
		//if (NULL != FileFlag)
			//ExFreePool(FileFlag);
	}
	//KeReleaseSpinLock(&g_SpinLockForReadFileFlag, irql);
    return status;
}

NTSTATUS UpdateFileFlag(
    __in PFLT_CALLBACK_DATA Data,
    PFILE_OBJECT FileObject ,
    PCFLT_RELATED_OBJECTS FltObjects,
    PVOID WriteBuf,
    ULONG Offset,
    ULONG WriteLen
    )
{
    LARGE_INTEGER ByteOffset = {0} ;
    ULONG BytesWritten = 0 ;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PFILE_FLAG pFileFlag;
    PCHAR p;

    if (NULL == WriteBuf)
        return status;
    __try 
    {
        pFileFlag = (PFILE_FLAG)ExAllocatePoolWithTag(NonPagedPool, FILE_FLAG_LENGTH, NAME_TAG);
        status = ReadFileFlag(FileObject,FltObjects,pFileFlag,FILE_FLAG_LENGTH);
        if (!NT_SUCCESS(status))
            leave;

        p = pFileFlag;
        p += Offset;
        if (RtlCompareMemory(p, WriteBuf, WriteLen) != WriteLen)
        {
            RtlCopyMemory(p, WriteBuf, WriteLen);

            ByteOffset.QuadPart = 0;
            status = File_ReadWriteFile(
                	IRP_MJ_WRITE, 
                	FltObjects->Instance, 
                	FileObject, 
                	&ByteOffset, 
                	FILE_FLAG_LENGTH, 
                	pFileFlag, 
                	&BytesWritten,
                	FLTFL_IO_OPERATION_NON_CACHED|FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET) ;
        } else {
            status = STATUS_SUCCESS;
        }
    }__finally {
        if (NULL != pFileFlag)
            ExFreePool(pFileFlag);
    }
    return status;
}

NTSTATUS 
File_GetFileStandardInfoNew(
	__in  PFILE_OBJECT FileObject,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileAllocationSize,
	__in PLARGE_INTEGER FileSize,
	__in PBOOLEAN bDirectory
	)
{
	NTSTATUS status = STATUS_SUCCESS ;
	FILE_STANDARD_INFORMATION sFileStandardInfo ;

	//修改为向下层Call
	status = FltQueryInformationFile(FltObjects->Instance,
									 FileObject,
									 &sFileStandardInfo,
									 sizeof(FILE_STANDARD_INFORMATION),
									 FileStandardInformation,
									 NULL
									 ) ;
	if (NT_SUCCESS(status))
	{
		if (NULL != FileSize)
			*FileSize = sFileStandardInfo.EndOfFile ;
		if (NULL != FileAllocationSize)
			*FileAllocationSize = sFileStandardInfo.AllocationSize ;
		if (NULL != bDirectory)
			*bDirectory = sFileStandardInfo.Directory ;
	}

	return status ;
}

//初始化链表
// void S_InitList()
// {
//     //初始化链表
//     InitializeListHead(&s_List_SecProc);
//     KeInitializeSpinLock(&s_Lock);
// }

//锁定消息链表
// void S_ProcListLock(PKIRQL OldIRQL)
// {
//     if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
//         KeAcquireSpinLock(&s_Lock, OldIRQL);
//     else
//         KeAcquireSpinLockAtDpcLevel(&s_Lock);
// }
// 
// //解除消息链表锁定
// void S_ProcListUnLock(KIRQL OldIRQL)
// {
//     if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
//         KeReleaseSpinLock(&s_Lock, OldIRQL);
//     else
//         KeReleaseSpinLockFromDpcLevel(&s_Lock);
// }
// 
// //清除进程信息列表
// void S_ClearProcList(PLIST_ENTRY ClearList)
// {
//     PPROCESS_INFO pListProcInfo;
//     PFILTER_PROCESS_NODE ProcNode;
//     KIRQL OldIRQL;
// 
//     S_ProcListLock(&OldIRQL);
//     try {
//         while (!IsListEmpty(ClearList)) {
//             ProcNode = (PFILTER_PROCESS_NODE)RemoveTailList(ClearList);
// 
//             if (ProcNode->ProcessInfo !=NULL) {
//                 pListProcInfo = (PPROCESS_INFO)ProcNode->ProcessInfo;
// 
//                 ExFreePoolWithTag(pListProcInfo,NAME_TAG);
//                 ExFreePoolWithTag(ProcNode,NAME_TAG);
//             }
//         }
//     } finally {
//         S_ProcListUnLock(OldIRQL);
//     }
// }



typedef struct _iPROCESS_INFO{
	CHAR    szProcessName[256] ;
	BOOLEAN bMonitor ;
	WCHAR   wsszRelatedExt[64][6] ; /*< related file extension, containing maximum 10 extensions and each length is 6 characters */
	LIST_ENTRY ProcessList ;
}iPROCESS_INFO,*PiPROCESS_INFO ;

#define MGAPI_RESULT_SUCCESS        0x00000000
#define MGAPI_RESULT_ALREADY_EXIST  0x00000001
#define MGAPI_RESULT_INTERNEL_ERROR 0x00000002

static BOOLEAN Psi_SearchForSpecifiedProcessInList(PUCHAR pszProcessName, BOOLEAN bRemove)
{
	BOOLEAN bRet = TRUE ;
	KIRQL oldIrql ;
	PLIST_ENTRY TmpListEntryPtr = NULL ;
	PiPROCESS_INFO psProcessInfo = NULL ;

	try{

		TmpListEntryPtr = g_ProcessListHead.Flink ;
		while(&g_ProcessListHead != TmpListEntryPtr)
		{
			psProcessInfo = CONTAINING_RECORD(TmpListEntryPtr, iPROCESS_INFO, ProcessList) ;

			if (!_strnicmp(psProcessInfo->szProcessName, pszProcessName, strlen(pszProcessName)))
			{
				bRet = TRUE;
				if (bRemove)
				{
					KeAcquireSpinLock(&g_ProcessListLock, &oldIrql) ;
					RemoveEntryList(&psProcessInfo->ProcessList) ;
					KeReleaseSpinLock(&g_ProcessListLock, oldIrql) ;
					ExFreePool(psProcessInfo) ;
					psProcessInfo = NULL ;
				}
				__leave ;
			}

			TmpListEntryPtr = TmpListEntryPtr->Flink ;
		}

		bRet = FALSE ;
	}
	finally{
		/**/
		//Todo some post work here
	}

	return bRet ;
}

/*
 * 在进程链表中添加新进程的信息，并根据参数二设置启用状态，如果已经存在该进程的信息，则放弃处理。
 */
ULONG Psi_AddProcessInfo(PUCHAR pszProcessName, BOOLEAN bMonitor)
{
	ULONG uRes = MGAPI_RESULT_SUCCESS ;
	PiPROCESS_INFO psProcInfo = NULL ;
	BOOLEAN bRet ;

	try{
		if (NULL == pszProcessName)
		{
			uRes = MGAPI_RESULT_INTERNEL_ERROR ;
			_leave ;
		}

		/**
		* search for process name, if exists, donnot insert again
		*/
		bRet = Psi_SearchForSpecifiedProcessInList(pszProcessName, FALSE) ;
		if (bRet)
		{
			uRes = MGAPI_RESULT_ALREADY_EXIST ;
			_leave ;
		}

		/**
		* allocate process info structure
		*/
		psProcInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(iPROCESS_INFO), 'ipws') ;
		if (NULL == psProcInfo)
		{
			uRes = MGAPI_RESULT_INTERNEL_ERROR ;
			_leave ;
		}

		RtlZeroMemory(psProcInfo, sizeof(iPROCESS_INFO)) ;

		/**
		* initialize process info and insert it into global process list
		*/
		if (!_strnicmp(pszProcessName, "winword.exe", strlen("winword.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".html") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".txt") ;
			wcscpy(psProcInfo->wsszRelatedExt[2], L".mh_") ; //relative to .mht and .mhtml extension
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".rtf") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".ht_") ; //relative to .htm and .html extension
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".xml") ;
			wcscpy(psProcInfo->wsszRelatedExt[6],  L".mht") ;
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".mhtml") ;
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".htm") ;
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".dot") ;
			wcscpy(psProcInfo->wsszRelatedExt[10],  L".tmp") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".docm") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".docx") ;
			wcscpy(psProcInfo->wsszRelatedExt[13],  L".doc") ;
		}
		else if (!_strnicmp(pszProcessName, "notepad.exe", strlen("notepad.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".txt") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".log") ;
		}
		else if (!_strnicmp(pszProcessName, "et.exe", strlen("et.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0], L".et") ;
			wcscpy(psProcInfo->wsszRelatedExt[1], L".ett") ;	
			wcscpy(psProcInfo->wsszRelatedExt[2], L".xls") ;
			wcscpy(psProcInfo->wsszRelatedExt[3], L".xlt") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".txt") ;
			wcscpy(psProcInfo->wsszRelatedExt[5], L".csv") ;
			wcscpy(psProcInfo->wsszRelatedExt[6], L".dbf") ;

		}
		else if (!_strnicmp(pszProcessName, "excel.exe", strlen("excel.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".xls") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".xml") ;
			wcscpy(psProcInfo->wsszRelatedExt[2],  L".mht") ;
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".mhtml") ;
			wcscpy(psProcInfo->wsszRelatedExt[4],  L".htm") ;
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".html") ;
			wcscpy(psProcInfo->wsszRelatedExt[6],  L".mh_") ; //relative to .mht extension
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".ht_") ; //relative to .htm and .html extension
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".xlt") ;
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".txt") ;
			wcscpy(psProcInfo->wsszRelatedExt[10], L".tmp") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".xlsx") ;
		}
		else if (!_strnicmp(pszProcessName, "powerpnt.exe", strlen("powerpnt.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".ppt") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".tmp") ;
			wcscpy(psProcInfo->wsszRelatedExt[2],  L".rtf") ;
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".pot") ;
			wcscpy(psProcInfo->wsszRelatedExt[4],  L".ppsm") ;
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".mht") ;
			wcscpy(psProcInfo->wsszRelatedExt[6],  L".mhtml") ;
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".htm") ;
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".html") ;	
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".pps") ;
			wcscpy(psProcInfo->wsszRelatedExt[10], L".ppa") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".pptx") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".pptm") ;
			wcscpy(psProcInfo->wsszRelatedExt[13], L".potx") ;
			wcscpy(psProcInfo->wsszRelatedExt[14], L".potm") ;
			wcscpy(psProcInfo->wsszRelatedExt[15], L".ppsx") ;
			wcscpy(psProcInfo->wsszRelatedExt[16], L".mh_") ; //relative to .mht and .mhtml extension
			wcscpy(psProcInfo->wsszRelatedExt[17], L".ht_") ; //relative to .htm and .html extension	
			wcscpy(psProcInfo->wsszRelatedExt[18], L".pptx") ;
		}
		else if (!_strnicmp(pszProcessName, "wmplayer.exe", strlen("wmplayer.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".mid") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".rmi") ;
			wcscpy(psProcInfo->wsszRelatedExt[2],  L".midi") ;
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".asf") ;
			wcscpy(psProcInfo->wsszRelatedExt[4],  L".wm") ;
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".wma") ;
			wcscpy(psProcInfo->wsszRelatedExt[6],  L".wmv") ;
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".avi") ;
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".wav") ;
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".mpg") ;
			wcscpy(psProcInfo->wsszRelatedExt[10], L".mpeg") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".mp2") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".mp3") ;
		}
		else if (!_strnicmp(pszProcessName, "explorer.exe", strlen("explorer.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0], L".mp3") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],L".mp2") ;	
			wcscpy(psProcInfo->wsszRelatedExt[2], L".xml") ;
			wcscpy(psProcInfo->wsszRelatedExt[3], L".mht") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".mhtml") ;
			wcscpy(psProcInfo->wsszRelatedExt[5], L".htm") ;
			wcscpy(psProcInfo->wsszRelatedExt[6], L".html") ;	
			wcscpy(psProcInfo->wsszRelatedExt[7], L".xlt") ;
			wcscpy(psProcInfo->wsszRelatedExt[8], L".mid") ;
			wcscpy(psProcInfo->wsszRelatedExt[9], L".rmi") ;
			wcscpy(psProcInfo->wsszRelatedExt[10],L".midi") ;
			wcscpy(psProcInfo->wsszRelatedExt[11],L".asf") ;
			wcscpy(psProcInfo->wsszRelatedExt[12],L".wm") ;
			wcscpy(psProcInfo->wsszRelatedExt[13],L".wma") ;
			wcscpy(psProcInfo->wsszRelatedExt[14],L".wmv") ;
			wcscpy(psProcInfo->wsszRelatedExt[15],L".avi") ;
			wcscpy(psProcInfo->wsszRelatedExt[16],L".wav") ;
			wcscpy(psProcInfo->wsszRelatedExt[17],L".mpg") ;
			wcscpy(psProcInfo->wsszRelatedExt[18],L".mpeg") ;
			wcscpy(psProcInfo->wsszRelatedExt[19], L".xls") ;
			wcscpy(psProcInfo->wsszRelatedExt[20], L".ppt") ;
		}
		else if (!_strnicmp(pszProcessName, "wps.exe", strlen("wps.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0], L".wps") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],L".mpt") ;	
			wcscpy(psProcInfo->wsszRelatedExt[2], L".doc") ;
			wcscpy(psProcInfo->wsszRelatedExt[3], L".dot") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".txt") ;
		}
		else if (!_strnicmp(pszProcessName, "wpp.exe", strlen("wpp.exe")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0], L".dps") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],L".dpt") ;	
			wcscpy(psProcInfo->wsszRelatedExt[2], L".ppt") ;
			wcscpy(psProcInfo->wsszRelatedExt[3], L".pot") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".pps") ;
		}
		else if (!_strnicmp(pszProcessName, "System", strlen("System")))
		{
			wcscpy(psProcInfo->wsszRelatedExt[0],  L".doc") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".xls") ;
			wcscpy(psProcInfo->wsszRelatedExt[2],  L".ppt") ;
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".txt") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".mp2") ;
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".rtf") ;
			wcscpy(psProcInfo->wsszRelatedExt[6], L".mp3") ;
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".xml") ;
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".mht") ;
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".mhtml") ;
			wcscpy(psProcInfo->wsszRelatedExt[10], L".htm") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".html") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".docx") ;
			wcscpy(psProcInfo->wsszRelatedExt[13], L".docm") ;	
			wcscpy(psProcInfo->wsszRelatedExt[14], L".pps") ;
			wcscpy(psProcInfo->wsszRelatedExt[15], L".ppa") ;
			wcscpy(psProcInfo->wsszRelatedExt[16], L".pptx") ;
			wcscpy(psProcInfo->wsszRelatedExt[17], L".pptm") ;
			wcscpy(psProcInfo->wsszRelatedExt[18], L".potx") ;
			wcscpy(psProcInfo->wsszRelatedExt[19], L".potm") ;
			wcscpy(psProcInfo->wsszRelatedExt[20], L".ppsx") ;	
			wcscpy(psProcInfo->wsszRelatedExt[21], L".pot") ;
			wcscpy(psProcInfo->wsszRelatedExt[22], L".ppsm") ;	
			wcscpy(psProcInfo->wsszRelatedExt[23], L".mh_") ; //relative to .mht extension
			wcscpy(psProcInfo->wsszRelatedExt[24], L".ht_") ; //relative to .htm and .html extension
			wcscpy(psProcInfo->wsszRelatedExt[25], L".xlt") ;
			wcscpy(psProcInfo->wsszRelatedExt[26], L".mid") ;
			wcscpy(psProcInfo->wsszRelatedExt[27], L".rmi") ;
			wcscpy(psProcInfo->wsszRelatedExt[28], L".midi") ;
			wcscpy(psProcInfo->wsszRelatedExt[29], L".asf") ;
			wcscpy(psProcInfo->wsszRelatedExt[30], L".wm") ;
			wcscpy(psProcInfo->wsszRelatedExt[31], L".wma") ;
			wcscpy(psProcInfo->wsszRelatedExt[32], L".wmv") ;
			wcscpy(psProcInfo->wsszRelatedExt[33], L".avi") ;
			wcscpy(psProcInfo->wsszRelatedExt[34], L".wav") ;
			wcscpy(psProcInfo->wsszRelatedExt[35], L".mpg") ;
			wcscpy(psProcInfo->wsszRelatedExt[36], L".mpeg");			
			wcscpy(psProcInfo->wsszRelatedExt[37],  L".tmp") ;
			wcscpy(psProcInfo->wsszRelatedExt[38],  L".dot") ;

		}
		else if (!_strnicmp(pszProcessName, "photoshop.exe", strlen("photoshop.exe")))
		{

			wcscpy(psProcInfo->wsszRelatedExt[0],  L".psd") ;
			wcscpy(psProcInfo->wsszRelatedExt[1],  L".pdd") ;
			wcscpy(psProcInfo->wsszRelatedExt[2],  L".bmp") ;
			wcscpy(psProcInfo->wsszRelatedExt[3],  L".rle") ;
			wcscpy(psProcInfo->wsszRelatedExt[4], L".dib") ;
			wcscpy(psProcInfo->wsszRelatedExt[5],  L".tif") ;
			wcscpy(psProcInfo->wsszRelatedExt[6], L".crw") ;
			wcscpy(psProcInfo->wsszRelatedExt[7],  L".nef") ;
			wcscpy(psProcInfo->wsszRelatedExt[8],  L".raf") ;
			wcscpy(psProcInfo->wsszRelatedExt[9],  L".orf") ;
			wcscpy(psProcInfo->wsszRelatedExt[10], L".mrw") ;
			wcscpy(psProcInfo->wsszRelatedExt[11], L".dcr") ;
			wcscpy(psProcInfo->wsszRelatedExt[12], L".mos") ;
			wcscpy(psProcInfo->wsszRelatedExt[13], L".raw") ;	
			wcscpy(psProcInfo->wsszRelatedExt[14], L".pef") ;
			wcscpy(psProcInfo->wsszRelatedExt[15], L".srf") ;
			wcscpy(psProcInfo->wsszRelatedExt[16], L".dng") ;
			wcscpy(psProcInfo->wsszRelatedExt[17], L".x3f") ;
			wcscpy(psProcInfo->wsszRelatedExt[18], L".cr2") ;
			wcscpy(psProcInfo->wsszRelatedExt[19], L".erf") ;
			wcscpy(psProcInfo->wsszRelatedExt[20], L".sr2") ;	
			wcscpy(psProcInfo->wsszRelatedExt[21], L".kdc") ;
			wcscpy(psProcInfo->wsszRelatedExt[22], L".mfw") ;	
			wcscpy(psProcInfo->wsszRelatedExt[23], L".mef") ; //relative to .mht extension
			wcscpy(psProcInfo->wsszRelatedExt[24], L".arw") ; //relative to .htm and .html extension
			wcscpy(psProcInfo->wsszRelatedExt[25], L".dcm") ;
			wcscpy(psProcInfo->wsszRelatedExt[26], L".dc3") ;
			wcscpy(psProcInfo->wsszRelatedExt[27], L".dic") ;
			wcscpy(psProcInfo->wsszRelatedExt[28], L".eps") ;
			wcscpy(psProcInfo->wsszRelatedExt[29], L".jpg") ;
			wcscpy(psProcInfo->wsszRelatedExt[30], L".jpeg") ;
			wcscpy(psProcInfo->wsszRelatedExt[31], L".jpe") ;
			wcscpy(psProcInfo->wsszRelatedExt[32], L".pdf") ;
			wcscpy(psProcInfo->wsszRelatedExt[33], L".pdp") ;
			wcscpy(psProcInfo->wsszRelatedExt[34], L".raw") ;
			wcscpy(psProcInfo->wsszRelatedExt[35], L".pct") ;
			wcscpy(psProcInfo->wsszRelatedExt[36], L".pict");			
			wcscpy(psProcInfo->wsszRelatedExt[37], L".mov") ;
			wcscpy(psProcInfo->wsszRelatedExt[38], L".avi") ;
			wcscpy(psProcInfo->wsszRelatedExt[39], L".mpg") ;
			wcscpy(psProcInfo->wsszRelatedExt[40], L".mpeg") ;
			wcscpy(psProcInfo->wsszRelatedExt[41], L".mp4") ;
			wcscpy(psProcInfo->wsszRelatedExt[42], L".m4v") ;
			wcscpy(psProcInfo->wsszRelatedExt[43], L".sct") ;
			wcscpy(psProcInfo->wsszRelatedExt[44], L".psb") ;

		}
		psProcInfo->bMonitor = bMonitor ;
		RtlCopyMemory(psProcInfo->szProcessName, pszProcessName, strlen(pszProcessName)) ;

		//将新建进程信息添加到进程链表
		ExInterlockedInsertTailList(&g_ProcessListHead, &psProcInfo->ProcessList, &g_ProcessListLock) ;
	}
	finally{
	}

	return uRes ;
}


/*

BOOLEAN OriginalPs_IsCurrentProcessMonitored(WCHAR* pwszFilePathName, ULONG uLength, BOOLEAN* bIsSystemProcess, BOOLEAN* bIsPPTFile)
{
	BOOLEAN bRet = TRUE ;
	UCHAR szProcessName[16] = {0} ;
	PLIST_ENTRY TmpListEntryPtr = NULL ;
	PiPROCESS_INFO psProcessInfo = NULL ;
	WCHAR wszFilePathName[MAX_PATH] = {0} ;
	WCHAR* pwszExt = NULL ;

	try{
		// get current process name. 
		GetCurrentProcessName(szProcessName);



		// save file path name in local buffer
		RtlCopyMemory(wszFilePathName, pwszFilePathName, uLength*sizeof(WCHAR)) ;
		

		// recognize process name and return to caller
		if (bIsSystemProcess != NULL)
		{
			if ((strlen(szProcessName) == strlen("explorer.exe")) && !_strnicmp(szProcessName, "explorer.exe", strlen("explorer.exe")))
			{
				*bIsSystemProcess = SYSTEM_PROCESS ;
			}
			else
			{
				*bIsSystemProcess = NORMAL_PROCESS ;
				if ((strlen(szProcessName) == strlen("excel.exe")) && !_strnicmp(szProcessName, "excel.exe", strlen("excel.exe")))
				{
					*bIsSystemProcess = EXCEL_PROCESS ;
#ifdef DBG
					DbgPrint("szProcessName:%s\n", szProcessName);
#endif
				}
				if ((strlen(szProcessName) == strlen("powerpnt.exe")) && !_strnicmp(szProcessName, "powerpnt.exe", strlen("powerpnt.exe")))
				{
					*bIsSystemProcess = POWERPNT_PROCESS ;
				}				
			}
		}

		//将路径转换为小写字母
		_wcslwr(wszFilePathName) ;

#ifdef DBG
		DbgPrint("wszFilePathName:%S\n", wszFilePathName);	
#endif

		//当前文件如果出现在系统临时文件夹，那么直接返回真，表示应该监视当前进程
		if (wcsstr(wszFilePathName, L"\\local settings\\temp"))//\Local Settings\Temp
		{
#ifdef DBG
			DbgPrint("\\local settings\\temp=true\n");	
#endif
			return TRUE;
			
		}


		if (wcsstr(wszFilePathName, L"microsoft shared"))//\Local Settings\Temp
		{
			return TRUE;
			
		}

		if (wcsstr(wszFilePathName, L"temporary internet files"))//\Local Settings\Temp
		{
			return TRUE;
			
		}


		if (wcsstr(wszFilePathName, L"local\\microsoft"))//\Local Settings\Temp
		{

			return TRUE;
		}		

		return FALSE;

		// go to end of file path name, save pointer in pwszExt
		pwszExt = wszFilePathName + uLength - 1 ;

		// verify file attribute, if directory, return false
		if (pwszFilePathName[uLength-1] == L'\\')
		{//if directory, filter it
			bRet = FALSE ;
#ifdef DBG
			DbgPrint("It is a dir:False\n");	
#endif
			__leave ;
		}

		// redirect to file extension name(including point)
		while (((pwszExt != wszFilePathName) && (*pwszExt != L'\\')) && ((*pwszExt) != L'.')) //定向至扩展名
		{//direct into file extension
			pwszExt -- ;
		}

		// verify this is a file without extension name
		if ((pwszExt == wszFilePathName) || (*pwszExt == L'\\'))
		{//no file extension exists in input filepath name, filter it.
			bRet = FALSE ;
#ifdef DBG
			DbgPrint("It has no ext:False\n");	
#endif
			__leave ;
			pwszExt[0] = L'.' ;
			pwszExt[1] = L'\0' ;
		}

		// verify tmp file
		if ((bIsPPTFile != NULL) && !_wcsnicmp(pwszExt, L".ppt", wcslen(L".ppt")))
		{
			*bIsPPTFile = TRUE ;
		}

		// verify tmp file
		if ((bIsPPTFile != NULL) && !_wcsnicmp(pwszExt, L".pptx", wcslen(L".pptx")))
		{
			DbgPrint("------------------------------------------verify tmp file\n");	
			*bIsPPTFile = TRUE ;
		}

		// compare current process name with process info in monitored list
		// if existing, match file extension name
		TmpListEntryPtr = g_ProcessListHead.Flink ;
		while(&g_ProcessListHead != TmpListEntryPtr)
		{
			DbgPrint("------------------------------------------A\n");	
			psProcessInfo = CONTAINING_RECORD(TmpListEntryPtr, iPROCESS_INFO, ProcessList) ;

			if(!_strnicmp(psProcessInfo->szProcessName, szProcessName, strlen(szProcessName)))//if(1)
			{

				int nIndex = 0 ;
				DbgPrint("------%s------------------------------------B\n",psProcessInfo->szProcessName);	

				if (psProcessInfo->wsszRelatedExt[0][0] == L'\0')
				{//no filter file extension, return monitor flag
					bRet = psProcessInfo->bMonitor;
					DbgPrint("matched:%u __leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave__leave\n", bRet);	
					__leave ;
				}

				while (TRUE)
				{// judge whether current file extension name is matched with monitored file type in list
					if (psProcessInfo->wsszRelatedExt[nIndex][0] == L'\0')
					{
						DbgPrint("------------------------------------------C\n");	
#ifdef DBG
						DbgPrint("[nIndex][0] == 0\n");	
#endif
						bRet = FALSE ;
						break ;
					}
					else if ((wcslen(pwszExt) == wcslen(psProcessInfo->wsszRelatedExt[nIndex])) && !_wcsnicmp(pwszExt, psProcessInfo->wsszRelatedExt[nIndex], wcslen(pwszExt)))
					{// matched, return monitor flag
						DbgPrint("------------------------------------------D\n");	
						bRet = TRUE ;
#ifdef DBG
						DbgPrint("matched:%u\n", bRet);	
#endif						
						__leave ;
						//break ;
					}
					nIndex ++ ;
				}
				//__leave ;
			}

			// move to next process info in list
			TmpListEntryPtr = TmpListEntryPtr->Flink ;
		}

		bRet = FALSE ;
	}
	finally{
		
		//Todo some post work here
	}

	return bRet ;
}

*/
BOOLEAN bRetForPs_IsCurrentProcessMonitored = FALSE ;
WCHAR wszFilePathName[MAX_PATH] = {0} ;
BOOLEAN Ps_IsCurrentProcessMonitored(WCHAR* pwszFilePathName, ULONG uLength,BOOLEAN* bSystemAndExt)
{
	
	UCHAR szProcessName[16] = {0} ;
	PLIST_ENTRY TmpListEntryPtr = NULL ;
	PiPROCESS_INFO psProcessInfo = NULL ;	
	WCHAR* pwszExt = NULL ;
	INT bIsSystemProcess = NORMAL_PROCESS ;

	try{
		// get current process name. 
		GetCurrentProcessName(szProcessName);

		// save file path name in local buffer
		RtlCopyMemory(wszFilePathName, pwszFilePathName, uLength*sizeof(WCHAR)) ;
/*
		if ((strlen(szProcessName) == strlen("System")) && !_strnicmp(szProcessName, "System", strlen("System")))
		{
			bIsSystemProcess = SYSTEM_PROCESS ;
		}

		if ((strlen(szProcessName) == strlen("taskhost.exe")) && !_strnicmp(szProcessName, "taskhost.exe", strlen("taskhost.exe")))
		{
			bIsSystemProcess = SYSTEM_PROCESS ;
		}

		if ((strlen(szProcessName) == strlen("explorer.exe")) && !_strnicmp(szProcessName, "explorer.exe", strlen("explorer.exe")))
		{
			bIsSystemProcess = EXPLORER_PROCESS ;
		}*/

			if ((strlen(szProcessName) == strlen("wps.exe")) && !_strnicmp(szProcessName, "wps.exe", strlen("wps.exe")))
			{
				bIsSystemProcess = WPS_PROCESS ;
			}

			if ((strlen(szProcessName) == strlen("et.exe")) && !_strnicmp(szProcessName, "et.exe", strlen("et.exe")))
			{
				bIsSystemProcess = WPS_PROCESS ;
			}

			if ((strlen(szProcessName) == strlen("wpp.exe")) && !_strnicmp(szProcessName, "wpp.exe", strlen("wpp.exe")))
			{
				bIsSystemProcess = WPS_PROCESS ;
			}

			if ((strlen(szProcessName) == strlen("winword.exe")) && !_strnicmp(szProcessName, "winword.exe", strlen("winword.exe")))
			{
				bIsSystemProcess = WINWORD_PROCESS ;
			}

			if ((strlen(szProcessName) == strlen("excel.exe")) && !_strnicmp(szProcessName, "excel.exe", strlen("excel.exe")))
			{
				bIsSystemProcess = EXCEL_PROCESS ;
			}

/*
			if (bIsSystemProcess == WPS_PROCESS)//若用tmp判断后就是WPS全都不加密了，也包括wps文件
			{				
				//将路径转换为小写字母
				_wcslwr(wszFilePathName) ;
				if (!wcsstr(wszFilePathName, L"jiangkai307"))//\Local Settings\Temp 不会显示盘符，所以不能捕获c
				{

					return TRUE;//这样后往桌面保存后就怎么都不是密文了
				}
			}	
*/

		//将路径转换为小写字母
		_wcslwr(wszFilePathName) ;
		
#ifdef DBG
		//DbgPrint("wszFilePathName:%S\n", wszFilePathName);	
#endif

		// go to end of file path name, save pointer in pwszExt
		pwszExt = wszFilePathName + uLength - 1 ;

		// verify file attribute, if directory, return false
		if (pwszFilePathName[uLength-1] == L'\\')
		{//if directory, filter it
			bRetForPs_IsCurrentProcessMonitored = FALSE ;
#ifdef DBG
			//DbgPrint("It is a dir:False\n");	
#endif
			__leave ;
		}

		// redirect to file extension name(including point)
		while (((pwszExt != wszFilePathName) && (*pwszExt != L'\\')) && ((*pwszExt) != L'.')) //定向至扩展名
		{//direct into file extension
			pwszExt -- ;
		}

		// verify this is a file without extension name
		if ((pwszExt == wszFilePathName) || (*pwszExt == L'\\'))
		{//no file extension exists in input filepath name, filter it.
			bRetForPs_IsCurrentProcessMonitored = FALSE ;
#ifdef DBG
			//DbgPrint("It has no ext:False\n");	
#endif
			__leave ;
			pwszExt[0] = L'.' ;
			pwszExt[1] = L'\0' ;
		}

	//若tmp放过就全不加密，若全不放过就全加密，说明某些路径下的tmp需要放过？
		/*
		if ((bIsSystemProcess == WPS_PROCESS) && (_wcsnicmp(pwszExt, L".tmp", wcslen(L".tmp"))))
		{
			bRet = TRUE;
		}
		*/

		//通过测试发现若不对进程判断，word 会不起到加密作用的,说明WORD时.tmp必须要加密，而且excel的tmp不能加密，否则无法创建xlsx文件。
 		if ((bIsSystemProcess == EXCEL_PROCESS) && (!_wcsnicmp(pwszExt, L".tmp", wcslen(L".tmp"))))
 		{
 			bRetForPs_IsCurrentProcessMonitored = TRUE ;
 		}

/*
		if ((bIsSystemProcess == SYSTEM_PROCESS)||(bIsSystemProcess == EXPLORER_PROCESS))
		{
			
			if((!_wcsnicmp(pwszExt, L".tmp", wcslen(L".tmp")))||(!_wcsnicmp(pwszExt, L".doc", wcslen(L".doc")))||(!_wcsnicmp(pwszExt, L".docx", wcslen(L".docx")))||(!_wcsnicmp(pwszExt, L".wps", wcslen(L".wps"))))
			{

				*bSystemAndExt = TRUE;
				bRet = TRUE ;
			}
	
		}
*/
	}
	finally{
		/**/
		//Todo some post work here
	}

	return bRetForPs_IsCurrentProcessMonitored ;
}


//判断是否机密进程
char szProcessName1[NT_PROCNAMELEN] ;
BOOLEAN S_IsCurProcSecEx(
    __in PFLT_CALLBACK_DATA Data,
    PUCHAR ProcessType
    )
{	
	int bRet;
	PROCESS_INFO*  var = NULL;
	BOOLEAN bSystemAndExt = FALSE;

	



	//未对EXCEL起作用，所以将其注释掉，可能是需要匹配扩展名的问题~
	NTSTATUS status = STATUS_SUCCESS ;
	
	PFLT_FILE_NAME_INFORMATION pfNameInfo = NULL ;
	WCHAR wszFilePathName[MAX_PATH] = {0} ;
	BOOLEAN bIsSystemProcess = FALSE ;
	status = FltGetFileNameInformation(Data, 
		FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT, 
		&pfNameInfo) ;
	if (NT_SUCCESS(status))
	{

/*
		if (0 != pfNameInfo->Name.Length)
		{

			// save file path name in local buffer
			RtlCopyMemory(wszFilePathName, pfNameInfo->Name.Buffer, pfNameInfo->Name.Length) ;

			//将路径转换为小写字母
			_wcslwr(wszFilePathName) ;

			if (wcsstr(wszFilePathName, L"wps"))//\Local Settings\Temp
			{

				return FALSE;
			}
		}

		if (0 != pfNameInfo->Name.Length)
		{

			// save file path name in local buffer
			RtlCopyMemory(wszFilePathName, pfNameInfo->Name.Buffer, pfNameInfo->Name.Length) ;

			//将路径转换为小写字母
			//_wcslwr(wszFilePathName) ;

			if (wcsstr(wszFilePathName, L"WPS"))//\Local Settings\Temp
			{

				return FALSE;
			}
		}
*/
		//发现EXCEL无法创建的问题是因为对临时目录的文件加密了
		if (Ps_IsCurrentProcessMonitored(pfNameInfo->Name.Buffer,pfNameInfo->Name.Length/sizeof(WCHAR),&bSystemAndExt) == TRUE)
		{/*
			//只有当系统进程或Excel进程的.tmp文件时才会返回True。
			if(bSystemAndExt==TRUE)
			{
				if(ProcessType!=NULL)
				*ProcessType = SYSTEM_PROCESS;
				return TRUE;
			}				
			else*/
				return FALSE;//代表只是excel进程的.tmp文件，所以放开。
		}


		/*
		//解决EXCL无法创建xlsx文档的问题。
		if (0 != pfNameInfo->Name.Length)
		{

			// save file path name in local buffer
			RtlCopyMemory(wszFilePathName, pfNameInfo->Name.Buffer, pfNameInfo->Name.Length) ;

			//将路径转换为小写字母
			_wcslwr(wszFilePathName) ;

			if (wcsstr(wszFilePathName, L"\\local settings\\temp"))//\Local Settings\Temp
			{
				
				return FALSE;
			}


			if (wcsstr(wszFilePathName, L"microsoft shared"))//\Local Settings\Temp
			{
				return FALSE;

			}

			if (wcsstr(wszFilePathName, L"local\\microsoft"))//\Local Settings\Temp
			{

				return FALSE;
			}	

			if (wcsstr(wszFilePathName, L"wps"))//\Local Settings\Temp
			{

				return FALSE;
			}	
			

		}*/

	}
	
	








	memset(szProcessName1,0,16);

	GetCurrentProcessName(szProcessName1);
	//if(strlen(szProcessName))
	//KdPrint(("%s\n",szProcessName));


// 	if ((!strcmp(szProcessName,"WINWORD.EXE"))||(!strcmp(szProcessName,"WINWORD.EXE")))
// 	{
// 		bRet = TRUE;
// 		return bRet;
// 	}

	//问题出现在匹配函数中，研究如何下断点是的字符串是WINWORD.EXE的条件断点
	var = CompareString(szProcessName1);//先通过进程名查找
	if(var!=NULL)
		bRet = TRUE;
	else
		bRet = FALSE;

	return bRet;
	/*
    PLIST_ENTRY P;
    PPROCESS_INFO pListProcInfo;
    PFILTER_PROCESS_NODE ProcNode;
    int iRet;
    HANDLE ProcID;
    BOOLEAN bRet=FALSE;
    LARGE_INTEGER interval={0};
    BOOLEAN bFound = FALSE;
    LARGE_INTEGER st={0};
    LARGE_INTEGER et={0};
    //tickcount的单位，TickInc单位为100ns
    ULONG uTickInc = 0;
    
    KIRQL OldIRQL;

    //进程常量
    CHAR PN_Explorer[]="explorer.exe";
    CHAR PN_VMWareService[]="VMwareService.exe";
	CHAR PN_Word[]="winword.exe";
    CHAR PN_Excel[]="excel.exe";
    CHAR PN_PowerPnt[]="powerpnt.exe";
    CHAR PN_SVCHost[]="svchost.exe";
    CHAR PN_ACAD[]="acad.exe";
    CHAR PN_DELPHI32[]="delphi32.exe";

    //return S_IsCurProcSec(Proc,ProcessType);
    uTickInc = KeQueryTimeIncrement() * 10000;
    KeQueryTickCount(&st);
    
    ProcID = PsGetCurrentProcessId();
    do {
        S_ProcListLock(&OldIRQL);
        try {
            for (P = s_List_SecProc.Flink; P != &s_List_SecProc; P = P->Flink) {
                ProcNode = (PFILTER_PROCESS_NODE)P;
                pListProcInfo = (PPROCESS_INFO)ProcNode->ProcessInfo;

                if (pListProcInfo->ProcessID == ProcID) {
                	bRet = pListProcInfo->bMonitor;
                     if (ProcessType != NULL)
                     {
                        if (!_strnicmp(pListProcInfo->ProcessName, PN_Explorer, strlen(PN_Explorer))
                            || !_strnicmp(pListProcInfo->ProcessName, PN_SVCHost, strlen(PN_SVCHost))
                            ||(ProcID == 4)
                            || !_strnicmp(pListProcInfo->ProcessName, PN_VMWareService, strlen(PN_VMWareService)))
                        {
                        	*ProcessType = SYSTEM_PROCESS ;
                        } 
						else {
                		
							*ProcessType = NORMAL_PROCESS ;

							//ADDTION START
							if (!_strnicmp(pListProcInfo->ProcessName, PN_Word, strlen(PN_Word)))
								*ProcessType = WINWORD_PROCESS ;
							//ADDTION END
	                	   	
							if (!_strnicmp(pListProcInfo->ProcessName, PN_Excel, strlen(PN_Excel)))
                				*ProcessType = EXCEL_PROCESS ;

                	   		if (!_strnicmp(pListProcInfo->ProcessName, PN_PowerPnt, strlen(PN_PowerPnt)))
                				*ProcessType = POWERPNT_PROCESS ;
	                       
							if (!_strnicmp(pListProcInfo->ProcessName, PN_ACAD, strlen(PN_ACAD)))
                				*ProcessType = ACAD_PROCESS ;
	                            
							if (!_strnicmp(pListProcInfo->ProcessName, PN_DELPHI32, strlen(PN_DELPHI32)))
                				*ProcessType = DELPHI7_PROCESS ;
                	  }
                     }
                     bFound = TRUE;
                     break;
                }
            }
        } finally {
            S_ProcListUnLock(OldIRQL);
        }

        KeQueryTickCount(&et);
        //如果超过20毫秒未在列表内找到进程，则直接返回非机密
        //以免外部应用被关闭而造成函数无法退出
        if (et.QuadPart - st.QuadPart >= 20 * uTickInc)
            break;
        
        //等待10毫秒重新判断，一直要到判断成功为止
        if (!bFound && (KeGetCurrentIrql() <= APC_LEVEL))
        {
            interval.QuadPart = -(2*10000);//10ms
            KeDelayExecutionThread(KernelMode,FALSE,&interval);
			//continue;加上后未起作用
        }
	} while(FALSE) ;//Original:while(FALSE)

	bRet = TRUE;
    return bRet;
	*/
}
// 
// //设置根据应用传入的信息更新进程列表
// BOOLEAN S_SetProcInfo(PVOID InputBuffer)
// {
//     PPROCESS_INFO pProcInfo,pListProcInfo;
//     PMSG_SEND_SET_PROCESS_INFO pSetProcInfo = (PMSG_SEND_SET_PROCESS_INFO)InputBuffer;
//     ULONG i;
//     PLIST_ENTRY P;
//     PFILTER_PROCESS_NODE ProcNode;
//     BOOLEAN bFound;
// 
//     KIRQL OldIrql ;
// 
//     S_ProcListLock(&OldIrql);
//     try
//     {
//         pProcInfo = &pSetProcInfo->sProcInfo;
//         for (i=0;i<pSetProcInfo->sSendType.uFlag;i++)
//         {
//             bFound = FALSE;
//             for (P = s_List_SecProc.Flink; P != &s_List_SecProc; P = P->Flink) 
//             {
//                 ProcNode = (PFILTER_PROCESS_NODE)P;
//                 pListProcInfo = (PPROCESS_INFO)ProcNode->ProcessInfo;
// 
//                 if (pListProcInfo->ProcessID == pProcInfo->ProcessID)
//                 {
//                     bFound = TRUE;
//                     if (!pProcInfo->bPorcessValid)
//                     {
//                         //如果进程已无效关闭，则从列表删除，减少循环次数
//                         P = P->Blink;
//                         RemoveEntryList((PLIST_ENTRY)ProcNode);
//                         
//                         ExFreePoolWithTag(pListProcInfo,NAME_TAG);
//                         ExFreePoolWithTag(ProcNode,NAME_TAG);
//                         KdPrint(("DelProc:[%d]%s\n",pProcInfo->ProcessID,pProcInfo->ProcessName));
//                     } else {
//                         RtlCopyMemory(pListProcInfo, pProcInfo, sizeof(PROCESS_INFO));
//                         KdPrint(("UptProc:[%d]%s\n",pListProcInfo->ProcessID,pListProcInfo->ProcessName));
//                     }
//                     break;
//                 }
//             }
//             if (!bFound)
//             {
//                 ProcNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILTER_PROCESS_NODE), NAME_TAG);
//                 if (NULL != ProcNode)
//                 {
//                     pListProcInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_INFO), NAME_TAG);
//                     ProcNode->ProcessInfo = pListProcInfo;
//                     RtlCopyMemory(pListProcInfo, pProcInfo, sizeof(PROCESS_INFO));
//                     InsertHeadList(&s_List_SecProc, (PLIST_ENTRY)ProcNode);
// 
//                     KdPrint(("AddProc:[%d]%s\n",pListProcInfo->ProcessID,pListProcInfo->ProcessName));
//                 }
//             }
//             pProcInfo += 1;
//         }
//     } finally {
//         S_ProcListUnLock(OldIrql);
//     }
// 
//     return TRUE;
// }

//判断指定文件是否为加密文件
BOOLEAN S_CheckDirFile(
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PUNICODE_STRING PathName,
    __in BOOLEAN bVolumePath,
    __in PWCHAR FileName,
    __in ULONG FileNameLength
    )
{
    NTSTATUS status;
    WCHAR  wszFileDosFullPath[MAX_PATH] = {0};
    PWCHAR pszRelativePathPtr = NULL ;	
    UNICODE_STRING sFileDosFullPath ;
    OBJECT_ATTRIBUTES ob ;
    IO_STATUS_BLOCK IoStatus ;
    HANDLE hFile = NULL ;
    PFILE_OBJECT FileObject = NULL ;

    PVOID pFileGUID = NULL;

    BOOLEAN CheckRet = FALSE;
    LARGE_INTEGER FileSize = {0} ;
    BOOLEAN bDirectory = FALSE;
    
    try {
         RtlCopyMemory(wszFileDosFullPath, PathName->Buffer, PathName->Length);
         pszRelativePathPtr = wszFileDosFullPath;
         pszRelativePathPtr = pszRelativePathPtr + wcslen(pszRelativePathPtr);
         if (!bVolumePath)
            wcscpy(pszRelativePathPtr, L"\\");
         pszRelativePathPtr = pszRelativePathPtr + wcslen(pszRelativePathPtr);
         RtlCopyMemory(pszRelativePathPtr, FileName, FileNameLength);

         RtlInitUnicodeString(&sFileDosFullPath, wszFileDosFullPath) ;

	  // init object attribute
	  InitializeObjectAttributes(&ob, 
	        &sFileDosFullPath, 
	        OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE, 
	        NULL,
	        NULL) ;

	  // open file manually
	  status = FltCreateFile(FltObjects->Filter,
			FltObjects->Instance,
			&hFile,
			FILE_READ_DATA|FILE_WRITE_DATA,
                     &ob,
                     &IoStatus,
                     NULL,
                     FILE_ATTRIBUTE_NORMAL,
                     FILE_SHARE_READ,
                     FILE_OPEN,
                     FILE_NON_DIRECTORY_FILE,
                     NULL,
                     0,
                     IO_IGNORE_SHARE_ACCESS_CHECK
                     ) ;
	  if (!NT_SUCCESS(status))
			leave ;

				// get fileobject
	  status = ObReferenceObjectByHandle(hFile,
			STANDARD_RIGHTS_ALL,
			*IoFileObjectType,
			KernelMode,
			&FileObject,
			NULL
			) ;
	  if (!NT_SUCCESS(status))
			leave ;
         status = File_GetFileStandardInfoNew(FileObject,FltObjects,NULL,&FileSize,&bDirectory);
	  if (!NT_SUCCESS(status))
			leave ;
         if (bDirectory || (FileSize.QuadPart < FILE_HEAD_LENGTH))
                    leave ;
         pFileGUID = ExAllocatePoolWithTag(NonPagedPool, FILE_GUID_LENGTH, NAME_TAG) ;
         status = ReadFileFlag(FileObject,FltObjects,pFileGUID,FILE_GUID_LENGTH);
         if (!NT_SUCCESS(status))
                    leave;

         CheckRet = FILE_GUID_LENGTH == RtlCompareMemory(g_psFileFlag, pFileGUID, FILE_GUID_LENGTH);
    }finally {
         if (NULL != FileObject)
		ObDereferenceObject(FileObject) ;
	  if (NULL != hFile)
		FltClose(hFile) ;

         if (NULL != pFileGUID)
              ExFreePool(pFileGUID);
    }

    return CheckRet;
}

//执行PosDirCtrl的判断，explorer.exe就是发送此IRP来获取文件大小
NTSTATUS S_DoDirCtrl(
    __in PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout PVOID pBuf,
    __in FILE_INFORMATION_CLASS FileInfoClass
    )
{
    PFLT_FILE_NAME_INFORMATION pfNameInfo = NULL ;
    NTSTATUS status;
    BOOLEAN bVolumePath=FALSE;;

    if ((FileBothDirectoryInformation != FileInfoClass) 
        && (FileDirectoryInformation != FileInfoClass) 
         && (FileFullDirectoryInformation != FileInfoClass))
        return STATUS_SUCCESS;
    
    try {
             //获取文件全路径
         status = FltGetFileNameInformation(Data, 
     	        FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT, 
     	        &pfNameInfo) ;
         if (!NT_SUCCESS(status))
         	leave ;

         FltParseFileNameInformation(pfNameInfo);
         if (0 == pfNameInfo->Name.Length)
         	leave ;

         //判断是否卷根目录，如果是，Name最后有'\'，否则，没有
         bVolumePath = sizeof(WCHAR) == pfNameInfo->ParentDir.Length;
         switch (FileInfoClass)
         {
            case FileBothDirectoryInformation :
                    {
                         PFILE_BOTH_DIR_INFORMATION pbdi = (PFILE_BOTH_DIR_INFORMATION)pBuf;
                         do {           
                            if (S_CheckDirFile(
                                    FltObjects,
                                    &pfNameInfo->Name,
                                    bVolumePath,
                                    pbdi->FileName,
                                    pbdi->FileNameLength))
                            {
                                pbdi->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
                                pbdi->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
                            }
                            if (!pbdi->NextEntryOffset)
                                break;
                            pbdi = (PFILE_BOTH_DIR_INFORMATION)((CHAR*)pbdi + pbdi->NextEntryOffset);
                        } while (FALSE);

                        break;
                    }
            case FileDirectoryInformation :
                    {
                         PFILE_DIRECTORY_INFORMATION pfdi = (PFILE_DIRECTORY_INFORMATION)pBuf;
                         do {           
                            if (S_CheckDirFile(
                                    FltObjects,
                                    &pfNameInfo->Name,
                                    bVolumePath,
                                    pfdi->FileName,
                                    pfdi->FileNameLength))
                            {
                                pfdi->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
                                pfdi->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
                            }
                            if (!pfdi->NextEntryOffset)
                                break;
                            pfdi = (PFILE_DIRECTORY_INFORMATION)((CHAR*)pfdi + pfdi->NextEntryOffset);
                        } while (FALSE);

                        break;
                    }
            case FileFullDirectoryInformation :
                    {
                         PFILE_FULL_DIR_INFORMATION pffdi = (PFILE_FULL_DIR_INFORMATION)pBuf;
                         do {           
                            if (S_CheckDirFile(
                                    FltObjects,
                                    &pfNameInfo->Name,
                                    bVolumePath,
                                    pffdi->FileName,
                                    pffdi->FileNameLength))
                            {
                                pffdi->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
                                pffdi->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
                            }
                            if (!pffdi->NextEntryOffset)
                                break;
                            pffdi = (PFILE_BOTH_DIR_INFORMATION)((CHAR*)pffdi + pffdi->NextEntryOffset);
                        } while (FALSE);

                        break;
                    }
            default : break;    
         }
    } finally {
	   if (NULL != pfNameInfo)
		FltReleaseFileNameInformation(pfNameInfo) ;
    }

    return STATUS_SUCCESS;
}

// BOOLEAN S_ProcValid(
//     HANDLE uProcID
//     )
// {
//     PLIST_ENTRY P;
//     PPROCESS_INFO pListProcInfo;
//     PFILTER_PROCESS_NODE ProcNode;
//     BOOLEAN bRet = FALSE;
//     KIRQL OldIRQL;
// 
//     S_ProcListLock(&OldIRQL);
//     try {
//         for (P = s_List_SecProc.Flink; P != &s_List_SecProc; P = P->Flink) {
//            ProcNode = (PFILTER_PROCESS_NODE)P;
//            pListProcInfo = (PPROCESS_INFO)ProcNode->ProcessInfo;
// 
//            if (pListProcInfo->ProcessID == uProcID) 
//            {
//            	bRet =pListProcInfo->bPorcessValid;
//               leave;
//            }
//         }
//     } finally {
//         S_ProcListUnLock(OldIRQL);
//     }
//     
//     return bRet;
// }

BOOLEAN S_CheckProcCanGo(
    BOOLEAN bSecProc,
    HANDLE ProcID,
    PSTREAM_CONTEXT pStreamCtx
    )
{
    int i;
    HANDLE OpenProcID;
    KIRQL OldIrql ;
    
    if (bSecProc)//是机密进程时，记录进程PID到文件流对象中并返回TRUE可以打开，满足条件是：OpenProcID == NULL 或 已经记录了(OpenProcID == ProcID)并覆盖下即可，不算8个进程中第个机密进程打开它的进程。
    {
        for (i=0;i<SECPROC_MAXCOUNT;i++)
        {
            OpenProcID = pStreamCtx->uOpenSecProcID[i];
            if ((OpenProcID == 0) ||(OpenProcID == ProcID))// ||(!S_ProcValid(OpenProcID)))
            {
                SC_LOCK(pStreamCtx, &OldIrql) ;
                pStreamCtx->uOpenSecProcID[i] = ProcID;
                SC_UNLOCK(pStreamCtx, &OldIrql) ;
                return TRUE;
            }
        }

        return FALSE;//若8个机密进程已经满了则不允许打开
    } else {//不是机密进程时
        for (i=0;i<SECPROC_MAXCOUNT;i++)
        {
            OpenProcID = pStreamCtx->uOpenSecProcID[i];
            if (OpenProcID != 0) //if ((OpenProcID != 0) && S_ProcValid(OpenProcID))
            {
                return FALSE;
            }
            SC_LOCK(pStreamCtx, &OldIrql) ;
            pStreamCtx->uOpenSecProcID[i] = 0;
            SC_UNLOCK(pStreamCtx, &OldIrql) ;
        }
        return TRUE;
    }
}

//判断是否可以生成加密文件
BOOLEAN S_CanGenCryptFile(
    __in PFLT_FILE_NAME_INFORMATION pfNameInfo,
    __in BOOLEAN bProcType
    )
{
       CHAR FN[MAX_PATH]={0};
       CHAR FExt[MAX_PATH]={0};
       
    //   S_UnicodeStringToChar(&pfNameInfo->Name,FN,FALSE);
       
       //AutoCAD不加密csa文件
      // S_UnicodeStringToChar(&pfNameInfo->Extension,FExt,FALSE);    
       //if ((ACAD_PROCESS == bProcType) && !_stricmp(FExt,"csa"))
           // return FALSE;

	//  if((_stricmp(FExt,"docx")==0) ||(_stricmp(FExt,"tmp")==0))//结论：只针对docx扩展名加密会起不到加密效果，但若加密所有tmp后未开启驱动创建的明文也会被加密，而且excel的tmp不能加密，否则无法创建excel。
		 // return TRUE;
       
     //  KdPrint(("GenCryptFile:%s[%d][%s]\n",FN,PsGetCurrentProcessId(),FExt));
      return TRUE;// return TRUE;
}

/***************************************************************************************
*以下为微过滤回调函数实现
****************************************************************************************/

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/



NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    PSECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;		

    
    UNREFERENCED_PARAMETER( RegistryPath );

    KdPrint(("S_Crypt!DriverEntry: Entered\n") );
	
	//KeInitializeSpinLock(&g_SpinLockForReadFileFlag);
	//KeInitializeSpinLock(&g_SpinLockForWriteFileFlag);

#if DBG
     //_asm int 3
#endif


	//InitializeListHead(&g_ProcessListHead) ;
	//KeInitializeSpinLock(&g_ProcessListLock) ;


	ProcessNameOffset=DecideProcessNameOffsetInEPROCESS();//用于获得进程名，注意只能用于这里，因为DriverEntry运行于system进程

    //S_InitList();
    //
    //  Init lookaside list used to allocate our context structure used to
    //  pass information from out preOperation callback to our postOperation
    //  callback.
    //
    ExInitializeNPagedLookasideList(&Pre2PostContextList, NULL, NULL, 0, sizeof(PRE_2_POST_CONTEXT), PRE_2_POST_TAG, 0 );

    // init global file flag structure
    status = File_InitFileFlag();
    if (!NT_SUCCESS(status)) {
	return status ;
    }
    
    //注册过滤器
    status = FltRegisterFilter( DriverObject,
              &FilterRegistration,
              &gFilterHandle );

    ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {
        //启动过滤器
        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {
            KdPrint(("FltUnregisterFilter SUCCESS\n"));
            FltUnregisterFilter( gFilterHandle );
        }
	 KdPrint(("FltRegisterFilter SUCCESS\n"));
    }

    //创建通讯端口
    status  = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
    
    if (!NT_SUCCESS( status )) 
    {
       goto final;
    }


    status  = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );

    if (!NT_SUCCESS( status )) 
    {
        goto final;
    }
                                 
                     
    RtlInitUnicodeString( &uniString, MINISPY_PORT_NAME );

    InitializeObjectAttributes( &oa,
            &uniString,
            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
            NULL,
            sd );

    status = FltCreateCommunicationPort( gFilterHandle,
            &gServerPort,
            &oa,
            NULL,
            S_MiniConnect,
            S_MiniDisconnect,
            S_MiniMessage,
            1 );

    FltFreeSecurityDescriptor( sd );

    if (!NT_SUCCESS( status )) 
    {
        goto final;
    }    
    //CanFilter = TRUE;
final :
    
    if (!NT_SUCCESS( status ) ) {
        ExDeleteNPagedLookasideList( &Pre2PostContextList );
         if (NULL != gFilterHandle) {
             FltUnregisterFilter( gFilterHandle );
         }       
    } 
    return status;
}

NTSTATUS
S_Unload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
{        
    LARGE_INTEGER interval;
    
    //PAGED_CODE();
  
    UNREFERENCED_PARAMETER( Flags );
    
    KdPrint( ("S_Crypt!S_Unload: Entered\n") );

	FreeChain();
	KdPrint( ("S_Crypt!S_Unload: FreeChain\n") );

    CanFilter = FALSE;

    FltCloseCommunicationPort( gServerPort );
    FltUnregisterFilter( gFilterHandle );

    // 睡眠5秒。等待所有irp处理结束
    //KdPrint( ("KeDelayExecutionThread\n"));
   // interval.QuadPart = -(1 * 10000000);//5S
    //KeDelayExecutionThread(KernelMode,FALSE,&interval);

    ExDeleteNPagedLookasideList( &Pre2PostContextList );
   // S_ClearProcList(&s_List_SecProc);

    if (g_psFileFlag) {
	File_UninitFileFlag() ;
    }
    
    KdPrint( ("S_Crypt!S_Unload\n") );

    return STATUS_SUCCESS;
}

VOID
CleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    )
{
    PSTREAM_CONTEXT streamCtx = NULL ;

    //PAGED_CODE();

    switch(ContextType) {
	case FLT_STREAM_CONTEXT:
		{
			streamCtx = (PSTREAM_CONTEXT)Context ;

			if (streamCtx == NULL)
				break ;

			if (NULL != streamCtx->Resource)
			{
				ExDeleteResourceLite(streamCtx->Resource) ;
				ExFreePoolWithTag(streamCtx->Resource, RESOURCE_TAG) ;
			}
                     break ;	
		}
        default : break ;	
    }
}

FLT_PREOP_CALLBACK_STATUS
S_PreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    //PAGED_CODE();
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( FltObjects );
    
    if (!CanFilter) 
         return FLT_PREOP_SUCCESS_NO_CALLBACK;
    if (Data->RequestorMode == KernelMode)
         return FLT_PREOP_SUCCESS_NO_CALLBACK;

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;//FLT_PREOP_SUCCESS_NO_CALLBACK/FLT_PREOP_SUCCESS_WITH_CALLBACK：这个返回值表示处理成功，让过滤管理器去做自己的事，区别在于WITH_CALLBACK的返回值会标明需要回调post函数。而NO_CALLBACK的则标明不需要。
}

CHAR NameExt[512]={0};
CHAR FileName[512]={0};

FLT_POSTOP_CALLBACK_STATUS
S_PostCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	

	NTSTATUS status = STATUS_SUCCESS ;
	NTSTATUS status1 = 8 ;
	UCHAR szProcessNameCur[16] = {0} ;
	PFLT_FILE_NAME_INFORMATION pfNameInfo = NULL ;

	PSTREAM_CONTEXT pStreamCtx = NULL ;
	LARGE_INTEGER FileSize = {0} ;

	LARGE_INTEGER ByteOffset = {0} ;
	LARGE_INTEGER OrigByteOffset = {0} ;
	PFILE_FLAG psFileFlag = NULL ;

	PFILE_FLAG psFileFlag2 = NULL ;
	DWORD FileFlag2Flag = 0 ;
	KIRQL OldIrql ;

	BOOLEAN bDirectory = FALSE ;
	UCHAR bIsSystemProcess = 0 ;
       BOOLEAN bEncrypt=FALSE;
       BOOLEAN bOpenSecProc;
       HANDLE curProcID;

       ULONG retLen;
       UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
       PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;
	
	//PAGED_CODE();

	 

	try{
		//  如果创建失败了，则不需要再进行下面的处理了
 		if (!NT_SUCCESS( Data->IoStatus.Status ))
			leave ;   

		//获取文件全路径
 		status = FltGetFileNameInformation(Data, 
		    FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT, 
		    &pfNameInfo) ;
		if (!NT_SUCCESS(status))
			leave ;

             FltParseFileNameInformation(pfNameInfo);
			 if (!NT_SUCCESS(status))
				 leave;
		if (0 == pfNameInfo->Name.Length)
			leave ;

		if (0 == RtlCompareUnicodeString(&pfNameInfo->Name, &pfNameInfo->Volume, TRUE))//应该是盘符而不是文件的时候
			leave ;

              status = FltGetVolumeProperties( FltObjects->Volume, volProp, sizeof(volPropBuffer), &retLen );
              if (!NT_SUCCESS(status))
                    leave;
                
		//获取文件的大小(此大小包括了文件正文及头部)
		status = File_GetFileStandardInfo(Data, FltObjects, NULL, &FileSize, &bDirectory) ;
		if (!NT_SUCCESS(status))
			leave ;
        

		status1 = status;

		//不处理目录
		if (bDirectory)
			leave ; 

              bOpenSecProc = S_IsCurProcSecEx(Data,&bIsSystemProcess);//通过当前进程名来匹配链表来判断
              curProcID = PsGetCurrentProcessId();

			  status = -1;
              status = Ctx_FindOrCreateStreamContext(Data,FltObjects,FALSE,//FALSE,//这里的FALSE代表若没找到也不用创建
									&pStreamCtx,NULL); 
		
		
		//判断当前进程是否机密进程
		if (NT_SUCCESS(status))
		{
					//如果bIsFileCrypt被设置为1说明这已经是一个加密文件了
                    if ((pStreamCtx->bIsFileCrypt )/*&&(pStreamCtx->bOpenBySecProc != bOpenSecProc)*/) 
                       Cc_ClearFileCache(FltObjects->FileObject, TRUE, NULL, 0);			

                    //运行Explorer等系统进程打开，但不允许其读取文件

                    SC_LOCK(pStreamCtx, &OldIrql) ;
                    pStreamCtx->bOpenBySecProc = bOpenSecProc;
					SC_UNLOCK(pStreamCtx, OldIrql) ;
					
					GetCurrentProcessName(szProcessNameCur);
					RtlZeroMemory(NameExt,512);//难道是大于0了？主要看是否输出了非WPS.EXE进程,taskhost.exe
					S_UnicodeStringToChar(&pfNameInfo->Extension,NameExt,FALSE); 

					if(bOpenSecProc)
					{
						psFileFlag2 = (PFILE_FLAG)ExAllocatePoolWithTag(NonPagedPool, FILE_FLAG_LENGTH, FILEFLAG_POOL_TAG2) ;
						FileFlag2Flag = 1;
						

						if (FileSize.QuadPart >= FILE_FLAG_LENGTH)
						{
							//文件大小大于文件头则读取,读取文件头	
							ByteOffset.QuadPart = 0; 
							status = ReadFileFlag(FltObjects->FileObject,FltObjects,psFileFlag2,FILE_FLAG_LENGTH);
							if (NT_SUCCESS(status))
							{
								//经典返回BOOL值得写法 BOOLEAN bEncrypt=FALSE;
								if(FILE_GUID_LENGTH != RtlCompareMemory(g_psFileFlag, psFileFlag2, FILE_GUID_LENGTH))
								{	
									//凡是机密进程操作的文件全都认为是机密进程										
									WriteFileFlag(FltObjects->FileObject,FltObjects, psFileFlag2);
									KdPrint(("In if (NT_SUCCESS(status)) write filehead ! \n"));
								}
							}										
						}						
					}					

					if((FileFlag2Flag == 1))
					{
						
						ExFreePoolWithTag(psFileFlag2, FILEFLAG_POOL_TAG2);
						
					}

                leave ;//代表已经找到了旧的文件流信息，所以就不往下走了。
		}

              //保存原偏移量
        	File_GetFileOffset(Data,FltObjects,&OrigByteOffset) ;

              psFileFlag = (PFILE_FLAG)ExAllocatePoolWithTag(NonPagedPool, FILE_FLAG_LENGTH, FILEFLAG_POOL_TAG1) ;//
              if ((FileSize.QuadPart == 0)\
				  && (Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_GENERIC_WRITE) \
                && bOpenSecProc)//如果文件内容大小为0，并且是支持写数据，并且是机密进程打开，则写入加密头
              {
                    //如果文件内容大小为0，并且可写入数据，并且是机密进程打开，则写入加密头
                    if (S_CanGenCryptFile(pfNameInfo,bIsSystemProcess))
                    {
                        status = WriteFileFlag(FltObjects->FileObject,FltObjects, psFileFlag);
                        if (NT_SUCCESS(status))
                        {
                            bEncrypt = TRUE;
                        }
                    }
              } else if (FileSize.QuadPart >= FILE_FLAG_LENGTH){//文件大小大于文件头则读取
                	// 读取文件头	
                	ByteOffset.QuadPart = 0; 
                     status = ReadFileFlag(FltObjects->FileObject,FltObjects,psFileFlag,FILE_FLAG_LENGTH);
                	if (!NT_SUCCESS(status))
					{
						if (psFileFlag != NULL)//未释放导致的蓝屏
							ExFreePoolWithTag(psFileFlag, FILEFLAG_POOL_TAG1);
						
						if((FileFlag2Flag == 1))
						{
							ExFreePoolWithTag(psFileFlag2, FILEFLAG_POOL_TAG2);
							psFileFlag2 = NULL;
						}
						leave ;
					}
                		
					//经典返回BOOL值得写法 BOOLEAN bEncrypt=FALSE;
                	bEncrypt =FILE_GUID_LENGTH == RtlCompareMemory(g_psFileFlag, psFileFlag, FILE_GUID_LENGTH);

        	} else {
        	       //如果文件大小小于头大小，则不处理
        		bEncrypt = FALSE;
        	} 
        	//还原文件偏移
              File_SetFileOffset(Data, FltObjects, &OrigByteOffset) ;

        	//比较加密标志，看是否加密文件
        	if (!bEncrypt)
			{
				if (psFileFlag != NULL)//未释放导致的蓝屏
					ExFreePoolWithTag(psFileFlag, FILEFLAG_POOL_TAG1);
				

				//if((FileFlag2Flag == 1))
				//{
					//ExFreePoolWithTag(psFileFlag2, FILEFLAG_POOL_TAG2);
				//}
				leave ;
			}

		//获取文件对象的流上下文，如果已存在则直接返回，否则创建并返回，找不到流会创建一个流并附到文件对象上
		status = Ctx_FindOrCreateStreamContext(Data,FltObjects,TRUE,
									&pStreamCtx,NULL);
		if (!NT_SUCCESS(status))
		{
			if (psFileFlag != NULL)//未释放导致的蓝屏
				ExFreePoolWithTag(psFileFlag, FILEFLAG_POOL_TAG1);
			
			if(FileFlag2Flag == 1)
			{
				ExFreePoolWithTag(psFileFlag2, FILEFLAG_POOL_TAG2);
			}
			leave ;
		}

		GetCurrentProcessName(szProcessNameCur);

		if (bEncrypt)
		{
			RtlZeroMemory(NameExt,512);
			S_UnicodeStringToChar(&pfNameInfo->Extension,NameExt,FALSE);  

			
			if (!strcmp(szProcessNameCur,"winword.exe")||!strcmp(szProcessNameCur,"WINWORD.EXE"))
			{
				//KdPrint(("winword.exe ProcessName is %s, FILE ENCRYPT  %wZ\n",szProcessNameCur,&pfNameInfo->Name));			

			}else if (!strcmp(szProcessNameCur,"excel.exe")||!strcmp(szProcessNameCur,"EXCEL.EXE"))
			{
				//excel 的 tmp 已经通过 if ((bIsSystemProcess == EXCEL_PROCESS) && (!_wcsnicmp(pwszExt, L".tmp", wcslen(L".tmp")))) 放过
				
			}else if (!strcmp(szProcessNameCur,"powerpnt.exe")||!strcmp(szProcessNameCur,"POWERPNT.EXE"))
			{
				//KdPrint(("powerpnt.exe ProcessName is %s, FILE ENCRYPT  %wZ\n",szProcessNameCur,&pfNameInfo->Name));
			}	
			else
			{
				//显示出不是以上进程的加密进程和加密文件：为什么taskhost.exe & explorer.exe 会加密文件？
				//KdPrint(("FILE ENCRYPT  %wZ, OTHER ProcessName is %s\n",&pfNameInfo->Name,szProcessNameCur));
			}

		}else
		{
			//KdPrint(("NO FILE ENCRYPT %wZ,ProcessName is %s, \n",&pfNameInfo->Name,szProcessNameCur));
		}

		//初始化上下文结构各域的值
		SC_LOCK(pStreamCtx, &OldIrql) ;
		pStreamCtx->bIsFileCrypt = bEncrypt ;
              pStreamCtx->SectorSize = max(volProp->SectorSize,MIN_SECTOR_SIZE);
              pStreamCtx->bOpenBySecProc = bOpenSecProc;
              pStreamCtx->bWriteBySecProc = bOpenSecProc;
             RtlZeroMemory(pStreamCtx->uOpenSecProcID, SECPROC_MAXCOUNT * sizeof(HANDLE));
			if (bOpenSecProc)
			{
                  //KdPrint(("SecCreate:[%d][%d]\n",pStreamCtx->uOpenSecProcID,curProcID));
                 pStreamCtx->uOpenSecProcID[0] = curProcID;
			}
			else
			{
                  //KdPrint(("UnSecCreate:[%d][%d]\n",pStreamCtx->uOpenSecProcID,curProcID));
                  pStreamCtx->uOpenSecProcID[0] = 0;
			}
				
				pStreamCtx->CryptIndex = psFileFlag->FileFlagInfo.CryptIndex;
				SC_UNLOCK(pStreamCtx, OldIrql) ;

				//清除文件缓存，以免文件缓存中保存了文件头数据
				//KdPrint(("OK,Crypt File[%d]\n",curProcID));
				//Cc_ClearFileCache(FltObjects->FileObject, TRUE, NULL, 0);


				if (psFileFlag != NULL)//未释放导致的蓝屏
					ExFreePoolWithTag(psFileFlag, FILEFLAG_POOL_TAG1);

				if((FileFlag2Flag == 1))
				{
					ExFreePoolWithTag(psFileFlag2, FILEFLAG_POOL_TAG2);
				}

	} finally{

		if (NULL != pfNameInfo)
			FltReleaseFileNameInformation(pfNameInfo) ;//pfNameInfo = NULL;

		if (NULL != pStreamCtx)
			FltReleaseContext(pStreamCtx) ;//pStreamCtx= NULL;		
	
		
	}
	
	
	
    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
S_PreWrite (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    NTSTATUS status;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;

    PVOID origBuf;
    ULONG writeLen = iopb->Parameters.Write.Length;
    LARGE_INTEGER writeOffset = iopb->Parameters.Write.ByteOffset ;
    
    PSTREAM_CONTEXT pStreamCtx = NULL ;

    KIRQL OldIrql ;

    LARGE_INTEGER filesize={0};
    BOOLEAN bOpenSec;
    UCHAR bIsSystemProcess = 0 ;

	PFLT_FILE_NAME_INFORMATION pfNameInfo = NULL ;
	UCHAR szProcessNameCur[16] = {0} ;
    
	WCHAR wszFilePathName[MAX_PATH] = {0} ;


    if (!CanFilter) 
         return FLT_PREOP_SUCCESS_NO_CALLBACK;
    
    try {
/*For Test

		GetCurrentProcessName(szProcessNameCur);
		
			status = FltGetFileNameInformation(Data, 
				FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_DEFAULT, 
				&pfNameInfo) ;
			if (NT_SUCCESS(status))
			{

				if (0 != pfNameInfo->Name.Length)
				{

					// save file path name in local buffer
					RtlCopyMemory(wszFilePathName, pfNameInfo->Name.Buffer, pfNameInfo->Name.Length) ;

					//将路径转换为小写字母
					_wcslwr(wszFilePathName) ;

					if (wcsstr(wszFilePathName, L".docx"))//
					{
						KdPrint(("In S_PreWrite wszFilePathName is %ws ~~~.docx~~~ ProcessName  szProcessNameCur is %s  writeOffset = %u \n",wszFilePathName,szProcessNameCur,iopb->Parameters.Write.ByteOffset.LowPart));
						//if (szProcessNameCur,iopb->Parameters.Write.ByteOffset.LowPart<10)
						//{
							//Data->IoStatus.Status = 0;
							//Data->IoStatus.Information = 0;
							//FltStatus = FLT_PREOP_COMPLETE;
							//leave;
						//}						
					}
				}

			}
*/
		
		

	status = Ctx_FindOrCreateStreamContext(Data,FltObjects,FALSE,&pStreamCtx,NULL) ;
	if (!NT_SUCCESS(status))
	    leave ;

       //非加密文件不处理
       if (!pStreamCtx->bIsFileCrypt) 
            leave;

 	// if fast io, disallow it and pass
	if (FLT_IS_FASTIO_OPERATION(Data))
	{// disallow fast io path
	    FltStatus = FLT_PREOP_DISALLOW_FASTIO ;
	    leave ;
	}  

      // 4为system进程，特殊处理
       if (PsGetCurrentProcessId() == 4)
       {

		   //KdPrint(("In S_PreWrite ProcessName is System\n",wszFilePathName,szProcessNameCur));

           if (!pStreamCtx->bWriteBySecProc)
		   {
			  // KdPrint(("Leave! In S_PreWrite ProcessName is System  \n",wszFilePathName,szProcessNameCur));
				 leave;
		   }
               
       } else {
            bOpenSec = S_IsCurProcSecEx(Data,&bIsSystemProcess);
            //在NTFS下，有可能是explorer.exe来写大文件的最后一块
            //考虑到explorer不会向文加密文件内写入数据，所以explorer在写文件的时候可以认为是机密进程
            //在虚拟机下VMwareService也可能会代写最后一块数据
            if (bOpenSec || (SYSTEM_PROCESS != bIsSystemProcess) || (!pStreamCtx->bWriteBySecProc))
            {
                 SC_LOCK(pStreamCtx, &OldIrql) ;
                 pStreamCtx->bOpenBySecProc = bOpenSec;
                 pStreamCtx->bWriteBySecProc = bOpenSec;
                 SC_UNLOCK(pStreamCtx, OldIrql) ;
            }
            if (!pStreamCtx->bWriteBySecProc)
			{

				//KdPrint(("Leave! In S_PreWrite ProcessName is System\n",wszFilePathName,szProcessNameCur));
				 leave;
			}
               
       }   
       
       writeOffset.QuadPart = iopb->Parameters.Write.ByteOffset.QuadPart;
       writeOffset.QuadPart += FILE_HEAD_LENGTH;
    
	// update file real size in stream context timely
	// since file size can be extended in cached io path, so we record file size here
	if (!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
	{
	    //由于在未设置文件大小的情况下写文件，这个操作会设置文件大小，所以要设置偏移
	    File_GetFileSize(Data, FltObjects, &filesize) ;
           if (writeOffset.QuadPart + iopb->Parameters.Write.Length> filesize.QuadPart)
           {
               filesize.QuadPart = writeOffset.QuadPart + iopb->Parameters.Write.Length;
               File_SetFileSize(Data, FltObjects,&filesize);
           }
           //写到缓存后要立即刷缓存，要不然在NTFS下会出现写文件失败的情况
           FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	    leave ;
	}

		// if write length is zero, pass
        if (writeLen == 0)
            leave;
        
		// nocache write length must sector size aligned
        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) {
            writeLen = (ULONG)ROUND_TO_SIZE(writeLen,pStreamCtx->SectorSize);
        }

        newBuf = ExAllocatePoolWithTag( NonPagedPool, writeLen,BUFFER_SWAP_TAG );
        if (newBuf == NULL) 
            leave;

        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation because it is a waste of time since
        //  the FASTIO interface has no parameter for passing the MDL to the
        //  file system.
        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
	{
            newMdl = IoAllocateMdl( newBuf, writeLen, FALSE,FALSE, NULL );
            if (newMdl == NULL) 
                leave;
            MmBuildMdlForNonPagedPool( newMdl );
        }

        //  If the users original buffer had a MDL, get a system address.
        if (iopb->Parameters.Write.MdlAddress != NULL)
	{
            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Write.MdlAddress,  NormalPagePriority );
            if (origBuf == NULL)
	     {
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                FltStatus = FLT_PREOP_COMPLETE;
                leave;
            }
        } else {
            //  There was no MDL defined, use the given buffer address.
            origBuf = iopb->Parameters.Write.WriteBuffer;
        }

        //  Copy the memory, we must do this inside the try/except because we
        //  may be using a users buffer address
        try {			
		RtlCopyMemory( newBuf,origBuf,writeLen );

        	do {
        		// This judgement is useless in fact.
        		if (pStreamCtx->bIsFileCrypt) 
        		{
        			KeEnterCriticalRegion() ;
        			//加密，如果是指定进程则不加密，由指定进程加密
              		status = File_EncryptBuffer(
              		        newBuf,
              		        writeLen,
              		        NULL,
              		        &pStreamCtx->CryptIndex,
              		        iopb->Parameters.Write.ByteOffset.QuadPart);
              		if (NT_SUCCESS(status))
              		{
              		    KeLeaveCriticalRegion() ;
                                break;
              		}
                            KeLeaveCriticalRegion() ;
        		}
        	}while(FALSE) ;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  The copy failed, return an error, failing the operation.
            //
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
            FltStatus = FLT_PREOP_COMPLETE;
            leave;
        }

        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL)
            leave;


		// Set new buffer and Pass state to our post-operation callback.
        iopb->Parameters.Write.WriteBuffer = newBuf;
        iopb->Parameters.Write.MdlAddress = newMdl;
        //偏移，如果是指定进程则
        iopb->Parameters.Write.ByteOffset.QuadPart = writeOffset.QuadPart;
        FltSetCallbackDataDirty( Data );

        p2pCtx->SwappedBuffer = newBuf;
		p2pCtx->pStreamCtx = pStreamCtx ;
        *CompletionContext = p2pCtx;

        //  Return we want a post-operation callback
        FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } finally {		
	if (FLT_PREOP_SUCCESS_WITH_CALLBACK != FltStatus)
	{
		if (newBuf != NULL) 
			ExFreePoolWithTag( newBuf,BUFFER_SWAP_TAG );

		try {	

			if (newMdl != NULL)
			{
				MmUnlockPages(newMdl);//注释掉任何一个就会报：PFN_LIST_CORRUP，所以可以认为少释放了？
				IoFreeMdl( newMdl );
			}

		} except (EXCEPTION_EXECUTE_HANDLER) {

			KdPrint(("---------------EXCEPTION_EXECUTE_HANDLER---------------------\n"));//看是否还会蓝屏哈
		}
		
			
	}
	if (NULL != pStreamCtx)
		FltReleaseContext(pStreamCtx) ;
    }
    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
S_PostWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;

	UNREFERENCED_PARAMETER( FltObjects );
	UNREFERENCED_PARAMETER( Flags );

       if (!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
        {
            //在NTFS下，如果应用未设置大小，直接写

            //必须调用此才能将数据写回硬盘，否则，只有文件大小在变化，内容并未写到硬盘
            Cc_ClearFileCache(FltObjects->FileObject, TRUE, 
               &Data->Iopb->Parameters.Write.ByteOffset, Data->Iopb->Parameters.Write.Length);
        } else {
            /*  if (0 != Data->Iopb->Parameters.Write.ByteOffset.QuadPart % 4096)
                {
                      KdPrint(("Write:[BO=%I64d][Len=%d][WL=%d]\n",
                        Data->Iopb->Parameters.Write.ByteOffset.QuadPart + FILE_HEAD_LENGTH,
                        Data->Iopb->Parameters.Write.Length,
                        Data->IoStatus.Information));
                }*/
//try {	
              ExFreePoolWithTag( p2pCtx->SwappedBuffer,BUFFER_SWAP_TAG );
        	ExFreeToNPagedLookasideList( &Pre2PostContextList, p2pCtx );

//} except (EXCEPTION_EXECUTE_HANDLER) {

	//KdPrint(("---------------EXCEPTION_EXECUTE_HANDLER2---------------------\n"));//看是否还会蓝屏哈
//}
        }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
S_PreRead (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    NTSTATUS status;
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    ULONG readLen = iopb->Parameters.Read.Length;
	
    PVOID newBuf = NULL;
    PMDL newMdl  = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    BOOLEAN bOpenSec;

    PSTREAM_CONTEXT pStreamCtx = NULL ;

    KIRQL OldIrql ;

    BOOLEAN bRet= FALSE;
    
    if (!CanFilter) 
         return FLT_PREOP_SUCCESS_NO_CALLBACK;
    
    try {  
        //获取文件对象上下文
	status = Ctx_FindOrCreateStreamContext(Data,FltObjects,FALSE,&pStreamCtx,NULL) ;
	if (!NT_SUCCESS(status)) 
            leave ;
    
       if (!pStreamCtx->bIsFileCrypt) 
            leave ;
                  
	if (FLT_IS_FASTIO_OPERATION(Data))
	{
	    FltStatus = FLT_PREOP_DISALLOW_FASTIO ;
	    leave ;
	} 
        
       // 4为system进程，特殊处理
       if (PsGetCurrentProcessId() == 4)
       {
           if (!pStreamCtx->bOpenBySecProc)
                leave;
       } else {
           bOpenSec = S_IsCurProcSecEx(Data,NULL);
//            bRet = S_CheckProcCanGo(bOpenSec,PsGetCurrentProcessId(),pStreamCtx);
//            if (!bRet)
//            {
//                Data->IoStatus.Status = STATUS_ACCESS_DENIED;
//                FltStatus = FLT_PREOP_COMPLETE ;
//                leave;
//            }

           SC_LOCK(pStreamCtx, &OldIrql) ;
    	    pStreamCtx->bOpenBySecProc = bOpenSec;
    	    SC_UNLOCK(pStreamCtx, OldIrql) ;

           if (!bOpenSec)
                leave;
       }
    
        //如果Data->Iopb->IrpFlags内不含有IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO这三个标志，就退出
        if (!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO)))
	    leave ;
        
        if (readLen == 0) 
	    leave;

        // nocache read length must sector size aligned
        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) 
            readLen = (ULONG)ROUND_TO_SIZE(readLen,pStreamCtx->SectorSize);

        newBuf = ExAllocatePoolWithTag( NonPagedPool, readLen,BUFFER_SWAP_TAG );
        if (newBuf == NULL) 
            leave;
        RtlZeroMemory(newBuf, readLen);

        //
        //  We only need to build a MDL for IRP operations.  We don't need to
        //  do this for a FASTIO operation since the FASTIO interface has no
        //  parameter for passing the MDL to the file system.
        //
        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
        {
            newMdl = IoAllocateMdl( newBuf,readLen, FALSE,FALSE, NULL );
            if (newMdl == NULL)
                leave;
            MmBuildMdlForNonPagedPool( newMdl );//调用MmBuildMdlForNonPagedPool的主要作用是将MDL描述的物理页面集合映射到系统地址空间(4G虚拟地址空间的高2G部分)。
        }

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //
        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL)
            leave;
        
        // Update the buffer pointers and MDL address, mark we have changed something.
        iopb->Parameters.Read.ReadBuffer = newBuf;
        iopb->Parameters.Read.MdlAddress = newMdl;
        //偏移，如果是指定进程则不偏移
        iopb->Parameters.Read.ByteOffset.QuadPart += FILE_HEAD_LENGTH;
        FltSetCallbackDataDirty( Data );

        //  Pass state to our post-read operation callback.
        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->pStreamCtx = pStreamCtx ;
        *CompletionContext = p2pCtx;

        FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;

    } finally {

         if (FltStatus != FLT_PREOP_SUCCESS_WITH_CALLBACK) 
 	  {
            if (newBuf != NULL)
                ExFreePoolWithTag( newBuf,BUFFER_SWAP_TAG );

            if (newMdl != NULL) 
			{
				MmUnlockPages(newMdl);
				IoFreeMdl( newMdl );
			}

	     if (NULL != pStreamCtx)
		    FltReleaseContext(pStreamCtx) ;
       }
    }
    
    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
S_PostRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS ;
    FLT_POSTOP_CALLBACK_STATUS FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    PVOID origBuf;  
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    KIRQL OldIrql ;

    //
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
    //
    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));
    
    try {		
        //
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        //
        if (!NT_SUCCESS(Data->IoStatus.Status) ||  (Data->IoStatus.Information == 0))
            leave;

        //
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers.
        //

        if (iopb->Parameters.Read.MdlAddress != NULL) {//直接IO方式
			//根据R3的MDL所指向的物理内存页，映射到内核地址上。
            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress, NormalPagePriority );
            if (origBuf == NULL) 
	     {
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                leave;
            }

        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) 
	{
		origBuf = iopb->Parameters.Read.ReadBuffer;//缓冲区方式
        } else {
            //
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            //
            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   S_PostReadWhenSafe,
                                                   &FltStatus )) 
			{
                //
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                //
                cleanupAllocatedBuffer = FALSE;
            } else {
                //
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }
            leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //
        try {
	        do{
			// decrypt file data only if the file has been encrypted before 
			// or has been set decrypted on read
			KeEnterCriticalRegion() ;
			//如果是机密进程则解密数据到SwappedBuffer
			status = File_DecryptBuffer(p2pCtx->SwappedBuffer,//因为是同一个指针值newBuf  p2pCtx->SwappedBuffer = newBuf; & iopb->Parameters.Read.ReadBuffer = newBuf;
			        Data->IoStatus.Information,
			        NULL,
			        &p2pCtx->pStreamCtx->CryptIndex,
			        iopb->Parameters.Read.ByteOffset.QuadPart);
			if (NT_SUCCESS(status))
			{
    			    KeLeaveCriticalRegion() ;
                         break;
			}
                     KeLeaveCriticalRegion() ;			
		  }while(FALSE) ;	

		RtlCopyMemory( origBuf,
				p2pCtx->SwappedBuffer, 
				Data->IoStatus.Information );
              //S_UnicodeStringToChar(&p2pCtx->pStreamCtx->FileName,FN);
              //KdPrint(("ReadBuf:%s \n",(PCHAR)origBuf));
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
        }

    } finally {
        if (cleanupAllocatedBuffer) 
	 {
            ExFreePoolWithTag( p2pCtx->SwappedBuffer,BUFFER_SWAP_TAG );
			FltReleaseContext( p2pCtx->pStreamCtx) ;
            ExFreeToNPagedLookasideList( &Pre2PostContextList, p2pCtx );
        }
    }

    return FltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
S_PostReadWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    KIRQL OldIrql ;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);
    
    //  This is some sort of user buffer without a MDL, lock the user buffer
    //  so we can access it.  This will create a MDL for it.
    status = FltLockUserBuffer(Data);

    if (!NT_SUCCESS(status)) 
    {
        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;
    } else {
        //  Get a system address for this buffer.
        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress,
                                                NormalPagePriority );
        if (origBuf == NULL) 
	{
            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;
        } else {
    		 do{
    			// decrypt file data only if the file has been encrypted before 
    			// or has been set decrypted on read
    			KeEnterCriticalRegion() ;
    			//如果是机密进程则解密数据到SwappedBuffer
			status = File_DecryptBuffer(p2pCtx->SwappedBuffer,
			        Data->IoStatus.Information,
			        NULL,
			        &p2pCtx->pStreamCtx->CryptIndex,
			        iopb->Parameters.Read.ByteOffset.QuadPart);
			if (NT_SUCCESS(status))
			{
    			    KeLeaveCriticalRegion() ;
                         break;
			}
                     KeLeaveCriticalRegion() ;

    		  }while(FALSE) ;	

            //  Copy the data back to the original buffer.  Note that we
            //  don't need a try/except because we will always have a system
            //  buffer address.
            RtlCopyMemory( origBuf,p2pCtx->SwappedBuffer,Data->IoStatus.Information );
        }
    }

    ExFreePoolWithTag( p2pCtx->SwappedBuffer,BUFFER_SWAP_TAG );
    FltReleaseContext(p2pCtx->pStreamCtx) ;

    ExFreeToNPagedLookasideList( &Pre2PostContextList, p2pCtx );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
S_PreQueryInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	NTSTATUS status = STATUS_SUCCESS ;
	FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SYNCHRONIZE ;
	FILE_INFORMATION_CLASS FileInfoClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass ;

	PSTREAM_CONTEXT pStreamCtx = NULL ;
    
       BOOLEAN bOpenSec;
       KIRQL OldIrql ;
       
       if (!CanFilter) 
           return FLT_PREOP_SUCCESS_NO_CALLBACK;
       
	try{
		//get per-stream context
		status = Ctx_FindOrCreateStreamContext(Data,FltObjects,FALSE,&pStreamCtx,NULL) ;
		if (!NT_SUCCESS(status))
		{
			FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK ;
			leave ;
		}

		// if file has been encrypted, if not, Pass down directly, 
		// otherwise, go on to post-query routine
		if (!pStreamCtx->bIsFileCrypt)
		{
			FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK ;
			leave ;
		}

		if (FLT_IS_FASTIO_OPERATION(Data))
		{
			FltStatus = FLT_PREOP_DISALLOW_FASTIO ;
			leave ;
		}
        
               // 4为system进程，特殊处理
              if (PsGetCurrentProcessId() == 4)
              {
                   if (!pStreamCtx->bOpenBySecProc)
                        return FLT_PREOP_SUCCESS_NO_CALLBACK;
              } else {
                    bOpenSec = S_IsCurProcSecEx(Data,NULL);
                    SC_LOCK(pStreamCtx, &OldIrql) ;
            	      pStreamCtx->bOpenBySecProc = bOpenSec;
            	      SC_UNLOCK(pStreamCtx, OldIrql) ;

                   if (!bOpenSec)
                        return FLT_PREOP_SUCCESS_NO_CALLBACK;
              }

		if (FileInfoClass == FileAllInformation || 
			FileInfoClass == FileAllocationInformation ||
			FileInfoClass == FileEndOfFileInformation ||
			FileInfoClass == FileStandardInformation ||
			FileInfoClass == FilePositionInformation ||
			FileInfoClass == FileValidDataLengthInformation||
			FileInfoClass == FileNetworkOpenInformation)
		{
		      FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK ;
		} else {
                    FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK ;
		}
	}
	finally{
		if (NULL != pStreamCtx)
			FltReleaseContext(pStreamCtx) ;
	}

	return FltStatus ;
}

FLT_POSTOP_CALLBACK_STATUS
S_PostQueryInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	NTSTATUS status = STATUS_SUCCESS ;
	FLT_POSTOP_CALLBACK_STATUS FltStatus = FLT_POSTOP_FINISHED_PROCESSING ;
	FILE_INFORMATION_CLASS FileInfoClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass ;
	PVOID FileInfoBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer ;
	ULONG FileInfoLength = Data->IoStatus.Information ;
              
	switch (FileInfoClass)
	{
		case FileAllInformation:
		{
			PFILE_ALL_INFORMATION psFileAllInfo = (PFILE_ALL_INFORMATION)FileInfoBuffer ;
                     // 注意FileAllInformation，即使长度不够，
                     // 依然可以返回前面的字节。
                     // 需要注意的是返回的字节里是否包含了StandardInformation
                     // 这个可能影响文件的大小的信息。
                     if (Data->IoStatus.Information >= 
                         sizeof(FILE_BASIC_INFORMATION) + 
                         sizeof(FILE_STANDARD_INFORMATION))
                     {
                         psFileAllInfo->StandardInformation.EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
                         psFileAllInfo->StandardInformation.AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
                         if(Data->IoStatus.Information >= 
                             sizeof(FILE_BASIC_INFORMATION) + 
                             sizeof(FILE_STANDARD_INFORMATION) +
                             sizeof(FILE_INTERNAL_INFORMATION) +
                             sizeof(FILE_EA_INFORMATION) +
                             sizeof(FILE_ACCESS_INFORMATION) +
                             sizeof(FILE_POSITION_INFORMATION))
                         {
                             if(psFileAllInfo->PositionInformation.CurrentByteOffset.QuadPart >= FILE_HEAD_LENGTH)
                                psFileAllInfo->PositionInformation.CurrentByteOffset.QuadPart -= FILE_HEAD_LENGTH;
                         }
                     }
			break ;
		}
		
		case FileAllocationInformation:
		{
		      PFILE_ALLOCATION_INFORMATION psFileAllocInfo = (PFILE_ALLOCATION_INFORMATION)FileInfoBuffer;

                    ASSERT(psFileAllocInfo->AllocationSize.QuadPart >= FILE_HEAD_LENGTH);
		      psFileAllocInfo->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
		      break ;
		}
		case FileValidDataLengthInformation:
		{
			PFILE_VALID_DATA_LENGTH_INFORMATION psFileValidLengthInfo = (PFILE_VALID_DATA_LENGTH_INFORMATION)FileInfoBuffer ;
                     ASSERT(psFileValidLengthInfo->ValidDataLength.QuadPart >= FILE_HEAD_LENGTH);
		       psFileValidLengthInfo->ValidDataLength.QuadPart -= FILE_HEAD_LENGTH;
			break ;
		}
		case FileStandardInformation:
		{
			PFILE_STANDARD_INFORMATION psFileStandardInfo = (PFILE_STANDARD_INFORMATION)FileInfoBuffer ;
			//ASSERT(psFileStandardInfo->AllocationSize.QuadPart >= FILE_HEAD_LENGTH);
                     psFileStandardInfo->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;            
                     psFileStandardInfo->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
			break ;
		}
		case FileEndOfFileInformation:
		{
			PFILE_END_OF_FILE_INFORMATION psFileEndInfo = (PFILE_END_OF_FILE_INFORMATION)FileInfoBuffer ;
			ASSERT(psFileEndInfo->EndOfFile.QuadPart >= FILE_HEAD_LENGTH);
		       psFileEndInfo->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
			break ;
		}
		case FilePositionInformation:
		{
			PFILE_POSITION_INFORMATION psFilePosInfo = (PFILE_POSITION_INFORMATION)FileInfoBuffer ;
                     if(psFilePosInfo->CurrentByteOffset.QuadPart >= FILE_HEAD_LENGTH)
			    psFilePosInfo->CurrentByteOffset.QuadPart -= FILE_HEAD_LENGTH;
			break ;
		}
              case FileNetworkOpenInformation:
		{
			PFILE_NETWORK_OPEN_INFORMATION psFileNetOpenInfo = (PFILE_NETWORK_OPEN_INFORMATION)FileInfoBuffer ;
                     psFileNetOpenInfo->AllocationSize.QuadPart -= FILE_HEAD_LENGTH;
                     psFileNetOpenInfo->EndOfFile.QuadPart -= FILE_HEAD_LENGTH;
			break ;
		}
		default:
			ASSERT(FALSE) ;
	};

	return  FltStatus ;
}

FLT_PREOP_CALLBACK_STATUS
S_PreSetInfo (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    NTSTATUS status = STATUS_SUCCESS ;
    FILE_INFORMATION_CLASS FileInfoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
    PVOID FileInfoBuffer = Data->Iopb->Parameters.SetFileInformation.InfoBuffer ;
    FLT_PREOP_CALLBACK_STATUS FltPreStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PSTREAM_CONTEXT pStreamCtx = NULL ;
    
    KIRQL OldIrql ;
    BOOLEAN bOpenSec;

    UNREFERENCED_PARAMETER( CompletionContext );

    //PAGED_CODE();
           
    try{
	status = Ctx_FindOrCreateStreamContext(Data,FltObjects,FALSE,&pStreamCtx,NULL) ;
	if (!NT_SUCCESS(status))
	    leave ;
		
	// if file has been encrypted, if not, Pass down directly, 
	// otherwise, go on
	if (!pStreamCtx->bIsFileCrypt)
	    leave ;
        
       if (FLT_IS_FASTIO_OPERATION(Data))
       {
    	   FltPreStatus = FLT_PREOP_DISALLOW_FASTIO ;
          leave;
       }

       // 4为system进程，特殊处理
       if (PsGetCurrentProcessId() == 4)
       {
           if (!pStreamCtx->bOpenBySecProc)
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
       } else {
            bOpenSec = S_IsCurProcSecEx(Data,NULL);
           SC_LOCK(pStreamCtx, &OldIrql) ;
    	    pStreamCtx->bOpenBySecProc = bOpenSec;
    	    SC_UNLOCK(pStreamCtx, OldIrql) ;

           if (!bOpenSec)
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
       }
       
	if (!(FileInfoClass == FileAllInformation || 
	    FileInfoClass == FileAllocationInformation ||
	    FileInfoClass == FileEndOfFileInformation ||
	    FileInfoClass == FileStandardInformation ||
	    FileInfoClass == FilePositionInformation ||
	    FileInfoClass == FileValidDataLengthInformation))
	{
	    //KdPrint(("SetInfo:%d\n",FileInfoClass));
	    leave ;
	}
    
	switch(FileInfoClass)
	{
	    case FileAllInformation:
	    {
	        PFILE_ALL_INFORMATION psFileAllInfo = (PFILE_ALL_INFORMATION)FileInfoBuffer ;
               psFileAllInfo->PositionInformation.CurrentByteOffset.QuadPart += FILE_HEAD_LENGTH;
               psFileAllInfo->StandardInformation.EndOfFile.QuadPart += FILE_HEAD_LENGTH;
               psFileAllInfo->StandardInformation.AllocationSize.QuadPart += FILE_HEAD_LENGTH;  
		 break ;
	    }
	    case FileAllocationInformation:
	    {
	        PFILE_ALLOCATION_INFORMATION psFileAllocInfo = (PFILE_ALLOCATION_INFORMATION)FileInfoBuffer ;
               psFileAllocInfo->AllocationSize.QuadPart += FILE_HEAD_LENGTH;  
		 break ;
	    }
	    case FileEndOfFileInformation:
	    {// update file size on disk
	        PFILE_END_OF_FILE_INFORMATION psFileEndInfo = (PFILE_END_OF_FILE_INFORMATION)FileInfoBuffer ;
               psFileEndInfo->EndOfFile.QuadPart += FILE_HEAD_LENGTH;
		 break ;
	    }
	    case FileStandardInformation:
	    {
	        PFILE_STANDARD_INFORMATION psStandardInfo = (PFILE_STANDARD_INFORMATION)FileInfoBuffer ;
               psStandardInfo->EndOfFile.QuadPart += FILE_HEAD_LENGTH;
               psStandardInfo->AllocationSize.QuadPart += FILE_HEAD_LENGTH;
		 break ;
	    }
	    case FilePositionInformation:
	    {
	        PFILE_POSITION_INFORMATION psFilePosInfo = (PFILE_POSITION_INFORMATION)FileInfoBuffer ;
               psFilePosInfo->CurrentByteOffset.QuadPart += FILE_HEAD_LENGTH;
		 break ;
	    }
	    case FileValidDataLengthInformation:
	    {
	        PFILE_VALID_DATA_LENGTH_INFORMATION psFileValidInfo = (PFILE_VALID_DATA_LENGTH_INFORMATION)FileInfoBuffer ;
               psFileValidInfo->ValidDataLength.QuadPart += FILE_HEAD_LENGTH;
		 break ;
	    }
	};
    } finally {
	if (NULL != pStreamCtx)
	    FltReleaseContext(pStreamCtx) ;
    }

    return FltPreStatus;
}

FLT_PREOP_CALLBACK_STATUS
S_PreDirCtrl(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PPRE_2_POST_CONTEXT p2pCtx;
    NTSTATUS status;

    if (!CanFilter)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    if (!S_IsCurProcSecEx(Data,NULL))
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    try {
		//if fast io, forbid it
	if (FLT_IS_FASTIO_OPERATION(Data))
	{
	    FltStatus = FLT_PREOP_DISALLOW_FASTIO ;
	    leave ;
	}

		//if not dir query, skip it
	if ((iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY) ||
	    (iopb->Parameters.DirectoryControl.QueryDirectory.Length == 0))
		leave ;

       newBuf = ExAllocatePoolWithTag( NonPagedPool,iopb->Parameters.DirectoryControl.QueryDirectory.Length,BUFFER_SWAP_TAG );
       if (newBuf == NULL) 
            leave;

       if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) 
	{
		newMdl = IoAllocateMdl(newBuf, iopb->Parameters.DirectoryControl.QueryDirectory.Length,FALSE,FALSE,NULL );
		if (newMdl == NULL) 
			leave;
		MmBuildMdlForNonPagedPool( newMdl );
	}

        p2pCtx = ExAllocateFromNPagedLookasideList( &Pre2PostContextList );
        if (p2pCtx == NULL) 
            leave;

        //Update the buffer pointers and MDL address
        iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = newBuf;
        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //Pass state to our post-operation callback.
        p2pCtx->SwappedBuffer = newBuf;
        *CompletionContext = p2pCtx;

        FltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    } finally {
       if (FltStatus != FLT_PREOP_SUCCESS_WITH_CALLBACK) 
	{
            if (newBuf != NULL) 
                ExFreePool( newBuf );

            if (newMdl != NULL) 
			{
				MmUnlockPages(newMdl);
				IoFreeMdl( newMdl );
			}
        }
    }
    return FltStatus ;
}


FLT_POSTOP_CALLBACK_STATUS
S_PostDirCtrl(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS ;
    PVOID origBuf;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS FltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;

    FILE_INFORMATION_CLASS FileInfoClass = iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass ;

    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    try {

       if (!NT_SUCCESS(Data->IoStatus.Status) || (Data->IoStatus.Information == 0)) 
            leave;

        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers
       if (iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
	{
            origBuf = MmGetSystemAddressForMdlSafe( 
                    iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                    NormalPagePriority );
            if (origBuf == NULL)
	     {
                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                leave;
            }
        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) || 
            FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
	 {
            origBuf = iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
        } else {
            if (FltDoCompletionProcessingWhenSafe( Data, 
                    FltObjects,
                    CompletionContext,
                    Flags,
                    S_PostDirCtrlWhenSafe
                    ,&FltStatus )) 
	      {
                cleanupAllocatedBuffer = FALSE;
              } else {
                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
              }
              leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
	//  exception.
	//
	//  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
	//         length in the information field (it is sort) we are always
	//         going to copy the original buffer length.
	//
	try {		
            status = S_DoDirCtrl(Data,FltObjects,p2pCtx->SwappedBuffer,FileInfoClass);
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;
        }

    } finally {
        //
        //  If we are supposed to, cleanup the allocate memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        //
        if (cleanupAllocatedBuffer) 
	 {
            ExFreePool( p2pCtx->SwappedBuffer );
            ExFreeToNPagedLookasideList( &Pre2PostContextList,p2pCtx );
        }
    }
    return FltStatus;
}


FLT_POSTOP_CALLBACK_STATUS
S_PostDirCtrlWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf;
    NTSTATUS status;

    FILE_INFORMATION_CLASS  FileInfoClass = iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass ;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);

    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) 
    {
        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;
    } else {
        origBuf = MmGetSystemAddressForMdlSafe( 
                iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                NormalPagePriority );
        if (origBuf == NULL) 
	 {
            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;
        } else {
            status = S_DoDirCtrl(Data,FltObjects,p2pCtx->SwappedBuffer,FileInfoClass);

           RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );       
        }
    }

    ExFreePool( p2pCtx->SwappedBuffer );
    ExFreeToNPagedLookasideList( &Pre2PostContextList,
                                 p2pCtx );
    return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS
S_MiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
    KdPrint(("[mini-filter] S_MiniConnect\n"));
    //PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie );
    
    UserProcess = PsGetCurrentProcess();
    gClientPort = ClientPort;

    return STATUS_SUCCESS;
}

//user application Disconect
VOID
S_MiniDisconnect(
    __in_opt PVOID ConnectionCookie
   )
{		
    //PAGED_CODE();
    UNREFERENCED_PARAMETER( ConnectionCookie );
    KdPrint(("[mini-filter] S_MiniDisconnect\n"));
    
    //  Close our handle
    FltCloseClientPort( gFilterHandle, &gClientPort );
}

NTSTATUS
S_MiniMessage (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS ;
    PMSG_SEND_TYPE psSendType= NULL;
    ULONG uType  = 0;

	PMSG_SEND_SET_PROCESS_INFO pSetProcInfo = NULL;
	PPROC_INFO p = NULL;
		
    //PAGED_CODE();

    UNREFERENCED_PARAMETER( ConnectionCookie );
    UNREFERENCED_PARAMETER( OutputBuffer );
    UNREFERENCED_PARAMETER( OutputBufferSize );
    UNREFERENCED_PARAMETER( ReturnOutputBufferLength );

    KdPrint(("[mini-filter] S_MiniMessage\n"));

    try {
        //由于如果消息还在处理的时候，如果应用突然被关闭
        //会造成应用传入的InputBuffer内存无效，驱动访问这些内存错误
        //从而引起蓝屏，故这里拷贝一下
        psSendType = (PMSG_SEND_TYPE)ExAllocatePoolWithTag(NonPagedPool, InputBufferSize, NAME_TAG);
        RtlCopyMemory(psSendType, InputBuffer, InputBufferSize);
        
        uType  = psSendType->uSendType;
        
        if (InputBufferSize < sizeof(MSG_SEND_TYPE))
             return STATUS_BUFFER_TOO_SMALL ;

        switch (uType)
        {
        	case IOCTL_SET_PROCESS_INFO:
        	    {

					if(ChainHead==NULL)//只有第一次设置为全部开启时，才会读取文件内容来填充链表
					{
						status = CreateHead();
					}
				
					pSetProcInfo = (PMSG_SEND_SET_PROCESS_INFO)InputBuffer;

					p =(PPROC_INFO)ExAllocatePool(NonPagedPool, sizeof(PROC_INFO));// 分配节点的空间
					RtlZeroMemory(p,sizeof(PROCESS_INFO));

					strcpy(p->ProcessName,pSetProcInfo->sProcInfo.ProcessName);
					
					//Psi_AddProcessInfo(p->ProcessName, TRUE) ;

					InsertInfo(p);
					
					//END
					*ReturnOutputBufferLength = 0 ;
					break ;
					/*
                        //刷新进程信息
                        if (!S_SetProcInfo(psSendType))
                            status = STATUS_UNSUCCESSFUL;
                        *ReturnOutputBufferLength = 0 ;
                        break ;
					*/
        	    }
			case IOCTL_DEL_PROCESS_INFO:
				{
					if(ChainHead == NULL)
					{
						status = STATUS_CHAINHEAD_NULL;
						break;
					}

					FreeChain();
					break;
				}
        	case IOCTL_SET_FILTER_INFO:
        	    {
                        if (psSendType->uFlag == 1)
                            CanFilter = TRUE;
                        else
                            CanFilter = FALSE;
                        *ReturnOutputBufferLength = 0 ;
        		  break ;
        	    }
        	default:
        		break ;
        }
    } finally {
        if (NULL != psSendType)
            ExFreePool(psSendType);
    }

    return status ;
}

FLT_PREOP_CALLBACK_STATUS
PreClose (
		  __inout PFLT_CALLBACK_DATA Data,
		  __in PCFLT_RELATED_OBJECTS FltObjects,
		  __deref_out_opt PVOID *CompletionContext
		  )
{   
	NTSTATUS status = STATUS_SUCCESS ;
	FLT_PREOP_CALLBACK_STATUS FltStatus = FLT_PREOP_SUCCESS_NO_CALLBACK ;

	PSTREAM_CONTEXT pStreamCtx = NULL ;
	KIRQL OldIrql ;
	BOOLEAN bDirectory = FALSE ;

	BOOLEAN bIsSystemProcess = FALSE ;

	UNREFERENCED_PARAMETER( CompletionContext );

	//PAGED_CODE(); //comment this line to avoid IRQL_NOT_LESS_OR_EQUAL error when accessing stream context

	try{

		// verify file attribute, if directory, pass down directly
		File_GetFileStandardInfo(Data, FltObjects, NULL, NULL, &bDirectory) ;
		if (bDirectory)
		{
			__leave ;
		}	

		
		if(!S_IsCurProcSecEx(Data,NULL))
		{
			__leave ;
		}

		// retrieve stream context
		status = Ctx_FindOrCreateStreamContext(Data, FltObjects,FALSE, &pStreamCtx, NULL) ;
		if (!NT_SUCCESS(status))
		{
			__leave ;
		}

		//current process monitored or not
		//if (!Ps_IsCurrentProcessMonitored(pStreamCtx->FileName.Buffer, 
			//pStreamCtx->FileName.Length/sizeof(WCHAR), &bIsSystemProcess, NULL))
		//{// not monitored, pass
			//__leave ;
		//}		

		SC_LOCK(pStreamCtx, &OldIrql) ;

		// if it is a stream file object, we don't care about it and don't decrement on reference count
		// since this object is opened by other kernel component
		//if ((FltObjects->FileObject->Flags & FO_STREAM_FILE) != FO_STREAM_FILE)
		//{
			//pStreamCtx->RefCount -- ; // decrement reference count
		//}

		SC_UNLOCK(pStreamCtx, OldIrql) ;
	
	}
	finally{

		if (NULL != pStreamCtx)
			FltReleaseContext(pStreamCtx) ;		
	}	

	return FltStatus;
}