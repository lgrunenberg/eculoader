#include "common.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"


#define DEBUGUART  USART2


static volatile uint32_t m_timeout;

void set_Timeout(const uint16_t ms);


void uart_putchar(char ch, FILE *f) {
	while(USART_GetFlagStatus(DEBUGUART, USART_FLAG_TXE) == RESET)  ;
	USART_SendData(DEBUGUART, ch);
}

char uart_getchar() {
	while(!USART_GetFlagStatus(DEBUGUART, USART_FLAG_RXNE))  ;
	return (DEBUGUART->DR)&0xFF;
}

int fputc(int ch, FILE *f) {
	if (ch == '\n') { fputc('\r', f); }
	while(USART_GetFlagStatus(DEBUGUART, USART_FLAG_TXE) == RESET)  ;
	USART_SendData(DEBUGUART, ch);
	return(ch);
}


void sleep(const uint16_t ms)
{
    set_Timeout(ms);
    while (!m_timeout) ;
}

void set_Timeout(const uint16_t ms)
{
    TIM_Cmd(TIM2,DISABLE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = ms-1;
    TIM_TimeBaseStructure.TIM_Prescaler = 48000;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    m_timeout = 0;
    TIM_Cmd(TIM2,ENABLE);
}

uint32_t get_Timeout()
{
    return m_timeout;
}

void TIM2_IRQHandler(void)
{
    TIM_Cmd(TIM2,DISABLE);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    m_timeout = 1;
}






