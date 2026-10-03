/*
 * adc.c - ADC12, canal A0, 12 bits (mesmos registradores do original).
 * ------------------------------------------------------------------
 * Correcoes:
 *   - ADC12ENC e habilitado na inicializacao (o original so mexia em
 *     ADC12SHT0_2|ADC12ON e confiava no |= ADC12ENC de cada leitura);
 *   - espera de estabilizacao da referencia (__delay_cycles) apos ligar
 *     o ADC, como manda o datasheet;
 *   - ADC12IFG do canal e limpo apos ler ADC12MEM0;
 *   - o valor lido e uint16_t (antes era int).
 * ==================================================================
 */
#include "adc.h"

void adc_inicia(void)
{
    ADC_SEL_REG |= ADC_SEL_BIT;             /* P6SEL |= BIT0 (A0)      */
    ADC12CTL0 = ADC12_TEMPO_AMOSTRAGEM | ADC12ON;
    ADC12CTL1 = ADC12SHP;                   /* sample-and-hold pulsado */
    ADC12CTL2 = ADC12_RESOLUCAO_CFG;        /* 12 bits                 */
    ADC12MCTL0 = ADC12_REFERENCIA_CFG | ADC12_CANAL_CFG;

    ADC12CTL0 |= ADC12ENC;                  /* habilita a conversao    */
    __delay_cycles(ADC12_CICLOS_REFERENCIA);/* referencia estabiliza   */
}

uint16_t adc_le_valor(void)
{
    uint16_t valor;

    ADC12CTL0 |= ADC12ENC | ADC12SC;        /* dispara a conversao     */
    while ((ADC12IFG & ADC_BIT_CANAL) == 0u) {
        /* aguarda o fim da conversao */
    }

    valor = (uint16_t)ADC12MEM0;
    ADC12IFG &= (uint16_t)~ADC_BIT_CANAL;   /* limpa a flag do canal   */
    return valor;
}
