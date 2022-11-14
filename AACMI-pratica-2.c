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


void buzzer(void)
{
  GPIOA->ODR |= 1 << 1;    // liga o buzzer
  Delay_ms(10);            // aguarda
  GPIOA->ODR &= ~(1 << 1); // desliga o buzzer
  Delay_ms(80);            // aguarda
  GPIOA->ODR |= 1 << 1;    // liga o buzzer
  Delay_ms(10);            // aguarda
  GPIOA->ODR &= ~(1 << 1); // desliga o buzzer
}


int ack = 0; // flag que confirma que o envio funcionou
int main(void) {

	Configure_Clock(); // configura o sistema de clock
	Delay_Start();     // inicializa funções de Delay
	USART1_Init();

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN; // habilita o clock do GPIOA e GPIOE

	GPIOA->ODR |= (1 << 7) | (1 << 6) | 1;       // inicia com leds e buzzer desligados
	GPIOA->MODER |= (0b01 << 14) | (0b01 << 12) | (0b01 << 2); // pinos PA0, PA1, PA6 e PA7 no modo saída
	GPIOE->PUPDR |= (0b01 << 8) | (0b01 << 6); // pull up para os botões

	Delay_ms(100);

  while (1) {
	   // ta direcionando a string pra USAR1 pela função __io_putchar()
	  if (!(GPIOE->IDR & (1 << 3))) { // apertou k0
		  ack = 0;
		  buzzer();
		  printf("2\n"); // envia comando pra ligar led d2
		  Delay_ms(100); // evita flutuação
		  if (!ack) {
			  GPIOA->ODR ^= (1<<7);
		  }
		  while (!(GPIOE->IDR & (1 << 3))); // segura execução até soltar botão
	  }
	  if (!(GPIOE->IDR & (1 << 4))) { // apertou k1
		  ack = 0;
		  buzzer();
		  printf("3\n"); // envia comando pra ligar led d3
		  Delay_ms(100); // evita flutuação
		  if (!ack) {
			  GPIOA->ODR ^= (1<<6);
		  }
		  while (!(GPIOE->IDR & (1 << 4))); // segura execução até soltar botão
	  }
  }
}

void USART1_IRQHandler(void) // função de interrupção quando recebe um dado pelo RX da USART, lembrar de comentar a função na utiliy.h
{
	char caracter = USART1->DR;
	if (caracter == '2') {
		GPIOA->ODR ^= (1<<7);
		printf("1\n"); // manda ack
	}
	if (caracter == '3') {
		GPIOA->ODR ^= (1<<6);
		printf("1\n");
	}
	if (caracter == '1') { // le ack
		printf("1\n");
		ack = 1;
	}
}
