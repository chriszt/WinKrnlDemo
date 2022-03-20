#ifndef MY_FLT_SYS
#define MY_FLT_SYS

#include <fltKernel.h>

#define MINISPY_PORT_NAME    L"\\MyFltPort"
#define FILE_IMAGE_TAG       'gmif'

NTKERNELAPI
UCHAR *
PsGetProcessImageFileName (PEPROCESS Process);

typedef NTSTATUS(*pfnZwQueryInformationProcess) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

pfnZwQueryInformationProcess gpfnZwQueryInformationProcess = NULL;

NTSTATUS
Sym2Dev (PWCHAR symName, PWCHAR devName, USHORT devNameLen);

NTSTATUS
QueryInstDir (void);

NTSTATUS
GetFilePathByProcessId(
    ULONG pid,
    PUNICODE_STRING *pusFilePath
    );

BOOLEAN
IsTrustedAccess (
    _Inout_ PFLT_CALLBACK_DATA Data
    );

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

FLT_PREOP_CALLBACK_STATUS
PreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
PostReadWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PreSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
PostSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
PostCleanup (
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

NTSTATUS
MiniConn (
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie
    );

VOID
MiniDisConn(
    _In_opt_ PVOID ConnectionCookie
    );

NTSTATUS
MiniMsg (
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
    );

const FLT_OPERATION_REGISTRATION gCallbacks[] = {
    { IRP_MJ_CREATE, 0, PreCreate, PostCreate },
    { IRP_MJ_READ, 0, PreOperation, PostReadWrite },
    { IRP_MJ_WRITE, 0, PreOperation, PostReadWrite },
    { IRP_MJ_SET_INFORMATION, FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO, 
        PreSetInfo, PostSetInfo },
    { IRP_MJ_CLEANUP, 0, PreOperation, PostCleanup },
    
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
