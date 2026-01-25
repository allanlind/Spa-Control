/**
 * Countdown Timer Example
 * Counts down from 60 seconds, displays "End" when finished
 */

#include <SevSegShift.h>

#define DATA_PIN   11   // PB3
#define CLOCK_PIN  13   // PB5 (CP)
#define LATCH_PIN  5    // PD5 (STR)

SevSegShift display;

void setup() {
    display.begin(COMMON_ANODE, 4, DATA_PIN, CLOCK_PIN, LATCH_PIN);
    display.setNumber(60);
}

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
