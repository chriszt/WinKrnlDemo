#include <cstdio>
#include <Windows.h>

#pragma comment(lib, "Advapi32.lib")

#define SRV_NAME  L"drv"

int main()
{
    SC_HANDLE hSCM = NULL;
    SC_HANDLE hSrv = NULL;

    do {
        hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (hSCM == NULL) {
            break;
        }

        hSrv = CreateServiceW(hSCM,
                             SRV_NAME,
                             SRV_NAME,
                             SERVICE_ALL_ACCESS,
                             SERVICE_KERNEL_DRIVER,
                             SERVICE_DEMAND_START,
                             SERVICE_ERROR_IGNORE,
                             L"c:\\drv\\drv.sys",
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
        if (hSrv == NULL) {
            DWORD dwErrCode = GetLastError();
            if (dwErrCode == ERROR_SERVICE_EXISTS) {
                hSrv = OpenServiceW(hSCM, SRV_NAME, SERVICE_ALL_ACCESS);
                if (hSrv == NULL) {
                    break;
                }
            } else {
                break;
            }
        }
        printf_s("CreateServiceW or OpenServiceW SUCCEED\n");

        getchar();

        BOOL bRet = StartServiceW(hSrv, 0, NULL);
        printf_s("StartServiceW: %d\n", bRet);

        getchar();

        SERVICE_STATUS srvStatus = { 0 };
        bRet = ControlService(hSrv, SERVICE_CONTROL_STOP, &srvStatus);
        printf_s("ControlService: %d\n", bRet);

        getchar();

        DeleteService(hSrv);
    } while (FALSE);

    if (hSrv != NULL) {
        CloseServiceHandle(hSrv);
        hSrv = NULL;
    }

    if (hSCM != NULL) {
        CloseServiceHandle(hSCM);
        hSCM = NULL;
    }

    return 0;
}
