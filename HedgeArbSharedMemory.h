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
    volatile long lockFlag; // 简单的自旋锁（0=未锁定，1=已锁定）
};

// DLL导出函数声明（供MQL4 / GUI / 控制台程序调用）
extern "C" {
    // 初始化共享内存连接
    __declspec(dllexport) bool InitSharedMemory(const char* symbol);
    
    // 设置报价（brokerId: "A" or "B"）
    __declspec(dllexport) bool SetQuote(const char* brokerId, double bid, double ask);
    
    // 获取报价（brokerId: "A" or "B"）
    __declspec(dllexport) bool GetQuote(const char* brokerId, double* bid, double* ask);
    
    // 设置盈亏
    __declspec(dllexport) bool SetProfit(const char* brokerId, double profitPips, double profitUSD);
    
    // 获取盈亏
    __declspec(dllexport) bool GetProfit(const char* brokerId, double* profitPips, double* profitUSD);
    
    // 设置指令
    __declspec(dllexport) bool SetCommand(const char* code, const char* longBroker, const char* shortBroker);
    
    // 获取指令
    __declspec(dllexport) bool GetCommand(char* code, char* longBroker, char* shortBroker, unsigned long long* timestamp, bool* isActive);
    
    // 清除指令
    __declspec(dllexport) bool ClearCommand();
    
    // 设置确认
    __declspec(dllexport) bool SetAck(const char* brokerId, const char* ackCode);
    
    // 获取确认
    __declspec(dllexport) bool GetAck(const char* brokerId, char* ackCode, unsigned long long* timestamp);
    
    // 清除确认
    __declspec(dllexport) bool ClearAck(const char* brokerId);
    
    // 关闭共享内存连接
    __declspec(dllexport) void CloseSharedMemory();
    
    // === 诊断辅助接口 ===
    // 重置共享内存（在持有锁的前提下清空 SharedData，并重新写入签名/版本）
    __declspec(dllexport) void ResetSharedMemory();
    
    // 将当前 SharedData 的关键字段以文本形式追加写入调试日志文件（例如 D:\mt4\SharedMemQuoteDebug.log）
    __declspec(dllexport) void DumpSharedData();
}

#endif // HEDGE_ARB_SHARED_MEMORY_H


