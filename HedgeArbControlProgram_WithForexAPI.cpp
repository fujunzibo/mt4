// HedgeArbEA 总控程序（集成 ForexAPI 版本）
// 功能：管理两个MT4实例之间的共享内存通信，并通过 ForexAPI 获取实时报价

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "../HedgeArbEA/HedgeArbSharedMemory_New/HedgeArbSharedMemory.h"
#include "../ForexAPI/ForexComAPI.h"

using namespace ForexComAPI;

// 配置（可通过命令行参数覆盖）
std::string g_symbolName = "GOLD";  // 根据实际使用的品种修改
std::string g_dataDir = "D:\\mt4\\test\\";  // 数据文件目录
const int CHECK_INTERVAL_MS = 100;  // 检查间隔（毫秒）
const int MAX_DATA_AGE_MS = 5000;   // 数据最大有效期（毫秒）
const int STATUS_LOG_INTERVAL = 10;  // 状态日志输出间隔（每N次更新输出一次）

// ForexAPI 配置
bool g_useForexAPI = false;  // 是否使用 ForexAPI（默认：否，使用MT4报价）
std::string g_forexApiKey = "";
std::string g_forexApiSecret = "";
std::string g_forexSymbol = "XAUUSD";  // ForexAPI 使用的品种名称（可能与MT4不同）
bool g_useForexWebSocket = false;  // 是否使用 WebSocket（默认：REST API）

// 全局变量
SharedData* g_pSharedData = NULL;
HANDLE g_hMapFile = NULL;
bool g_running = true;
ForexComAPIClient* g_forexClient = NULL;
std::thread* g_forexThread = NULL;

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

// 初始化 ForexAPI
bool InitForexAPI() {
    if (!g_useForexAPI) {
        return true;  // 不使用 ForexAPI
    }
    
    if (g_forexApiKey.empty() || g_forexApiSecret.empty()) {
        std::cerr << "警告：ForexAPI 已启用，但 API Key 或 Secret 为空" << std::endl;
        return false;
    }
    
    // 配置 ForexAPI
    APIConfig config;
    config.apiKey = g_forexApiKey;
    config.apiSecret = g_forexApiSecret;
    config.useSandbox = true;  // 使用测试环境
    config.timeoutMs = 5000;
    
    if (g_useForexWebSocket) {
        config.wsUrl = "wss://api-demo.forex.com/streaming";
    } else {
        config.baseUrl = "https://api-demo.forex.com/v1";
    }
    
    // 创建客户端
    g_forexClient = new ForexComAPIClient(config);
    
    // 初始化
    if (!g_forexClient->Initialize()) {
        std::cerr << "ForexAPI 初始化失败: " << g_forexClient->GetLastError() << std::endl;
        delete g_forexClient;
        g_forexClient = NULL;
        return false;
    }
    
    std::cout << "ForexAPI 初始化成功" << std::endl;
    
    // 如果使用 WebSocket，连接并订阅
    if (g_useForexWebSocket) {
        if (!g_forexClient->ConnectWebSocket()) {
            std::cerr << "ForexAPI WebSocket 连接失败: " << g_forexClient->GetLastError() << std::endl;
            std::cerr << "将使用 REST API 模式" << std::endl;
            g_useForexWebSocket = false;
        } else {
            // 订阅报价
            if (!g_forexClient->SubscribeQuote(g_forexSymbol)) {
                std::cerr << "订阅报价失败: " << g_forexClient->GetLastError() << std::endl;
            } else {
                std::cout << "已订阅 " << g_forexSymbol << " 的实时报价" << std::endl;
            }
        }
    }
    
    return true;
}

