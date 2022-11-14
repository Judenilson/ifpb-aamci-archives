/* Descrição:
 Implementação de uma comunicação serial simples controlada por software:
 - O pino PA0 opera em dreno aberto como pino de comunicação entre duas placas
 - Os pinos PA0 de duas placas devem ser interligados juntamente com o GND
 - Os botões K0 e K1 acionam, respectivamente, os LEDs D2 e D3 da outra placa (operação remota)
 - Há um buzzer no pino PA1 para sinalizar o envio de um dado
*/

#include "stm32f4xx.h"
#include "Utility.h"

// Protótipos de funções
void envia_cmd(uint8_t);  // função para enviar um comando no barramento
uint8_t recebe_cmd(void); // função para receber um comando
void buzzer(void);        // função de ativação do buzzer

int main(void)
{
  Configure_Clock(); // configura o sistema de clock
  Delay_Start();     // inicializa funções de Delay

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN; // habilita o clock do GPIOA e GPIOE

  GPIOA->ODR |= (1 << 7) | (1 << 6) | 1;                              // inicia com leds e buzzer desligados e linha COM em idle
  GPIOA->OTYPER |= 1;                                                 // saída open-drain em PA0
  GPIOA->PUPDR |= 0b01;                                               // habilita pull-up em PA0
  GPIOA->MODER |= (0b01 << 14) | (0b01 << 12) | (0b01 << 2) | (0b01); // pinos PA0, PA1, PA6 e PA7 no modo saída
  GPIOE->PUPDR |= (0b01 << 8) | (0b01 << 6);                          // habilita pull-up em PE4 e PE3

  Delay_ms(100); // aguarda sinais estabilizarem
  while (1)
  {
    if (!(GPIOE->IDR & (1 << 4))) // verifica se PE4 foi pressionado
    {
        envia_cmd(0b01);
        buzzer();     // sinaliza o fim do envio
        Delay_ms(75); // filtro de bouncing
        while (!(GPIOE->IDR & (1 << 4)))
          ; // aguarda o botão ser solto
    }

    if (!(GPIOE->IDR & (1 << 3))) // verifica se PE3 foi pressionado
    {
        envia_cmd(0b10);
        buzzer();     // sinaliza o fim do envio
        Delay_ms(75); // filtro de bouncing
        while (!(GPIOE->IDR & (1 << 3)))
          ; // aguarda o botão ser solto
    }

    if (!(GPIOA->IDR & 1)) // verifica se há início de comunicação !(GPIOA->IDR & 1) == !(...aaaaaaaa & ...00000001) == !(...0000000a) == ...0000000ã, se GPIOA->IDR == ...aaaaaaa0
    {
      uint8_t recebido = recebe_cmd(); // recebe o comando
      if (recebido == 0b01)
      {
        GPIOA->ODR ^= 1 << 6; // alterna o estado do LED em PA6
      }
      if (recebido == 0b10)
      {
        GPIOA->ODR ^= 1 << 7; // alterna o estado do LED em PA7
      }
    }
  }
}

// Função para envio de um comando
void envia_cmd(uint8_t dado)
// algorítmo:
// sendo dado = ...a1a2a3a4a5a6a7a8
// mantendo a porta serial sempre em nível lógico alto
// enviando dado da direita para esquerda
// definimos o pacote com a ordem de envio (descida no nível lógico)(a8)(a7)(1)
{
  GPIOA->ODR &= ~1;                                 // start bit (descida no nível lógico) (GPIOA->ODR & ...11111110 -> GPIOA->ODR = ...aaaaaaa0)
  Delay_us(50);                                     // aguarda tempo do start bit
  for (uint8_t counter = 0; counter < 2; ++counter) // envia os bits do comando
  {
    // (dado & 1 -> ...aaaaaaaa & ...00000001 -> ...00000000a)
    if (dado & 1)       // se dado == ...aaaaaaa1
      GPIOA->ODR |= 1;  // envia o próximo bit (GPIOA->ODR | ...00000001 -> GPIOA->ODR = ...aaaaaaa1)
    else                // se dado == ...aaaaaaa0
      GPIOA->ODR &= ~1; // (GPIOA->ODR & ...11111110 -> GPIOA->ODR = ...aaaaaaa0)
    dado >>= 1;         // desloca os bits para envio posterior (...a1a2a3a4a5a6a7a8 -> ...0a1a2a3a4a5a6a7)
    Delay_us(50);       // aguarda o tempo do bit
  }
  GPIOA->ODR |= 1; // stop bit (subida no nível lógico) (GPIOA->ODR | ...00000001 -> GPIOA->ODR = ...aaaaaaa1)
  Delay_us(50);    // tempo do stop bit
}

