/*
 * i2c.c - driver USCI_B0 (I2C mestre) para o expander PCF8574 do LCD.
 * ------------------------------------------------------------------
 * Preserva os MESMOS registradores do sketch original (UCB0*), com tres
 * correcoes de robustez:
 *   1. a espera do START+endereco (UCTXSTT) vem ANTES de escrever o
 *      dado em UCB0TXBUF - no original o dado era carregado antes de o
 *      endereco ter sido confirmado;
 *   2. todas as esperas tem timeout (barramento preso nao trava a ISR);
 *   3. UCNACKIFG e limpo explicitamente e o STOP e sempre emitido,
 *      deixando o periferico livre para a proxima transferencia.
 * ==================================================================
 */
#include "i2c.h"

static void i2c_para(void)
{
    UCB0CTL1 |= UCTXSTP;
    while ((UCB0CTL1 & UCTXSTP) == UCTXSTP) {
        /* aguarda o STOP ser concluido */
    }
}

static uint8_t i2c_espera_stt_concluido(void)
{
    uint16_t timeout = (uint16_t)I2C_TIMEOUT_CICLOS;

    while (((UCB0CTL1 & UCTXSTT) == UCTXSTT) && (timeout > 0u)) {
        timeout--;
    }
    return (timeout > 0u) ? I2C_OK : I2C_ERRO_TIMEOUT;
}

static uint8_t i2c_espera_txifg(void)
{
    uint16_t timeout = (uint16_t)I2C_TIMEOUT_CICLOS;

    while (((UCB0IFG & UCTXIFG) == 0u) && (timeout > 0u)) {
        timeout--;
    }
    return (timeout > 0u) ? I2C_OK : I2C_ERRO_TIMEOUT;
}

void i2c_inicia(void)
{
    I2C_SEL_REG |= I2C_SEL_BITS;   /* P3SEL |= BIT0|BIT1 (SDA/SCL) */
    I2C_REN_REG |= I2C_SEL_BITS;   /* pull-up/pull-down interno     */
    I2C_OUT_REG |= I2C_SEL_BITS;

    UCB0CTL1 |= UCSWRST;           /* segura o periferico em reset  */
    UCB0CTL0  = I2C_CFG_CTL0;      /* mestre, I2C, sincrono         */
    UCB0CTL1  = I2C_CFG_CTL1;      /* SMCLK + reset                 */
    UCB0BRW   = I2C_DIVISOR;       /* ver observacao no README      */
    UCB0CTL1 &= (uint8_t)~UCSWRST; /* libera o periferico           */
}

uint8_t i2c_envia_byte(uint8_t endereco, uint8_t dado)
{
    UCB0I2CSA = endereco;
    UCB0CTL1 |= UCTR | UCTXSTT;    /* modo transmissor + START */

    /* 1) espera START + endereco serem transmitidos */
    if (i2c_espera_stt_concluido() != I2C_OK) {
        i2c_para();
        return I2C_ERRO_TIMEOUT;
    }

    /* 2) so agora faz sentido olhar o NACK do endereco */
    if ((UCB0IFG & UCNACKIFG) != 0u) {
        UCB0IFG &= (uint8_t)~UCNACKIFG;
        i2c_para();
        return I2C_NACK;
    }

    /* 3) transmite o dado e aguarda o ACK */
    if (i2c_espera_txifg() != I2C_OK) {
        i2c_para();
        return I2C_ERRO_TIMEOUT;
    }
    UCB0TXBUF = dado;
    if (i2c_espera_txifg() != I2C_OK) {
        i2c_para();
        return I2C_ERRO_TIMEOUT;
    }

    i2c_para();
    return I2C_OK;
}
