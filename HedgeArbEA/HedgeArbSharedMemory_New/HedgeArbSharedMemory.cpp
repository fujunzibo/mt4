#include "HedgeArbSharedMemory.h"
#include <string.h>
#include <time.h>
#include <fstream>

// 全局变量
static HANDLE     g_hMapFile = NULL;
static SharedData* g_pSharedData = NULL;
static std::string g_symbolName;
static const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";
static const int   SHARED_MEMORY_SIZE = sizeof(SharedData);

// 共享内存结构版本与签名
static const int  SHARED_DATA_VERSION = 1;
static const char SHARED_DATA_SIGNATURE[8] = "HEDGEV1";

// 调试日志文件路径
static const char* SHARED_MEM_LOG_PATH = "D:\\mt4\\SharedMemQuoteDebug.log";

// 获取当前时间戳（毫秒）
static unsigned long long GetCurrentTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000; // 毫秒
}

// 简单文件追加日志
static void AppendLogLine(const char* line) {
    if (!line) return;
    std::ofstream ofs(SHARED_MEM_LOG_PATH, std::ios::out | std::ios::app);
    if (!ofs.is_open()) return;
    ofs << line << std::endl;
}

// 初始化共享内存连接
bool InitSharedMemory(const char* symbol) {
    if (g_hMapFile != NULL) {
        if (g_pSharedData) {
            UnmapViewOfFile(g_pSharedData);
            g_pSharedData = NULL;
        }
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }

    if (!symbol || strlen(symbol) == 0) {
        return false;
    }

    g_symbolName = symbol;
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;

    // 先尝试打开已存在的共享内存
    g_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mapName.c_str());

    bool isNewlyCreated = false;

    if (g_hMapFile == NULL) {
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
            return false;
        }
        DWORD lastError = GetLastError();
        isNewlyCreated = (lastError != ERROR_ALREADY_EXISTS);
    }

    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    );
    if (!g_pSharedData) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }

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
        AppendLogLine("[InitSharedMemory] SharedData reinitialized with new signature/version.");
    }

    return true;
}

// 设置报价
bool SetQuote(const char* brokerId, double bid, double ask) {
    if (!g_pSharedData || !brokerId) return false;
    if (bid <= 0 || ask <= 0 || ask <= bid) return false;

    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    unsigned long long ts = GetCurrentTimestamp();
    g_pSharedData->quotes[index].bid = bid;
    g_pSharedData->quotes[index].ask = ask;
    g_pSharedData->quotes[index].timestamp = ts;
    g_pSharedData->quotes[index].isValid = true;

    InterlockedIncrement(&g_pSharedData->version);

    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
        "[SetQuote] sym=%s broker=%s idx=%d bid=%.5f ask=%.5f ts=%llu",
        g_symbolName.c_str(), brokerId, index, bid, ask, ts);
    AppendLogLine(buf);

    return true;
}

// 获取报价
bool GetQuote(const char* brokerId, double* bid, double* ask) {
    if (!g_pSharedData || !brokerId || !bid || !ask) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->quotes[index].isValid) return false;
    *bid = g_pSharedData->quotes[index].bid;
    *ask = g_pSharedData->quotes[index].ask;
    return true;
}

// 设置盈亏
bool SetProfit(const char* brokerId, double profitPips, double profitUSD) {
    if (!g_pSharedData || !brokerId) return false;
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
    if (!g_pSharedData || !brokerId || !profitPips || !profitUSD) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->profits[index].isValid) return false;
    *profitPips = g_pSharedData->profits[index].profitPips;
    *profitUSD = g_pSharedData->profits[index].profitUSD;
    return true;
}

// 设置指令
bool SetCommand(const char* code, const char* longBroker, const char* shortBroker) {
    if (!g_pSharedData || !code) return false;

    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));

    strncpy_s(g_pSharedData->command.code, sizeof(g_pSharedData->command.code), code, _TRUNCATE);
    if (longBroker && strlen(longBroker) > 0)
        strncpy_s(g_pSharedData->command.longBroker, sizeof(g_pSharedData->command.longBroker), longBroker, _TRUNCATE);
    if (shortBroker && strlen(shortBroker) > 0)
        strncpy_s(g_pSharedData->command.shortBroker, sizeof(g_pSharedData->command.shortBroker), shortBroker, _TRUNCATE);

    unsigned long long ts = GetCurrentTimestamp();
    g_pSharedData->command.timestamp = ts;
    g_pSharedData->command.isActive = true;

    InterlockedIncrement(&g_pSharedData->version);

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
    if (!g_pSharedData) return false;

    if (code)
        strncpy_s(code, 16, g_pSharedData->command.code, _TRUNCATE);
    if (longBroker)
        strncpy_s(longBroker, 4, g_pSharedData->command.longBroker, _TRUNCATE);
    if (shortBroker)
        strncpy_s(shortBroker, 4, g_pSharedData->command.shortBroker, _TRUNCATE);
    if (timestamp)
        *timestamp = g_pSharedData->command.timestamp;
    if (isActive)
        *isActive = g_pSharedData->command.isActive;

    bool active = g_pSharedData->command.isActive;

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
    if (!g_pSharedData) return false;
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
    if (!g_pSharedData || !brokerId || !ackCode) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    strncpy_s(g_pSharedData->acks[index].code, sizeof(g_pSharedData->acks[index].code), ackCode, _TRUNCATE);
    g_pSharedData->acks[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->acks[index].isValid = true;
    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// 获取确认
bool GetAck(const char* brokerId, char* ackCode, unsigned long long* timestamp) {
    if (!g_pSharedData || !brokerId || !ackCode) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->acks[index].isValid) return false;

    strncpy_s(ackCode, 16, g_pSharedData->acks[index].code, _TRUNCATE);
    if (timestamp)
        *timestamp = g_pSharedData->acks[index].timestamp;
    return true;
}

// 清除确认
bool ClearAck(const char* brokerId) {
    if (!g_pSharedData || !brokerId) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    g_pSharedData->acks[index].code[0] = '\0';
    g_pSharedData->acks[index].timestamp = 0;
    g_pSharedData->acks[index].isValid = false;
    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// 关闭共享内存连接
void CloseSharedMemory() {
    if (g_pSharedData) {
        UnmapViewOfFile(g_pSharedData);
        g_pSharedData = NULL;
    }
    if (g_hMapFile) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}