#include "MyFltSys.h"

PFLT_FILTER gFilter;
PFLT_PORT gSrvPort;
PFLT_PORT gCliPort;

UNICODE_STRING gWhiteProcess;
WCHAR gObjectDirBuf[260];
UNICODE_STRING gObjectDir;
UNICODE_STRING gObjectFile;

NTSTATUS
Sym2Dev (PWCHAR symName, PWCHAR devName, USHORT devNameLen)
{
    NTSTATUS status = STATUS_SUCCESS;

    // NOTE: 下面两种方法效果相同
    // WCHAR szSymNameBuf[] = L"\\??\\C:";
    // WCHAR szSymNameBuf[] = L"\\DosDevices\\C:";

    UNICODE_STRING usSymName = { 0 };
    HANDLE hSymHdr = NULL;
    OBJECT_ATTRIBUTES oa = { 0 };
    //WCHAR szDevNameBuf[100] = { 0 };
    UNICODE_STRING usDevName = { 0 };
    ULONG ulRetLen = 0;

    KdPrint(("MyFltSys!Sym2Dev: ENTERED\n"));

    // 第1步：获取符号名的句柄
    RtlInitUnicodeString(&usSymName, symName);
    InitializeObjectAttributes(
        &oa,
        &usSymName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    status = ZwOpenSymbolicLinkObject(&hSymHdr, GENERIC_READ, &oa);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!Sym2Dev: Open Sym FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!Sym2Dev: Open Sym SUCCEED\n"));

    // 第2步：通过句柄获取设备名
    // NOTE: 两个函数不能互换，因为RtlInitUnicodeString无法初始化usDevName.MaximumLength
    RtlInitEmptyUnicodeString(&usDevName, devName, devNameLen);
    // RtlInitUnicodeString(&usDevName, szDevNameBuf);

    status = ZwQuerySymbolicLinkObject(hSymHdr, &usDevName, &ulRetLen);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!Sym2Dev: Query Sym FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!Sym2Dev: Query Sym SUCCEED\n"));

    KdPrint(("%wZ --> %wZ\n", &usSymName, &usDevName));
    // \??\C: --> \Device\HarddiskVolume2


exit:
    if (NULL != hSymHdr) {
        ZwClose(hSymHdr);
        hSymHdr = NULL;
    }

    KdPrint(("MyFltSys!Sym2Dev: FINISHED\n"));
    return status;
}

NTSTATUS
QueryInstDir (void)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hInstDirKey = NULL;
    UNICODE_STRING usInstDirRegPath = 
        RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Install\\QianKun-EDR-Agent");
    UNICODE_STRING usValueName = RTL_CONSTANT_STRING(L"InstallDir");
    OBJECT_ATTRIBUTES oa = { 0 };
    ULONG ulRetLen = 0;
    WCHAR wszInstRootBuf[260] = { 0 };
    //UNICODE_STRING usInstRoot = { 0 };

    InitializeObjectAttributes(
        &oa,
        &usInstDirRegPath,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    status = ZwOpenKey(&hInstDirKey, KEY_ALL_ACCESS, &oa);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!QueryInstDir: Open Key FAILED (0x&08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!QueryInstDir: Open Key SUCCEED\n"));

    status = ZwQueryValueKey(
                hInstDirKey, 
                &usValueName, 
                KeyValuePartialInformation, 
                NULL, 
                0, 
                &ulRetLen);
    if (STATUS_BUFFER_TOO_SMALL == status && 0 != ulRetLen) {
        PKEY_VALUE_PARTIAL_INFORMATION pData = ExAllocatePoolWithTag(NonPagedPool, ulRetLen, FILE_IMAGE_TAG);
        if (NULL == pData) {
            KdPrint(("MyFltSys!QueryInstDir: Allocate Memory FAILED\n"));
            goto exit;
        }
        KdPrint(("MyFltSys!QueryInstDir: Allocate Memory SUCCEED\n"));
        RtlZeroMemory(pData, ulRetLen);

        status = ZwQueryValueKey(
                    hInstDirKey,
                    &usValueName,
                    KeyValuePartialInformation,
                    pData,
                    ulRetLen,
                    &ulRetLen);
        if (NT_SUCCESS(status)) {
            WCHAR wszSymLink[] = L"\\??\\X:";
            WCHAR wszDevName[260] = { 0 };
            
            RtlCopyMemory(wszInstRootBuf, pData->Data, pData->DataLength);
            //KdPrint(("MyFltSys!QueryInstDir: InstRoot=%ws\n", wszInstRootBuf));
            
            wszSymLink[4] = wszInstRootBuf[0];
            //KdPrint(("MyFltSys!QueryInstDir: wszSymLink=%ws\n", wszSymLink));

            Sym2Dev(wszSymLink, wszDevName, sizeof(wszDevName));
            //gObjectDirBuf
            RtlCopyMemory(gObjectDirBuf, wszDevName, wcslen(wszDevName) * sizeof(WCHAR));
            RtlCopyMemory(gObjectDirBuf + wcslen(wszDevName), wcschr(wszInstRootBuf, L'\\'), wcslen(wszInstRootBuf) * sizeof(WCHAR) - 4);
            KdPrint(("%ws\n", gObjectDirBuf));
        }

        ExFreePoolWithTag(pData, FILE_IMAGE_TAG);
        pData = NULL;
    }

exit:
    if (NULL != hInstDirKey) {
        ZwClose(hInstDirKey);
        hInstDirKey = NULL;
    }
    return status;
}

NTSTATUS
GetFilePathByProcessId (
    ULONG pid,
    PUNICODE_STRING *pusFilePath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hProcessHdr = NULL;
    CLIENT_ID cid = { 0 };
    OBJECT_ATTRIBUTES oa = { 0 };
    PUNICODE_STRING pusImageName = NULL;
    ULONG ulRetLen = 0;
    
    //KdPrint(("MyFltSys!GetFilePathByProcessId: ENTERED\n"));

    cid.UniqueProcess = (HANDLE)pid;
    InitializeObjectAttributes(
        &oa,
        NULL,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    status = ZwOpenProcess(
                &hProcessHdr,
                PROCESS_ALL_ACCESS,
                &oa,
                &cid);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!GetFilePathByProcessId: Open Process FAILED (0x%08x)\n", status));
        goto exit;
    }

    status = gpfnZwQueryInformationProcess(
                hProcessHdr,
                ProcessImageFileName,
                NULL,
                0,
                &ulRetLen);
    if (STATUS_INFO_LENGTH_MISMATCH != status) {
        KdPrint(("MyFltSys!GetFilePathByProcessId: Query ProcessName Length FAILED (0x%08x)\n", status));
        goto exit;
    }

    pusImageName = ExAllocatePoolWithTag(PagedPool, ulRetLen, FILE_IMAGE_TAG);
    if (NULL == pusImageName) {
        KdPrint(("MyFltSys!GetFilePathByProcessId: Allocate Memory FAILED\n"));
        goto exit;
    }

    status = gpfnZwQueryInformationProcess(
                hProcessHdr,
                ProcessImageFileName,
                pusImageName,
                ulRetLen,
                &ulRetLen);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!GetFilePathByProcessId: Query ProcessName FAILED (0x%08x)\n", status));
        goto exit;
    }

    *pusFilePath = pusImageName;

exit:
    if (!NT_SUCCESS(status)) {
        if (NULL != pusImageName) {
            ExFreePoolWithTag(pusImageName, 'gmif');
            pusImageName = NULL;
        }
    }

    if (NULL != hProcessHdr) {
        ZwClose(hProcessHdr);
        hProcessHdr = NULL;
    }

    //KdPrint(("MyFltSys!GetFilePathByProcessId: FINISHED\n"));
    return status;
}

BOOLEAN
IsTrustedAccess (
    _Inout_ PFLT_CALLBACK_DATA Data
    )
{
    BOOLEAN bTrusted = TRUE;
    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

    status = FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &nameInfo);
    if (NT_SUCCESS(status)) {
        status = FltParseFileNameInformation(nameInfo);
        if (NT_SUCCESS(status)) {
            //KdPrint(("MyFltSys!IsTrustedAccess: Name=%wZ^NamesParsed=0x%08x^Volume=%wZ^Share=%wZ^Extension=%wZ^Stream=%wZ^FinalComponent=%wZ^ParentDir=%wZ\n",
            //    &nameInfo->Name,
            //    nameInfo->NamesParsed,
            //    &nameInfo->Volume,
            //    &nameInfo->Share,
            //    &nameInfo->Extension,
            //    &nameInfo->Stream,
            //    &nameInfo->FinalComponent,
            //    &nameInfo->ParentDir));

            // 判定客体是否为被保护的目录和文件
            //if (RtlSuffixUnicodeString(&gObjectFile, &nameInfo->Name, TRUE)) {...}
            if (0 == RtlCompareUnicodeString(&gObjectDir, &nameInfo->Name, TRUE) ||
                0 == RtlCompareUnicodeString(&gObjectFile, &nameInfo->Name, TRUE)) {
                // *获取主体进程名
                // **方法1(不可行): EPROCESS + 0x174, 在32位Win10中获取不到数据
                //   CHAR  *szProcessName = (char *)pEProcess + 0x174;
                // **方法2(可行): 通过EPROCESS获取进程名（文件名）
                //   PEPROCESS pEProcess = FltGetRequestorProcess(Data);
                //   UCHAR *szProcessName = PsGetProcessImageFileName(pEProcess);
                //   if (0 != strcmp((const char *)szProcessName, "notepad.exe")) {...}
                // **方法3(可行): 通过PID获取进程名（全路径），用完后要释放进程名的内存
                //   UNICODE_STRING usWhiteProcess = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume2\\Windows\\System32\\notepad.exe");
                ULONG ulSubjectProcessId = FltGetRequestorProcessId(Data);
                PUNICODE_STRING pusSubjectProcessName = NULL;

                status = GetFilePathByProcessId(ulSubjectProcessId, &pusSubjectProcessName);
                if (NT_SUCCESS(status)) {
                    KdPrint(("MyFltSys!IsTrustedAccess: Subject=%wZ -> Object=%wZ\n",
                        pusSubjectProcessName,
                        &nameInfo->Name));
                    // 判定主体进程是否在白名单内
                    if (0 == RtlCompareUnicodeString(pusSubjectProcessName, &gWhiteProcess, TRUE)) {
                        // 是白名单进程
                    } else {
                        // 不是白名单进程
                        bTrusted = FALSE;
                        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
                        Data->IoStatus.Information = 0;
                    }

                    if (NULL != pusSubjectProcessName) {
                        ExFreePoolWithTag(pusSubjectProcessName, FILE_IMAGE_TAG);
                        pusSubjectProcessName = NULL;
                    }
                    goto exit;
                } else {
                    //KdPrint(("MyFltSys!PreCreate: Query Image Name FAILED (0x%08x)\n", status));
                }
            }
        } else {
            //KdPrint(("MyFltSys!PreCreate: Parse File Name Info FAILED (0x%08x)\n", status));
        }
    } else {
        //KdPrint(("MyFltSys!PreCreate: Get File Name Info FAILED (0x%08x)\n", status));
    }

exit:
    if (NULL != nameInfo) {
        FltReleaseFileNameInformation(nameInfo);
        nameInfo = NULL;
    }

    return bTrusted;
}

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING usZwQueryInformationProcess = RTL_CONSTANT_STRING(L"ZwQueryInformationProcess");
    PSECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING usCommPort;
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("MyFltSys!DriverEntry: ENTERED\n"));

    RtlInitUnicodeString(&gWhiteProcess, L"\\Device\\HarddiskVolume2\\Windows\\System32\\notepad.exe");
    //RtlInitUnicodeString(&gObjectDir, L"\\Device\\HarddiskVolume2\\aaa");
    RtlInitEmptyUnicodeString(&gObjectDir, gObjectDirBuf, sizeof(gObjectDirBuf));
    RtlInitUnicodeString(&gObjectFile, L"\\Device\\HarddiskVolume2\\aaa\\1.txt");
    
    status = QueryInstDir();
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!DriverEntry: Query Install Dirtory FAILED (0x%08x)\n", status));
        goto exit;
    }
    
    gpfnZwQueryInformationProcess = (pfnZwQueryInformationProcess)MmGetSystemRoutineAddress(&usZwQueryInformationProcess);
    if (NULL == gpfnZwQueryInformationProcess) {
        KdPrint(("MyFltSys!DriverEntry: Init ZwQueryInformationProcess(...) FAILED\n"));
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }
    
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

    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!DriverEntry: Build SD FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("MyFltSys!DriverEntry: Build SD SUCCEED\n"));


    RtlInitUnicodeString(&usCommPort, MINISPY_PORT_NAME);

    InitializeObjectAttributes(
        &oa, 
        &usCommPort,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, 
        NULL,
        sd);

    status = FltCreateCommunicationPort(
                 gFilter,
                 &gSrvPort,
                 &oa,
                 NULL,
                 MiniConn,
                 MiniDisConn,
                 MiniMsg,
                 1);
    FltFreeSecurityDescriptor(sd);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MyFltSys!DriverEntry: Create Comm Port FAILED (0x%08x)\n", status));
        goto exit;
    }

    KdPrint(("MyFltSys!DriverEntry: Create Comm Port SUCCEED\n"));

