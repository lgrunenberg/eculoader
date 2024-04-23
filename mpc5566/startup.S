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

    # Determine where start of loader is
    mflr    %r31
    addi    %r31, %r31, -4

    # -:Read TCR:-
    # Flip WP to increase period
    # Ditto for WPEXT
    # Flip WIE
    # Flip DIE
    # Flip FIE
    # Flip DEC
    mfspr    %r4, 340
    lis      %r3, 0xCCDE
    andc     %r4, %r4, %r3
    mtspr    340, %r4

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
    # Clear
    # 18 WE
    # 17 CE
    # 15 EE
    # 14 PR
    # 13 FP
    #  8 FE1
    #  5 IS
    #  4 DS
    mfmsr    %r4
    lis      %r3, 6
    ori      %r3, %r3, 0xEB30
    andc     %r4, %r4, %r3    /* One's complement AND, bits set to 1 in r3 are cleared from r4 */
    ori      %r4, %r4, 0x1000 /* ME   */
    oris     %r4, %r4, 0x0200 /* SPE  */
    mtmsr    %r4

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

    # Remap regions to known safe ones
    bl configureTBL

    # Set stack to 0x4000_1fe8
    lis      %sp, 0x4000
    ori      %sp, %sp, 0x1fe8


    # Low portion of SRAM is almost guaranteed to be initialised in regular mode since it's used for battery backup et al so this is not necessary in regular mode
#ifdef BAMMODE

    # ECC initialization of SRAM
    lis      %r3, 0x4000
    li       %r4, (0x4000 / 128) /* end @ Offset 0x4000 */
    mtctr    %r4
    lis      %r6, 0x4000

ECCinitLow:
    stmw     %r0, 0(%r3)
    addi     %r3, %r3, 128
    mtspr    336, %r6
    bdnz ECCinitLow

#endif

#    make sure to disable branch control unit (bucsr)

    # Turn off cache to make things slightly less annoying to deal with
    # Alternative two is to invalidate it every time flash is written or erased
    li       %r3, 0
    sync
    isync
    mtspr   1010, %r3

    # -:No need to set up SDA:-

    # Send base address to main
    mr       %r3, %r31

# Done! Your turn, C code.
    b loaderEntry

# # # # # # # # # # # # # # # # # # # #
# Restore TBL config to sane values (e39...)
configureTBL:

    li        %r3, (eMMU - sMMU) / 16
    mtctr     %r3
    lis       %r3, MMUDATA@h
    ori       %r3, %r3, MMUDATA@l

# mas0: 624
# mas1: 625
# mas2: 626
# mas3: 627
# mas4: 628
# mas6: 630

# MMUCSR0:  1012
# MMUCFG :  1015
# TLB0CFG:   688
# TLB1CFG:   689

MMU_InitL:
    lwzu      %r4, 4(%r3)
    mtspr     624, %r4
    lwzu      %r4, 4(%r3)
    mtspr     625, %r4
    lwzu      %r4, 4(%r3)
    mtspr     626, %r4
    lwzu      %r4, 4(%r3)
    mtspr     627, %r4
    sync
    isync
    tlbwe
    isync
    bdnz MMU_InitL

# Disable all other pages

    li        %r3, 32 - ((eMMU - sMMU) / 16)
    mtctr     %r3
    lis       %r3, 0x1000 + ((eMMU - sMMU) / 16)
    li        %r4, 0

MMU_NukeL:
    mtspr     624, %r3
    mtspr     625, %r4
    mtspr     626, %r4
    mtspr     627, %r4
    addis     %r3, %r3, 1
    sync
    isync
    tlbwe
    isync
    bdnz MMU_NukeL

    tlbsync

# The table needs 4 bytes of padding so this did the trick
MMUDATA:     
blr

# # # # # # # # # # # # # # # # # # # #
# Vanilla H/W standard used by the BAM code.
# If it's good enough for freescale, it's good enough for us.
# Restore defaults for page 3 before the other ones to circumvent a crash on e39
sMMU:
#       MAS0          MAS1          MAS2          MAS3
.long   0x10030000,   0xC0000400,   0x40000008,   0x4000003F     /*   Page 3   */
.long   0x10000000,   0xC0000500,   0xFFF0000A,   0xFFF0003F     /*   Page 0   */
.long   0x10010000,   0xC0000700,            0,         0x3F     /*   Page 1   */
.long   0x10020000,   0xC0000700,   0x20000000,         0x3F     /*   Page 2   */
# .long 0x10030000,   0xC0000400,   0x40000008,   0x4000003F     /*   Page 3   */
.long   0x10040000,   0xC0000500,   0xC3F0000A,   0xC3F0003F     /*   Page 4   */
eMMU:
