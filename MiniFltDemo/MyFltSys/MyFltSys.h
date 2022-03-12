#ifndef MY_FLT_SYS
#define MY_FLT_SYS

#include <fltKernel.h>

//#define MINISPY_PORT_NAME               L"\\MyFltPort"

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

//void
//Unload (
//    __in PDRIVER_OBJECT DriverObject
//    );

FLT_PREOP_CALLBACK_STATUS
PreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
PostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

NTSTATUS
Unload (
    FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
InstSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
InstQueryTearDown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

VOID
InstTearDownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Reason
    );

VOID
InstTearDownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Reason
    );

//NTSTATUS
//MiniConn (
//    _In_ PFLT_PORT ClientPort,
//    _In_opt_ PVOID ServerPortCookie,
//    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
//    _In_ ULONG SizeOfContext,
//    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie
//    );
//
//VOID
//MiniDisConn(
//    _In_opt_ PVOID ConnectionCookie
//    );
//
//NTSTATUS
//MiniMsg (
//    _In_opt_ PVOID PortCookie,
//    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
//    _In_ ULONG InputBufferLength,
//    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
//    _In_ ULONG OutputBufferLength,
//    _Out_ PULONG ReturnOutputBufferLength
//    );

const FLT_OPERATION_REGISTRATION gCallbacks[] = {
    { IRP_MJ_CREATE, 0, PreCreate, PostCreate },
    
    { IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION gFltReg = {
    sizeof( FLT_REGISTRATION ),
    FLT_REGISTRATION_VERSION,
    0,

    NULL,
    gCallbacks,

    Unload,

    InstSetup,
    InstQueryTearDown,
    InstTearDownStart,
    InstTearDownComplete,

    NULL,
    NULL,
    NULL
};

#endif // !MY_FLT_SYS
