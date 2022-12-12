// #include "stm32f4xx.h"
#include "stm32f407xx.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdarg.h"

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

// declaração de tipo de estrutura de inicialização do calendário
typedef struct
{
    uint8_t formato, // especifica o formato da hora
        dia,         // especifica o dia da semana
        data,        // especifica a data do mês
        mes,         // especifica o mês
        ano,         // especifica o ano
        horas,       // especifica a hora
        minutos,     // especifica os minutos
        segundos;    // especifica os segundos
} RTC_CalendarTypeDef;
RTC_CalendarTypeDef RTC_Calendar; // declaração da variável estruturada

typedef struct
{
    uint8_t formato,     // especifica o formato da hora
        dia,             // especifica o dia da semana
        dataUnidade,     // especifica a data do mês
        dataDezena,      // especifica a data do mês
        mesUnidade,      // especifica o mês
        mesDezena,       // especifica o mês
        anoUnidade,      // especifica o ano
        anoDezena,       // especifica o ano
        horasUnidade,    // especifica a hora
        horasDezena,     // especifica a hora
        minutosUnidade,  // especifica os minutos
        minutosDezena,   // especifica os minutos
        segundosUnidade, // especifica os segundos
        segundosDezena;  // especifica os segundos
} RTC_FullCalendarTypeDef;
RTC_FullCalendarTypeDef calendar;

void RTC_SetCalendar(RTC_CalendarTypeDef *RTC_CalendarStruct)
{
    // Habilitando o acesso ao domínio de backup
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;  // habilita o clock da interface de energia
    PWR->CR |= PWR_CR_DBP;              // acesso de escrita aos registradores do RTC
    RCC->APB1ENR &= ~RCC_APB1ENR_PWREN; // desabilita o clock da interface de energia
    // Desabilitando a proteção de escrita dos registradores do RTC
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    RTC->ISR |= RTC_ISR_INIT; // entra no modo de inicialização
    while (!(RTC->ISR & RTC_ISR_INITF)); // aguarda o modo de inicialização

    // Escolha do formato da hora
    if (RTC_CalendarStruct->formato)
        RTC->CR |= RTC_CR_FMT; // formato AM/PM
    else
        RTC->CR &= ~RTC_CR_FMT; // formato 24h
    // Atualização de data e hora
    uint8_t dezena_data = (RTC_CalendarStruct->data) / 10;
    uint8_t unidade_data = (RTC_CalendarStruct->data) - 10 * dezena_data;
    uint8_t dezena_mes = (RTC_CalendarStruct->mes) / 10;
    uint8_t unidade_mes = (RTC_CalendarStruct->mes) - 10 * dezena_mes;
    uint8_t dezena_ano = (RTC_CalendarStruct->ano) / 10;
    uint8_t unidade_ano = (RTC_CalendarStruct->ano) - 10 * dezena_ano;
    uint8_t dezena_hora = (RTC_CalendarStruct->horas) / 10;
    uint8_t unidade_hora = (RTC_CalendarStruct->horas) - 10 * dezena_hora;
    uint8_t dezena_minuto = (RTC_CalendarStruct->minutos) / 10;
    uint8_t unidade_minuto = (RTC_CalendarStruct->minutos) - 10 * dezena_minuto;
    uint8_t dezena_segundo = (RTC_CalendarStruct->segundos) / 10;
    uint8_t unidade_segundo = (RTC_CalendarStruct->segundos) - 10 * dezena_segundo;
    RTC->TR = dezena_hora << 20 | unidade_hora << 16 | // ajusta a hora como HH:MM:SS
              dezena_minuto << 12 | unidade_minuto << 8 |
              dezena_segundo << 4 | unidade_segundo;
    RTC->DR = dezena_ano << 20 | unidade_ano << 16 | // ajusta a data como YY/MM/DD
              (RTC_CalendarStruct->dia) << 13 |
              dezena_mes << 12 | unidade_mes << 8 |
              dezena_data << 4 | unidade_data;
    // Saída do modo de inicialização
    RTC->ISR &= ~RTC_ISR_INIT;
    while (!(RTC->ISR & RTC_ISR_RSF))
        ; // aguarda o calendário sincronizar
    // Reabilitando a proteção de escrita nos registradores do RTC
    RTC->WPR = 0xFF;
}

