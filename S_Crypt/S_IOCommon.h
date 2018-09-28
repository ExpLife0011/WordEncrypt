#ifndef _IOCOMMON_H_
#define _IOCOMMON_H_

//maximum path length
#ifndef 	MAX_PATH
#define 	MAX_PATH					260
#endif

// max user name length
#define  MAX_USER_NAME_LENGTH			128

//password length limitation(in bytes)
#define  MAX_PASSWORD_LENGTH			20
#define  MIN_PASSWORD_LENGTH			4

//maximum key length
#define MAX_KEY_LENGTH  32

//password question length limitation(in bytes)
#define  MAX_SECRETQUESTION_LENGTH		64
#define  MAX_SECRETANSWER_LENGTH		32
#define  MAX_PASSWORDHINT_LENGTH		32

#define  HASH_SIZE  20
#define  SECTION_SIZE 512

typedef void* HANDLE ;

typedef enum {OFF, ON}STATE ;

#pragma pack(1)

#pragma pack()

typedef void (*GETRESULTCALLBACK)(PVOID pUserParam, TCHAR* pszProcessPathName) ;

#endif