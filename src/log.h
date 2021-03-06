#ifndef VKE_LOG_H
#define VKE_LOG_H

#define VKE_LOG_DEBUG 0
#define VKE_LOG_INFO 1
#define VKE_LOG_WARN 2
#define VKE_LOG_ERROR 3

// Sets the global log level (VKE_LOG_DEBUG by default).
void vkeSetLogLevel(int level);

// Logs the given message with the DEBUG tag.
void vkeLogDebug(const char* message);

// Logs the given message with the INFO tag.
void vkeLogInfo(const char* message);

// Logs the given message with the WARN tag.
void vkeLogWarn(const char* message);

// Logs the given message with the ERROR tag.
void vkeLogError(const char* message);

#endif
