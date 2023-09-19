.include 'inc/target.inc'

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Notes etc

; RAM starts at 0040
; No need for absolute addressing but this was the base in use during development
; .area SYS (ABS)
; .org 0x00b0

; ; ; ; ; ; ; ; ; ;
; CPU osc set to 8 MHz ( bus == 2 MHz )

; COP is configured to trigger a hard reset after 130~ ms (or maybe 32.5? Plenty of time either way)

; ; ; ; ; ; ; ; ; ;
; Timer 1
; Divider = 2
;
; T1MOD
; 0x07D0 ( > 2.000 ms < )

; ; ; ; ; ; ; ; ; ;
; Timer 2
; Divider = 2
;
; T2MOD
; 0x61a8 ( > 25.000 ms < ) in boot mode 99 ( Special sauce, ie how we wound up in this code )
; 0x186a (    6.250 ms   ) in boot mode a5 ( Regular operational / boot mode )

; ; ; ; ; ; ; ; ; ;
; Interrupts are 100% disabled in special sauce mode


; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Settings

CONSEC_RX_TRIES  .equ    5      ; Number of CONSECUTIVE reception errors to accept before giving up. -Reset after each successful reception
LAST_STACK_ADDR  .equ    0x023f ; Last usable stack address

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Symbols

TEMP_0040        .equ    0x0040 ; Dealing with certain data in stack makes the code explode in size
TEMP_0041        .equ    0x0041

; Imported symbols
; .globl _testFunc

; Exported symbols
.globl _entry  ; Entry point of this app. -Of no interest to the rest of the code but it's a nice thing to have in the map file

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; App

_entry:
; Beware. txs is sinister. sp = h:x - 1
    ldhx    #(LAST_STACK_ADDR + 1)
    txs

