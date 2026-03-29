# AQUA-Flux Refactoring Notes

---

## 1. K30 Sensor Class

The K30 implementation is currently split across three `.ino` files
(`activateK30.ino`, `setupK30.ino`, `readK30_CO2_withRetry.ino`) with one
global variable (`miscalibratedK30`) in `AQUA_Flux_Colgate.ino`.
Consolidate into a single class in `K30Sensor.h`.

### Proposed interface

```cpp
// K30Sensor.h
class K30Sensor {
public:
    // targetAddr — the I2C address the sensor should be configured to use
    // (K30_I2C_ADDR).  Pass 0x7F for "any sensor on a point-to-point bus".
    explicit K30Sensor(uint8_t targetAddr);

    // Setup: relay power-on (if HAS_K30_RELAY), address verification, error
    // status check.  Replaces activateK30() + setupK30().
    void begin();

    // Read CO2 in ppm.  Returns 0 on failure after retries.
    // Replaces readK30_CO2_withRetry().
    int16_t readCO2();

    // True if the error status register reported a calibration warning.
    // Replaces the global miscalibratedK30.
    bool isMiscalibrated() const { return _miscalibrated; }

private:
    // Wire primitives — shared by begin() and readCO2()
    void    readRAM(uint8_t i2cAddr, uint16_t ramAddr,
                    uint8_t *buf, uint8_t n = 2);
    void    writeEEPROM(uint8_t i2cAddr, uint16_t eepromAddr, uint8_t val);

    // Address management
    uint8_t readConfiguredAddress();   // reads RAM 0x0020 via 0x7F
    bool    ensureAddress();           // verify/write + power-cycle if needed
    bool    checkErrorStatus();        // reads RAM 0x1E; sets _miscalibrated

    uint8_t _addr;
    bool    _miscalibrated = false;
    char    _errbuf[64];
};
```

---

## 2. Duplicate I2C Wire Protocol

`readK30_CO2_withRetry` contains its own hand-rolled Wire read sequence
that duplicates `k30ReadRAM` from `setupK30.ino`. The two implementations
diverge in subtle ways (retry logic, flush placement, timeout handling).

After the class refactor (§1), rewrite `readCO2()` to call `readRAM` for
the actual bus transaction and keep only the CO2-specific logic (address
`0x0008`, range validation) in `readCO2` itself.

The error handling of `readCO2()` should be retained.

---

## 3. Global miscalibratedK30

`miscalibratedK30` is a `bool` declared at file scope in
`AQUA_Flux_Colgate.ino` under `#if USE_K30`. It is written by
`checkK30ErrorStatus()` and read in `loop()`.

After the class refactor (§1) this becomes `K30Sensor::_miscalibrated`,
accessed via `k30.isMiscalibrated()`. This eliminates the only mutable
K30 global from the main file.

