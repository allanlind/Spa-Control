/**
 * SevSegShift - 7-Segment Display Library with Shift Register Support
 *
 * Fork of SevSeg library adapted for daisy-chained shift registers.
 * Designed for HEF4094B or similar 8-bit shift registers.
 *
 * Original SevSeg: https://github.com/DeanIsMe/SevSeg
 */

#ifndef SevSegShift_h
#define SevSegShift_h

#include <Arduino.h>

// Display types
#define COMMON_CATHODE 0
#define COMMON_ANODE   1

// Maximum digits supported
#define MAX_DIGITS 8

class SevSegShift {
public:
    SevSegShift();

    /**
     * Initialize the display
     * @param displayType COMMON_CATHODE or COMMON_ANODE
     * @param numDigits Number of digits (1-8)
     * @param dataPin Pin connected to shift register DATA
     * @param clockPin Pin connected to shift register CLOCK
     * @param latchPin Pin connected to shift register LATCH/STROBE
     * @param leadingZeros Show leading zeros if true
     */
    void begin(uint8_t displayType, uint8_t numDigits,
               uint8_t dataPin, uint8_t clockPin, uint8_t latchPin,
               bool leadingZeros = false);

    /**
     * Refresh the display (call continuously in loop)
     */
    void refreshDisplay();

    /**
     * Display an integer number
     * @param num Number to display
     * @param decPlace Decimal point position from right (0 = none)
     */
    void setNumber(int32_t num, int8_t decPlace = 0);

    /**
     * Display a floating point number
     * @param num Number to display
     * @param decPlaces Number of decimal places
     */
    void setNumberF(float num, int8_t decPlaces = 1);

    /**
     * Display a character string
     * @param str String to display (supports 0-9, A-F, dash, space)
     */
    void setChars(const char* str);

    /**
     * Clear the display
     */
    void blank();

    /**
     * Set brightness (not directly applicable to shift registers,
     * but adjusts multiplex timing)
     * @param brightness Value from 0-100
     */
    void setBrightness(uint8_t brightness);

private:
    void shiftOut16(uint8_t segments, uint8_t digitSelect);
    uint8_t charToSegment(char c);

    uint8_t _dataPin;
    uint8_t _clockPin;
    uint8_t _latchPin;
    uint8_t _numDigits;
    uint8_t _displayType;
    bool _leadingZeros;

    uint8_t _digitCodes[MAX_DIGITS];   // Segment patterns for each digit
    uint8_t _decimalPoint;             // Decimal point position (0 = none)
    uint8_t _currentDigit;             // Current digit being refreshed
    uint16_t _refreshDelay;            // Microseconds between refreshes
};

#endif