; Stack frame
;   19: Reset countdown (How many consecutive unrecognised messages to accept before going into reset mode)
;   18: rx fault flag   (Unused atm)
;   17: rx count        (Number of bytes left to read before a complete frame has been assembled)
; 1-16: spi data        (SPI frame. tsx has a nifty trick up its sleeve so it'll automagically point here when using said instruction)
;    0: * do not use *  (Data is stored BEFORE the stack pointer is decremented. Thus, this is used when calling functions or pushing data)
    ais #-22


; ; ; ; ; Start of a new frame
loopResetErrCnt:

    lda     #CONSEC_RX_TRIES   ; Reset Command err
    sta     19, s

loopKeepErrCnt:

; Beware. tsx is sinister. h:x = sp + 1
    tsx           ; Point H:X at spi buffer start

    lda     #16   ; Reset rx count
    sta     16, x
    clr     17, x ; Clear rx fault

; Have to disable and enable the controller to enable a nifty coding hack
    bclr    #1, *SPCR ; Disable SPI
    lda     *SPSCR    ; Read stuff just to please the SPI controller
    lda     *SPDR
    bset    #1, *SPCR ; Enable SPI

    lda     ,x        ; Preload first byte to be sent
    sta     *SPDR

    lda     *PTA      ; Toggle busy pin (PTA1) as to signal start of a new loop to the master
    eor     #0x02
    sta     *PTA

; ; ; ; ; RX/TX loop
rxLoop:

    sta     COPCTL    ; Pet watchdog. There, there. Nice doggo

    lda     *SPSCR    ; Load SPI flags register
    bit     #0x30     ; Check for error conditions; Overflow or mode fault
    bne     rxError

    tsta
    bpl     rxLoop    ; Check reception flag
    ; Check for timeout?

    mov     *SPDR, x+ ; Read SPI and store received byte
    lda     ,x        ; Read next byte to send
    sta     *SPDR

    dbnz    17, s, rxLoop ; Decrement counter and continue if not 0

; bra is using a relative 8-bit offset. rxError can't be too far away for some of the other functions
    bra rxOk

; Received length out of bounds
lenError:
    tsx
    lda     #0x20
    sta     1, x

; Command error one way or another
rxError:
; Use internal delay function and wait for 2.2 ms
; 8 + (3 * 8 * 183 + 8) = 4408 bus cycles
; (8 MHz cpu == 2 MHz bus)
; 4408 / 2000000 = 0.002204 = 2.2 ms (Actual count is 4405)
; Delay 2202.5 micro (2.2 ms)
    lda     #8
    ldx     #183
    sta     COPCTL ; Service watchdog
    jsr     DELNUS ; Jump!

; TODO: Determine how to signal error
;   tsx
;   lda     #0x7f
;   sta     ,x

; Check remaining error count. Die a fiery death if 0
    dbnz    19, s, loopKeepErrCnt
deathLoop:          ; Wait for watchdog reset
    bra     deathLoop

commandOk:
    tsx
    incx    ; Slightly naughy but we know stack resides betweent 200 and 23f
    lda     , x
    ora     #0x80
    sta     , x
    bra     loopResetErrCnt

rxOk:

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Command interpretation

; ; ; ; ; Data done
; RX:
;  0: step
;  1: command
;2-3: Address

    ; sp 0 1 2 3 4 5 6 7 8 9 a b c d e f 10
    ; rx   0 1 2 3 4 5 6 7 8 9 a b c d e f

    pula    ; 0, Step
    pula    ; 1, Command
    pulh    ; 2, Hi address
    pulx    ; 3, Lo address
    ais     #-4

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Commands
;
; x0 ?? - <MAIN  > Running status / flags / ram
; x6 00 - <MAIN  > Enter shutdown
;
; 4f xx - <BOOT  > Idle / polling
; 6f xx - <BOOT  > Erase and write flash combo weirdness
; af 0x - <BOOT  > Firmware information (Handled by boot but main and cal must be present)
;
; yy 01 - <SAUCE > Ping / idle message                - implemented -
; yy 02 - <SAUCE > Read address                       - implemented -
; yy 03 - <SAUCE > Write address                      - implemented -
; yy 04 - <SAUCE > Jump to arbitrary address          - implemented -
; yy 05 - <SAUCE > Identity string                    - implemented -
; yy 06 - * * * * * * * * * shutdown ? ?
; yy 07 - <LOADER> Set flash protection mask          - implemented -
; yy 08 - <LOADER> Erase flash                        - missing -
; YY 09 - <LOADER> Upload block of data / write data  - missing -
;
; SAUCE  = Commands that are present in the loader and boot sauce mode
; LOADER = Commands that are only available in the loader
; yy     = Stepper, 06 / 19 hex

    dbnza    checkRamRead

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 1 - Idle message
; yy 01     00 00 00 00 00 00 00 00 00 00 00 00 00 00

    bra     commandOk
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

checkRamRead:
    dbnza    checkRamWrite

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 2 - RAM Read
; yy 02 HH LL NN     00 00 00 00 00 00 00 00 00 00 00
; HH LL: 16-bit address
; NN   : Count 1 to 10
; DD   : Data
; CC   : Checkum-8 of the whole response, byte [0] to [14]
    lda     5, s     ; Read len
    beq     lenError ; 0-len
    cmp     #11
    bcc     lenError ; 11 or more bytes (error)
; Copy data
    pshh         ; Push source address
    pshx
    tsx          ; Load destination address
    aix     #7   ; Increment past pushed address, stepper, command, address and length
    bsr     memcpy
    ais     #2
; Flag OK and store checksum
    tsx
    lda     #0x82
    sta     1, x

    bsr     getChecksum
    sta     ,x   ; Store checksum

    bra     loopResetErrCnt

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

checkRamWrite:
    dbnza    checkRamRun

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 3 - RAM Write
; yy 03 HH LL NN     DD DD DD DD DD DD DD DD DD DD CC
; HH LL: 16-bit address
; NN   : Count 1 to 10
; DD   : Data
; CC   : Checkum-8 of the whole request, byte [0] to [14]
    lda     5, s     ; Read len
    beq     lenError ; 0-len
    cmp     #11
    bcc     lenError ; 11 or more bytes (error)

; Verify checksum of received data
    tsx
    bsr     getChecksum
    cmp     ,x
    beq     wrChecksumOk
    lda     #0x04
    sta     2, s
    bra     rxError
wrChecksumOk:

    aix     #-10
    pshh
    pshx

    lda     5, s
    psha
    pulh
    ldx     6, s

    lda     7, s ; Read len
    bsr     memcpy

    ais     #2

commandOkOOR:
    bra     commandOk


; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Relative addressing so must be fairly close...

jumprxError:
    bra rxError

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

checkRamRun:
    dbnza    checkIdentity

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 4 - RAM Execute
; yy 04 HH LL - Jump to address
; HH LL: 16-bit address
    jmp     ,x
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

; sp: Source address
; h:X Destination address
; a: Count
memcpy:
    psha    ; Push count
    lda     5, s
    sta     *TEMP_0040
    lda     4, s
    sta     *TEMP_0041
; SP:
; 05: SOURCE H
; 04: SOURCE L
; 03: RETURN L
; 02: RETURN H
; 01: Count

cpyLoop:

    pshh
    pshx

    ldhx    *TEMP_0040
    lda     ,x
    aix     #1
    sthx    *TEMP_0040

    pulx
    pulh
    sta     ,x
    aix     #1

    dbnz    1, s, cpyLoop

    pula

    rts

; Checksum
; h:x source  (h:x will point at the next byte after the last checksummed one)
; a: n Bytes
; Ret: a = checksum
getChecksum:
    lda     #15  ; Number of bytes (Always 15)
    psha
    clra
moreCsum:
    add     ,x
    aix     #1
    dbnz    1, s, moreCsum
    ais     #1
    rts

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

checkIdentity:
    dbnza    checkProtect

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 5 - Loader identification

    bsr     afterIdStr  ; Ugly trick to get absolute address of string
.ascii "TXSUITE_E39MCP" ; Return address points at this string
afterIdStr:
; SP:
; 03: PCL   (bsr will push PCL, then PCH)
; 02: PCH
; 01: PCL   (Copied)

    ldx     2, s
    pshx

    tsx
    aix     #5  ; Increment past three pushed bytes, stepper and command
    lda     #14 ; Number of chars

    bsr     memcpy
    ais     #3

    bra commandOkOOR

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;

checkProtect:
    deca      ; 6 -> 7
    dbnza    jumprxError

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 7 - Flash protection
; yy 07 MM       00 00 00 00 00 00 00 00 00 00 00 00 00  
; MM: Protection mask
    ; H:X contains MM:00
    pshh
    pula
    sta     FLBPR
    bra     commandOkOOR



