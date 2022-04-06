#include <ntddk.h>
#include <ntstrsafe.h>

NTSTATUS testCbPrint()
{
    NTSTATUS status = STATUS_SUCCESS;
    WCHAR dstBuf[512] = { 0 };
    UNICODE_STRING dst = { 0 };
    UNICODE_STRING filePath = RTL_CONSTANT_STRING(L"\\??\\c:\\aaa\\ccc.txt");
    USHORT fileSize = 1024;

    RtlInitEmptyUnicodeString(&dst, dstBuf, sizeof(dstBuf));

    status = RtlStringCbPrintfW(dst.Buffer,
                                sizeof(dstBuf),
                                L"file path = %wZ, file size = %u\n",
                                &filePath, fileSize);
    if(!NT_SUCCESS(status)) {
        KdPrint(("drv!CbPrint: StringCbPrintf FAILED (0x%08X)\n", status));
        goto EXIT;
    }
    dst.Length = (USHORT) wcslen(dst.Buffer) * sizeof(WCHAR);

    KdPrint(("%wZ\n", &dst));
    
EXIT:
    return status;
}

void Unload(PDRIVER_OBJECT pDrvObj)
{
    UNREFERENCED_PARAMETER(pDrvObj);
    KdPrint(("drv!Unload: ENTERED\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pRegPath);

    pDrvObj->DriverUnload = Unload;

    KdPrint(("drv!DriverEntry: ENTERED\n"));

    status = testCbPrint();

    return status;
}
