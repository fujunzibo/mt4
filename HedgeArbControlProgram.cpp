// HedgeArbEA 总控程序
// 功能：管理两个MT4实例之间的共享内存通信，并在平仓完成后写入历史记录

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include "HedgeArbSharedMemory.h"

// 配置（可通过命令行参数覆盖）
std::string g_symbolName = "GOLD";  // 根据实际使用的品种修改
std::string g_dataDir = "D:\\mt4\\test\\";  // 数据文件目录
const int CHECK_INTERVAL_MS = 100;  // 检查间隔（毫秒）
const int MAX_DATA_AGE_MS = 5000;   // 数据最大有效期（毫秒），超过此时间认为数据过期
const int STATUS_LOG_INTERVAL = 10;  // 状态日志输出间隔（每N次更新输出一次）

// 全局变量
SharedData* g_pSharedData = NULL;
HANDLE g_hMapFile = NULL;
bool g_running = true;

static const char* SHARED_MEMORY_NAME_PREFIX = "HedgeArbEA_SharedMemory_";
// 获取当前时间字符串
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

// 获取当前时间戳（毫秒）
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

// 检查数据是否过期
bool IsDataExpired(unsigned long long timestamp) {
    unsigned long long current = GetCurrentTimestamp();
    return (current - timestamp) > MAX_DATA_AGE_MS;
}

// 初始化共享内存
bool InitSharedMemory() {
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + g_symbolName;
    
    // 尝试打开已存在的共享内存
    g_hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        mapName.c_str()
    );
    
    if (g_hMapFile == NULL) {
        // 如果不存在，创建新的共享内存
        g_hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(SharedData),
            mapName.c_str()
        );
        
        if (g_hMapFile == NULL) {
            std::cerr << "Failed to create shared memory. Error: " << GetLastError() << std::endl;
            return false;
        }
    }
    
    // 映射到进程地址空间
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    );
    
    if (g_pSharedData == NULL) {
        std::cerr << "Failed to map view of file. Error: " << GetLastError() << std::endl;
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
        return false;
    }
    
    // ==== 修复：主控程序不应该初始化或清空共享内存 ====
    // 共享内存应该由EA端初始化，主控程序只负责读取和显示
    // 如果主控程序先启动，共享内存会被创建但保持未初始化状态
    // 等待EA启动后初始化，这样不会清除EA已写入的指令
    
    // 验证数据结构（检查signature和version）
    bool isValid = true;
    if (memcmp(g_pSharedData->signature, "HEDGEV1", 8) != 0) {
        // 如果signature不匹配，可能是新创建的共享内存，等待EA初始化
        std::cout << "Warning: Shared memory signature not found. Waiting for EA initialization..." << std::endl;
        isValid = false;
    } else if (g_pSharedData->structVersion != 1) {
        std::cerr << "Warning: Shared memory version mismatch. Expected 1, got " 
                  << g_pSharedData->structVersion << std::endl;
        isValid = false;
    }
    
    if (isValid) {
        std::cout << "Shared memory initialized successfully. Version: " 
                  << g_pSharedData->structVersion << std::endl;
    } else {
        std::cout << "Shared memory connected, waiting for EA to initialize..." << std::endl;
    }
    
    return true;
}

// 写入报价历史记录
void WriteQuoteHistory() {
    std::string filename = g_dataDir + "ArbPrice_" + g_symbolName + "_History_" + GetCurrentTimeString() + ".csv";
    std::ofstream file(filename, std::ios::app);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    
    // 写入CSV头
    file << "Time,A_Bid,A_Ask,B_Bid,B_Ask" << std::endl;
    
    // 写入当前报价
    std::string timeStr = GetCurrentTimeString();
    file << timeStr << ","
         << g_pSharedData->quotes[0].bid << ","
         << g_pSharedData->quotes[0].ask << ","
         << g_pSharedData->quotes[1].bid << ","
         << g_pSharedData->quotes[1].ask << std::endl;
    
    file.close();
}

