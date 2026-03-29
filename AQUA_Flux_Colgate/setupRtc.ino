// setupRtc.ino
//
// Initializes the PCF8523 RTC:
//   1. Connects via I2C (rtc.begin); halts if not found
//   2. Ensures CONTROL_1 bit 3 is clear (24-hour mode workaround)
//   3. Prompts operator to set the time if the RTC is uninitialized or lost power
//   4. Starts the oscillator (rtc.start)
//   5. Validates the date; prompts again and halts if implausible
//
// Must be called after setupI2c() so Wire is initialized at the correct
// clock speed before any RTC transactions are issued.
//
// Guarded by USE_DATALOGGER — rtc and related globals are only declared
// under that guard in AQUA_Flux_Colgate.ino.

#if USE_DATALOGGER
void setRtcTo24HrMode()
{
  // Check if the RTC is configured for 24 hour mode
  // Bug workaround for the situation where RTC fails to
  // rollover after midnight and reports invalid time like 24:01
  Wire.beginTransmission(RTC_ADDRESS); // PCF8523 I2C address
  Wire.write(0x00);                    // CONTROL_1 register
  uint8_t err = Wire.endTransmission();
  if (err != 0)
  {
    LOG_STREAM.print(F("RTC Data Logger error when trying to read CONTROL_1 register: "));
    LOG_STREAM.println(err);
    error("Could not transmit command to read the CONTROL_1 register of RTC");
  }

  // (possibly unnecessary) delay to allow RTC to respond
  delay(20);

  uint8_t received = Wire.requestFrom(RTC_ADDRESS, (uint8_t)1);
  // Confirm we got 1 byte back
  if (received != 1)
  {
    LOG_STREAM.print(F("ERROR: Expected 1 byte, got "));
    LOG_STREAM.println(received);
    error("Could not read the CONTROL_1 register of RTC");
  }

  uint8_t control1 = Wire.read();

  bool is24HourMode = (control1 & (1 << 3)) == 0; // bit 3 = 0 → 24-hour mode

  // Force PCF8523 into 24-hour mode (clear bit 3 in CONTROL_1 register)
  if (!is24HourMode)
  {
    LOG_STREAM.print(F("RTC Data Logger is not in 24-hour mode. Setting to 24hr mode..."));
    // Clear 12_24 bit → 24 - hour mode
    uint8_t bit = (1 << 3);       // 00001000  — isolate bit 3
    uint8_t mask = (uint8_t)~bit; // 11110111  — invert to create a clear-mask
    control1 = control1 & mask;   // force bit 3 to 0, preserve all others

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x00);
    Wire.write(control1);
    err = Wire.endTransmission();
    if (err != 0)
    {
      LOG_STREAM.print(F("Could not write CONTROL_1 register for 24hr mode. Error: "));
      LOG_STREAM.println(err);
      error("Could not transmit command to set the CONTROL_1 register of RTC to 24hr mode");
    }
    LOG_STREAM.println(F("Done"));
  }
  else
  {
    DEBUG_PRINTLN(F("RTC is in 24 hour mode"));
  }

  return;
}
#endif // USE_DATALOGGER

void setupRtc(void)
{
#if USE_DATALOGGER

  DEBUG_PRINTLN(F("DEBUG - RTC enabled"));

  // Initialize RTC
  if (!rtc.begin())
    error("Couldn't find RTC");

  // 24-hr bug workaround: ensure CONTROL_1 bit 3 = 0 (24-hour mode).
  // Without this the PCF8523 can report 24:01 instead of rolling over to 00:01.
  setRtcTo24HrMode();

  // Prompt operator if the clock was never set or the backup battery died.
  if (!rtc.initialized() || rtc.lostPower())
  {
    LOG_STREAM.println(F("RTC is NOT initialized. Time Must Be Set"));
    setRtcDate();
  }

  // Start the oscillator (clears the STOP bit in CONTROL_1).
  rtc.start();

  // Validate the RTC date and time.
  // An implausible date (e.g. power loss with dead battery) means the SD log
  // file would get a wrong name. Prompt the operator, then halt so the reboot
  // confirms the RTC retains the new time.
  DateTime rtcNow = rtc.now();
  if (!isRtcDateValid(rtcNow))
  {
    char buf[24]; // "YYYY/MM/DD HH:MM:SS" + null
    snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d",
             rtcNow.year(), rtcNow.month(), rtcNow.day(),
             rtcNow.hour(), rtcNow.minute(), rtcNow.second());
    LOG_STREAM.print(F("The RTC has an implausible date: "));
    LOG_STREAM.println(buf);
    setRtcDate();
    error("Reboot to confirm RTC will maintain the correct date.");
  }
#else
  DEBUG_PRINTLN(F("DEBUG - RTC disabled"));
#endif // USE_DATALOGGER
}
