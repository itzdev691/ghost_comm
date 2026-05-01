#ifndef CRASH_LOGGER_H
#define CRASH_LOGGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_system.h>
// #include <esp_task_wdt.h>  // REMOVE THIS LINE

#define CRASH_LOG_FILE "/crash_log.txt"
#define MAX_LOG_SIZE 10000

class CrashLogger {
public:
    void begin();
    void logCrash(const char* reason, const char* details = "");
    void checkPreviousCrashes();
    void printCrashLog();
    void clearLogs();
    
private:
    void rotateLogs();
};

#endif