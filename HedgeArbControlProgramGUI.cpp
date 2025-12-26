// HedgeArbEA Control Program GUI
// C++ Win32 GUI for monitoring shared memory data (bid, ask, spread)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "../HedgeArbSharedMemory_New/HedgeArbSharedMemory.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

// Helper functions for ListView operations (ANSI version)
static int ListView_InsertItemA_Helper(HWND hwnd, LVITEMA* pitem)
{
    return (int)SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)pitem);
}

static BOOL ListView_SetItemTextA_Helper(HWND hwnd, int i, int iSubItem, const char* pszText)
{
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;
    lvi.pszText = const_cast<LPSTR>(pszText);  // Remove const for ListView API
    return (BOOL)SendMessageA(hwnd, LVM_SETITEMTEXTA, (WPARAM)i, (LPARAM)&lvi);
}

static int ListView_InsertColumnA_Helper(HWND hwnd, int iCol, LVCOLUMNA* pcol)
{
    return (int)SendMessageA(hwnd, LVM_INSERTCOLUMNA, (WPARAM)iCol, (LPARAM)pcol);
}

// Define macros to use helper functions (inline to avoid multiple definition)
inline int ListView_InsertItemA(HWND hwnd, LVITEMA* pitem) { return ListView_InsertItemA_Helper(hwnd, pitem); }
inline BOOL ListView_SetItemTextA(HWND hwnd, int i, int iSubItem, const char* pszText) { return ListView_SetItemTextA_Helper(hwnd, i, iSubItem, pszText); }
inline int ListView_InsertColumnA(HWND hwnd, int iCol, LVCOLUMNA* pcol) { return ListView_InsertColumnA_Helper(hwnd, iCol, pcol); }

// Helper functions for ListView operations that might conflict with macros
static int ListView_GetItemCountA(HWND hwnd) { return (int)SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0); }
static int ListView_DeleteAllItemsA(HWND hwnd) { return (int)SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0); }

// Window handles
HWND g_hWnd = NULL;
HWND g_hStatusBar = NULL;
HWND g_hListQuotes = NULL;
HWND g_hListPositions = NULL;
HWND g_hListHistory = NULL;

// Shared memory
SharedData* g_pSharedData = NULL;
HANDLE g_hMapFile = NULL;
std::string g_symbolName = "";  // Will be extracted from shared memory name

// Constants
const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";
const int TIMER_ID = 1;
const int TIMER_INTERVAL_MS = 500;  // Update every 500ms
const unsigned long long DATA_EXPIRY_MS = 5000;  // Data expires after 5 seconds

// MT4 Common Files path
std::string GetMT4CommonFilesPath()
{
    char appDataPath[MAX_PATH];
    if (SHGetSpecialFolderPathA(NULL, appDataPath, CSIDL_APPDATA, FALSE))
    {
        char commonPath[MAX_PATH];
        _snprintf_s(commonPath, sizeof(commonPath), _TRUNCATE,
            "%s\\MetaQuotes\\Terminal\\Common\\Files", appDataPath);
        return std::string(commonPath);
    }
    return std::string("C:\\MT4\\Common\\Files");  // Fallback
}

// Forward declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool InitSharedMemory();
void UpdateQuoteDisplay();
void UpdateStatusBar();
void CleanupSharedMemory();

// Get current timestamp (milliseconds)
unsigned long long GetCurrentTimestamp()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000;
}

// Check if data is expired (older than 5 seconds)
bool IsDataExpired(unsigned long long timestamp)
{
    unsigned long long current = GetCurrentTimestamp();
    return (current - timestamp) > 5000;
}

