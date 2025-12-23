#include "HedgeArbSharedMemory.h"
#include <string.h>
#include <time.h>
#include <fstream>
#include <stdio.h>
#include <shlobj.h>  // For SHGetFolderPathA

// 全局变量
static HANDLE g_hMapFile = NULL;
static SharedData* g_pSharedData = NULL;
static std::string g_symbolName;
static const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";
static const int SHARED_MEMORY_SIZE = sizeof(SharedData);

// 共享内存结构版本与签名
static const int SHARED_DATA_VERSION = 1;
static const char SHARED_DATA_SIGNATURE[8] = "HEDGEV1";

// 调试日志文件路径（使用Windows临时目录，避免硬编码盘符）
static char g_logFilePath[MAX_PATH] = {0};
static bool g_logPathInitialized = false;

// 获取日志文件完整路径（首次调用时初始化，尝试多个路径）
static const char* GetLogFilePath() {
    if (!g_logPathInitialized) {
        // 策略1：尝试使用临时目录
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) > 0) {
            _snprintf_s(g_logFilePath, sizeof(g_logFilePath), _TRUNCATE,
                        "%sHedgeArbSharedMemory_Debug.log", tempPath);
            // 测试是否可以写入
            HANDLE hTest = CreateFileA(g_logFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hTest != INVALID_HANDLE_VALUE) {
                CloseHandle(hTest);
                g_logPathInitialized = true;
                return g_logFilePath;
            }
        }
        
        // 策略2：尝试使用当前目录（MT4的MQL4\Libraries目录）
        char currentDir[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, currentDir) > 0) {
            _snprintf_s(g_logFilePath, sizeof(g_logFilePath), _TRUNCATE,
                        "%s\\HedgeArbSharedMemory_Debug.log", currentDir);
            HANDLE hTest = CreateFileA(g_logFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hTest != INVALID_HANDLE_VALUE) {
                CloseHandle(hTest);
                g_logPathInitialized = true;
                return g_logFilePath;
            }
        }
        
        // 策略3：使用系统目录（最后备选）
        char sysDir[MAX_PATH];
        if (GetSystemDirectoryA(sysDir, MAX_PATH) > 0) {
            _snprintf_s(g_logFilePath, sizeof(g_logFilePath), _TRUNCATE,
                        "%s\\HedgeArbSharedMemory_Debug.log", sysDir);
        } else {
            // 最后的备选：使用固定文件名
            strcpy_s(g_logFilePath, sizeof(g_logFilePath), "HedgeArbSharedMemory_Debug.log");
        }
        g_logPathInitialized = true;
    }
    return g_logFilePath;
}

// 注意：为简化兼容性，这里临时不使用自旋锁，直接依赖单进程顺序访问。
// 如果后续需要严格并发安全，可以再用 InterlockedCompareExchange 实现锁。

// 获取当前时间戳（毫秒）
unsigned long long GetCurrentTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000; // 转换为毫秒
}