// 写入平仓完成记录（增强版，包含更多信息）
void WriteCloseRecord(const char* longBroker, const char* shortBroker, 
                      double profitA, double profitB, double profitTotal) {
    std::string filename = g_dataDir + "ArbClose_" + g_symbolName + "_" + GetCurrentTimeString() + ".csv";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    
    // 写入详细记录
    file << "Time,LongBroker,ShortBroker,ProfitA_Pips,ProfitB_Pips,ProfitTotal_Pips,"
         << "ProfitA_USD,ProfitB_USD,ProfitTotal_USD,"
         << "QuoteA_Bid,QuoteA_Ask,QuoteB_Bid,QuoteB_Ask" << std::endl;
    
    double profitA_USD = g_pSharedData->profits[0].profitUSD;
    double profitB_USD = g_pSharedData->profits[1].profitUSD;
    double profitTotal_USD = profitA_USD + profitB_USD;
    
    file << GetCurrentTimeString() << ","
         << longBroker << ","
         << shortBroker << ","
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
    std::cout << "Close record written to: " << filename << std::endl;
}

// 检查是否平仓完成（增强版，包含更详细的检查）
bool CheckCloseCompleted() {
    // 首先验证数据结构
    if (g_pSharedData == NULL) {
        return false;
    }
    
    // 检查signature（如果EA已初始化）
    if (memcmp(g_pSharedData->signature, "HEDGEV1", 8) != 0) {
        return false;  // EA尚未初始化
    }
    
    // 检查是否有CLOSEALL指令
    if (!g_pSharedData->command.isActive) {
        return false;
    }
    
    if (strcmp(g_pSharedData->command.code, "CLOSEALL") != 0) {
        return false;
    }
    
    // 检查两个终端是否都已确认成功
    if (!g_pSharedData->acks[0].isValid || !g_pSharedData->acks[1].isValid) {
        return false;
    }
    
    if (strcmp(g_pSharedData->acks[0].code, "SUCCESS") != 0 ||
        strcmp(g_pSharedData->acks[1].code, "SUCCESS") != 0) {
        return false;
    }
    
    return true;
}

// 处理平仓完成（增强版，包含更详细的日志）
void HandleCloseCompleted() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Close completed detected. Writing history..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 读取盈亏数据
    double profitA = g_pSharedData->profits[0].profitPips;
    double profitB = g_pSharedData->profits[1].profitPips;
    double profitTotal = profitA + profitB;
    double profitA_USD = g_pSharedData->profits[0].profitUSD;
    double profitB_USD = g_pSharedData->profits[1].profitUSD;
    double profitTotal_USD = profitA_USD + profitB_USD;
    
    // 输出详细信息
    std::cout << "Long Broker: " << g_pSharedData->command.longBroker << std::endl;
    std::cout << "Short Broker: " << g_pSharedData->command.shortBroker << std::endl;
    std::cout << "Profit A (Pips): " << profitA << std::endl;
    std::cout << "Profit B (Pips): " << profitB << std::endl;
    std::cout << "Profit Total (Pips): " << profitTotal << std::endl;
    std::cout << "Profit A (USD): " << profitA_USD << std::endl;
    std::cout << "Profit B (USD): " << profitB_USD << std::endl;
    std::cout << "Profit Total (USD): " << profitTotal_USD << std::endl;
    std::cout << "Quote A: " << g_pSharedData->quotes[0].bid << " / " << g_pSharedData->quotes[0].ask << std::endl;
    std::cout << "Quote B: " << g_pSharedData->quotes[1].bid << " / " << g_pSharedData->quotes[1].ask << std::endl;
    
    // 写入平仓记录
    WriteCloseRecord(
        g_pSharedData->command.longBroker,
        g_pSharedData->command.shortBroker,
        profitA,
        profitB,
        profitTotal
    );
    
    std::cout << "========================================\n" << std::endl;
    
    // 清除指令和确认（为下一轮开仓做准备）
    // 注意：这里不清除，让EA自己清除，避免竞争条件
}

