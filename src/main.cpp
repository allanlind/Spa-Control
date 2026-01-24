/**
 * 4-Digit 7-Segment Display for Davies SPA-QUIP v6
 * ATmega8A-AU with Two Daisy-Chained HEF4094B Shift Registers
 *
 * Based on schematic: SPA-QUIP_V6.kicad.sch (2026-01-19)
 *
 * Hardware Configuration:
 * =======================
 * - MCU: ATmega8A-AU (U1) with 4MHz crystal
 * - Shift Registers: U6 (HEF4094B) -> U7 (HEF4094B) daisy-chained
 * - Display: CA56-125URWA (U8) - 4-digit COMMON ANODE 7-segment
 * - Digit Drivers: BC856 PNP transistors (accent LOW to enable digit)
 *
 * Shift Register Chain (16 bits total):
 * =====================================
 * First 8 bits  -> U6 (digit select + control)
 * Second 8 bits -> U7 (segments A-G + DP)
 *
 * U7 Output Mapping (Segments) - directly to display via 1k5 resistors:
 * ----------------------------------------------------------------------
 * QP0 -> Segment A
 * QP1 -> Segment B
 * QP2 -> Segment C
 * QP3 -> Segment D
 * QP4 -> Segment E
 * QP5 -> Segment F
 * QP6 -> Segment G
 * QP7 -> DP (Decimal Point)
 *
 * U6 Output Mapping (Digit Select via BC856 PNP transistors):
 * -----------------------------------------------------------
 * QP0 -> Digit 1 (CA1) via BC856 Q2
 * QP1 -> Digit 2 (CA2) via BC856 Q3
 * QP2 -> Digit 3 (CA3) via BC856 Q4
 * QP3 -> Digit 4 (CA4) via BC856 (accent LOW = digit ON)
 * QP4-QP7 -> Available for other use
 *
 * ATmega8A-AU Pin Connections to HEF4094B (adjust as per your wiring):
 * --------------------------------------------------------------------
 * PD5 (Pin 11) -> DATA   (Pin 2 on U6)
 * PD6 (Pin 12) -> CLOCK  (Pin 3 on U6 & U7)
 * PD7 (Pin 13) -> STROBE (Pin 1 on U6 & U7)
 *
 * Note: Common Anode display requires ACTIVE LOW for segments
 *       BC856 PNP transistors require ACTIVE LOW for digit selection
 *
 * 7-Segment Display Layout:
 *      AAA
 *     F   B
 *      GGG
 *     E   C
 *      DDD  .DP
 */

#include <Arduino.h>

// =============================================================================
// PIN CONFIGURATION - Adjust these to match your actual wiring
// =============================================================================
#define DATA_PIN    5    // Arduino pin for DATA (PD5)
#define CLOCK_PIN   6    // Arduino pin for CLOCK (PD6)
#define STROBE_PIN  7    // Arduino pin for STROBE (PD7)

// =============================================================================
// DISPLAY CONFIGURATION
// =============================================================================
#define NUM_DIGITS      4
#define MULTIPLEX_DELAY 2    // ms between digit updates (adjust for brightness)

// Segment patterns for digits 0-9 (active HIGH, will be inverted for common anode)
// Bit order: DP G F E D C B A
const uint8_t segmentPatterns[] = {
    0b00111111,  // 0: A B C D E F
    0b00000110,  // 1: B C
    0b01011011,  // 2: A B D E G
    0b01001111,  // 3: A B C D G
    0b01100110,  // 4: B C F G
    0b01101101,  // 5: A C D F G
    0b01111101,  // 6: A C D E F G
    0b00000111,  // 7: A B C
    0b01111111,  // 8: A B C D E F G
    0b01101111,  // 9: A B C D F G
    0b01110111,  // A (10)
    0b01111100,  // b (11)
    0b00111001,  // C (12)
    0b01011110,  // d (13)
    0b01111001,  // E (14)
    0b01110001,  // F (15)
    0b00000000,  // Blank (16)
};

#define CHAR_BLANK 16
#define CHAR_DASH  0b01000000  // Just segment G

// Digit select patterns (active LOW for PNP transistors)
// Only one digit active at a time
const uint8_t digitSelect[] = {
    0b11111110,  // Digit 1 ON (QP0 LOW)
    0b11111101,  // Digit 2 ON (QP1 LOW)
    0b11111011,  // Digit 3 ON (QP2 LOW)
    0b11110111,  // Digit 4 ON (QP3 LOW)
};

// =============================================================================
// DISPLAY BUFFER
// =============================================================================
volatile uint8_t displayBuffer[NUM_DIGITS] = {0, 0, 0, 0};  // Digit values (0-16)
volatile uint8_t decimalPoints = 0;  // Bit mask for decimal points (bit 0 = digit 1)
volatile uint8_t currentDigit = 0;   // Currently displayed digit (0-3)

// =============================================================================
// SHIFT REGISTER FUNCTIONS
// =============================================================================

/**
 * Shift out 16 bits to the daisy-chained HEF4094B registers
 * Data is shifted MSB first
 * First byte goes to U7 (segments), second byte goes to U6 (digit select)
 * @param segments Segment data for U7 (will be inverted for common anode)
 * @param digitSel Digit select data for U6
 */
