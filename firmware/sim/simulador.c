/*
 * simulador.c - demonstracao do detector de gas no PC (host).
 * ------------------------------------------------------------------
 * Roda o laco principal do firmware com leituras simuladas do sensor e
 * mostra a tela 2x16 resultante, o tom do buzzer (TA2CCR1) e a decisao.
 * Usa as MESMAS funcoes do firmware (src/app.c, src/texto.c) compiladas
 * com -DSIM_HOST; o que muda e apenas o "hardware": aqui o LCD e uma
 * tela virtual (sim/lcd_virtual.c) em vez de I2C + PCF8574.
 *
 * Compilar (exemplo):
 *   gcc -std=c99 -Wall -Wextra -DSIM_HOST -Iinclude -Isim \
 *       src/app.c src/texto.c sim/lcd_virtual.c sim/simulador.c -o simulador
 * ==================================================================
 */
#include <stdio.h>

#include "app.h"
#include "config.h"
#include "lcd_virtual.h"
#include "texto.h"

typedef struct {
    uint16_t nivel;
    uint8_t mutar;   /* 1 = botao de mute pressionado antes da leitura */
} passo_t;

static const passo_t ROTEIRO[] = {
    {  120u, 0u },   /* ar limpo                                  */
    {  900u, 0u },   /* ainda seguro                              */
    { 1500u, 0u },   /* exatamente no limiar => seguro            */
    { 1501u, 0u },   /* primeiro nivel de alerta                  */
    { 3200u, 0u },   /* vazamento forte                           */
    { 3200u, 1u },   /* usuario pressiona o botao (mute)          */
    { 2900u, 0u },   /* continua alto, porem silenciado           */
    { 1500u, 0u },   /* gas cai: mute e rearmado                  */
    { 3400u, 0u },   /* novo pico volta a soar                    */
    {  250u, 0u }    /* volta ao normal                           */
};

static const char *nome_situacao(app_situacao_t situacao)
{
    switch (situacao) {
        case APP_SEGURO:        return "SEGURO";
        case APP_ALERTA:        return "ALERTA";
        case APP_ALERTA_MUTADO: return "ALERTA MUTADO";
        default:                return "?";
    }
}

int main(void)
{
    app_estado_t estado;
    size_t i;

    printf("=================================================================\n");
    printf(" Simulador host - Detector de Vazamento de Gas (MSP430F5529)\n");
    printf(" limiar = %u contagens (ADC 12 bits) | LCD %dx%d\n",
           (unsigned)LIMIAR_GAS_ADC, (unsigned)LCD_LINHAS, (unsigned)LCD_COLUNAS);
    printf("=================================================================\n");

    app_inicia(&estado);

    for (i = 0u; i < (sizeof(ROTEIRO) / sizeof(ROTEIRO[0])); i++) {
        char linha1[LCD_TAM_LINHA];
        app_tela_t tela;
        lcd_virtual_t lcd;
        app_decisao_t decisao;

        if (ROTEIRO[i].mutar != 0u) {
            printf("\n-- botao de mute pressionado --\n");
            app_mutar(&estado);
        }

        /* --- mesma sequencia de main.c --- */
        decisao = app_avalia(&estado, ROTEIRO[i].nivel);
        formata_linha_gas(ROTEIRO[i].nivel, linha1);

        /* Tela em memoria (usada nos testes) */
        app_tela_monta(&tela, linha1, decisao.mensagem_lcd);

        /* Tela virtual alimentada pelo MESMO emissor de texto.c que o
         * driver lcd.c usa no firmware. */
        lcd_virtual_inicia(&lcd);
        texto_escreve_em(lcd_virtual_emissor, &lcd, linha1, 0u);
        texto_escreve_em(lcd_virtual_emissor, &lcd, decisao.mensagem_lcd, 1u);

        printf("\npasso %2u: ADC=%4u  TA2CCR1=%4u  LED1=%s  %s\n",
               (unsigned)(i + 1u), (unsigned)ROTEIRO[i].nivel,
               (unsigned)decisao.tom_buzzer,
               (decisao.led_alarme != 0u) ? "aceso  " : "apagado",
               nome_situacao(decisao.situacao));
        printf("           tela montada : |%s|%s|\n", tela.linha[0], tela.linha[1]);
        printf("           tela virtual : |%s|%s|\n", lcd.linha[0], lcd.linha[1]);
    }

    printf("\nSimulacao concluida: %u passos, mute final = %u\n",
           (unsigned)(sizeof(ROTEIRO) / sizeof(ROTEIRO[0])), (unsigned)estado.mutado);
    return 0;
}
