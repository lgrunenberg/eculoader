.global mdHash


# Special version of the read function to save some space

readMemmd:
    addi   %r19,    %r19, 1 /* One MUST increment manually before reading to make sure ecc exceptions can be recovered */
    lbz     %r3, -1(%r19)
blr

mdHash:
    stwu    %sp,          -164(%sp)
    mflr    %r0
    stmw   %r10,            76(%sp)
    stw     %r0,           168(%sp)

    lwz    %r19,    0(%r3)          /* Address */
    lwz     %r4,    4(%r3)          /* Count   */
    lwz     %r6,    8(%r3)          /* A       */
    lwz     %r7,   12(%r3)          /* B       */
    lwz     %r8,   16(%r3)          /* C       */
    lwz     %r9,   20(%r3)          /* D       */
    addi    %r5,      %r3,        8 /* Offset r5 to point at keys */
    addi   %r18,      %sp,       12 /* Buffer window              */
    li     %r20,                 16 /* Preload copy count         */

LoopOver:

    mtctr %r20
copyMore:
    bl readMemmd
    stb    %r3,  3(%r18)
    bl readMemmd
    stb    %r3,  2(%r18)
    bl readMemmd
    stb    %r3,  1(%r18)
    bl readMemmd
    stb    %r3,  0(%r18)
    addi  %r18, %r18, 4
    bdnz copyMore

    addi   %r18,                %sp, 12

    # Backing up A, B, C and D
    addi   %r12,                %r6,  0
    addi   %r13,                %r7,  0
    addi   %r14,                %r8,  0
    addi   %r15,                %r9,  0

    # # # # # STEP  0 # # # # #

    lwz    %r17,   0(%r18) 
    addis  %r12,     %r12,    55147

    # F  = D ^ (B & (C ^ D))
    andc   %r10,     %r15,     %r13
    and    %r16,     %r14,     %r13
    or     %r10,     %r10,     %r16

    # A = A + data[00] + K + F
    addi   %r12,     %r12,   -23432
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  7) + B
    rotlwi %r12,     %r12,        7
    add    %r12,     %r13,     %r12


    # # # # # STEP  1 # # # # #

    lwz    %r17,   4(%r18) 
    addis  %r15,     %r15,    59592

    # F  = C ^ (A & (B ^ C))
    andc   %r10,     %r14,     %r12
    and    %r16,     %r13,     %r12
    or     %r10,     %r10,     %r16

    # D = D + data[04] + K + F
    addi   %r15,     %r15,   -18602
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 12) + A
    rotlwi %r15,     %r15,       12
    add    %r15,     %r12,     %r15


    # # # # # STEP  2 # # # # #

    lwz    %r17,   8(%r18) 
    addis  %r14,     %r14,     9248

    # F  = B ^ (D & (A ^ B))
    andc   %r10,     %r13,     %r15
    and    %r16,     %r12,     %r15
    or     %r10,     %r10,     %r16

    # C = C + data[08] + K + F
    addi   %r14,     %r14,    28891
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 17) + D
    rotlwi %r14,     %r14,       17
    add    %r14,     %r15,     %r14


    # # # # # STEP  3 # # # # #

    lwz    %r17,  12(%r18) 
    addis  %r13,     %r13,    49598

    # F  = A ^ (C & (D ^ A))
    andc   %r10,     %r12,     %r14
    and    %r16,     %r15,     %r14
    or     %r10,     %r10,     %r16

    # B = B + data[12] + K + F
    addi   %r13,     %r13,   -12562
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 22) + C
    rotlwi %r13,     %r13,       22
    add    %r13,     %r14,     %r13


    # # # # # STEP  4 # # # # #

    lwz    %r17,  16(%r18) 
    addis  %r12,     %r12,    62844

    # F  = D ^ (B & (C ^ D))
    andc   %r10,     %r15,     %r13
    and    %r16,     %r14,     %r13
    or     %r10,     %r10,     %r16

    # A = A + data[16] + K + F
    addi   %r12,     %r12,     4015
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  7) + B
    rotlwi %r12,     %r12,        7
    add    %r12,     %r13,     %r12


    # # # # # STEP  5 # # # # #

    lwz    %r17,  20(%r18) 
    addis  %r15,     %r15,    18312

    # F  = C ^ (A & (B ^ C))
    andc   %r10,     %r14,     %r12
    and    %r16,     %r13,     %r12
    or     %r10,     %r10,     %r16

    # D = D + data[20] + K + F
    addi   %r15,     %r15,   -14806
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 12) + A
    rotlwi %r15,     %r15,       12
    add    %r15,     %r12,     %r15


    # # # # # STEP  6 # # # # #

    lwz    %r17,  24(%r18) 
    addis  %r14,     %r14,    43056

    # F  = B ^ (D & (A ^ B))
    andc   %r10,     %r13,     %r15
    and    %r16,     %r12,     %r15
    or     %r10,     %r10,     %r16

    # C = C + data[24] + K + F
    addi   %r14,     %r14,    17939
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 17) + D
    rotlwi %r14,     %r14,       17
    add    %r14,     %r15,     %r14


    # # # # # STEP  7 # # # # #

    lwz    %r17,  28(%r18) 
    addis  %r13,     %r13,    64839

    # F  = A ^ (C & (D ^ A))
    andc   %r10,     %r12,     %r14
    and    %r16,     %r15,     %r14
    or     %r10,     %r10,     %r16

    # B = B + data[28] + K + F
    addi   %r13,     %r13,   -27391
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 22) + C
    rotlwi %r13,     %r13,       22
    add    %r13,     %r14,     %r13


    # # # # # STEP  8 # # # # #

    lwz    %r17,  32(%r18) 
    addis  %r12,     %r12,    27009

    # F  = D ^ (B & (C ^ D))
    andc   %r10,     %r15,     %r13
    and    %r16,     %r14,     %r13
    or     %r10,     %r10,     %r16

    # A = A + data[32] + K + F
    addi   %r12,     %r12,   -26408
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  7) + B
    rotlwi %r12,     %r12,        7
    add    %r12,     %r13,     %r12


    # # # # # STEP  9 # # # # #

    lwz    %r17,  36(%r18) 
    addis  %r15,     %r15,    35653

    # F  = C ^ (A & (B ^ C))
    andc   %r10,     %r14,     %r12
    and    %r16,     %r13,     %r12
    or     %r10,     %r10,     %r16

    # D = D + data[36] + K + F
    addi   %r15,     %r15,    -2129
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 12) + A
    rotlwi %r15,     %r15,       12
    add    %r15,     %r12,     %r15


    # # # # # STEP 10 # # # # #

    lwz    %r17,  40(%r18) 
    addis  %r14,     %r14,    65535

    # F  = B ^ (D & (A ^ B))
    andc   %r10,     %r13,     %r15
    and    %r16,     %r12,     %r15
    or     %r10,     %r10,     %r16

    # C = C + data[40] + K + F
    addi   %r14,     %r14,    23473
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 17) + D
    rotlwi %r14,     %r14,       17
    add    %r14,     %r15,     %r14


    # # # # # STEP 11 # # # # #

    lwz    %r17,  44(%r18) 
    addis  %r13,     %r13,    35165

    # F  = A ^ (C & (D ^ A))
    andc   %r10,     %r12,     %r14
    and    %r16,     %r15,     %r14
    or     %r10,     %r10,     %r16

    # B = B + data[44] + K + F
    addi   %r13,     %r13,   -10306
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 22) + C
    rotlwi %r13,     %r13,       22
    add    %r13,     %r14,     %r13


    # # # # # STEP 12 # # # # #

    lwz    %r17,  48(%r18) 
    addis  %r12,     %r12,    27536

    # F  = D ^ (B & (C ^ D))
    andc   %r10,     %r15,     %r13
    and    %r16,     %r14,     %r13
    or     %r10,     %r10,     %r16

    # A = A + data[48] + K + F
    addi   %r12,     %r12,     4386
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  7) + B
    rotlwi %r12,     %r12,        7
    add    %r12,     %r13,     %r12


    # # # # # STEP 13 # # # # #

    lwz    %r17,  52(%r18) 
    addis  %r15,     %r15,    64920

    # F  = C ^ (A & (B ^ C))
    andc   %r10,     %r14,     %r12
    and    %r16,     %r13,     %r12
    or     %r10,     %r10,     %r16

    # D = D + data[52] + K + F
    addi   %r15,     %r15,    29075
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 12) + A
    rotlwi %r15,     %r15,       12
    add    %r15,     %r12,     %r15


    # # # # # STEP 14 # # # # #

    lwz    %r17,  56(%r18) 
    addis  %r14,     %r14,    42617

    # F  = B ^ (D & (A ^ B))
    andc   %r10,     %r13,     %r15
    and    %r16,     %r12,     %r15
    or     %r10,     %r10,     %r16

    # C = C + data[56] + K + F
    addi   %r14,     %r14,    17294
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 17) + D
    rotlwi %r14,     %r14,       17
    add    %r14,     %r15,     %r14


    # # # # # STEP 15 # # # # #

    lwz    %r17,  60(%r18) 
    addis  %r13,     %r13,    18868

    # F  = A ^ (C & (D ^ A))
    andc   %r10,     %r12,     %r14
    and    %r16,     %r15,     %r14
    or     %r10,     %r10,     %r16

    # B = B + data[60] + K + F
    addi   %r13,     %r13,     2081
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 22) + C
    rotlwi %r13,     %r13,       22
    add    %r13,     %r14,     %r13


    # # # # # STEP 16 # # # # #

    lwz    %r17,   4(%r18) 
    addis  %r12,     %r12,    63006

    # F  = C ^ (D & (B ^ C))
    andc   %r10,     %r14,     %r15
    and    %r16,     %r13,     %r15
    or     %r10,     %r10,     %r16

    # A = A + data[04] + K + F
    addi   %r12,     %r12,     9570
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  5) + B
    rotlwi %r12,     %r12,        5
    add    %r12,     %r13,     %r12


    # # # # # STEP 17 # # # # #

    lwz    %r17,  24(%r18) 
    addis  %r15,     %r15,    49217

    # F  = B ^ (C & (A ^ B))
    andc   %r10,     %r13,     %r14
    and    %r16,     %r12,     %r14
    or     %r10,     %r10,     %r16

    # D = D + data[24] + K + F
    addi   %r15,     %r15,   -19648
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D <<  9) + A
    rotlwi %r15,     %r15,        9
    add    %r15,     %r12,     %r15


    # # # # # STEP 18 # # # # #

    lwz    %r17,  44(%r18) 
    addis  %r14,     %r14,     9822

    # F  = A ^ (B & (D ^ A))
    andc   %r10,     %r12,     %r13
    and    %r16,     %r15,     %r13
    or     %r10,     %r10,     %r16

    # C = C + data[44] + K + F
    addi   %r14,     %r14,    23121
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 14) + D
    rotlwi %r14,     %r14,       14
    add    %r14,     %r15,     %r14


    # # # # # STEP 19 # # # # #

    lwz    %r17,   0(%r18) 
    addis  %r13,     %r13,    59831

    # F  = D ^ (A & (C ^ D))
    andc   %r10,     %r15,     %r12
    and    %r16,     %r14,     %r12
    or     %r10,     %r10,     %r16

    # B = B + data[00] + K + F
    addi   %r13,     %r13,   -14422
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 20) + C
    rotlwi %r13,     %r13,       20
    add    %r13,     %r14,     %r13


    # # # # # STEP 20 # # # # #

    lwz    %r17,  20(%r18) 
    addis  %r12,     %r12,    54831

    # F  = C ^ (D & (B ^ C))
    andc   %r10,     %r14,     %r15
    and    %r16,     %r13,     %r15
    or     %r10,     %r10,     %r16

    # A = A + data[20] + K + F
    addi   %r12,     %r12,     4189
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  5) + B
    rotlwi %r12,     %r12,        5
    add    %r12,     %r13,     %r12


    # # # # # STEP 21 # # # # #

    lwz    %r17,  40(%r18) 
    addis  %r15,     %r15,      580

    # F  = B ^ (C & (A ^ B))
    andc   %r10,     %r13,     %r14
    and    %r16,     %r12,     %r14
    or     %r10,     %r10,     %r16

    # D = D + data[40] + K + F
    addi   %r15,     %r15,     5203
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D <<  9) + A
    rotlwi %r15,     %r15,        9
    add    %r15,     %r12,     %r15


    # # # # # STEP 22 # # # # #

    lwz    %r17,  60(%r18) 
    addis  %r14,     %r14,    55458

    # F  = A ^ (B & (D ^ A))
    andc   %r10,     %r12,     %r13
    and    %r16,     %r15,     %r13
    or     %r10,     %r10,     %r16

    # C = C + data[60] + K + F
    addi   %r14,     %r14,    -6527
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 14) + D
    rotlwi %r14,     %r14,       14
    add    %r14,     %r15,     %r14


    # # # # # STEP 23 # # # # #

    lwz    %r17,  16(%r18) 
    addis  %r13,     %r13,    59348

    # F  = D ^ (A & (C ^ D))
    andc   %r10,     %r15,     %r12
    and    %r16,     %r14,     %r12
    or     %r10,     %r10,     %r16

    # B = B + data[16] + K + F
    addi   %r13,     %r13,    -1080
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 20) + C
    rotlwi %r13,     %r13,       20
    add    %r13,     %r14,     %r13


    # # # # # STEP 24 # # # # #

    lwz    %r17,  36(%r18) 
    addis  %r12,     %r12,     8674

    # F  = C ^ (D & (B ^ C))
    andc   %r10,     %r14,     %r15
    and    %r16,     %r13,     %r15
    or     %r10,     %r10,     %r16

    # A = A + data[36] + K + F
    addi   %r12,     %r12,   -12826
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  5) + B
    rotlwi %r12,     %r12,        5
    add    %r12,     %r13,     %r12


    # # # # # STEP 25 # # # # #

    lwz    %r17,  56(%r18) 
    addis  %r15,     %r15,    49975

    # F  = B ^ (C & (A ^ B))
    andc   %r10,     %r13,     %r14
    and    %r16,     %r12,     %r14
    or     %r10,     %r10,     %r16

    # D = D + data[56] + K + F
    addi   %r15,     %r15,     2006
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D <<  9) + A
    rotlwi %r15,     %r15,        9
    add    %r15,     %r12,     %r15


    # # # # # STEP 26 # # # # #

    lwz    %r17,  12(%r18) 
    addis  %r14,     %r14,    62677

    # F  = A ^ (B & (D ^ A))
    andc   %r10,     %r12,     %r13
    and    %r16,     %r15,     %r13
    or     %r10,     %r10,     %r16

    # C = C + data[12] + K + F
    addi   %r14,     %r14,     3463
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 14) + D
    rotlwi %r14,     %r14,       14
    add    %r14,     %r15,     %r14


    # # # # # STEP 27 # # # # #

    lwz    %r17,  32(%r18) 
    addis  %r13,     %r13,    17754

    # F  = D ^ (A & (C ^ D))
    andc   %r10,     %r15,     %r12
    and    %r16,     %r14,     %r12
    or     %r10,     %r10,     %r16

    # B = B + data[32] + K + F
    addi   %r13,     %r13,     5357
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 20) + C
    rotlwi %r13,     %r13,       20
    add    %r13,     %r14,     %r13


    # # # # # STEP 28 # # # # #

    lwz    %r17,  52(%r18) 
    addis  %r12,     %r12,    43492

    # F  = C ^ (D & (B ^ C))
    andc   %r10,     %r14,     %r15
    and    %r16,     %r13,     %r15
    or     %r10,     %r10,     %r16

    # A = A + data[52] + K + F
    addi   %r12,     %r12,    -5883
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  5) + B
    rotlwi %r12,     %r12,        5
    add    %r12,     %r13,     %r12


    # # # # # STEP 29 # # # # #

    lwz    %r17,   8(%r18) 
    addis  %r15,     %r15,    64752

    # F  = B ^ (C & (A ^ B))
    andc   %r10,     %r13,     %r14
    and    %r16,     %r12,     %r14
    or     %r10,     %r10,     %r16

    # D = D + data[08] + K + F
    addi   %r15,     %r15,   -23560
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D <<  9) + A
    rotlwi %r15,     %r15,        9
    add    %r15,     %r12,     %r15


    # # # # # STEP 30 # # # # #

    lwz    %r17,  28(%r18) 
    addis  %r14,     %r14,    26479

    # F  = A ^ (B & (D ^ A))
    andc   %r10,     %r12,     %r13
    and    %r16,     %r15,     %r13
    or     %r10,     %r10,     %r16

    # C = C + data[28] + K + F
    addi   %r14,     %r14,      729
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 14) + D
    rotlwi %r14,     %r14,       14
    add    %r14,     %r15,     %r14


    # # # # # STEP 31 # # # # #

    lwz    %r17,  48(%r18) 
    addis  %r13,     %r13,    36138

    # F  = D ^ (A & (C ^ D))
    andc   %r10,     %r15,     %r12
    and    %r16,     %r14,     %r12
    or     %r10,     %r10,     %r16

    # B = B + data[48] + K + F
    addi   %r13,     %r13,    19594
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 20) + C
    rotlwi %r13,     %r13,       20
    add    %r13,     %r14,     %r13


    # # # # # STEP 32 # # # # #

    lwz    %r17,  20(%r18) 
    addis  %r12,     %r12,    65530

    # F  = B ^ C ^ D
    xor    %r10,     %r14,     %r15
    xor    %r10,     %r10,     %r13

    # A = A + data[20] + K + F
    addi   %r12,     %r12,    14658
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  4) + B
    rotlwi %r12,     %r12,        4
    add    %r12,     %r13,     %r12


    # # # # # STEP 33 # # # # #

    lwz    %r17,  32(%r18) 
    addis  %r15,     %r15,    34674

    # F  = A ^ B ^ C
    xor    %r10,     %r13,     %r14
    xor    %r10,     %r10,     %r12

    # D = D + data[32] + K + F
    addi   %r15,     %r15,    -2431
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 11) + A
    rotlwi %r15,     %r15,       11
    add    %r15,     %r12,     %r15


    # # # # # STEP 34 # # # # #

    lwz    %r17,  44(%r18) 
    addis  %r14,     %r14,    28061

    # F  = D ^ A ^ B
    xor    %r10,     %r12,     %r13
    xor    %r10,     %r10,     %r15

    # C = C + data[44] + K + F
    addi   %r14,     %r14,    24866
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 16) + D
    rotlwi %r14,     %r14,       16
    add    %r14,     %r15,     %r14


    # # # # # STEP 35 # # # # #

    lwz    %r17,  56(%r18) 
    addis  %r13,     %r13,    64997

    # F  = C ^ D ^ A
    xor    %r10,     %r15,     %r12
    xor    %r10,     %r10,     %r14

    # B = B + data[56] + K + F
    addi   %r13,     %r13,    14348
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 23) + C
    rotlwi %r13,     %r13,       23
    add    %r13,     %r14,     %r13


    # # # # # STEP 36 # # # # #

    lwz    %r17,   4(%r18) 
    addis  %r12,     %r12,    42175

    # F  = B ^ C ^ D
    xor    %r10,     %r14,     %r15
    xor    %r10,     %r10,     %r13

    # A = A + data[04] + K + F
    addi   %r12,     %r12,    -5564
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  4) + B
    rotlwi %r12,     %r12,        4
    add    %r12,     %r13,     %r12


    # # # # # STEP 37 # # # # #

    lwz    %r17,  16(%r18) 
    addis  %r15,     %r15,    19423

    # F  = A ^ B ^ C
    xor    %r10,     %r13,     %r14
    xor    %r10,     %r10,     %r12

    # D = D + data[16] + K + F
    addi   %r15,     %r15,   -12375
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 11) + A
    rotlwi %r15,     %r15,       11
    add    %r15,     %r12,     %r15


    # # # # # STEP 38 # # # # #

    lwz    %r17,  28(%r18) 
    addis  %r14,     %r14,    63163

    # F  = D ^ A ^ B
    xor    %r10,     %r12,     %r13
    xor    %r10,     %r10,     %r15

    # C = C + data[28] + K + F
    addi   %r14,     %r14,    19296
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 16) + D
    rotlwi %r14,     %r14,       16
    add    %r14,     %r15,     %r14


    # # # # # STEP 39 # # # # #

    lwz    %r17,  40(%r18) 
    addis  %r13,     %r13,    48832

    # F  = C ^ D ^ A
    xor    %r10,     %r15,     %r12
    xor    %r10,     %r10,     %r14

    # B = B + data[40] + K + F
    addi   %r13,     %r13,   -17296
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 23) + C
    rotlwi %r13,     %r13,       23
    add    %r13,     %r14,     %r13


    # # # # # STEP 40 # # # # #

    lwz    %r17,  52(%r18) 
    addis  %r12,     %r12,    10395

    # F  = B ^ C ^ D
    xor    %r10,     %r14,     %r15
    xor    %r10,     %r10,     %r13

    # A = A + data[52] + K + F
    addi   %r12,     %r12,    32454
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  4) + B
    rotlwi %r12,     %r12,        4
    add    %r12,     %r13,     %r12


    # # # # # STEP 41 # # # # #

    lwz    %r17,   0(%r18) 
    addis  %r15,     %r15,    60065

    # F  = A ^ B ^ C
    xor    %r10,     %r13,     %r14
    xor    %r10,     %r10,     %r12

    # D = D + data[00] + K + F
    addi   %r15,     %r15,    10234
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 11) + A
    rotlwi %r15,     %r15,       11
    add    %r15,     %r12,     %r15


    # # # # # STEP 42 # # # # #

    lwz    %r17,  12(%r18) 
    addis  %r14,     %r14,    54511

    # F  = D ^ A ^ B
    xor    %r10,     %r12,     %r13
    xor    %r10,     %r10,     %r15

    # C = C + data[12] + K + F
    addi   %r14,     %r14,    12421
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 16) + D
    rotlwi %r14,     %r14,       16
    add    %r14,     %r15,     %r14


    # # # # # STEP 43 # # # # #

    lwz    %r17,  24(%r18) 
    addis  %r13,     %r13,     1160

    # F  = C ^ D ^ A
    xor    %r10,     %r15,     %r12
    xor    %r10,     %r10,     %r14

    # B = B + data[24] + K + F
    addi   %r13,     %r13,     7429
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 23) + C
    rotlwi %r13,     %r13,       23
    add    %r13,     %r14,     %r13


    # # # # # STEP 44 # # # # #

    lwz    %r17,  36(%r18) 
    addis  %r12,     %r12,    55765

    # F  = B ^ C ^ D
    xor    %r10,     %r14,     %r15
    xor    %r10,     %r10,     %r13

    # A = A + data[36] + K + F
    addi   %r12,     %r12,   -12231
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  4) + B
    rotlwi %r12,     %r12,        4
    add    %r12,     %r13,     %r12


    # # # # # STEP 45 # # # # #

    lwz    %r17,  48(%r18) 
    addis  %r15,     %r15,    59100

    # F  = A ^ B ^ C
    xor    %r10,     %r13,     %r14
    xor    %r10,     %r10,     %r12

    # D = D + data[48] + K + F
    addi   %r15,     %r15,   -26139
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 11) + A
    rotlwi %r15,     %r15,       11
    add    %r15,     %r12,     %r15


    # # # # # STEP 46 # # # # #

    lwz    %r17,  60(%r18) 
    addis  %r14,     %r14,     8098

    # F  = D ^ A ^ B
    xor    %r10,     %r12,     %r13
    xor    %r10,     %r10,     %r15

    # C = C + data[60] + K + F
    addi   %r14,     %r14,    31992
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 16) + D
    rotlwi %r14,     %r14,       16
    add    %r14,     %r15,     %r14


    # # # # # STEP 47 # # # # #

    lwz    %r17,   8(%r18) 
    addis  %r13,     %r13,    50348

    # F  = C ^ D ^ A
    xor    %r10,     %r15,     %r12
    xor    %r10,     %r10,     %r14

    # B = B + data[08] + K + F
    addi   %r13,     %r13,    22117
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 23) + C
    rotlwi %r13,     %r13,       23
    add    %r13,     %r14,     %r13


    # # # # # STEP 48 # # # # #

    lwz    %r17,   0(%r18) 
    addis  %r12,     %r12,    62505

    # F  = C ^ (B | (~D))
    orc    %r10,     %r13,     %r15
    xor    %r10,     %r10,     %r14

    # A = A + data[00] + K + F
    addi   %r12,     %r12,     8772
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  6) + B
    rotlwi %r12,     %r12,        6
    add    %r12,     %r13,     %r12


    # # # # # STEP 49 # # # # #

    lwz    %r17,  28(%r18) 
    addis  %r15,     %r15,    17195

    # F  = B ^ (A | (~C))
    orc    %r10,     %r12,     %r14
    xor    %r10,     %r10,     %r13

    # D = D + data[28] + K + F
    addi   %r15,     %r15,     -105
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 10) + A
    rotlwi %r15,     %r15,       10
    add    %r15,     %r12,     %r15


    # # # # # STEP 50 # # # # #

    lwz    %r17,  56(%r18) 
    addis  %r14,     %r14,    43924

    # F  = A ^ (D | (~B))
    orc    %r10,     %r15,     %r13
    xor    %r10,     %r10,     %r12

    # C = C + data[56] + K + F
    addi   %r14,     %r14,     9127
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 15) + D
    rotlwi %r14,     %r14,       15
    add    %r14,     %r15,     %r14


    # # # # # STEP 51 # # # # #

    lwz    %r17,  20(%r18) 
    addis  %r13,     %r13,    64660

    # F  = D ^ (C | (~A))
    orc    %r10,     %r14,     %r12
    xor    %r10,     %r10,     %r15

    # B = B + data[20] + K + F
    addi   %r13,     %r13,   -24519
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 21) + C
    rotlwi %r13,     %r13,       21
    add    %r13,     %r14,     %r13


    # # # # # STEP 52 # # # # #

    lwz    %r17,  48(%r18) 
    addis  %r12,     %r12,    25947

    # F  = C ^ (B | (~D))
    orc    %r10,     %r13,     %r15
    xor    %r10,     %r10,     %r14

    # A = A + data[48] + K + F
    addi   %r12,     %r12,    22979
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  6) + B
    rotlwi %r12,     %r12,        6
    add    %r12,     %r13,     %r12


    # # # # # STEP 53 # # # # #

    lwz    %r17,  12(%r18) 
    addis  %r15,     %r15,    36621

    # F  = B ^ (A | (~C))
    orc    %r10,     %r12,     %r14
    xor    %r10,     %r10,     %r13

    # D = D + data[12] + K + F
    addi   %r15,     %r15,   -13166
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 10) + A
    rotlwi %r15,     %r15,       10
    add    %r15,     %r12,     %r15


    # # # # # STEP 54 # # # # #

    lwz    %r17,  40(%r18) 
    addis  %r14,     %r14,    65520

    # F  = A ^ (D | (~B))
    orc    %r10,     %r15,     %r13
    xor    %r10,     %r10,     %r12

    # C = C + data[40] + K + F
    addi   %r14,     %r14,    -2947
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 15) + D
    rotlwi %r14,     %r14,       15
    add    %r14,     %r15,     %r14


    # # # # # STEP 55 # # # # #

    lwz    %r17,   4(%r18) 
    addis  %r13,     %r13,    34180

    # F  = D ^ (C | (~A))
    orc    %r10,     %r14,     %r12
    xor    %r10,     %r10,     %r15

    # B = B + data[04] + K + F
    addi   %r13,     %r13,    24017
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 21) + C
    rotlwi %r13,     %r13,       21
    add    %r13,     %r14,     %r13


    # # # # # STEP 56 # # # # #

    lwz    %r17,  32(%r18) 
    addis  %r12,     %r12,    28584

    # F  = C ^ (B | (~D))
    orc    %r10,     %r13,     %r15
    xor    %r10,     %r10,     %r14

    # A = A + data[32] + K + F
    addi   %r12,     %r12,    32335
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  6) + B
    rotlwi %r12,     %r12,        6
    add    %r12,     %r13,     %r12


    # # # # # STEP 57 # # # # #

    lwz    %r17,  60(%r18) 
    addis  %r15,     %r15,    65069

    # F  = B ^ (A | (~C))
    orc    %r10,     %r12,     %r14
    xor    %r10,     %r10,     %r13

    # D = D + data[60] + K + F
    addi   %r15,     %r15,    -6432
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 10) + A
    rotlwi %r15,     %r15,       10
    add    %r15,     %r12,     %r15


    # # # # # STEP 58 # # # # #

    lwz    %r17,  24(%r18) 
    addis  %r14,     %r14,    41729

    # F  = A ^ (D | (~B))
    orc    %r10,     %r15,     %r13
    xor    %r10,     %r10,     %r12

    # C = C + data[24] + K + F
    addi   %r14,     %r14,    17172
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 15) + D
    rotlwi %r14,     %r14,       15
    add    %r14,     %r15,     %r14


    # # # # # STEP 59 # # # # #

    lwz    %r17,  52(%r18) 
    addis  %r13,     %r13,    19976

    # F  = D ^ (C | (~A))
    orc    %r10,     %r14,     %r12
    xor    %r10,     %r10,     %r15

    # B = B + data[52] + K + F
    addi   %r13,     %r13,     4513
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 21) + C
    rotlwi %r13,     %r13,       21
    add    %r13,     %r14,     %r13


    # # # # # STEP 60 # # # # #

    lwz    %r17,  16(%r18) 
    addis  %r12,     %r12,    63315

    # F  = C ^ (B | (~D))
    orc    %r10,     %r13,     %r15
    xor    %r10,     %r10,     %r14

    # A = A + data[16] + K + F
    addi   %r12,     %r12,    32386
    add    %r12,     %r12,     %r17
    add    %r12,     %r12,     %r10

    # A  = (A <<  6) + B
    rotlwi %r12,     %r12,        6
    add    %r12,     %r13,     %r12


    # # # # # STEP 61 # # # # #

    lwz    %r17,  44(%r18) 
    addis  %r15,     %r15,    48443

    # F  = B ^ (A | (~C))
    orc    %r10,     %r12,     %r14
    xor    %r10,     %r10,     %r13

    # D = D + data[44] + K + F
    addi   %r15,     %r15,    -3531
    add    %r15,     %r15,     %r17
    add    %r15,     %r15,     %r10

    # D  = (D << 10) + A
    rotlwi %r15,     %r15,       10
    add    %r15,     %r12,     %r15


    # # # # # STEP 62 # # # # #

    lwz    %r17,   8(%r18) 
    addis  %r14,     %r14,    10968

    # F  = A ^ (D | (~B))
    orc    %r10,     %r15,     %r13
    xor    %r10,     %r10,     %r12

    # C = C + data[08] + K + F
    addi   %r14,     %r14,   -11589
    add    %r14,     %r14,     %r17
    add    %r14,     %r14,     %r10

    # C  = (C << 15) + D
    rotlwi %r14,     %r14,       15
    add    %r14,     %r15,     %r14


    # # # # # STEP 63 # # # # #

    lwz    %r17,  36(%r18) 
    addis  %r13,     %r13,    60295

    # F  = D ^ (C | (~A))
    orc    %r10,     %r14,     %r12
    xor    %r10,     %r10,     %r15

    # B = B + data[36] + K + F
    addi   %r13,     %r13,   -11375
    add    %r13,     %r13,     %r17
    add    %r13,     %r13,     %r10

    # B  = (B << 21) + C
    rotlwi %r13,     %r13,       21
    add    %r13,     %r14,     %r13


    # Updating A, B, C and D
    add     %r6,      %r6,     %r12
    add     %r7,      %r7,     %r13
    add     %r8,      %r8,     %r14
    add     %r9,      %r9,     %r15

    addic.  %r4,      %r4,      -64
    bne                    LoopOver

    # Saving A, B, C and D
    # stswi   %r6,      %r5,       16 /* This processor / compiler flags combination goes berserk with this */
    stw %r6,  0(%r5)
    stw %r7,  4(%r5)
    stw %r8,  8(%r5)
    stw %r9, 12(%r5)

    # Restoring registers and stack
    lwz     %r0,           168( %sp)
    mtlr    %r0
    lmw    %r10,            76( %sp)
    addi    %sp,      %sp,      164
    blr

