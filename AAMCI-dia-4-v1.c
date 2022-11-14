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


void beep() {
  GPIOD->ODR &= ~(1 << 7); // liga buzzer
  int counter = 2000000; // aproximadamente 500 ms
  while (counter > 0) counter--;
  GPIOD->ODR |= (1 << 7); // desliga buzzer
} 

int main(void) {

  // setup
  // led
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // ativa o clock para GPIOA (pino do led d2)
  GPIOA->MODER |= (0b01 << 12); // configura o pino do led d2 em modo de saída
  GPIOA->MODER |= (0b01 << 14); // configura o pino do led d3 em modo de saída

  // buzzer
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // ativa o clock para GPIOD (pinos do buzzer)
  GPIOD->MODER |= (0b01 << 14); // configura o pino do buzzer em modo de saída

  // botões
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; // ativa o clock para GPIOE (pino dos botões k0 e k1)
  GPIOE->MODER &= ~(0b11 << 6); // configura o pino pe4 em modo de entrada (pino do botão k1)
  GPIOE->MODER &= ~(0b11 << 8); // configura o pino pe3 em modo de entrada (pino do botão k0)
  GPIOE->PUPDR |= (0b01 << 8); // habilita o registor pull-up de pe4
  GPIOE->PUPDR |= (0b01 << 6); // habilita o registor pull-up de pe3

  // control vars
  int prev_k0_state = 0; // variável pra guardar o estado anterior do botão k0 (0 == solto, 1 == precionado)
  int prev_k1_state = 0;  // variável pra guardar o estado anterior do botão k1 (0 == solto, 1 == precionado)
  int read = GPIOE->IDR; // variável para leitura dos botões

  // init begin
  GPIOA->ODR |= (1 << 6); // desliga o led d2
  GPIOA->ODR |= (1 << 7); // desliga o led d3
  GPIOD->ODR |= (1 << 7); // desliga buzzer

  // main loop
  while (1) {
    read = GPIOE->IDR; // leitura dos botões

    // k0 precionado
    if (!(read & (1 << 4)) && !prev_k0_state) {
      GPIOA->ODR ^= (1 << 6); // inverte d2
      beep();
      prev_k0_state = 1;
    } 
    // k0 solto
    else if ((read & (1 << 4))) {
      prev_k0_state = 0;
    }

    // k1 precionado
    if (!(read & (1 << 3)) && !prev_k1_state) {
      GPIOA->ODR ^= (1 << 7); // inverte d3
      beep();
      prev_k1_state = 1;
    } 
    // k1 solto
    else if ((read & (1 << 3))) {
      prev_k1_state = 0;
    }
  }
}
