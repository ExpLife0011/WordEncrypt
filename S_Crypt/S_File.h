#ifndef _FILENAME_H_
#define _FILENMAE_H_

#include "S_Common.h"

//#define FILEFLAG_POOL_TAG 'FASV'
#define FILEFLAG_POOL_TAG 'AA11'
#define ENC_BUFF_KEY_BIT 0x77

#pragma pack(1)

/**
 * declared global file flag infomation,
 * it is used to compare with file flag of open file
 * to judge whether specified file are encrypted or
 * not.
 */
PFILE_FLAG g_psFileFlag;

#pragma pack()

/**
 * read data from specified file by calling underlying file system
 */
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
		);

/**
 * Get file pointer
 */
NTSTATUS 
File_GetFileOffset(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__out PLARGE_INTEGER FileOffset
	) ;

/**
 * Set file pointer
 */
NTSTATUS 
File_SetFileOffset(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileOffset
	) ;

/**
 * Set file size
 */
NTSTATUS 
File_SetFileSize(
	__in PFLT_CALLBACK_DATA Data,
    __in PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileSize
	) ;

/**
 * Get file size, returned size is summation of  
 * file data length, padding length and file flag length
 */
NTSTATUS 
File_GetFileSize(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileSize
	) ;

/**
 * Get file information
 */
NTSTATUS 
File_GetFileStandardInfo(
	__in  PFLT_CALLBACK_DATA Data,
	__in  PFLT_RELATED_OBJECTS FltObjects,
	__in PLARGE_INTEGER FileAllocationSize,
	__in PLARGE_INTEGER FileSize,
	__in PBOOLEAN bDirectory
	) ;
/**
 * allocate memory for global file flag information,
 * init global file flag and fill GUID in it.
 */
NTSTATUS
File_InitFileFlag() ;

/**
 * deallocate memory occupied by global file flag,
 * when engine is unloaded.
 */
NTSTATUS
File_UninitFileFlag() ;

/**
 * 加密缓存内容
 *buffer:指向缓存的指针
 *Length:缓存长度
 *PassWord:密码，目前未使用
 *CryptIndex:加密解密算法索引号
 */
NTSTATUS
File_EncryptBuffer(
    __in PVOID buffer,
    __in ULONG Length,
    __in PCHAR PassWord,
    __inout PUSHORT CryptIndex,
    __in ULONG ByteOffset
);

/**
 * 解密缓存内容
  *buffer:指向缓存的指针
 *Length:缓存长度
 *PassWord:密码，目前未使用
 *CryptIndex:加密解密算法索引号
 */
NTSTATUS
File_DecryptBuffer(
    __in PVOID buffer,
    __in ULONG Length,
    __in PCHAR PassWord,
    __inout PUSHORT CryptIndex,
    __in ULONG ByteOffset
);

int File_InitKeyWords(
    CHAR *KeyWords,
    ULONG KeyLen
    );
    

#endif