// 监控循环
void MonitorLoop() {
    long lastVersion = -1;
    bool lastCloseState = false;
    
    while (g_running) {
        // 检查数据版本是否更新
        long currentVersion = g_pSharedData->version;
        if (currentVersion != lastVersion) {
            lastVersion = currentVersion;
            
            // 检查报价数据有效性
            bool quoteAValid = g_pSharedData->quotes[0].isValid && 
                               !IsDataExpired(g_pSharedData->quotes[0].timestamp);
            bool quoteBValid = g_pSharedData->quotes[1].isValid && 
                               !IsDataExpired(g_pSharedData->quotes[1].timestamp);
            
            // 输出状态（每N次更新输出一次，避免日志过多）
            static int logCount = 0;
            logCount++;
            if (logCount % STATUS_LOG_INTERVAL == 0) {
                // 检查数据结构是否已初始化
                bool dataInitialized = (memcmp(g_pSharedData->signature, "HEDGEV1", 8) == 0);
                
                std::cout << "[Status] Version: " << currentVersion;
                if (dataInitialized) {
                    std::cout << " | Quote A: " << (quoteAValid ? "Valid" : "Invalid");
                    if (quoteAValid) {
                        std::cout << " (" << g_pSharedData->quotes[0].bid << "/" << g_pSharedData->quotes[0].ask << ")";
                    }
                    std::cout << " | Quote B: " << (quoteBValid ? "Valid" : "Invalid");
                    if (quoteBValid) {
                        std::cout << " (" << g_pSharedData->quotes[1].bid << "/" << g_pSharedData->quotes[1].ask << ")";
                    }
                    std::cout << " | Command: " << (g_pSharedData->command.isActive ? g_pSharedData->command.code : "None");
                    if (g_pSharedData->command.isActive) {
                        std::cout << " [" << g_pSharedData->command.longBroker << "/" << g_pSharedData->command.shortBroker << "]";
                    }
                    // 显示确认状态
                    std::cout << " | Ack A: " << (g_pSharedData->acks[0].isValid ? g_pSharedData->acks[0].code : "None");
                    std::cout << " | Ack B: " << (g_pSharedData->acks[1].isValid ? g_pSharedData->acks[1].code : "None");
                } else {
                    std::cout << " | Waiting for EA initialization...";
                }
                std::cout << std::endl;
            }
            
            // 检查平仓完成
            bool currentCloseState = CheckCloseCompleted();
            if (currentCloseState && !lastCloseState) {
                HandleCloseCompleted();
            }
            lastCloseState = currentCloseState;
        }
        
        Sleep(CHECK_INTERVAL_MS);
    }
}

// 清理资源
void Cleanup() {
    if (g_pSharedData != NULL) {
        UnmapViewOfFile(g_pSharedData);
        g_pSharedData = NULL;
    }
    
    if (g_hMapFile != NULL) {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}

// 信号处理
BOOL WINAPI ConsoleHandler(DWORD dwType) {
    switch (dwType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        std::cout << "\nShutting down..." << std::endl;
        g_running = false;
        return TRUE;
    default:
        return FALSE;
    }
}

// 显示使用说明
void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -s <symbol>     Symbol name (default: GOLD)" << std::endl;
    std::cout << "  -d <directory> Data directory (default: D:\\mt4\\test\\)" << std::endl;
    std::cout << "  -h             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " -s XAUUSD -d C:\\MT4\\data\\" << std::endl;
}

// 解析命令行参数
void ParseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            g_symbolName = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            g_dataDir = argv[++i];
            // 确保目录路径以反斜杠结尾
            if (g_dataDir.back() != '\\' && g_dataDir.back() != '/') {
                g_dataDir += "\\";
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            exit(0);
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            PrintUsage(argv[0]);
            exit(1);
        }
    }
}

// 主函数
int main(int argc, char* argv[]) {
    // 解析命令行参数
    ParseCommandLine(argc, argv);
    
    std::cout << "========================================" << std::endl;
    std::cout << "HedgeArbEA Control Program" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Symbol: " << g_symbolName << std::endl;
    std::cout << "Data Directory: " << g_dataDir << std::endl;
    std::cout << "Check Interval: " << CHECK_INTERVAL_MS << " ms" << std::endl;
    std::cout << "Max Data Age: " << MAX_DATA_AGE_MS << " ms" << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 设置控制台信号处理
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    // 初始化共享内存
    if (!InitSharedMemory()) {
        std::cerr << "Failed to initialize shared memory. Exiting." << std::endl;
        return 1;
    }
    
    // 启动监控循环
    MonitorLoop();
    
    // 清理资源
    Cleanup();
    
    std::cout << "Program exited." << std::endl;
    return 0;
}


