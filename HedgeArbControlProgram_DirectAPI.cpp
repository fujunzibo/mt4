// HedgeArbEA 主控程序 - 直接API交易版本（完整GUI）
// 功能：通过API直接与各经纪商交易，所有参数在主控界面设置

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <fstream>
#include "ForexAPI/ForexComAPI.h"
#include "ForexAPI/HttpClient.h"

#pragma comment(lib, "comctl32.lib")

using namespace ForexComAPI;

// 辅助函数：将 UTF-8 字符串转换为系统代码页（GBK）字符串
// 使用静态缓冲区，返回的指针在下次调用前有效
const char* UTF8ToGBK(const char* utf8) {
    static char gbk_buf[4][512];
    static int buf_idx = 0;
    buf_idx = (buf_idx + 1) % 4;
    
    if (!utf8 || !*utf8) return utf8;
    
    // 先转换为宽字符串（UTF-8 -> UTF-16）
    int wsize = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wsize <= 0) return utf8;
    
    wchar_t* wbuf = new wchar_t[wsize];
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, wsize);
    
    // 再转换为系统代码页（UTF-16 -> GBK）
    int size = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, gbk_buf[buf_idx], 512, NULL, NULL);
    delete[] wbuf;
    
    if (size <= 0) return utf8;
    return gbk_buf[buf_idx];
}

// 重载版本：接受 std::string，返回 std::string
std::string UTF8ToGBK(const std::string& utf8) {
    if (utf8.empty()) return std::string();
    const char* gbk = UTF8ToGBK(utf8.c_str());
    return std::string(gbk);
}

// 宏：将 UTF-8 字符串字面量转换为 GBK
#define U8(s) UTF8ToGBK(s)

// ==================== 中文字符串常量 ====================
// 为了避免编码问题，将所有中文字符串定义为函数，使用静态局部变量延迟初始化
inline const char* GetStr(const char* utf8) {
    static char buf[4][512];
    static int idx = 0;
    idx = (idx + 1) % 4;
    
    // 先转换为宽字符串（UTF-8 -> UTF-16）
    int wsize = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wsize <= 0) return utf8;
    
    wchar_t* wbuf = new wchar_t[wsize];
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, wsize);
    
    // 再转换为系统代码页（UTF-16 -> GBK）
    int size = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, buf[idx], 512, NULL, NULL);
    delete[] wbuf;
    
    if (size <= 0) return utf8;
    return buf[idx];
}