// Initialize shared memory connection
// Try to find shared memory by scanning common symbols or use provided symbol
bool InitSharedMemory()
{
    std::string mapName;
    
    // If symbol is not set, try common symbols (GOLD, EURUSD, etc.)
    if (g_symbolName.empty())
    {
        const char* commonSymbols[] = {"GOLD", "EURUSD", "GBPUSD", "USDJPY", "XAUUSD", NULL};
        for (int i = 0; commonSymbols[i] != NULL; i++)
        {
            mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + commonSymbols[i];
            HANDLE hTest = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mapName.c_str());
            if (hTest != NULL)
            {
                CloseHandle(hTest);
                g_symbolName = commonSymbols[i];
                break;
            }
        }
        
        // If still not found, use default
        if (g_symbolName.empty())
        {
            g_symbolName = "GOLD";
        }
    }
    
    mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;
    
    char debugMsg[512];
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
        "[GUI] InitSharedMemory: Attempting to connect to '%s'", mapName.c_str());
    OutputDebugStringA(debugMsg);
    
    // Try to open existing shared memory
    g_hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        mapName.c_str()
    );
    
    if (g_hMapFile == NULL)
    {
        DWORD error = GetLastError();
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[GUI] InitSharedMemory: Failed to open shared memory '%s'. Error: %lu", 
            mapName.c_str(), error);
        OutputDebugStringA(debugMsg);
        // Shared memory doesn't exist yet (EAs haven't started)
        return false;
    }
    
    // Map view of file
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    );
    
    if (g_pSharedData == NULL)
    {
        DWORD error = GetLastError();
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[GUI] InitSharedMemory: Failed to map view of file. Error: %lu", error);
        OutputDebugStringA(debugMsg);
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    
    // Verify signature
    if (memcmp(g_pSharedData->signature, "HEDGEV1", 8) != 0)
    {
        _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
            "[GUI] InitSharedMemory: Invalid signature. Expected 'HEDGEV1', got '%.8s'",
            g_pSharedData->signature);
        OutputDebugStringA(debugMsg);
        UnmapViewOfFile(g_pSharedData);
        CloseHandle(g_hMapFile);
        g_pSharedData = NULL;
        g_hMapFile = NULL;
        return false;
    }
    
    _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
        "[GUI] InitSharedMemory: Successfully connected to '%s'. Version: %ld",
        mapName.c_str(), g_pSharedData->version);
    OutputDebugStringA(debugMsg);
    
    // Extract symbol from map name (HedgeArbEA_SharedMemory_GOLD -> GOLD)
    size_t prefixLen = strlen(SHARED_MEMORY_NAME_PREFIX);
    if (mapName.length() > prefixLen)
    {
        g_symbolName = mapName.substr(prefixLen);
        char symbolMsg[256];
        _snprintf_s(symbolMsg, sizeof(symbolMsg), _TRUNCATE,
            "[GUI] Extracted symbol from shared memory name: '%s'", g_symbolName.c_str());
        OutputDebugStringA(symbolMsg);
    }
    
    return true;
}