// Função para recebimento de um comando
uint8_t recebe_cmd(void)
// algorítmo:
// sendo dado_recebido = ...aaaaaaa8a7
// recebendo o pacote de dados da direita para esquerda (descida no nível lógico)(a8)(a7)(1)
// dado_retornado = ...000000a7a8
{
  uint8_t dado_recebido = 0;
  uint8_t dado_retornado = 0;
  Delay_us(25);          // aguarda metade do start bit
  if (!(GPIOA->IDR & 1)) // confirma que houve um start bit (!(GPIOA->IDR & 1) == !(...aaaaaaaa & ...00000001) == !(...0000000a) == ...000000ã) se GPIOA->IDR == ...0000000
  {
    for (uint8_t counter = 0; counter < 2; ++counter) // leitura dos bits
    {
      Delay_us(50);          // aguarda o tempo do bit
      if (GPIOA->IDR & 1)    // (GPIOA->IDR & 1 == ...aaaaaaaa & ...00000001 == ...0000000a) se GPIOA->IDR == ...aaaaaa1
        dado_recebido |= 1;  // (dado_recebido | ...00000001 == ...aaaaaaa1) dado_recebido = ...aaaaaaa1
      else                   // se GPIOA->IDR == ...aaaaaaa0
        dado_recebido &= ~1; // (dado_recebido & ...11111110 == ...aaaaaaa0) dado_recebido = ...aaaaaaa0
      dado_recebido <<= 1;   // dado_recebido = ...aaaaaaa80
    }

    Delay_us(50);         // aguarda para fazer leitura do stop bit
    if ((GPIOA->IDR & 1)) // confirma que houve um stop bit (GPIOA->IDR & 1 == ...aaaaaaaa & ...00000001 == ...0000000a) se GPIOA->IDR == ...0000001
    {
      // dado_recebido = ...aaaaaa8a70
      dado_recebido >>= 1; // dado_recebido >> 1 == ...0aaaaaa8a7
      Delay_us(25);        // aguarda o fim do tempo do stop bit
      // dado_recebido == ...0aaaaaa8a7
      // ((dado_recebido & 0b100) >> 2) | (dado_recebido & 0b010) | ((dado_recebido & 0b001) << 2)
      // ((...aaaaaa8a7a6 & 0b00000100) >> 2) | (...aaaaaa8a7a6 & 0b00000010) | ((...aaaaaa8a7a6 & 0b00000001) << 2)
      // (...00000a800 >> 2) | ...000000a70 | (...0000000a6 << 2)
      // ...0000000a8 | ...000000a70 | ...00000a600
      // ...000000a7a8
      // dado_retornado = ...000000a7a8
      return dado_retornado = ((dado_recebido & 0b10) >> 1) | ((dado_recebido & 0b01) << 1); // retorna o dado recebido
    }
    else        // se GPIOA->IDR == ...0000000
      return 0; // não houve recepção do stop bit, aborta recepção
  }
  else
    return 0; // não houve recepção do start bit, aborta recepção
}

// Função de sinalização de fim de envio de dado
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