// 使用内联函数返回中文字符串，避免宏展开问题
inline const char* STR_LOGIC_SYMBOL() { return GetStr("\xe9\x80\xbb\xe8\xbe\x91\xe5\x93\x81\xe7\xa7\x8d:"); }
inline const char* STR_A_CONFIG() { return GetStr("A" "\xe7\xab\xaf\xe9\x85\x8d\xe7\xbd\xae:"); }
inline const char* STR_B_CONFIG() { return GetStr("B" "\xe7\xab\xaf\xe9\x85\x8d\xe7\xbd\xae:"); }
inline const char* STR_TRADE_SYMBOL() { return GetStr("\xe4\xba\xa4\xe6\x98\x93\xe5\x93\x81\xe7\xa7\x8d:"); }
inline const char* STR_LOTS() { return GetStr("\xe6\x89\x8b\xe6\x95\xb0:"); }
inline const char* STR_USE_WEBSOCKET() { return GetStr("\xe4\xbd\xbf\xe7\x94\xa8WebSocket"); }
inline const char* STR_TRADE_PARAMS() { return GetStr("\xe4\xba\xa4\xe6\x98\x93\xe5\x8f\x82\xe6\x95\xb0:"); }
inline const char* STR_OPEN_THRESHOLD() { return GetStr("\xe5\xbc\x80\xe4\xbb\x93\xe8\xa7\xa6\xe5\x8f\x91\xe7\x82\xb9\xe5\xb7\xae:"); }
inline const char* STR_CLOSE_TARGET() { return GetStr("\xe5\xb9\xb3\xe4\xbb\x93\xe7\x9b\xae\xe6\xa0\x87\xe7\x82\xb9\xe5\xb7\xae:"); }
inline const char* STR_STOP_LOSS() { return GetStr("\xe6\xad\xa2\xe6\x8d\x9f\xe7\x82\xb9\xe5\xb7\xae:"); }
inline const char* STR_SLIPPAGE() { return GetStr("\xe6\xbb\x91\xe7\x82\xb9:"); }
inline const char* STR_CHECK_INTERVAL() { return GetStr("\xe6\xa3\x80\xe6\x9f\xa5\xe9\x97\xb4\xe9\x9a\x94(ms):"); }
inline const char* STR_SAVE_CONFIG() { return GetStr("\xe4\xbf\x9d\xe5\xad\x98\xe9\x85\x8d\xe7\xbd\xae"); }
inline const char* STR_CONNECT_API() { return GetStr("\xe8\xbf\x9e\xe6\x8e\xa5" "API"); }
inline const char* STR_START_TRADING() { return GetStr("\xe5\x90\xaf\xe5\x8a\xa8\xe4\xba\xa4\xe6\x98\x93"); }
inline const char* STR_STOP_TRADING() { return GetStr("\xe5\x81\x9c\xe6\xad\xa2\xe4\xba\xa4\xe6\x98\x93"); }
inline const char* STR_EMERGENCY_CLOSE() { return GetStr("\xe7\xb4\xa7\xe6\x80\xa5\xe5\xb9\xb3\xe4\xbb\x93"); }
inline const char* STR_BROKER() { return GetStr("\xe7\xbb\x8f\xe7\xba\xaa\xe5\x95\x86"); }
inline const char* STR_SYMBOL() { return GetStr("\xe5\x93\x81\xe7\xa7\x8d"); }
inline const char* STR_BID() { return GetStr("\xe4\xb9\xb0\xe4\xbb\xb7"); }
inline const char* STR_ASK() { return GetStr("\xe5\x8d\x96\xe4\xbb\xb7"); }
inline const char* STR_SPREAD() { return GetStr("\xe4\xbb\xb7\xe5\xb7\xae"); }
inline const char* STR_UPDATE_TIME() { return GetStr("\xe6\x9b\xb4\xe6\x96\xb0\xe6\x97\xb6\xe9\x97\xb4"); }
inline const char* STR_ORDER_ID() { return GetStr("\xe8\xae\xa2\xe5\x8d\x95\xe5\x8f\xb7"); }
inline const char* STR_TYPE() { return GetStr("\xe7\xb1\xbb\xe5\x9e\x8b"); }
inline const char* STR_OPEN_PRICE() { return GetStr("\xe5\xbc\x80\xe4\xbb\x93\xe4\xbb\xb7"); }
inline const char* STR_CURRENT_PRICE() { return GetStr("\xe5\xbd\x93\xe5\x89\x8d\xe4\xbb\xb7"); }
inline const char* STR_PROFIT() { return GetStr("\xe7\x9b\x88\xe4\xba\x8f"); }
inline const char* STR_TIME() { return GetStr("\xe6\x97\xb6\xe9\x97\xb4"); }
inline const char* STR_ORDER_ID_A() { return GetStr("\xe8\xae\xa2\xe5\x8d\x95\xe5\x8f\xb7" "A"); }
inline const char* STR_ORDER_ID_B() { return GetStr("\xe8\xae\xa2\xe5\x8d\x95\xe5\x8f\xb7" "B"); }
inline const char* STR_STATUS() { return GetStr("\xe7\x8a\xb6\xe6\x80\x81"); }
inline const char* STR_TOTAL() { return GetStr("\xe5\x90\x88\xe8\xae\xa1"); }
inline const char* STR_CONNECTED() { return GetStr("\xe5\xb7\xb2\xe8\xbf\x9e\xe6\x8e\xa5"); }
inline const char* STR_DISCONNECTED() { return GetStr("\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5"); }
inline const char* STR_RUNNING() { return GetStr("\xe8\xbf\x90\xe8\xa1\x8c\xe4\xb8\xad"); }
inline const char* STR_STOPPED() { return GetStr("\xe5\xb7\xb2\xe5\x81\x9c\xe6\xad\xa2"); }
inline const char* STR_CLOSED() { return GetStr("\xe5\xb7\xb2\xe5\xb9\xb3\xe4\xbb\x93"); }
inline const char* STR_A_END() { return GetStr("A" "\xe7\xab\xaf"); }
inline const char* STR_B_END() { return GetStr("B" "\xe7\xab\xaf"); }
inline const char* STR_SUCCESS() { return GetStr("\xe6\x88\x90\xe5\x8a\x9f"); }
inline const char* STR_FAILED() { return GetStr("\xe5\xa4\xb1\xe8\xb4\xa5"); }
inline const char* STR_INFO() { return GetStr("\xe6\x8f\x90\xe7\xa4\xba"); }
inline const char* STR_ERROR() { return GetStr("\xe9\x94\x99\xe8\xaf\xaf"); }
inline const char* STR_CONFIG_SAVED() { return GetStr("\xe9\x85\x8d\xe7\xbd\xae\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98"); }
inline const char* STR_API_CONNECTED() { return GetStr("API" "\xe8\xbf\x9e\xe6\x8e\xa5\xe6\x88\x90\xe5\x8a\x9f"); }
inline const char* STR_CONNECT_FAILED() { return GetStr("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xa4\xb1\xe8\xb4\xa5"); }
inline const char* STR_PLEASE_CONNECT() { return GetStr("\xe8\xaf\xb7\xe5\x85\x88\xe8\xbf\x9e\xe6\x8e\xa5" "API"); }
inline const char* STR_TRADING_STARTED() { return GetStr("\xe4\xba\xa4\xe6\x98\x93\xe5\xb7\xb2\xe5\x90\xaf\xe5\x8a\xa8"); }
inline const char* STR_TRADING_STOPPED() { return GetStr("\xe4\xba\xa4\xe6\x98\x93\xe5\xb7\xb2\xe5\x81\x9c\xe6\xad\xa2"); }
inline const char* STR_EMERGENCY_EXECUTED() { return GetStr("\xe5\xb7\xb2\xe6\x89\xa7\xe8\xa1\x8c\xe7\xb4\xa7\xe6\x80\xa5\xe5\xb9\xb3\xe4\xbb\x93"); }
inline const char* STR_WINDOW_REG_FAILED() { return GetStr("\xe7\xaa\x97\xe5\x8f\xa3\xe6\xb3\xa8\xe5\x86\x8c\xe5\xa4\xb1\xe8\xb4\xa5"); }
inline const char* STR_WINDOW_CREATE_FAILED() { return GetStr("\xe7\xaa\x97\xe5\x8f\xa3\xe5\x88\x9b\xe5\xbb\xba\xe5\xa4\xb1\xe8\xb4\xa5"); }
inline const char* STR_WINDOW_TITLE() { return GetStr("\xe5\xaf\xb9\xe5\x86\xb2\xe5\xa5\x97\xe5\x88\xa9\xe4\xb8\xbb\xe6\x8e\xa7\xe7\xa8\x8b\xe5\xba\x8f - \xe7\x9b\xb4\xe6\x8e\xa5" "API" "\xe4\xba\xa4\xe6\x98\x93"); }
inline const char* STR_STATUS_FORMAT() { return GetStr("\xe9\x80\xbb\xe8\xbe\x91\xe5\x93\x81\xe7\xa7\x8d: %s | " "A" "\xe7\xab\xaf: %s (%.2f" "\xe6\x89\x8b" ") %s | " "B" "\xe7\xab\xaf: %s (%.2f" "\xe6\x89\x8b" ") %s | " "\xe4\xba\xa4\xe6\x98\x93: %s"); }
inline const char* STR_CONNECT_FAILED_FORMAT() { return GetStr("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xa4\xb1\xe8\xb4\xa5\n" "A" "\xe7\xab\xaf: %s\n" "B" "\xe7\xab\xaf: %s"); }

// ==================== 配置结构 ====================
struct TradingConfig {
    std::string logicalSymbol = "GOLD";
    
    struct BrokerConfig {
        std::string apiKey;
        std::string apiSecret;
        std::string tradeSymbol;
        double lots = 1.0;
        bool useWebSocket = true;
        bool isConnected = false;
        ForexComAPIClient* client = nullptr;
    } brokerA, brokerB;
    
