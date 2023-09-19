typedef unsigned char u8;
typedef unsigned short u16;

typedef struct {
    const u8 padding[ 8 ];

    // Control byte setting erase size
    u8  CTRLBYT;

    // CPU speed — the nearest integer of fop (in MHz) × 4; for example, if fop = 2.4576 MHz, CPUSPD = 10.
    // 8 MHz == 2 MHz bus == 2 * 4 = 8 (Also confirmed in firmware)
    u8  CPUSPD;

    // Last address of a 16-bit range
    u16 LADDR;

    // First location of DATA array;
    // DATA array size must match a programming or verifying range
    u8  buffer[ 32 ];
} romprm_t;

#define    rom_prm      ((romprm_t*) 0x0080)

void testFunc(void) {
    rom_prm->CPUSPD = 8;

    while (1) {  }
}
