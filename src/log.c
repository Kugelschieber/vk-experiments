#include "log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

int logLevel = 0;

// TODO
/*const char* timeStr() {
    time_t now = time(NULL);
    struct tm * p = localtime(&now);
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%d %H:%M:%S", p);
    //buffer[24] = '\0';
    const char* str = buffer;
    return str;
}*/

void vkeSetLogLevel(int level) {
    logLevel = level;
}

void vkeLogDebug(const char* message) {
    if(logLevel == 0) {
        time_t now;
        time(&now);
        printf("%s [DEBUG] %s\n", ctime(&now), message);
    }
}

void vkeLogError(const char* message) {
    if(logLevel > 0) {
        time_t now;
        time(&now);
        printf("%s [ERROR] %s\n", ctime(&now), message);
    }
}
