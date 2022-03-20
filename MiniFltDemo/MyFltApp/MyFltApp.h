#ifndef MY_FLT_APP
#define MY_FLT_APP

#include <Windows.h>
#include <vector>
#include <string>

#define MINISPY_PORT_NAME    L"\\MyFltPort"

typedef enum _MY_FLT_CMD {
    EM_PASS = 0,
    EM_BLOCK
} MY_FLT_CMD;

typedef struct _CMD_MSG {
    MY_FLT_CMD cmd;
} CMD_MSG, *PCMD_MSG;

class MyFltApp
{
public:
    MyFltApp ();
    virtual ~MyFltApp () = NULL;
    void SendMsg(CMD_MSG msg);

private:
    HANDLE m_hDll;
    bool LoadDll(void);
};

#endif // !MY_FLT_APP