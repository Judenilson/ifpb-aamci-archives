#include "stm32f4xx.h"
#include "Utility.h"


int main(void) {

	Configure_Clock(); // configura o sistema de clock
	Delay_Start();     // inicializa funções de Delay
	USART1_Init();


  while (1) {
	  printf("AACMI\n");
	  Delay_ms(500);
  }
}