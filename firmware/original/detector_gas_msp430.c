/*
 * ============================================================================
 *  detector_gas_msp430.c  --  SKETCH ORIGINAL DO PROJETO
 * ============================================================================
 *
 *  Este arquivo e o codigo como ele foi escrito e rodou na bancada durante o
 *  Laboratorio de Sistemas Microprocessados. Ele esta preservado aqui como
 *  referencia historica: e contra ele que o firmware organizado em `src/` foi
 *  comparado, e e dele que saiu a lista de bugs corrigidos em `../README.md`.
 *
 *  NAO edite este arquivo. Ele e um documento, nao o firmware do projeto.
 *  O firmware mantido esta em `../src/` e `../include/`.
 *
 *  Observacoes conhecidas (detalhadas em ../README.md):
 *    - config_Buzzer() usa P2.4 / canal TA2.1 do Timer A2, mas o comentario
 *      original dizia "P2.5 controlado pelo Timer A2 (TA2.2)".
 *    - lcdWrite() testa `col == 16` ANTES de escrever, o que faz o caractere
 *      17 invadir a segunda linha; e nao preenche as 16 colunas, deixando
 *      restos da mensagem anterior na tela.
 *    - O LED1 (P1.0) e citado no relatorio mas nunca e configurado aqui.
 *    - A ISR do botao faz um debounce bloqueante de 20 ms (__delay_cycles).
 *    - <stdio.h> foi incluido para um sprintf() que nunca foi usado.
 *    - O limiar pratico de bancada e 1500, nao os 2048 teoricos do relatorio.
 *
 *  Alvo: MSP430F5529 (LaunchPad MSP-EXP430F5529LP)
 *  Toolchain usado na epoca: Code Composer Studio / msp430-gcc
 * ============================================================================
 */

#include <msp430.h>
#include <stdint.h>
#include <stdio.h> // Necessário para a função sprintf() formatar o texto do LCD

// ---------------------------------------------------------
// Configurações Gerais
// ---------------------------------------------------------
#define LIMIAR_GAS 1500
#define LCD_ADDR 0x27 // Endereço I2C do display (pode ser 0x3F dependendo do seu chip PCF8574)

volatile int alarme_mutado = 0;

// ---------------------------------------------------------
// Funções do Barramento I2C e LCD
// ---------------------------------------------------------
void config_I2C(void) {
    UCB0CTL1 |= UCSWRST;                  // Coloca a máquina I2C em Reset
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC; // Mestre, I2C, Síncrono
    UCB0CTL1 = UCSSEL_2 | UCSWRST;        // Clock = SMCLK (~1MHz)
    UCB0BRW  = 105;                       // Divisor para gerar velocidade segura (~10 kHz)

    P3OUT |= BIT0 | BIT1;                 // Define Pull-up para P3.0 e P3.1
    P3REN |= BIT0 | BIT1;                 // Habilita resistor interno
    P3SEL |= BIT0 | BIT1;                 // Avisa que os pinos farão a função I2C dedicada (SDA e SCL)

    UCB0CTL1 &= ~UCSWRST;                 // Tira a máquina I2C do Reset
}

uint8_t i2cSend(uint8_t addr, uint8_t data) {
    UCB0I2CSA = addr;                        // Define o endereço do escravo
    UCB0CTL1 |= UCTR | UCTXSTT;              // Mestre Transmissor e gera START
    
    while ((UCB0IFG & UCTXIFG) == 0);        // Espera o buffer ficar pronto
    UCB0TXBUF = data;                        // Joga o dado na linha
    
    while ((UCB0CTL1 & UCTXSTT) == UCTXSTT); // Espera o START ser enviado e checa ACK
    
    if ((UCB0IFG & UCNACKIFG) != 0) {        // Se houve NACK (Erro)
        UCB0CTL1 |= UCTXSTP;                 // Força o STOP
        while (UCB0CTL1 & UCTXSTP);
        return 1;                            // Retorna erro
    }
    
    while ((UCB0IFG & UCTXIFG) == 0);        // Espera o dado terminar de sair
    UCB0CTL1 |= UCTXSTP;                     // Gera STOP normal
    while ((UCB0CTL1 & UCTXSTP) == UCTXSTP); // Aguarda STOP concluir
    return 0;                                // Retorna sucesso
}

void lcdWriteNibble(uint8_t nibble, uint8_t isChar) {
    // Organiza os 4 bits superiores, liga a Luz de Fundo (0x08) e o bit RS (isChar)
    uint8_t data = (nibble & 0xF0) | 0x08 | (isChar ? 0x01 : 0x00);
    i2cSend(LCD_ADDR, data);        // Enable = 0
    i2cSend(LCD_ADDR, data | 0x04); // Enable = 1 (Pulso)
    i2cSend(LCD_ADDR, data);        // Enable = 0
}

void lcdWriteByte(uint8_t byte, uint8_t isChar) {
    lcdWriteNibble(byte & 0xF0, isChar);         // Envia os 4 bits mais significativos
    lcdWriteNibble((byte << 4) & 0xF0, isChar);  // Envia os 4 bits menos significativos
}

