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

#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdarg.h"
void Configure_Clock(void)
{
// Par�metros do PLL principal
#define PLL_M 4
#define PLL_N 168
#define PLL_P 2
#define PLL_Q 7

    // Reseta os registradores do m�dulo RCC para o estado inicial
    RCC->CIR = 0;            // desabilita todas as interrup��es de RCC
    RCC->CR |= RCC_CR_HSION; // liga o oscilador HSI
    RCC->CFGR = 0;           // reseta o registrador CFGR
    // Desliga HSE, CSS e o PLL e o bypass de HSE
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON |
                 RCC_CR_PLLON | RCC_CR_HSEBYP);
    RCC->PLLCFGR = 0x24003010; // reseta o registrador PLLCFGR

    // Configura a fonte de clock (HSE), os par�metros do PLL,
    // prescalers dos barramentos AHB, APB
    RCC->CR |= RCC_CR_HSEON; // habilita HSE
    while (!((RCC->CR) & RCC_CR_HSERDY))
        ;                // espera HSE ficar pronto
    RCC->CFGR |= 0x9400; // HCLK = SYSCLK/1, PCLK2 = HCLK/2, PCLK1 = HCLK/4

    // Configura a fonte de clock e os par�metros do PLL principal
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) - 1) << 16) |
                   (0x400000) | (PLL_Q << 24);

    RCC->CR |= RCC_CR_PLLON; // habilita o PLL
    while (!(RCC->CR & RCC_CR_PLLRDY))
        ; // espera o PLL ficar pronto verificando a flag PLLRDY

    // Seleciona o PLL como fonte de SYSCLK e espera o PLL ser a fonte de SYSCLK
    RCC->CFGR |= 0x2;
    while ((RCC->CFGR & 0xC) != 0x8)
        ;
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
    // configura��o da USART1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // habilita o clock da USART1
    USART1->BRR = 91.14;                  // ajusta baud rate para 1 Mbps (frequ�ncia de clock de 84MHz) 91.14 = 921600 BAUD; 364.58 = 230400 BAUD;
    // O estado default do registrador USART1->CR1 garante:
    // 1 stop bit, 8 bits de dados, sem bit de paridade,
    // oversampling de 16 amostras em cada bit
    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE |     // habilita o trasmissor e o receptor
                    USART_CR1_RXNEIE | USART_CR1_UE); // habilita interrup��o de RX e a USART1

    // Habilita a interrup��o da USART1 no NVIC
    NVIC_SetPriority(USART1_IRQn, 0); // seta a prioridade da USART1
    NVIC_EnableIRQ(USART1_IRQn);      // habilita a interrup��o da USART1

    // Configura��o dos pinos PA9 (TX) e PA10(RX)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;            // habilita o clock do GPIOA
    GPIOA->MODER |= (0b10 << 20) | (0b10 << 18);    // pinos PA10 e PA9 como fun��o alternativa
    GPIOA->AFR[1] |= (0b0111 << 8) | (0b0111 << 4); // fun��o alternativa 7 (USART1)
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
        ; // espera pelo fim da transmiss�o
}

void sendString(char *string)
{
    while (*string)
        sendChar(*string++);
}

void SPI1_W25Q16_Init()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // habilita o clock do GPIOB
    GPIOB->ODR |= 1;                     // pino PB0 inicialmente em nível alto
    // modo de saída em PB0 e função alternativa em PB5, PB4 e PB3 com alta velocidade
    GPIOB->MODER |= (0b10 << 10) | (0b10 << 8) | (0b10 << 6) | (0b01);
    GPIOB->OSPEEDR |= (0b11 << 10) | (0b11 << 8) | (0b11 << 6) | (0b11);
    GPIOB->AFR[0] |= (0b0101 << 20) | (0b0101 << 16) | (0b0101 << 12); // AF7 (SPI1)
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;                                // habilita o clock da interface SPI1
    // pino SS controlado por software, configura o modo master e habilita a interface
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR | SPI_CR1_SPE;
}

// Leitura do registrador de status 1
uint8_t Read_Status_Register1(void)
{
    GPIOB->ODR &= ~1; // pino CS em nível baixo (inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x05; // comando de leitura do status register 1 (05h)
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício do escravo
    (void)SPI1->DR;  // lê o byte fictício
    SPI1->DR = 0xFF; // envia byte fictício para receber o valor do reg
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o valor ser recebido
    GPIOB->ODR |= 1; // pino CS em nível alto (encerra o comando)
    return SPI1->DR; // retorna o valor do registrador de status 1
}

// habilitação de escrita na memória
void Write_Enable(void)
{
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x06; // comando para habilitação de escrita (06h)
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício do escravo
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
    while (!(Read_Status_Register1() & 0b10))
        ; // aguarda a escrita estar habilitada
}

// apagamento de setor
void Sector_Erase(uint32_t address)
{
    Write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x20; // envia comando para apagar o setor
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
}

// apagamento de bloco de 32 kB
void _32kB_Block_Erase(uint32_t address)
{
    Write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x52; // envia comando para apagar o bloco
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
}

// apagamento de bloco de 64 kB
void _64kB_Block_Erase(uint32_t address)
{
    Write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0xD8; // envia comando para apagar o bloco
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
}

// apagamento completo do chip
void Chip_Erase(void)
{
    Write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0xC7; // envia comando para apagar o chip
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício do escravo
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
}

// leitura de um byte na memória
uint8_t Read_Data(uint32_t address)
{
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x03; // envia comando para ler um dado
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício
    uint8_t RX = SPI1->DR;                 // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    RX = SPI1->DR;                      // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    RX = SPI1->DR;               // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    RX = SPI1->DR;   // lê o byte fictício
    SPI1->DR = 0xFF; // envia um byte fictício
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte de dado ser recebido
    RX = SPI1->DR;   // lê o byte gravado na memória
    GPIOB->ODR |= 1; //(encerra o comando)
    return RX;       // retorna com o byte armazenado
}

