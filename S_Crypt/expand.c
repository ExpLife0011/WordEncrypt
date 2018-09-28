
#include "expand.h"

//NTSYSAPI NTSTATUS NTAPI ZwDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes);

//CHAIN OPERATION

PPROC_INFO ChainHead = NULL;//ȫ�ֱ���
INT NetConnectState = NULL;//ȫ�ֱ���
DWORD CreateHead()// ��������ͷ
{
	ChainHead =	(PPROC_INFO)ExAllocatePool(NonPagedPool, sizeof(PROC_INFO));// �����ͷ�ڵ�Ŀռ�
	if(ChainHead == NULL)
	{
		return STATUS_CREATECHAINHEAD_FAILED;

	}else
	{
		RtlZeroMemory(ChainHead,sizeof(PROC_INFO));
	}
	return STATUS_SUCCESS;
}

VOID InsertInfo(PROC_INFO* NodeData)//�������в���ڵ�����,�ڵ��ñ�����֮ǰ���з���ڵ�ռ�
{
	PROC_INFO* q;
	if(ChainHead == NULL || NodeData == NULL)return;
	NodeData->Index = ChainHead->Index;//��Ϊ��ͷ���Ľ�������Ǵ�0��ʼ��
	ChainHead->Index += 1;//��������ܽڵ�����1
	q = ChainHead;
	while( q->Next != NULL)
	{
		q = q->Next;
	}
	q->Next = NodeData;	
}

void DeleteInfo(int Index)
{
	PROC_INFO* p,*q;
	if(ChainHead == NULL)return;

	ChainHead->Index-=1;//�Ƚ�����ڵ㳤�ȼ�1
	q = ChainHead;
	p = q->Next;
	while(( p != NULL) && ( p->Index != Index))
	{
		q = p;
		p = p->Next;
	}
	if( p != NULL )
	{
		PROC_INFO* temp;
		q->Next = p->Next ;
		temp = p->Next;
		ExFreePool((PVOID)p);//�������ͷ�Ⱥ��ڵ�
		while( temp != NULL)
		{
			temp->Index-=1;
			temp = temp->Next;
		}
	}
}

VOID FreeChain()//�ͷ�����
{
	PROC_INFO *p,*q;
	if(ChainHead == NULL)return;
	p = ChainHead->Next;//������ĵ�һ���ڵ㿪ʼ�ͷ�,pָ���˵�һ���ڵ�
	while( p != NULL)//��P��ס��ǰ�ڵ�
	{
		q = p->Next;//��q��סҪ�ͷŽڵ����һ���ڵ�ĵ�ַ
		ExFreePool((PVOID)p);//�������ͷŽڵ�
		p = NULL;
		p = q;
	}
	ExFreePool((PVOID)ChainHead);//�ͷű�ͷ
	ChainHead = NULL;
}

PROC_INFO* SearchChain(INT Index)//����ĳ���ڵ��Index�򣬷���ĳ���ڵ�ĵ�ַ
{
	PROC_INFO* p = NULL;
	if(ChainHead == NULL)return NULL;
	p = ChainHead->Next;
	while(( p != NULL) && ( p->Index!= Index))
	{
		p = p->Next;
	}
	if(p==NULL)
		return NULL;
	else
		return p;
}

DWORD ProcessNameOffset;
DWORD GetProcessNameOffset()
{  
	PEPROCESS curproc = PsGetCurrentProcess();  
	int i;  
	for( i = 0; i < 3*PAGE_SIZE; i++ )   
	{  
		if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))  
		{  
			if (i<3*PAGE_SIZE)  
			{  
				ProcessNameOffset = i;  
				//DbgMsg("ProcessNameOffset: %.8X",ProcessNameOffset);  
				break;  
			}  
		}  
	}  
	return ProcessNameOffset;  
}
CHAR ImageFileNameCopyed[16];
BOOL GetProcessName(PCHAR* ImageFileName, DWORD dwProcessNameOffset)  
{  
	PEPROCESS   curproc;  
	//char        *nameptr;  
	if(dwProcessNameOffset)  
	{  /*old
		curproc = PsGetCurrentProcess();  
		*ImageFileName = (PCHAR) curproc + dwProcessNameOffset; 
		*/
		curproc = PsGetCurrentProcess();  
		memset(ImageFileNameCopyed,0,16);
		strncpy(ImageFileNameCopyed,  (PCHAR) curproc + dwProcessNameOffset, 16);  
		ImageFileNameCopyed[15] = 0;    /* NULL at end */
		*ImageFileName = ImageFileNameCopyed;
		return TRUE;
	} 
	return FALSE;  
}  

PROC_INFO* CompareString(PCHAR ImageFileName)
{
	PROC_INFO* p = NULL;
	int i = 0;
	if ((ImageFileName == NULL)|| strlen(ImageFileName)==0)
	{
		return FALSE;
	}	
	if(ChainHead == NULL)return FALSE;
	p = ChainHead->Next;
	while(p!= NULL) 
	{
		if(p->ProcessName!=NULL)
		{			
			if (!_strnicmp(ImageFileName,p->ProcessName,strlen(ImageFileName)))
			{
				return p;
			}			
		}
		p = p->Next;
	}	
	return p;
}


//process name offset flag
#define SYSNAME    "System"
ULONG DecideProcessNameOffsetInEPROCESS(VOID)
{
	PEPROCESS       curproc;
	int             i;
	curproc = PsGetCurrentProcess();
	// Scan for 12KB, hoping the KPEB never grows that big!
	for(i = 0; i < 3*PAGE_SIZE; i++)
	{     
		if(!strncmp(SYSNAME, (PCHAR)curproc + i, strlen(SYSNAME)))
		{
			return i;
		}
	}
	// Name not found - oh, well
	return 0;
}

PCHAR GetCurrentProcessName(PCHAR ProcessName)
{
	PEPROCESS       curproc;
	char            *nameptr;
	ULONG           i;

	// We only do this if we determined the process name offset
	if( ProcessNameOffset )
	{
		// Get a pointer to the current process block
		curproc = PsGetCurrentProcess();

		// Dig into it to extract the name. Make sure to leave enough room
		// in the buffer for the appended process ID.
		nameptr   = (PCHAR) curproc + ProcessNameOffset;

		RtlCopyMemory( ProcessName, nameptr, NT_PROCNAMELEN-1 );
		ProcessName[NT_PROCNAMELEN-1] = 0;
	}
	else
	{
		ProcessName[0] = '\0';
	}
	return ProcessName;
}