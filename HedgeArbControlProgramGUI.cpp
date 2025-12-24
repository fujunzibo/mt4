// HedgeArbEA Control GUI Program (C++ Win32 Version)
// Function: Control and GUI merged, manage shared memory and display real-time data
// Feature: Direct access to shared memory (optimal performance), can start anywhere

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include "HedgeArbSharedMemory.h"

#pragma comment(lib, "comctl32.lib")

// Configuration (can be modified in UI)
std::string g_symbolName = "GOLD";
std::string g_dataDir = "D:\\mt4\\test\\";
const int UPDATE_INTERVAL_MS = 50;   // UI update interval (ms)
const int MAX_DATA_AGE_MS = 5000;    // Max data age (ms)

// Shared memory
SharedData* g_pSharedData = NULL;
HANDLE g_hMapFile = NULL;
static const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";

// GUI control handles
HWND g_hWnd = NULL;
HWND g_hLabelStatus = NULL;
HWND g_hLabelQuoteA = NULL;
HWND g_hLabelQuoteB = NULL;
HWND g_hLabelQuoteDiff = NULL;
HWND g_hLabelProfitA = NULL;
HWND g_hLabelProfitB = NULL;
HWND g_hLabelProfitTotal = NULL;
HWND g_hLabelCommand = NULL;
HWND g_hLabelAckA = NULL;
HWND g_hLabelAckB = NULL;
HWND g_hEditSymbol = NULL;
HWND g_hEditDataDir = NULL;

// State
bool g_running = true;
long g_lastVersion = -1;
bool g_lastCloseState = false;

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitSharedMemory();
void CleanupSharedMemory();
void UpdateGUI();
void WriteCloseHistory();
bool CheckCloseCompleted();
std::string GetCurrentTimeString();
unsigned long long GetCurrentTimestamp();
bool IsDataExpired(unsigned long long timestamp);
void CreateControls(HWND hWnd);
void SetWindowTextSafe(HWND hWnd, const char* text);
std::string FormatDouble(double value, int precision);

// Get current time string
std::string GetCurrentTimeString() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << st.wYear
        << std::setw(2) << st.wMonth
        << std::setw(2) << st.wDay
        << "_"
        << std::setw(2) << st.wHour
        << std::setw(2) << st.wMinute
        << std::setw(2) << st.wSecond;
    return oss.str();
}

// Get current timestamp (milliseconds)
unsigned long long GetCurrentTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000;
}

// Check if data is expired
bool IsDataExpired(unsigned long long timestamp) {
    unsigned long long current = GetCurrentTimestamp();
    return (current - timestamp) > MAX_DATA_AGE_MS;
}

// Initialize shared memory (direct access, optimal performance)
bool InitSharedMemory() {
    CleanupSharedMemory();
    
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;
    
    // Try to open existing shared memory
    g_hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        mapName.c_str()
    );
    
    if (g_hMapFile == NULL) {
        // If not exists, create new shared memory
        g_hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(SharedData),
            mapName.c_str()
        );
        
        if (g_hMapFile == NULL) {
            return false;
        }
    }
    
    // Map to process address space
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    );
    
    if (g_pSharedData == NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    
    g_lastVersion = -1;
    return true;
}

