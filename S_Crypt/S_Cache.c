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
	//FCB（File Control Block），文件控制块，存储文件在磁盘中的相关信息；
	//应该是Create时创建的。文件和fcb一一对应，一个FCB即是一个文件目录项。创建一个文件时就往文件目录里添加一项新的目录项，即Fcb
    Fcb = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext ;
	if (Fcb == NULL)
	{
		return ;
	}

Acquire:

	FsRtlEnterFileSystem() ;

	/*
	FsRtlEnterFileSystem(KeEnterCriticalRegion)使用说明   .
	在做文件系统过滤驱动时，会经常用到FsRtlEnterFileSystem函数
	在什么地方调用？
	每一个文件系统驱动例程在进行一个文件读写请求时，需要先调用FsRtlEnterFileSystem，同时在完成请求时，要立即调用FsRtlExitFileSystem。
	而文件系统过滤驱动基本上不用调用FsRtlEnterFileSystem，只有在你调用ExAcquireResourceExclusive(Lite)时，才需要调用，同时在调用完ExReleaseResource后，要立即调用FsRtlExitFileSystem。
	调用的目的？
	把IRQL Level提高到 APC_LEVEL，来禁止一般的内核APC例程。
	注意事项？
	需啊哟在IRQL PASSIVE_LEVEL级别上调用。
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

	if(FileObject->SectionObjectPointer)//这个应该是判断文件是否被映射了
	{
		IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

		if (bIsFlushCache)
		{
			//这个函数应该完成的是将数据写回磁盘，而下面的应该是清除缓冲段上的文件数据。
			CcFlushCache( FileObject->SectionObjectPointer, FileOffset, Length, &IoStatus );//The CcFlushCache routine flushes all or a portion of a cached file to disk.
		}

		/* 这段英文说明了当 DataSectionObject & ImageSectionObject不为空时该文件被映射到了某个地方，所以要调用相关的函数进行释放。
				Memory-mapping is handled entirely by the Memory Manager. 
				The Cache Manager uses services provided by the Memory Manager, not vice-versa. 
				A file can quite easily have valid pointers in the 
				FileObject->SectionObjectPointer->DataSectionObject or FileObject->SectionObjectPointer->ImageSectionObject fields 
				(which means that the file may be mapped somewhere) but have a NULL pointer at FileObject->SectionObjectPointer->SharedCacheMap (whichs means that the file is not cached by Cc)
		*/

		if(FileObject->SectionObjectPointer->ImageSectionObject)//为真时说明文件被映射到了某个地方
		{
			//在删除文件的过程中要调用这个函数判定文件在运行是否已经释放，如果文件在运行中没有释放，这个函数会返回False，让文件无法删除。
			MmFlushImageSection(
				FileObject->SectionObjectPointer,
				MmFlushForWrite//The file is being opened for write access. 
				) ;
		}

		if(FileObject->SectionObjectPointer->DataSectionObject)
		{ 

			//FileObject的SectionObjectPointer的DataSectionObject成员不为NULL，应该调用 CcPurgeCacheSection 函数清除缓存管理器产生的数据。
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