    int thresholdPips = 100;
    int targetPips = 5;
    int stopLossPips = 180;
    int slippage = 3;
    int checkIntervalMs = 50;
    int magic = 20251126;
};

// ==================== 全局变量 ====================
TradingConfig g_config;
HWND g_hWnd = NULL;
HWND g_hStatusBar = NULL;
HWND g_hListQuotes = NULL;
HWND g_hListPositions = NULL;
HWND g_hListHistory = NULL;

// 参数设置控件
HWND g_hEditLogicalSymbol = NULL;
HWND g_hEditA_ApiKey = NULL, g_hEditA_ApiSecret = NULL, g_hEditA_Symbol = NULL, g_hEditA_Lots = NULL;
HWND g_hEditB_ApiKey = NULL, g_hEditB_ApiSecret = NULL, g_hEditB_Symbol = NULL, g_hEditB_Lots = NULL;
HWND g_hCheckA_WebSocket = NULL, g_hCheckB_WebSocket = NULL;
HWND g_hEditThreshold = NULL, g_hEditTarget = NULL, g_hEditStopLoss = NULL;
HWND g_hEditSlippage = NULL, g_hEditCheckInterval = NULL, g_hEditMagic = NULL;
HWND g_hBtnSave = NULL, g_hBtnConnect = NULL, g_hBtnStart = NULL, g_hBtnStop = NULL, g_hBtnCloseAll = NULL;

// 报价和持仓数据
struct QuoteData {
    double bid = 0.0;
    double ask = 0.0;
    unsigned long long timestamp = 0;
    bool isValid = false;
} g_quoteA, g_quoteB;

struct PositionData {
    std::string orderId;
    std::string symbol;
    ForexComAPI::OrderSide side;
    double quantity;
    double openPrice;
    double currentPrice;
    double profit;
    unsigned long long openTime;
    bool isValid = false;
} g_positionA, g_positionB;

// 交易历史
struct TradeHistory {
    std::string time;
    std::string orderIdA, orderIdB;
    double profit;
    std::string status;
};
std::vector<TradeHistory> g_tradeHistory;

// 线程控制
bool g_running = false;
std::thread* g_tradingThread = NULL;
std::mutex g_dataMutex;

// ==================== 工具函数 ====================
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

std::string GetTimeString() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(buf);
}

std::string DoubleToString(double value, int precision = 5) {
    char buf[64];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.*f", precision, value);
    return std::string(buf);
}

// ListView 辅助函数
inline int ListView_InsertItemA(HWND hwnd, LVITEMA* pitem) {
    return (int)SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)pitem);
}

inline BOOL ListView_SetItemTextA(HWND hwnd, int i, int iSubItem, const char* pszText) {
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;
    lvi.pszText = const_cast<LPSTR>(pszText);
    return (BOOL)SendMessageA(hwnd, LVM_SETITEMTEXTA, (WPARAM)i, (LPARAM)&lvi);
}

inline int ListView_InsertColumnA(HWND hwnd, int iCol, LVCOLUMNA* pcol) {
    return (int)SendMessageA(hwnd, LVM_INSERTCOLUMNA, (WPARAM)iCol, (LPARAM)pcol);
}

inline int ListView_GetItemCountA(HWND hwnd) {
    return (int)SendMessageA(hwnd, LVM_GETITEMCOUNT, 0, 0);
}

inline int ListView_DeleteAllItemsA(HWND hwnd) {
    return (int)SendMessageA(hwnd, LVM_DELETEALLITEMS, 0, 0);
}

// ==================== API 初始化 ====================
bool InitBrokerAPI(TradingConfig::BrokerConfig& broker, const std::string& name) {
    if (broker.apiKey.empty() || broker.apiSecret.empty()) {
        return false;
    }
    
    if (broker.client) {
        if (broker.useWebSocket) {
            broker.client->DisconnectWebSocket();
        }
        delete broker.client;
        broker.client = nullptr;
    }
    
    ForexComAPI::APIConfig config;
    config.apiKey = broker.apiKey;
    config.apiSecret = broker.apiSecret;
    config.useSandbox = true;
    config.timeoutMs = 5000;
    
    if (broker.useWebSocket) {
        config.wsUrl = "wss://api-demo.forex.com/streaming";
    } else {
        config.baseUrl = "https://api-demo.forex.com/v1";
    }
    
    broker.client = new ForexComAPIClient(config);
    
    if (!broker.client->Initialize()) {
        return false;
    }
    
    if (broker.useWebSocket) {
        if (!broker.client->ConnectWebSocket()) {
            broker.useWebSocket = false;
        } else {
            broker.client->SubscribeQuote(broker.tradeSymbol);
        }
    }
    
    broker.isConnected = true;
    return true;
}

// ==================== 报价获取 ====================
void FetchQuote(TradingConfig::BrokerConfig& broker, QuoteData& quote, const std::string& symbol) {
    if (!broker.client || !broker.isConnected) return;
    
    ForexComAPI::Quote apiQuote;
    bool success = false;
    
    if (broker.useWebSocket) {
        success = broker.client->ReceiveQuote(apiQuote, 100);
    } else {
        success = broker.client->GetQuote(symbol, apiQuote);
    }
    
    if (success && apiQuote.isValid) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        quote.bid = apiQuote.bid;
        quote.ask = apiQuote.ask;
        quote.timestamp = apiQuote.timestamp;
        quote.isValid = true;
    }
}

// ==================== 交易逻辑 ====================
bool OpenPosition(TradingConfig::BrokerConfig& broker, PositionData& position, 
                  const std::string& symbol, ForexComAPI::OrderSide side, double lots) {
    if (!broker.client || !broker.isConnected) return false;
    
    ForexComAPI::OrderRequest req;
    req.symbol = symbol;
    req.side = side;
    req.quantity = lots;
    req.type = ForexComAPI::OrderType::MARKET;
    
    ForexComAPI::OrderResponse resp;
    if (broker.client->PlaceOrder(req, resp) && resp.isSuccess) {
        position.orderId = resp.orderId;
        position.symbol = symbol;
        position.side = side;
        position.quantity = lots;
        position.openPrice = resp.price;
        position.currentPrice = resp.price;
        position.openTime = GetCurrentTimestamp();
        position.isValid = true;
        return true;
    }
    return false;
}

