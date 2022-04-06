#include <ntddk.h>

NTSTATUS
Sym2Dev(void)
{
    NTSTATUS status = STATUS_SUCCESS;

    // NOTE: �������ַ���Ч����ͬ
    // WCHAR szSymNameBuf[] = L"\\??\\C:";
    WCHAR szSymNameBuf[] = L"\\DosDevices\\C:";

    UNICODE_STRING usSymName = { 0 };
    HANDLE hSymHdr = NULL;
    OBJECT_ATTRIBUTES oa = { 0 };
    WCHAR szDevNameBuf[100] = { 0 };
    UNICODE_STRING usDevName = { 0 };
    ULONG ulRetLen = 0;

    KdPrint(("Sym2Dev!Sym2Dev: ENTERED\n"));

    // ��1������ȡ�������ľ��
    RtlInitUnicodeString(&usSymName, szSymNameBuf);
    InitializeObjectAttributes(
        &oa,
        &usSymName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    status = ZwOpenSymbolicLinkObject(&hSymHdr, GENERIC_READ, &oa);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Sym2Dev!Sym2Dev: Open Sym FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("Sym2Dev!Sym2Dev: Open Sym SUCCEED\n"));

    // ��2����ͨ�������ȡ�豸��
    // NOTE: �����������ܻ�������ΪRtlInitUnicodeString�޷���ʼ��usDevName.MaximumLength
    RtlInitEmptyUnicodeString(&usDevName, szDevNameBuf, sizeof(WCHAR) * 100);
    // RtlInitUnicodeString(&usDevName, szDevNameBuf);

    status = ZwQuerySymbolicLinkObject(hSymHdr, &usDevName, &ulRetLen);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Sym2Dev!Sym2Dev: Query Sym FAILED (0x%08x)\n", status));
        goto exit;
    }
    KdPrint(("Sym2Dev!Sym2Dev: Query Sym SUCCEED\n"));

    KdPrint(("%wZ --> %wZ\n", &usSymName, &usDevName));

exit:
    if (NULL != hSymHdr) {
        ZwClose(hSymHdr);
        hSymHdr = NULL;
    }

    KdPrint(("Sym2Dev!Sym2Dev: FINISHED\n"));
    return status;
}

void Unload(PDRIVER_OBJECT pDrvObj)
{
    UNREFERENCED_PARAMETER(pDrvObj);
    KdPrint(("Sym2Dev!Unload: ENTERED\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pRegPath);

    pDrvObj->DriverUnload = Unload;

    KdPrint(("Sym2Dev!DriverEntry: ENTERED\n"));

    //status = Sym2Dev();

    KdPrint(("Sym2Dev!DriverEntry: FINISHED\n"));
    return status;
}
