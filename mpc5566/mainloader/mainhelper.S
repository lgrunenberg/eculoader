# # # # # # # # # # # # # # # # # # # #
# _USE_ this one to read flash. No exception!
# Reason: The processor goes berserk and throws an ECC exception if you read something that has ECC corruption (semi-erased block, broken etc).
# It'll get stuck in a never ending loop if actions are not taken so you have two options:
# 1. Restart the bootloader, drop a bunch of frames, read the same address again and crash.. again.
# 2. Do some trickery with pc and lr. (this is why you must ALWAYS use this function)
# It'll patch PC to point at "ReadDataFail" and expect LR to contain the correct return address.
.global ReadData

.global waitTimerZero
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
# loaderID[] = { "MPC5566-LOADER: TXSUITE.ORG" };
loaderID: .byte 27
.ascii "MPC5566-LOADER: TXSUITE.ORG"
