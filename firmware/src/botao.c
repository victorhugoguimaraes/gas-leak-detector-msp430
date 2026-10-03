/*
 * botao.c - botao de mute (P2.1) com ISR curta + debounce no laco principal.
 */
#include "botao.h"

/* Sinalizado pela ISR; consumido por botao_pressao_confirmada(). */
static volatile uint8_t botao_evento = 0u;

void botao_inicia(void)
{
    BOTAO_DIR_REG &= (uint16_t)~BOTAO_PIN_BIT;  /* P2DIR &= ~BIT1 (entrada) */
    BOTAO_REN_REG |= BOTAO_PIN_BIT;             /* habilita resistor        */
    BOTAO_OUT_REG |= BOTAO_PIN_BIT;             /* pull-up                  */
    BOTAO_IES_REG |= BOTAO_PIN_BIT;             /* borda de descida         */
    BOTAO_IFG_REG &= (uint16_t)~BOTAO_PIN_BIT;  /* limpa flag pendente      */
    BOTAO_IE_REG  |= BOTAO_PIN_BIT;             /* habilita interrupcao     */
    botao_evento = 0u;
}

uint8_t botao_pressao_confirmada(void)
{
    if (botao_evento == 0u) {
        return 0u;
    }
    botao_evento = 0u;

    /* Debounce no contexto do laco principal (a ISR nao bloqueia mais). */
    __delay_cycles(BOTAO_CICLOS_DEBOUNCE);

    /* Botao em pull-up: nivel baixo = continua pressionado = pressao valida. */
    return ((BOTAO_IN_REG & BOTAO_PIN_BIT) == 0u) ? 1u : 0u;
}

#ifndef SIM_HOST
#pragma vector = PORT2_VECTOR
__interrupt void ISR_Botao_Mute(void)
{
    /* A leitura de P2IV limpa a flag de maior prioridade pendente.
     * Usa-se switch para nao reavaliar o registrador mais de uma vez. */
    switch (P2IV) {
        case BOTAO_P2IV_VALOR:      /* P2.1 => P2IV = 0x04 */
            botao_evento = 1u;
            break;
        default:
            break;                  /* outro pino da porta 2: ignora */
    }
}
#endif /* SIM_HOST */
