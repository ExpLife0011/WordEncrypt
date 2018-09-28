/****************************************************************************/
/*                             User include                                 */
/****************************************************************************/
#include "S_File.h"


/****************************************************************************/
/*                        Routine definition                               */
/****************************************************************************/

static
NTSTATUS File_ReadWriteFileComplete(
    PDEVICE_OBJECT dev,
    PIRP irp,
    PVOID context
    )
{
	PMDL mdl = NULL;
	*irp->UserIosb = irp->IoStatus;//为了返回后的这句：*BytesReadWrite = IoStatusBlock.Information;
	if (irp->PendingReturned == TRUE)
	{		
		KeSetEvent(irp->UserEvent, 0, FALSE);
	}	
	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
File_ReadWriteFile(
	    __in ULONG MajorFunction,
	    __in PFLT_INSTANCE Instance,
		__in PFILE_OBJECT FileObject,
		__in PLARGE_INTEGER ByteOffset,
		__in ULONG Length,
		__in PVOID  Buffer,
		__out PULONG BytesReadWrite,
		__in FLT_IO_OPERATION_FLAGS FltFlags
		)
{
	PMDL mdl = NULL;
	ULONG i;
        PIRP irp;
        KEVENT Event;
        PIO_STACK_LOCATION ioStackLocation;
	IO_STATUS_BLOCK IoStatusBlock = { 0 };

	PDEVICE_OBJECT pVolumeDevObj = NULL ;
	PDEVICE_OBJECT pFileSysDevObj= NULL ;
	PDEVICE_OBJECT pNextDevObj = NULL ;

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	//获取minifilter相邻下层的设备对象
	pVolumeDevObj = IoGetDeviceAttachmentBaseRef(FileObject->DeviceObject) ;
	if (NULL == pVolumeDevObj)
	{
		return STATUS_UNSUCCESSFUL ;
	}

       //共享路径没有这个值，故这里需要判断一下，也就是说共享读取写入目前不支持
       if (NULL == pVolumeDevObj->Vpb)
       {
		return STATUS_UNSUCCESSFUL ;
	}
       
	pFileSysDevObj = pVolumeDevObj->Vpb->DeviceObject ;
	pNextDevObj = pFileSysDevObj ;

	if (NULL == pNextDevObj)
	{
		ObDereferenceObject(pVolumeDevObj) ;
		return STATUS_UNSUCCESSFUL ;
	}

	//开始构建读写IRP
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

	// 分配irp.
    irp = IoAllocateIrp(pNextDevObj->StackSize, FALSE);
    if(irp == NULL) {
		ObDereferenceObject(pVolumeDevObj) ;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
  
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->MdlAddress = NULL;
    irp->UserBuffer = Buffer;
    irp->UserEvent = &Event;
    irp->UserIosb = &IoStatusBlock;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;
	if(MajorFunction == IRP_MJ_READ)
		irp->Flags = IRP_DEFER_IO_COMPLETION|IRP_READ_OPERATION|IRP_NOCACHE;
	else if (MajorFunction == IRP_MJ_WRITE)
		irp->Flags = IRP_DEFER_IO_COMPLETION|IRP_WRITE_OPERATION|IRP_NOCACHE;
	else
	{
		ObDereferenceObject(pVolumeDevObj) ;
		return STATUS_UNSUCCESSFUL ;
	}

	if ((FltFlags & FLTFL_IO_OPERATION_PAGING) == FLTFL_IO_OPERATION_PAGING)//这是一个分页操作
	{
		irp->Flags |= IRP_PAGING_IO ;////表示此时执行内存页的I/O操作
	}

	// 填写irpsp
    ioStackLocation = IoGetNextIrpStackLocation(irp);
	ioStackLocation->MajorFunction = (UCHAR)MajorFunction;
    ioStackLocation->MinorFunction = (UCHAR)IRP_MN_NORMAL;
    ioStackLocation->DeviceObject = pNextDevObj;
    ioStackLocation->FileObject = FileObject ;
	if(MajorFunction == IRP_MJ_READ)
	{
		ioStackLocation->Parameters.Read.ByteOffset = *ByteOffset;
		ioStackLocation->Parameters.Read.Length = Length;
	}
	else
	{
		ioStackLocation->Parameters.Write.ByteOffset = *ByteOffset;
		ioStackLocation->Parameters.Write.Length = Length ;
	}

	// 设置完成
    IoSetCompletionRoutine(irp,File_ReadWriteFileComplete, 0, TRUE, TRUE, TRUE);
    
	status = IoCallDriver(pNextDevObj, irp);
	if (status == STATUS_PENDING)  //若底层驱动返回的是STATUS_SUCCESS，则这里不会执行，也不会永远等待
	{
		KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);		
	}
	
	*BytesReadWrite = IoStatusBlock.Information;		
	status = IoStatusBlock.Status;

	ObDereferenceObject(pVolumeDevObj) ;

	//虽然在底层驱动已经将IRP完成了，但是由于完成例程返回的是STATUS_MORE_PROCESSING_REQUIRED,因此需要再次调用IoCompleteRequest!
	IoCompleteRequest(irp, IO_NO_INCREMENT);// 由于完成函数中返回了STATUS_MORE_PROCESSING_REQUIRED，所以要再次调用IoCompleteRequest！	

	return status;
}

NTSTATUS 
File_GetFileOffset(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__out PLARGE_INTEGER FileOffset
	)
{
	NTSTATUS status;
	FILE_POSITION_INFORMATION NewPos;	

	//修改为向下层Call
	status = FltQueryInformationFile(FltObjects->Instance,
									 FltObjects->FileObject,
									 &NewPos,
									 sizeof(FILE_POSITION_INFORMATION),
									 FilePositionInformation,
									 NULL
									 ) ;
	if(NT_SUCCESS(status))
	{
		FileOffset->QuadPart = NewPos.CurrentByteOffset.QuadPart;
	}

	return status;
}


NTSTATUS File_SetFileOffset(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileOffset
	)
{
	NTSTATUS status;
	FILE_POSITION_INFORMATION NewPos;
	//修改为向下层Call
	LARGE_INTEGER NewOffset = {0};

	NewOffset.QuadPart = FileOffset->QuadPart;
	NewOffset.LowPart = FileOffset->LowPart;

	NewPos.CurrentByteOffset = NewOffset;

	status = FltSetInformationFile(FltObjects->Instance,
								   FltObjects->FileObject,
								   &NewPos,
								   sizeof(FILE_POSITION_INFORMATION),
								   FilePositionInformation
								   ) ;
	return status;
}


NTSTATUS 
File_SetFileSize(
    __in PFLT_CALLBACK_DATA Data,
	__in PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileSize
	)
{
	NTSTATUS status = STATUS_SUCCESS ;
	FILE_END_OF_FILE_INFORMATION EndOfFile;
	PFSRTL_COMMON_FCB_HEADER Fcb = (PFSRTL_COMMON_FCB_HEADER)FltObjects->FileObject->FsContext ;;

	EndOfFile.EndOfFile.QuadPart = FileSize->QuadPart;
	EndOfFile.EndOfFile.LowPart = FileSize->LowPart;

	//修改为向下层Call
	status = FltSetInformationFile(FltObjects->Instance,
		FltObjects->FileObject,
		&EndOfFile,
		sizeof(FILE_END_OF_FILE_INFORMATION),
		FileEndOfFileInformation
		) ;

	return status;	
}

NTSTATUS 
File_GetFileSize(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileSize
	)
{
	NTSTATUS status;
	FILE_STANDARD_INFORMATION fileInfo ;

	//修改为向下层Call
	status = FltQueryInformationFile(FltObjects->Instance,
									 FltObjects->FileObject,
									 &fileInfo,
									 sizeof(FILE_STANDARD_INFORMATION),
									 FileStandardInformation,
									 NULL
									 ) ;
	if (NT_SUCCESS(status))
	{
		FileSize->QuadPart = fileInfo.EndOfFile.QuadPart ;
	}
	else
	{
		FileSize->QuadPart = 0 ;
	}
	return status;
}

NTSTATUS 
File_GetFileStandardInfo(
	__in  PFLT_CALLBACK_DATA Data,
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
									 FltObjects->FileObject,
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

NTSTATUS
File_InitFileFlag()
{       
	if (NULL != g_psFileFlag)
		return STATUS_SUCCESS ;

	g_psFileFlag = ExAllocatePoolWithTag(NonPagedPool, FILE_FLAG_LENGTH, FILEFLAG_POOL_TAG) ;
	if (NULL == g_psFileFlag)
	{
		return STATUS_INSUFFICIENT_RESOURCES ;
	}
	RtlZeroMemory(g_psFileFlag, FILE_FLAG_LENGTH) ;

	RtlCopyMemory(g_psFileFlag->FileFlagInfo.FileFlagHeader, gc_sGuid, FILE_GUID_LENGTH) ;
       g_psFileFlag->FileFlagInfo.CryptIndex = CRYPT_INDEX_BIT;
              
	return STATUS_SUCCESS ;
}

NTSTATUS
File_UninitFileFlag()
{
	if (NULL != g_psFileFlag)
	{
		ExFreePoolWithTag(g_psFileFlag, FILEFLAG_POOL_TAG) ;
		g_psFileFlag = NULL ;
	}

	return STATUS_SUCCESS ;
}

NTSTATUS
File_EncryptBuffer(
    __in PVOID buffer,
    __in ULONG Length,
    __in PCHAR PassWord,
    __inout PUSHORT CryptIndex,
    __in ULONG ByteOffset
    )
{
    PCHAR buf;
    UINT64 i;

    UNREFERENCED_PARAMETER(PassWord);//作用：告诉编译器，已经使用了该变量，不必检测警告！

    buf = (PCHAR)buffer;

    if (NULL != CryptIndex)
    {
        if (*CryptIndex == CRYPT_INDEX_BIT)
        {
            for(i=0;i<Length;i++)
            {
                buf[i] ^= ENC_BUFF_KEY_BIT;
				//buf[i] = '$';
            }
        } else {
            //其他加密算法
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
File_DecryptBuffer(
    __in PVOID buffer,
    __in ULONG Length,
    __in PCHAR PassWord,
    __inout PUSHORT CryptIndex,
    __in ULONG ByteOffset
)
{
    PCHAR buf;
    UINT64 i;

    UNREFERENCED_PARAMETER(PassWord);//作用：告诉编译器，已经使用了该变量，不必检测警告！

    buf = (PCHAR)buffer;

    if (NULL != CryptIndex)
    {
        if (*CryptIndex == CRYPT_INDEX_BIT)
        {
            for(i=0;i<Length;i++)
            {
				buf[i] ^= ENC_BUFF_KEY_BIT;				
            }
        } else {
            //其他加密算法
        }
    }    
    return STATUS_SUCCESS;
}