// HedgeArbEA 总控程序
// 功能：管理两个MT4实例之间的共享内存通信，并在平仓完成后写入历史记录

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "HedgeArbSharedMemory.h"

// 配置
const char* SYMBOL_NAME = "GOLD";  // 根据实际使用的品种修改
const char* DATA_DIR = "D:\\mt4\\test\\";  // 数据文件目录
const int CHECK_INTERVAL_MS = 100;  // 检查间隔（毫秒）
const int MAX_DATA_AGE_MS = 5000;   // 数据最大有效期（毫秒），超过此时间认为数据过期

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
    std::string mapName = std::string(SHARED_MEMORY_NAME_PREFIX) + SYMBOL_NAME;
    
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
        return false;
    }
    
    // 如果是新创建的，初始化数据结构
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        memset(g_pSharedData, 0, sizeof(SharedData));
        g_pSharedData->version = 0;
        g_pSharedData->lockFlag = 0;
    }
    
    std::cout << "Shared memory initialized successfully." << std::endl;
    return true;
}

// 写入报价历史记录
void WriteQuoteHistory() {
    std::string filename = std::string(DATA_DIR) + "ArbPrice_" + SYMBOL_NAME + "_History_" + GetCurrentTimeString() + ".csv";
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

// 写入平仓完成记录
void WriteCloseRecord(const char* longBroker, const char* shortBroker, 
                      double profitA, double profitB, double profitTotal) {
    std::string filename = std::string(DATA_DIR) + "ArbClose_" + SYMBOL_NAME + "_" + GetCurrentTimeString() + ".csv";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    
    file << "Time,LongBroker,ShortBroker,ProfitA,ProfitB,ProfitTotal" << std::endl;
    file << GetCurrentTimeString() << ","
         << longBroker << ","
         << shortBroker << ","
         << profitA << ","
         << profitB << ","
         << profitTotal << std::endl;
    
    file.close();
    std::cout << "Close record written to: " << filename << std::endl;
}

// 检查是否平仓完成
bool CheckCloseCompleted() {
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

// 处理平仓完成
void HandleCloseCompleted() {
    std::cout << "Close completed detected. Writing history..." << std::endl;
    
    // 读取盈亏数据
    double profitA = g_pSharedData->profits[0].profitPips;
    double profitB = g_pSharedData->profits[1].profitPips;
    double profitTotal = profitA + profitB;
    
    // 写入平仓记录
    WriteCloseRecord(
        g_pSharedData->command.longBroker,
        g_pSharedData->command.shortBroker,
        profitA,
        profitB,
        profitTotal
    );
    
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
            
            // 输出状态（每10次更新输出一次，避免日志过多）
            static int logCount = 0;
            logCount++;
            if (logCount % 10 == 0) {
                std::cout << "Version: " << currentVersion 
                          << " | Quote A: " << (quoteAValid ? "Valid" : "Invalid")
                          << " | Quote B: " << (quoteBValid ? "Valid" : "Invalid")
                          << " | Command: " << (g_pSharedData->command.isActive ? g_pSharedData->command.code : "None")
                          << std::endl;
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

// 主函数
int main(int argc, char* argv[]) {
    std::cout << "HedgeArbEA Control Program" << std::endl;
    std::cout << "Symbol: " << SYMBOL_NAME << std::endl;
    std::cout << "Data Directory: " << DATA_DIR << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl;
    
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


