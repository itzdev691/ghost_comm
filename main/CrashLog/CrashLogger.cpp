#include "CrashLogger.h"
#include <esp_system.h>

// ---------------------------------------------------------------------------
// Static member definition
// ---------------------------------------------------------------------------
bool CrashLogger::s_fsReady = false;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Returns the last "Details: <X>" value written to the log, or empty string.
static String readLastDetails() {
    if (!LittleFS.exists(CRASH_LOG_FILE)) return "";

    File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    if (!file) return "";

    // Walk through all lines, keep the most recent "Details:" line
    String lastDetails = "";
    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("Details: ")) {
            lastDetails = line.substring(9);  // everything after "Details: "
        }
    }
    file.close();
    return lastDetails;
}

// Read the repeat counter for the current crash reason. Returns 0 if none.
static uint32_t readRepeatCount() {
    if (!LittleFS.exists(CRASH_COUNT_FILE)) return 0;
    File f = LittleFS.open(CRASH_COUNT_FILE, FILE_READ);
    if (!f) return 0;
    uint32_t count = f.parseInt();
    f.close();
    return count;
}

// Write (overwrite) the repeat counter file.
static void writeRepeatCount(uint32_t count) {
    File f = LittleFS.open(CRASH_COUNT_FILE, FILE_WRITE);
    if (!f) return;
    f.print(count);
    f.close();
}

// ---------------------------------------------------------------------------
// Panic handler — registered with ESP-IDF's shutdown handler mechanism.
// Runs during a crash, before the chip resets. Keep it minimal.
// ---------------------------------------------------------------------------
void CrashLogger::panicHandler() {
    if (!s_fsReady) return;

    File file = LittleFS.open(CRASH_LOG_FILE, FILE_APPEND);
    if (!file) return;

    file.printf("\n=== PANIC HANDLER CRASH ===\n");
    file.printf("Uptime: %lu ms\n", millis());
    file.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    file.printf("Reset Reason: %d\n", (int)esp_reset_reason());
    file.printf("===========================\n");
    file.close();
}

// ---------------------------------------------------------------------------
// begin() — mount FS, register panic handler, check last boot's reset reason
// ---------------------------------------------------------------------------
void CrashLogger::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[CrashLogger] LittleFS mount failed");
        return;
    }
    s_fsReady = true;

    esp_err_t err = esp_register_shutdown_handler(panicHandler);
    if (err != ESP_OK) {
        Serial.printf("[CrashLogger] Warning: shutdown handler registration failed (%d)\n", err);
    }

    checkPreviousCrashes();
}

// ---------------------------------------------------------------------------
// logCrash() — append or deduplicate a crash entry.
//
// If the last logged crash has the same reason as this one, we don't write a
// new block — we just increment the repeat counter file instead. The display
// reads that counter and shows "x<N>" next to the entry.
// ---------------------------------------------------------------------------
void CrashLogger::logCrash(const char* reason, const char* details) {
    if (!s_fsReady) return;

    String lastDetails = readLastDetails();
    String thisDetails = details ? details : "";

    if (lastDetails.length() > 0 && lastDetails == thisDetails) {
        // Same crash reason as last time — just bump the counter
        uint32_t count = readRepeatCount();
        writeRepeatCount(count + 1);
        Serial.printf("[CrashLogger] Repeated crash (%s), count now %lu\n",
                      details, (unsigned long)(count + 1));
        return;
    }

    // New or different crash — write a full entry and reset the counter
    LittleFS.remove(CRASH_COUNT_FILE);

    File file = LittleFS.open(CRASH_LOG_FILE, FILE_APPEND);
    if (!file) return;

    file.printf("\n=== CRASH REPORT ===\n");
    file.printf("Uptime: %lu ms\n", millis());
    file.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    file.printf("Reason: %s\n", reason);
    if (details && details[0] != '\0') {
        file.printf("Details: %s\n", details);
    }
    file.printf("====================\n");
    file.close();

    rotateLogs();
}

// ---------------------------------------------------------------------------
// checkPreviousCrashes()
// ---------------------------------------------------------------------------
void CrashLogger::checkPreviousCrashes() {
    esp_reset_reason_t reason = esp_reset_reason();

    const char* resetReasons[] = {
        "Unknown",       // ESP_RST_UNKNOWN   0
        "Power-on",      // ESP_RST_POWERON   1
        "External pin",  // ESP_RST_EXT       2
        "Software",      // ESP_RST_SW        3
        "Panic",         // ESP_RST_PANIC     4
        "Interrupt WDT", // ESP_RST_INT_WDT   5
        "Task WDT",      // ESP_RST_TASK_WDT  6
        "Other WDT",     // ESP_RST_WDT       7
        "Deep sleep",    // ESP_RST_DEEPSLEEP 8
        "Brownout",      // ESP_RST_BROWNOUT  9
        "SDIO",          // ESP_RST_SDIO      10
    };

    const char* reasonStr = (reason < 11) ? resetReasons[reason] : "Out-of-range";
    Serial.printf("[CrashLogger] Reset reason: %s (%d)\n", reasonStr, (int)reason);

    switch (reason) {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
        case ESP_RST_BROWNOUT:
        case ESP_RST_UNKNOWN:
            logCrash("Unexpected reset", reasonStr);
            break;

        case ESP_RST_POWERON:
        case ESP_RST_EXT:
        case ESP_RST_SW:
        case ESP_RST_DEEPSLEEP:
        default:
            break;
    }

    printCrashLog();
}

// ---------------------------------------------------------------------------
// printCrashLog()
// ---------------------------------------------------------------------------
void CrashLogger::printCrashLog() {
    if (!s_fsReady || !LittleFS.exists(CRASH_LOG_FILE)) {
        Serial.println("[CrashLogger] No crash log found");
        return;
    }

    File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    if (!file) {
        Serial.println("[CrashLogger] Failed to open crash log");
        return;
    }

    Serial.println("\n--- Previous Crash Logs ---");
    uint8_t buffer[256];
    size_t bytesRead;
    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        Serial.write(buffer, bytesRead);
        delay(1);
    }
    file.close();

    uint32_t repeats = readRepeatCount();
    if (repeats > 0) {
        Serial.printf("(above crash repeated %lu more time(s))\n", (unsigned long)repeats);
    }

    Serial.println("\n--- End of Logs ---\n");
}

// ---------------------------------------------------------------------------
// clearLogs()
// ---------------------------------------------------------------------------
void CrashLogger::clearLogs() {
    if (!s_fsReady) return;
    LittleFS.remove(CRASH_LOG_FILE);
    LittleFS.remove(CRASH_COUNT_FILE);
    Serial.println("[CrashLogger] Crash logs cleared");
}

// ---------------------------------------------------------------------------
// rotateLogs()
// ---------------------------------------------------------------------------
void CrashLogger::rotateLogs() {
    if (!s_fsReady) return;

    File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    if (!file) return;

    size_t size = file.size();
    file.close();

    if (size <= MAX_LOG_SIZE) return;

    File oldFile = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
    File newFile = LittleFS.open("/crash_log_tmp.txt", FILE_WRITE);

    if (oldFile && newFile) {
        oldFile.seek(size - MAX_LOG_SIZE / 2);

        uint8_t buffer[256];
        size_t bytesRead;
        while ((bytesRead = oldFile.read(buffer, sizeof(buffer))) > 0) {
            newFile.write(buffer, bytesRead);
            delay(1);
        }

        oldFile.close();
        newFile.close();
        LittleFS.remove(CRASH_LOG_FILE);
        LittleFS.rename("/crash_log_tmp.txt", CRASH_LOG_FILE);
    }
}
