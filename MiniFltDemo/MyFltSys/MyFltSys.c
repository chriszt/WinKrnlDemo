#include "MyFltSys.h"

ULONG gTraceFlags = 1;
PFLT_FILTER gFilter;
PFLT_PORT gSrvPort;
PFLT_PORT gCliPort;

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    //PSECURITY_DESCRIPTOR sd;
    //OBJECT_ATTRIBUTES oa;
    //UNICODE_STRING usCommPort;
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("MyFltSys!DriverEntry: ENTERED\n"));
    
    status = FltRegisterFilter(DriverObject, &gFltReg, &gFilter);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!DriverEntry: Register Filter FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!DriverEntry: Register Filter SUCCEED\n"));

    status = FltStartFiltering(gFilter);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!DriverEntry: Start Filtering FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!DriverEntry: Start Filtering SUCCEED\n"));

    //status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    //if (!NT_SUCCESS(status)) {
    //    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("MyFltSys!DriverEntry: Build SD Failed:(\n"));
    //    goto exit;
    //}
    //PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("MyFltSys!DriverEntry: Build SD Succeed:)\n"));

    //RtlInitUnicodeString(&usCommPort, MINISPY_PORT_NAME);

    //InitializeObjectAttributes(&oa, 
    //                           &usCommPort,
    //                           OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, 
    //                           NULL,
    //                           sd);
    //status = FltCreateCommunicationPort(
    //            gFltHdr, 
    //            &gSrvPort,
    //            &oa,
    //            NULL,
    //            MiniConn,
    //            MiniDisConn,
    //            MiniMsg,
    //            1);

exit:
    if (!NT_SUCCESS(status)) {
        if (NULL != gFilter) {
            FltUnregisterFilter(gFilter);
            gFilter = NULL;
        }
    }

    KdPrint(("MyFltSys!DriverEntry: FINISHED\n"));
    return status;
}

//void
//Unload (
//    __in PDRIVER_OBJECT DriverObject
//    )
//{
//    UNREFERENCED_PARAMETER(DriverObject);
//    KdPrint(("MyFltSys!Unload: Entered\n"));
//}

FLT_PREOP_CALLBACK_STATUS
PreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    //KdPrint(("MyFltSys!PreCreate: ENTERED\n"));
    status = FltGetFileNameInformation(
                Data,
                FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                &nameInfo);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(nameInfo);
        if (NT_SUCCESS(status)) {
            KdPrint(("MyFltSys!PreCreate: Name=%wZ^NamesParsed=0x%08x^Volume=%wZ^Share=%wZ^Extension=%wZ^Stream=%wZ^FinalComponent=%wZ^ParentDir=%wZ\n",
                &nameInfo->Name,
                nameInfo->NamesParsed,
                &nameInfo->Volume,
                &nameInfo->Share,
                &nameInfo->Extension,
                &nameInfo->Stream,
                &nameInfo->FinalComponent,
                &nameInfo->ParentDir));
            // TODO: 如果客体是目标的文件或目录，可以进行管控
        } else {
            //KdPrint(("MyFltSys!PreCreate: Parse File Name Info FAILED (0x%08x)\n", status));
        }
    } else {
        //KdPrint(("MyFltSys!PreCreate: Get File Name Info FAILED (0x%08x)\n", status));
    }

    if (NULL != nameInfo) {
        FltReleaseFileNameInformation(nameInfo);
        nameInfo = NULL;
    }
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
PostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    //KdPrint(("MyFltSys!PostCreate: ENTERED\n"));

    return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS
Unload (
    FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);

    KdPrint(("MyFltSys!Unload: ENTERED\n"));

    if (NULL != gFilter) {
        FltUnregisterFilter(gFilter);
        gFilter = NULL;
    }

    KdPrint(("MyFltSys!Unload: FINISHED\n"));
    return STATUS_SUCCESS;
}

NTSTATUS
InstSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    KdPrint(("MyFltSys!InstSetup: ENTERED\n"));

    return STATUS_SUCCESS;
}

NTSTATUS
InstQueryTearDown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    KdPrint(("MyFltSys!InstQueryTearDown: ENTERED\n"));

    return STATUS_SUCCESS;
}

VOID
InstTearDownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Reason
    )
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Reason);

    KdPrint(("MyFltSys!InstTearDownStart: ENTERED\n"));
}

VOID
InstTearDownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Reason
    )
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Reason);

    KdPrint(("MyFltSys!InstTearDownComplete: ENTERED\n"));
}

//NTSTATUS
//MiniConn (
//    _In_ PFLT_PORT ClientPort,
//    _In_opt_ PVOID ServerPortCookie,
//    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
//    _In_ ULONG SizeOfContext,
//    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie
//    )
//{
//
//}
//
//VOID
//MiniDisConn(
//    _In_opt_ PVOID ConnectionCookie
//    )
//{
//
//}
//
//NTSTATUS
//MiniMsg (
//    _In_opt_ PVOID PortCookie,
//    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
//    _In_ ULONG InputBufferLength,
//    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
//    _In_ ULONG OutputBufferLength,
//    _Out_ PULONG ReturnOutputBufferLength
//    )
//{
//
//}
