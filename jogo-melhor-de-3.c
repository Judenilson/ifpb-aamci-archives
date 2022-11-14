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

int vc = 0;
int cpu = 0;
unsigned int a;
unsigned int b;

int main(void) {
	Configure_Clock(); // configura o sistema de clock
	Delay_Start();     // inicializa funções de Delay
	USART1_Init();

	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN; //habilita o clock do RNG
	RNG->CR |= RNG_CR_RNGEN; //liga o RNG

	printf("----------------------- JOGO MELHOR DE 3 -----------------------\n");
	printf("!!!!!!!!!!!!!! Bem vindo a uma nova partida !!!!!!!!!!!!!!\n");
	printf("Pressione k para realizar um lançamento:\n");
}

unsigned int random() {
	while (1) {
		unsigned int numero = 0;
		if(RNG->SR & RNG_SR_DRDY) //verifica se há dados prontos
		{
			numero = RNG->DR; //lê o dado gerado
			numero &= 0b00000000000000000000000000000111;

		}
		if (numero > 0 && numero < 7)
		{
			return numero;
		}
	}
}

void USART1_IRQHandler(void) // função de interrupção quando recebe um dado pelo RX da USART, lembrar de comentar a função na utiliy.h
{
	char caracter = USART1->DR;
	if (caracter == 'k') {
		jogo();
	}
}

void printRodada (){
	if (vc > cpu) {
		printf("jogador: %u\n", a);
		printf("computador: %u\n", b);
		printf("%d x %d você está na frente.\n", vc, cpu);
	} else if (vc < cpu) {
		printf("jogador: %u\n", a);
		printf("computador: %u\n", b);
		printf("%d x %d o computador está na frente.\n", cpu, vc);
	} else {
		printf("jogador: %u\n", a);
		printf("computador: %u\n", b);
		printf("%d x %d Jogo Empatado!!!\n", vc, cpu);
	}
}

void jogo() {
	a = random();
	b = random();

	if (a > b) {
		vc ++;
		Delay_ms(1);
		printRodada();
	} else if (a < b) {
		cpu ++;
		Delay_ms(1);
		printRodada();
	} else {
		printf("Lançamento empatado!\n");
		printRodada();
	}

	if (vc == 2){
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("O JOGADOR GANHOU a partida!\n");
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		vc = 0;
		cpu = 0;
		printf("\n!!!!!!!!!!!!!! Bem vindo a uma nova partida !!!!!!!!!!!!!!\n");
	}
	if (cpu == 2){
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("O COMPUTADOR GANHOU a partida!\n");
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		vc = 0;
		cpu = 0;
		printf("\n!!!!!!!!!!!!!! Bem vindo a uma nova partida !!!!!!!!!!!!!!\n");
	}
	printf("\nPressione k para realizar um lançamento:\n");

}
