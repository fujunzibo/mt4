// Clean, minimal implementation of the shared-memory DLL used by MT4 EAs.
// ASCII-only, simple logging, with a small diagnostic code for EA OnInit.

#include "HedgeArbSharedMemory.h"
#include <string.h>

// Global state
static HANDLE      g_hMapFile = NULL;
static SharedData* g_pSharedData = NULL;
static std::string g_symbolName;
static const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";
static const int   SHARED_MEMORY_SIZE = sizeof(SharedData);

// Simple versioning/signature (layout check)
static const int   SHARED_DATA_VERSION = 1;
static const char  SHARED_DATA_SIGNATURE[8] = "HEDGEV1";

// Log file path (determined at runtime)
static char g_logPath[MAX_PATH] = {0};

// Get current timestamp in milliseconds
static unsigned long long GetCurrentTimestamp()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000ULL; // to milliseconds
}

// Get log file path (try multiple locations)
static const char* GetLogPath()
{
    // If already determined, return it
    if (g_logPath[0] != '\0')
        return g_logPath;

    // Try paths in order:
    // 1. C:\MT4\SharedMemQuoteDebug.log (production environment)
    // 2. D:\mt4\SharedMemQuoteDebug.log (development environment)
    // 3. Current directory
    // 4. Temp directory

    const char* paths[] = {
        "C:\\MT4\\SharedMemQuoteDebug.log",
        "D:\\mt4\\SharedMemQuoteDebug.log",
        ".\\SharedMemQuoteDebug.log",
        NULL  // Will be set to temp path if needed
    };

    // Try temp directory
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath) > 0)
    {
        char tempLogPath[MAX_PATH];
        _snprintf_s(tempLogPath, sizeof(tempLogPath), _TRUNCATE,
                   "%sSharedMemQuoteDebug.log", tempPath);
        paths[3] = tempLogPath;
    }

    // Try each path
    for (int i = 0; i < 4; i++)
    {
        if (paths[i] == NULL) continue;

        // Try to create/open the file
        HANDLE hFile = CreateFileA(paths[i],
                                   FILE_APPEND_DATA,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            // This path works, save it
            strncpy_s(g_logPath, sizeof(g_logPath), paths[i], _TRUNCATE);
            return g_logPath;
        }
    }

    // If all paths failed, use C:\MT4 as default (will fail silently if can't write)
    strncpy_s(g_logPath, sizeof(g_logPath), "C:\\MT4\\SharedMemQuoteDebug.log", _TRUNCATE);
    return g_logPath;
}