bool ClosePosition(TradingConfig::BrokerConfig& broker, PositionData& position) {
    if (!broker.client || !broker.isConnected || !position.isValid) return false;
    
    std::string message;
    if (broker.client->ClosePosition(position.orderId, message)) {
        position.isValid = false;
        return true;
    }
    return false;
}

// ==================== 交易线程 ====================
void TradingThread() {
    while (g_running) {
        FetchQuote(g_config.brokerA, g_quoteA, g_config.brokerA.tradeSymbol);
        FetchQuote(g_config.brokerB, g_quoteB, g_config.brokerB.tradeSymbol);
        
        bool quotesValid = g_quoteA.isValid && g_quoteB.isValid;
        
        if (quotesValid) {
            double spread1 = g_quoteB.bid - g_quoteA.ask;
            double spread2 = g_quoteA.bid - g_quoteB.ask;
            double maxSpread = (spread1 > spread2) ? spread1 : spread2;
            bool useSpread1 = (spread1 > spread2);
            
            bool hasPosition = g_positionA.isValid || g_positionB.isValid;
            
            if (!hasPosition) {
                double threshold = g_config.thresholdPips * 0.0001;
                if (maxSpread >= threshold) {
                    bool successA = false, successB = false;
                    
                    if (useSpread1) {
                        successA = OpenPosition(g_config.brokerA, g_positionA, 
                                               g_config.brokerA.tradeSymbol, 
                                               ForexComAPI::OrderSide::BUY, g_config.brokerA.lots);
                        successB = OpenPosition(g_config.brokerB, g_positionB, 
                                               g_config.brokerB.tradeSymbol, 
                                               ForexComAPI::OrderSide::SELL, g_config.brokerB.lots);
                    } else {
                        successA = OpenPosition(g_config.brokerA, g_positionA, 
                                               g_config.brokerA.tradeSymbol, 
                                               ForexComAPI::OrderSide::SELL, g_config.brokerA.lots);
                        successB = OpenPosition(g_config.brokerB, g_positionB, 
                                               g_config.brokerB.tradeSymbol, 
                                               ForexComAPI::OrderSide::BUY, g_config.brokerB.lots);
                    }
                    
                    if (!successA || !successB) {
                        if (successA) ClosePosition(g_config.brokerA, g_positionA);
                        if (successB) ClosePosition(g_config.brokerB, g_positionB);
                    }
                }
            } else {
                double currentSpread = useSpread1 ? spread1 : spread2;
                double target = g_config.targetPips * 0.0001;
                double stopLoss = g_config.stopLossPips * 0.0001;
                
                if (currentSpread <= target || maxSpread >= stopLoss) {
                    bool closedA = ClosePosition(g_config.brokerA, g_positionA);
                    bool closedB = ClosePosition(g_config.brokerB, g_positionB);
                    
                    if (closedA && closedB) {
                        TradeHistory history;
                        history.time = GetTimeString();
                        history.orderIdA = g_positionA.orderId;
                        history.orderIdB = g_positionB.orderId;
                        history.profit = g_positionA.profit + g_positionB.profit;
                        history.status = UTF8ToGBK("\xe5\xb7\xb2\xe5\xb9\xb3\xe4\xbb\x93");
                        g_tradeHistory.push_back(history);
                    }
                }
            }
        }
        
        // 更新持仓盈亏
        if (g_positionA.isValid) {
            FetchQuote(g_config.brokerA, g_quoteA, g_config.brokerA.tradeSymbol);
            g_positionA.currentPrice = (g_positionA.side == ForexComAPI::OrderSide::BUY) ? g_quoteA.bid : g_quoteA.ask;
            g_positionA.profit = (g_positionA.currentPrice - g_positionA.openPrice) * 
                               ((g_positionA.side == ForexComAPI::OrderSide::BUY) ? 1 : -1) * g_positionA.quantity;
        }
        
        if (g_positionB.isValid) {
            FetchQuote(g_config.brokerB, g_quoteB, g_config.brokerB.tradeSymbol);
            g_positionB.currentPrice = (g_positionB.side == ForexComAPI::OrderSide::BUY) ? g_quoteB.bid : g_quoteB.ask;
            g_positionB.profit = (g_positionB.currentPrice - g_positionB.openPrice) * 
                               ((g_positionB.side == ForexComAPI::OrderSide::BUY) ? 1 : -1) * g_positionB.quantity;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.checkIntervalMs));
    }
}

// ==================== GUI 更新 ====================
void UpdateQuoteDisplay() {
    if (!g_hListQuotes) return;
    
    ListView_DeleteAllItemsA(g_hListQuotes);
    
    std::lock_guard<std::mutex> lock(g_dataMutex);
    
    // A端报价
    if (g_quoteA.isValid) {
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        char brokerA[] = "A";
        lvi.pszText = brokerA;
        int idx = ListView_InsertItemA(g_hListQuotes, &lvi);
        
        ListView_SetItemTextA(g_hListQuotes, idx, 1, g_config.brokerA.tradeSymbol.c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 2, DoubleToString(g_quoteA.bid).c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 3, DoubleToString(g_quoteA.ask).c_str());
        
        double spreadA = g_quoteA.ask - g_quoteA.bid;
        ListView_SetItemTextA(g_hListQuotes, idx, 4, DoubleToString(spreadA).c_str());
        
        unsigned long long age = GetCurrentTimestamp() - g_quoteA.timestamp;
        char ageStr[32];
        _snprintf_s(ageStr, sizeof(ageStr), _TRUNCATE, "%llu ms", age);
        ListView_SetItemTextA(g_hListQuotes, idx, 5, ageStr);
    }
    
    // B端报价
    if (g_quoteB.isValid) {
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 1;
        char brokerB[] = "B";
        lvi.pszText = brokerB;
        int idx = ListView_InsertItemA(g_hListQuotes, &lvi);
        
        ListView_SetItemTextA(g_hListQuotes, idx, 1, g_config.brokerB.tradeSymbol.c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 2, DoubleToString(g_quoteB.bid).c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 3, DoubleToString(g_quoteB.ask).c_str());
        
        double spreadB = g_quoteB.ask - g_quoteB.bid;
        ListView_SetItemTextA(g_hListQuotes, idx, 4, DoubleToString(spreadB).c_str());
        
        unsigned long long age = GetCurrentTimestamp() - g_quoteB.timestamp;
        char ageStr[32];
        _snprintf_s(ageStr, sizeof(ageStr), _TRUNCATE, "%llu ms", age);
        ListView_SetItemTextA(g_hListQuotes, idx, 5, ageStr);
    }
    
    // 价差
    if (g_quoteA.isValid && g_quoteB.isValid) {
        double spread1 = g_quoteB.bid - g_quoteA.ask;
        double spread2 = g_quoteA.bid - g_quoteB.ask;
        double maxSpread = (spread1 > spread2) ? spread1 : spread2;
        
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 2;
        lvi.pszText = const_cast<LPSTR>(STR_SPREAD());
        int idx = ListView_InsertItemA(g_hListQuotes, &lvi);
        
        ListView_SetItemTextA(g_hListQuotes, idx, 2, DoubleToString(spread1).c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 3, DoubleToString(spread2).c_str());
        ListView_SetItemTextA(g_hListQuotes, idx, 4, DoubleToString(maxSpread).c_str());
    }
}

