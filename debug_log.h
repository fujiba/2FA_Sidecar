#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#ifdef DEBUG_LOG_ENABLED
#define D_BEGIN(...) Serial.begin(__VA_ARGS__)
#define D_PRINT(...) Serial.print(__VA_ARGS__)
#define D_PRINTLN(...) Serial.println(__VA_ARGS__)
#define D_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define D_BEGIN(...)
#define D_PRINT(...)
#define D_PRINTLN(...)
#define D_PRINTF(...)
#endif

#endif  // DEBUG_LOG_H