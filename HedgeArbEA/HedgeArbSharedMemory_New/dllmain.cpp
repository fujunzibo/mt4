// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <windows.h>
#include <stdio.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            // 测试日志功能：在DLL加载时写入测试文件（尝试多个位置）
            SYSTEMTIME st;
            GetLocalTime(&st);
            char msg[512];
            _snprintf_s(msg, sizeof(msg), _TRUNCATE,
                       "[DllMain] DLL loaded at %04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
                       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            
            // 输出到调试器
            OutputDebugStringA("[HedgeArbDLL] DLL_PROCESS_ATTACH\n");
            OutputDebugStringA(msg);
            
            // 尝试多个路径写入文件
            HANDLE hFile = INVALID_HANDLE_VALUE;
            
            // 路径1：临时目录
            char tempPath[MAX_PATH];
            if (GetTempPathA(MAX_PATH, tempPath) > 0) {
                char logPath[MAX_PATH];
                _snprintf_s(logPath, sizeof(logPath), _TRUNCATE,
                           "%sHedgeArbSharedMemory_DllMain_Test.log", tempPath);
                hFile = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            
            // 路径2：当前目录（如果路径1失败）
            if (hFile == INVALID_HANDLE_VALUE) {
                char currentDir[MAX_PATH];
                if (GetCurrentDirectoryA(MAX_PATH, currentDir) > 0) {
                    char logPath[MAX_PATH];
                    _snprintf_s(logPath, sizeof(logPath), _TRUNCATE,
                               "%s\\HedgeArbSharedMemory_DllMain_Test.log", currentDir);
                    hFile = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                }
            }
            
            // 路径3：C盘根目录（如果前两个都失败）
            if (hFile == INVALID_HANDLE_VALUE) {
                hFile = CreateFileA("C:\\HedgeArbSharedMemory_DllMain_Test.log",
                                   FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(hFile, msg, (DWORD)strlen(msg), &written, NULL);
                CloseHandle(hFile);
            }
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

