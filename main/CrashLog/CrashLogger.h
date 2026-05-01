#ifndef CRASH_LOGGER_H
#define CRASH_LOGGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_system.h>

#define CRASH_LOG_FILE   "/crash_log.txt"
#define CRASH_COUNT_FILE "/crash_count.txt"
#define MAX_LOG_SIZE 10000

class CrashLogger {
public:
    void begin();
    void logCrash(const char* reason, const char* details = "");
    void checkPreviousCrashes();
    void printCrashLog();
    void clearLogs();

    // Called by the panic handler — must be static so it can be used as a
    // plain C function pointer. Writes directly to flash without heap allocation.
    static void panicHandler();

private:
    void rotateLogs();

    // Shared LittleFS-mounted flag so panicHandler knows if it's safe to write.
    static bool s_fsReady;
};

#endif
