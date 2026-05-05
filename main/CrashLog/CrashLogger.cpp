#include "CrashLogger.h"
#include <esp_system.h>

// ---------------------------------------------------------------------------
// Static member definition
// ---------------------------------------------------------------------------
bool CrashLogger::s_fsReady = false;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
 * @brief Reads the most recent "Details:" value from the crash log file.
 *
 * Scans every line of the log and returns the last line that starts with
 * "Details: ", stripping the prefix. Returns an empty string if the file
 * does not exist, cannot be opened, or contains no such line.
 *
 * @return String The last recorded details value, or "" if none found.
 */
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

/**
 * @brief Reads the repeat counter for the current crash reason from the
 *        counter file.
 *
 * The counter file stores a single integer representing how many times the
 * most-recently logged crash has been seen consecutively. Returns 0 if the
 * file does not exist or cannot be opened.
 *
 * @return uint32_t The current repeat count, or 0 if unavailable.
 */
static uint32_t readRepeatCount() {
    if (!LittleFS.exists(CRASH_COUNT_FILE)) return 0;
    File f = LittleFS.open(CRASH_COUNT_FILE, FILE_READ);
    if (!f) return 0;
    uint32_t count = f.parseInt();
    f.close();
    return count;
}

/**
 * @brief Writes (overwrites) the repeat counter file with the given count.
 *
 * Creates or truncates the counter file and writes @p count as a decimal
 * string. Silently returns if the file cannot be opened.
 *
 * @param count The repeat count value to persist.
 */
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
/**
 * @brief ESP-IDF shutdown/panic handler called just before the chip resets.
 *
 * Appends a minimal crash record (uptime, free heap, reset reason) to the
 * log file. Intentionally kept short to reduce the risk of a secondary fault
 * during an already-unstable crash state. Does nothing if the filesystem was
 * never successfully mounted (s_fsReady == false).
 */
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
/**
 * @brief Initialises the CrashLogger subsystem.
 *
 * Mounts LittleFS (formatting on first use if necessary), marks the
 * filesystem as ready, registers panicHandler() as an ESP-IDF shutdown
 * handler so it runs on the next crash, and then calls
 * checkPreviousCrashes() to evaluate and log the current boot's reset
 * reason. Must be called once during setup() before any other CrashLogger
 * methods.
 */
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
/**
 * @brief Appends a crash entry to the log, deduplicating consecutive repeats.
 *
 * Compares @p details against the last recorded "Details:" value. If they
 * match, the repeat counter is incremented instead of writing a new block,
 * keeping the log compact. If the crash is new or different, a full entry
 * (uptime, free heap, reason, details) is appended and the counter file is
 * removed. rotateLogs() is called afterwards to enforce the size limit.
 *
 * @param reason  Short human-readable label for the crash category
 *                (e.g. "Unexpected reset").
 * @param details Specific detail string (e.g. the reset-reason name).
 *                May be nullptr or empty.
 */
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
/**
 * @brief Evaluates the current boot's reset reason and logs it if abnormal.
 *
 * Reads esp_reset_reason() and maps it to a human-readable string. Abnormal
 * resets (panic, watchdog, brownout, unknown) are forwarded to logCrash().
 * Normal resets (power-on, external pin, software, deep-sleep) are silently
 * ignored. Always calls printCrashLog() at the end so the full log is echoed
 * to Serial on every boot.
 */
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
/**
 * @brief Dumps the entire crash log file to Serial.
 *
 * Reads the log in 256-byte chunks and writes them directly to Serial,
 * followed by the repeat count (if non-zero). Prints a "No crash log found"
 * message if the file does not exist, and a failure message if it cannot be
 * opened. Does nothing if the filesystem is not ready.
 */
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
/**
 * @brief Deletes both the crash log file and the repeat counter file.
 *
 * Removes CRASH_LOG_FILE and CRASH_COUNT_FILE from LittleFS and prints a
 * confirmation to Serial. Does nothing if the filesystem is not ready.
 */
void CrashLogger::clearLogs() {
    if (!s_fsReady) return;
    LittleFS.remove(CRASH_LOG_FILE);
    LittleFS.remove(CRASH_COUNT_FILE);
    Serial.println("[CrashLogger] Crash logs cleared");
}

// ---------------------------------------------------------------------------
// rotateLogs()
// ---------------------------------------------------------------------------
/**
 * @brief Trims the crash log file when it exceeds MAX_LOG_SIZE bytes.
 *
 * If the log file is within the size limit, returns immediately. Otherwise,
 * copies the most recent MAX_LOG_SIZE/2 bytes into a temporary file, removes
 * the original, and renames the temporary file back to CRASH_LOG_FILE. This
 * keeps the newest entries while bounding flash usage. Does nothing if the
 * filesystem is not ready or the file cannot be opened.
 */
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
