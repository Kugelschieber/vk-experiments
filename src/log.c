#include "log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

int logLevel = 0;

char* timeNow() {
    time_t now = time(NULL);
    struct tm * p = localtime(&now);
    static char buffer[21];
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", p);
    buffer[20] = '\0';
    return buffer;
}

void vkeSetLogLevel(int level) {
    logLevel = level;
}

void vkeLogDebug(const char* message) {
    if(logLevel < VKE_LOG_INFO) {
        printf("%s [DEBUG] %s\n", timeNow(), message);
    }
}

void vkeLogInfo(const char* message) {
    if(logLevel < VKE_LOG_WARN) {
        printf("%s [INFO] %s\n", timeNow(), message);
    }
}

void vkeLogWarn(const char* message) {
    if(logLevel < VKE_LOG_ERROR) {
        printf("%s [WARN] %s\n", timeNow(), message);
    }
}

void vkeLogError(const char* message) {
    if(logLevel < VKE_LOG_ERROR+1) {
        printf("%s [ERROR] %s\n", timeNow(), message);
    }
}
