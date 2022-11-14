/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f4xx.h"
#include "Utility.h"


int main(void) {

	Configure_Clock(); // configura o sistema de clock
	Delay_Start();     // inicializa funções de Delay
	USART1_Init();

	RCC->AHB1ENR |= 1;
	GPIOA->ODR |= 1<<6;
	GPIOA->MODER |= 0b01 << 12;

  while (1) {
	  printf("AACMI\n"); // ta direcionando a string pra USAR1 pela função __io_putchar()
	  Delay_ms(500);
  }
}

void USART1_IRQHandler(void) // função de interrupção quando recebe um dado pelo RX da USART, lembrar de comentar a função na utiliy.h
{
	char caracter = USART1->DR;
	if (caracter == 'A') {
		GPIOA->ODR ^= (1<<6);
	}
}