// 将一行文本追加写入调试日志文件（使用Win32 API，更可靠）
static void AppendLogLine(const char* line) {
    if (!line) return;
    
    // 首先使用OutputDebugString输出到调试器（可以通过DebugView查看）
    OutputDebugStringA("[HedgeArbDLL] ");
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
    
    const char* logPath = GetLogFilePath();
    
    // 使用Win32 API写入，避免std::ofstream在DLL中的潜在问题
    HANDLE hFile = CreateFileA(
        logPath,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,  // 如果不存在则创建
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        // 如果创建失败，尝试创建目录后重试
        char dirPath[MAX_PATH];
        strncpy_s(dirPath, sizeof(dirPath), logPath, _TRUNCATE);
        char* lastSlash = strrchr(dirPath, '\\');
        if (lastSlash) {
            *lastSlash = '\0';
            CreateDirectoryA(dirPath, NULL);  // 尝试创建目录（忽略错误）
            // 重试打开文件
            hFile = CreateFileA(
                logPath,
                FILE_APPEND_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
        }
        
        // 如果仍然失败，尝试写入当前目录的备用文件
        if (hFile == INVALID_HANDLE_VALUE) {
            char currentDir[MAX_PATH];
            if (GetCurrentDirectoryA(MAX_PATH, currentDir) > 0) {
                char fallbackPath[MAX_PATH];
                _snprintf_s(fallbackPath, sizeof(fallbackPath), _TRUNCATE,
                           "%s\\HedgeArbSharedMemory_Debug_Fallback.log", currentDir);
                hFile = CreateFileA(
                    fallbackPath,
                    FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                if (hFile != INVALID_HANDLE_VALUE) {
                    logPath = fallbackPath;  // 使用备用路径
                }
            }
        }
    }
    
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten = 0;
        char logLine[512];
        SYSTEMTIME st;
        GetLocalTime(&st);
        _snprintf_s(logLine, sizeof(logLine), _TRUNCATE,
                    "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s\r\n",
                    st.wYear, st.wMonth, st.wDay,
                    st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                    line);
        WriteFile(hFile, logLine, (DWORD)strlen(logLine), &bytesWritten, NULL);
        CloseHandle(hFile);
    } else {
        // 如果所有路径都失败，尝试写入一个绝对路径的测试文件
        // 使用用户文档目录，通常有写入权限
        char userProfile[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, userProfile) == S_OK) {
            char testPath[MAX_PATH];
            _snprintf_s(testPath, sizeof(testPath), _TRUNCATE,
                       "%s\\HedgeArbSharedMemory_Error.log", userProfile);
            HANDLE hTest = CreateFileA(testPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hTest != INVALID_HANDLE_VALUE) {
                SYSTEMTIME st;
                GetLocalTime(&st);
                char errorMsg[512];
                _snprintf_s(errorMsg, sizeof(errorMsg), _TRUNCATE,
                           "[%04d-%02d-%02d %02d:%02d:%02d.%03d] LOG_ERROR: Failed to write to normal log path. Original message: %s\r\n",
                           st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, line);
                DWORD written = 0;
                WriteFile(hTest, errorMsg, (DWORD)strlen(errorMsg), &written, NULL);
                CloseHandle(hTest);
            }
        }
    }
    // 注意：即使写入失败也不抛出异常，避免影响主程序
}

// 初始化共享内存连接
bool InitSharedMemory(const char* symbol) {
    // 立即写入日志，确认日志功能正常（即使后续失败也要记录）
    AppendLogLine("[InitSharedMemory] Function called.");
    
    if (g_hMapFile != NULL) {
        // 已经初始化，先关闭
        CloseSharedMemory();
    }
    
    if (symbol == NULL || strlen(symbol) == 0) {
        AppendLogLine("[InitSharedMemory] ERROR: symbol is NULL or empty.");
        return false;
    }
    
    g_symbolName = symbol;
    std::string mapName = SHARED_MEMORY_NAME_PREFIX + g_symbolName;
    
    // 先尝试打开已存在的共享内存
    g_hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        mapName.c_str()
    );
    
    bool isNewlyCreated = false;
    
    if (g_hMapFile == NULL) {
        // 如果不存在，创建新的共享内存
        // 注意：必须在CreateFileMapping之前调用SetLastError(0)，清除之前的错误码
        SetLastError(0);
        g_hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            SHARED_MEMORY_SIZE,
            mapName.c_str()
        );
        
        if (g_hMapFile == NULL) {
            // 创建失败
            DWORD error = GetLastError();
            return false;
        }
        
        // 检查是否是新建的（GetLastError在CreateFileMapping后立即调用才有效）
        DWORD lastError = GetLastError();
        isNewlyCreated = (lastError != ERROR_ALREADY_EXISTS);
    }
    // 如果OpenFileMapping成功，说明共享内存已存在，不需要初始化
    
    // 映射到进程地址空间
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    );
    
    if (g_pSharedData == NULL) {
        // 映射失败
        DWORD error = GetLastError();
        if (g_hMapFile != NULL) {
            CloseHandle(g_hMapFile);
            g_hMapFile = NULL;
        }
        return false;
    }
    
    // ==== 修复：初始化数据结构并设置版本 / 签名 ====
    // 规则：
    // 1) 如果是新创建的内存，一定要初始化并写入签名/版本
    // 2) 如果不是新创建，但签名或版本不匹配，也视为“需要初始化”（兼容旧布局或脏数据）
    bool needInit = isNewlyCreated;
    if (!needInit) {
        if (memcmp(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature)) != 0 ||
            g_pSharedData->structVersion != SHARED_DATA_VERSION) {
            needInit = true;
        }
    }

    if (needInit) {
        memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);
        memcpy(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature));
        g_pSharedData->structVersion = SHARED_DATA_VERSION;
        g_pSharedData->version = 0;
        g_pSharedData->lockFlag = 0;
        AppendLogLine("[InitSharedMemory] SharedData initialized/reinitialized with signature/version.");
    }
    
    // 写入初始化成功日志，确认日志功能正常
    char initMsg[256];
    _snprintf_s(initMsg, sizeof(initMsg), _TRUNCATE,
                "[InitSharedMemory] SUCCESS: symbol=%s mapName=%s isNewlyCreated=%d",
                g_symbolName.c_str(), mapName.c_str(), (int)isNewlyCreated);
    AppendLogLine(initMsg);
    
    return true;
}

