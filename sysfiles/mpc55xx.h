// Partial MPC55xx header

#ifndef __MPC55XX_H__
#define __MPC55XX_H__

#define SIU_BASE         ( 0xC3F90000 )

#define FLEXCAN_A_BASE   ( 0xFFFC0000 )
#define FLEXCAN_B_BASE   ( 0xFFFC4000 )
#define FLEXCAN_C_BASE   ( 0xFFFC8000 )
#define FLEXCAN_D_BASE   ( 0xFFFCC000 )


///////////////////////////////////////////////////////////////////////////////////////
// SIU

// MCU ID register
#define SIU_MIDR         (*(volatile uint32_t *)     ( SIU_BASE + 0x0004 ))

//  System reset control register
#define SIU_SRCR         (*(volatile uint32_t *)     ( SIU_BASE + 0x0010 ))


///////////////////////////////////////////////////////////////////////////////////////
// FlexCAN

typedef struct {
    volatile uint32_t MDIS      :  1;        //  0    - Enable / Disable module ( 1 dis, 0 ena )
    volatile uint32_t FRZ       :  1;        //  1    - Request freeze mode
             uint32_t pad_2     :  1;        //  2
    volatile uint32_t HALT      :  1;        //  3    - Request halt mode bit
    volatile uint32_t NOTRDY    :  1;        //  4    - Flexcan busy/not ready bit
             uint32_t pad_5     :  1;        //  5
    volatile uint32_t SOFTRST   :  1;        //  6    - Soft reset. 1 triggers an internal reset to default values for most registers
    volatile uint32_t FRZACK    :  1;        //  7    - Freeze mode ack ( Is it in a frozen state or not )
             uint32_t pad_8_9   :  2;        //  8:9
    volatile uint32_t WRNEN     :  1;        // 10    - Warning interrupt enable
    volatile uint32_t MDISACK   :  1;        // 11    - Low power mode ack ( Is it enabled or disabled )
             uint32_t pad_12_13 :  2;        // 12:13
    volatile uint32_t SRXDIS    :  1;        // 14    - Self reception ( 1 Self rec dis, 0 self rec en) 
    volatile uint32_t MBFEN     :  1;        // 15    - Message buffer filter enable
             uint32_t pad_16_25 : 10;        // 16:25
    volatile uint32_t MAXMB     :  6;        // 26:31 - Max number of message boxes
} can_mcr_t;

typedef struct {
    volatile uint32_t PRESDIV   :  8;        //  0:7  - Prescaler division factor
    volatile uint32_t RJW       :  2;        //  8:9  - Resync jump width
    volatile uint32_t PSEG1     :  3;        // 10:12 - Phase segment 1
    volatile uint32_t PSEG2     :  3;        // 13:15 - Phase segment 2
    volatile uint32_t BOFFMSK   :  1;        // 16    - Bus off mask. (1 int if bus off)
    volatile uint32_t ERRMSK    :  1;        // 17    - Error mask    (1 int if error)
    volatile uint32_t CLK_SRC   :  1;        // 18    - Clock source (0 osc clk, 1 sys clk)
    volatile uint32_t LPB       :  1;        // 19    - Loop back mode ( 0 dis, 1 ena )
    volatile uint32_t TWRNMSK   :  1;        // 20    - Tx warning int en ( 1 ena, 0 dis )
    volatile uint32_t RWRNMSK   :  1;        // 21    - Rx warning int en ( 1 ena, 0 dis )
             uint32_t pad_22_23 :  2;        // 22:23
    volatile uint32_t SMP       :  1;        // 24    - Sampling mode ( 0 one samp, 1 three samples )
    volatile uint32_t BOFFREC   :  1;        // 25    - Bus off recovery ( 0 Auto recovery, 1 do not recover )
    volatile uint32_t TSYN      :  1;        // 26    - Timer sync ( 0 dis, 1 ena )
    volatile uint32_t LBUF      :  1;        // 27    - Lowest buffer transmit mode ( 0 lowest ID, 1 lowest buf number )
    volatile uint32_t LOM       :  1;        // 28    - Listen only mode ( 0 normal op, 1 listen only mode )
    volatile uint32_t PROPSEG   :  3;        // 29:31 - Propagation segment
} can_cr_t;