// Append a single line to a simple log file
static void AppendLogLine(const char* line)
{
    if (!line) return;

    const char* logPath = GetLogPath();
    HANDLE hFile = CreateFileA(logPath,
                               FILE_APPEND_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;  // Silently fail if can't write

    DWORD written = 0;
    WriteFile(hFile, line, (DWORD)strlen(line), &written, NULL);
    WriteFile(hFile, "\r\n", 2, &written, NULL);  // Windows line ending
    FlushFileBuffers(hFile);
    CloseHandle(hFile);
}

// Init shared memory
bool InitSharedMemory(const char* symbol)
{
    // Close previous mapping if any
    if (g_hMapFile != NULL)
    {
        if (g_pSharedData)
        {
            UnmapViewOfFile(g_pSharedData);
            g_pSharedData = NULL;
        }
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }

    if (!symbol || strlen(symbol) == 0)
    {
        AppendLogLine("[InitSharedMemory] ERROR: symbol is null or empty.");
        return false;
    }

    g_symbolName = symbol;
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;

    // Try open existing
    g_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mapName.c_str());
    bool isNewlyCreated = false;

    if (g_hMapFile == NULL)
    {
        // Create new
        g_hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            SHARED_MEMORY_SIZE,
            mapName.c_str()
        );
        if (g_hMapFile == NULL)
        {
            AppendLogLine("[InitSharedMemory] ERROR: CreateFileMappingA failed.");
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
    if (!g_pSharedData)
    {
        AppendLogLine("[InitSharedMemory] ERROR: MapViewOfFile failed.");
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }

    bool needInit = isNewlyCreated;
    if (!needInit)
    {
        if (memcmp(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature)) != 0 ||
            g_pSharedData->structVersion != SHARED_DATA_VERSION)
        {
            needInit = true;
        }
    }

    if (needInit)
    {
        memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);
        memcpy(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature));
        g_pSharedData->structVersion = SHARED_DATA_VERSION;
        g_pSharedData->version = 0;
        g_pSharedData->lockFlag = 0;
        AppendLogLine("[InitSharedMemory] SharedData initialized with signature/version.");
    }

    // Simple diagnostic code for EA OnInit check
    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));
    // Ensure null-terminated string (code[16] can hold "DLL_OK_INIT" + null)
    const char* diagCode = "DLL_OK_INIT";
    size_t diagLen = strlen(diagCode);
    if (diagLen < sizeof(g_pSharedData->command.code))
    {
        memcpy(g_pSharedData->command.code, diagCode, diagLen);
        g_pSharedData->command.code[diagLen] = '\0'; // explicit null terminator
    }
    else
    {
        // Fallback: truncate if too long (should not happen)
        memcpy(g_pSharedData->command.code, diagCode, sizeof(g_pSharedData->command.code) - 1);
        g_pSharedData->command.code[sizeof(g_pSharedData->command.code) - 1] = '\0';
    }
    g_pSharedData->command.isActive = false; // just diagnostic
    g_pSharedData->command.timestamp = 0;    // diagnostic code has no timestamp

    // Verify write by reading back
    char verifyBuf[32];
    memcpy(verifyBuf, g_pSharedData->command.code, sizeof(g_pSharedData->command.code));
    verifyBuf[sizeof(g_pSharedData->command.code)] = '\0';
    
    char logMsg[256];
    _snprintf_s(logMsg, sizeof(logMsg), _TRUNCATE,
        "[InitSharedMemory] Diagnostic command.code='%s' written (verified: '%s').",
        diagCode, verifyBuf);
    AppendLogLine(logMsg);

    return true;
}

// Set quote
bool SetQuote(const char* brokerId, double bid, double ask)
{
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

// Get quote
bool GetQuote(const char* brokerId, double* bid, double* ask)
{
    if (!g_pSharedData || !brokerId || !bid || !ask) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->quotes[index].isValid) return false;
    *bid = g_pSharedData->quotes[index].bid;
    *ask = g_pSharedData->quotes[index].ask;
    return true;
}

// Set profit
bool SetProfit(const char* brokerId, double profitPips, double profitUSD)
{
    if (!g_pSharedData || !brokerId) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    g_pSharedData->profits[index].profitPips = profitPips;
    g_pSharedData->profits[index].profitUSD = profitUSD;
    g_pSharedData->profits[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->profits[index].isValid = true;

    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// Get profit
bool GetProfit(const char* brokerId, double* profitPips, double* profitUSD)
{
    if (!g_pSharedData || !brokerId || !profitPips || !profitUSD) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->profits[index].isValid) return false;
    *profitPips = g_pSharedData->profits[index].profitPips;
    *profitUSD = g_pSharedData->profits[index].profitUSD;
    return true;
}

// Set command
bool SetCommand(const char* code, const char* longBroker, const char* shortBroker)
{
    if (!g_pSharedData || !code) return false;

    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));

    strncpy_s(g_pSharedData->command.code,
        sizeof(g_pSharedData->command.code),
        code,
        _TRUNCATE);
    if (longBroker && strlen(longBroker) > 0)
        strncpy_s(g_pSharedData->command.longBroker,
            sizeof(g_pSharedData->command.longBroker),
            longBroker,
            _TRUNCATE);
    if (shortBroker && strlen(shortBroker) > 0)
        strncpy_s(g_pSharedData->command.shortBroker,
            sizeof(g_pSharedData->command.shortBroker),
            shortBroker,
            _TRUNCATE);

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

