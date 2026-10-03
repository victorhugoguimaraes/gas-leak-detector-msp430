/*
 * lcd_virtual.h - LCD 16x2 virtual do simulador host.
 * ------------------------------------------------------------------
 * Modela a tela (2 linhas x 16 colunas) e o cursor do HD44780 como o
 * driver real os movimenta. A partir do mesmo emissor de texto.c, a
 * tela virtual e alimentada pela MESMA rotina de layout usada pelo
 * firmware - por isso os testes valem como prova da logica embarcada.
 *
 * Tambem contem uma reproducao FIEL do lcdWrite() original (com bug),
 * usada nos testes para demonstrar o defeito e a correcao.
 * ==================================================================
 */
#ifndef LCD_VIRTUAL_H
#define LCD_VIRTUAL_H

#include <stdint.h>
#include "config.h"

typedef struct {
    char linha[LCD_LINHAS][LCD_TAM_LINHA];
    uint8_t cursor_linha;
    uint8_t cursor_coluna;
} lcd_virtual_t;

void lcd_virtual_inicia(lcd_virtual_t *lcd);

/* Comandos que interessam ao modelo: 0x80 (linha 1), 0xC0 (linha 2), 0x01 (limpa). */
void lcd_virtual_comando(lcd_virtual_t *lcd, uint8_t comando);

/* Escreve um caractere na posicao do cursor e avanca a coluna. */
void lcd_virtual_caractere(lcd_virtual_t *lcd, char c);

/* Emissor compativel com texto_emissor_t (usado por texto.c). */
void lcd_virtual_emissor(void *ctx, uint8_t comando, uint8_t valor);

/*
 * Reproducao fiel do lcdWrite() do sketch original:
 *   - a checagem `col == 16` acontece ANTES de escrever o proximo
 *     caractere, entao o caractere 17 reposiciona o cursor em 0xC0 e
 *     sobrescreve o inicio da linha 2;
 *   - nao ha preenchimento com espacos (restos de texto antigo);
 *   - `col` e local ao texto, ignorando a posicao real do cursor.
 */
void lcd_virtual_escreve_original(lcd_virtual_t *lcd, const char *texto);

/* Imprime a tela entre barras: |linha 1| / |linha 2|. */
void lcd_virtual_imprime(const lcd_virtual_t *lcd);

#endif /* LCD_VIRTUAL_H */
