// Clean, minimal implementation of the shared-memory DLL used by MT4 EAs.
// ASCII-only, simple logging, with a small diagnostic code for EA OnInit.

#include "HedgeArbSharedMemory.h"
#include <string.h>
#include <windows.h>

// Global state
static HANDLE      g_hMapFile = NULL;
static SharedData* g_pSharedData = NULL;
static std::string g_symbolName;
static std::string g_pendingSymbol;  // For SetSymbolForInit pattern
static std::string g_pendingCommandCode;  // For SetCommandParams pattern
static std::string g_pendingLongBroker;   // For SetCommandParams pattern
static std::string g_pendingShortBroker;  // For SetCommandParams pattern
static std::string g_pendingAckBrokerId;  // For SetAckParams pattern
static std::string g_pendingAckCode;      // For SetAckParams pattern
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

// Helper: Check if memory is readable (simple check)
static bool IsReadableMemory(const void* ptr, size_t size)
{
    if (!ptr) return false;
    
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
        return false;
    
    // Check if memory is committed and readable
    if (mbi.State != MEM_COMMIT)
        return false;
    
    if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        return false;
    
    // Check if the requested size is within the region
    if ((char*)ptr + size > (char*)mbi.BaseAddress + mbi.RegionSize)
        return false;
    
    return true;
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

// Set symbol for InitSharedMemory (avoids MQL4 string passing issues)
// Uses unsigned char* to receive MQL4 uchar array (official MQL4 method)
bool SetSymbolForInit(unsigned char* symbol)
{
    if (!symbol)
    {
        AppendLogLine("[SetSymbolForInit] ERROR: symbol parameter is NULL.");
        return false;
    }
    
    // MQL4 passes uchar array, which is null-terminated
    // Convert unsigned char* to char* for string operations
    const char* symbolStr = reinterpret_cast<const char*>(symbol);
    size_t directStrLen = strlen(symbolStr);
    
    char entryMsg[512];
    _snprintf_s(entryMsg, sizeof(entryMsg), _TRUNCATE,
        "[SetSymbolForInit] FUNCTION CALLED: strlen=%zu, symbol='%s'",
        directStrLen, symbolStr);
    AppendLogLine(entryMsg);
    
    // MQL4 passes uchar array which is null-terminated, so we can use it directly
    if (directStrLen == 0)
    {
        AppendLogLine("[SetSymbolForInit] ERROR: Empty string received.");
        return false;
    }
    
    // Use the string directly (MQL4 uchar array is null-terminated)
    if (directStrLen > 0 && directStrLen <= 256)
    {
        g_pendingSymbol = symbolStr;
        char debugMsg[256];
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[SetSymbolForInit] SUCCESS: Received uchar array: symbol='%s' (len=%zu)",
            g_pendingSymbol.c_str(), directStrLen);
        AppendLogLine(debugMsg);
        return true;
    }
    
    AppendLogLine("[SetSymbolForInit] ERROR: Cannot extract valid symbol.");
    return false;
}

// Set command parameters (avoids MQL4 string passing issues)
// Uses unsigned char* to receive MQL4 uchar arrays (official MQL4 method)
bool SetCommandParams(unsigned char* code, unsigned char* longBroker, unsigned char* shortBroker)
{
    // Clear previous values
    g_pendingCommandCode.clear();
    g_pendingLongBroker.clear();
    g_pendingShortBroker.clear();
    
    if (code)
    {
        const char* codeStr = reinterpret_cast<const char*>(code);
        size_t codeLen = strlen(codeStr);
        if (codeLen > 0 && codeLen <= 256)
        {
            g_pendingCommandCode = codeStr;
        }
    }
    
    if (longBroker)
    {
        const char* longBrokerStr = reinterpret_cast<const char*>(longBroker);
        size_t longBrokerLen = strlen(longBrokerStr);
        if (longBrokerLen > 0 && longBrokerLen <= 256)
        {
            g_pendingLongBroker = longBrokerStr;
        }
    }
    
    if (shortBroker)
    {
        const char* shortBrokerStr = reinterpret_cast<const char*>(shortBroker);
        size_t shortBrokerLen = strlen(shortBrokerStr);
        if (shortBrokerLen > 0 && shortBrokerLen <= 256)
        {
            g_pendingShortBroker = shortBrokerStr;
        }
    }
    
    char debugMsg[512];
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
        "[SetCommandParams] SUCCESS: code='%s' longBroker='%s' shortBroker='%s'",
        g_pendingCommandCode.c_str(), g_pendingLongBroker.c_str(), g_pendingShortBroker.c_str());
    AppendLogLine(debugMsg);
    
    return true;
}

// Set ack parameters (avoids MQL4 string passing issues)
// Uses unsigned char* to receive MQL4 uchar arrays (official MQL4 method)
bool SetAckParams(unsigned char* brokerId, unsigned char* ackCode)
{
    // Clear previous values
    g_pendingAckBrokerId.clear();
    g_pendingAckCode.clear();
    
    if (brokerId)
    {
        const char* brokerIdStr = reinterpret_cast<const char*>(brokerId);
        size_t brokerIdLen = strlen(brokerIdStr);
        if (brokerIdLen > 0 && brokerIdLen <= 256)
        {
            g_pendingAckBrokerId = brokerIdStr;
        }
    }
    
    if (ackCode)
    {
        const char* ackCodeStr = reinterpret_cast<const char*>(ackCode);
        size_t ackCodeLen = strlen(ackCodeStr);
        if (ackCodeLen > 0 && ackCodeLen <= 256)
        {
            g_pendingAckCode = ackCodeStr;
        }
    }
    
    char debugMsg[512];
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
        "[SetAckParams] SUCCESS: brokerId='%s' ackCode='%s'",
        g_pendingAckBrokerId.c_str(), g_pendingAckCode.c_str());
    AppendLogLine(debugMsg);
    
    return true;
}