void UpdatePositionDisplay() {
    if (!g_hListPositions) return;
    
    ListView_DeleteAllItemsA(g_hListPositions);
    
    std::lock_guard<std::mutex> lock(g_dataMutex);
    
    // A端持仓
    if (g_positionA.isValid) {
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        char brokerA[] = "A";
        lvi.pszText = brokerA;
        int idx = ListView_InsertItemA(g_hListPositions, &lvi);
        
        ListView_SetItemTextA(g_hListPositions, idx, 1, g_positionA.orderId.c_str());
        char sideA[8];
        strcpy_s(sideA, sizeof(sideA), (g_positionA.side == ForexComAPI::OrderSide::BUY) ? "BUY" : "SELL");
        ListView_SetItemTextA(g_hListPositions, idx, 2, sideA);
        ListView_SetItemTextA(g_hListPositions, idx, 3, DoubleToString(g_positionA.quantity, 2).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 4, DoubleToString(g_positionA.openPrice).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 5, DoubleToString(g_positionA.currentPrice).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 6, DoubleToString(g_positionA.profit, 2).c_str());
    }
    
    // B端持仓
    if (g_positionB.isValid) {
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 1;
        char brokerB[] = "B";
        lvi.pszText = brokerB;
        int idx = ListView_InsertItemA(g_hListPositions, &lvi);
        
        ListView_SetItemTextA(g_hListPositions, idx, 1, g_positionB.orderId.c_str());
        char sideB[8];
        strcpy_s(sideB, sizeof(sideB), (g_positionB.side == ForexComAPI::OrderSide::BUY) ? "BUY" : "SELL");
        ListView_SetItemTextA(g_hListPositions, idx, 2, sideB);
        ListView_SetItemTextA(g_hListPositions, idx, 3, DoubleToString(g_positionB.quantity, 2).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 4, DoubleToString(g_positionB.openPrice).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 5, DoubleToString(g_positionB.currentPrice).c_str());
        ListView_SetItemTextA(g_hListPositions, idx, 6, DoubleToString(g_positionB.profit, 2).c_str());
    }
    
    // 总盈亏
    if (g_positionA.isValid || g_positionB.isValid) {
        double totalProfit = (g_positionA.isValid ? g_positionA.profit : 0) + 
                            (g_positionB.isValid ? g_positionB.profit : 0);
        
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 2;
        lvi.pszText = const_cast<LPSTR>(STR_TOTAL());
        int idx = ListView_InsertItemA(g_hListPositions, &lvi);
        
        char profitStr[64];
        _snprintf_s(profitStr, sizeof(profitStr), _TRUNCATE, "%.2f USD", totalProfit);
        ListView_SetItemTextA(g_hListPositions, idx, 6, profitStr);
    }
}

void UpdateHistoryDisplay() {
    if (!g_hListHistory) return;
    
    ListView_DeleteAllItemsA(g_hListHistory);
    
    std::lock_guard<std::mutex> lock(g_dataMutex);
    
    for (size_t i = 0; i < g_tradeHistory.size() && i < 100; i++) {
        const auto& h = g_tradeHistory[g_tradeHistory.size() - 1 - i];
        
        LVITEMA lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        char pairLabel[] = "A+B";
        lvi.pszText = pairLabel;
        int idx = ListView_InsertItemA(g_hListHistory, &lvi);
        
        ListView_SetItemTextA(g_hListHistory, idx, 1, h.time.c_str());
        ListView_SetItemTextA(g_hListHistory, idx, 2, h.orderIdA.c_str());
        ListView_SetItemTextA(g_hListHistory, idx, 3, h.orderIdB.c_str());
        
        char profitStr[32];
        _snprintf_s(profitStr, sizeof(profitStr), _TRUNCATE, "%.2f", h.profit);
        ListView_SetItemTextA(g_hListHistory, idx, 4, profitStr);
        ListView_SetItemTextA(g_hListHistory, idx, 5, h.status.c_str());
    }
}

void UpdateStatusBar() {
    if (!g_hStatusBar) return;
    
    char status[512];
    const char* connectedA = g_config.brokerA.isConnected ? STR_CONNECTED() : STR_DISCONNECTED();
    const char* connectedB = g_config.brokerB.isConnected ? STR_CONNECTED() : STR_DISCONNECTED();
    const char* tradingStatus = g_running ? STR_RUNNING() : STR_STOPPED();
    _snprintf_s(status, sizeof(status), _TRUNCATE,
        STR_STATUS_FORMAT(),
        g_config.logicalSymbol.c_str(),
        g_config.brokerA.tradeSymbol.c_str(), g_config.brokerA.lots,
        connectedA,
        g_config.brokerB.tradeSymbol.c_str(), g_config.brokerB.lots,
        connectedB,
        tradingStatus);
    
    SendMessageA(g_hStatusBar, SB_SETTEXTA, 0, (LPARAM)status);
}

