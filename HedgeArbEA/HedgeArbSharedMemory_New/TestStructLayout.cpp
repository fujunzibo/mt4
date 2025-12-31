// Test program to verify Command struct layout
#include "HedgeArbSharedMemory.h"
#include <stdio.h>

int main() {
    printf("Command struct size: %zu bytes\n", sizeof(SharedData::Command));
    printf("  code[16]: offset=%zu, size=%zu\n", 
           offsetof(SharedData::Command, code), 
           sizeof(((SharedData::Command*)0)->code));
    printf("  longBroker[4]: offset=%zu, size=%zu\n", 
           offsetof(SharedData::Command, longBroker), 
           sizeof(((SharedData::Command*)0)->longBroker));
    printf("  shortBroker[4]: offset=%zu, size=%zu\n", 
           offsetof(SharedData::Command, shortBroker), 
           sizeof(((SharedData::Command*)0)->shortBroker));
    printf("  timestamp: offset=%zu, size=%zu\n", 
           offsetof(SharedData::Command, timestamp), 
           sizeof(((SharedData::Command*)0)->timestamp));
    printf("  isActive: offset=%zu, size=%zu\n", 
           offsetof(SharedData::Command, isActive), 
           sizeof(((SharedData::Command*)0)->isActive));
    
    // Verify field boundaries
    SharedData::Command cmd;
    memset(&cmd, 0, sizeof(cmd));
    
    // Test: Write to code field
    strncpy_s(cmd.code, sizeof(cmd.code), "OPEN", _TRUNCATE);
    printf("\nTest: Write 'OPEN' to code field\n");
    printf("  code='%s' (len=%zu)\n", cmd.code, strlen(cmd.code));
    printf("  longBroker='%.4s' (should be empty)\n", cmd.longBroker);
    
    // Test: Write to longBroker field
    memset(&cmd, 0, sizeof(cmd));
    strncpy_s(cmd.longBroker, sizeof(cmd.longBroker), "A", _TRUNCATE);
    printf("\nTest: Write 'A' to longBroker field\n");
    printf("  code='%.16s' (should be empty)\n", cmd.code);
    printf("  longBroker='%s' (len=%zu)\n", cmd.longBroker, strlen(cmd.longBroker));
    
    // Test: Write both
    memset(&cmd, 0, sizeof(cmd));
    strncpy_s(cmd.code, sizeof(cmd.code), "OPEN", _TRUNCATE);
    strncpy_s(cmd.longBroker, sizeof(cmd.longBroker), "A", _TRUNCATE);
    strncpy_s(cmd.shortBroker, sizeof(cmd.shortBroker), "B", _TRUNCATE);
    printf("\nTest: Write all fields\n");
    printf("  code='%s'\n", cmd.code);
    printf("  longBroker='%s'\n", cmd.longBroker);
    printf("  shortBroker='%s'\n", cmd.shortBroker);
    
    return 0;
}


