 #include "stm32f4xx.h"
//#include "stm32f407xx.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdarg.h"
#include "tm_stm32f4_rtc.h" //https://stm32f4-discovery.net/2014/07/library-19-use-internal-rtc-on-stm32f4xx-devices/

void Configure_Clock(void)
{
	//Parâmetros do PLL principal
	#define PLL_M	4
	#define PLL_N	168
	#define PLL_P	2
	#define PLL_Q	7

	//Reseta os registradores do módulo RCC para o estado inicial
	RCC->CIR = 0;				//desabilita todas as interrupções de RCC
	RCC->CR |= RCC_CR_HSION;	//liga o oscilador HSI
	RCC->CFGR = 0;				//reseta o registrador CFGR
	//Desliga HSE, CSS e o PLL e o bypass de HSE
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON |
			   RCC_CR_PLLON | RCC_CR_HSEBYP);
	RCC->PLLCFGR = 0x24003010;	//reseta o registrador PLLCFGR

	//Configura a fonte de clock (HSE), os parâmetros do PLL,
	//prescalers dos barramentos AHB, APB
	RCC->CR |= RCC_CR_HSEON;				//habilita HSE
	while(!((RCC->CR) & RCC_CR_HSERDY));	//espera HSE ficar pronto
    RCC->CFGR |= 0x9400;	//HCLK = SYSCLK/1, PCLK2 = HCLK/2, PCLK1 = HCLK/4

    //Configura a fonte de clock e os parâmetros do PLL principal
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                   (0x400000)           | (PLL_Q << 24);

    RCC->CR |= RCC_CR_PLLON;			//habilita o PLL
    while(!(RCC->CR & RCC_CR_PLLRDY));	//espera o PLL ficar pronto verificando a flag PLLRDY

    //Seleciona o PLL como fonte de SYSCLK e espera o PLL ser a fonte de SYSCLK
    RCC->CFGR |= 0x2;
    while((RCC->CFGR & 0xC) != 0x8);
}

void Delay_Start(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // liga o clock do Timer2
    TIM2->CR1 &= ~TIM_CR1_DIR;          // contador em modo de contagem crescente
    TIM2->PSC = 83;                     // prescaler para pulsos a cada 1uS
    TIM2->EGR = TIM_EGR_UG;             // update event para escrever o valor do prescaler
    TIM2->CR1 |= TIM_CR1_CEN;           // habilita o timer
    // DBGMCU->APB1FZ |= 1;				//permite vizualizar o valor do contador no modo debug
}

void USART1_Init(void)
{
	//configuração da USART1
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;				//habilita o clock da USART1
	USART1->BRR = 364.58;									//ajusta baud rate para 1 Mbps (frequência de clock de 84MHz)
	//O estado default do registrador USART1->CR1 garante:
	//1 stop bit, 8 bits de dados, sem bit de paridade,
	//oversampling de 16 amostras em cada bit
	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE |		//habilita o trasmissor e o receptor
					USART_CR1_RXNEIE | USART_CR1_UE);	//habilita interrupção de RX e a USART1

	//Habilita a interrupção da USART1 no NVIC
	NVIC_SetPriority(USART1_IRQn, 0);		//seta a prioridade da USART1
	NVIC_EnableIRQ(USART1_IRQn);			//habilita a interrupção da USART1

	//Configuração dos pinos PA9 (TX) e PA10(RX)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;			//habilita o clock do GPIOA
	GPIOA->MODER |= (0b10 << 20) | (0b10 << 18) ;	//pinos PA10 e PA9 como função alternativa
	GPIOA->AFR[1] |= (0b0111 << 8) | (0b0111 << 4);	//função alternativa 7 (USART1)
}

void Delay_ms(uint32_t delay)
{
    uint32_t max = 1000 * delay;
    TIM2->CNT = 0; // inicializa o contador com 0
    while (TIM2->CNT < max)
        ; // aguarda o tempo passar
}

void sendChar(int ch)
{
    // USART1->DR = 0b00000001;	//escreve o dado a ser transmitido
    USART1->DR = (ch & (uint16_t)0x01FF); // escreve o dado a ser transmitido
    while (!(USART1->SR & USART_SR_TXE))
        ; // espera pelo fim da transmissão
}

void sendString(char *string)
{
    while (*string)
        sendChar(*string++);
}

