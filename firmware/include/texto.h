/*
 * texto.h - layout de texto portatil para o LCD 16x2.
 * ------------------------------------------------------------------
 * Este modulo NAO conhece hardware: ele apenas produz a sequencia de
 * comandos/caracteres que um driver de LCD deve executar, por meio de
 * um "emissor" (ponteiro de funcao). Com isso a MESMA implementacao de
 * quebra de linha e preenchimento roda:
 *   - no firmware, com um emissor que fala I2C com o PCF8574 (lcd.c);
 *   - no simulador host, com um emissor que escreve numa tela virtual.
 * E isso que torna a prova da logica honesta: nao ha copia da regra.
 * ==================================================================
 */
#ifndef TEXTO_H
#define TEXTO_H

#include <stdint.h>
#include "config.h"

/* comando = QUE comando; valor = parametro (posicao ou caractere) */
typedef void (*texto_emissor_t)(void *ctx, uint8_t comando, uint8_t valor);

#define TEXTO_CMD_POSICAO  0x80u  /* valor = LCD_CMD_LINHA1|LCD_CMD_LINHA2 */
#define TEXTO_CMD_CHAR     0x01u  /* valor = caractere a escrever          */

/*
 * Copia no maximo 16 caracteres de `origem` para `destino`, completa
 * com espacos ate 16 colunas e termina com '\0' (destino >= 17 bytes).
 */
void texto_ajusta_linha16(char *destino, const char *origem);

/*
 * Escreve `texto` a partir da linha `linha_inicial` (0 ou 1), quebrando
 * linha ao completar 16 colunas, completando a ultima linha com espacos
 * e DESCARTANDO o excedente depois da linha 2 (evita escrever alem da
 * tela / sobrescrever texto).
 */
void texto_escreve_em(texto_emissor_t emissor, void *ctx,
                      const char *texto, uint8_t linha_inicial);

/* Mesma coisa, comecando na linha 1. */
void texto_escreve(texto_emissor_t emissor, void *ctx, const char *texto);

#endif /* TEXTO_H */