void shiftOut16(uint8_t segments, uint8_t digitSel) {
    // Invert segments for common anode display (LOW = segment ON)
    uint8_t segInverted = ~segments;

    // Shift out segment data first (goes to U7 at end of chain)
    for (int8_t i = 7; i >= 0; i--) {
        // Set DATA pin
        digitalWrite(DATA_PIN, (segInverted >> i) & 0x01);

        // Pulse CLOCK
        digitalWrite(CLOCK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLOCK_PIN, LOW);
    }

    // Shift out digit select data (goes to U6)
    for (int8_t i = 7; i >= 0; i--) {
        // Set DATA pin
        digitalWrite(DATA_PIN, (digitSel >> i) & 0x01);

        // Pulse CLOCK
        digitalWrite(CLOCK_PIN, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLOCK_PIN, LOW);
    }

    // Pulse STROBE to latch data to outputs
    digitalWrite(STROBE_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(STROBE_PIN, LOW);
}

/**
 * Turn off all segments and digits
 */
void clearDisplay() {
    shiftOut16(0x00, 0xFF);  // All segments off, all digits off
}

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================

/**
 * Refresh one digit of the display (call this in loop or timer ISR)
 * Cycles through all digits for multiplexing
 */
void refreshDisplay() {
    uint8_t value = displayBuffer[currentDigit];
    uint8_t segments = (value <= 16) ? segmentPatterns[value] : 0;

    // Add decimal point if enabled for this digit
    if (decimalPoints & (1 << currentDigit)) {
        segments |= 0b10000000;  // Set DP bit
    }

    // Output to shift registers
    shiftOut16(segments, digitSelect[currentDigit]);

    // Move to next digit
    currentDigit = (currentDigit + 1) % NUM_DIGITS;
}

/**
 * Set a single digit value
 * @param position Digit position (0-3, left to right)
 * @param value Digit value (0-15 for hex, 16 for blank)
 */
void setDigit(uint8_t position, uint8_t value) {
    if (position < NUM_DIGITS) {
        displayBuffer[position] = value;
    }
}

/**
 * Display a 4-digit number
 * @param number Number to display (0-9999)
 * @param leadingZeros Show leading zeros if true
 */
void displayNumber(uint16_t number, bool leadingZeros = false) {
    if (number > 9999) number = 9999;

    displayBuffer[0] = (number / 1000) % 10;
    displayBuffer[1] = (number / 100) % 10;
    displayBuffer[2] = (number / 10) % 10;
    displayBuffer[3] = number % 10;

    // Blank leading zeros if not wanted
    if (!leadingZeros) {
        if (displayBuffer[0] == 0) {
            displayBuffer[0] = CHAR_BLANK;
            if (displayBuffer[1] == 0) {
                displayBuffer[1] = CHAR_BLANK;
                if (displayBuffer[2] == 0) {
                    displayBuffer[2] = CHAR_BLANK;
                }
            }
        }
    }
}

/**
 * Set decimal point
 * @param position Digit position (0-3)
 * @param on True to enable, false to disable
 */
void setDecimalPoint(uint8_t position, bool on) {
    if (position < NUM_DIGITS) {
        if (on) {
            decimalPoints |= (1 << position);
        } else {
            decimalPoints &= ~(1 << position);
        }
    }
}

/**
 * Display a temperature value (e.g., 38.5 displays as "38.5")
 * @param tempTenths Temperature in tenths of a degree (e.g., 385 for 38.5°C)
 */
void displayTemperature(int16_t tempTenths) {
    bool negative = tempTenths < 0;
    if (negative) tempTenths = -tempTenths;

    if (tempTenths > 999) tempTenths = 999;

    uint8_t hundreds = tempTenths / 100;
    uint8_t tens = (tempTenths / 10) % 10;
    uint8_t ones = tempTenths % 10;

    if (negative) {
        displayBuffer[0] = CHAR_BLANK;  // Or use CHAR_DASH for minus sign
        displayBuffer[1] = hundreds > 0 ? hundreds : CHAR_BLANK;
    } else {
        displayBuffer[0] = hundreds > 0 ? hundreds : CHAR_BLANK;
        displayBuffer[1] = tens;
    }
    displayBuffer[2] = negative ? tens : ones;
    displayBuffer[3] = negative ? ones : CHAR_BLANK;

    // Set decimal point after position 1 (showing X.X format)
    decimalPoints = negative ? 0b0100 : 0b0010;
}

// =============================================================================
// SETUP AND MAIN LOOP
// =============================================================================

void setup() {
    // Configure control pins as outputs
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(STROBE_PIN, OUTPUT);

    // Initialize pins LOW
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(STROBE_PIN, LOW);

    // Clear display
    clearDisplay();

    // Initialize display buffer with test pattern
    displayNumber(0);
}

void loop() {
    // Demo: Count from 0 to 9999
    static uint16_t counter = 0;
    static unsigned long lastUpdate = 0;

    // Refresh display continuously (multiplexing)
    refreshDisplay();
    delay(MULTIPLEX_DELAY);

    // Update counter every second
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        counter++;
        if (counter > 9999) counter = 0;
        displayNumber(counter);
    }
}

// =============================================================================
// ALTERNATIVE: Timer-based display refresh (uncomment to use)
// =============================================================================
/*
// Use Timer2 for automatic display refresh
void setupTimer2() {
    // Set up Timer2 for ~500Hz refresh (125Hz per digit)
    TCCR2A = (1 << WGM21);           // CTC mode
    TCCR2B = (1 << CS22) | (1 << CS21); // Prescaler 256
    OCR2A = 31;                       // Compare value for ~500Hz at 4MHz
    TIMSK2 = (1 << OCIE2A);          // Enable compare interrupt
}

ISR(TIMER2_COMPA_vect) {
    refreshDisplay();
}
*/
