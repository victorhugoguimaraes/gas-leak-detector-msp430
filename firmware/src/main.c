/*
 * main.c - Detector de Vazamento de Gas (MSP430F5529)
 * ------------------------------------------------------------------
 * Programa principal: apenas orquestra os modulos. Nenhum registrador,
 * pino, limiar ou tempo e usado diretamente aqui - tudo vem de config.h
 * e dos drivers (adc, buzzer, botao, i2c, lcd, app).
 *
 * Laco principal (mesma sequencia do sketch original):
 *   1. verifica o botao de mute (debounce fora da ISR) e aplica o mute;
 *   2. le o sensor no ADC12 (A0 / P6.0);
 *   3. avalia a regra de decisao (app_avalia);
 *   4. aplica o tom no buzzer (TA2CCR1);
 *   5. acende o LED1 (P1.0) enquanto houver risco;
 *   6. escreve as duas linhas de 16 colunas no LCD;
 *   7. espera ~2 s e repete.
 *
 * O botao vem PRIMEIRO de proposito: como o laco termina com um atraso de
 * ~2 s, verificar o botao no fim faria uma pressao confirmada silenciar o
 * buzzer so na volta seguinte — ou seja, depois de mais um ciclo inteiro de
 * alarme. No inicio da volta, o mute recem-confirmado ja vale para a leitura
 * que vem logo abaixo.
 * ==================================================================
 */
#include "config.h"
#include "app.h"
#include "adc.h"
#include "botao.h"
#include "buzzer.h"
#include "i2c.h"
#include "lcd.h"

int main(void)
{
    app_estado_t estado;
    char linha1[LCD_TAM_LINHA];

    WDTCTL = WDTPW | WDTHOLD;   /* desliga o watchdog */

    adc_inicia();
    buzzer_inicia();
    botao_inicia();
    i2c_inicia();

    /* LED1 (P1.0) como saida digital. O sketch original citava o LED no
     * relatorio mas nao o configurava; ver config.h secao "LED1". */
    LED_DIR_REG |= LED_PIN_BIT;
    LED_OUT_REG &= (uint8_t)~LED_PIN_BIT;

    __enable_interrupt();
    lcd_inicia();

    app_inicia(&estado);

    for (;;) {
        uint16_t nivel_gas;
        app_decisao_t decisao;

        /* 1. O botao e verificado na PRIMEIRA coisa da volta, como no sketch
         *    original: uma pressao confirmada silencia o alarme ja nesta
         *    leitura, sem esperar o atraso de ~2 s do fim do laco. */
        if (botao_pressao_confirmada() != 0u) {
            app_mutar(&estado);
        }

        /* 2. So depois le o sensor e avalia a decisao, para que o mute
         *    recem-confirmado ja valha para este ciclo. */
        nivel_gas = adc_le_valor();
        decisao   = app_avalia(&estado, nivel_gas);

        formata_linha_gas(nivel_gas, linha1);
        buzzer_tom(decisao.tom_buzzer);

        if (decisao.led_alarme != 0u) {
            LED_OUT_REG |= LED_PIN_BIT;
        } else {
            LED_OUT_REG &= (uint8_t)~LED_PIN_BIT;
        }

        lcd_escreve_linha(0u, linha1);
        lcd_escreve_linha(1u, decisao.mensagem_lcd);

        __delay_cycles(APP_CICLOS_ENTRE_LEITURAS);
    }

    /* return inalcancavel (o compilador do MSP430 nao reclama) */
}
