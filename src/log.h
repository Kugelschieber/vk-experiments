#ifndef VKE_LOG_H
#define VKE_LOG_H

#define VKE_LOG_DEBUG = 0
#define VKE_LOG_ERROR = 1

// Sets the global log level (VKE_LOG_DEBUG by default).
void vkeSetLogLevel(int level);

// Logs the given message with the DEBUG tag.
void vkeLogDebug(const char* message);

// Logs the given message with the ERROR tag.
void vkeLogError(const char* message);

#endif
