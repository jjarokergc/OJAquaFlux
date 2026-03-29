// TODO
// The miscalibrated message will not appear on the first log file because
// K30 setup occurs after the SD card is initialized.
// Separate the log file creation so that it occurs after K30 setup completes
// to put the miscalibration warning into the first log file.

// setupSdCard.ino
//
// Initializes the SD card and manages log files:
//   1. Configures the SD CS pin and calls SD.begin(); halts on failure
//   2. Validates read/write integrity with testSdCard()
//   3. Opens a date-named CSV log file (YYYYMMDD.CSV); appends on same-day reboot
//
// Must be called after setupRtc() — openNextLogfile() reads the RTC date to
// build the filename.
//
// Functions:
//   openNextLogfile() - open or append to today's YYYYMMDD.CSV; write header if new
//   rotateLogfile()   - flush/close current file and open the next (called at midnight)
//   setupSdCard()     - called once from setup() after setupRtc()

#if USE_DATALOGGER

// Opens YYYYMMDD.CSV for today's date (appends if it already exists, e.g. same-day
// reboot). Writes the CSV header only for new files. Calls error() on failure.
void openNextLogfile()
{
  DateTime now = rtc.now();
  char filename[13]; // "YYYYMMDD.CSV"
  snprintf(filename, sizeof(filename), "%04d%02d%02d.CSV",
           now.year(), now.month(), now.day());

  bool isNewFile = !SD.exists(filename);
  logfile = SD.open(filename, FILE_WRITE); // FILE_WRITE appends if file exists
  if (!logfile)
    error("Couldn't create file");

  currentLogDay = now.day();
  DEBUG_PRINT(F("Logging to: "));
  DEBUG_PRINTLN(filename);

  if (isNewFile)
  {
    // Write header only once per file; skip on same-day reboot (append mode)
    logfile.print(F("millis, stampunix, datetime, K30_CO2"));
#if USE_K30
    if (K30_WARN_IF_MISCALIBRATED && miscalibratedK30)
      logfile.print(F("(**)"));
#endif
    logfile.println(F(", CH4smV, Vbat, SHT_RH, SHT_temp, AQUA_Flux1 "));
#if USE_K30
    if (K30_WARN_IF_MISCALIBRATED && miscalibratedK30)
      logfile.println(F("** The K30 is miscalibrated, according to Error Status."));
#endif
    logfile.flush(); // persist header immediately — first data row may not arrive for >30 s
  }
}

// Closes the current log file and opens the next one.
void rotateLogfile()
{
  logfile.flush();
  logfile.close();
  LOG_STREAM.println(F("Log file rotated."));
  openNextLogfile();
}
#endif // USE_DATALOGGER

void setupSdCard(void)
{
#if USE_DATALOGGER

  DEBUG_PRINT(F("Initializing SD card..."));
  // Make sure that the default chip select pin is set to OUTPUT
  pinMode(SD_CARD_CS, OUTPUT);

  // See if the SD card is present and can be initialized:
  if (!SD.begin(SD_CARD_CS))
    error("Card failed or not present");

  // Validate SD card by writing a test file
  testSdCard();
  DEBUG_PRINTLN(F("card initialized."));

  openNextLogfile();
#else
  DEBUG_PRINTLN(F("DEBUG - SD card logging disabled"));
#endif // USE_DATALOGGER
}