// Update quote display in ListView
void UpdateQuoteDisplay()
{
    if (!g_pSharedData || !g_hListQuotes)
        return;
    
    // Clear existing items
    ListView_DeleteAllItemsA(g_hListQuotes);
    
    // Read quotes from shared memory
    double bidA = 0.0, askA = 0.0;
    double bidB = 0.0, askB = 0.0;
    bool validA = false, validB = false;
    
    // Read Broker A quote
    if (g_pSharedData->quotes[0].isValid && !IsDataExpired(g_pSharedData->quotes[0].timestamp))
    {
        bidA = g_pSharedData->quotes[0].bid;
        askA = g_pSharedData->quotes[0].ask;
        validA = true;
    }
    else
    {
        // Debug: Log why A data is not valid
        char debugMsg[256];
        if (!g_pSharedData->quotes[0].isValid)
        {
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[GUI] Broker A quote is invalid (isValid=false)");
        }
        else
        {
            unsigned long long ts = g_pSharedData->quotes[0].timestamp;
            unsigned long long current = GetCurrentTimestamp();
            unsigned long long age = current - ts;
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[GUI] Broker A quote expired (age=%llu ms, limit=%llu ms)",
                age, DATA_EXPIRY_MS);
        }
        OutputDebugStringA(debugMsg);
    }
    
    // Read Broker B quote
    if (g_pSharedData->quotes[1].isValid && !IsDataExpired(g_pSharedData->quotes[1].timestamp))
    {
        bidB = g_pSharedData->quotes[1].bid;
        askB = g_pSharedData->quotes[1].ask;
        validB = true;
    }
    else
    {
        // Debug: Log why B data is not valid
        char debugMsg[256];
        if (!g_pSharedData->quotes[1].isValid)
        {
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[GUI] Broker B quote is invalid (isValid=false)");
        }
        else
        {
            unsigned long long ts = g_pSharedData->quotes[1].timestamp;
            unsigned long long current = GetCurrentTimestamp();
            unsigned long long age = current - ts;
            _snprintf_s(debugMsg, sizeof(debugMsg), _TRUNCATE,
                "[GUI] Broker B quote expired (age=%llu ms, limit=%llu ms)",
                age, DATA_EXPIRY_MS);
        }
        OutputDebugStringA(debugMsg);
    }
    
    // Add Broker A row
    char brokerAText[] = "Broker A";
    LVITEMA lviA = {0};
    lviA.mask = LVIF_TEXT;
    lviA.iItem = 0;
    lviA.iSubItem = 0;
    lviA.pszText = brokerAText;
    int nItemA = ListView_InsertItemA(g_hListQuotes, &lviA);
    
    if (validA)
    {
        char buf[64];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", bidA);
        ListView_SetItemTextA(g_hListQuotes, nItemA, 1, buf);
        
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", askA);
        ListView_SetItemTextA(g_hListQuotes, nItemA, 2, buf);
        
        double spreadA = askA - bidA;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", spreadA);
        ListView_SetItemTextA(g_hListQuotes, nItemA, 3, buf);
        
        // Format timestamp
        unsigned long long ts = g_pSharedData->quotes[0].timestamp;
        unsigned long long current = GetCurrentTimestamp();
        unsigned long long age = current - ts;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%llu ms ago", age);
        ListView_SetItemTextA(g_hListQuotes, nItemA, 4, buf);
    }
    else
    {
        ListView_SetItemTextA(g_hListQuotes, nItemA, 1, "N/A");
        ListView_SetItemTextA(g_hListQuotes, nItemA, 2, "N/A");
        ListView_SetItemTextA(g_hListQuotes, nItemA, 3, "N/A");
        ListView_SetItemTextA(g_hListQuotes, nItemA, 4, "N/A");
    }
    
    // Add Broker B row
    char brokerBText[] = "Broker B";
    LVITEMA lviB = {0};
    lviB.mask = LVIF_TEXT;
    lviB.iItem = 1;
    lviB.iSubItem = 0;
    lviB.pszText = brokerBText;
    int nItemB = ListView_InsertItemA(g_hListQuotes, &lviB);
    
    if (validB)
    {
        char buf[64];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", bidB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 1, buf);
        
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", askB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 2, buf);
        
        double spreadB = askB - bidB;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", spreadB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 3, buf);
        
        // Format timestamp
        unsigned long long ts = g_pSharedData->quotes[1].timestamp;
        unsigned long long current = GetCurrentTimestamp();
        unsigned long long age = current - ts;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%llu ms ago", age);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 4, buf);
    }
    else
    {
        char naTextB[] = "N/A";
        ListView_SetItemTextA(g_hListQuotes, nItemB, 1, naTextB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 2, naTextB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 3, naTextB);
        ListView_SetItemTextA(g_hListQuotes, nItemB, 4, naTextB);
    }
    
    // Add Spread row (Bid_B - Ask_A and Bid_A - Ask_B)
    if (validA && validB)
    {
        char spreadText[] = "Spread (B-A)";
        LVITEMA lviSpread = {0};
        lviSpread.mask = LVIF_TEXT;
        lviSpread.iItem = 2;
        lviSpread.iSubItem = 0;
        lviSpread.pszText = spreadText;
        int nItemSpread = ListView_InsertItemA(g_hListQuotes, &lviSpread);
        
        char buf[64];
        // Diff1 = Bid_B - Ask_A (positive means opportunity to open long A, short B)
        double diff1 = bidB - askA;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", diff1);
        ListView_SetItemTextA(g_hListQuotes, nItemSpread, 1, buf);
        
        // Diff2 = Bid_A - Ask_B (positive means opportunity to open long B, short A)
        double diff2 = bidA - askB;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", diff2);
        ListView_SetItemTextA(g_hListQuotes, nItemSpread, 2, buf);
        
        // Max spread (the better opportunity)
        double maxSpread = (diff1 > diff2) ? diff1 : diff2;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", maxSpread);
        ListView_SetItemTextA(g_hListQuotes, nItemSpread, 3, buf);
        
        // Timestamp (use the newer one)
        unsigned long long tsA = g_pSharedData->quotes[0].timestamp;
        unsigned long long tsB = g_pSharedData->quotes[1].timestamp;
        unsigned long long latestTs = (tsA > tsB) ? tsA : tsB;
        unsigned long long current = GetCurrentTimestamp();
        unsigned long long age = current - latestTs;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%llu ms ago", age);
        ListView_SetItemTextA(g_hListQuotes, nItemSpread, 4, buf);
    }
    
    // Add summary row with key metrics
    if (validA && validB)
    {
        char summaryText[] = "Summary";
        LVITEMA lviSummary = {0};
        lviSummary.mask = LVIF_TEXT;
        lviSummary.iItem = 3;
        lviSummary.iSubItem = 0;
        lviSummary.pszText = summaryText;
        int nItemSummary = ListView_InsertItemA(g_hListQuotes, &lviSummary);
        
        char buf[64];
        // Average bid
        double avgBid = (bidA + bidB) / 2.0;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", avgBid);
        ListView_SetItemTextA(g_hListQuotes, nItemSummary, 1, buf);
        
        // Average ask
        double avgAsk = (askA + askB) / 2.0;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", avgAsk);
        ListView_SetItemTextA(g_hListQuotes, nItemSummary, 2, buf);
        
        // Average spread
        double avgSpread = ((askA - bidA) + (askB - bidB)) / 2.0;
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.5f", avgSpread);
        ListView_SetItemTextA(g_hListQuotes, nItemSummary, 3, buf);
        
        // Timestamp
        char liveText[] = "Live";
        ListView_SetItemTextA(g_hListQuotes, nItemSummary, 4, liveText);
    }
}

