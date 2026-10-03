/*
 * app.c - logica de decisao (identica no firmware e no simulador host).
 * ------------------------------------------------------------------
 * Reproduz fielmente o comportamento do sketch original:
 *   - nivel > LIMIAR_GAS_ADC  => alarme;
 *       - alarme nao mutado  => TA2CCR1 = 500  ("ALERTA DE GAS!")
 *       - alarme mutado      => TA2CCR1 = 1000 ("ALERTA (MUTADO)")
 *   - nivel <= LIMIAR_GAS_ADC => TA2CCR1 = 1000, mute REARMADO
 *                                ("Ambiente Seguro")
 * Diferencas conscientes em relacao ao original (ver README.md):
 *   - o texto vai para uma tela 2x16 preenchida com espacos, sem restos;
 *   - o buffer da linha 1 e montado com uint16_t (antes era int).
 * ==================================================================
 */
#include "app.h"

void app_inicia(app_estado_t *estado)
{
    if (estado != 0) {
        estado->mutado = 0u;
    }
}

uint8_t app_acima_do_limiar(uint16_t nivel_adc)
{
    /* Comparacao ESTRITAMENTE MAIOR, igual ao sketch original:
     * exatamente no limiar ainda e "seguro". */
    return (nivel_adc > (uint16_t)LIMIAR_GAS_ADC) ? 1u : 0u;
}

app_decisao_t app_avalia(app_estado_t *estado, uint16_t nivel_adc)
{
    app_decisao_t decisao;

    if (app_acima_do_limiar(nivel_adc) != 0u) {
        if (estado->mutado == 0u) {
            decisao.situacao     = APP_ALERTA;
            decisao.tom_buzzer   = (uint16_t)BUZZER_CCR1_ALARME;
            decisao.mensagem_lcd = "ALERTA DE GAS!";
        } else {
            decisao.situacao     = APP_ALERTA_MUTADO;
            decisao.tom_buzzer   = (uint16_t)BUZZER_CCR1_REPOUSO;
            decisao.mensagem_lcd = "ALERTA (MUTADO)";
        }
        /* Decisao de seguranca: o LED permanece aceso enquanto houver risco,
         * inclusive com o buzzer silenciado. O botao silencia o SOM, nunca a
         * indicacao visual - ver secao 3.4.7 do relatorio. */
        decisao.led_alarme = 1u;
    } else {
        estado->mutado       = 0u;   /* rearme automatico do mute */
        decisao.situacao     = APP_SEGURO;
        decisao.tom_buzzer   = (uint16_t)BUZZER_CCR1_REPOUSO;
        decisao.led_alarme   = 0u;
        decisao.mensagem_lcd = "Ambiente Seguro";
    }

    decisao.mutado = estado->mutado;
    return decisao;
}

void app_mutar(app_estado_t *estado)
{
    /* O sketch original silencia em qualquer pressionamento, mesmo com o
     * ambiente seguro (o mute so e desfeito na proxima leitura baixa).
     * Comportamento mantido de proposito. */
    if (estado != 0) {
        estado->mutado = 1u;
    }
}

void formata_linha_gas(uint16_t nivel_gas, char *buffer)
{
    static const char prefixo[] = "Nivel Gas: ";
    uint8_t i;

    if (buffer == 0) {
        return;
    }

    for (i = 0u; i < 11u; i++) {
        buffer[i] = prefixo[i];
    }
    buffer[11] = (char)(((nivel_gas / 1000u) % 10u) + (uint16_t)'0');
    buffer[12] = (char)(((nivel_gas / 100u) % 10u) + (uint16_t)'0');
    buffer[13] = (char)(((nivel_gas / 10u) % 10u) + (uint16_t)'0');
    buffer[14] = (char)((nivel_gas % 10u) + (uint16_t)'0');
    buffer[15] = '\0';
}

void app_tela_limpa(app_tela_t *tela)
{
    uint8_t l;
    uint8_t c;

    if (tela == 0) {
        return;
    }
    for (l = 0u; l < (uint8_t)LCD_LINHAS; l++) {
        for (c = 0u; c < (uint8_t)LCD_COLUNAS; c++) {
            tela->linha[l][c] = ' ';
        }
        tela->linha[l][LCD_COLUNAS] = '\0';
    }
    tela->linha_atual  = 0u;
    tela->coluna_atual = 0u;
}

void app_tela_emissor(void *ctx, uint8_t comando, uint8_t valor)
{
    app_tela_t *tela = (app_tela_t *)ctx;

    if (tela == 0) {
        return;
    }

    if (comando == TEXTO_CMD_POSICAO) {
        tela->linha_atual  = (valor == (uint8_t)LCD_CMD_LINHA2) ? 1u : 0u;
        tela->coluna_atual = 0u;
        return;
    }

    if ((comando == TEXTO_CMD_CHAR) && (tela->coluna_atual < (uint8_t)LCD_COLUNAS)) {
        tela->linha[tela->linha_atual][tela->coluna_atual] = (char)valor;
        tela->coluna_atual++;
        tela->linha[tela->linha_atual][LCD_COLUNAS] = '\0';
    }
}

void app_tela_monta(app_tela_t *tela, const char *linha1, const char *linha2)
{
    if (tela == 0) {
        return;
    }
    app_tela_limpa(tela);
    texto_escreve_em(app_tela_emissor, tela, linha1, 0u);
    texto_escreve_em(app_tela_emissor, tela, linha2, 1u);
}
