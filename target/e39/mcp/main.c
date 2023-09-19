#include "common.h"

#include "misc.h"

// rand
#include <stdlib.h>


#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"
/*
7







*/


static void RCC_Configuration()
{
    RCC_APB2PeriphClockCmd(
        0x0101D |
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_GPIOB
        , ENABLE); // GPIO A,B,C, SPI, AFIO for SPI

    // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(
        0x24005 |
        RCC_APB1Periph_USART2
        , ENABLE); // Usart 2, spi, spi afio, more spi
}

static void init_debugUart()
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}


void InitSPI(void)
{
    SPI_I2S_DeInit(SPI2);
    SPI_InitTypeDef   SPI_InitStructure;

    SPI_InitStructure.SPI_Direction     = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode          = SPI_Mode_Master;
    SPI_InitStructure.SPI_NSS           = SPI_NSS_Soft;
    SPI_InitStructure.SPI_CRCPolynomial = 0;

/*
    SPI_InitStructure.SPI_FirstBit      = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_DataSize      = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL          = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA          = SPI_CPHA_2Edge;
*/
    SPI_InitStructure.SPI_FirstBit      = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_DataSize      = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL          = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA          = SPI_CPHA_2Edge;

    // 48 / 64 = 0.75 MHz
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;

    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_CalculateCRC(SPI2, DISABLE);
    SPI_Cmd(SPI2, ENABLE);
}

static void init_Timeout()
{
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_Cmd(TIM2,DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void InitSys()
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_Configuration();

    init_debugUart(); // Debug uart

    /////////////////////////////////////////////////////////////////////
    // SPI 2
    // Chip select
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIOB->BSRR = (1UL << 12); // Set CS high
    // BRR low

    // SCK / MOSI
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // MISO
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    InitSPI();

    /////////////////////////////////////////////////////////////////////
    // Busy signal from MCP
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);













    init_Timeout();

    // Enable DWT timer
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}


extern void mcpPlay(void);


int main(void) {

    InitSys();


    // srand (time(NULL));

    printf("Test\n\r");

    mcpPlay();

    while ( 1 ) {


    }

    return 0;
}
