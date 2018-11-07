#ifndef SERIAL_HELPER_H
#define SERIAL_HELPER_H

#ifdef DEBUG_OFF

#define sbegin(baud)       // ignored
#define sprint(message)       // ignored
#define sprintln(message)     // ignored

#else

#define sbegin(baud)          Serial.begin(baud); \
  while(!Serial) { \
    ;\
  };\
  Serial.println(""); \
  Serial.println("Serial connected.");

#define sprint(message)       Serial.print(message);
#define sprintln(message)     Serial.println(message);

#endif

#endif // End of serial helper
