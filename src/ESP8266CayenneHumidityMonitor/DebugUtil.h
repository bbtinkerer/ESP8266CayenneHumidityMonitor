#ifndef DEBUGUTIL_H
#define DEBUGUTIL_H

#ifdef DEBUG
  #define DEBUG_UTIL_BEGIN       Serial.begin(115200); while(!Serial) { }
  #define DEBUG_UTIL_PRINT(x)    Serial.print(x)
  #define DEBUG_UTIL_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_UTIL_BEGIN
  #define DEBUG_UTIL_PRINT(x)
  #define DEBUG_UTIL_PRINTLN(x)
#endif

#endif
