# Código original

`detector_gas_msp430.c` é o **sketch original** do projeto, exatamente como foi
escrito e rodou na bancada. Ele está preservado como documento histórico.

**Não é o firmware do repositório.** O firmware mantido, organizado em módulos e
testado, está em [`../src/`](../src/) e [`../include/`](../include/).

## Por que manter o original versionado

1. **Rastreabilidade.** Toda correção e toda linha da tabela de bugs em
   [`../README.md`](../README.md) pode ser conferida contra o código que a
   originou.
2. **É uma peça de portfólio honesta.** Mostra o ponto de partida real — um
   arquivo único que funcionou — e torna visível o trabalho de organizar,
   documentar e testar, em vez de apresentar só o resultado final arrumado.
3. **Comparação direta.** A lógica de decisão é idêntica; o que muda é
   estrutura, tratamento de bordas e legibilidade. Ter os dois lado a lado deixa
   essa diferença evidente.

## O que mudou entre este arquivo e `../src/`

| Aspecto | Original | Firmware organizado |
| :--- | :--- | :--- |
| Estrutura | um arquivo | módulos: `app`, `texto`, `i2c`, `lcd`, `adc`, `buzzer`, `botao` |
| Configuração | `#define` espalhados | tudo em [`../include/config.h`](../include/config.h) |
| Linha do LCD | `col == 16` testado antes de escrever, sem padding | quebra e padding corretos, truncamento seguro |
| LED1 (P1.0) | citado no relatório, nunca configurado | implementado e testado |
| ISR do botão | debounce bloqueante de 20 ms | ISR curta, debounce fora do caminho crítico |
| `#include <stdio.h>` | presente para um `sprintf()` não usado | removido |
| Tipo do nível | `int` | `uint16_t` |
| I2C | sem timeout | com timeout e limpeza de NACK |
| Testes | nenhum | 128 verificações em C (`../sim/testes.c`) |

## Observação

Este arquivo **não compila** como parte do build de `../src/` — ele é
intencionalmente independente e mantém os defeitos originais. Não o edite: se
precisar registrar uma correção, ela vai para `../src/` e para
[`../../docs/BUGS_E_MELHORIAS.md`](../../docs/BUGS_E_MELHORIAS.md).
