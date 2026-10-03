# Bugs encontrados e melhorias

Registro honesto do que estava errado no código original do protótipo, o que foi corrigido no firmware
entregue e o que ficou conscientemente como **trabalho futuro**.

> **Base da revisão.** O código original (sketch único) foi reestruturado em módulos em
> [`firmware/src/`](../firmware/src/) e [`firmware/include/`](../firmware/include/). A lista abaixo foi
> levantada por leitura do código e está alinhada com o
> [README do projeto](../README.md) e com a seção 5 do
> [README do firmware](../firmware/README.md) (“Bugs corrigidos: antes → depois”), que traz o diff
> comentado de cada correção.
>
> **Legenda de status**
> - ✅ **Corrigido** — implementado no firmware atual.
> - 🟡 **Parcial** — mitigado, mas ainda depende de validação em bancada.
> - ⬜ **Trabalho futuro** — fora do escopo desta entrega, com justificativa.
> - 🔒 **Mantido de propósito** — comportamento preservado para não alterar o protótipo validado.

## Resumo

| # | Item | Tipo | Severidade | Status |
| --- | --- | --- | --- | --- |
| 1 | `lcdWrite()` testava a coluna **antes** de escrever | Bug funcional | Alta | ✅ Corrigido |
| 2 | Mensagens não preenchiam as 16 colunas (restos na tela) | Bug visual | Média | ✅ Corrigido |
| 3 | Comentário do buzzer dizia `P2.5/TA2.2`, mas o código usava P2.4 | Documentação/engano | Média | ✅ Corrigido |
| 4 | Debounce com `__delay_cycles(20000)` **dentro da ISR** do botão | Bug de arquitetura | Alta | ✅ Corrigido |
| 5 | `#include <stdio.h>` para um `sprintf()` nunca usado | Desperdício de flash | Baixa | ✅ Corrigido |
| 6 | Limiar `LIMIAR_GAS` espalhado e com nome ambíguo | Manutenibilidade | Média | ✅ Corrigido |
| 7 | `int` com sinal para o resultado do ADC | Bug de tipo | Média | ✅ Corrigido |
| 8 | Tratamento de NACK no I²C incompleto | Bug de robustez | Média | ✅ Corrigido |
| 9 | `ADC12ENC` não era habilitado na inicialização | Bug funcional | Alta | ✅ Corrigido |
| 10 | Sem espera de estabilização da referência do ADC | Bug de confiabilidade | Média | ✅ Corrigido |
| 11 | Flag `ADC12IFG` do canal não era limpa após a leitura | Bug funcional | Média | ✅ Corrigido |
| 12 | Comentário do atraso do LCD citava “1,53 ms” e usava 20 ms | Documentação | Baixa | ✅ Corrigido |
| 13 | Cadência de leitura por `__delay_cycles` (≈2 s), não por timer | Arquitetura | Alta | ⬜ Trabalho futuro |
| 14 | Decisão de alarme com uma única amostra do ADC | Robustez | Alta | ⬜ Trabalho futuro |
| 15 | LED1 (P1.0) previsto no relatório mas não acionado no firmware | Redundância de alerta | Alta | ✅ Corrigido |
| 16 | Silenciamento persistente até o nível cair (sem janela de tempo) | Comportamento | Média | 🔒 Mantido |
| 17 | Limiar fixo em 1500, sem caminho de autocalibração | Confiabilidade | Média | 🟡 Parcial |
| 18 | Calibração metrológica do MQ-2 (ppm) | Confiabilidade | Baixa | ⬜ Trabalho futuro |
| 19 | Tom do buzzer em ≈1 kHz (relatório previa 2 kHz) | Conforto acústico | Baixa | ⬜ Trabalho futuro |
| 19b | Buzzer em repouso fica com a saída em nível ALTO (consumo contínuo) | Consumo | Baixa | 🟡 Documentado |
| 20 | Sem watchdog e sem diagnóstico de falha do sensor | Robustez | Média | ⬜ Trabalho futuro |
| 21 | Caixa de proteção e instalação física | Produto | Alta (para uso real) | ⬜ Trabalho futuro |
| 22 | Notificação por celular (Wi-Fi/BLE/GSM) | Produto | Média | ⬜ Trabalho futuro |
| 23 | Nenhuma verificação fora da bancada | Qualidade | Média | ✅ Corrigido (simulação no PC) |
| 24 | Clock do I²C em ≈620 Hz (lento, mantido por fidelidade) | Desempenho | Baixa | 🔒 Mantido |

---

## 1. Bugs corrigidos

### 1. `lcdWrite()` testava a coluna antes de escrever ✅

**Sintoma:** a 17ª letra de uma mensagem era escrita na segunda linha em vez de ser descartada, podendo
sobrescrever texto já exibido.