// Update status bar
void UpdateStatusBar()
{
    if (!g_hStatusBar)
        return;
    
    char statusText[512];
    
    if (g_pSharedData)
    {
        // Check data validity with detailed status
        bool hasA = g_pSharedData->quotes[0].isValid && !IsDataExpired(g_pSharedData->quotes[0].timestamp);
        bool hasB = g_pSharedData->quotes[1].isValid && !IsDataExpired(g_pSharedData->quotes[1].timestamp);
        
        // Get detailed status for A
        char statusA[64];
        if (hasA)
        {
            unsigned long long age = GetCurrentTimestamp() - g_pSharedData->quotes[0].timestamp;
            _snprintf_s(statusA, sizeof(statusA), _TRUNCATE, "Live (%llu ms)", age);
        }
        else if (g_pSharedData->quotes[0].isValid)
        {
            unsigned long long age = GetCurrentTimestamp() - g_pSharedData->quotes[0].timestamp;
            _snprintf_s(statusA, sizeof(statusA), _TRUNCATE, "Expired (%llu ms)", age);
        }
        else
        {
            _snprintf_s(statusA, sizeof(statusA), _TRUNCATE, "No Data");
        }
        
        // Get detailed status for B
        char statusB[64];
        if (hasB)
        {
            unsigned long long age = GetCurrentTimestamp() - g_pSharedData->quotes[1].timestamp;
            _snprintf_s(statusB, sizeof(statusB), _TRUNCATE, "Live (%llu ms)", age);
        }
        else if (g_pSharedData->quotes[1].isValid)
        {
            unsigned long long age = GetCurrentTimestamp() - g_pSharedData->quotes[1].timestamp;
            _snprintf_s(statusB, sizeof(statusB), _TRUNCATE, "Expired (%llu ms)", age);
        }
        else
        {
            _snprintf_s(statusB, sizeof(statusB), _TRUNCATE, "No Data");
        }
        
        _snprintf_s(statusText, sizeof(statusText), _TRUNCATE,
            "Symbol: %s | Shared Memory: Connected | Version: %ld | A: %s | B: %s",
            g_symbolName.c_str(), g_pSharedData->version, statusA, statusB);
    }
    else
    {
        std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;
        _snprintf_s(statusText, sizeof(statusText), _TRUNCATE,
            "Symbol: %s | Shared Memory: Not Connected | MapName: %s | Waiting for EA...",
            g_symbolName.c_str(), mapName.c_str());
    }
    
    SendMessageA(g_hStatusBar, SB_SETTEXTA, 0, (LPARAM)statusText);
}

// Update positions display
void UpdatePositionsDisplay()
{
    if (!g_hListPositions || g_symbolName.empty())
        return;
    
    // Clear existing items
    ListView_DeleteAllItemsA(g_hListPositions);
    
    std::string commonPath = GetMT4CommonFilesPath();
    
    // Read Broker A position (only first/current position)
    char fileA[512];
    _snprintf_s(fileA, sizeof(fileA), _TRUNCATE, "%s\\ArbState_Positions_%s_A.csv",
        commonPath.c_str(), g_symbolName.c_str());
    
    std::string posA[9];  // Store A position data
    bool hasA = false;
    
    std::ifstream inA(fileA);
    if (inA.is_open())
    {
        std::string line;
        bool firstLine = true;
        if (std::getline(inA, line))  // Skip header
        {
            if (std::getline(inA, line))  // Read first position only
            {
                std::istringstream iss(line);
                std::string token;
                int idx = 0;
                while (std::getline(iss, token, ';') && idx < 9)
                {
                    posA[idx++] = token;
                }
                if (idx >= 9)
                {
                    hasA = true;
                }
            }
        }
        inA.close();
    }
    
    // Read Broker B position (only first/current position)
    char fileB[512];
    _snprintf_s(fileB, sizeof(fileB), _TRUNCATE, "%s\\ArbState_Positions_%s_B.csv",
        commonPath.c_str(), g_symbolName.c_str());
    
    std::string posB[9];  // Store B position data
    bool hasB = false;
    
    std::ifstream inB(fileB);
    if (inB.is_open())
    {
        std::string line;
        bool firstLine = true;
        if (std::getline(inB, line))  // Skip header
        {
            if (std::getline(inB, line))  // Read first position only
            {
                std::istringstream iss(line);
                std::string token;
                int idx = 0;
                while (std::getline(iss, token, ';') && idx < 9)
                {
                    posB[idx++] = token;
                }
                if (idx >= 9)
                {
                    hasB = true;
                }
            }
        }
        inB.close();
    }
    
    // Display combined position (A and B as one row)
    if (hasA || hasB)
    {
        char pairText[] = "A+B";
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = pairText;
        int nItem = ListView_InsertItemA(g_hListPositions, &lvi);
        
        // Ticket: A/B
        char ticketBuf[64];
        if (hasA && hasB)
            _snprintf_s(ticketBuf, sizeof(ticketBuf), _TRUNCATE, "%s/%s", posA[0].c_str(), posB[0].c_str());
        else if (hasA)
            _snprintf_s(ticketBuf, sizeof(ticketBuf), _TRUNCATE, "%s/-", posA[0].c_str());
        else
            _snprintf_s(ticketBuf, sizeof(ticketBuf), _TRUNCATE, "-/%s", posB[0].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 1, ticketBuf);
        
        // Type: A/B (BUY/SELL)
        char typeBuf[64];
        if (hasA && hasB)
            _snprintf_s(typeBuf, sizeof(typeBuf), _TRUNCATE, "%s/%s", posA[1].c_str(), posB[1].c_str());
        else if (hasA)
            _snprintf_s(typeBuf, sizeof(typeBuf), _TRUNCATE, "%s/-", posA[1].c_str());
        else
            _snprintf_s(typeBuf, sizeof(typeBuf), _TRUNCATE, "-/%s", posB[1].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 2, typeBuf);
        
        // Lots: A/B
        char lotsBuf[64];
        if (hasA && hasB)
            _snprintf_s(lotsBuf, sizeof(lotsBuf), _TRUNCATE, "%s/%s", posA[2].c_str(), posB[2].c_str());
        else if (hasA)
            _snprintf_s(lotsBuf, sizeof(lotsBuf), _TRUNCATE, "%s/-", posA[2].c_str());
        else
            _snprintf_s(lotsBuf, sizeof(lotsBuf), _TRUNCATE, "-/%s", posB[2].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 3, lotsBuf);
        
        // OpenPrice: A/B
        char openPriceBuf[64];
        if (hasA && hasB)
            _snprintf_s(openPriceBuf, sizeof(openPriceBuf), _TRUNCATE, "%s/%s", posA[3].c_str(), posB[3].c_str());
        else if (hasA)
            _snprintf_s(openPriceBuf, sizeof(openPriceBuf), _TRUNCATE, "%s/-", posA[3].c_str());
        else
            _snprintf_s(openPriceBuf, sizeof(openPriceBuf), _TRUNCATE, "-/%s", posB[3].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 4, openPriceBuf);
        
        // CurrentPrice: A/B
        char currentPriceBuf[64];
        if (hasA && hasB)
            _snprintf_s(currentPriceBuf, sizeof(currentPriceBuf), _TRUNCATE, "%s/%s", posA[4].c_str(), posB[4].c_str());
        else if (hasA)
            _snprintf_s(currentPriceBuf, sizeof(currentPriceBuf), _TRUNCATE, "%s/-", posA[4].c_str());
        else
            _snprintf_s(currentPriceBuf, sizeof(currentPriceBuf), _TRUNCATE, "-/%s", posB[4].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 5, currentPriceBuf);
        
        // Profit: Combined (A + B)
        char profitBuf[64];
        if (hasA && hasB)
        {
            double profitA = atof(posA[5].c_str());
            double profitB = atof(posB[5].c_str());
            double totalProfit = profitA + profitB;
            _snprintf_s(profitBuf, sizeof(profitBuf), _TRUNCATE, "%.2f (%.2f+%.2f)", totalProfit, profitA, profitB);
        }
        else if (hasA)
            _snprintf_s(profitBuf, sizeof(profitBuf), _TRUNCATE, "%s", posA[5].c_str());
        else
            _snprintf_s(profitBuf, sizeof(profitBuf), _TRUNCATE, "%s", posB[5].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 6, profitBuf);
        
        // Time: A/B
        char timeBuf[64];
        if (hasA && hasB)
            _snprintf_s(timeBuf, sizeof(timeBuf), _TRUNCATE, "%s/%s", posA[8].c_str(), posB[8].c_str());
        else if (hasA)
            _snprintf_s(timeBuf, sizeof(timeBuf), _TRUNCATE, "%s", posA[8].c_str());
        else
            _snprintf_s(timeBuf, sizeof(timeBuf), _TRUNCATE, "%s", posB[8].c_str());
        ListView_SetItemTextA(g_hListPositions, nItem, 7, timeBuf);
    }
}

