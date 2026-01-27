/**
 * 4-Digit 7-Segment Display for Spa Control
 *
 * Hardware:
 *   MCU: ATmega8A-AU @ 4MHz
 *   Display: CA56-125URWA (Common Anode)
 *   Shift Registers: 2x HEF4094B daisy-chained
 *
 * Pins:
 *   PD5 -> DATA, PD6 -> CLOCK, PD7 -> STROBE
 */

#include <Arduino.h>
#include <SevSegShift.h>

#define DATA_PIN   5
#define CLOCK_PIN  6
#define LATCH_PIN  7

SevSegShift display;

void setup() {
    display.begin(COMMON_ANODE, 4, DATA_PIN, CLOCK_PIN, LATCH_PIN);
    display.setNumber(0);
}

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
