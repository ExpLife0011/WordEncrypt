#include "S_GlobalFunc.h"

//初始化分配UnicodeString
NTSTATUS
S_AllocateUnicodeString (
    __inout PUNICODE_STRING pUS
    )
{
    //PAGED_CODE();

    pUS->Buffer = ExAllocatePoolWithTag( PagedPool, pUS->MaximumLength,S_MM_TAG);
    if (pUS->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pUS->Length = 0;
    return STATUS_SUCCESS;
}

//释放UnicodeString
VOID
S_FreeUnicodeString (
    __inout PUNICODE_STRING pUS
    )
{
    //PAGED_CODE();

    ExFreePoolWithTag( pUS->Buffer,S_MM_TAG);

    pUS->Length = pUS->MaximumLength = 0;
    pUS->Buffer = NULL;
}

//将Unicode指针变量转换为Char数组
BOOLEAN 
S_UnicodeStringToChar(
    PUNICODE_STRING UniName, 
    CHAR Name[],
    BOOLEAN CharUpper
    )
{
     ANSI_STRING	AnsiName;			
    NTSTATUS	ntstatus;
    CHAR*	nameptr;			
    
    __try {	    		   		    		
		ntstatus = RtlUnicodeStringToAnsiString(&AnsiName, UniName, TRUE);		
						
		if (AnsiName.Length < DEFAULT_FILE_NAME_LENGTH) {
			nameptr = (PCHAR)AnsiName.Buffer;
			//Convert into upper case and copy to buffer
			if (CharUpper)
			{
			    strcpy(Name, _strupr(nameptr));
			} else {
                        strcpy(Name,nameptr);
			}
			//KdPrint(("S_UnicodeStringToChar : %s\n", Name));	
		}		  	
		RtlFreeAnsiString(&AnsiName);		 
	} 
	__except(EXCEPTION_EXECUTE_HANDLER) {
		KdPrint(("NPUnicodeStringToChar EXCEPTION_EXECUTE_HANDLER\n"));	
		return FALSE;
	}
    return TRUE;
}      


