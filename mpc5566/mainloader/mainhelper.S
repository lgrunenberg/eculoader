# # # # # # # # # # # # # # # # # # # #
# _USE_ this one to read flash. No exception!
# Reason: The processor goes berserk and throws an ECC exception if you read something that has ECC corruption (semi-erased block, broken etc).
# It'll get stuck in a never ending loop if actions are not taken so you have two options:
# 1. Restart the bootloader, drop a bunch of frames, read the same address again and crash.. again.
# 2. Do some trickery with pc and lr. (this is why you must ALWAYS use this function)
# It'll patch PC to point at "ReadDataFail" and expect LR to contain the correct return address.
.global ReadData

.global waitTimerZero
.global configureTBL
.global loaderID

# # # # # # # # # # # # # # # # # # # #
# Use DEC for general delay purposes
waitTimerZero:
    mfspr %r3, 22 
    or.   %r3, %r3, %r3
    bne waitTimerZero
blr

# # # # # # # # # # # # # # # # # # # #
# Read data. "ReadDataFail" can be found in the "prehelper.S"
ReadData:
    lbz %r3, 0(%r3)
blr

# # # # # # # # # # # # # # # # # # # #
# Restore TBL config to sane values (fucking e39...)
configureTBL:

    li        %r3, 5
    mtctr     %r3
    lis       %r3, MMUDATA@h
    ori       %r3, %r3, MMUDATA@l

# mas0: 624
# mas1: 625
# mas2: 626
# mas3: 627
# mas4: 628
# mas6: 630

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
    bdnz      MMU_InitL

# Nuke ALL other pages!

    li        %r3, 27
    mtctr     %r3
    lis       %r3, 0x1004
    li        %r4, 0

MMU_NukeL:
    addis     %r3, %r3, 1
    mtspr     624, %r3
    mtspr     625, %r4
    mtspr     626, %r4
    mtspr     627, %r4
    sync
    isync
    tlbwe
    isync
    bdnz      MMU_NukeL

    tlbsync

# The table needs 4 bytes of padding so this did the trick
MMUDATA:     
blr

# # # # # # # # # # # # # # # # # # # #
# Vanilla H/W standard used by the BAM code.
# If it's good enough for freescale, it's good enough for us.
# Restore defaults for page 3 before the other ones to circumvent a crash on e39
.long   0x10030000,   0xC0000400,   0x40000008,   0x4000003F

.long   0x10000000,   0xC0000500,   0xFFF0000A,   0xFFF0003F
.long   0x10010000,   0xC0000700,            0,         0x3F
.long   0x10020000,   0xC0000700,   0x20000000,         0x3F
# .long 0x10030000,   0xC0000400,   0x40000008,   0x4000003F
.long   0x10040000,   0xC0000500,   0xC3F0000A,   0xC3F0003F

# # # # # # # # # # # # # # # # # # # #
# loaderID[] = { "MPC5566-LOADER: TXSUITE.ORG" };
loaderID: .byte 27
.ascii "MPC5566-LOADER: TXSUITE.ORG"
