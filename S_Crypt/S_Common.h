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

//������
#define CRYPT_INDEX_BIT 1

#define FILE_FLAG_INFO_LEN sizeof(FILE_FLAG_INFO)
#define FILE_FLAG_LENGTH  sizeof(FILE_FLAG)
#define FILE_HEAD_LENGTH PAGE_SIZE

#define SECPROC_MAXCOUNT    8

//ͷ��������
//��־16λ
#define FILE_GUID_LENGTH  16
//�����㷨������
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
	//�ļ����ھ��������С
    USHORT SectorSize;
    //�����㷨������
    USHORT CryptIndex;
    //�Ƿ񱻻��ܽ��̴�
    BOOLEAN bOpenBySecProc;
    //�ϴ�д�Ƿ���ܽ���
    BOOLEAN bWriteBySecProc;
    //������ļ��Ļ��ܽ���ID��������8�����ܽ���ͬʱ�򿪴��ļ�
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
