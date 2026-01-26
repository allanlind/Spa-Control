; =============================================================================
; HEF4094B Shift Register Assembly Routine for ATmega8
; Optimized for inverted signal polarity (all signals active LOW)
;
; Pin assignments:
;   DATA  = PB3 (bit 3 of PORTB)
;   CLOCK = PB5 (bit 5 of PORTB)
;   LATCH = PD5 (bit 5 of PORTD)
;
; Signal polarity (active LOW / inverted):
;   - All signals idle HIGH
;   - Data: 1=LOW, 0=HIGH (inverted output)
;   - Clock: falling edge (HIGH->LOW) samples data
;   - Latch: active LOW pulse transfers data to outputs
;
; Timing (at 16MHz, 1 cycle = 62.5ns):
;   - Data setup before clock falling edge: ~2µs
;   - Clock LOW hold time: ~40µs
;   - Clock HIGH between bits: ~20µs
;   - Latch pulse width: ~40µs
; =============================================================================

#include <avr/io.h>

; Pin definitions
.equ DATA_BIT,  3       ; PB3
.equ CLK_BIT,   5       ; PB5
.equ STR_BIT,   5       ; PD5

; Masks for bit manipulation
.equ DATA_MASK, (1 << DATA_BIT)
.equ CLK_MASK,  (1 << CLK_BIT)
.equ STR_MASK,  (1 << STR_BIT)

; Delay loop counts (at 16MHz)
; Each loop iteration = 3 cycles (dec + brne)
; delay_us = (count * 3) / 16
.equ DELAY_2US,   10    ; ~2µs  (10 * 3 = 30 cycles = 1.875µs)
.equ DELAY_20US,  106   ; ~20µs (106 * 3 = 318 cycles = 19.875µs)
.equ DELAY_40US,  213   ; ~40µs (213 * 3 = 639 cycles = 39.9µs)

    .section .text
    .global shiftOut16_asm

; =============================================================================
; void shiftOut16_asm(uint8_t segments, uint8_t digitSelect)
;
; Shifts out 16 bits to cascaded 4094 shift registers.
; First byte (segments) goes to second register in chain (segment anodes).
; Second byte (digitSelect) goes to first register (LEDs + cathodes).
;
; Parameters:
;   r24 = segments (first byte - segment data for second 4094)
;   r22 = digitSelect (second byte - LEDs/cathodes for first 4094)
;
; Registers used:
;   r24 = current byte being shifted
;   r22 = second byte (digitSelect), saved
;   r18 = bit counter
;   r19 = delay loop counter
;   r20 = PORTB shadow register
; =============================================================================

shiftOut16_asm:
    ; Save registers we'll modify
    push r18
    push r19
    push r20

    ; Ensure clock and data start HIGH (idle state)
    in r20, _SFR_IO_ADDR(PORTB)
    ori r20, (CLK_MASK | DATA_MASK)
    out _SFR_IO_ADDR(PORTB), r20

    ; =========================================================================
    ; Shift out first byte (segments) - 8 bits, MSB first
    ; =========================================================================
    ldi r18, 8                  ; Bit counter

shift_byte1_loop:
    ; --- Set data line based on MSB (inverted: 1->LOW, 0->HIGH) ---
    in r20, _SFR_IO_ADDR(PORTB)
    sbrc r24, 7                 ; Skip if bit 7 is clear
    rjmp byte1_bit_set
    ; Bit is 0 -> output HIGH
    ori r20, DATA_MASK
    rjmp byte1_output
byte1_bit_set:
    ; Bit is 1 -> output LOW
    andi r20, ~DATA_MASK
byte1_output:
    ori r20, CLK_MASK           ; Keep clock HIGH while setting data
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Data setup delay (~2µs) ---
    ldi r19, DELAY_2US
byte1_setup_delay:
    dec r19
    brne byte1_setup_delay

    ; --- Clock LOW (falling edge - 4094 samples data here) ---
    andi r20, ~CLK_MASK
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Clock LOW hold time (~40µs) ---
    ldi r19, DELAY_40US
byte1_low_delay:
    dec r19
    brne byte1_low_delay

    ; --- Clock HIGH (return to idle) ---
    ori r20, CLK_MASK
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Clock HIGH time (~20µs) before next bit ---
    ldi r19, DELAY_20US
byte1_high_delay:
    dec r19
    brne byte1_high_delay

    ; Shift to next bit
    lsl r24
    dec r18
    brne shift_byte1_loop

    ; =========================================================================
    ; Shift out second byte (digitSelect) - 8 bits, MSB first
    ; =========================================================================
    mov r24, r22                ; Move digitSelect to working register
    ldi r18, 8                  ; Reset bit counter

shift_byte2_loop:
    ; --- Set data line based on MSB (inverted: 1->LOW, 0->HIGH) ---
    in r20, _SFR_IO_ADDR(PORTB)
    sbrc r24, 7                 ; Skip if bit 7 is clear
    rjmp byte2_bit_set
    ; Bit is 0 -> output HIGH
    ori r20, DATA_MASK
    rjmp byte2_output
byte2_bit_set:
    ; Bit is 1 -> output LOW
    andi r20, ~DATA_MASK
byte2_output:
    ori r20, CLK_MASK           ; Keep clock HIGH while setting data
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Data setup delay (~2µs) ---
    ldi r19, DELAY_2US
byte2_setup_delay:
    dec r19
    brne byte2_setup_delay

    ; --- Clock LOW (falling edge - 4094 samples data here) ---
    andi r20, ~CLK_MASK
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Clock LOW hold time (~40µs) ---
    ldi r19, DELAY_40US
byte2_low_delay:
    dec r19
    brne byte2_low_delay

    ; --- Clock HIGH (return to idle) ---
    ori r20, CLK_MASK
    out _SFR_IO_ADDR(PORTB), r20

    ; --- Clock HIGH time (~20µs) before next bit ---
    ldi r19, DELAY_20US
byte2_high_delay:
    dec r19
    brne byte2_high_delay

    ; Shift to next bit
    lsl r24
    dec r18
    brne shift_byte2_loop

    ; =========================================================================
    ; Latch pulse (active LOW) - transfers shift register to outputs
    ; =========================================================================

    ; Return data line to idle HIGH
    in r20, _SFR_IO_ADDR(PORTB)
    ori r20, DATA_MASK
    out _SFR_IO_ADDR(PORTB), r20

    ; Latch LOW (active - begin transfer)
    cbi _SFR_IO_ADDR(PORTD), STR_BIT

    ; Latch pulse width (~40µs)
    ldi r19, DELAY_40US
latch_delay:
    dec r19
    brne latch_delay

    ; Latch HIGH (idle - transfer complete)
    sbi _SFR_IO_ADDR(PORTD), STR_BIT

    ; Restore registers
    pop r20
    pop r19
    pop r18
    ret
