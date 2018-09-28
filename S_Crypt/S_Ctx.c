#include "S_Ctx.h"

static NTSTATUS iCtx_CreateStreamContext(PFLT_RELATED_OBJECTS FltObjects, PSTREAM_CONTEXT *StreamContext) ;

VOID
SC_LOCK(PSTREAM_CONTEXT SC, PKIRQL OldIrql)
{
	if (KeGetCurrentIrql() <= APC_LEVEL)
	{
		SC_iLOCK(SC->Resource) ;
	}
	else 
	{
		KeAcquireSpinLock(&SC->Resource1, OldIrql) ;
	}
}

VOID 
SC_UNLOCK(PSTREAM_CONTEXT SC, KIRQL OldIrql)
{
    if (KeGetCurrentIrql() <= APC_LEVEL)
    {
    	SC_iUNLOCK(SC->Resource) ;
    }
	else
	{
		KeReleaseSpinLock(&SC->Resource1, OldIrql) ;
	}
}

NTSTATUS
Ctx_FindOrCreateStreamContext (
    __in PFLT_CALLBACK_DATA Data,
	__in PFLT_RELATED_OBJECTS FltObjects,
    __in BOOLEAN CreateIfNotFound,
    __deref_out PSTREAM_CONTEXT *StreamContext,
    __out_opt PBOOLEAN ContextCreated
    )
/*++

Routine Description:

    This routine finds the stream context for the target stream.
    Optionally, if the context does not exist this routing creates
    a new one and attaches the context to the stream.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    CreateIfNotFound      - Supplies if the stream must be created if missing
    StreamContext         - Returns the stream context
    ContextCreated        - Returns if a new context was created

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAM_CONTEXT streamContext = NULL;
    PSTREAM_CONTEXT oldStreamContext = NULL;
	BOOLEAN ForTest = TRUE;
    //PAGED_CODE();

    *StreamContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //  First try to get the stream context.
    status = FltGetStreamContext( Data->Iopb->TargetInstance,Data->Iopb->TargetFileObject,&streamContext );
    if (!NT_SUCCESS( status ) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound) 
	{
        status = iCtx_CreateStreamContext(FltObjects, &streamContext );
        if (!NT_SUCCESS( status ))
            return status;

        status = FltSetStreamContext( Data->Iopb->TargetInstance,
                                      Data->Iopb->TargetFileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      streamContext,
                                      &oldStreamContext );

        if (!NT_SUCCESS( status ))
	{
            FltReleaseContext(streamContext);

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) 
			{
                //  FltSetStreamContext failed for a reason other than the context already
                //  existing on the stream. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                return status;
            }
            streamContext = oldStreamContext;
            status = STATUS_SUCCESS;
			ForTest = FALSE;
        } else {
            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }
    *StreamContext = streamContext;
    return status;
}

NTSTATUS
iCtx_CreateStreamContext (
    __in PFLT_RELATED_OBJECTS FltObjects,
    __deref_out PSTREAM_CONTEXT *StreamContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext         - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAM_CONTEXT streamContext;

    //PAGED_CODE();


	status = FltAllocateContext( FltObjects->Filter,
                                 FLT_STREAM_CONTEXT,
                                 STREAM_CONTEXT_SIZE,
                                 NonPagedPool,
                                 &streamContext );
    if (!NT_SUCCESS( status )) 
	{
        return status;
    }

    //  Initialize the newly created context
    RtlZeroMemory( streamContext, STREAM_CONTEXT_SIZE );

    streamContext->Resource = ExAllocatePoolWithTag( NonPagedPool, sizeof( ERESOURCE ),RESOURCE_TAG );
    if(streamContext->Resource == NULL)
	{
        FltReleaseContext( streamContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite( streamContext->Resource );

	KeInitializeSpinLock(&streamContext->Resource1) ; 

    *StreamContext = streamContext;

    return STATUS_SUCCESS;
}