**Causa:** a condição de quebra de linha (`col == 16`) era avaliada **antes** de escrever o caractere.

**Correção:** a quebra de linha e o preenchimento foram reescritos no módulo de texto
([`firmware/src/texto.c`](../firmware/src/texto.c)), com contagem correta de colunas e descarte do excedente
depois da linha 2.

### 2. Mensagens curtas deixavam restos na tela ✅

**Sintoma:** ao trocar `ALERTA DE GAS!` por `Ambiente Seguro`, sobravam caracteres da mensagem anterior.

**Correção:** cada linha é escrita **preenchida com espaços até as 16 colunas**, sem sobras.

### 3. Comentário do buzzer descrevia outro pino ✅

**Sintoma:** o comentário dizia “P2.5 controlado pelo Timer A2 (TA2.2)”, mas o código configurava o
**P2.4** (BIT4) e o canal 1 do Timer A2.

**Correção:** pino e comentário foram alinhados, e o valor passou a viver em
[`config.h`](../firmware/include/config.h). No MSP430F5529, o pino P2.4 corresponde à saída **TA2.1** —
o canal efetivamente usado no código (`TA2CCTL1`/`TA2CCR1`).

### 4. Debounce bloqueante dentro da ISR do botão ✅

**Sintoma:** `__delay_cycles(20000)` era executado **dentro da interrupção**, prendendo o processador
(20 ms sem ler o sensor nem atualizar o alarme).

**Correção:** a ISR ficou **curta** — apenas sinaliza o evento (`botao_evento = 1`) e lê `P2IV` para
distinguir o pino. O debounce de 20 ms é aplicado no contexto do laço principal, em
[`botao.c`](../firmware/src/botao.c).

### 5. `#include <stdio.h>` desnecessário ✅

**Sintoma:** o header era incluído para um `sprintf()` que nunca foi usado — arrastando código morto para a
flash.

**Correção:** os includes foram reduzidos ao que o código realmente usa.

### 6. Limiar espalhado e com nome ambíguo ✅

**Sintoma:** `LIMIAR_GAS` aparecia em vários pontos e não deixava claro que era um **valor de ADC** (0–4095),
não uma tensão.

**Correção:** renomeado para `LIMIAR_GAS_ADC` e centralizado em `config.h`, junto do valor teórico do
relatório (`LIMIAR_GAS_TEORICO_ADC = 2048`), para que a diferença entre o planejado e o praticado fique
rastreável.

### 7. Tipo do resultado do ADC ✅

**Sintoma:** o valor convertido (naturalmente `uint16_t`, 0–4095) era tratado como `int` com sinal.

**Correção:** tipos ajustados para `uint16_t` em todo o caminho (driver, formatação e decisão).

### 8. Tratamento de NACK no I²C ✅

**Sintoma:** a sequência de START/ACK não cobria todos os casos de NACK esperados pelo periférico UCB0, o
que podia deixar o barramento preso.

**Correção:** sequência START/ACK e liberação do barramento revisadas, com **timeout** de segurança
(`I2C_TIMEOUT_CICLOS`) para não travar o firmware se o LCD for desconectado.

### 9. `ADC12ENC` não era habilitado na inicialização ✅

**Sintoma:** o original mexia só em `ADC12SHT0_2 | ADC12ON` e confiava no `|= ADC12ENC` de cada leitura —
frágil e fora da ordem recomendada pelo datasheet.

**Correção:** `ADC12ENC` é habilitado na inicialização, depois da configuração completa.

### 10. Sem espera de estabilização da referência ✅

**Sintoma:** a primeira conversão podia acontecer antes de a referência estabilizar.

**Correção:** atraso de estabilização após ligar o ADC (`ADC12_CICLOS_REFERENCIA` ≈ 5 ms).

### 11. `ADC12IFG` do canal não era limpo ✅

**Sintoma:** a flag do canal permanecia setada após a leitura de `ADC12MEM0`, o que pode mascarar a próxima
conversão.

**Correção:** a flag é explicitamente limpa depois de ler o resultado.

### 12. Comentário do atraso do LCD ✅

**Sintoma:** o comentário dizia “atraso obrigatório de 1,53 ms” associado a `__delay_cycles(20000)` — que
são 20 ms a 1 MHz.

**Correção:** o comentário passou a descrever o que o código faz (20 ms é um valor seguro para o comando de
limpeza, cujo mínimo de datasheet é ~1,52 ms) e o valor foi nomeado em `config.h`.

### 23. Verificação fora da bancada ✅

**Como era:** a verificação dependia de olhar a placa.

