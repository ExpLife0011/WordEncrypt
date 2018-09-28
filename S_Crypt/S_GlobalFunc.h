#include <ntddk.h>

#define S_MM_TAG                            "SCMM"
#define DEFAULT_FILE_NAME_LENGTH     260

#ifndef _S_GlobalFunc_HEADER_
#define _S_GlobalFunc_HEADER_

//��ʼ������UnicodeString
NTSTATUS
S_AllocateUnicodeString (
    __inout PUNICODE_STRING pUS
    );

//�ͷ�UnicodeString
VOID
S_FreeUnicodeString (
    __inout PUNICODE_STRING pUS
    );

//��UnicodeStringת��ΪChar����
BOOLEAN 
S_UnicodeStringToChar(
    PUNICODE_STRING UniName, 
    CHAR Name[],
    BOOLEAN CharUpper
    );

#endif // _S_GlobalFunc_HEADER_
