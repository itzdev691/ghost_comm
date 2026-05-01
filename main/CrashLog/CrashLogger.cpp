#include "CrashLogger.h"

void CrashLogger::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    checkPreviousCrashes();
}

void CrashLogger::logCrash(const char* reason, const char* details) {
    File file = LittleFS.open(CRASH_LOG_FILE, FILE_APPEND);
    if (!file) return;
    
    file.printf("\n=== CRASH REPORT ===\n");
    file.printf("Time: %lu ms\n", millis());
    file.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    file.printf("Reason: %s\n", reason);
    if (details && details[0] != '\0') {
        file.printf("Details: %s\n", details);
    }
    file.printf("Reset Reason: %d\n", esp_reset_reason());
    file.printf("====================\n");
    file.close();
    
    rotateLogs();
}

void CrashLogger::checkPreviousCrashes() {
    esp_reset_reason_t reason = esp_reset_reason();
    
    const char* resetReasons[] = {
        "Unknown", "Power-on", "External pin", "Software", 
        "Panic", "Interrupt WDT", "Task WDT", "Other WDT",
        "Deep sleep", "Brownout", "SDIO"
    };
    
    Serial.printf("Reset reason: %s (%d)\n", 
                 resetReasons[reason], reason);
    
    if (reason == ESP_RST_PANIC || 
        reason == ESP_RST_INT_WDT || 
        reason == ESP_RST_TASK_WDT) {
        logCrash("Unexpected reset", resetReasons[reason]);
    }
    
    printCrashLog();
}

void CrashLogger::printCrashLog() {
    if (!LittleFS.exists(CRASH_LOG_FILE)) {
        Serial.println("No crash log found");
        return;
    }
    
    File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    if (!file) {
        Serial.println("Failed to open crash log");
        return;
    }
    
    Serial.println("\n--- Previous Crash Logs ---");
    
    // Read in chunks with yielding
    uint8_t buffer[256];
    size_t bytesRead;
    
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        Serial.write(buffer, bytesRead);
        delay(1);  // Yield to other tasks
    }
    
    file.close();
    Serial.println("--- End of Logs ---\n");
}

void CrashLogger::clearLogs() {
    LittleFS.remove(CRASH_LOG_FILE);
    Serial.println("Crash logs cleared");
}

void CrashLogger::rotateLogs() {
    File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    if (!file) return;
    
    size_t size = file.size();
    file.close();
    
    if (size > MAX_LOG_SIZE) {
        File oldFile = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
        File newFile = LittleFS.open("/crash_log_tmp.txt", FILE_WRITE);
        
        if (oldFile && newFile) {
            oldFile.seek(size - MAX_LOG_SIZE/2);
            
            // Use chunked I/O with yielding
            uint8_t buffer[256];
            size_t bytesRead;
            
            while ((bytesRead = oldFile.read(buffer, sizeof(buffer))) > 0) {
                newFile.write(buffer, bytesRead);
                delay(1);  // Yield
            }
            
            oldFile.close();
            newFile.close();
            LittleFS.remove(CRASH_LOG_FILE);
            LittleFS.rename("/crash_log_tmp.txt", CRASH_LOG_FILE);
        }
    }
}