// Init shared memory
bool InitSharedMemory(const char* symbol)
{
    // CRITICAL: Log function entry immediately to verify DLL is being called
    char entryMsg[512];
    if (symbol)
    {
        _snprintf_s(entryMsg, sizeof(entryMsg), _TRUNCATE,
            "[InitSharedMemory] FUNCTION CALLED: symbol parameter='%s' (strlen=%zu, first_char='%c'=0x%02X)",
            symbol, strlen(symbol), symbol[0], (unsigned char)symbol[0]);
    }
    else
    {
        _snprintf_s(entryMsg, sizeof(entryMsg), _TRUNCATE,
            "[InitSharedMemory] FUNCTION CALLED: symbol parameter is NULL");
    }
    AppendLogLine(entryMsg);
    
    // Close previous mapping if any
    if (g_hMapFile != NULL)
    {
        AppendLogLine("[InitSharedMemory] Closing previous shared memory mapping...");
        if (g_pSharedData)
        {
            UnmapViewOfFile(g_pSharedData);
            g_pSharedData = NULL;
        }
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }

    // CRITICAL: If symbol is empty or only 1 character, use g_pendingSymbol from SetSymbolForInit
    if (!symbol || strlen(symbol) == 0 || (strlen(symbol) == 1 && symbol[0] != '\0'))
    {
        if (!g_pendingSymbol.empty())
        {
            g_symbolName = g_pendingSymbol;
            char debugMsg[256];
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[InitSharedMemory] Using symbol from SetSymbolForInit: '%s' (len=%zu)",
                g_symbolName.c_str(), g_symbolName.length());
            AppendLogLine(debugMsg);
        }
        else
        {
            // No SetSymbolForInit was called, try to extract from symbol parameter
            if (symbol && strlen(symbol) == 1)
            {
                AppendLogLine("[InitSharedMemory] WARNING: Only got 1 character and no SetSymbolForInit was called. Trying to extract from structure...");
                const int* structPtr = reinterpret_cast<const int*>(symbol);
                int mqlLen = structPtr[0];
                int mqlReserved = structPtr[1];
                
                if (mqlLen > 1 && mqlLen <= 256 && (mqlReserved == 0 || mqlReserved < 1000))
                {
                    const char** strPtrPtr = reinterpret_cast<const char**>(const_cast<char*>(symbol) + 8);
                    const char* actualStr = *strPtrPtr;
                    
                    if (actualStr != NULL && actualStr != symbol)
                    {
                        g_symbolName.assign(actualStr, mqlLen);
                        char debugMsg[256];
                        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                            "[InitSharedMemory] Extracted from structure: len=%d symbol='%s'",
                            mqlLen, g_symbolName.c_str());
                        AppendLogLine(debugMsg);
                    }
                    else
                    {
                        AppendLogLine("[InitSharedMemory] ERROR: Only got 1 character and cannot extract from structure. Please call SetSymbolForInit() first.");
                        return false;
                    }
                }
                else
                {
                    AppendLogLine("[InitSharedMemory] ERROR: Only got 1 character and no SetSymbolForInit was called. Please call SetSymbolForInit() first.");
                    return false;
                }
            }
            else
            {
                AppendLogLine("[InitSharedMemory] ERROR: symbol is empty and no SetSymbolForInit was called.");
                return false;
            }
        }
    }
    else
    {
        // Symbol parameter has length > 1, try to extract normally
        // CRITICAL: MQL4 passes string to DLL
        // MQL4 string structure: { int len; int reserved; char* str; } (12 bytes on 32-bit)
        // When MQL4 calls DLL with "string symbol", it may pass:
        //   1. A pointer to the structure (most common)
        //   2. A direct pointer to string data (less common, but possible)
        
        // Try to interpret as MQL4 string structure first
        const int* structPtr = reinterpret_cast<const int*>(symbol);
        int mqlLen = structPtr[0];
        int mqlReserved = structPtr[1];
        
        // Also check strlen to see what we actually got
        size_t directStrLen = strlen(symbol);
        
        char structCheckMsg[512];
        _snprintf_s(structCheckMsg, sizeof(structCheckMsg), _TRUNCATE,
            "[InitSharedMemory] Parameter analysis: structPtr[0]=%d (as len), structPtr[1]=%d (as reserved), strlen(symbol)=%zu, symbol='%s', symbol pointer=0x%p",
            mqlLen, mqlReserved, directStrLen, symbol, symbol);
        AppendLogLine(structCheckMsg);
        
        // Check if this looks like a valid MQL4 string structure
        // Valid structure: len > 0, len <= 256, reserved is 0 or small value
        // BUT: if strlen is 1 and structPtr[0] is a large number, it's NOT a structure
        bool looksLikeStructure = (mqlLen > 0 && mqlLen <= 256 && (mqlReserved == 0 || mqlReserved < 1000));
        bool looksLikeDirectString = (directStrLen > 0 && directStrLen <= 256 && (mqlLen > 1000 || mqlReserved > 1000));
        
        if (looksLikeStructure && !looksLikeDirectString)
        {
            // This is likely a MQL4 string structure
            // Extract the string pointer from offset 8
            const char** strPtrPtr = reinterpret_cast<const char**>(const_cast<char*>(symbol) + 8);
            const char* actualStr = *strPtrPtr;
            
            char ptrMsg[512];
            _snprintf_s(ptrMsg, sizeof(ptrMsg), _TRUNCATE,
                "[InitSharedMemory] MQL4 structure detected: len=%d, strPtrPtr=0x%p, actualStr pointer=0x%p",
                mqlLen, strPtrPtr, actualStr);
            AppendLogLine(ptrMsg);
            
            if (actualStr != NULL && actualStr != symbol)
            {
                // Verify the string is valid (not empty, not too long)
                size_t actualLen = strlen(actualStr);
                if (actualLen > 0 && actualLen <= 256 && actualLen == mqlLen)
                {
                    // Use the string from structure
                    g_symbolName.assign(actualStr, mqlLen);
                    
                    char debugMsg[256];
                    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                        "[InitSharedMemory] SUCCESS: Extracted symbol from MQL4 structure: len=%d symbol='%s'",
                        mqlLen, g_symbolName.c_str());
                    AppendLogLine(debugMsg);
                }
                else
                {
                    // String length mismatch, try using actualLen
                    if (actualLen > 0 && actualLen <= 256)
                    {
                        g_symbolName.assign(actualStr, actualLen);
                        char debugMsg[256];
                        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                            "[InitSharedMemory] SUCCESS: Extracted symbol (len mismatch: mqlLen=%d, actualLen=%zu): symbol='%s'",
                            mqlLen, actualLen, g_symbolName.c_str());
                        AppendLogLine(debugMsg);
                    }
                    else
                    {
                        // Invalid string, try direct access at offset 8
                        const char* directStr = reinterpret_cast<const char*>(symbol + 8);
                        if (directStr != NULL && directStr[0] != '\0')
                        {
                            g_symbolName.assign(directStr, mqlLen);
                            char debugMsg[256];
                            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                                "[InitSharedMemory] SUCCESS: Extracted symbol from direct offset: len=%d symbol='%s'",
                                mqlLen, g_symbolName.c_str());
                            AppendLogLine(debugMsg);
                        }
                        else
                        {
                            AppendLogLine("[InitSharedMemory] ERROR: Cannot extract valid string from MQL4 structure.");
                            return false;
                        }
                    }
                }
            }
            else
            {
                // String pointer is invalid, try direct access at offset 8
                const char* directStr = reinterpret_cast<const char*>(symbol + 8);
                if (directStr != NULL && directStr[0] != '\0')
                {
                    g_symbolName.assign(directStr, mqlLen);
                    char debugMsg[256];
                    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                        "[InitSharedMemory] SUCCESS: Extracted symbol from direct offset (pointer invalid): len=%d symbol='%s'",
                        mqlLen, g_symbolName.c_str());
                    AppendLogLine(debugMsg);
                }
                else
                {
                    AppendLogLine("[InitSharedMemory] ERROR: Cannot extract valid string from MQL4 structure (pointer and direct access both failed).");
                    return false;
                }
            }
        }
    else if (looksLikeDirectString)
    {
        // This looks like a direct string pointer (not a structure)
        // MQL4 may have passed the string data directly, but only the first character
        // This is a known MQL4 bug/limitation when passing strings to DLL
        
        if (directStrLen == 1)
        {
            // CRITICAL: Only got 1 character - this is the MQL4 string passing bug
            // MQL4 is only passing the first character of the string
            // We cannot recover the full string from here
            // The EA must use a different approach (e.g., SetSymbolForInit pattern)
            
            AppendLogLine("[InitSharedMemory] ERROR: MQL4 string passing bug detected - only received 1 character!");
            AppendLogLine("[InitSharedMemory] This is a known MQL4 limitation. EA should use SetSymbolForInit() before InitSharedMemory().");
            char charMsg[256];
            _snprintf_s(charMsg, sizeof(charMsg), _TRUNCATE,
                "[InitSharedMemory] Received character: '%c' (0x%02X)", symbol[0], (unsigned char)symbol[0]);
            AppendLogLine(charMsg);
            
            // Try to read from offset -8 as if symbol is the str pointer in a structure
            const char* possibleStruct = reinterpret_cast<const char*>(symbol) - 8;
            if (IsReadableMemory(possibleStruct, 12))
            {
                const int* backStructPtr = reinterpret_cast<const int*>(possibleStruct);
                int backLen = backStructPtr[0];
                int backReserved = backStructPtr[1];
                
                if (backLen > 1 && backLen <= 256 && (backReserved == 0 || backReserved < 1000))
                {
                    const char** backStrPtrPtr = reinterpret_cast<const char**>(const_cast<char*>(possibleStruct) + 8);
                    const char* backActualStr = *backStrPtrPtr;
                    
                    if (backActualStr == symbol)
                    {
                        // Found it! symbol is the str pointer, structure is at symbol-8
                        // Read the full string using the length from structure
                        char tempBuf[256];
                        size_t copyLen = (backLen < 255) ? backLen : 255;
                        memcpy(tempBuf, symbol, copyLen);
                        tempBuf[copyLen] = '\0';
                        g_symbolName = tempBuf;
                        
                        char debugMsg[256];
                        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                            "[InitSharedMemory] SUCCESS: Found structure at offset -8, extracted: len=%d symbol='%s'",
                            backLen, g_symbolName.c_str());
                        AppendLogLine(debugMsg);
                    }
                    else
                    {
                        // Fallback: use what we have (will fail, but at least we tried)
                        g_symbolName = symbol;
                        AppendLogLine("[InitSharedMemory] ERROR: Structure found but str pointer mismatch. Only got 1 character.");
                        return false;
                    }
                }
                else
                {
                    // No valid structure found
                    g_symbolName = symbol;
                    AppendLogLine("[InitSharedMemory] ERROR: No valid structure found. Only got 1 character.");
                    return false;
                }
            }
            else
            {
                // Cannot read memory at offset -8
                g_symbolName = symbol;
                AppendLogLine("[InitSharedMemory] ERROR: Cannot access memory at offset -8. Only got 1 character.");
                return false;
            }
        }
        else
        {
            // Direct string with length > 1, use it
            g_symbolName = symbol;
            char normalMsg[256];
            _snprintf_s(normalMsg, sizeof(normalMsg), _TRUNCATE,
                "[InitSharedMemory] Using direct string (not MQL4 structure): '%s' (strlen=%zu)",
                symbol, directStrLen);
            AppendLogLine(normalMsg);
        }
    }
    else
    {
        // Neither structure nor valid direct string
        char errorMsg[256];
        _snprintf_s(errorMsg, sizeof(errorMsg), _TRUNCATE,
            "[InitSharedMemory] ERROR: Cannot determine string format. structPtr[0]=%d, structPtr[1]=%d, strlen=%zu",
            mqlLen, mqlReserved, directStrLen);
        AppendLogLine(errorMsg);
        return false;
    }
    }  // End of else block starting at line 344
    
    // Final validation
    if (g_symbolName.empty() || g_symbolName.length() == 0)
    {
        AppendLogLine("[InitSharedMemory] ERROR: Extracted symbol is empty.");
        return false;
    }
    
    // Log the symbol we're using
    char symbolDebug[256];
    _snprintf_s(symbolDebug, sizeof(symbolDebug), _TRUNCATE,
        "[InitSharedMemory] Using symbol: '%s' (len=%zu)",
        g_symbolName.c_str(), g_symbolName.length());
    AppendLogLine(symbolDebug);
    
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;
    
    char mapNameMsg[256];
    _snprintf_s(mapNameMsg, sizeof(mapNameMsg), _TRUNCATE,
        "[InitSharedMemory] Using mapName: '%s' (prefix='%s', symbol='%s')",
        mapName.c_str(), SHARED_MEMORY_NAME_PREFIX, g_symbolName.c_str());
    AppendLogLine(mapNameMsg);

    // Try open existing
    g_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mapName.c_str());
    bool isNewlyCreated = false;

    if (g_hMapFile == NULL)
    {
        AppendLogLine("[InitSharedMemory] Shared memory does not exist, creating new...");
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
            DWORD err = GetLastError();
            char errMsg[256];
            _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
                "[InitSharedMemory] ERROR: CreateFileMappingA failed. Error code: %lu", err);
            AppendLogLine(errMsg);
            return false;
        }
        DWORD lastError = GetLastError();
        isNewlyCreated = (lastError != ERROR_ALREADY_EXISTS);
        char createMsg[256];
        _snprintf_s(createMsg, sizeof(createMsg), _TRUNCATE,
            "[InitSharedMemory] Shared memory created. isNewlyCreated=%d, GetLastError()=%lu",
            isNewlyCreated ? 1 : 0, lastError);
        AppendLogLine(createMsg);
    }
    else
    {
        AppendLogLine("[InitSharedMemory] Shared memory already exists, opened successfully.");
    }

    AppendLogLine("[InitSharedMemory] Mapping view of file...");
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    );
    if (!g_pSharedData)
    {
        DWORD err = GetLastError();
        char errMsg[256];
        _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
            "[InitSharedMemory] ERROR: MapViewOfFile failed. Error code: %lu", err);
        AppendLogLine(errMsg);
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    AppendLogLine("[InitSharedMemory] MapViewOfFile succeeded.");

    bool needInit = isNewlyCreated;
    if (!needInit)
    {
        if (memcmp(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature)) != 0 ||
            g_pSharedData->structVersion != SHARED_DATA_VERSION)
        {
            // CRITICAL: Only initialize if signature/version mismatch AND we created it
            // If we opened existing memory with wrong signature, log warning but don't clear
            // This prevents one EA from clearing another EA's data
            if (isNewlyCreated)
            {
                needInit = true;
            }
            else
            {
                char warnMsg[256];
                _snprintf_s(warnMsg, sizeof(warnMsg), _TRUNCATE,
                    "[InitSharedMemory] WARNING: Signature/version mismatch but memory already exists. "
                    "Not clearing to preserve existing data. sig='%.8s' ver=%d (expected '%.8s' ver=%d)",
                    g_pSharedData->signature, g_pSharedData->structVersion,
                    SHARED_DATA_SIGNATURE, SHARED_DATA_VERSION);
                AppendLogLine(warnMsg);
                needInit = false;
            }
        }
    }

    if (needInit)
    {
        // CRITICAL: Only clear memory if we just created it (isNewlyCreated == true)
        // This ensures we don't clear data written by another process
        if (isNewlyCreated)
        {
            memset(g_pSharedData, 0, SHARED_MEMORY_SIZE);
            memcpy(g_pSharedData->signature, SHARED_DATA_SIGNATURE, sizeof(g_pSharedData->signature));
            g_pSharedData->structVersion = SHARED_DATA_VERSION;
            g_pSharedData->version = 0;
            g_pSharedData->lockFlag = 0;
            AppendLogLine("[InitSharedMemory] SharedData initialized with signature/version (newly created).");
        }
        else
        {
            AppendLogLine("[InitSharedMemory] WARNING: needInit=true but isNewlyCreated=false. Not clearing existing data.");
        }
    }
    else
    {
        // Log current state of quotes for diagnostic purposes
        bool hasA = g_pSharedData->quotes[0].isValid;
        bool hasB = g_pSharedData->quotes[1].isValid;
        char stateMsg[256];
        _snprintf_s(stateMsg, sizeof(stateMsg), _TRUNCATE,
            "[InitSharedMemory] Using existing shared memory. Quote[0] (A): valid=%d bid=%.5f ask=%.5f, "
            "Quote[1] (B): valid=%d bid=%.5f ask=%.5f, version=%ld",
            hasA ? 1 : 0, g_pSharedData->quotes[0].bid, g_pSharedData->quotes[0].ask,
            hasB ? 1 : 0, g_pSharedData->quotes[1].bid, g_pSharedData->quotes[1].ask,
            g_pSharedData->version);
        AppendLogLine(stateMsg);
        
        // Also log current command state
        char existingCode[32];
        memcpy(existingCode, g_pSharedData->command.code, sizeof(g_pSharedData->command.code));
        existingCode[sizeof(g_pSharedData->command.code)] = '\0';
        char logExisting[256];
        _snprintf_s(logExisting, sizeof(logExisting), _TRUNCATE,
            "[InitSharedMemory] Current command.code='%s' (len=%zu)",
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

    AppendLogLine("[InitSharedMemory] SUCCESS: Shared memory initialized and ready.");
    return true;
}

// Set quote
bool SetQuote(const char* brokerId, double bid, double ask)
{
    if (!g_pSharedData || !brokerId) 
    {
        AppendLogLine("[SetQuote] ERROR: g_pSharedData is NULL or brokerId is NULL");
        return false;
    }
    if (bid <= 0 || ask <= 0 || ask <= bid) 
    {
        char errMsg[256];
        _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
            "[SetQuote] ERROR: Invalid price values - bid=%.5f ask=%.5f", bid, ask);
        AppendLogLine(errMsg);
        return false;
    }

    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;

    unsigned long long ts = GetCurrentTimestamp();
    
    // Write data
    g_pSharedData->quotes[index].bid = bid;
    g_pSharedData->quotes[index].ask = ask;
    g_pSharedData->quotes[index].timestamp = ts;
    
    // CRITICAL: Set isValid LAST, after all data is written
    // Use memory barrier to ensure writes are visible before isValid is set
    _ReadWriteBarrier();
    g_pSharedData->quotes[index].isValid = true;
    
    // Increment version counter (provides additional memory barrier)
    InterlockedIncrement(&g_pSharedData->version);
    
    // CRITICAL: Flush memory view to ensure writes are visible to other processes
    // This is essential for cross-process memory visibility
    if (g_hMapFile != NULL)
    {
        FlushViewOfFile(g_pSharedData, 0);  // 0 = flush entire view
    }

    // Enhanced diagnostic logging (every 100 writes to avoid spam)
    static int writeCount[2] = {0, 0};
    writeCount[index]++;
    
    // Read both quotes for diagnostic purposes
    bool isValidA = g_pSharedData->quotes[0].isValid;
    bool isValidB = g_pSharedData->quotes[1].isValid;
    double bidA = g_pSharedData->quotes[0].bid;
    double askA = g_pSharedData->quotes[0].ask;
    double bidB = g_pSharedData->quotes[1].bid;
    double askB = g_pSharedData->quotes[1].ask;
    
    if (writeCount[index] % 100 == 0 || writeCount[index] == 1)
    {
        char buf[512];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "[SetQuote] sym=%s broker=%s idx=%d bid=%.5f ask=%.5f ts=%llu isValid=true memPtr=%p version=%ld (write count: %d)\n"
            "  [DIAG] Quote[0] (A): valid=%d bid=%.5f ask=%.5f\n"
            "  [DIAG] Quote[1] (B): valid=%d bid=%.5f ask=%.5f",
            g_symbolName.c_str(), brokerId, index, bid, ask, ts, g_pSharedData, g_pSharedData->version, writeCount[index],
            isValidA ? 1 : 0, bidA, askA,
            isValidB ? 1 : 0, bidB, askB);
        AppendLogLine(buf);
    }

    return true;
}

