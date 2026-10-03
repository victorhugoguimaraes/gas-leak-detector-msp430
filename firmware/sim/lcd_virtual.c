/*
 * lcd_virtual.c - modelo do LCD 16x2 e reproducao do bug original.
 */
#include <stdio.h>
#include <string.h>

#include "lcd_virtual.h"
#include "texto.h"   /* TEXTO_CMD_POSICAO / TEXTO_CMD_CHAR */

static void lcd_virtual_preenche(lcd_virtual_t *lcd, uint8_t linha)
{
    uint8_t c;

    for (c = 0u; c < (uint8_t)LCD_COLUNAS; c++) {
        lcd->linha[linha][c] = ' ';
    }
    lcd->linha[linha][LCD_COLUNAS] = '\0';
}

void lcd_virtual_inicia(lcd_virtual_t *lcd)
{
    if (lcd == 0) {
        return;
    }
    memset(lcd, 0, sizeof(*lcd));
    lcd_virtual_preenche(lcd, 0u);
    lcd_virtual_preenche(lcd, 1u);
    lcd->cursor_linha  = 0u;
    lcd->cursor_coluna = 0u;
}

void lcd_virtual_comando(lcd_virtual_t *lcd, uint8_t comando)
{
    if (lcd == 0) {
        return;
    }

    if (comando == (uint8_t)LCD_CMD_CLEAR) {
        lcd_virtual_preenche(lcd, 0u);
        lcd_virtual_preenche(lcd, 1u);
        lcd->cursor_linha  = 0u;
        lcd->cursor_coluna = 0u;
    } else if (comando == (uint8_t)LCD_CMD_LINHA1) {
        lcd->cursor_linha  = 0u;
        lcd->cursor_coluna = 0u;
    } else if (comando == (uint8_t)LCD_CMD_LINHA2) {
        lcd->cursor_linha  = 1u;
        lcd->cursor_coluna = 0u;
    } else {
        /* outros comandos nao alteram o modelo */
    }
}

void lcd_virtual_caractere(lcd_virtual_t *lcd, char c)
{
    if (lcd == 0) {
        return;
    }
    /* Fora da area visivel (coluna >= 16) o modelo ignora o caractere:
     * o cursor so volta para dentro da tela se o codigo emitir um
     * comando de posicao - exatamente o que o bug original fazia. */
    if (lcd->cursor_coluna < (uint8_t)LCD_COLUNAS) {
        lcd->linha[lcd->cursor_linha][lcd->cursor_coluna] = c;
        lcd->linha[lcd->cursor_linha][LCD_COLUNAS] = '\0';
        lcd->cursor_coluna++;
    }
}

void lcd_virtual_emissor(void *ctx, uint8_t comando, uint8_t valor)
{
    lcd_virtual_t *lcd = (lcd_virtual_t *)ctx;

    if (lcd == 0) {
        return;
    }
    if (comando == TEXTO_CMD_POSICAO) {
        lcd_virtual_comando(lcd, valor);
    } else if (comando == TEXTO_CMD_CHAR) {
        lcd_virtual_caractere(lcd, (char)valor);
    } else {
        /* ignora */
    }
}

void lcd_virtual_escreve_original(lcd_virtual_t *lcd, const char *texto)
{
    uint8_t col = 0u;

    if ((lcd == 0) || (texto == 0)) {
        return;
    }

    while (*texto != '\0') {
        if ((*texto == '\n') || (col == (uint8_t)LCD_COLUNAS)) {
            lcd_virtual_comando(lcd, (uint8_t)LCD_CMD_LINHA2);
            col = 0u;
            if (*texto == '\n') {
                texto++;
                continue;
            }
        }
        lcd_virtual_caractere(lcd, *texto);
        col++;
        texto++;
    }
}

void lcd_virtual_imprime(const lcd_virtual_t *lcd)
{
    if (lcd == 0) {
        return;
    }
    printf("|%s|\n|%s|\n", lcd->linha[0], lcd->linha[1]);
}