// Cleanup shared memory
void CleanupSharedMemory() {
    if (g_pSharedData != NULL) {
        UnmapViewOfFile(g_pSharedData);
        g_pSharedData = NULL;
    }
    
    if (g_hMapFile != NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}

// Check if close completed
bool CheckCloseCompleted() {
    if (g_pSharedData == NULL) return false;
    
    if (memcmp(g_pSharedData->signature, "HEDGEV1", 8) != 0) {
        return false;
    }
    
    if (!g_pSharedData->command.isActive) {
        return false;
    }
    
    if (strcmp(g_pSharedData->command.code, "CLOSEALL") != 0) {
        return false;
    }
    
    if (!g_pSharedData->acks[0].isValid || !g_pSharedData->acks[1].isValid) {
        return false;
    }
    
    if (strcmp(g_pSharedData->acks[0].code, "SUCCESS") != 0 ||
        strcmp(g_pSharedData->acks[1].code, "SUCCESS") != 0) {
        return false;
    }
    
    return true;
}

// Write close history
void WriteCloseHistory() {
    if (g_pSharedData == NULL) return;
    
    if (g_dataDir.back() != '\\' && g_dataDir.back() != '/') {
        g_dataDir += "\\";
    }
    
    DWORD dwAttrib = GetFileAttributesA(g_dataDir.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        CreateDirectoryA(g_dataDir.c_str(), NULL);
    }
    
    std::string filename = g_dataDir + "ArbClose_" + g_symbolName + "_" + GetCurrentTimeString() + ".csv";
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    double profitA = g_pSharedData->profits[0].profitPips;
    double profitB = g_pSharedData->profits[1].profitPips;
    double profitTotal = profitA + profitB;
    double profitA_USD = g_pSharedData->profits[0].profitUSD;
    double profitB_USD = g_pSharedData->profits[1].profitUSD;
    double profitTotal_USD = profitA_USD + profitB_USD;
    
    file << "Time,LongBroker,ShortBroker,ProfitA_Pips,ProfitB_Pips,ProfitTotal_Pips,"
         << "ProfitA_USD,ProfitB_USD,ProfitTotal_USD,"
         << "QuoteA_Bid,QuoteA_Ask,QuoteB_Bid,QuoteB_Ask" << std::endl;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeStr[64];
    sprintf_s(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02d:%02d:%02d",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    file << timeStr << ","
         << g_pSharedData->command.longBroker << ","
         << g_pSharedData->command.shortBroker << ","
         << profitA << ","
         << profitB << ","
         << profitTotal << ","
         << profitA_USD << ","
         << profitB_USD << ","
         << profitTotal_USD << ","
         << g_pSharedData->quotes[0].bid << ","
         << g_pSharedData->quotes[0].ask << ","
         << g_pSharedData->quotes[1].bid << ","
         << g_pSharedData->quotes[1].ask << std::endl;
    
    file.close();
}

// Safe set window text
void SetWindowTextSafe(HWND hWnd, const char* text) {
    if (hWnd != NULL && IsWindow(hWnd)) {
        SetWindowTextA(hWnd, text);
    }
}

// Format double
std::string FormatDouble(double value, int precision) {
    char buf[64];
    sprintf_s(buf, sizeof(buf), "%.*f", precision, value);
    return buf;
}

// Update GUI
void UpdateGUI() {
    if (g_pSharedData == NULL) {
        SetWindowTextSafe(g_hLabelStatus, "Status: Shared memory not connected");
        return;
    }
    
    long currentVersion = g_pSharedData->version;
    bool dataChanged = (currentVersion != g_lastVersion);
    
    if (dataChanged) {
        g_lastVersion = currentVersion;
        
        bool dataInitialized = (memcmp(g_pSharedData->signature, "HEDGEV1", 8) == 0);
        
        if (!dataInitialized) {
            SetWindowTextSafe(g_hLabelStatus, "Status: Waiting for EA initialization...");
            return;
        }
        
        bool quoteAValid = g_pSharedData->quotes[0].isValid && 
                          !IsDataExpired(g_pSharedData->quotes[0].timestamp);
        bool quoteBValid = g_pSharedData->quotes[1].isValid && 
                          !IsDataExpired(g_pSharedData->quotes[1].timestamp);
        
        if (quoteAValid) {
            double midA = (g_pSharedData->quotes[0].bid + g_pSharedData->quotes[0].ask) / 2.0;
            std::string text = "A: " + FormatDouble(midA, 5);
            SetWindowTextSafe(g_hLabelQuoteA, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelQuoteA, "A: -");
        }
        
        if (quoteBValid) {
            double midB = (g_pSharedData->quotes[1].bid + g_pSharedData->quotes[1].ask) / 2.0;
            std::string text = "B: " + FormatDouble(midB, 5);
            SetWindowTextSafe(g_hLabelQuoteB, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelQuoteB, "B: -");
        }
        
        if (quoteAValid && quoteBValid) {
            double midA = (g_pSharedData->quotes[0].bid + g_pSharedData->quotes[0].ask) / 2.0;
            double midB = (g_pSharedData->quotes[1].bid + g_pSharedData->quotes[1].ask) / 2.0;
            double diff = midA - midB;
            std::string text = "Diff: " + FormatDouble(diff, 5);
            SetWindowTextSafe(g_hLabelQuoteDiff, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelQuoteDiff, "Diff: -");
        }
        
        if (g_pSharedData->profits[0].isValid) {
            std::string text = "A: " + FormatDouble(g_pSharedData->profits[0].profitUSD, 2) + " USD";
            SetWindowTextSafe(g_hLabelProfitA, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelProfitA, "A: -");
        }
        
        if (g_pSharedData->profits[1].isValid) {
            std::string text = "B: " + FormatDouble(g_pSharedData->profits[1].profitUSD, 2) + " USD";
            SetWindowTextSafe(g_hLabelProfitB, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelProfitB, "B: -");
        }
        
        if (g_pSharedData->profits[0].isValid && g_pSharedData->profits[1].isValid) {
            double total = g_pSharedData->profits[0].profitUSD + g_pSharedData->profits[1].profitUSD;
            std::string text = "Total: " + FormatDouble(total, 2) + " USD";
            SetWindowTextSafe(g_hLabelProfitTotal, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelProfitTotal, "Total: -");
        }
        
        if (g_pSharedData->command.isActive && strlen(g_pSharedData->command.code) > 0) {
            char cmdText[256];
            if (strcmp(g_pSharedData->command.code, "OPEN") == 0) {
                sprintf_s(cmdText, sizeof(cmdText), "Command: OPEN [%s/%s]",
                         g_pSharedData->command.longBroker,
                         g_pSharedData->command.shortBroker);
            } else {
                sprintf_s(cmdText, sizeof(cmdText), "Command: %s",
                         g_pSharedData->command.code);
            }
            SetWindowTextSafe(g_hLabelCommand, cmdText);
        } else {
            SetWindowTextSafe(g_hLabelCommand, "Command: -");
        }
        
        if (g_pSharedData->acks[0].isValid) {
            std::string text = "A Ack: " + std::string(g_pSharedData->acks[0].code);
            SetWindowTextSafe(g_hLabelAckA, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelAckA, "A Ack: -");
        }
        
        if (g_pSharedData->acks[1].isValid) {
            std::string text = "B Ack: " + std::string(g_pSharedData->acks[1].code);
            SetWindowTextSafe(g_hLabelAckB, text.c_str());
        } else {
            SetWindowTextSafe(g_hLabelAckB, "B Ack: -");
        }
        
        char statusText[256];
        const char* dataStatus = (quoteAValid && quoteBValid) ? "OK" : 
                                (quoteAValid || quoteBValid) ? "Partial" : "Waiting";
        sprintf_s(statusText, sizeof(statusText), 
                 "Status: Connected (C++ Direct Access) | Version: %ld | Data: %s | Update: %dms",
                 currentVersion, dataStatus, UPDATE_INTERVAL_MS);
        SetWindowTextSafe(g_hLabelStatus, statusText);
        
        bool currentCloseState = CheckCloseCompleted();
        if (currentCloseState && !g_lastCloseState) {
            WriteCloseHistory();
        }
        g_lastCloseState = currentCloseState;
    }
}

// Create controls
void CreateControls(HWND hWnd) {
    g_hLabelStatus = CreateWindowA("STATIC", "Status: Initializing... (C++ Direct Access - Optimal Performance)",
                                   WS_VISIBLE | WS_CHILD,
                                   10, 10, 750, 20,
                                   hWnd, NULL, NULL, NULL);
    
    CreateWindowA("STATIC", "Symbol:",
                 WS_VISIBLE | WS_CHILD,
                 10, 40, 60, 20,
                 hWnd, NULL, NULL, NULL);
    g_hEditSymbol = CreateWindowA("EDIT", g_symbolName.c_str(),
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
                                   80, 38, 100, 22,
                                   hWnd, (HMENU)1001, NULL, NULL);
    
    CreateWindowA("STATIC", "Data Dir:",
                 WS_VISIBLE | WS_CHILD,
                 190, 40, 60, 20,
                 hWnd, NULL, NULL, NULL);
    g_hEditDataDir = CreateWindowA("EDIT", g_dataDir.c_str(),
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
                                   260, 38, 300, 22,
                                   hWnd, (HMENU)1002, NULL, NULL);
    
    CreateWindowA("BUTTON", "Reconnect",
                 WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                 570, 37, 80, 24,
                 hWnd, (HMENU)2001, NULL, NULL);
    
    CreateWindowA("STATIC", "=== Quotes ===",
                 WS_VISIBLE | WS_CHILD,
                 10, 70, 200, 20,
                 hWnd, NULL, NULL, NULL);
    g_hLabelQuoteA = CreateWindowA("STATIC", "A: -",
                                   WS_VISIBLE | WS_CHILD,
                                   10, 95, 200, 20,
                                   hWnd, NULL, NULL, NULL);
    g_hLabelQuoteB = CreateWindowA("STATIC", "B: -",
                                   WS_VISIBLE | WS_CHILD,
                                   10, 120, 200, 20,
                                   hWnd, NULL, NULL, NULL);
    g_hLabelQuoteDiff = CreateWindowA("STATIC", "Diff: -",
                                      WS_VISIBLE | WS_CHILD,
                                      10, 145, 200, 20,
                                      hWnd, NULL, NULL, NULL);
    
    CreateWindowA("STATIC", "=== Profits ===",
                 WS_VISIBLE | WS_CHILD,
                 220, 70, 200, 20,
                 hWnd, NULL, NULL, NULL);
    g_hLabelProfitA = CreateWindowA("STATIC", "A: -",
                                     WS_VISIBLE | WS_CHILD,
                                     220, 95, 200, 20,
                                     hWnd, NULL, NULL, NULL);
    g_hLabelProfitB = CreateWindowA("STATIC", "B: -",
                                     WS_VISIBLE | WS_CHILD,
                                     220, 120, 200, 20,
                                     hWnd, NULL, NULL, NULL);
    g_hLabelProfitTotal = CreateWindowA("STATIC", "Total: -",
                                        WS_VISIBLE | WS_CHILD,
                                        220, 145, 200, 20,
                                        hWnd, NULL, NULL, NULL);
    
    CreateWindowA("STATIC", "=== Commands ===",
                 WS_VISIBLE | WS_CHILD,
                 430, 70, 200, 20,
                 hWnd, NULL, NULL, NULL);
    g_hLabelCommand = CreateWindowA("STATIC", "Command: -",
                                    WS_VISIBLE | WS_CHILD,
                                    430, 95, 200, 20,
                                    hWnd, NULL, NULL, NULL);
    g_hLabelAckA = CreateWindowA("STATIC", "A Ack: -",
                                  WS_VISIBLE | WS_CHILD,
                                  430, 120, 200, 20,
                                  hWnd, NULL, NULL, NULL);
    g_hLabelAckB = CreateWindowA("STATIC", "B Ack: -",
                                  WS_VISIBLE | WS_CHILD,
                                  430, 145, 200, 20,
                                  hWnd, NULL, NULL, NULL);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateControls(hWnd);
        if (InitSharedMemory()) {
            SetWindowTextSafe(g_hLabelStatus, "Status: Shared memory connected");
        } else {
            SetWindowTextSafe(g_hLabelStatus, "Status: Shared memory connection failed");
        }
        SetTimer(hWnd, 1, UPDATE_INTERVAL_MS, NULL);
        break;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == 2001) {
            char symbol[64] = {0};
            char dataDir[512] = {0};
            GetWindowTextA(g_hEditSymbol, symbol, sizeof(symbol));
            GetWindowTextA(g_hEditDataDir, dataDir, sizeof(dataDir));
            
            if (strlen(symbol) > 0) {
                g_symbolName = symbol;
            }
            if (strlen(dataDir) > 0) {
                g_dataDir = dataDir;
            }
            
            CleanupSharedMemory();
            if (InitSharedMemory()) {
                SetWindowTextSafe(g_hLabelStatus, "Status: Shared memory connected");
                g_lastVersion = -1;
            } else {
                SetWindowTextSafe(g_hLabelStatus, "Status: Shared memory connection failed");
            }
        }
        break;
        
    case WM_TIMER:
        if (wParam == 1) {
            UpdateGUI();
        }
        break;
        
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        CleanupSharedMemory();
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    WNDCLASSEXA wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "HedgeArbControlGUI";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExA(&wcex)) {
        return 1;
    }
    
    g_hWnd = CreateWindowExA(
        0,
        "HedgeArbControlGUI",
        "HedgeArb Control GUI (C++ Version - Optimal Performance)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        800, 400,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hWnd) {
        return 1;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
