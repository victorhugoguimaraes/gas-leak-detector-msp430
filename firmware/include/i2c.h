/*
 * i2c.h - driver do modulo USCI_B0 em modo I2C mestre (PCF8574 do LCD).
 */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "config.h"

/* Codigos de retorno de i2c_envia_byte() - 0 = sucesso, != 0 = erro,
 * mesma convencao do i2cSend() original (que retornava 1 em NACK). */
#define I2C_OK           0u
#define I2C_NACK         1u
#define I2C_ERRO_TIMEOUT 2u

/* Configura P3.0/P3.1 como UCB0SDA/UCB0SCL e habilita o mestre. */
void i2c_inicia(void);

/*
 * Envia 1 byte para o escravo `endereco`:
 *   START -> endereco -> ACK? -> dado -> ACK? -> STOP
 * Ordem corrigida: espera o START+endereco terminarem ANTES de checar
 * o NACK e ANTES de carregar UCB0TXBUF. Todas as esperas tem timeout
 * para nao travar o firmware com o barramento preso.
 */
uint8_t i2c_envia_byte(uint8_t endereco, uint8_t dado);

#endif /* I2C_H */