void getValueRTC(RTC_FullCalendarTypeDef *calendarNow)
{
	//RTC_DR DIAS
    uint8_t dia = RTC->DR & RTC_DR_WDU;
    uint8_t dezena_data = RTC->DR & RTC_DR_DT;
    uint8_t unidade_data = RTC->DR & RTC_DR_DU;
    uint8_t dezena_mes =  RTC->DR & RTC_DR_MT;
    uint8_t unidade_mes =  RTC->DR & RTC_DR_MU;
    uint8_t dezena_ano = RTC->DR & RTC_DR_YT;
    uint8_t unidade_ano = RTC->DR & RTC_DR_YU;
    //RTC_TR HORAS
    uint8_t dezena_hora = RTC->TR & RTC_TR_HT;
    uint8_t unidade_hora = RTC->TR & RTC_TR_HU;
    uint8_t dezena_minuto = RTC->TR & RTC_TR_MNT;
    uint8_t unidade_minuto = RTC->TR & RTC_TR_MNU;
    uint8_t dezena_segundo = RTC->TR & RTC_TR_ST;
    uint8_t unidade_segundo = RTC->TR & RTC_TR_SU;

    calendarNow->dia = dia;
    calendarNow->dataDezena = dezena_data;
    calendarNow->dataUnidade = unidade_data;
    calendarNow->mesDezena = dezena_mes;
    calendarNow->mesUnidade = unidade_mes;
    calendarNow->anoDezena = dezena_ano;
    calendarNow->anoUnidade = unidade_ano;
    calendarNow->horasDezena = dezena_hora;
    calendarNow->horasUnidade = unidade_hora;
    calendarNow->minutosDezena = dezena_minuto;
    calendarNow->minutosUnidade = unidade_minuto;
    calendarNow->segundosDezena = dezena_segundo;
    calendarNow->segundosUnidade = unidade_segundo;
}

char diaDaSemana[8][15] = {"proibido", "segunda-feira", "terça-feira", "quarta-feira", "quinta-feira", "sexta-feira", "sábado", "domingo"};
char mesD[11][10] = {"proibido", "janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro"};
char mesU[3][10] = {"outubro", "novembro", "dezembro"};
char mesNow[10];

void mes(int d, int u)
{
    if (!d)
    {
        strcpy(mesNow, mesD[u]);
    }
    else
    {
        strcpy(mesNow, mesU[u]);
    }
}

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
    RTC_Calendar.formato = 0;  // definição do formato da hora (24h)
    RTC_Calendar.dia = 1;      // definição do dia (terça-feira)
    RTC_Calendar.data = 12;     // definição da data (dia do mês)
    RTC_Calendar.mes = 12;     // definição do mês (dezembro)
    RTC_Calendar.ano = 22;     // definição do ano (2025)
    RTC_Calendar.horas = 10;   // definição da hora
    RTC_Calendar.minutos = 5;  // definição dos minutos
    RTC_Calendar.segundos = 20; // definição dos segundos
    RTC_SetCalendar(&RTC_Calendar); // atualização do calendário

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

            getValueRTC(&calendar); // Pegando os dados do RTC

            // Aqui deve ser colocado o envio da String com a data. O formato é:
            // Hoje é quarta-feira, 05 de junho de 2019.
            // A hora atual é 09:15:20.
            char concatenar[20];
            char textoEnviar[120] = "Hoje é ";

        //    sprintf(concatenar, "%d", calendar.dia);
        //     sendString(concatenar);

           strncat(textoEnviar, diaDaSemana[calendar.dia], 15);
           strcpy(concatenar, ", ");
           strncat(textoEnviar, concatenar, 2);
           sprintf(concatenar, "%d", calendar.dataDezena);
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.dataUnidade);
           strncat(textoEnviar, concatenar, 1);
           strcpy(concatenar, " de ");
           strncat(textoEnviar, concatenar, 4);
           mes(calendar.mesDezena, calendar.mesUnidade);
           sprintf(concatenar, "%d", mesNow);
           strncat(textoEnviar, mesNow, 9);
           strcpy(concatenar, " de 20");
           strncat(textoEnviar, concatenar, 6);
           sprintf(concatenar, "%d", calendar.anoDezena);
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.anoUnidade);
           strncat(textoEnviar, concatenar, 1);
           strcpy(concatenar, ".\nA hora atual é ");
           strncat(textoEnviar, concatenar, 18);
           sprintf(concatenar, "%d", calendar.horasDezena);
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.horasUnidade);
           strncat(textoEnviar, concatenar, 1);
           strcpy(concatenar, ":");
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.minutosDezena);
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.minutosUnidade);
           strncat(textoEnviar, concatenar, 1);
           strcpy(concatenar, ":");
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.segundosDezena);
           strncat(textoEnviar, concatenar, 1);
           sprintf(concatenar, "%d", calendar.segundosUnidade);
           strncat(textoEnviar, concatenar, 1);
           strcpy(concatenar, "\n");
           strncat(textoEnviar, concatenar, 1);

           sendString(textoEnviar);
        }
    }
}