// ForexAPI 报价获取线程（REST API 模式）
void ForexAPIFetchThread() {
    if (!g_forexClient || !g_pSharedData) {
        return;
    }
    
    std::cout << "[ForexAPI] 报价获取线程启动" << std::endl;
    
    while (g_running) {
        if (g_useForexWebSocket) {
            // WebSocket 模式：接收报价
            Quote quote;
            if (g_forexClient->ReceiveQuote(quote, 100)) {  // 100ms 超时
                // 将报价写入共享内存（作为 Broker A 的报价）
                if (g_pSharedData) {
                    g_pSharedData->quotes[0].bid = quote.bid;
                    g_pSharedData->quotes[0].ask = quote.ask;
                    g_pSharedData->quotes[0].timestamp = quote.timestamp;
                    g_pSharedData->quotes[0].isValid = quote.isValid;
                    InterlockedIncrement(&g_pSharedData->version);
                }
            }
        } else {
            // REST API 模式：定期获取报价
            Quote quote;
            if (g_forexClient->GetQuote(g_forexSymbol, quote)) {
                // 将报价写入共享内存（作为 Broker A 的报价）
                if (g_pSharedData) {
                    g_pSharedData->quotes[0].bid = quote.bid;
                    g_pSharedData->quotes[0].ask = quote.ask;
                    g_pSharedData->quotes[0].timestamp = GetCurrentTimestamp();
                    g_pSharedData->quotes[0].isValid = quote.isValid;
                    InterlockedIncrement(&g_pSharedData->version);
                }
            } else {
                std::cerr << "[ForexAPI] 获取报价失败: " << g_forexClient->GetLastError() << std::endl;
            }
            
            // REST API 模式：每 100ms 获取一次
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "[ForexAPI] 报价获取线程退出" << std::endl;
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
    
    // 验证数据结构
    bool isValid = true;
    if (memcmp(g_pSharedData->signature, "HEDGEV1", 8) != 0) {
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
            
            // 输出状态（每N次更新输出一次）
            static int logCount = 0;
            logCount++;
            if (logCount % STATUS_LOG_INTERVAL == 0) {
                bool dataInitialized = (memcmp(g_pSharedData->signature, "HEDGEV1", 8) == 0);
                
                std::cout << "[Status] Version: " << currentVersion;
                if (dataInitialized) {
                    std::cout << " | Quote A: " << (quoteAValid ? "Valid" : "Invalid");
                    if (quoteAValid) {
                        std::cout << " (" << g_pSharedData->quotes[0].bid << "/" << g_pSharedData->quotes[0].ask << ")";
                        if (g_useForexAPI) {
                            std::cout << " [ForexAPI]";
                        }
                    }
                    std::cout << " | Quote B: " << (quoteBValid ? "Valid" : "Invalid");
                    if (quoteBValid) {
                        std::cout << " (" << g_pSharedData->quotes[1].bid << "/" << g_pSharedData->quotes[1].ask << ")";
                    }
                    std::cout << " | Command: " << (g_pSharedData->command.isActive ? g_pSharedData->command.code : "None");
                } else {
                    std::cout << " | Waiting for EA initialization...";
                }
                std::cout << std::endl;
            }
        }
        
        Sleep(CHECK_INTERVAL_MS);
    }
}

// 清理资源
void Cleanup() {
    g_running = false;
    
    // 等待 ForexAPI 线程结束
    if (g_forexThread) {
        g_forexThread->join();
        delete g_forexThread;
        g_forexThread = NULL;
    }
    
    // 清理 ForexAPI 客户端
    if (g_forexClient) {
        if (g_useForexWebSocket) {
            g_forexClient->DisconnectWebSocket();
        }
        delete g_forexClient;
        g_forexClient = NULL;
    }
    
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
    std::cout << "  -s <symbol>          Symbol name (default: GOLD)" << std::endl;
    std::cout << "  -d <directory>       Data directory (default: D:\\mt4\\test\\)" << std::endl;
    std::cout << "  --use-forex-api      Enable ForexAPI for quote fetching" << std::endl;
    std::cout << "  --forex-key <key>    ForexAPI API Key" << std::endl;
    std::cout << "  --forex-secret <secret>  ForexAPI API Secret" << std::endl;
    std::cout << "  --forex-symbol <symbol>  ForexAPI symbol (default: XAUUSD)" << std::endl;
    std::cout << "  --use-websocket      Use WebSocket instead of REST API" << std::endl;
    std::cout << "  -h                   Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " -s XAUUSD --use-forex-api --forex-key YOUR_KEY --forex-secret YOUR_SECRET" << std::endl;
}

// 解析命令行参数
void ParseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            g_symbolName = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            g_dataDir = argv[++i];
            if (g_dataDir.back() != '\\' && g_dataDir.back() != '/') {
                g_dataDir += "\\";
            }
        } else if (strcmp(argv[i], "--use-forex-api") == 0) {
            g_useForexAPI = true;
        } else if (strcmp(argv[i], "--forex-key") == 0 && i + 1 < argc) {
            g_forexApiKey = argv[++i];
        } else if (strcmp(argv[i], "--forex-secret") == 0 && i + 1 < argc) {
            g_forexApiSecret = argv[++i];
        } else if (strcmp(argv[i], "--forex-symbol") == 0 && i + 1 < argc) {
            g_forexSymbol = argv[++i];
        } else if (strcmp(argv[i], "--use-websocket") == 0) {
            g_useForexWebSocket = true;
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
    std::cout << "HedgeArbEA Control Program (with ForexAPI)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Symbol: " << g_symbolName << std::endl;
    std::cout << "Data Directory: " << g_dataDir << std::endl;
    std::cout << "Use ForexAPI: " << (g_useForexAPI ? "Yes" : "No") << std::endl;
    if (g_useForexAPI) {
        std::cout << "ForexAPI Symbol: " << g_forexSymbol << std::endl;
        std::cout << "Use WebSocket: " << (g_useForexWebSocket ? "Yes" : "No") << std::endl;
    }
    std::cout << "Check Interval: " << CHECK_INTERVAL_MS << " ms" << std::endl;
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
    
    // 初始化 ForexAPI（如果启用）
    if (g_useForexAPI) {
        if (!InitForexAPI()) {
            std::cerr << "警告：ForexAPI 初始化失败，将使用 MT4 报价" << std::endl;
            g_useForexAPI = false;
        } else {
            // 启动报价获取线程
            g_forexThread = new std::thread(ForexAPIFetchThread);
        }
    }
    
    // 启动监控循环
    MonitorLoop();
    
    // 清理资源
    Cleanup();
    
    std::cout << "Program exited." << std::endl;
    return 0;
}

