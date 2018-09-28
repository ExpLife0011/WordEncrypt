#include "S_Cache.h"

void Cc_ClearFileCache(PFILE_OBJECT FileObject, BOOLEAN bIsFlushCache, PLARGE_INTEGER FileOffset, ULONG Length)
{
	BOOLEAN PurgeRes ;
	BOOLEAN ResourceAcquired = FALSE ;
	BOOLEAN PagingIoResourceAcquired = FALSE ;
	PFSRTL_COMMON_FCB_HEADER Fcb = NULL ;
	LARGE_INTEGER Delay50Milliseconds = {(ULONG)(-50 * 1000 * 10), -1};
	IO_STATUS_BLOCK IoStatus = {0} ;

	if ((FileObject == NULL))
	{
		return ;
	}
	//FCB��File Control Block�����ļ����ƿ飬�洢�ļ��ڴ����е������Ϣ��
	//Ӧ����Createʱ�����ġ��ļ���fcbһһ��Ӧ��һ��FCB����һ���ļ�Ŀ¼�����һ���ļ�ʱ�����ļ�Ŀ¼�����һ���µ�Ŀ¼���Fcb
    Fcb = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext ;
	if (Fcb == NULL)
	{
		return ;
	}

Acquire:

	FsRtlEnterFileSystem() ;

	/*
	FsRtlEnterFileSystem(KeEnterCriticalRegion)ʹ��˵��   .
	�����ļ�ϵͳ��������ʱ���ᾭ���õ�FsRtlEnterFileSystem����
	��ʲô�ط����ã�
	ÿһ���ļ�ϵͳ���������ڽ���һ���ļ���д����ʱ����Ҫ�ȵ���FsRtlEnterFileSystem��ͬʱ���������ʱ��Ҫ��������FsRtlExitFileSystem��
	���ļ�ϵͳ�������������ϲ��õ���FsRtlEnterFileSystem��ֻ���������ExAcquireResourceExclusive(Lite)ʱ������Ҫ���ã�ͬʱ�ڵ�����ExReleaseResource��Ҫ��������FsRtlExitFileSystem��
	���õ�Ŀ�ģ�
	��IRQL Level��ߵ� APC_LEVEL������ֹһ����ں�APC���̡�
	ע�����
	�谡Ӵ��IRQL PASSIVE_LEVEL�����ϵ��á�
	*/

	if (Fcb->Resource)
		ResourceAcquired = ExAcquireResourceExclusiveLite(Fcb->Resource, TRUE) ;
	if (Fcb->PagingIoResource)
		PagingIoResourceAcquired = ExAcquireResourceExclusive(Fcb->PagingIoResource,FALSE);
	else
		PagingIoResourceAcquired = TRUE ;
	if (!PagingIoResourceAcquired)
	{
		if (Fcb->Resource)  ExReleaseResource(Fcb->Resource);
		FsRtlExitFileSystem();
		//KeDelayExecutionThread(KernelMode,FALSE,&Delay50Milliseconds);	
		goto Acquire;	
	}

	if(FileObject->SectionObjectPointer)//���Ӧ�����ж��ļ��Ƿ�ӳ����
	{
		IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

		if (bIsFlushCache)
		{
			//�������Ӧ����ɵ��ǽ�����д�ش��̣��������Ӧ�������������ϵ��ļ����ݡ�
			CcFlushCache( FileObject->SectionObjectPointer, FileOffset, Length, &IoStatus );//The CcFlushCache routine flushes all or a portion of a cached file to disk.
		}

		/* ���Ӣ��˵���˵� DataSectionObject & ImageSectionObject��Ϊ��ʱ���ļ���ӳ�䵽��ĳ���ط�������Ҫ������صĺ��������ͷš�
				Memory-mapping is handled entirely by the Memory Manager. 
				The Cache Manager uses services provided by the Memory Manager, not vice-versa. 
				A file can quite easily have valid pointers in the 
				FileObject->SectionObjectPointer->DataSectionObject or FileObject->SectionObjectPointer->ImageSectionObject fields 
				(which means that the file may be mapped somewhere) but have a NULL pointer at FileObject->SectionObjectPointer->SharedCacheMap (whichs means that the file is not cached by Cc)
		*/

		if(FileObject->SectionObjectPointer->ImageSectionObject)//Ϊ��ʱ˵���ļ���ӳ�䵽��ĳ���ط�
		{
			//��ɾ���ļ��Ĺ�����Ҫ������������ж��ļ��������Ƿ��Ѿ��ͷţ�����ļ���������û���ͷţ���������᷵��False�����ļ��޷�ɾ����
			MmFlushImageSection(
				FileObject->SectionObjectPointer,
				MmFlushForWrite//The file is being opened for write access. 
				) ;
		}

		if(FileObject->SectionObjectPointer->DataSectionObject)
		{ 

			//FileObject��SectionObjectPointer��DataSectionObject��Ա��ΪNULL��Ӧ�õ��� CcPurgeCacheSection �������������������������ݡ�
			PurgeRes = CcPurgeCacheSection( FileObject->SectionObjectPointer,
				NULL,
				0,
				FALSE );                                                    
		}                                      
		IoSetTopLevelIrp(NULL);                                   
	}

	if (Fcb->PagingIoResource)
		ExReleaseResourceLite(Fcb->PagingIoResource );                                       
	if (Fcb->Resource)
		ExReleaseResourceLite(Fcb->Resource );                     

	FsRtlExitFileSystem() ;	
}