// Get quote
bool GetQuote(const char* brokerId, double* bid, double* ask)
{
    if (!g_pSharedData || !brokerId || !bid || !ask) 
    {
        char errMsg[256];
        _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
            "[GetQuote] ERROR: Invalid parameters - g_pSharedData=%p brokerId=%p bid=%p ask=%p",
            g_pSharedData, brokerId, bid, ask);
        AppendLogLine(errMsg);
        return false;
    }
    int index = (brokerId[0] == 'A' || brokerId[0] == 'a') ? 0 : 1;
    
    // Read data with memory barrier to ensure we see latest values
    _ReadWriteBarrier();
    
    // CRITICAL: Read both quotes to diagnose if we're accessing the same shared memory
    // This helps verify that A and B are using the same memory region
    bool isValidA = g_pSharedData->quotes[0].isValid;
    bool isValidB = g_pSharedData->quotes[1].isValid;
    double bidA = g_pSharedData->quotes[0].bid;
    double askA = g_pSharedData->quotes[0].ask;
    double bidB = g_pSharedData->quotes[1].bid;
    double askB = g_pSharedData->quotes[1].ask;
    unsigned long long tsA = g_pSharedData->quotes[0].timestamp;
    unsigned long long tsB = g_pSharedData->quotes[1].timestamp;
    
    // Check if quote is valid
    bool isValid = g_pSharedData->quotes[index].isValid;
    double readBid = g_pSharedData->quotes[index].bid;
    double readAsk = g_pSharedData->quotes[index].ask;
    unsigned long long ts = g_pSharedData->quotes[index].timestamp;
    
    // Enhanced diagnostic logging (every 50 failed reads or every 200 successful reads)
    static int readCount[2] = {0, 0};
    static int failCount[2] = {0, 0};
    readCount[index]++;
    
    if (!isValid)
    {
        failCount[index]++;
        unsigned long long current = GetCurrentTimestamp();
        unsigned long long age = (ts > 0) ? (current - ts) : 0;
        
        // Log detailed diagnostic info every 10 failures or first failure
        if (failCount[index] == 1 || failCount[index] % 10 == 0)
        {
            char errMsg[512];
            _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
                "[GetQuote] Quote[%d] for broker '%s' is invalid - bid=%.5f ask=%.5f ts=%llu age=%llu ms sym=%s memPtr=%p\n"
                "  [DIAG] Quote[0] (A): valid=%d bid=%.5f ask=%.5f ts=%llu\n"
                "  [DIAG] Quote[1] (B): valid=%d bid=%.5f ask=%.5f ts=%llu\n"
                "  [DIAG] Shared memory version=%ld",
                index, brokerId, readBid, readAsk, ts, age, g_symbolName.c_str(), g_pSharedData,
                isValidA ? 1 : 0, bidA, askA, tsA,
                isValidB ? 1 : 0, bidB, askB, tsB,
                g_pSharedData->version);
            AppendLogLine(errMsg);
        }
        return false;
    }
    
    // Validate price values
    if (readBid <= 0 || readAsk <= 0 || readAsk <= readBid)
    {
        failCount[index]++;
        char errMsg[512];
        _snprintf_s(errMsg, sizeof(errMsg), _TRUNCATE,
            "[GetQuote] Quote[%d] for broker '%s' has invalid prices - bid=%.5f ask=%.5f sym=%s memPtr=%p\n"
            "  [DIAG] Quote[0] (A): valid=%d bid=%.5f ask=%.5f\n"
            "  [DIAG] Quote[1] (B): valid=%d bid=%.5f ask=%.5f",
            index, brokerId, readBid, readAsk, g_symbolName.c_str(), g_pSharedData,
            isValidA ? 1 : 0, bidA, askA,
            isValidB ? 1 : 0, bidB, askB);
        AppendLogLine(errMsg);
        return false;
    }
    
    *bid = readBid;
    *ask = readAsk;
    
    // Log successful read (every 200 calls to avoid spam)
    if (readCount[index] % 200 == 0)
    {
        char logMsg[512];
        _snprintf_s(logMsg, sizeof(logMsg), _TRUNCATE,
            "[GetQuote] Quote[%d] for broker '%s' - bid=%.5f ask=%.5f ts=%llu sym=%s memPtr=%p (read count: %d)\n"
            "  [DIAG] Quote[0] (A): valid=%d bid=%.5f ask=%.5f\n"
            "  [DIAG] Quote[1] (B): valid=%d bid=%.5f ask=%.5f",
            index, brokerId, *bid, *ask, ts, g_symbolName.c_str(), g_pSharedData, readCount[index],
            isValidA ? 1 : 0, bidA, askA,
            isValidB ? 1 : 0, bidB, askB);
        AppendLogLine(logMsg);
    }
    
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