// 设置报价
bool SetQuote(const char* brokerId, double bid, double ask) {
    if (g_pSharedData == NULL) return false;
    if (bid <= 0 || ask <= 0 || ask <= bid) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    unsigned long long ts = GetCurrentTimestamp();
    g_pSharedData->quotes[index].bid = bid;
    g_pSharedData->quotes[index].ask = ask;
    g_pSharedData->quotes[index].timestamp = ts;
    g_pSharedData->quotes[index].isValid = true;
    
    InterlockedIncrement(&g_pSharedData->version);

    // 追加调试日志（记录写入的报价及其在 SharedData 中的大致偏移）
    char buf[256];
    size_t offset = (size_t)((char*)&(g_pSharedData->quotes[index]) - (char*)g_pSharedData);
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[SetQuote] sym=%s broker=%s idx=%d bid=%.5f ask=%.5f ts=%llu offset=%zu",
                g_symbolName.c_str(), brokerId, index, bid, ask, ts, offset);
    AppendLogLine(buf);
    
    return true;
}

// 获取报价
bool GetQuote(const char* brokerId, double* bid, double* ask) {
    if (g_pSharedData == NULL || bid == NULL || ask == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    if (!g_pSharedData->quotes[index].isValid) {
        return false;
    }
    
    *bid = g_pSharedData->quotes[index].bid;
    *ask = g_pSharedData->quotes[index].ask;
    
    return true;
}

// 设置盈亏
bool SetProfit(const char* brokerId, double profitPips, double profitUSD) {
    if (g_pSharedData == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    g_pSharedData->profits[index].profitPips = profitPips;
    g_pSharedData->profits[index].profitUSD = profitUSD;
    g_pSharedData->profits[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->profits[index].isValid = true;
    
    InterlockedIncrement(&g_pSharedData->version);
    
    return true;
}

// 获取盈亏
bool GetProfit(const char* brokerId, double* profitPips, double* profitUSD) {
    if (g_pSharedData == NULL || profitPips == NULL || profitUSD == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    if (!g_pSharedData->profits[index].isValid) {
        return false;
    }
    
    *profitPips = g_pSharedData->profits[index].profitPips;
    *profitUSD = g_pSharedData->profits[index].profitUSD;
    
    return true;
}

// 设置指令
bool SetCommand(const char* code, const char* longBroker, const char* shortBroker) {
    if (g_pSharedData == NULL || code == NULL) return false;
    
    // ==== 修复：确保数据正确设置，并清空未使用的部分 ====
    // 清空整个command结构，避免残留数据
    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));
    
    // 复制指令代码
    strncpy_s(g_pSharedData->command.code, sizeof(g_pSharedData->command.code), code, _TRUNCATE);
    g_pSharedData->command.code[sizeof(g_pSharedData->command.code) - 1] = '\0'; // 确保以null结尾
    
    // 复制longBroker
    if (longBroker && strlen(longBroker) > 0) {
        strncpy_s(g_pSharedData->command.longBroker, sizeof(g_pSharedData->command.longBroker), longBroker, _TRUNCATE);
        g_pSharedData->command.longBroker[sizeof(g_pSharedData->command.longBroker) - 1] = '\0'; // 确保以null结尾
    } else {
        g_pSharedData->command.longBroker[0] = '\0';
    }
    
    // 复制shortBroker
    if (shortBroker && strlen(shortBroker) > 0) {
        strncpy_s(g_pSharedData->command.shortBroker, sizeof(g_pSharedData->command.shortBroker), shortBroker, _TRUNCATE);
        g_pSharedData->command.shortBroker[sizeof(g_pSharedData->command.shortBroker) - 1] = '\0'; // 确保以null结尾
    } else {
        g_pSharedData->command.shortBroker[0] = '\0';
    }
    
    unsigned long long ts = GetCurrentTimestamp();
    g_pSharedData->command.timestamp = ts;
    g_pSharedData->command.isActive = true;
    
    InterlockedIncrement(&g_pSharedData->version);

    // 记录指令写入日志，便于发现异常字符串
    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[SetCommand] sym=%s code=\"%s\" long=\"%s\" short=\"%s\" ts=%llu",
                g_symbolName.c_str(),
                g_pSharedData->command.code,
                g_pSharedData->command.longBroker,
                g_pSharedData->command.shortBroker,
                ts);
    AppendLogLine(buf);
    
    return true;
}

// 获取指令
bool GetCommand(char* code, char* longBroker, char* shortBroker, unsigned long long* timestamp, bool* isActive) {
    if (g_pSharedData == NULL) {
        // g_pSharedData为NULL，说明共享内存未初始化
        return false;
    }
    
    // ==== 修复：即使isActive为false，也要返回数据，让调用者知道指令存在但未激活 ====
    // 这样GetGlobalCommand可以区分"指令不存在"和"指令存在但未激活"
    
    // ==== 修复：确保数据正确复制，并验证数据有效性 ====
    // MQL4的string类型在传递给DLL时，会分配固定大小的缓冲区（通常是256字节）
    // 我们需要确保正确复制字符串，并确保以null结尾
    
    // 复制指令数据（无论isActive是否为true）
    if (code) {
        // MQL4的string缓冲区通常足够大，但我们仍然需要确保正确复制
        // 先清空缓冲区（MQL4可能已经分配了缓冲区，但内容未初始化）
        // 注意：MQL4的string缓冲区大小是固定的，我们不能假设它的大小
        // 使用strcpy_s而不是strncpy_s，因为MQL4的string缓冲区足够大
        strcpy_s(code, 256, g_pSharedData->command.code); // MQL4 string缓冲区通常是256字节
    }
    if (longBroker) {
        strcpy_s(longBroker, 256, g_pSharedData->command.longBroker);
    }
    if (shortBroker) {
        strcpy_s(shortBroker, 256, g_pSharedData->command.shortBroker);
    }
    if (timestamp) {
        *timestamp = g_pSharedData->command.timestamp;
    }
    if (isActive) {
        *isActive = g_pSharedData->command.isActive;
    }
    
    // 只有在isActive为true时才返回true
    // 如果isActive为false，返回false，但数据已经复制到输出参数中
    bool active = g_pSharedData->command.isActive;

    // 读取指令时也写一条日志，用于比对 EA 端看到的内容
    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "[GetCommand] sym=%s code=\"%s\" long=\"%s\" short=\"%s\" ts=%llu active=%d",
                g_symbolName.c_str(),
                g_pSharedData->command.code,
                g_pSharedData->command.longBroker,
                g_pSharedData->command.shortBroker,
                g_pSharedData->command.timestamp,
                (int)active);
    AppendLogLine(buf);

    return active;
}