exit:
    if (!NT_SUCCESS(status)) {
        if (NULL != gSrvPort) {
            FltCloseCommunicationPort(gSrvPort);
            gSrvPort = NULL;
        }
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
    FLT_PREOP_CALLBACK_STATUS fltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    //KdPrint(("MyFltSys!PreCreate: ENTERED\n"));
    
    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //} else {
    //    fltStatus = FLT_PREOP_COMPLETE;
    //}

    return fltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
PostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS fltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    //KdPrint(("MyFltSys!PostCreate: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //}
    //else {
    //    // TODO: Do Nothing
    //}
    
    return fltStatus;
}

FLT_PREOP_CALLBACK_STATUS
PreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
{
    FLT_PREOP_CALLBACK_STATUS fltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    //KdPrint(("MyFltSys!PreOperation: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //} else {
    //    fltStatus = FLT_PREOP_COMPLETE;
    //}
    
    return fltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
PostReadWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS fltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    //KdPrint(("MyFltSys!PostReadWrite: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //}
    //else {
    //    // TODO: Do Nothing
    //}

    return fltStatus;
}

FLT_PREOP_CALLBACK_STATUS
PreSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext
    )
{
    FLT_PREOP_CALLBACK_STATUS fltStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    //KdPrint(("MyFltSys!PreSetInfo: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //}
    //else {
    //    fltStatus = FLT_PREOP_COMPLETE;
    //}

    return fltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
PostSetInfo (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS fltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    //KdPrint(("MyFltSys!PostSetInfo: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //}
    //else {
    //    // TODO: Do Nothing
    //}

    return fltStatus;
}

FLT_POSTOP_CALLBACK_STATUS
PostCleanup(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS fltStatus = FLT_POSTOP_FINISHED_PROCESSING;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    //KdPrint(("MyFltSys!PostCleanup: ENTERED\n"));

    //if (IsTrustedAccess(Data)) {
    //    // TODO: Do Nothing
    //}
    //else {
    //    // TODO: Do Nothing
    //}

    return fltStatus;
}

NTSTATUS
Unload (
    FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    KdPrint(("MyFltSys!Unload: ENTERED\n"));

    if (NULL != gSrvPort) {
        FltCloseCommunicationPort(gSrvPort);
        gSrvPort = NULL;
    }

    if (NULL != gFilter) {
        FltUnregisterFilter(gFilter);
        gFilter = NULL;
    }

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

    PAGED_CODE();

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

    PAGED_CODE();

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

    PAGED_CODE();

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

    PAGED_CODE();
    
    KdPrint(("MyFltSys!InstTearDownComplete: ENTERED\n"));
}

NTSTATUS
MiniConn (
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie
    )
{
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionPortCookie);

    PAGED_CODE();

    KdPrint(("MyFltSys!MiniConn: ENTERED\n"));

    if (NULL != ClientPort) {
        gCliPort = ClientPort;
    }

    return STATUS_SUCCESS;
}

VOID
MiniDisConn(
    _In_opt_ PVOID ConnectionCookie
    )
{
    UNREFERENCED_PARAMETER(ConnectionCookie);

    PAGED_CODE();

    KdPrint(("MyFltSys!MiniDisConn: ENTERED\n"));

    FltCloseClientPort(gFilter, &gCliPort);
    gCliPort = NULL;
}

NTSTATUS
MiniMsg (
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
    )
{
    UNREFERENCED_PARAMETER(PortCookie);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

    PAGED_CODE();

    KdPrint(("MyFltSys!MiniMsg: ENTERED\n"));

    return STATUS_SUCCESS;
}