void RTC_Config(void)
{
    // Habilitando o acesso ao domínio de backup
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;  // habilita o clock da interface de energia
    PWR->CR |= PWR_CR_DBP;              // acesso de escrita aos registradores do RTC
    RCC->APB1ENR &= ~RCC_APB1ENR_PWREN; // desabilita o clock da interface de energia
    RCC->BDCR |= RCC_BDCR_BDRST;        // reseta o domínio de backup
    RCC->BDCR &= ~RCC_BDCR_BDRST;       // reativa o domínio de backup
    // Configurando a fonte de clock para o RTC no registrador BDCR do módulo RCC
    RCC->BDCR &= ~(RCC_BDCR_LSEON | RCC_BDCR_LSEBYP); // desliga o oscilador LSE
    RCC->BDCR |= RCC_BDCR_LSEON;                      // liga o oscilador LSE
    while (!(RCC->BDCR & RCC_BDCR_LSERDY))
        ;                           // espera o clock LSE ficar pronto
    RCC->BDCR &= ~RCC_BDCR_RTCSEL;  // limpa os bits RTCSEL
    RCC->BDCR |= RCC_BDCR_RTCSEL_0; // seleciona LSE como fonte
    RCC->BDCR |= RCC_BDCR_RTCEN;    // habilita o clock do RTC
    // Desabilitando a proteção de escrita dos registradores do RTC
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    // Geração do sinal de clock de 1Hz a partir de LSE (32,768 kHz)
    RTC->ISR |= RTC_ISR_INIT; // entra no modo de inicialização
    while (!(RTC->ISR & RTC_ISR_INITF))
        ;                      // aguarda o modo de inicialização
    RTC->PRER |= 255;          // prescaler síncrono = 255
    RTC->PRER |= (127 << 16);  // prescaler assíncrono = 127
    RTC->ISR &= ~RTC_ISR_INIT; // sai do modo de inicialização
                               // Reabilitando a proteção de escrita nos registradores do RTC
    RTC->WPR = 0xFF;
}

char diaDaSemana[8][15] = {"proibido", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado", "domingo"};
char mesName[13][10] = {"proibido", "janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"};

char buf[50];
TM_RTC_Time_t datatime;

int main()
{
	Configure_Clock();
    Delay_Start();
    USART1_Init();
    RTC_Config();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN; // habilita o clock do GPIOA e GPIOE
    GPIOA->ODR |= (1 << 6);                                    // inicia com led D2 desligado
    GPIOA->MODER |= (0b01 << 14) | (0b01 << 12);               // pinos PA6 e PA7 no modo saída
    GPIOE->PUPDR |= (0b01 << 8) | (0b01 << 6);                 // habilita pull-up em PE4 e PE3

    // inicializando a estrutura do calendário
    datatime.hours = 12;
	datatime.minutes = 19;
	datatime.seconds = 30;
	datatime.year = 22;
	datatime.month = 12;
	datatime.date = 12;
	datatime.day = 1;

	/* Setando horário */
	TM_RTC_SetDateTime(&datatime, TM_RTC_Format_BIN);

    int second_status = 0;

    while (1)
    {

        if (RTC->TR & RTC_TR_SU_0)
        {
            GPIOA->ODR ^= 1 << 6; // faz o LED D2 acender quando o bit do segundo é 1
        }

        if (!(RTC->TR & RTC_TR_SU_0))
        {
            GPIOA->ODR |= 1 << 6; // faz o LED D2 apagar quando o bit do segundo é 0
        }

        if ((RTC->TR & RTC_TR_SU_0) != second_status)
        {
            second_status = RTC->TR & RTC_TR_SU_0;
            GPIOA->ODR ^= 1 << 7; // inverte o estado do LED D3

            // Aqui deve ser colocado o envio da String com a data. O formato é:
            // Hoje é quarta-feira, 05 de junho de 2019.
            // A hora atual é 09:15:20.

            TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);

			/* Format time */
			sprintf(buf, "Hoje é %s, %02d de %s de %04d.\nA hora atual é %02d:%02d:%02d.\n",
					 diaDaSemana[datatime.day],
						datatime.date,
						mesName[datatime.month],
						datatime.year + 2000,
						datatime.hours,
						datatime.minutes,
						datatime.seconds
			);
            sendString(buf);
        }
    }
}