void lcdInit(void) {
    __delay_cycles(50000);          // Espera o LCD ligar
    lcdWriteNibble(0x30, 0);
    __delay_cycles(50000);
    lcdWriteNibble(0x30, 0);
    __delay_cycles(10000);
    lcdWriteNibble(0x30, 0);
    __delay_cycles(10000);
    lcdWriteNibble(0x20, 0);        // Força o LCD a entrar no Modo 4-bits
    __delay_cycles(10000);

    lcdWriteByte(0x28, 0);          // 4-bit, 2 linhas, fonte 5x8
    lcdWriteByte(0x0C, 0);          // Display ON, Cursor OFF
    lcdWriteByte(0x06, 0);          // Incrementa cursor automaticamente
    lcdWriteByte(0x01, 0);          // Limpa a tela toda
    __delay_cycles(20000);          // Atraso obrigatório de 1.53ms após limpar
}

void lcdWrite(char *str) {
    uint8_t col = 0;
    while (*str != '\0') {
        if (*str == '\n' || col == 16) {
            lcdWriteByte(0xC0, 0);  // Pula para a 2ª linha do display
            col = 0;
            if (*str == '\n') { str++; continue; }
        }
        lcdWriteByte(*str, 1);      // Escreve a letra (isChar = 1)
        col++;
        str++;
    }
}

// ---------------------------------------------------------
// Configurações do Sensor, Buzzer e Botão (S1)
// ---------------------------------------------------------
void config_ADC(void) {
    P6SEL |= BIT0;                          // Ativa entrada analógica A0 no P6.0
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;      // Sample time (16 clocks) e liga o ADC
    ADC12CTL1 = ADC12SHP;                   // Timer interno de amostragem
    ADC12CTL2 = ADC12RES_2;                 // Resolução de 12 bits
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0; // Vincula à entrada A0
}

void config_Buzzer(void) {
    P2DIR |= BIT4;
    P2SEL |= BIT4;                          // P2.5 controlado pelo Timer A2 (TA2.2)
    TA2CTL = TASSEL_2 | MC_1 | TACLR;       // SMCLK, Modo UP
    TA2CCR0 = 1000 - 1;                     // Frequência do som (1 kHz)
    TA2CCTL1 = OUTMOD_7;                    // PWM Reset/Set
    TA2CCR1 = 1000;                         // Buzzer inicia desligado
}

void config_Botao(void) {
    P2DIR &= ~BIT1;                         // P2.1 como Entrada
    P2REN |= BIT1;                          // Resistor interno
    P2OUT |= BIT1;                          // Resistor Pull-up
    P2IES |= BIT1;                          // Interrompe na borda de descida (aperto)
    P2IFG &= ~BIT1;                         // Limpa bandeira
    P2IE |= BIT1;                           // Habilita interrupção
}

void formata_linha_gas(int gas, char* buffer) {
    // Monta o texto base (índices 0 a 10)
    buffer[0]='N'; buffer[1]='i'; buffer[2]='v'; buffer[3]='e'; buffer[4]='l';
    buffer[5]=' '; buffer[6]='G'; buffer[7]='a'; buffer[8]='s'; buffer[9]=':'; buffer[10]=' ';
    
    // Calcula cada dígito e converte para caractere (índices 11 a 14)
    buffer[11] = (gas / 1000) % 10 + '0'; // Milhar
    buffer[12] = (gas / 100) % 10 + '0';  // Centena
    buffer[13] = (gas / 10) % 10 + '0';   // Dezena
    buffer[14] = gas % 10 + '0';          // Unidade
    
    // Finaliza a string
    buffer[15] = '\0';
}

// ---------------------------------------------------------
// Programa Principal
// ---------------------------------------------------------
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Para o Watchdog

    config_ADC();
    config_Buzzer();
    config_Botao();
    config_I2C();

    __enable_interrupt();                   // Chave Geral de Interrupções

    lcdInit();                              // Inicializa a tela LCD

    char linha1[17];                        // CORRIGIDO: Texto de 16 letras + terminador

    while(1) {
        // 1. Amostra o Gás
        ADC12CTL0 |= ADC12ENC | ADC12SC;
        while((ADC12IFG & BIT0) == 0);
        int nivel_gas = ADC12MEM0;

        // 2. Prepara a primeira linha do LCD (Sem sprintf)
        formata_linha_gas(nivel_gas, linha1);
        lcdWriteByte(0x80, 0);              // Força o cursor para Linha 1, Coluna 1
        lcdWrite(linha1);                   // Escreve na tela

        // 3. Lógica do Alarme e Atualização da segunda linha
        lcdWriteByte(0xC0, 0);              // Força o cursor para Linha 2, Coluna 1
        
        if (nivel_gas > LIMIAR_GAS) {
            if (alarme_mutado == 0) {
                TA2CCR1 = 500;              // Buzzer tocando (50% de Duty Cycle)
                lcdWrite("ALERTA DE GAS!  ");
            } else {
                TA2CCR1 = 1000;                // Buzzer silencioso
                lcdWrite("ALERTA (MUTADO) ");
            }
        } else {
            TA2CCR1 = 1000;                    // Buzzer desligado
            alarme_mutado = 0;              // Rearma a variável de mudo
            lcdWrite("Ambiente Seguro ");
        }

        // 4. Pequeno atraso (Atualiza a tela ~5 vezes por segundo)
        __delay_cycles(2000000); // +- uma leitura cada 2s
    }
}

// ---------------------------------------------------------
// Rotina de Serviço de Interrupção (Mudo do Alarme)
// ---------------------------------------------------------
#pragma vector = PORT2_VECTOR
__interrupt void ISR_Botao_S1(void) {
    if (P2IV == 0x04) {                     // Botão P2.1 (S1) pressionado
        alarme_mutado = 1;                  // Silencia
        __delay_cycles(20000);              // Debounce
    }
}