// Update history display - Pair A and B records and show combined profit
void UpdateHistoryDisplay()
{
    if (!g_hListHistory || g_symbolName.empty())
        return;
    
    // Clear existing items
    ListView_DeleteAllItemsA(g_hListHistory);
    
    std::string commonPath = GetMT4CommonFilesPath();
    
    // Read Broker A history into vector
    char fileA[512];
    _snprintf_s(fileA, sizeof(fileA), _TRUNCATE, "%s\\ArbState_History_%s_A.csv",
        commonPath.c_str(), g_symbolName.c_str());
    
    std::vector<std::vector<std::string> > historyA;
    std::ifstream inA(fileA);
    if (inA.is_open())
    {
        std::string line;
        bool firstLine = true;
        while (std::getline(inA, line) && historyA.size() < 50)  // Limit to 50 records
        {
            if (firstLine) { firstLine = false; continue; }  // Skip header
            
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(iss, token, ';'))
            {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 9)
            {
                historyA.push_back(tokens);
            }
        }
        inA.close();
    }
    
    // Read Broker B history into vector
    char fileB[512];
    _snprintf_s(fileB, sizeof(fileB), _TRUNCATE, "%s\\ArbState_History_%s_B.csv",
        commonPath.c_str(), g_symbolName.c_str());
    
    std::vector<std::vector<std::string> > historyB;
    std::ifstream inB(fileB);
    if (inB.is_open())
    {
        std::string line;
        bool firstLine = true;
        while (std::getline(inB, line) && historyB.size() < 50)  // Limit to 50 records
        {
            if (firstLine) { firstLine = false; continue; }  // Skip header
            
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(iss, token, ';'))
            {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 9)
            {
                historyB.push_back(tokens);
            }
        }
        inB.close();
    }
    
    // Pair A and B records by time (within 5 seconds tolerance)
    // Format: Time;Action;Ticket;Type;Lots;OpenPrice;ClosePrice;Profit;Comment
    int row = 0;
    std::vector<bool> usedB(historyB.size(), false);
    
    for (size_t i = 0; i < historyA.size() && row < 50; i++)
    {
        // Try to find matching B record (same time, opposite type)
        bool foundMatch = false;
        for (size_t j = 0; j < historyB.size(); j++)
        {
            if (usedB[j]) continue;
            
            // Check if times are close (within 5 seconds)
            // Simple string comparison (assuming format: "YYYY.MM.DD HH:MM:SS")
            std::string timeA = historyA[i][0];
            std::string timeB = historyB[j][0];
            
            // Check if types are opposite (A: BUY, B: SELL or vice versa)
            std::string typeA = historyA[i][3];
            std::string typeB = historyB[j][3];
            bool oppositeTypes = ((typeA == "BUY" && typeB == "SELL") || (typeA == "SELL" && typeB == "BUY"));
            
            // Match if times are same (or very close) and types are opposite
            if (timeA == timeB && oppositeTypes)
            {
                // Found a pair!
                char pairText[] = "A+B";
                LVITEMA lvi = {0};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = row;
                lvi.iSubItem = 0;
                lvi.pszText = pairText;
                int nItem = ListView_InsertItemA(g_hListHistory, &lvi);
                
                // Time
                ListView_SetItemTextA(g_hListHistory, nItem, 1, const_cast<char*>(timeA.c_str()));
                
                // Action (should be CLOSE for both)
                ListView_SetItemTextA(g_hListHistory, nItem, 2, const_cast<char*>(historyA[i][1].c_str()));
                
                // Ticket: A/B
                char ticketBuf[64];
                _snprintf_s(ticketBuf, sizeof(ticketBuf), _TRUNCATE, "%s/%s", 
                    historyA[i][2].c_str(), historyB[j][2].c_str());
                ListView_SetItemTextA(g_hListHistory, nItem, 3, ticketBuf);
                
                // Type: A/B
                char typeBuf[64];
                _snprintf_s(typeBuf, sizeof(typeBuf), _TRUNCATE, "%s/%s", 
                    historyA[i][3].c_str(), historyB[j][3].c_str());
                ListView_SetItemTextA(g_hListHistory, nItem, 4, typeBuf);
                
                // Lots: A/B
                char lotsBuf[64];
                _snprintf_s(lotsBuf, sizeof(lotsBuf), _TRUNCATE, "%s/%s", 
                    historyA[i][4].c_str(), historyB[j][4].c_str());
                ListView_SetItemTextA(g_hListHistory, nItem, 5, lotsBuf);
                
                // OpenPrice: A/B
                char openPriceBuf[64];
                _snprintf_s(openPriceBuf, sizeof(openPriceBuf), _TRUNCATE, "%s/%s", 
                    historyA[i][5].c_str(), historyB[j][5].c_str());
                ListView_SetItemTextA(g_hListHistory, nItem, 6, openPriceBuf);
                
                // ClosePrice: A/B
                char closePriceBuf[64];
                _snprintf_s(closePriceBuf, sizeof(closePriceBuf), _TRUNCATE, "%s/%s", 
                    historyA[i][6].c_str(), historyB[j][6].c_str());
                ListView_SetItemTextA(g_hListHistory, nItem, 7, closePriceBuf);
                
                // Combined Profit: A + B
                double profitA = atof(historyA[i][7].c_str());
                double profitB = atof(historyB[j][7].c_str());
                double totalProfit = profitA + profitB;
                char profitBuf[64];
                _snprintf_s(profitBuf, sizeof(profitBuf), _TRUNCATE, "%.2f (%.2f+%.2f)", 
                    totalProfit, profitA, profitB);
                ListView_SetItemTextA(g_hListHistory, nItem, 8, profitBuf);
                
                usedB[j] = true;
                foundMatch = true;
                row++;
                break;
            }
        }
        
        // If no match found, show A only
        if (!foundMatch && row < 50)
        {
            char brokerText[] = "A";
            LVITEMA lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = row;
            lvi.iSubItem = 0;
            lvi.pszText = brokerText;
            int nItem = ListView_InsertItemA(g_hListHistory, &lvi);
            
            ListView_SetItemTextA(g_hListHistory, nItem, 1, const_cast<char*>(historyA[i][0].c_str()));  // Time
            ListView_SetItemTextA(g_hListHistory, nItem, 2, const_cast<char*>(historyA[i][1].c_str()));  // Action
            ListView_SetItemTextA(g_hListHistory, nItem, 3, const_cast<char*>(historyA[i][2].c_str()));  // Ticket
            ListView_SetItemTextA(g_hListHistory, nItem, 4, const_cast<char*>(historyA[i][3].c_str()));  // Type
            ListView_SetItemTextA(g_hListHistory, nItem, 5, const_cast<char*>(historyA[i][4].c_str()));  // Lots
            ListView_SetItemTextA(g_hListHistory, nItem, 6, const_cast<char*>(historyA[i][5].c_str()));  // OpenPrice
            ListView_SetItemTextA(g_hListHistory, nItem, 7, const_cast<char*>(historyA[i][6].c_str()));  // ClosePrice
            ListView_SetItemTextA(g_hListHistory, nItem, 8, const_cast<char*>(historyA[i][7].c_str()));  // Profit
            row++;
        }
    }
    
    // Show unmatched B records
    for (size_t j = 0; j < historyB.size() && row < 50; j++)
    {
        if (!usedB[j])
        {
            char brokerText[] = "B";
            LVITEMA lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = row;
            lvi.iSubItem = 0;
            lvi.pszText = brokerText;
            int nItem = ListView_InsertItemA(g_hListHistory, &lvi);
            
            ListView_SetItemTextA(g_hListHistory, nItem, 1, const_cast<char*>(historyB[j][0].c_str()));  // Time
            ListView_SetItemTextA(g_hListHistory, nItem, 2, const_cast<char*>(historyB[j][1].c_str()));  // Action
            ListView_SetItemTextA(g_hListHistory, nItem, 3, const_cast<char*>(historyB[j][2].c_str()));  // Ticket
            ListView_SetItemTextA(g_hListHistory, nItem, 4, const_cast<char*>(historyB[j][3].c_str()));  // Type
            ListView_SetItemTextA(g_hListHistory, nItem, 5, const_cast<char*>(historyB[j][4].c_str()));  // Lots
            ListView_SetItemTextA(g_hListHistory, nItem, 6, const_cast<char*>(historyB[j][5].c_str()));  // OpenPrice
            ListView_SetItemTextA(g_hListHistory, nItem, 7, const_cast<char*>(historyB[j][6].c_str()));  // ClosePrice
            ListView_SetItemTextA(g_hListHistory, nItem, 8, const_cast<char*>(historyB[j][7].c_str()));  // Profit
            row++;
        }
    }
}