**Como está:** a lógica de decisão vive em um módulo **puro** (`app.c`) e é exercitada no PC por
[`firmware/sim/`](../firmware/sim/) — casos de teste para abaixo do limiar, exatamente no limiar, acima,
mutado, rearme após a queda e formatação das 16 colunas do LCD. Sem MSP430, sem sensor e sem bancada.

> **Ressalva honesta:** o toolchain `msp430-elf-gcc` não estava disponível no ambiente de organização do
> repositório, portanto **o firmware não foi compilado para o alvo nem gravado na placa**. O que foi
> executado de verdade foram `app.c` e `texto.c` compilados no PC com `-DSIM_HOST` — o que cobre a **lógica
> de decisão** e o **layout da tela**, mas não os drivers (`adc`, `i2c`, `lcd`, `buzzer`, `botao`, `main`).
> Esses drivers foram escritos e revisados por leitura, não por execução.

---

## 2. Pendências e trabalho futuro

### 13. Cadência de leitura por timer, não por `__delay_cycles` ⬜

O laço principal espera ≈2 s com `__delay_cycles(APP_CICLOS_ENTRE_LEITURAS)`. Funciona, mas amarra o timing
ao clock e bloqueia o processador. O próprio relatório planeja a solução: interrupção de timer a cada
**100 ms** (SMCLK/8 = 125 kHz, tick de 8 µs, `CCR0 = 12499`). É a melhoria de maior impacto na
responsividade do detector.

### 14. Média de amostras antes de decidir ⬜

A decisão usa **uma única conversão**. O relatório prevê média de k amostras para reduzir falso disparo.
Sem isso, um pico de ruído ou uma fritura na cozinha podem acionar o alarme. A média (ou uma média móvel
exponencial) é a evolução natural.

### 15. LED1 sem acionamento no firmware ✅

O relatório e a arquitetura preveem **LED aceso** durante o alarme, como redundância visual para quem está
sem contato direto com o LCD e longe do som. O sketch original acionava buzzer e LCD, mas **não havia código
que ligasse o P1.0**.

**Status: corrigido no firmware organizado.** `config.h` ganhou `PORTA_LED`, `PINO_LED_BIT_NUM`,
`LED_DIR_REG`, `LED_OUT_REG` e `LED_PIN_BIT`; `app_decisao_t` passou a expor `led_alarme`; `main.c` faz
`P1DIR |= LED_PIN_BIT` na inicialização e espelha o estado a cada leitura. O LED fica **aceso acima do
limiar, inclusive com o buzzer silenciado**, e apaga quando o nível cai — exatamente o comportamento
descrito na seção 3.4.7 do relatório. Há teste dedicado no simulador (`teste_led`, 6 verificações).

### 16. Silenciamento sem janela de tempo 🔒

No firmware, o mute **persiste** enquanto o nível estiver acima do limiar e é **rearmado** quando o nível
cai. O relatório previa uma janela temporizada (o som volta sozinho depois de um tempo). O comportamento
atual foi mantido por fidelidade ao protótipo validado; trocar para janela temporizada é uma decisão de
produto a tomar com o usuário.

### 17. Limiar fixo, sem autocalibração 🟡

O limiar está em `LIMIAR_GAS_ADC = 1500`, valor obtido em bancada, centralizado em `config.h` e
documentado. O que falta é a **autocalibração**: medir o ar limpo na inicialização, aplicar uma margem e
derivar o limiar sozinho — além de compensação por temperatura e umidade.

### 18. Calibração metrológica do MQ-2 ⬜

Exibir concentração certificada em ppm exigiria câmara de calibração e instrumento de referência. Por isso
o LCD mostra o **valor bruto do ADC** relativo ao limiar, e não ppm — decisão registrada no relatório
(§ 3.4.3) e no [README](../README.md).

### 19. Tom do buzzer em ≈1 kHz ⬜

A implementação usa `TA2CCR0 = 999` (≈1,05 kHz), enquanto o relatório previa **2 kHz** (`CCR0 = 499`,
`CCR1 = 250`). O tom atual é audível, mas menos agudo; unificar com o planejado é uma linha de mudança em
`config.h`.

### 19b. Buzzer em "silêncio" fica com a saída em nível ALTO 🟡

No modo 7 (`OUTMOD_7`) do Timer A2, o pino fica em **nível alto** quando o CCR1 é maior que o CCR0 — e é
exatamente isso que a implementação faz para "desligar" o som (`BUZZER_CCR1_REPOUSO = 1000` > período 999).
Ou seja: o repouso não é 0 V, é **alto contínuo**. Com um buzzer passivo isso não gera som, mas mantém
corrente na base do transistor (e no buzzer) durante todo o tempo em que o sistema está calmo — o que é
relevante num dispositivo que fica ligado 24/7 e cujo público valoriza "baixo consumo de energia"
(10 menções na pesquisa).

