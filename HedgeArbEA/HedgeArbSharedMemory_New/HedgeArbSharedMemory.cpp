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

// Helper: Create directory if it doesn't exist
static bool EnsureDirectoryExists(const char* filePath)
{
    char dirPath[MAX_PATH];
    strncpy_s(dirPath, sizeof(dirPath), filePath, _TRUNCATE);
    
    // Find last backslash
    char* lastSlash = strrchr(dirPath, '\\');
    if (lastSlash == NULL)
        return true;  // No directory component
    
    *lastSlash = '\0';  // Remove filename, keep directory
    
    // Try to create directory
    DWORD dwAttrib = GetFileAttributesA(dirPath);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES)
        return true;  // Directory exists
    
    // Create directory (recursive)
    if (CreateDirectoryA(dirPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
        return true;
    
    return false;
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

        // Ensure directory exists
        if (!EnsureDirectoryExists(paths[i]))
            continue;

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
// Also outputs to DebugView via OutputDebugStringA
static void AppendLogLine(const char* line)
{
    if (!line) return;

    // Format complete message for DebugView (single call to avoid fragmentation)
    char debugMsg[512];
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE, "[HedgeArbDLL] %s", line);
    OutputDebugStringA(debugMsg);

    // Also try to write to file
    const char* logPath = GetLogPath();
    HANDLE hFile = CreateFileA(logPath,
                               FILE_APPEND_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // Log failure to DebugView
        char errMsg[256];
        DWORD err = GetLastError();
        _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
                   "[HedgeArbDLL] ERROR: Failed to write log file '%s', error=%lu",
                   logPath, err);
        OutputDebugStringA(errMsg);
        return;
    }

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
    else
    {
        // Shared memory already exists - log current state
        char existingCode[32];
        memcpy(existingCode, g_pSharedData->command.code, sizeof(g_pSharedData->command.code));
        existingCode[sizeof(g_pSharedData->command.code)] = '\0';
        char logExisting[256];
        _snprintf_s(logExisting, sizeof(logExisting), _TRUNCATE,
            "[InitSharedMemory] Shared memory already exists. Current command.code='%s' (len=%zu)",
            existingCode, strlen(existingCode));
        AppendLogLine(logExisting);
    }

    // ==== CRITICAL: Always write diagnostic code, regardless of needInit ====
    // This ensures EA can verify DLL is working, even if shared memory was created by another process
    // Simple diagnostic code for EA OnInit check
    // Clear the entire command structure first
    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));
    
    // Write diagnostic code "DLL_OK_INIT" (11 bytes + null = 12 bytes total, fits in code[16])
    const char* diagCode = "DLL_OK_INIT";
    size_t diagLen = strlen(diagCode);  // Should be 11
    
    // Ensure we don't overflow
    size_t copyLen = (diagLen < sizeof(g_pSharedData->command.code)) ? diagLen : (sizeof(g_pSharedData->command.code) - 1);
    
    // Write the string
    memcpy(g_pSharedData->command.code, diagCode, copyLen);
    g_pSharedData->command.code[copyLen] = '\0';  // Explicit null terminator
    
    // Set other fields
    g_pSharedData->command.isActive = false;  // Diagnostic, not a real command
    g_pSharedData->command.timestamp = 0;     // No timestamp for diagnostic
    
    // Memory barrier to ensure writes are visible
    _ReadWriteBarrier();
    
    // Force memory flush to ensure writes are visible to other processes
    // Note: FlushViewOfFile flushes the entire view, but we only need the command structure
    if (g_hMapFile != NULL)
    {
        FlushViewOfFile(g_pSharedData, 0);  // 0 = flush entire view
    }
    
    // Verify write by reading back immediately
    char verifyBuf[32];
    memcpy(verifyBuf, g_pSharedData->command.code, sizeof(g_pSharedData->command.code));
    verifyBuf[sizeof(g_pSharedData->command.code)] = '\0';
    
    // Log hex dump for debugging
    char hexDump[128];
    hexDump[0] = '\0';
    for (size_t i = 0; i < 16 && i < sizeof(g_pSharedData->command.code); i++)
    {
        char hexByte[4];
        _snprintf_s(hexByte, sizeof(hexByte), _TRUNCATE, "%02X ", (unsigned char)g_pSharedData->command.code[i]);
        strcat_s(hexDump, sizeof(hexDump), hexByte);
    }
    
    char logMsg[512];
    _snprintf_s(logMsg, sizeof(logMsg), _TRUNCATE,
        "[InitSharedMemory] Diagnostic code written: expected='%s' (len=%zu), verified='%s', hex=%s",
        diagCode, diagLen, verifyBuf, hexDump);
    AppendLogLine(logMsg);
    
    // Double-check: if verification failed, try writing again
    if (strcmp(verifyBuf, diagCode) != 0)
    {
        AppendLogLine("[InitSharedMemory] WARNING: First write verification failed, retrying...");
        memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));
        memcpy(g_pSharedData->command.code, diagCode, copyLen);
        g_pSharedData->command.code[copyLen] = '\0';
        g_pSharedData->command.isActive = false;
        g_pSharedData->command.timestamp = 0;
        _ReadWriteBarrier();
        if (g_hMapFile != NULL)
        {
            FlushViewOfFile(g_pSharedData, 0);
        }
        
        // Verify again
        memcpy(verifyBuf, g_pSharedData->command.code, sizeof(g_pSharedData->command.code));
        verifyBuf[sizeof(g_pSharedData->command.code)] = '\0';
        _snprintf_s(logMsg, sizeof(logMsg), _TRUNCATE,
            "[InitSharedMemory] After retry: verified='%s'", verifyBuf);
        AppendLogLine(logMsg);
    }

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

    // Debug: Log raw memory content
    char hexBuf[128];
    hexBuf[0] = '\0';
    for (int i = 0; i < 16 && i < sizeof(g_pSharedData->command.code); i++)
    {
        char hexByte[4];
        _snprintf_s(hexByte, sizeof(hexByte), _TRUNCATE, "%02X ", (unsigned char)g_pSharedData->command.code[i]);
        strcat_s(hexBuf, sizeof(hexBuf), hexByte);
    }
    char debugMsg[256];
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
        "[GetCommand] RAW MEMORY (first 16 bytes): %s", hexBuf);
    AppendLogLine(debugMsg);

    // MQL4 strings usually provide ~256 bytes buffer; we use 256 to be safe.
    // Ensure null-terminated strings when copying
    if (code)
    {
        // Copy with explicit null termination
        // First, ensure we don't read beyond the buffer
        size_t maxCopy = sizeof(g_pSharedData->command.code);
        if (maxCopy > 255) maxCopy = 255;  // MQL4 string buffer is typically 256 bytes
        
        // Copy the string, ensuring null termination
        memcpy(code, g_pSharedData->command.code, maxCopy);
        code[maxCopy] = '\0';  // Ensure null terminator
        
        // Find actual length (stop at first null or end of buffer)
        size_t actualLen = 0;
        while (actualLen < maxCopy && code[actualLen] != '\0')
            actualLen++;
        
        // If no null found, explicitly null-terminate at the end
        if (actualLen >= maxCopy)
        {
            code[maxCopy - 1] = '\0';
            actualLen = maxCopy - 1;
        }
        
        // Log what we're copying
        char copyMsg[256];
        _snprintf_s(copyMsg, sizeof(copyMsg), _TRUNCATE,
            "[GetCommand] Copying code: len=%zu content=\"%s\"", actualLen, code);
        AppendLogLine(copyMsg);
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