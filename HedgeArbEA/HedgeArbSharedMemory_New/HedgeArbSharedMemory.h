#ifndef HEDGE_ARB_SHARED_MEMORY_H
#define HEDGE_ARB_SHARED_MEMORY_H

#include <windows.h>
#include <string>

// Shared memory data structure
#pragma pack(push, 1)  // Ensure consistent packing
struct SharedData {
    // Structure version/signature (for validation / compatibility check)
    char signature[8];      // Fixed value "HEDGEV1"
    int  structVersion;     // Current structure version number, currently 1

    // Quote data
    struct Quote {
        double bid;
        double ask;
        unsigned long long timestamp;  // Timestamp in milliseconds
        bool isValid;
    } quotes[2];  // [0]=Broker A, [1]=Broker B

    // Profit data
    struct Profit {
        double profitPips;
        double profitUSD;
        unsigned long long timestamp;
        bool isValid;
    } profits[2];

    // Command data
    struct Command {
        char code[16];      // "OPEN", "CLOSEALL", ""
        char longBroker[4];  // "A" or "B"
        char shortBroker[4];
        unsigned long long timestamp;
        bool isActive;
    } command;

    // Acknowledgment data
    struct Ack {
        char code[16];      // "SUCCESS", "FAILED", ""
        unsigned long long timestamp;
        bool isValid;
    } acks[2];

    // Synchronization flags
    volatile long version;  // Incremented on each update, used to detect data changes
    volatile long lockFlag; // Reserved for future use, currently unused
};
#pragma pack(pop)

// DLL exported functions (used by MQL4 / GUI / other programs)
extern "C" {
    __declspec(dllexport) bool InitSharedMemory(const char* symbol);
    __declspec(dllexport) bool SetQuote(const char* brokerId, double bid, double ask);
    __declspec(dllexport) bool GetQuote(const char* brokerId, double* bid, double* ask);
    __declspec(dllexport) bool SetProfit(const char* brokerId, double profitPips, double profitUSD);
    __declspec(dllexport) bool GetProfit(const char* brokerId, double* profitPips, double* profitUSD);
    __declspec(dllexport) bool SetCommand(const char* code, const char* longBroker, const char* shortBroker);
    __declspec(dllexport) bool GetCommand(char* code, char* longBroker, char* shortBroker, long* timestamp, bool* isActive);
    __declspec(dllexport) bool ClearCommand();
    __declspec(dllexport) bool SetAck(const char* brokerId, const char* ackCode);
    __declspec(dllexport) bool GetAck(const char* brokerId, char* ackCode, long* timestamp);
    __declspec(dllexport) bool ClearAck(const char* brokerId);
    __declspec(dllexport) void CloseSharedMemory();
}

#endif // HEDGE_ARB_SHARED_MEMORY_H
