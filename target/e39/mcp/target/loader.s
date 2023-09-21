.include 'inc/target.inc'

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Notes etc

; RAM starts at 0040
.area SYS (ABS)
.org 0x00b0

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
M_FLAGS          .equ    0x0042
M_RXCNT          .equ    0x0043
M_RETS           .equ    0x0044


; Exported symbols
.globl _entry  ; Entry point of this app. -Of no interest to the rest of the code but it's a nice thing to have in the map file

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; App

_entry:

; Stack frame
;   19: Reset countdown (How many consecutive unrecognised messages to accept before going into reset mode)
;   18: rx flags        (Unused atm)
;   17: rx count        (Number of bytes left to read before a complete frame has been assembled)
; 1-16: spi data        (SPI frame. tsx has a nifty trick up its sleeve so it'll automagically point here when using said instruction)
;    0: * do not use *  (Data is stored BEFORE the stack pointer is decremented. Thus, this is used when calling functions or pushing data)

; Beware. txs is sinister. sp = h:x - 1

    ldhx    #(LAST_STACK_ADDR - 21)
    txs

; Preconfigure cpu speed
; Not entirely sure what to do here...
; Recovery will set this value at boot up if the main firmware is missing but it's seemingly left alone if you directly enter the sauce mode from main
    mov     #0x08, *CPUSPD

; Prep tim1 for our use
; Freq == 2 Mhz / div 2 (1 MHz)
; Mod == 2000
    mov     #0x01, *T1SC    ; 2 MHz bus / 2 == 1 MHz
    ldhx    #2000
    sthx    *T1MOD

    bra     loopEntry_ResetErrCnt

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;  
; Another function inside the main loop because.. reasons

; sp: Source address
; h:X Destination address ( Swapped! )
; a: Count
memcpy:
    psha    ; Push count
    lda     4, s
    sta     *TEMP_0040
    lda     5, s
    sta     *TEMP_0041
; SP:
; 05: SOURCE L  <- To enable bsr trick <- notice me sensei, I will bite you!
; 04: SOURCE H
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


; Response handlers - We need these below 0x0100...
csumError:
    lda     #0x04
    bra     stErrResp

addrError:
; Received length out of bounds
lenError:
    lda     #0x20

stErrResp:
    sta     2, s

; Command error one way or another
rxErrorSPI:
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



; Check remaining error count. Die a fiery death if 0
    dbnz    *M_RETS, loopEntry_KeepErrCnt
deathLoop:          ; Wait for watchdog reset
    bra     deathLoop

; tsx etc takes less space than addressing stack items by offset
commandOk:
    tsx
    incx    ; Slightly naughy but we know stack resides betweent 200 and 23f
    lda     , x
    ora     #0x80
    sta     , x

commandOk_manual:

spiTimeoutErr:

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; Start of a new frame
loopEntry_ResetErrCnt:

    mov     #CONSEC_RX_TRIES, *M_RETS

loopEntry_KeepErrCnt:

; Beware. tsx is sinister. h:x = sp + 1
    tsx                    ; Point H:X at spi buffer start
    mov     #16, *M_RXCNT  ; Reset count
    clr     *M_FLAGS       ; Clear rx flags

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

    lda     *M_FLAGS
    bne     inRxLoop

    bset    #4, *T1SC  ; Reset counter
    lda     *T1SC      ; Read just to reset latch
    bclr    #7, *T1SC  ; Clear overflow bit

inRxLoop:

    sta     COPCTL    ; Pet watchdog. There, there. Nice doggo

    ; Check overflow / if more than 2 ms has passed since the last received byte (within this frame)
    ; That means we're somehow out of sync or the host stopped sending for one reason or another. Reset and wait for a new frame
    ; Can't use error counter since we could see in excess of 15 state resets 
    brset   #7, *T1SC, loopEntry_ResetErrCnt

    lda     *SPSCR    ; Load SPI flags register
    bit     #0x30     ; Check for error conditions; Overflow or mode fault
    bne     rxErrorSPI
    tsta
    bpl     rxLoop    ; Check reception flag

    ; Store any random junk in flag location to indicate "At least one byte received"
    sta     *M_FLAGS

    mov     *SPDR, x+ ; Read SPI and store received byte
    lda     ,x        ; Read next byte to send
    sta     *SPDR

    dbnz    *M_RXCNT, inRxLoop ; Decrement counter and continue if not 0

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Command interpretation

    tsx
    bsr     getChecksum
    cmp     ,x
    beq     msgSumOk
    jmp     *csumError
msgSumOk:

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
; yy 06 - <LOADER> Shutdown                           - implemented -
; yy 07 - <LOADER> Set flash protection mask          - implemented -
; yy 08 - <LOADER> Erase flash                        - implemented -
; YY 09 - <LOADER> Write data in buffer               - missing -

; SAUCE  = Commands that are present in the loader and boot sauce mode
; LOADER = Commands that are only available in the loader
; yy     = Stepper, 06 / 19 hex
;
; Todo:
; Determine if commands 02-05 are really needed in the loader? Can easily make it 128 bytes otherwise

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 1 - Idle message
    dbnza   checkRamRead

; yy 01     00 00 00 00 00 00 00 00 00 00 00 00 00 00
    jmp     *commandOk

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 2 - RAM Read
checkRamRead:
    dbnza   checkRamWrite

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
    pshx         ; Push source address
    pshh
    tsx          ; Load destination address
    aix     #7   ; Increment past pushed address, stepper, command, address and length
    jsr     *memcpy
    ais     #2
; Flag OK and store checksum
    tsx
    lda     #0x82
    sta     1, x

    bsr     getChecksum
    sta     ,x   ; Store checksum

; Must complete response before checksumming...
    jmp     *commandOk_manual

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 3 - RAM Write
checkRamWrite:
    dbnza   checkRamRun

; yy 03 HH LL NN     DD DD DD DD DD DD DD DD DD DD CC
; HH LL: 16-bit address
; NN   : Count 1 to 10
; DD   : Data
; CC   : Checkum-8 of the whole request, byte [0] to [14]
    lda     5, s     ; Read len
    beq     lenErrorOOR ; 0-len
    cmp     #11
    bcc     lenErrorOOR ; 11 or more bytes (error)

    tsx
    aix     #5

    pshx
    pshh

    lda     5, s
    psha
    pulh
    ldx     6, s

    lda     7, s ; Read len
    jsr     *memcpy

    ais     #2

    jmp     *commandOk

lenErrorOOR:
    jmp     *lenError


; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
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
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 4 - RAM Execute
checkRamRun:
    dbnza   checkIdentity

; yy 04 HH LL - Jump to address
; HH LL: 16-bit address
    jsr     ,x
    jmp     *commandOk ; Highly unlikely

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 5 - Loader identification
checkIdentity:
    dbnza   checkShutdown

    bsr     idStr_e     ; Ugly trick to get absolute address of string
idStr:
.ascii "TXSUITE_E39MCP" ; <- Return address points at this string
idStr_e:

    tsx
    aix     #4                 ; Increment past two pushed bytes, stepper and command
    lda     #(idStr_e - idStr) ; Number of chars

    jsr     *memcpy
    ais     #2

    jmp     *commandOk

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 6 - Shutdown
checkShutdown:
    dbnza   checkProtect
    jmp     *deathLoop

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 7 - Flash protection
checkProtect:
    dbnza   checkErase

; yy 07 MM       00 00 00 00 00 00 00 00 00 00 00 00 00
 
; MM: Protection mask
    ; H:X contains MM:00
    pshh
    pula
    sta     FLBPR
    jmp     *commandOk

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 8 - Erase flash
checkErase:
    dbnza   checkWrite

; yy 08 HH LL       00 00 00 00 00 00 00 00 00 00 00 00 
; HH LL: Start erase from 

; Set Page erase (0x00)
; The other mode is 0x40, full erase, but we don't want it
    clr     *CTRLBYT
    jsr     ERARNGE
    jmp     *commandOk

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; 9 - Write flash
checkWrite:
    dbnza   jumprxError

; Reminder of index in stack:
; 01 02 03 04 05
; yy 09 HH LL NN    00 00 00 00 00 00 00 00 00 00 00 00 
; HH LL: Start write from (must be aligned to nearest 32)
; NN   : Number of bytes

    ; Not fully checked! What you want to prevent is row overlap

    lda     5, s        ; Read len
    beq     lenErrorOOR ; 0-len
    cmp     #32
    bcc     lenErrorOOR ; 32 or more bytes (error)

    sthx    *LADDR  ; Store initial last address
    deca            ; From length to last index / address

; Check if LO + last idx overflows
    add     4, s
    sta     *(LADDR + 1)
    bcc     doWr
    inc     *(LADDR + 0)  ; Increment HI
doWr:
    jsr     PRGRNGE
    jmp     *commandOk

jumprxError:
    jmp     *rxError
