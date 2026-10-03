/*
 * lcd.c - driver do LCD 16x2 (HD44780) via PCF8574 em I2C.
 * ------------------------------------------------------------------
 * O PCF8574 tem 8 bits: P0 = RS, P1 = RW, P2 = E, P3 = backlight
 * (e por isso 0x08 = backlight ligado e 0x04 = pulso de Enable).
 * A escrita em 4 bits manda a nibble alta e depois a baixa.
 *
 * O bug do lcdWrite() original foi removido: a formatacao/quebra de
 * linha agora vem de texto.c, que quebra DEPOIS das 16 colunas,
 * completa a linha com espacos e trunca o excedente. Antes, uma linha
 * de 16 caracteres fazia o cursor voltar para a coluna 0 da linha 2 e
 * sobrescrevia texto ja escrito, e textos mais curtos deixavam restos.
 * ==================================================================
 */
#include "lcd.h"
#include "i2c.h"
#include "texto.h"

static uint8_t lcd_ultimo_erro = I2C_OK;

static void lcd_escreve_nibble(uint8_t nibble, uint8_t eh_caractere)
{
    uint8_t dado = (uint8_t)((nibble & 0xF0u) | (uint8_t)LCD_BIT_BACKLIGHT);

    if (eh_caractere != 0u) {
        dado |= (uint8_t)LCD_BIT_RS;
    }

    lcd_ultimo_erro = i2c_envia_byte((uint8_t)LCD_I2C_ENDERECO, dado);
    (void)i2c_envia_byte((uint8_t)LCD_I2C_ENDERECO, (uint8_t)(dado | (uint8_t)LCD_BIT_ENABLE));
    (void)i2c_envia_byte((uint8_t)LCD_I2C_ENDERECO, dado);
}

void lcd_escreve_byte(uint8_t byte, uint8_t eh_caractere)
{
    lcd_escreve_nibble((uint8_t)(byte & 0xF0u), eh_caractere);
    lcd_escreve_nibble((uint8_t)((byte << 4) & 0xF0u), eh_caractere);
}

void lcd_comando(uint8_t comando)
{
    lcd_escreve_byte(comando, 0u);
}

void lcd_limpa(void)
{
    lcd_comando((uint8_t)LCD_CMD_CLEAR);
    __delay_cycles(LCD_CICLOS_LIMPEZA);   /* datasheet: > 1,52 ms */
}

void lcd_inicia(void)
{
    __delay_cycles(LCD_CICLOS_POWER_ON);  /* > 40 ms apos energizar */
    lcd_escreve_nibble(0x30u, 0u);
    __delay_cycles(LCD_CICLOS_POWER_ON);
    lcd_escreve_nibble(0x30u, 0u);
    __delay_cycles(LCD_CICLOS_INIT);
    lcd_escreve_nibble(0x30u, 0u);
    __delay_cycles(LCD_CICLOS_INIT);
    lcd_escreve_nibble(0x20u, 0u);        /* entra em modo 4 bits   */
    __delay_cycles(LCD_CICLOS_INIT);

    lcd_escreve_byte((uint8_t)LCD_CMD_FUNCTION_SET, 0u);
    lcd_escreve_byte((uint8_t)LCD_CMD_DISPLAY_ON, 0u);
    lcd_escreve_byte((uint8_t)LCD_CMD_ENTRY_MODE, 0u);
    lcd_limpa();
}

/* Emissor que traduz os comandos portateis de texto.c para o LCD real. */
static void lcd_emissor_i2c(void *ctx, uint8_t comando, uint8_t valor)
{
    (void)ctx;

    if (comando == TEXTO_CMD_POSICAO) {
        lcd_comando(valor);
    } else if (comando == TEXTO_CMD_CHAR) {
        lcd_escreve_byte(valor, 1u);
    } else {
        /* comando desconhecido: ignora */
    }
}

void lcd_escreve_linha(uint8_t linha, const char *texto)
{
    texto_escreve_em(lcd_emissor_i2c, 0, texto, linha);
}

uint8_t lcd_erro_i2c(void)
{
    return lcd_ultimo_erro;
}
