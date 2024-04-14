; MC68HC908QC16
.hc08

; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; Internal registers

; Port registers
PTA       .equ    0x0000    ; Port A Data register

; SPI registers
SPCR      .equ    0x000d    ; SPI Control register
SPSCR     .equ    0x000e    ; SPI Status and Control register
SPDR      .equ    0x000f    ; SPI Data register

; TIM1 registers
T1SC      .equ    0x0020    ; TIM1 Status and Control register
T1MOD     .equ    0x0023    ; TIM1 Modulo register
T1SC2     .equ    0x0030    ; TIM1 Channel 2 Status and Control register

OSCSC     .equ    0x0036    ; Oscillator Status and Control Register
OSCTRIM   .equ    0x0038    ; Oscillator Trim Register
OSC_TRIM  .equ    0xffc0    ; Internal Oscillator Trim Value

; TIM2 registers
T2SC      .equ    0x0240    ; TIM2 Status and Control register

FLBPR     .equ    0xffbe    ; FLAS Block Protect register

COPCTL    .equ    0xffff    ; Watchdog controller. Write any to reset counter



; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ; ;
; ROM stuff
; Routine pointers are UNIQUE to MC68HC908QC16 / QC8!
; MCU Name            RAM   GetByte   PutByte   RDVRRNG   PRGRNGE   ERARNGE   DELNUS
; MC68HC908LB8        $80   $037E     $0381     $0384     $038A     $0387     $038D
; MC68HC908QL4        $80   $2B7E     $2B81     $2B84     $2B8A     $2B87     $2B8D
; MC68HC908QY4A **    $80   $2800     $280F     $2803     $2809     $2806     $280C
; MC68HC908QB4/QB8    $80   $2800     $280F     $2803     $2809     $2806     $280C
; MC68HC908QY8        $80   $2800     $280F     $2803     $2809     $2806     $280C
; MC68HC908QC16/QC8   $80   $2800     $280F     $2803     $2809     $2806     $280C
; ** MC68HC908QY4A Series includes MC68HC908QY4A, MC68HC908QY2A, MC68HC908QY1A, MC68HC908QT4A, MC68HC908QT2A, MC68HC908QT1A
GetByte   .equ    0x2800    ; Get serial byte
PutByte   .equ    0x280F    ; Put serial byte
RDVRRNG   .equ    0x2803    ; Verify written data
ERARNGE   .equ    0x2806    ; Erase flash page  (64 bytes)
PRGRNGE   .equ    0x2809    ; Program flash row (32 bytes)
DELNUS    .equ    0x280C    ; Delay routine

; ROM routine data structure
ROM_STRUC .equ    0x0080    ; Base of structure
CTRLBYT   .equ    (ROM_STRUC + 0x08)  ; Control byte setting erase size
CPUSPD    .equ    (ROM_STRUC + 0x09)  ; CPU speed, nearest integer in MHz
LADDR     .equ    (ROM_STRUC + 0x0a)  ; Hi byte, Last address of a 16-bit range
LADDR_H   .equ    (ROM_STRUC + 0x0a)  ; Hi byte, Last address of a 16-bit range
LADDR_L   .equ    (ROM_STRUC + 0x0b)  ; Lo byte, Last address of a 16-bit range
ROM_BUF   .equ    (ROM_STRUC + 0x0c)  ; First location of DATA array
