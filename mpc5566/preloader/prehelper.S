# # # # # # # # # # # # # # # # # # # #
# Main loader
.global mainloader

# # # # # # # # # # # # # # # # # # # #
# Interrupt
.global rebaseInterrupt
.global enableEE
.global intEntry

# # # # # # # # # # # # # # # # # # # #
# Timer facilities
.global ackTSR

# # # # # # # # # # # # # # # # # # # #
# ECC initialisation of SRAM
.global initECC_hi

# # # # # # # # # # # # # # # # # # # #
# "External" interrupts
enableEE:
    mfmsr %r3
    ori   %r3, %r3, 0x8000
    mtmsr %r3
blr

# # # # # # # # # # # # # # # # # # # #
# FIS
ackTSR:
    lis   %r3, 0x0400
    mtspr 336, %r3
blr

# # # # # # # # # # # # # # # # # # # #
# An interrupt vector jumped here, save everything and let the C code handle the rest
intEntry:
    stwu   %sp,-156(%sp)
    stw    %r0,   8(%sp) /* Backup everything except SP */
    stmw   %r2,  12(%sp)

    mfmsr  %r4
    oris   %r4, %r4, 0x200
    mtmsr  %r4

    mfsrr0 %r0          /* Save restore reg 0 */
    stw    %r0, 132(%sp)
    mfsrr1 %r0          /* Save restore reg 1 */
    stw    %r0, 136(%sp)
    mfctr  %r0          /* Counter reg        */
    stw    %r0, 140(%sp)
    mfxer  %r0          /* Integer exception  */
    stw    %r0, 144(%sp)
    mfcr   %r0          /* Condition reg      */
    stw    %r0, 148(%sp)
    mflr   %r0          /* Link reg           */
    stw    %r0, 152(%sp)

    # Service watchdog
    lis   %r4, 0x4000
    mtspr 336, %r4

    # r3 contains which vector
    bl hardArbiter

    # Check if a data read exception occured..
    or.    %r3, %r3, %r3
    lwz    %r2, 132(%sp)
    beq noDE

    lis    %r2, ReadDataFail@ha
    addi   %r2, %r2, ReadDataFail@l

noDE:
    mtsrr0 %r2          /* Save restore reg 0 */
    lwz    %r0, 136(%sp)
    mtsrr1 %r0          /* Save restore reg 1 */
    lwz    %r0, 140(%sp)
    mtctr  %r0          /* Counter reg        */
    lwz    %r0, 144(%sp)
    mtxer  %r0          /* Integer exception  */
    lwz    %r0, 148(%sp)
    mtcr   %r0          /* Condition reg      */
    lwz    %r0, 152(%sp)
    mtlr   %r0          /* Link reg           */

    lwz    %r0,   8(%sp)
    lmw    %r2,  12(%sp) /* Restore what we did */
    lwz    %r3, 156(%sp) /* Restore what the hardware vector did */
    addi   %sp, %sp, 160 /* add 4, since we have to restore what the hardware vector did */
rfi

# # # # # # # # # # # # # # # # # # # #
# Configure where the table is and set offsets for vectors
rebaseInterrupt:
    mtspr        63, %r3 /* Store base */

    li          %r3, 0x20
    mtspr     0x191, %r3 /* ivor 1, machine check exception. This is catastrophic! */

#   Read or write access control exception
#   Byte ordering exception
#   Cache locking exception (Not present)
#   External termination
    li          %r3, 0x30
    mtspr     0x192, %r3 /* ivor 2, Data storage */

    li          %r3, 0x40
    mtspr     0x193, %r3 /* ivor 3, Instruction */

    li          %r3, 0x50
    mtspr     0x194, %r3 /* ivor 4, External input */

    li          %r3, 0x70
    mtspr     0x195, %r3 /* ivor 5, Alignment exception */

    li          %r3, 0x80
    mtspr     0x196, %r3 /* ivor 6, Program exception */

    li          %r3, 0x90
    mtspr     0x197, %r3 /* ivor 7, Code tried to use missing float instructions */

    li          %r3, 0xA0
    mtspr     0x198, %r3 /* ivor 8, System Call (sc) instruction used */

    li          %r3, 0xB0
    mtspr     0x199, %r3 /* ivor 9, AP unavailable. (Not present in this processor core) */

    li          %r3, 0xC0
    mtspr     0x19A, %r3 /* ivor 10, Decrementer */

    li          %r3, 0xD0
    mtspr     0x19B, %r3 /* ivor 11, Fixed interval timer */

    li          %r3, 0xE0
    mtspr     0x19C, %r3 /* ivor 12, Watchdog is cranky and WILL reset the processor very soon if actions are not taken! */

    li          %r3, 0xF0
    mtspr     0x19D, %r3 /* ivor 13, Data TLB error */

    li          %r3, 0x100
    mtspr     0x19E, %r3 /* ivor 14, Instruction TLB error */

    li          %r3, 0x110
    mtspr     0x19F, %r3 /* ivor 15, Debug */

# ...

    li          %r3, 0x120
    mtspr     0x210, %r3 /* ivor 32, SPE instructions forbidden in this mode */

    li          %r3, 0x130
    mtspr     0x211, %r3 /* ivor 33, SPE floating point _DATA_ exception */

    li          %r3, 0x140
    mtspr     0x212, %r3 /* ivor 34, SPE floating point rounding error */

    sync
    isync

# Missing blr is on purpose to save space!

# # # # # # # # # # # # # # # # # # # #
# Fallback function; Return 0xff when the flash threw an ECC exception
ReadDataFail:
    li  %r3, 0xFF
blr

# # # # # # # # # # # # # # # # # # # #
# SRAM has HW ecc so it must be written to in a particular way to initialise things properly
# - Main firmare is known not to do all of it (since it's not using everything) so we go kinda overboard to make properly sure
initECC_hi:
    stwu    %sp,-16(%sp)
    stmw   %r30,  8(%sp)

    li     %r30, 0
    li     %r31, 0

    lis     %r3, __e_loader@h
    ori     %r3, %r3, __e_loader@l
    lis     %r4, 0x4002        /* end at 4002_0000 */

    # Watchdog bit
    lis     %r5, 0x4000

ECCinitHi:
    # Service watchdog
    mtspr   336, %r5

    stmw   %r30, 0(%r3)
    addi    %r3, %r3, 8
    cmpw    %r4, %r3
    bne    ECCinitHi

    lmw   %r30,   8(%sp)
    addi   %sp, %sp, 16
blr


.section .rodata
.align 4
mainloader:
.incbin "out/mainloader.lz"