// ==================== 从控件读取配置 ====================
void LoadConfigFromUI() {
    char buf[256];
    
    GetWindowTextA(g_hEditLogicalSymbol, buf, sizeof(buf));
    g_config.logicalSymbol = buf;
    
    GetWindowTextA(g_hEditA_ApiKey, buf, sizeof(buf));
    g_config.brokerA.apiKey = buf;
    GetWindowTextA(g_hEditA_ApiSecret, buf, sizeof(buf));
    g_config.brokerA.apiSecret = buf;
    GetWindowTextA(g_hEditA_Symbol, buf, sizeof(buf));
    g_config.brokerA.tradeSymbol = buf;
    GetWindowTextA(g_hEditA_Lots, buf, sizeof(buf));
    g_config.brokerA.lots = atof(buf);
    g_config.brokerA.useWebSocket = (SendMessageA(g_hCheckA_WebSocket, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    GetWindowTextA(g_hEditB_ApiKey, buf, sizeof(buf));
    g_config.brokerB.apiKey = buf;
    GetWindowTextA(g_hEditB_ApiSecret, buf, sizeof(buf));
    g_config.brokerB.apiSecret = buf;
    GetWindowTextA(g_hEditB_Symbol, buf, sizeof(buf));
    g_config.brokerB.tradeSymbol = buf;
    GetWindowTextA(g_hEditB_Lots, buf, sizeof(buf));
    g_config.brokerB.lots = atof(buf);
    g_config.brokerB.useWebSocket = (SendMessageA(g_hCheckB_WebSocket, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    GetWindowTextA(g_hEditThreshold, buf, sizeof(buf));
    g_config.thresholdPips = atoi(buf);
    GetWindowTextA(g_hEditTarget, buf, sizeof(buf));
    g_config.targetPips = atoi(buf);
    GetWindowTextA(g_hEditStopLoss, buf, sizeof(buf));
    g_config.stopLossPips = atoi(buf);
    GetWindowTextA(g_hEditSlippage, buf, sizeof(buf));
    g_config.slippage = atoi(buf);
    GetWindowTextA(g_hEditCheckInterval, buf, sizeof(buf));
    g_config.checkIntervalMs = atoi(buf);
    GetWindowTextA(g_hEditMagic, buf, sizeof(buf));
    g_config.magic = atoi(buf);
}

// ==================== 将配置写入控件 ====================
void SaveConfigToUI() {
    SetWindowTextA(g_hEditLogicalSymbol, g_config.logicalSymbol.c_str());
    
    SetWindowTextA(g_hEditA_ApiKey, g_config.brokerA.apiKey.c_str());
    SetWindowTextA(g_hEditA_ApiSecret, g_config.brokerA.apiSecret.c_str());
    SetWindowTextA(g_hEditA_Symbol, g_config.brokerA.tradeSymbol.c_str());
    char lotsStr[32];
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%.2f", g_config.brokerA.lots);
    SetWindowTextA(g_hEditA_Lots, lotsStr);
    SendMessageA(g_hCheckA_WebSocket, BM_SETCHECK, g_config.brokerA.useWebSocket ? BST_CHECKED : BST_UNCHECKED, 0);
    
    SetWindowTextA(g_hEditB_ApiKey, g_config.brokerB.apiKey.c_str());
    SetWindowTextA(g_hEditB_ApiSecret, g_config.brokerB.apiSecret.c_str());
    SetWindowTextA(g_hEditB_Symbol, g_config.brokerB.tradeSymbol.c_str());
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%.2f", g_config.brokerB.lots);
    SetWindowTextA(g_hEditB_Lots, lotsStr);
    SendMessageA(g_hCheckB_WebSocket, BM_SETCHECK, g_config.brokerB.useWebSocket ? BST_CHECKED : BST_UNCHECKED, 0);
    
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.thresholdPips);
    SetWindowTextA(g_hEditThreshold, lotsStr);
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.targetPips);
    SetWindowTextA(g_hEditTarget, lotsStr);
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.stopLossPips);
    SetWindowTextA(g_hEditStopLoss, lotsStr);
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.slippage);
    SetWindowTextA(g_hEditSlippage, lotsStr);
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.checkIntervalMs);
    SetWindowTextA(g_hEditCheckInterval, lotsStr);
    _snprintf_s(lotsStr, sizeof(lotsStr), _TRUNCATE, "%d", g_config.magic);
    SetWindowTextA(g_hEditMagic, lotsStr);
}