// Get command
bool GetCommand(char* code, char* longBroker, char* shortBroker, long* timestamp, bool* isActive)
{
    if (!g_pSharedData) return false;

    // MQL4 strings usually provide ~256 bytes buffer; we use 256 to be safe.
    // Ensure null-terminated strings when copying
    if (code)
    {
        // Find actual length (may not be null-terminated in shared memory)
        size_t codeLen = 0;
        while (codeLen < sizeof(g_pSharedData->command.code) && 
               g_pSharedData->command.code[codeLen] != '\0')
            codeLen++;
        if (codeLen > 0)
        {
            memcpy(code, g_pSharedData->command.code, codeLen);
            code[codeLen] = '\0';
        }
        else
        {
            code[0] = '\0';
        }
    }
    if (longBroker)
    {
        size_t len = 0;
        while (len < sizeof(g_pSharedData->command.longBroker) && 
               g_pSharedData->command.longBroker[len] != '\0')
            len++;
        if (len > 0)
        {
            memcpy(longBroker, g_pSharedData->command.longBroker, len);
            longBroker[len] = '\0';
        }
        else
        {
            longBroker[0] = '\0';
        }
    }
    if (shortBroker)
    {
        size_t len = 0;
        while (len < sizeof(g_pSharedData->command.shortBroker) && 
               g_pSharedData->command.shortBroker[len] != '\0')
            len++;
        if (len > 0)
        {
            memcpy(shortBroker, g_pSharedData->command.shortBroker, len);
            shortBroker[len] = '\0';
        }
        else
        {
            shortBroker[0] = '\0';
        }
    }
    if (timestamp)
    {
        // Convert 64-bit timestamp to 32-bit long (MQL4 compatibility)
        unsigned long long ts64 = g_pSharedData->command.timestamp;
        *timestamp = (long)(ts64 & 0x7FFFFFFF); // truncate to 32-bit signed
    }
    if (isActive)
        *isActive = g_pSharedData->command.isActive;

    bool active = g_pSharedData->command.isActive;
    bool hasCode = (g_pSharedData->command.code[0] != '\0');

    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
        "[GetCommand] sym=%s code=\"%s\" long=\"%s\" short=\"%s\" ts=%llu active=%d hasCode=%d",
        g_symbolName.c_str(),
        g_pSharedData->command.code,
        g_pSharedData->command.longBroker,
        g_pSharedData->command.shortBroker,
        g_pSharedData->command.timestamp,
        (int)active, (int)hasCode);
    AppendLogLine(buf);

    // For diagnostics: if command.code is non-empty, treat as success,
    // so EA OnInit can confirm DLL is alive even if isActive == false.
    return (active || hasCode);
}

// Clear command
bool ClearCommand()
{
    if (!g_pSharedData) return false;
    g_pSharedData->command.code[0] = '\0';
    g_pSharedData->command.longBroker[0] = '\0';
    g_pSharedData->command.shortBroker[0] = '\0';
    g_pSharedData->command.timestamp = 0;
    g_pSharedData->command.isActive = false;
    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// Set ack
bool SetAck(const char* brokerId, const char* ackCode)
{
    if (!g_pSharedData || !brokerId || !ackCode) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    strncpy_s(g_pSharedData->acks[index].code,
        sizeof(g_pSharedData->acks[index].code),
        ackCode,
        _TRUNCATE);
    g_pSharedData->acks[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->acks[index].isValid = true;
    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// Get ack
bool GetAck(const char* brokerId, char* ackCode, long* timestamp)
{
    if (!g_pSharedData || !brokerId || !ackCode) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    if (!g_pSharedData->acks[index].isValid) return false;

    strncpy_s(ackCode,
        sizeof(g_pSharedData->acks[index].code),
        g_pSharedData->acks[index].code,
        _TRUNCATE);
    if (timestamp)
    {
        // Convert 64-bit timestamp to 32-bit long (MQL4 compatibility)
        unsigned long long ts64 = g_pSharedData->acks[index].timestamp;
        *timestamp = (long)(ts64 & 0x7FFFFFFF); // truncate to 32-bit signed
    }
    return true;
}

// Clear ack
bool ClearAck(const char* brokerId)
{
    if (!g_pSharedData || !brokerId) return false;
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    g_pSharedData->acks[index].code[0] = '\0';
    g_pSharedData->acks[index].timestamp = 0;
    g_pSharedData->acks[index].isValid = false;
    InterlockedIncrement(&g_pSharedData->version);
    return true;
}

// Close shared memory
void CloseSharedMemory()
{
    if (g_pSharedData)
    {
        UnmapViewOfFile(g_pSharedData);
        g_pSharedData = NULL;
    }
    if (g_hMapFile)
    {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}