// setupLogging.ino
//
// Opens the operator-facing log stream at startup:
//   USE_XBEE=1 — starts the SoftwareSerial XBee link at XBEE_BAUD_RATE
//   USE_XBEE=0 — starts USB Serial at SERIAL_BAUD_RATE
//
// Called once from setup() before any other subsystem so that all subsequent
// startup messages have somewhere to go.

void setupLogging(void)
{
#if USE_XBEE
  XBee.begin(XBEE_BAUD_RATE);
#else
  Serial.begin(SERIAL_BAUD_RATE);
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000UL); // wait up to 3 s for USB CDC (R4 native USB)
#endif
}
