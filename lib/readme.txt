Multiplexed Display Unit
========================

A multiplexed display unit containing status LEDs and a three-digit
7-segment LED display, driven by two cascaded 4094 shift registers.


AVR Pin Connections
-------------------
  Data (D)        - PB3
  Clock Pulse (CP) - PB5
  Latch/Strobe (STR) - PD5

The QS2 output from the first 4094 cascades into the Data input of the second.


First 4094 (Control & Digit Selection)
--------------------------------------
Inputs:
  D   <- PB3 (Data)
  CP  <- PB5 (Clock)
  STR <- PD5 (Latch)

Outputs:
  QP0 -> Heater LED
  QP1 -> Auto LED
  QP2    (n.c.)
  QP3 -> Air LED
  QP4 -> Pump LED
  QP5 -> Cathode 1 (left digit)
  QP6 -> Cathode 3 (right digit)
  QP7 -> Cathode 2 (middle digit)
  QS1    (n.c.)
  QS2 -> Cascade to second 4094


Second 4094 (Segment Control)
-----------------------------
Inputs:
  D   <- QS2 from first 4094
  CP  <- PB5 (Clock)
  STR <- PD5 (Latch)

Outputs:
  QP0 -> Anode seg g
  QP1 -> Anode seg c
  QP2 -> Anode seg d.p.
  QP3 -> Anode seg d
  QP4 -> Anode seg b
  QP5 -> Anode seg f
  QP6 -> Anode seg e
  QP7 -> Anode seg a
  QS1    (n.c.)
  QS2    (n.c.)


Segment Patterns
----------------
Bit order: G C DP D B F E A

  0b01011111  // 0
  0b01001000  // 1
  0b10011011  // 2
  0b11011001  // 3
  0b11001100  // 4
  0b11010101  // 5
  0b11010110  // 6
  0b01001001  // 7
  0b11011111  // 8
  0b11001101  // 9
  0b11001111  // A
  0b11010110  // b
  0b00010111  // C
  0b11011010  // d
  0b10010111  // E
  0b10000111  // F
  0b10100010  // r.  (used for error codes, e.g., Er.4)


Assembly Code Timing Requirements
---------------------------------
Data, Clock, and Latch are normally held high. Two 8-bit words are
sent every 6ms using the following sequence:

 1. Take Data line low.
 2. 16µs later, take Clock low.
 3. 16µs later, take Clock high.
 4. Send first 8-bit word (Data transitions on rising edge of Clock).
 5. On the 8th rising edge, hold Clock high for 30µs, then go low.
 6. 16µs after the 8th rising edge, take Data low for 30µs.
 7. Return Data and Clock lines high.
 8. Send second 8-bit word (Data transitions on rising edge of Clock).
 9. After the 8th rising edge, return Data and Clock lines high.
10. 8µs later, take Latch low for 40µs, then leave high.
11. Hold Data and Clock high for 6ms, then repeat from step 1.
