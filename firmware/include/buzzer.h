/*
 * buzzer.h - buzzer passivo no Timer A2 (P2.4).
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "config.h"

/*
 * Configura P2.4 como saida do Timer A2 (canal TA2.1: TA2CCTL1/TA2CCR1)
 * em modo reset/set (OUTMOD_7).
 *
 * BUG CORRIGIDO: o comentario original dizia "P2.5 controlado pelo
 * Timer A2 (TA2.2)", mas o codigo sempre configurou P2.4 (BIT4) e o
 * canal 1 (TA2CCTL1/TA2CCR1). O codigo estava certo para o hardware
 * montado; apenas o comentario estava errado.
 */
void buzzer_inicia(void);

/*
 * Define o tom via TA2CCR1:
 *   BUZZER_CCR1_REPOUSO (1000) => duty 100% => sem som;
 *   BUZZER_CCR1_ALARME  (500)  => duty 50%  => tom audivel.
 */
void buzzer_tom(uint16_t ccr1);

#endif /* BUZZER_H */
