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

void atraso (int num) {
	while (num > 0) num--;
}

void registerEvent(int* previous_event, int* actual_event, int event_definition) {
  if (*actual_event != event_definition) {
    *previous_event = *actual_event;
    *actual_event = event_definition;
  }
}

int main(void) {

  // setup begin
  // led
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // ativa o clock para GPIOA (pino do led d2)
  GPIOA->MODER |= (0b01 << 12); // configura o pino do led d2 em modo de saída
  GPIOA->MODER |= (0b01 << 14); // configura o pino do led d2 em modo de saída

  // buzzer
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // ativa o clock para GPIOA (pino do led d2)
  GPIOD->MODER |= (0b01 << 14); // configura o pino do led d2 em modo de saída

  // botões
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; // ativa o clock para GPIOE (pino dos botões k0 e k1)
  GPIOE->MODER &= ~(0b11 << 6); // configura o pino pe4 em modo de entrada (pino do botão k1)
  GPIOE->MODER &= ~(0b11 << 8); // configura o pino pe3 em modo de entrada (pino do botão k0)
  GPIOE->PUPDR |= (0b01 << 8); // habilita o registor pull-up de pe4
  GPIOE->PUPDR |= (0b01 << 6); // habilita o registor pull-up de pe3
  // setup end

  // consts
  #define NONE 0 // definição para quando não houve evento
  #define K0_PRESSED 1 // definição para evento de quando o botão k0 é precionado
  #define K0_RELEASED 2 // definição para evento de quando o botão k0 é solto
  #define K1_PRESSED 3 // definição para evento de quando o botão k1 é precionado
  #define K1_RELEASED 4 // definição para evento de quando o botão k1 é solto

  int actual_event = K0_RELEASED; // evento atual
  int previous_event = K0_RELEASED; // evento anterior ao atual

  int previous_event1 = K1_RELEASED; // evento anterior ao atual

  // init begin
  // led
  GPIOA->ODR |= (1 << 6); // desliga o led d2
  GPIOA->ODR |= (1 << 7); // desliga o led d3
  int read = GPIOE->IDR;

  while (1) {
    read = GPIOE->IDR;

    // eventos
//    // k0 esta precionado
    if (!(read & (1 << 4)) && previous_event == K0_RELEASED) {
    	GPIOA->ODR ^= (1 << 6); // inverte d2
    	GPIOD->ODR |= (1 << 7); // inverte d2
    	previous_event = K0_PRESSED;
    }
    if ((read & (1 << 4))) {
    	previous_event = K0_RELEASED;
    }

    if (!(read & (1 << 3)) && previous_event1 == K1_RELEASED) {
    	GPIOA->ODR ^= (1 << 7); // inverte d2
    	GPIOD->ODR |= (1 << 7); // inverte d2
    	previous_event1 = K1_PRESSED;
    }
    if ((read & (1 << 4))) {
    	previous_event1 = K1_RELEASED;
    }


    // k1 esta precionado

//    if (!(read & (1 << 3))) {
//      registerEvent(&previous_event, &actual_event, K1_PRESSED);
//    } else {
//      registerEvent(&previous_event, &actual_event, K1_RELEASED);
//    }
//
//    if (previous_event == K1_RELEASED && actual_event == K1_PRESSED) {
//        GPIOA->ODR ^= (1 << 7); // inverte d2
//    	GPIOD->ODR |= (1 << 7); // inverte d2
//    }

    int count = 0;
    while(count<100000){
    	count++;
    	if (count >= 100000){
    		GPIOD->ODR &= ~(1 << 7); // inverte d2
    	}
    }
  }
}