**Correção recomendada:** em repouso, limpar o modo de saída (`TA2CCTL1 &= ~OUTMOD_7`) ou levar o pino para
nível baixo, em vez de usar CCR1 > CCR0. Não foi alterado aqui para manter fidelidade ao protótipo de
bancada, que funciona. **A página do projeto documenta o nível lógico corretamente** (nível alto = mudo),
mas nenhum documento trata o consumo decorrente.

### 20. Watchdog e diagnóstico de falha do sensor ⬜

O firmware **desliga o watchdog** (`WDTHOLD`) e não detecta leitura presa (0 ou 4095). Em um dispositivo
ligado 24/7, um travamento silencioso significa **nenhum alerta** — exatamente o cenário que o projeto quer
evitar. Recomenda-se habilitar o WDT com reset periódico e sinalizar falha do sensor no LCD.

### 21. Caixa de proteção e instalação física ⬜

Para uso real, é necessário projetar uma **caixa ventilada** (o MQ-2 precisa de contato com o ar), definir
a posição a ~30 cm do fogão/botijão e validar se o volume do alarme é audível em outros cômodos — requisito
apontado por **95%** dos participantes da pesquisa.

### 22. Notificação por celular ⬜

**12 pessoas** citaram “integração com celular” como característica desejada, mas o recurso exigiria módulo
Wi-Fi, Bluetooth ou GSM, aumentando custo, consumo e complexidade — contra a proposta de manter o projeto
simples e barato. Fica registrado como evolução prioritária de uma eventual versão 2.

### 24. Clock do I²C em ≈620 Hz 🔒

O divisor do barramento está em `I2C_DIVISOR = 105`. No USCI_B a taxa em modo mestre é
`f_SCL = SMCLK / ((UCB0BRW + 1) × 16)`, então `1.048.576 / (106 × 16)` ≈ **620 Hz** — muito abaixo
dos 100 kHz máximos da especificação I²C.

O comentário original dizia "~10 kHz" e a documentação derivou "~9,5 kHz": as duas contas esqueceram o
fator **16** interno do periférico. O valor foi **mantido por fidelidade ao código que funciona na
bancada** e está comentado em `config.h`/`i2c.c`. Com a cadência atual (≈2 s por leitura), a lentidão não
incomoda; ela passa a importar se a atualização do LCD for acelerada para o intervalo de 500 ms previsto no
relatório. Para acelerar basta `UCB0BRW = 0` (SMCLK/16 ≈ 65 kHz).

---

## 3. O que ficou fora do escopo (e por quê)

| Fora do escopo | Motivo |
| --- | --- |
| Concentração certificada em ppm | Exigiria calibração metrológica e câmara de gás |
| Notificação por celular | Custo, consumo e complexidade de módulos de rádio |
| Múltiplos sensores por residência | A pesquisa sugere 1,5 pontos por casa; a montagem demo usa um sensor |
| Certificação como equipamento de segurança | O protótipo é didático e **não substitui equipamentos comerciais certificados** |
| Carcaça injetada / PCB dedicada | Montagem em protoboard para demonstração em laboratório |

---

## 4. Caminho sugerido para a versão 2

1. **Cadência por timer (100 ms)** — tira o `__delay_cycles` do laço e resolve o item 13.
2. **Média de amostras** — reduz falso disparo sem custo de hardware (item 14).
3. ~~**LED1 no alarme**~~ — **já feito** (item 15): a redundância som + luz + texto está fechada.
4. **Autocalibração na inicialização** — mede o ar limpo e define o limiar sozinho (item 17).
5. **Watchdog + diagnóstico de sensor** — o dispositivo precisa falhar de forma visível (item 20).
6. **Caixa ventilada com suporte de parede** — requisito de instalação e de aceitação dos usuários (item 21).
7. **Conectividade opcional (BLE)** — publica apenas o evento de alarme, mantendo o custo baixo (item 22).

---

## 5. Documentos relacionados

| Documento | Conteúdo |
| --- | --- |
| [`docs/relatorio.md`](relatorio.md) | Relatório acadêmico (inclui a seção de limitações) |
| [`docs/engenharia.md`](engenharia.md) | Arquitetura, pinagem, fórmulas e a comparação relatório × implementação |
| [`docs/pesquisa.md`](pesquisa.md) | Pesquisa que originou os requisitos |
| [`README.md`](../README.md) | Visão geral, bugs corrigidos e limitações do projeto |
| [`firmware/README.md`](../firmware/README.md) | Pinagem, build, testes e o diff comentado de cada correção |
| [`hardware/lista-de-materiais.md`](../hardware/lista-de-materiais.md) | BOM e custos |
