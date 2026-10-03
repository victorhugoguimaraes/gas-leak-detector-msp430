/*
 * adc.h - leitura do sensor de gas no ADC12 (canal A0 / P6.0).
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "config.h"

/*
 * Configura P6.0 como A0, liga o ADC12 em 12 bits e HABILITA a conversao
 * (ADC12ENC). BUG CORRIGIDO: o original nunca setava ADC12ENC na
 * configuracao e nao esperava a referencia estabilizar.
 */
void adc_inicia(void);

/* Dispara uma conversao, espera o fim e devolve 0..4095. */
uint16_t adc_le_valor(void);

#endif /* ADC_H */
