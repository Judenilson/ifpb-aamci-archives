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

/*
requisitos:
  if {
    precionando e mantendo k0 
    e em no máximo 1 segundo precionando e mantendo k1 
    o led d2 deve ligar e se manter ligado enquanto k0 e k1 permanecerem precionados
  } else {
    led d2 desliga e permanece desligado
  }
testes em que o led d2 não deve acender:
  precionar e soltar k0
  precionar e soltar k1
  precionar esoltar k0 e k1
  manter k0 precionado por 4 segundos
  manter k1 precionado por 4 segundos
  precionar k0 e k1 e manter por 4 segundos 
  manter k0 precionado e após 4 segundos também manter k1 precionado
  manter k1 precionado e após 4 segundos também manter k0 precionado
  manter k1 precionado e logo em seguida manter k0 precionado
testes específicos:
  precionar e manter k0 e logo em seguida precionar e soltar k1 e continuar precionando e soltando k1: 
    o led d2 deve piscar a primeira vez que k1 é precionado e solto mas logo em seguida deve ficar desligado
    isso força o requisito a ser atendido prevenindo interferencias em k1 (caso k1 fique flutuando ou seja solto por um breve tempo o led d2 não vai permanecer ligado) 
*/

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

  // botões
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; // ativa o clock para GPIOE (pino dos botões k0 e k1)
  GPIOE->MODER &= ~(0b11 << 6); // configura o pino pe4 em modo de entrada (pino do botão k1)
  GPIOE->MODER &= ~(0b11 << 8); // configura o pino pe3 em modo de entrada (pino do botão k0)
  GPIOE->PUPDR |= (0b01 << 8); // habilita o registor pull-up de pe4
  GPIOE->PUPDR |= (0b01 << 6); // habilita o registor pull-up de pe3
  // setup end
  
  // init begin
  // led
  GPIOA->ODR |= (1 << 6); // desliga o led d2
  // consts
  #define NONE 0 // definição para quando não houve evento
  #define K0_PRESSED 1 // definição para evento de quando o botão k0 é precionado
  #define K0_RELEASED 2 // definição para evento de quando o botão k0 é solto
  #define K1_PRESSED 3 // definição para evento de quando o botão k1 é precionado
  #define K1_RELEASED 4 // definição para evento de quando o botão k1 é solto
  #define TIMEOUT 5 // definição para evento de quando o tempo limite para apertar k1 foi ultrapassado
  // control vars
  int actual_event = NONE; // evento atual
  int previous_event = NONE; // evento anterior ao atual
  int read = GPIOE->IDR; // leitura dos botões
  // timer
  int timer = 4000000; // decrementando 1 a 1 a cada clock, dura aproximadamente 1 segundo  
  // init end

  while (1) {
    read = GPIOE->IDR;

    // eventos
    // k0 esta precionado
    if (!(read & (1 << 4))) { 
      registerEvent(&previous_event, &actual_event, K0_PRESSED);
      if (timer > -1) timer--; // faz o if pra não ficar decremetando infinitamente e gerar valores inesperados
    } else { 
      registerEvent(&previous_event, &actual_event, K0_RELEASED);
      timer = 4000000;
    }

    // k1 esta precionado
    if (!(read & (1 << 3))) { 
      registerEvent(&previous_event, &actual_event, K1_PRESSED);
    } else { 
      registerEvent(&previous_event, &actual_event, K1_RELEASED);
    }

    // já se passou aproximadamente 1 segundo desde que k0 foi mantido precionado
    if (timer <= 0 && !(previous_event == K0_PRESSED && actual_event == K1_PRESSED)) {
      registerEvent(&previous_event, &actual_event, TIMEOUT);
    }

    // resultados da sequência dos eventos
    // se a sequência de eventos preciona e mantém k0 seguidio de preciona e mantém o led d2 liga
    if (previous_event == K0_PRESSED && actual_event == K1_PRESSED) {
      GPIOA->ODR &= ~(1 << 6); // liga o led d2
    } else {
      GPIOA->ODR |= (1 << 6); // desliga o led d2
    }
  }
}