// ==================== 窗口过程 ====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // 创建参数设置区域
        int y = 10;
        CreateWindowA("STATIC", STR_LOGIC_SYMBOL(), WS_VISIBLE | WS_CHILD, 10, y, 80, 20, hWnd, NULL, NULL, NULL);
        g_hEditLogicalSymbol = CreateWindowA("EDIT", "GOLD", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            100, y, 100, 20, hWnd, NULL, NULL, NULL);
        y += 30;
        
        // A端配置
        CreateWindowA("STATIC", STR_A_CONFIG(), WS_VISIBLE | WS_CHILD, 10, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", "API Key:", WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditA_ApiKey = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_PASSWORD,
            100, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", "API Secret:", WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditA_ApiSecret = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_PASSWORD,
            100, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", STR_TRADE_SYMBOL(), WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditA_Symbol = CreateWindowA("EDIT", "XAUUSD", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            100, y, 100, 20, hWnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", STR_LOTS(), WS_VISIBLE | WS_CHILD, 210, y, 40, 20, hWnd, NULL, NULL, NULL);
        g_hEditA_Lots = CreateWindowA("EDIT", "0.1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            250, y, 50, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        g_hCheckA_WebSocket = CreateWindowA("BUTTON", STR_USE_WEBSOCKET(), 
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            100, y, 150, 20, hWnd, NULL, NULL, NULL);
        SendMessageA(g_hCheckA_WebSocket, BM_SETCHECK, BST_CHECKED, 0);
        y += 40;
        
        // B端配置
        CreateWindowA("STATIC", STR_B_CONFIG(), WS_VISIBLE | WS_CHILD, 10, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", "API Key:", WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditB_ApiKey = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_PASSWORD,
            100, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", "API Secret:", WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditB_ApiSecret = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_PASSWORD,
            100, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", STR_TRADE_SYMBOL(), WS_VISIBLE | WS_CHILD, 20, y, 70, 20, hWnd, NULL, NULL, NULL);
        g_hEditB_Symbol = CreateWindowA("EDIT", "GOLD", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            100, y, 100, 20, hWnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", STR_LOTS(), WS_VISIBLE | WS_CHILD, 210, y, 40, 20, hWnd, NULL, NULL, NULL);
        g_hEditB_Lots = CreateWindowA("EDIT", "0.01", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            250, y, 50, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        g_hCheckB_WebSocket = CreateWindowA("BUTTON", STR_USE_WEBSOCKET(), 
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            100, y, 150, 20, hWnd, NULL, NULL, NULL);
        SendMessageA(g_hCheckB_WebSocket, BM_SETCHECK, BST_CHECKED, 0);
        y += 40;
        
        // 交易参数
        CreateWindowA("STATIC", STR_TRADE_PARAMS(), WS_VISIBLE | WS_CHILD, 10, y, 200, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", STR_OPEN_THRESHOLD(), WS_VISIBLE | WS_CHILD, 20, y, 90, 20, hWnd, NULL, NULL, NULL);
        g_hEditThreshold = CreateWindowA("EDIT", "100", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            110, y, 60, 20, hWnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", STR_CLOSE_TARGET(), WS_VISIBLE | WS_CHILD, 180, y, 90, 20, hWnd, NULL, NULL, NULL);
        g_hEditTarget = CreateWindowA("EDIT", "5", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            270, y, 60, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", STR_STOP_LOSS(), WS_VISIBLE | WS_CHILD, 20, y, 90, 20, hWnd, NULL, NULL, NULL);
        g_hEditStopLoss = CreateWindowA("EDIT", "180", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            110, y, 60, 20, hWnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", STR_SLIPPAGE(), WS_VISIBLE | WS_CHILD, 180, y, 50, 20, hWnd, NULL, NULL, NULL);
        g_hEditSlippage = CreateWindowA("EDIT", "3", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            230, y, 60, 20, hWnd, NULL, NULL, NULL);
        y += 25;
        CreateWindowA("STATIC", STR_CHECK_INTERVAL(), WS_VISIBLE | WS_CHILD, 20, y, 90, 20, hWnd, NULL, NULL, NULL);
        g_hEditCheckInterval = CreateWindowA("EDIT", "50", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            110, y, 60, 20, hWnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", "Magic:", WS_VISIBLE | WS_CHILD, 180, y, 50, 20, hWnd, NULL, NULL, NULL);
        g_hEditMagic = CreateWindowA("EDIT", "20251126", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            230, y, 100, 20, hWnd, NULL, NULL, NULL);
        y += 35;
        
        // 控制按钮
        g_hBtnSave = CreateWindowA("BUTTON", STR_SAVE_CONFIG(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, y, 80, 30, hWnd, (HMENU)1001, NULL, NULL);
        g_hBtnConnect = CreateWindowA("BUTTON", STR_CONNECT_API(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            110, y, 80, 30, hWnd, (HMENU)1002, NULL, NULL);
        g_hBtnStart = CreateWindowA("BUTTON", STR_START_TRADING(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            200, y, 80, 30, hWnd, (HMENU)1003, NULL, NULL);
        g_hBtnStop = CreateWindowA("BUTTON", STR_STOP_TRADING(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            290, y, 80, 30, hWnd, (HMENU)1004, NULL, NULL);
        g_hBtnCloseAll = CreateWindowA("BUTTON", STR_EMERGENCY_CLOSE(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            380, y, 80, 30, hWnd, (HMENU)1005, NULL, NULL);
        
        // 创建报价显示ListView
        g_hListQuotes = CreateWindowA(WC_LISTVIEWA, "", 
            WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
            10, y + 50, 600, 150, hWnd, NULL, NULL, NULL);
        
        if (g_hListQuotes) {
            ListView_SetExtendedListViewStyle(g_hListQuotes, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            LVCOLUMNA lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.iSubItem = 0; lvc.pszText = const_cast<LPSTR>(STR_BROKER()); lvc.cx = 60;
            ListView_InsertColumnA(g_hListQuotes, 0, &lvc);
            lvc.iSubItem = 1; lvc.pszText = const_cast<LPSTR>(STR_SYMBOL()); lvc.cx = 80;
            ListView_InsertColumnA(g_hListQuotes, 1, &lvc);
            lvc.iSubItem = 2; lvc.pszText = const_cast<LPSTR>(STR_BID()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListQuotes, 2, &lvc);
            lvc.iSubItem = 3; lvc.pszText = const_cast<LPSTR>(STR_ASK()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListQuotes, 3, &lvc);
            lvc.iSubItem = 4; lvc.pszText = const_cast<LPSTR>(STR_SPREAD()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListQuotes, 4, &lvc);
            lvc.iSubItem = 5; lvc.pszText = const_cast<LPSTR>(STR_UPDATE_TIME()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListQuotes, 5, &lvc);
        }
        
        // 创建持仓显示ListView
        g_hListPositions = CreateWindowA(WC_LISTVIEWA, "", 
            WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
            10, y + 210, 600, 150, hWnd, NULL, NULL, NULL);
        
        if (g_hListPositions) {
            ListView_SetExtendedListViewStyle(g_hListPositions, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            LVCOLUMNA lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.iSubItem = 0; lvc.pszText = const_cast<LPSTR>(STR_BROKER()); lvc.cx = 60;
            ListView_InsertColumnA(g_hListPositions, 0, &lvc);
            lvc.iSubItem = 1; lvc.pszText = const_cast<LPSTR>(STR_ORDER_ID()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListPositions, 1, &lvc);
            lvc.iSubItem = 2; lvc.pszText = const_cast<LPSTR>(STR_TYPE()); lvc.cx = 60;
            ListView_InsertColumnA(g_hListPositions, 2, &lvc);
            lvc.iSubItem = 3; lvc.pszText = const_cast<LPSTR>(STR_LOTS()); lvc.cx = 60;
            ListView_InsertColumnA(g_hListPositions, 3, &lvc);
            lvc.iSubItem = 4; lvc.pszText = const_cast<LPSTR>(STR_OPEN_PRICE()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListPositions, 4, &lvc);
            lvc.iSubItem = 5; lvc.pszText = const_cast<LPSTR>(STR_CURRENT_PRICE()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListPositions, 5, &lvc);
            lvc.iSubItem = 6; lvc.pszText = const_cast<LPSTR>(STR_PROFIT()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListPositions, 6, &lvc);
        }
        
        // 创建历史显示ListView
        g_hListHistory = CreateWindowA(WC_LISTVIEWA, "", 
            WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
            620, 10, 600, 350, hWnd, NULL, NULL, NULL);
        
        if (g_hListHistory) {
            ListView_SetExtendedListViewStyle(g_hListHistory, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            LVCOLUMNA lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.iSubItem = 0; lvc.pszText = const_cast<LPSTR>(STR_BROKER()); lvc.cx = 60;
            ListView_InsertColumnA(g_hListHistory, 0, &lvc);
            lvc.iSubItem = 1; lvc.pszText = const_cast<LPSTR>(STR_TIME()); lvc.cx = 150;
            ListView_InsertColumnA(g_hListHistory, 1, &lvc);
            lvc.iSubItem = 2; lvc.pszText = const_cast<LPSTR>(STR_ORDER_ID_A()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListHistory, 2, &lvc);
            lvc.iSubItem = 3; lvc.pszText = const_cast<LPSTR>(STR_ORDER_ID_B()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListHistory, 3, &lvc);
            lvc.iSubItem = 4; lvc.pszText = const_cast<LPSTR>(STR_PROFIT()); lvc.cx = 100;
            ListView_InsertColumnA(g_hListHistory, 4, &lvc);
            lvc.iSubItem = 5; lvc.pszText = const_cast<LPSTR>(STR_STATUS()); lvc.cx = 80;
            ListView_InsertColumnA(g_hListHistory, 5, &lvc);
        }
        
        // 创建状态栏
        g_hStatusBar = CreateWindowA(STATUSCLASSNAMEA, "", 
            WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
            0, 0, 0, 0, hWnd, NULL, NULL, NULL);
        
        // 初始化配置显示
        SaveConfigToUI();
        
        // 启动定时器
        SetTimer(hWnd, 1, 500, NULL);
        }
        break;
        
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1001) {  // 保存配置
            LoadConfigFromUI();
            MessageBoxA(hWnd, STR_CONFIG_SAVED(), STR_INFO(), MB_OK);
        } else if (id == 1002) {  // 连接API
            LoadConfigFromUI();
            bool successA = InitBrokerAPI(g_config.brokerA, STR_A_END());
            bool successB = InitBrokerAPI(g_config.brokerB, STR_B_END());
            if (successA && successB) {
                MessageBoxA(hWnd, STR_API_CONNECTED(), STR_INFO(), MB_OK);
            } else {
                char msg[256];
                const char* statusA = successA ? STR_SUCCESS() : STR_FAILED();
                const char* statusB = successB ? STR_SUCCESS() : STR_FAILED();
                _snprintf_s(msg, sizeof(msg), _TRUNCATE, 
                    STR_CONNECT_FAILED_FORMAT(),
                    statusA,
                    statusB);
                MessageBoxA(hWnd, msg, STR_ERROR(), MB_OK | MB_ICONERROR);
            }
            UpdateStatusBar();
        } else if (id == 1003) {  // 启动交易
            if (!g_config.brokerA.isConnected || !g_config.brokerB.isConnected) {
                MessageBoxA(hWnd, STR_PLEASE_CONNECT(), STR_INFO(), MB_OK | MB_ICONWARNING);
                break;
            }
            if (!g_running) {
                g_running = true;
                g_tradingThread = new std::thread(TradingThread);
                MessageBoxA(hWnd, STR_TRADING_STARTED(), STR_INFO(), MB_OK);
            }
            UpdateStatusBar();
        } else if (id == 1004) {  // 停止交易
            if (g_running) {
                g_running = false;
                if (g_tradingThread) {
                    g_tradingThread->join();
                    delete g_tradingThread;
                    g_tradingThread = NULL;
                }
                MessageBoxA(hWnd, STR_TRADING_STOPPED(), STR_INFO(), MB_OK);
            }
            UpdateStatusBar();
        } else if (id == 1005) {  // 紧急平仓
            ClosePosition(g_config.brokerA, g_positionA);
            ClosePosition(g_config.brokerB, g_positionB);
            MessageBoxA(hWnd, STR_EMERGENCY_EXECUTED(), STR_INFO(), MB_OK);
        }
        }
        break;
        
    case WM_TIMER:
        UpdateQuoteDisplay();
        UpdatePositionDisplay();
        UpdateHistoryDisplay();
        UpdateStatusBar();
        break;
        
    case WM_SIZE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        if (g_hStatusBar) {
            SendMessageA(g_hStatusBar, WM_SIZE, 0, 0);
        }
        }
        break;
        
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        g_running = false;
        if (g_tradingThread) {
            g_tradingThread->join();
            delete g_tradingThread;
        }
        if (g_config.brokerA.client) {
            if (g_config.brokerA.useWebSocket) g_config.brokerA.client->DisconnectWebSocket();
            delete g_config.brokerA.client;
        }
        if (g_config.brokerB.client) {
            if (g_config.brokerB.useWebSocket) g_config.brokerB.client->DisconnectWebSocket();
            delete g_config.brokerB.client;
        }
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ==================== 主函数 ====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "HedgeArbDirectAPI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, STR_WINDOW_REG_FAILED(), STR_ERROR(), MB_OK);
        return 0;
    }
    
    g_hWnd = CreateWindowA("HedgeArbDirectAPI", STR_WINDOW_TITLE(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1250, 800,
        NULL, NULL, hInstance, NULL);
    
    if (!g_hWnd) {
        MessageBoxA(NULL, STR_WINDOW_CREATE_FAILED(), STR_ERROR(), MB_OK);
        return 0;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    
    return (int)msg.wParam;
}