// leitura de múltiplos bytes na memória
void Read_MData(uint32_t address, uint8_t *buffer, uint32_t size)
{
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x03; // comando para ler um dado (03h)
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;           // aguarda o byte fictício
    (void)SPI1->DR; // lê o byte fictício

    uint32_t contador = 0;
    while (size)
    {
        SPI1->DR = 0xFF; // envia um byte fictício
        while (!(SPI1->SR & SPI_SR_RXNE))
            ;                        // aguarda o byte de dado ser recebido
        buffer[contador] = SPI1->DR; // lê o byte gravado na memória
        ++contador;
        --size;
    }

    GPIOB->ODR |= 1; //(encerra o comando)
}

// gravação de um byte na memória
void Page_Program(uint32_t address, uint8_t data)
{
    Write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; //(inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x02; // envia comando para escrever um dado
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    (void)SPI1->DR;  // lê o byte fictício
    SPI1->DR = data; // envia o dado a ser gravado
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;            // aguarda o byte fictício
    (void)SPI1->DR;  // lê o byte fictício
    GPIOB->ODR |= 1; //(encerra o comando)
}

// gravação de múltiplos bytes na memória
void MPage_Program(uint32_t address, uint8_t *buffer, uint8_t size)
{
    write_Enable(); // habilita a escrita na memória
    while (Read_Status_Register1() & 1)
        ;             // aguarda a memória estar disponível
    GPIOB->ODR &= ~1; // pino CS em nível baixo (inicia o comando)
    while (!(SPI1->SR & SPI_SR_TXE))
        ;            // aguarda o buffer Tx estar vazio
    SPI1->DR = 0x02; // envia comando para escrever um dado (02h)
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                                  // aguarda o byte fictício do escravo
    (void)SPI1->DR;                        // lê o byte fictício
    SPI1->DR = (address & 0xFF0000) >> 16; // envia o byte 3 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                               // aguarda o byte fictício
    (void)SPI1->DR;                     // lê o byte fictício
    SPI1->DR = (address & 0xFF00) >> 8; // envia o byte 2 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;                        // aguarda o byte fictício
    (void)SPI1->DR;              // lê o byte fictício
    SPI1->DR = (address & 0xFF); // envia o byte 1 do endereço
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;           // aguarda o byte fictício
    (void)SPI1->DR; // lê o byte fictício

    uint8_t contador = 0;
    while (size)
    {
        SPI1->DR = buffer[contador]; // envia o byte de dado a ser gravado
        while (!(SPI1->SR & SPI_SR_RXNE))
            ;           // recebe o byte fictício
        (void)SPI1->DR; // lê o byte fictício
        ++contador;
        --size;
    }
    GPIOB->ODR |= 1; //(encerra o comando)
}

// // leitura do ID único de 64 bits
// uint64_t Read_Unique_ID_Number(void)
// {
//     Write_Enable(); // habilita a escrita na memória
//     while (Read_Status_Register1() & 1)
//         ;             // aguarda a memória estar disponível
//     GPIOB->ODR &= ~1; //(inicia o comando)
//     while (!(SPI1->SR & SPI_SR_TXE))
//         ;            // aguarda o buffer Tx estar vazio
//     SPI1->DR = 0x02; // envia comando para escrever um dado
//     while (!(SPI1->SR & SPI_SR_RXNE))
//         ;            // aguarda o byte fictício
//     (void)SPI1->DR;  // lê o dummy byte
//     SPI1->DR = 0xFF; // envia o byte fictício 1
//     while (!(SPI1->SR & SPI_SR_RXNE))
//         ;            // aguarda o byte fictício
//     (void)SPI1->DR;  // lê o byte fictício
//     SPI1->DR = 0xFF; // envia o byte fictício 2
//     while (!(SPI1->SR & SPI_SR_RXNE))
//         ;            // aguarda o byte fictício
//     (void)SPI1->DR;  // lê o byte fictício
//     SPI1->DR = 0xFF; // envia o byte fictício 3
//     while (!(SPI1->SR & SPI_SR_RXNE))
//         ;            // aguarda o byte fictício
//     (void)SPI1->DR;  // lê o byte fictício
//     SPI1->DR = 0xFF; // envia o byte fictício 4
//     while (!(SPI1->SR & SPI_SR_RXNE))
//         ;            // aguarda o byte fictício
//     (void)SPI1->DR;  // lê o byte fictício
//     uint64_t ID = 0; // variável que vai receber o ID
//     uint8_t cont = 1;
//     while (RX <= 8)
//     {
//         SPI1->DR = 0xFF; // envia um byte fictício
//         while (!(SPI1->SR & SPI_SR_RXNE))
//             ;                                           // aguarda um byte do ID ser recebido
//         ID |= ((uint64_t)SPI1->DR << (8 * (8 - cont))); // lê o byte do ID
//         ++cont;
//     }
//     GPIOB->ODR |= 1; //(encerra o comando)
//     return ID;       // retorna com o ID
// }

int main()
{
    Configure_Clock();
    Delay_Start();
    USART1_Init();
    SPI1_W25Q16_Init();

    Delay_ms(3000);
    char buf[255] = "Iniciando Leitura da Memoria:\n";
    sendString(buf);
    int i = 0;
    int memorySize = 896000;
    while (i < memorySize)
    {
        uint8_t data = Read_Data(i);
        if (data != 255)
        {
            sendChar(data);
        }
        i++;
    
    }
    sprintf(buf, "\nFim da Leitura da Memoria!\n");
    sendString(buf);

    while(1){
        ;
    }
}