// Cleanup shared memory
void CleanupSharedMemory()
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

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            // Create ListView for quotes
            g_hListQuotes = CreateWindowA(
                WC_LISTVIEWA,
                "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                10, 10, 600, 200,
                hWnd,
                NULL,
                ((LPCREATESTRUCTA)lParam)->hInstance,
                NULL
            );
            
            if (g_hListQuotes)
            {
                // Set extended style
                ListView_SetExtendedListViewStyle(g_hListQuotes, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
                
                // Add columns
                LVCOLUMNA lvc = {0};
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                
                char colBroker[] = "Broker";
                lvc.iSubItem = 0;
                lvc.pszText = colBroker;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListQuotes, 0, &lvc);
                
                char colBid[] = "Bid";
                lvc.iSubItem = 1;
                lvc.pszText = colBid;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListQuotes, 1, &lvc);
                
                char colAsk[] = "Ask";
                lvc.iSubItem = 2;
                lvc.pszText = colAsk;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListQuotes, 2, &lvc);
                
                char colSpread[] = "Spread";
                lvc.iSubItem = 3;
                lvc.pszText = colSpread;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListQuotes, 3, &lvc);
                
                // Add timestamp column
                char colUpdate[] = "Last Update";
                lvc.iSubItem = 4;
                lvc.pszText = colUpdate;
                lvc.cx = 150;
                ListView_InsertColumnA(g_hListQuotes, 4, &lvc);
            }
            
            // Create ListView for positions
            g_hListPositions = CreateWindowA(
                WC_LISTVIEWA,
                "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                10, 220, 600, 200,
                hWnd,
                NULL,
                ((LPCREATESTRUCTA)lParam)->hInstance,
                NULL
            );
            
            if (g_hListPositions)
            {
                ListView_SetExtendedListViewStyle(g_hListPositions, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
                
                LVCOLUMNA lvc = {0};
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                
                char colBroker[] = "Broker";
                lvc.iSubItem = 0;
                lvc.pszText = colBroker;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListPositions, 0, &lvc);
                
                char colTicket[] = "Ticket";
                lvc.iSubItem = 1;
                lvc.pszText = colTicket;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListPositions, 1, &lvc);
                
                char colType[] = "Type";
                lvc.iSubItem = 2;
                lvc.pszText = colType;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListPositions, 2, &lvc);
                
                char colLots[] = "Lots";
                lvc.iSubItem = 3;
                lvc.pszText = colLots;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListPositions, 3, &lvc);
                
                char colOpenPrice[] = "OpenPrice";
                lvc.iSubItem = 4;
                lvc.pszText = colOpenPrice;
                lvc.cx = 90;
                ListView_InsertColumnA(g_hListPositions, 4, &lvc);
                
                char colCurrentPrice[] = "CurrentPrice";
                lvc.iSubItem = 5;
                lvc.pszText = colCurrentPrice;
                lvc.cx = 90;
                ListView_InsertColumnA(g_hListPositions, 5, &lvc);
                
                char colProfit[] = "Profit";
                lvc.iSubItem = 6;
                lvc.pszText = colProfit;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListPositions, 6, &lvc);
                
                char colTime[] = "Time";
                lvc.iSubItem = 7;
                lvc.pszText = colTime;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListPositions, 7, &lvc);
            }
            
            // Create ListView for history
            g_hListHistory = CreateWindowA(
                WC_LISTVIEWA,
                "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                620, 10, 600, 410,
                hWnd,
                NULL,
                ((LPCREATESTRUCTA)lParam)->hInstance,
                NULL
            );
            
            if (g_hListHistory)
            {
                ListView_SetExtendedListViewStyle(g_hListHistory, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
                
                LVCOLUMNA lvc = {0};
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                
                char colBroker[] = "Broker";
                lvc.iSubItem = 0;
                lvc.pszText = colBroker;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListHistory, 0, &lvc);
                
                char colTime[] = "Time";
                lvc.iSubItem = 1;
                lvc.pszText = colTime;
                lvc.cx = 120;
                ListView_InsertColumnA(g_hListHistory, 1, &lvc);
                
                char colAction[] = "Action";
                lvc.iSubItem = 2;
                lvc.pszText = colAction;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListHistory, 2, &lvc);
                
                char colTicket[] = "Ticket";
                lvc.iSubItem = 3;
                lvc.pszText = colTicket;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListHistory, 3, &lvc);
                
                char colType[] = "Type";
                lvc.iSubItem = 4;
                lvc.pszText = colType;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListHistory, 4, &lvc);
                
                char colLots[] = "Lots";
                lvc.iSubItem = 5;
                lvc.pszText = colLots;
                lvc.cx = 60;
                ListView_InsertColumnA(g_hListHistory, 5, &lvc);
                
                char colOpenPrice[] = "OpenPrice";
                lvc.iSubItem = 6;
                lvc.pszText = colOpenPrice;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListHistory, 6, &lvc);
                
                char colClosePrice[] = "ClosePrice";
                lvc.iSubItem = 7;
                lvc.pszText = colClosePrice;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListHistory, 7, &lvc);
                
                char colProfit[] = "Profit";
                lvc.iSubItem = 8;
                lvc.pszText = colProfit;
                lvc.cx = 80;
                ListView_InsertColumnA(g_hListHistory, 8, &lvc);
            }
            
            // Create status bar
            g_hStatusBar = CreateWindowA(
                STATUSCLASSNAMEA,
                "",
                WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
                0, 0, 0, 0,
                hWnd,
                NULL,
                ((LPCREATESTRUCTA)lParam)->hInstance,
                NULL
            );
            
            // Initialize shared memory
            InitSharedMemory();
            
            // Start timer for periodic updates
            SetTimer(hWnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
            
            // Initial update
            UpdateQuoteDisplay();
            UpdatePositionsDisplay();
            UpdateHistoryDisplay();
            UpdateStatusBar();
        }
        break;
        
    case WM_TIMER:
        if (wParam == TIMER_ID)
        {
            // Try to connect if not connected
            if (!g_pSharedData)
            {
                InitSharedMemory();
            }
            
            // Update display
            UpdateQuoteDisplay();
            UpdatePositionsDisplay();
            UpdateHistoryDisplay();
            UpdateStatusBar();
        }
        break;
        
    case WM_SIZE:
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int statusHeight = 0;
            if (g_hStatusBar)
            {
                RECT statusRect;
                GetWindowRect(g_hStatusBar, &statusRect);
                statusHeight = statusRect.bottom - statusRect.top;
            }
            
            int clientHeight = rc.bottom - statusHeight;
            int clientWidth = rc.right;
            
            // Resize Quotes ListView (top left)
            if (g_hListQuotes)
            {
                SetWindowPos(g_hListQuotes, NULL, 10, 10,
                    (clientWidth - 30) / 2, 200,
                    SWP_NOZORDER);
            }
            
            // Resize Positions ListView (bottom left)
            if (g_hListPositions)
            {
                SetWindowPos(g_hListPositions, NULL, 10, 220,
                    (clientWidth - 30) / 2, clientHeight - 230,
                    SWP_NOZORDER);
            }
            
            // Resize History ListView (right side)
            if (g_hListHistory)
            {
                SetWindowPos(g_hListHistory, NULL, (clientWidth + 10) / 2, 10,
                    (clientWidth - 30) / 2, clientHeight - 20,
                    SWP_NOZORDER);
            }
            
            // Resize status bar
            if (g_hStatusBar)
            {
                SendMessageA(g_hStatusBar, WM_SIZE, 0, 0);
            }
        }
        break;
        
    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ID);
        CleanupSharedMemory();
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Parse command line for symbol (optional)
    if (lpCmdLine && strlen(lpCmdLine) > 0)
    {
        g_symbolName = lpCmdLine;
    }
    
    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "HedgeArbControlGUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassA(&wc))
    {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    g_hWnd = CreateWindowA(
        "HedgeArbControlGUI",
        "HedgeArb EA Control - Quote Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1250, 500,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hWnd)
    {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Message loop
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    return (int)msg.wParam;
}
