/*
 * texto.c - implementacao portatil do layout de texto do LCD.
 * ------------------------------------------------------------------
 * Regra implementada (correcao do bug do lcdWrite original):
 *   1. a quebra de linha acontece DEPOIS de completar as 16 colunas,
 *      nunca antes de escrever o 16o caractere;
 *   2. ao terminar, a linha corrente e completada com espacos, de modo
 *      que texto antigo mais longo nao deixe "restos" na tela;
 *   3. depois da ultima coluna da ultima linha o excedente e
 *      DESCARTADO, em vez de reposicionar o cursor no inicio da tela e
 *      sobrescrever o que ja foi escrito.
 * ==================================================================
 */
#include "texto.h"

/* Completa a linha corrente com espacos ate LCD_COLUNAS. */
static void texto_preenche(texto_emissor_t emissor, void *ctx, uint8_t *coluna)
{
    while (*coluna < (uint8_t)LCD_COLUNAS) {
        emissor(ctx, TEXTO_CMD_CHAR, (uint8_t)' ');
        (*coluna)++;
    }
}

static uint8_t texto_comando_da_linha(uint8_t linha)
{
    return (uint8_t)((linha == 0u) ? LCD_CMD_LINHA1 : LCD_CMD_LINHA2);
}

void texto_ajusta_linha16(char *destino, const char *origem)
{
    uint8_t i = 0u;

    if (destino == 0) {
        return;
    }
    if (origem != 0) {
        while ((i < (uint8_t)LCD_COLUNAS) && (origem[i] != '\0')) {
            destino[i] = origem[i];
            i++;
        }
    }
    while (i < (uint8_t)LCD_COLUNAS) {
        destino[i] = ' ';
        i++;
    }
    destino[LCD_COLUNAS] = '\0';
}

void texto_escreve_em(texto_emissor_t emissor, void *ctx,
                      const char *texto, uint8_t linha_inicial)
{
    uint8_t coluna = 0u;
    uint8_t linha;

    if ((emissor == 0) || (texto == 0)) {
        return;
    }

    linha = (linha_inicial < (uint8_t)LCD_LINHAS) ? linha_inicial
                                                  : (uint8_t)(LCD_LINHAS - 1);
    emissor(ctx, TEXTO_CMD_POSICAO, texto_comando_da_linha(linha));

    while (*texto != '\0') {
        if (*texto == '\n') {
            texto_preenche(emissor, ctx, &coluna);
            if ((uint8_t)(linha + 1u) >= (uint8_t)LCD_LINHAS) {
                return;                     /* fim da tela: ignora o resto */
            }
            linha++;
            coluna = 0u;
            emissor(ctx, TEXTO_CMD_POSICAO, texto_comando_da_linha(linha));
            texto++;
            continue;
        }

        if (coluna >= (uint8_t)LCD_COLUNAS) {
            if ((uint8_t)(linha + 1u) >= (uint8_t)LCD_LINHAS) {
                return;   /* linha 2 cheia: trunca em vez de sobrescrever */
            }
            linha++;
            coluna = 0u;
            emissor(ctx, TEXTO_CMD_POSICAO, texto_comando_da_linha(linha));
        }

        emissor(ctx, TEXTO_CMD_CHAR, (uint8_t)(*texto));
        coluna++;
        texto++;
    }

    texto_preenche(emissor, ctx, &coluna);
}

void texto_escreve(texto_emissor_t emissor, void *ctx, const char *texto)
{
    texto_escreve_em(emissor, ctx, texto, 0u);
}
