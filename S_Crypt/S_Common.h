#ifndef _COMMON_H_
#define _COMMON_H_

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <Ntifs.h>
#include "S_IOCommon.h"

#ifndef MAX_PATH
#define MAX_PATH 260 
#endif 

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512
#endif

#ifndef DBG
#define DBG 1
#endif

struct FILE_FLAG_INFO ;
struct FILE_FLAG ;

//异或加密
#define CRYPT_INDEX_BIT 1

#define FILE_FLAG_INFO_LEN sizeof(FILE_FLAG_INFO)
#define FILE_FLAG_LENGTH  sizeof(FILE_FLAG)
#define FILE_HEAD_LENGTH PAGE_SIZE

#define SECPROC_MAXCOUNT    8

//头部各域定义
//标志16位
#define FILE_GUID_LENGTH  16
//加密算法索引号
#define CRYPT_FUNC_INDEX sizeof(UCHAR)

/****************************************************************************/
/*                        Constant definition                               */
/****************************************************************************/
static UCHAR gc_sGuid[FILE_GUID_LENGTH] = { \
	                    0x7a, 0x4d, 0x2c, 0xd1, 0x3b, 0x2f, 0x97, 0x9f, \
					    0x7d, 0x6b, 0x3f, 0x6b, 0x74, 0x9c, 0x7e, 0x7b} ;

typedef struct _FILE_FLAG_INFO{

	/**
	 * This field holds GUID, GUID is used to distinguish encrypted 
	 * files and un-encrypted files. All encrypted files S_ 
	 * the same GUID that described in file.c
	 */
	UCHAR    FileFlagHeader[FILE_GUID_LENGTH] ; 
       UCHAR    CryptIndex;
}FILE_FLAG_INFO,*PFILE_FLAG_INFO;

/**
 * File Flag Structure
 * is written into end of file every time when file is closed. 
 */
typedef struct _FILE_FLAG{

	/**
	 * This field holds GUID, GUID is used to distinguish encrypted 
	 * files and un-encrypted files. All encrypted files S_ 
	 * the same GUID that described in file.c
	 */
	FILE_FLAG_INFO   FileFlagInfo;  
	
	/**
	 * For further usage and sector size alignment.
	 */
	UCHAR    Reserved[FILE_HEAD_LENGTH-FILE_FLAG_INFO_LEN] ;

}FILE_FLAG,*PFILE_FLAG;

#define FS_NAME_LENGTH 6*sizeof(WCHAR) // useless now

//
//  Stream context data structure
//



typedef struct _STREAM_CONTEXT {
	//文件所在卷的扇区大小
    USHORT SectorSize;
    //加密算法索引号
    USHORT CryptIndex;
    //是否被机密进程打开
    BOOLEAN bOpenBySecProc;
    //上次写是否机密进程
    BOOLEAN bWriteBySecProc;
    //打开这个文件的机密进程ID，最多可以8个机密进程同时打开此文件
    HANDLE uOpenSecProcID[SECPROC_MAXCOUNT];

	//Flags
    BOOLEAN bIsFileCrypt ;    //(init false)set after file flag is written into end of file

	LONG RefCount;

    //Lock used to protect this context.
    PERESOURCE Resource;

	//Spin lock used to protect this context when irql is too high
	KSPIN_LOCK Resource1 ;

	PFSRTL_ADVANCED_FCB_HEADER Stream;

} STREAM_CONTEXT, *PSTREAM_CONTEXT;

#define STREAM_CONTEXT_SIZE sizeof(STREAM_CONTEXT)

#endif
