/*
 * buzzer.c - Timer A2 gerando tom no pino P2.4.
 */
#include "buzzer.h"

void buzzer_inicia(void)
{
    BUZZER_DIR_REG |= BUZZER_PIN_BIT;   /* P2DIR |= BIT4  (saida)      */
    BUZZER_SEL_REG |= BUZZER_PIN_BIT;   /* P2SEL |= BIT4  (funcao TA2) */

    TA2CTL   = BUZZER_TA_CFG;                          /* SMCLK, up, limpo */
    TA2CCR0  = (uint16_t)(BUZZER_PERIODO_CCR0 - 1u);
    TA2CCTL1 = OUTMOD_7;                               /* reset/set        */
    TA2CCR1  = (uint16_t)BUZZER_CCR1_REPOUSO;          /* comeca mudo      */
}

void buzzer_tom(uint16_t ccr1)
{
    TA2CCR1 = ccr1;
}