typedef struct {
             uint32_t pad_0_15  : 16;        //  0:15
    volatile uint32_t RXECTR    :  8;        // 16:23 - Rx error counter
    volatile uint32_t TXECTR    :  8;        // 24:31 - Tx error counter
} can_ecr_t;

typedef struct {
             uint32_t pad_0_13  : 14;        //  0:13
    volatile uint32_t TWRNINT   :  1;        // 14
    volatile uint32_t RWRNINT   :  1;        // 15
    volatile uint32_t BIT1ERR   :  1;        // 16
    volatile uint32_t BIT0ERR   :  1;        // 17
    volatile uint32_t ACKERR    :  1;        // 18
    volatile uint32_t CRCERR    :  1;        // 19
    volatile uint32_t FRMERR    :  1;        // 20
    volatile uint32_t STFERR    :  1;        // 21
    volatile uint32_t TXWRN     :  1;        // 22
    volatile uint32_t RXWRN     :  1;        // 23
    volatile uint32_t IDLE      :  1;        // 24
    volatile uint32_t TXRX      :  1;        // 25
    volatile uint32_t FLTCONF   :  2;        // 26:27
             uint32_t pad_28    :  1;        // 28
    volatile uint32_t BOFFINT   :  1;        // 29
    volatile uint32_t ERRINT    :  1;        // 30
             uint32_t pad_31    :  1;        // 31
} can_esr_t;

typedef struct {
    // 0x0000
    union {
        can_mcr_t         fields;
        volatile uint32_t direct;
    } MCR;
    // 0x0004
    union {
        can_cr_t          fields;
        volatile uint32_t direct;
    } CR;
    // 0x0008
    volatile uint32_t TIMER;
    // 0x000C
    const uint32_t pad;
    // 0x0010
    volatile uint32_t RXGMASK;
    // 0x0014
    volatile uint32_t RX14MASK;
    // 0x0018
    volatile uint32_t RX15MASK;
    // 0x001C
    union {
        can_ecr_t         fields;
        volatile uint32_t direct;
    } ECR;
    // 0x0020
    union {
        can_esr_t         fields;
        volatile uint32_t direct;
    } ESR;
    // 0x0024
    volatile uint32_t IMRH;
    // 0x0028
    volatile uint32_t IMRL;
    // 0x002C
    volatile uint32_t IFRH;
    // 0x0030
    volatile uint32_t IFRL;
} flexcan_t;

typedef struct {
    uint32_t pad_0_2   :  3;
    uint32_t ID        : 11;
    uint32_t pad_14_31 : 18;
} canid_std_t;

typedef struct {
    uint32_t pad_0_2   :  3;
    uint32_t ID        : 29;
} canid_ext_t;

typedef struct {
    // dw 0
    uint32_t pad_0_3   :  4;
    uint32_t CODE      :  4;
    uint32_t pad_8     :  1;
    uint32_t SRR       :  1;
    uint32_t IDE       :  1;
    uint32_t RTR       :  1;
    uint32_t LENGTH    :  4;
    uint32_t TIMESTAMP : 16;

    // dw 1
    union {
        canid_std_t STD;
        canid_ext_t EXT;
    } ID;

    uint8_t data[8];
} canbox_t;

#define CAN_A           (*(flexcan_t*)         ( FLEXCAN_A_BASE + 0x0000 ))
#define CAN_A_BOX       ((canbox_t*)           ( FLEXCAN_A_BASE + 0x0080 ))

#define CAN_B           (*(flexcan_t*)         ( FLEXCAN_B_BASE + 0x0000 ))
#define CAN_B_BOX       ((canbox_t*)           ( FLEXCAN_B_BASE + 0x0080 ))

#define CAN_C           (*(flexcan_t*)         ( FLEXCAN_C_BASE + 0x0000 ))
#define CAN_C_BOX       ((canbox_t*)           ( FLEXCAN_C_BASE + 0x0080 ))

#define CAN_D           (*(flexcan_t*)         ( FLEXCAN_D_BASE + 0x0000 ))
#define CAN_D_BOX       ((canbox_t*)           ( FLEXCAN_D_BASE + 0x0080 ))

#endif