// Helper function to extract MQL4 string
// MQL4 may pass only the first character when passing strings to DLL
// This function attempts to detect and handle this case
static std::string ExtractMql4String(const char* str, const char* paramName = "")
{
    if (!str) return "";
    
    size_t directLen = strlen(str);
    
    // Log the received parameter for debugging
    if (directLen == 1 && paramName && strlen(paramName) > 0)
    {
        char debugMsg[256];
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[ExtractMql4String] %s: Only received 1 character '%c' (0x%02X) - MQL4 string passing issue detected",
            paramName, str[0], (unsigned char)str[0]);
        AppendLogLine(debugMsg);
    }
    
    // If we only got 1 character, it's likely the MQL4 string passing bug
    // Unfortunately, we cannot recover the full string from DLL side
    // The EA should use uchar array passing (like SetSymbolForInit) for reliable string passing
    // For now, we'll return what we got and log a warning
    if (directLen == 1)
    {
        char warnMsg[256];
        _snprintf_s(warnMsg, sizeof(warnMsg), _TRUNCATE,
            "[ExtractMql4String] WARNING: %s only has 1 character '%c'. MQL4 string passing limitation. Full string may be lost.",
            paramName ? paramName : "parameter", str[0]);
        AppendLogLine(warnMsg);
    }
    
    // Return the string as-is (may be truncated if MQL4 only passed first character)
    return std::string(str);
}

