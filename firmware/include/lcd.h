/*
 * lcd.h - driver do LCD 16x2 com backpack PCF8574 (I2C).
 */
#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "config.h"

/* Inicializa o LCD em 4 bits (sequencia do datasheet HD44780). */
void lcd_inicia(void);

/* Escreve um byte (comando ou dado) em duas nibbles. */
void lcd_escreve_byte(uint8_t byte, uint8_t eh_caractere);

/* Atalho para comando. */
void lcd_comando(uint8_t comando);

/* Limpa o display (comando 0x01 + tempo de execucao). */
void lcd_limpa(void);

/*
 * Escreve `texto` na linha `linha` (0 ou 1) usando o layout portatil de
 * texto.c: quebra correta em 16 colunas, preenchimento com espacos e
 * truncamento do excedente. Substitui o lcdWrite() original.
 */
void lcd_escreve_linha(uint8_t linha, const char *texto);

/* Ultimo erro de I2C observado (0 = nenhum). */
uint8_t lcd_erro_i2c(void);

#endif /* LCD_H */
