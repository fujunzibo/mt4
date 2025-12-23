#ifndef HEDGE_ARB_SHARED_MEMORY_H
#define HEDGE_ARB_SHARED_MEMORY_H

#include <windows.h>
#include <string>

// 共享内存数据结构
struct SharedData {
    // 结构版本与签名（用于检测旧数据 / 不兼容布局）
    char signature[8];      // 固定写入 "HEDGEV1"
    int  structVersion;     // 当前结构版本号，例如 1

    // 报价数据
    struct Quote {
        double bid;
        double ask;
        unsigned long long timestamp;  // 时间戳（毫秒）
        bool isValid;
    } quotes[2];  // [0]=A端, [1]=B端

    // 盈亏数据
    struct Profit {
        double profitPips;
        double profitUSD;
        unsigned long long timestamp;
        bool isValid;
    } profits[2];

    // 指令数据
    struct Command {
        char code[16];      // "OPEN", "CLOSEALL", ""
        char longBroker[4];  // "A" or "B"
        char shortBroker[4];
        unsigned long long timestamp;
        bool isActive;
    } command;

    // 确认数据
    struct Ack {
        char code[16];      // "SUCCESS", "FAILED", ""
        unsigned long long timestamp;
        bool isValid;
    } acks[2];

    // 同步标志
    volatile long version;  // 每次更新递增，用于检测数据变化
    volatile long lockFlag; // 预留锁标记（暂时不使用）
};

// DLL导出函数声明（供 MQL4 / GUI / 控制台程序调用）
extern "C" {
    __declspec(dllexport) bool InitSharedMemory(const char* symbol);
    __declspec(dllexport) bool SetQuote(const char* brokerId, double bid, double ask);
    __declspec(dllexport) bool GetQuote(const char* brokerId, double* bid, double* ask);
    __declspec(dllexport) bool SetProfit(const char* brokerId, double profitPips, double profitUSD);
    __declspec(dllexport) bool GetProfit(const char* brokerId, double* profitPips, double* profitUSD);
    __declspec(dllexport) bool SetCommand(const char* code, const char* longBroker, const char* shortBroker);
    __declspec(dllexport) bool GetCommand(char* code, char* longBroker, char* shortBroker, unsigned long long* timestamp, bool* isActive);
    __declspec(dllexport) bool ClearCommand();
    __declspec(dllexport) bool SetAck(const char* brokerId, const char* ackCode);
    __declspec(dllexport) bool GetAck(const char* brokerId, char* ackCode, unsigned long long* timestamp);
    __declspec(dllexport) bool ClearAck(const char* brokerId);
    __declspec(dllexport) void CloseSharedMemory();
}

#endif // HEDGE_ARB_SHARED_MEMORY_H