// Set command
// If parameters are provided, use them directly (for GUI/C++ calls)
// If parameters are NULL or empty, use global variables from SetCommandParams (for MQL4 calls)
bool SetCommand(const char* code, const char* longBroker, const char* shortBroker)
{
    if (!g_pSharedData) return false;

    memset(&g_pSharedData->command, 0, sizeof(g_pSharedData->command));

    // Determine which values to use: parameters or global variables
    std::string codeStr;
    std::string longBrokerStr;
    std::string shortBrokerStr;
    
    // If code parameter is NULL or empty, use global variables (from SetCommandParams)
    if (!code || strlen(code) == 0)
    {
        if (!g_pendingCommandCode.empty())
        {
            codeStr = g_pendingCommandCode;
            longBrokerStr = g_pendingLongBroker;
            shortBrokerStr = g_pendingShortBroker;
            
            char debugMsg[256];
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[SetCommand] Using parameters from SetCommandParams: code='%s' long='%s' short='%s'",
                codeStr.c_str(), longBrokerStr.c_str(), shortBrokerStr.c_str());
            AppendLogLine(debugMsg);
        }
        else
        {
            AppendLogLine("[SetCommand] ERROR: No command code provided and no SetCommandParams was called.");
            return false;
        }
    }
    else
    {
        // Use parameters directly (for GUI/C++ calls)
        codeStr = code;
        longBrokerStr = (longBroker && strlen(longBroker) > 0) ? longBroker : "";
        shortBrokerStr = (shortBroker && strlen(shortBroker) > 0) ? shortBroker : "";
    }
    
    if (codeStr.empty())
    {
        AppendLogLine("[SetCommand] ERROR: No command code provided.");
        return false;
    }

    // CRITICAL: Ensure strings fit in their buffers and are null-terminated
    // code field is 16 bytes, longBroker/shortBroker are 4 bytes each
    
    // Copy code (max 15 chars + null terminator = 16 bytes)
    size_t codeLen = codeStr.length();
    if (codeLen >= sizeof(g_pSharedData->command.code))
        codeLen = sizeof(g_pSharedData->command.code) - 1;
    memcpy(g_pSharedData->command.code, codeStr.c_str(), codeLen);
    g_pSharedData->command.code[codeLen] = '\0';  // Ensure null termination
    
    // Copy longBroker (max 3 chars + null terminator = 4 bytes)
    if (!longBrokerStr.empty())
    {
        size_t longLen = longBrokerStr.length();
        if (longLen >= sizeof(g_pSharedData->command.longBroker))
            longLen = sizeof(g_pSharedData->command.longBroker) - 1;
        memcpy(g_pSharedData->command.longBroker, longBrokerStr.c_str(), longLen);
        g_pSharedData->command.longBroker[longLen] = '\0';  // Ensure null termination
    }
    else
    {
        g_pSharedData->command.longBroker[0] = '\0';  // Clear if empty
    }
    
    // Copy shortBroker (max 3 chars + null terminator = 4 bytes)
    if (!shortBrokerStr.empty())
    {
        size_t shortLen = shortBrokerStr.length();
        if (shortLen >= sizeof(g_pSharedData->command.shortBroker))
            shortLen = sizeof(g_pSharedData->command.shortBroker) - 1;
        memcpy(g_pSharedData->command.shortBroker, shortBrokerStr.c_str(), shortLen);
        g_pSharedData->command.shortBroker[shortLen] = '\0';  // Ensure null termination
    }
    else
    {
        g_pSharedData->command.shortBroker[0] = '\0';  // Clear if empty
    }

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

    // MQL4 strings: When passed as output parameters (string &code), MQL4 provides a writable buffer
    // The buffer is typically 256 bytes. We need to copy the string correctly.
    // CRITICAL: MQL4 string reference parameters are passed as char* pointing to the actual buffer
    // We should directly write to this buffer, ensuring null termination
    
    if (code)
    {
        // Find source string length (code field is 16 bytes)
        size_t srcLen = 0;
        size_t maxSrcLen = sizeof(g_pSharedData->command.code) - 1;  // Reserve 1 byte for null terminator
        while (srcLen < maxSrcLen && 
               g_pSharedData->command.code[srcLen] != '\0')
            srcLen++;
        
        if (srcLen > 0)
        {
            // CRITICAL: MQL4 string buffers are 256 bytes, but we should only copy valid data
            // Copy string directly to MQL4 buffer, ensuring null termination
            size_t maxCopy = (srcLen < 255) ? srcLen : 255;
            memcpy(code, g_pSharedData->command.code, maxCopy);
            code[maxCopy] = '\0';  // CRITICAL: Ensure null termination
            
            // Verify copy by reading back (first 31 chars for safety)
            char verifyBuf[32];
            size_t verifyLen = (maxCopy < 31) ? maxCopy : 31;
            memcpy(verifyBuf, code, verifyLen);
            verifyBuf[verifyLen] = '\0';
            
            char copyMsg[256];
            _snprintf_s(copyMsg, sizeof(copyMsg), _TRUNCATE,
                "[GetCommand] code: srcLen=%zu maxCopy=%zu src=\"%.16s\" dst=\"%s\" verified=\"%s\"", 
                srcLen, maxCopy, g_pSharedData->command.code, code, verifyBuf);
            AppendLogLine(copyMsg);
        }
        else
        {
            // Source is empty, clear output
            code[0] = '\0';
            AppendLogLine("[GetCommand] Source code is empty, cleared output");
        }
    }
    if (longBroker)
    {
        // CRITICAL: longBroker field is only 4 bytes, must be careful with copying
        // Find actual string length (up to 3 chars + null terminator)
        size_t len = 0;
        size_t maxLen = sizeof(g_pSharedData->command.longBroker) - 1;  // Reserve 1 byte for null terminator
        while (len < maxLen && 
               g_pSharedData->command.longBroker[len] != '\0')
            len++;
        
        // CRITICAL: Always ensure null termination, even if source doesn't have it
        // MQL4 string buffers are 256 bytes, but we should only copy what's valid
        if (len > 0)
        {
            // Copy up to len characters, then add null terminator
            memcpy(longBroker, g_pSharedData->command.longBroker, len);
            longBroker[len] = '\0';  // Ensure null termination
        }
        else
        {
            // Source is empty or invalid, clear output
            longBroker[0] = '\0';
        }
        
        // Diagnostic: log what we're copying
        char diagMsg[128];
        _snprintf_s(diagMsg, sizeof(diagMsg), _TRUNCATE,
            "[GetCommand] longBroker: srcLen=%zu src='%.4s' dst='%s'",
            len, g_pSharedData->command.longBroker, longBroker);
        AppendLogLine(diagMsg);
    }
    if (shortBroker)
    {
        // CRITICAL: shortBroker field is only 4 bytes, must be careful with copying
        size_t len = 0;
        size_t maxLen = sizeof(g_pSharedData->command.shortBroker) - 1;  // Reserve 1 byte for null terminator
        while (len < maxLen && 
               g_pSharedData->command.shortBroker[len] != '\0')
            len++;
        
        // CRITICAL: Always ensure null termination
        if (len > 0)
        {
            memcpy(shortBroker, g_pSharedData->command.shortBroker, len);
            shortBroker[len] = '\0';  // Ensure null termination
        }
        else
        {
            // Source is empty or invalid, clear output
            shortBroker[0] = '\0';
        }
        
        // Diagnostic: log what we're copying
        char diagMsg[128];
        _snprintf_s(diagMsg, sizeof(diagMsg), _TRUNCATE,
            "[GetCommand] shortBroker: srcLen=%zu src='%.4s' dst='%s'",
            len, g_pSharedData->command.shortBroker, shortBroker);
        AppendLogLine(diagMsg);
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
    if (!g_pSharedData) return false;
    
    // Determine which values to use: parameters or global variables
    std::string brokerIdStr;
    std::string ackCodeStr;
    
    // If brokerId parameter is NULL or empty, use global variables (from SetAckParams)
    if (!brokerId || strlen(brokerId) == 0)
    {
        if (!g_pendingAckBrokerId.empty())
        {
            brokerIdStr = g_pendingAckBrokerId;
            ackCodeStr = g_pendingAckCode;
            
            char debugMsg[256];
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[SetAck] Using parameters from SetAckParams: brokerId='%s' ackCode='%s'",
                brokerIdStr.c_str(), ackCodeStr.c_str());
            AppendLogLine(debugMsg);
        }
        else
        {
            AppendLogLine("[SetAck] ERROR: No brokerId provided and no SetAckParams was called.");
            return false;
        }
    }
    else
    {
        // Use parameters directly (for GUI/C++ calls)
        brokerIdStr = brokerId;
        ackCodeStr = (ackCode && strlen(ackCode) > 0) ? ackCode : "";
    }
    
    if (brokerIdStr.empty() || ackCodeStr.empty())
    {
        AppendLogLine("[SetAck] ERROR: brokerId or ackCode is empty.");
        return false;
    }
    
    int index = (brokerIdStr[0] == 'A' || brokerIdStr[0] == 'a') ? 0 : 1;

    strncpy_s(g_pSharedData->acks[index].code,
        sizeof(g_pSharedData->acks[index].code),
        ackCodeStr.c_str(),
        _TRUNCATE);
    g_pSharedData->acks[index].timestamp = GetCurrentTimestamp();
    g_pSharedData->acks[index].isValid = true;
    InterlockedIncrement(&g_pSharedData->version);
    
    char buf[256];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
        "[SetAck] sym=%s brokerId='%s' ackCode='%s' index=%d",
        g_symbolName.c_str(), brokerIdStr.c_str(), ackCodeStr.c_str(), index);
    AppendLogLine(buf);
    
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