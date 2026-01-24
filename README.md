# Seven Segment Display Driver

Firmware for a 4-digit 7-segment display driver for the Davies SPA-QUIP v6 device. Uses the SevSegShift library for shift register-based multiplexing on an ATmega8A.

## Hardware

- **MCU**: ATmega8A-AU @ 4MHz external crystal
- **Display**: CA56-125URWA 4-digit common anode 7-segment display
- **Shift Registers**: Two daisy-chained HEF4094B 8-bit shift registers
- **Digit Drivers**: BC856 PNP transistors

## Pin Configuration

| Pin | Function |
|-----|----------|
| PD5 (Pin 11) | DATA - Serial data to shift registers |
| PD6 (Pin 12) | CLOCK - Clock signal to shift registers |
| PD7 (Pin 13) | LATCH - Latch/strobe signal to shift registers |

## SevSegShift Library

This project includes a custom fork of the [SevSeg](https://github.com/untr0py/SevSeg) library modified to support shift register-based displays. The library is located in [lib/SevSegShift/](lib/SevSegShift/).

### Usage

```cpp
#include <SevSegShift.h>

SevSegShift display;

void setup() {
    // begin(displayType, numDigits, dataPin, clockPin, latchPin, leadingZeros)
    display.begin(COMMON_ANODE, 4, 5, 6, 7);
}

void loop() {
    display.refreshDisplay();  // Call continuously for multiplexing
}
```

### API

```cpp
// Initialize display
display.begin(COMMON_ANODE, 4, dataPin, clockPin, latchPin);

// Display integer (0-9999)
display.setNumber(1234);

// Display with decimal point (2nd param = decimal places from right)
display.setNumber(1234, 2);  // Shows "12.34"

// Display float
display.setNumberF(12.34, 2);  // Shows "12.34"

// Display characters (0-9, A-F, dash, space)
display.setChars("CAFE");

// Clear display
display.blank();

// Adjust brightness (0-100)
display.setBrightness(80);
```

### Examples

**Counter (0-9999)**
```cpp
void loop() {
    static uint16_t counter = 0;
    static unsigned long lastUpdate = 0;

    display.refreshDisplay();

    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        counter = (counter + 1) % 10000;
        display.setNumber(counter);
    }
}
```

**Temperature Display**
```cpp
void displayTemperature(float tempC) {
    // Show temperature with 1 decimal place (e.g., 23.5)
    display.setNumberF(tempC, 1);
}

void loop() {
    display.refreshDisplay();

    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 2000) {
        lastRead = millis();
        float temp = readSensor();  // Your sensor reading function
        displayTemperature(temp);
    }
}
```

**Countdown Timer**
```cpp
void loop() {
    static int16_t seconds = 60;
    static unsigned long lastTick = 0;

    display.refreshDisplay();

    if (millis() - lastTick >= 1000 && seconds > 0) {
        lastTick = millis();
        seconds--;
        display.setNumber(seconds);
    }

    if (seconds == 0) {
        display.setChars("End");
    }
}
```

**Hex Display**
```cpp
void loop() {
    display.refreshDisplay();

    // Display hex values (0-9, A-F)
    display.setChars("CAFE");  // Shows "CAFE"
    // Or: display.setChars("DEAD");
    // Or: display.setChars("BEEF");
}
```

## Building

This project uses PlatformIO with the MiniCore board package.

```bash
# Build
pio run

# Upload (requires USBASP programmer)
pio run --target upload
```

## Schematic

See [schematic.pdf](schematic.pdf) for the hardware design.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.
