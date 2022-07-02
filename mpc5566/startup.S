# # # # # # # # # # # # # # # # # # # #
# True entry point. Better do some remodeling of this place!
.global _Reset
.global modeWord

# Branch to link to figure out where we are
_Reset:
    bl OurBase

# Patched by the host to tell us what we're running on (if need be)
modeWord: .long 0xFFFFFFFF

OurBase:

    # -:Read TCR:-
    # Flip WP to increase period
    # Ditto for WPEXT
    # Flip WIE
    # Flip DIE
    # Flip FIE
    # Flip DEC
    mfspr   %r4, 340
    addis   %r3, %r0, 0xCCDE
    andc    %r4, %r4, %r3
    mtspr   340, %r4

    # Since these d***heads had to reinvent how to count...
    # TCR:
    # wp:    31:30 /* Watchdog timer period           */
    # WRC:   29:28 /* Watchdog reset control, WO      */
    # WIE:   27    /* Watchdog interrupt ena          */
    # DIE:   26    /* Decrementer interrupt enable    */
    # FP:    25:24 /* Fixed interval timer period     */
    # FIE:   23    /* Fixed interval interrupt ena    */
    # ARE:   22    /* DEC auto-reload                 */
    # WPEXT: 20:17 /* Watchdog extension              */
    # FPEXT: 16:13 /* Fixed-interval extension        */

    # -:Read MSR:-
    # Flip WE
    # Flip CE
    # Flip EE
    # ME (Better to take an interrupt than to kill ourself)
    mfmsr   %r4
    lis     %r3, 6
    ori     %r3, %r3, 0xEB30
    andc    %r4, %r4, %r3
    ori     %r4, %r4, 0x1000 /* ME   */
    oris    %r4, %r4, 0x0200 /* SPE  */
    mtmsr   %r4

    # MSR:
    # UCLE:  26    /* User cache lock ena             */
    # SPE:   25    /* SPE available                   */
    # WE:    18    /* Wait state (Power management)   */
    # CE:    17    /* Critical Int ena                */
    # EE:    15    /* External int ena                */
    # PR:    14    /* 0 = supervisor, 1 = user        */
    # FP:    13    /* Floating point ena. should be 0 */
    # ME:    12    /* Machine check ena               */
    # FE0:   11    /* Float except mode 0. not used   */
    # DE:     9    /* Debug int ena                   */
    # FE1:    8    /* Float except mode 1. not used   */
    # IS:     5    /* Fetch ins from TLB space 0 or 1 */
    # DS:     4    /* Fetch dat from TLB space 0 or 1 */

#ifdef BAMMODE

    # ECC initialization of SRAM
    lis     %r3, 0x4000
    li      %r4, 0x0080 /* end @ Offset 0x4000 */
    mtctr   %r4
    addis   %r6, %r0, 0x4000

ECCinitLow:
    stmw    %r0, 0(%r3)
    addi    %r3, %r3, 128
    mtspr   336, %r6
    bdnz ECCinitLow


    lis     %r3, __LoaderEnd@h
    ori     %r3, %r3, __LoaderEnd@l
    lis     %r4, 0x4002

ECCinitHi:
    stmw   %r30, 0(%r3)
    addi    %r3, %r3, 8
    mtspr   336, %r6
    cmpw    %r4, %r3
    bne ECCinitHi

#endif

#    make sure to disable branch control unit (bucsr)

    # Turn of cache since it's just a _PAIN IN THE F*CKING ASS_ to tame. Plenty fast any way...
    # Alternative two is to invalidate it every time flash is written or erased
    li      %r3, 0
    sync
    isync
    mtspr   1010, %r3

    # mflr    %r4
    # addi    %sp, %r4, -20 /* Set stack to be our address -20 */

    # Set stack to 0x40002000
    lis     %sp, 0x4000
    ori     %sp, %sp, 0x1fe8

    # -:No need to set up SDA:-

# Done! Your turn, C code.
    bl loaderEntry
