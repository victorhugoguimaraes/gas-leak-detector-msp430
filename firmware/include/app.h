/*
 * app.h - logica de decisao do detector de gas (pura, sem hardware).
 * ------------------------------------------------------------------
 * Este modulo concentra TODA a regra de negocio do produto:
 *   - formatacao da linha "Nivel Gas: NNNN" (15 caracteres, fiel ao
 *     codigo original; o preenchimento para 16 colunas e feito pelo
 *     driver de LCD no momento de escrever);
 *   - comparacao com LIMIAR_GAS_ADC (estritamente maior);
 *   - estado de alarme: soa (CCR1 = 500) ou silencioso (CCR1 = 1000);
 *   - mute pelo botao, com rearme automatico quando o gas cai.
 * Nao inclui <msp430.h> quando compilado com -DSIM_HOST, o que permite
 * rodar exatamente esta logica no simulador host e nos testes.
 * ==================================================================
 */
#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "config.h"
#include "texto.h"

/* Situacao exibida/atuada a cada leitura. */
typedef enum {
    APP_SEGURO = 0,        /* nivel <= limiar                          */
    APP_ALERTA,            /* nivel > limiar e alarme ativo (soando)   */
    APP_ALERTA_MUTADO      /* nivel > limiar com alarme silenciado     */
} app_situacao_t;

/* Estado persistente entre leituras. */
typedef struct {
    uint8_t mutado;        /* 1 = usuario silenciou o alarme */
} app_estado_t;

/* Resultado de uma avaliacao: o que o hardware deve fazer agora. */
typedef struct {
    app_situacao_t situacao;
    uint16_t tom_buzzer;         /* valor a gravar em TA2CCR1            */
    uint8_t led_alarme;          /* 1 = LED1 aceso (P1.0 em nivel alto)  */
    uint8_t mutado;              /* estado do mute APOS a avaliacao      */
    const char *mensagem_lcd;    /* texto da linha 2 (sem padding)       */
} app_decisao_t;

/* Tela 2x16 montada em memoria (usada no firmware e no simulador). */
typedef struct {
    char linha[LCD_LINHAS][LCD_TAM_LINHA];
    uint8_t linha_atual;
    uint8_t coluna_atual;
} app_tela_t;

/* ---------- estado ---------- */
void app_inicia(app_estado_t *estado);

/* ---------- decisao ---------- */
uint8_t app_acima_do_limiar(uint16_t nivel_adc);
app_decisao_t app_avalia(app_estado_t *estado, uint16_t nivel_adc);
void app_mutar(app_estado_t *estado);

/* ---------- linha 1 ---------- */
/*
 * Escreve em `buffer` (>= 17 bytes) a string "Nivel Gas: NNNN"
 * (15 caracteres + '\0'), com a mesma matematica do codigo original.
 */
void formata_linha_gas(uint16_t nivel_gas, char *buffer);

/* ---------- tela 2x16 (portatil) ---------- */
void app_tela_limpa(app_tela_t *tela);
void app_tela_emissor(void *ctx, uint8_t comando, uint8_t valor);
void app_tela_monta(app_tela_t *tela, const char *linha1, const char *linha2);

#endif /* APP_H */
