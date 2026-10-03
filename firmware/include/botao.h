/*
 * botao.h - botao de mute (P2.1) com interrupcao e debounce.
 */
#ifndef BOTAO_H
#define BOTAO_H

#include <stdint.h>
#include "config.h"

/*
 * P2.1 com pull-up interno, interrupcao na borda de descida.
 *
 * BUG CORRIGIDO: a ISR original fazia __delay_cycles(20000) — 20 ms de
 * espera ocupada DENTRO da interrupcao, segurando o processador e
 * atrasando qualquer outra interrupcao nesse intervalo. Agora a ISR
 * apenas sinaliza o evento (uma unica leitura de P2IV) e o debounce de
 * 20 ms roda no laco principal, fora do caminho critico.
 *
 * Nota: P2IV devolve o vetor de maior prioridade pendente e e limpo pela
 * propria leitura, por isso a ISR le o registrador UMA vez e usa o valor
 * em um switch — reler o registrador poderia devolver 0.
 */
void botao_inicia(void);

/*
 * Devolve 1 se houve um pressionamento VALIDO desde a ultima chamada.
 * Faz o debounce (BOTAO_DEBOUNCE_MS) FORA da ISR e confirma que o pino
 * continua em nivel baixo. Nao bloqueia a ISR, apenas o laco principal.
 */
uint8_t botao_pressao_confirmada(void);

#endif /* BOTAO_H */
