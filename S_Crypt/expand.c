
#include "expand.h"

//NTSYSAPI NTSTATUS NTAPI ZwDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes);

//CHAIN OPERATION

PPROC_INFO ChainHead = NULL;//全局变量
INT NetConnectState = NULL;//全局变量
DWORD CreateHead()// 创建链表头
{
	ChainHead =	(PPROC_INFO)ExAllocatePool(NonPagedPool, sizeof(PROC_INFO));// 分配表头节点的空间
	if(ChainHead == NULL)
	{
		return STATUS_CREATECHAINHEAD_FAILED;

	}else
	{
		RtlZeroMemory(ChainHead,sizeof(PROC_INFO));
	}
	return STATUS_SUCCESS;
}

VOID InsertInfo(PROC_INFO* NodeData)//向链表中插入节点数据,在调用本函数之前进行分配节点空间
{
	PROC_INFO* q;
	if(ChainHead == NULL || NodeData == NULL)return;
	NodeData->Index = ChainHead->Index;//因为表头结点的结点总数是从0开始的
	ChainHead->Index += 1;//把链表的总节点数加1
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

	ChainHead->Index-=1;//先将链表节点长度减1
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
		ExFreePool((PVOID)p);//在这里释放群组节点
		while( temp != NULL)
		{
			temp->Index-=1;
			temp = temp->Next;
		}
	}
}

VOID FreeChain()//释放链表
{
	PROC_INFO *p,*q;
	if(ChainHead == NULL)return;
	p = ChainHead->Next;//从链表的第一个节点开始释放,p指向了第一个节点
	while( p != NULL)//用P记住当前节点
	{
		q = p->Next;//用q记住要释放节点的下一个节点的地址
		ExFreePool((PVOID)p);//在这里释放节点
		p = NULL;
		p = q;
	}
	ExFreePool((PVOID)ChainHead);//释放表头
	ChainHead = NULL;
}

PROC_INFO* SearchChain(INT Index)//根据某个节点的Index域，返回某个节点的地址
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