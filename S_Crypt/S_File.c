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
	*irp->UserIosb = irp->IoStatus;//Ϊ�˷��غ����䣺*BytesReadWrite = IoStatusBlock.Information;
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

	//��ȡminifilter�����²���豸����
	pVolumeDevObj = IoGetDeviceAttachmentBaseRef(FileObject->DeviceObject) ;
	if (NULL == pVolumeDevObj)
	{
		return STATUS_UNSUCCESSFUL ;
	}

       //����·��û�����ֵ����������Ҫ�ж�һ�£�Ҳ����˵�����ȡд��Ŀǰ��֧��
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

	//��ʼ������дIRP
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

	// ����irp.
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

	if ((FltFlags & FLTFL_IO_OPERATION_PAGING) == FLTFL_IO_OPERATION_PAGING)//����һ����ҳ����
	{
		irp->Flags |= IRP_PAGING_IO ;////��ʾ��ʱִ���ڴ�ҳ��I/O����
	}

	// ��дirpsp
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

	// �������
    IoSetCompletionRoutine(irp,File_ReadWriteFileComplete, 0, TRUE, TRUE, TRUE);
    
	status = IoCallDriver(pNextDevObj, irp);
	if (status == STATUS_PENDING)  //���ײ��������ص���STATUS_SUCCESS�������ﲻ��ִ�У�Ҳ������Զ�ȴ�
	{
		KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);		
	}
	
	*BytesReadWrite = IoStatusBlock.Information;		
	status = IoStatusBlock.Status;

	ObDereferenceObject(pVolumeDevObj) ;

	//��Ȼ�ڵײ������Ѿ���IRP����ˣ���������������̷��ص���STATUS_MORE_PROCESSING_REQUIRED,�����Ҫ�ٴε���IoCompleteRequest!
	IoCompleteRequest(irp, IO_NO_INCREMENT);// ������ɺ����з�����STATUS_MORE_PROCESSING_REQUIRED������Ҫ�ٴε���IoCompleteRequest��	

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

	//�޸�Ϊ���²�Call
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
	//�޸�Ϊ���²�Call
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

	//�޸�Ϊ���²�Call
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

	//�޸�Ϊ���²�Call
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

	//�޸�Ϊ���²�Call
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

    UNREFERENCED_PARAMETER(PassWord);//���ã����߱��������Ѿ�ʹ���˸ñ��������ؼ�⾯�棡

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
            //���������㷨
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

    UNREFERENCED_PARAMETER(PassWord);//���ã����߱��������Ѿ�ʹ���˸ñ��������ؼ�⾯�棡

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
            //���������㷨
        }
    }    
    return STATUS_SUCCESS;
}