// 清除指令
bool ClearCommand() {
    if (g_pSharedData == NULL) return false;
    
    g_pSharedData->command.code[0] = '\0';
    g_pSharedData->command.longBroker[0] = '\0';
    g_pSharedData->command.shortBroker[0] = '\0';
    g_pSharedData->command.timestamp = 0;
    g_pSharedData->command.isActive = false;
    
    InterlockedIncrement(&g_pSharedData->version);
    
    return true;
}

// 设置确认
bool SetAck(const char* brokerId, const char* ackCode) {
    if (g_pSharedData == NULL || ackCode == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    strncpy_s(g_pSharedData->acks[index].code, sizeof(g_pSharedData->acks[index].code), ackCode, _TRUNCATE);
    g_pSharedData->acks[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->acks[index].isValid = true;
    
    InterlockedIncrement(&g_pSharedData->version);
    
    return true;
}

// 获取确认
bool GetAck(const char* brokerId, char* ackCode, unsigned long long* timestamp) {
    if (g_pSharedData == NULL || ackCode == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    if (!g_pSharedData->acks[index].isValid) {
        return false;
    }
    
    strncpy_s(ackCode, 16, g_pSharedData->acks[index].code, _TRUNCATE);
    if (timestamp) {
        *timestamp = g_pSharedData->acks[index].timestamp;
    }
    
    return true;
}

// 清除确认
bool ClearAck(const char* brokerId) {
    if (g_pSharedData == NULL) return false;
    
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    g_pSharedData->acks[index].code[0] = '\0';
    g_pSharedData->acks[index].timestamp = 0;
    g_pSharedData->acks[index].isValid = false;
    
    InterlockedIncrement(&g_pSharedData->version);
    
    return true;
}

// 重置共享内存（清空后重新写入签名/版本，版本计数清零）
void ResetSharedMemory() {
    if (g_pSharedData == NULL) {
        AppendLogLine("[ResetSharedMemory] g_pSharedData is NULL, nothing to reset.");
        return;
    }

    AppendLogLine("[ResetSharedMemory] begin reset SharedData.");

    memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);
    memcpy(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature));
    g_pSharedData->structVersion = SHARED_DATA_VERSION;
    g_pSharedData->version = 0;
    g_pSharedData->lockFlag = 0;

    AppendLogLine("[ResetSharedMemory] end reset SharedData.");
}

// 将当前 SharedData 的关键字段写入调试日志
void DumpSharedData() {
    if (g_pSharedData == NULL) {
        AppendLogLine("[DumpSharedData] g_pSharedData is NULL.");
        return;
    }

    AppendLogLine("===== DumpSharedData BEGIN =====");

    char buf[512];

    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "Header: signature=\"%.8s\" structVersion=%d version=%ld lockFlag=%ld sym=%s",
                g_pSharedData->signature,
                g_pSharedData->structVersion,
                g_pSharedData->version,
                g_pSharedData->lockFlag,
                g_symbolName.c_str());
    AppendLogLine(buf);

    for (int i = 0; i < 2; ++i) {
        size_t qOffset = (size_t)((char*)&(g_pSharedData->quotes[i]) - (char*)g_pSharedData);
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "Quote[%d]: offset=%zu bid=%.5f ask=%.5f ts=%llu isValid=%d",
                    i,
                    qOffset,
                    g_pSharedData->quotes[i].bid,
                    g_pSharedData->quotes[i].ask,
                    g_pSharedData->quotes[i].timestamp,
                    (int)g_pSharedData->quotes[i].isValid);
        AppendLogLine(buf);
    }

    for (int i = 0; i < 2; ++i) {
        size_t pOffset = (size_t)((char*)&(g_pSharedData->profits[i]) - (char*)g_pSharedData);
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "Profit[%d]: offset=%zu pips=%.2f usd=%.2f ts=%llu isValid=%d",
                    i,
                    pOffset,
                    g_pSharedData->profits[i].profitPips,
                    g_pSharedData->profits[i].profitUSD,
                    g_pSharedData->profits[i].timestamp,
                    (int)g_pSharedData->profits[i].isValid);
        AppendLogLine(buf);
    }

    size_t cOffset = (size_t)((char*)&(g_pSharedData->command) - (char*)g_pSharedData);
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "Command: offset=%zu code=\"%s\" long=\"%s\" short=\"%s\" ts=%llu isActive=%d",
                cOffset,
                g_pSharedData->command.code,
                g_pSharedData->command.longBroker,
                g_pSharedData->command.shortBroker,
                g_pSharedData->command.timestamp,
                (int)g_pSharedData->command.isActive);
    AppendLogLine(buf);

    for (int i = 0; i < 2; ++i) {
        size_t aOffset = (size_t)((char*)&(g_pSharedData->acks[i]) - (char*)g_pSharedData);
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "Ack[%d]: offset=%zu code=\"%s\" ts=%llu isValid=%d",
                    i,
                    aOffset,
                    g_pSharedData->acks[i].code,
                    g_pSharedData->acks[i].timestamp,
                    (int)g_pSharedData->acks[i].isValid);
        AppendLogLine(buf);
    }

    AppendLogLine("===== DumpSharedData END =====");
}

// 关闭共享内存连接
void CloseSharedMemory() {
    if (g_pSharedData != NULL) {
        UnmapViewOfFile(g_pSharedData);
        g_pSharedData = NULL;
    }
    
    if (g_hMapFile != NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}


