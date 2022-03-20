#include "MyFltApp.h"
#include <iostream>
#include <fltUser.h>

#pragma comment(lib, "fltLib.lib")

MyFltApp::MyFltApp()
{
    m_hDll = NULL;
}

MyFltApp::~MyFltApp()
{
    m_hDll = NULL;
}

void
MyFltApp::SendMsg(CMD_MSG msg)
{
    
}

bool
MyFltApp::LoadDll(void)
{
    return true;
}

int
main (void)
{
    char chIn;
    HANDLE hPort = INVALID_HANDLE_VALUE;

    while (true) {
        std::cin >> chIn;
        if (chIn == 'e' || chIn == 'E') {
            DWORD dwRet = 0;

            dwRet = FilterConnectCommunicationPort(
                        MINISPY_PORT_NAME,
                        0,
                        NULL,
                        0,
                        NULL,
                        &hPort);
            if (S_OK != dwRet) {
                fprintf(stderr, "Enable FAILED (0x%08x)\n", dwRet);
                break;
            }

            std::cout << "Enable" << std::endl;

        } else if (chIn == 'd' || chIn == 'D') {
            
            CloseHandle(hPort);
            hPort = INVALID_HANDLE_VALUE;
            std::cout << "Disable" << std::endl;

        } else if (chIn == 'p' || chIn == 'P') {
            HRESULT hRet = 0;
            CMD_MSG msg = { EM_PASS };
            DWORD dwRetBytes = 0;

            hRet = FilterSendMessage(
                       hPort,
                       &msg,
                       sizeof(CMD_MSG),
                       NULL,
                       0,
                       &dwRetBytes);
            if (S_OK != hRet) {
                fprintf(stderr, "Send Command FAILED (0x%08x)\n", hRet);
                break;
            }
            
            std::cout << "Send PASS Command" << std::endl;

        } else if (chIn == 'b' || chIn == 'B') {
            HRESULT hRet = 0;
            CMD_MSG msg = { EM_PASS };
            DWORD dwRetBytes = 0;

            hRet = FilterSendMessage(
                hPort,
                &msg,
                sizeof(CMD_MSG),
                NULL,
                0,
                &dwRetBytes);
            if (S_OK != hRet) {
                fprintf(stderr, "Send Command FAILED (0x%08x)\n", hRet);
                break;
            }

            std::cout << "Send BLOCK Command" << std::endl;

        } else if (chIn == 'q' || chIn == 'Q') {
            
            std::cout << "Quit" << std::endl;

            break;
        } else {
            std::cout << "Unknow" << std::endl;
        }
    }
    
    return 0;
}
