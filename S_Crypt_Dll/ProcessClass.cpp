
#include "ProcessClass.h"


ProcessInfo::ProcessInfo(char* InProcName)
{
	ProcName = InProcName;
}


void ProcessHandle::InsertProcessInfo(char* CurProcName)
{
	
	ProcessInfos.push_back(new ProcessInfo(CurProcName));
	
}