# Firmware — Detector de Vazamento de Gás (MSP430F5529)

Firmware do detector de gás baseado em MSP430F5529 (sensor MQ em A0/P6.0,
LCD 16x2 via PCF8574 em I2C, buzzer passivo no Timer A2 e botão de mute).

O projeto partiu de **um único sketch** com tudo dentro do `main()` e foi
reorganizado em módulos, com os pinos/limiares centralizados em `config.h`,
bugs corrigidos e a lógica de decisão **provada por testes** que rodam no PC.

---

## 1. Estrutura de arquivos

```
firmware/
├── Makefile                  # all (MSP430) | clean | sim | simulador | python-sim
├── README.md                 # este arquivo
├── include/
│   ├── config.h              # FONTE ÚNICA: pinos, limiares, endereços e tempos
│   ├── app.h                 # decisão (limiar, mute, mensagens) + tela 2x16
│   ├── texto.h               # layout de 16 colunas (portátil, testável no PC)
│   ├── i2c.h                 # USCI_B0 mestre (PCF8574)
│   ├── lcd.h                 # HD44780 4 bits via I2C
│   ├── adc.h                 # ADC12 canal A0
│   ├── buzzer.h              # Timer A2 / P2.4
│   └── botao.h               # botão P2.1 + debounce
├── src/
│   ├── main.c                # só orquestra (nenhum registro/pino direto)
│   ├── app.c                 # REGRA DE DECISÃO (compilada também no PC)
│   ├── texto.c               # quebra/padding de linha (compilada também no PC)
│   ├── i2c.c  lcd.c  adc.c  buzzer.c  botao.c
└── sim/
    ├── lcd_virtual.h/.c      # LCD 16x2 virtual + reprodução do bug original
    ├── testes.c              # testes em C (prova principal)
    ├── simulador.c           # demonstração do laço principal no PC
    ├── modelo.py             # espelho em Python do firmware (fallback)
    ├── testes.py             # mesmos testes em Python puro
    └── simulador.py          # demonstração em Python puro
```

Módulo extra em relação à lista original: **`texto.c/h`**. Ele existe porque o
layout de 16 colunas precisa ser *o mesmo* no firmware (que fala I2C) e no
simulador (que escreve numa tela virtual). Em vez de duplicar a regra, o
driver `lcd.c` e a tela virtual do simulador usam o **mesmo** emissor de
`texto.c` — é isso que faz o teste valer como prova do código embarcado.

---

## 2. Pinagem — implementação real × relatório

O código original (que funciona em bancada) usa uma pinagem diferente da
descrita no relatório. **A implementação real foi mantida como padrão do
build** e todos os pinos estão em `config.h`.

| Sinal | Conforme o relatório | Conforme a implementação real | Decisão |
|---|---|---|---|
| Sensor de gás | A0 | **P6.0 / A0** (`ADC12INCH_0`) | igual |
| LCD 16x2 | PCF8574 I2C | **UCB0: P3.0 = SDA, P3.1 = SCL**, endereço `0x27` | igual |
| Buzzer | P2.0 / TA1.1 | **P2.4** (BIT4) com **Timer A2** — registradores `TA2CCTL1`/`TA2CCR1` (canal TA2.1) | mantido o real |
| Botão de mute | S2 em P1.1 | **P2.1** (`P2IV == 0x04`) | mantido o real |
| LED1 | P1.0 | **P1.0** (`LED_DIR_REG`/`LED_OUT_REG` em `config.h`) | **implementado** |
| Limiar de gás | 1,65 V ⇒ ≈ 2048 contagens | **1500 contagens** (valor de bancada) | mantido o real |

Observações:

* P2.4 é o pino cujos registradores o código realmente configura
  (`TA2CCTL1 = OUTMOD_7`, `TA2CCR1`), ou seja, o canal **TA2.1** do Timer A2.
  Vale conferir a Tabela 4 (Terminal Functions) do datasheet SLAS590 ao montar
  o hardware — o comentário antigo dizia "P2.5 / TA2.2", que não correspondia a
  nenhuma das duas coisas que o código fazia.
* Na LaunchPad MSP-EXP430F5529LP os dois botões de usuário são **P2.1 e P1.1**;
  qual deles recebe o rótulo S1/S2 depende da revisão da placa (conferir o
  user's guide SLAU533 e a serigrafia). O relatório cita "S2 em P1.1", o código
  usa **P2.1** — para o build o que importa é o pino P2.1.
* Nada foi "consertado" **na pinagem** dessa divergência: trocar pino aqui
  quebraria a placa já montada. A tabela acima é o registro oficial da diferença.
* **LED1 passou a ser implementado.** O sketch original citava o LED no
  relatório, mas nunca configurava `P1.0` — o alerta visual simplesmente não
  existia no código. Como o relatório descreve o LED permanecendo aceso
  enquanto houver risco (§3.4.7), e tanto o diagrama quanto a documentação do
  projeto o mostram, o firmware organizado passou a dirigir `P1.0`: acende
  acima do limiar, apaga abaixo, e **permanece aceso mesmo com o buzzer
  silenciado**. É coberto pelos testes do simulador.

### As duas (e apenas duas) mudanças de comportamento em relação ao sketch

Em relação a `../original/detector_gas_msp430.c`, o firmware organizado tem
exatamente duas diferenças de comportamento — o resto é correção de bug,
estrutura ou legibilidade:

| # | Mudança | Por quê |
|---|---|---|
| 1 | **LED1 (P1.0) passa a ser dirigido** | O sketch original nunca configurava o pino; o alerta visual prometido no relatório não existia |
| 2 | **O botão volta a ser verificado no início da volta do laço** | Na primeira versão desta refatoração o botão era checado no *fim* da volta, depois do LCD e imediatamente antes do `__delay_cycles` de ~2 s. Com isso uma pressão confirmada só silenciava o buzzer na volta seguinte, ou seja, depois de mais um ciclo inteiro de alarme. Restaurar a ordem do original (`main.c`: botão → ADC → decisão → atuadores → atraso) devolve o silenciamento já na leitura em que ele é confirmado |

Fora esses dois pontos, limiar (1500, comparado com `>` estrito), tons (500/1000),
as três mensagens de LCD, o rearme automático do mute e o formato
`Nivel Gas: NNNN` são **idênticos** ao sketch original.

### Cobertura dos testes (o que 128 verificações provam — e o que não)

Os testes compilam `src/app.c` e `src/texto.c` com `-DSIM_HOST`: são os mesmos
arquivos do firmware, então a lógica de decisão, a formatação das 16 colunas e
o comportamento do LED estão de fato provados no PC.

**Não estão cobertos por teste automatizado:** `i2c.c`, `lcd.c`, `adc.c`,
`buzzer.c`, `main.c` e `botao.c`. Esses arquivos foram revisados linha a linha
contra o original, mas **não passaram por nenhum compilador** neste ambiente,
porque o toolchain `msp430-elf-gcc` não está disponível. Em particular, o
debounce de 20 ms do botão é lógica dependente de tempo que só a bancada valida
de verdade — a mudança de comportamento nº 2 acima foi verificada por leitura de
código, não por execução.

### Limiar: 1500 × 2048

`config.h` guarda os **dois** valores, com nomes explícitos:

```c
#define LIMIAR_GAS_ADC           1500u  /* limiar prático de bancada   */
#define LIMIAR_GAS_TEORICO_MV    1650u  /* 1,65 V citado no relatório  */
#define LIMIAR_GAS_TEORICO_ADC   2048u  /* 1,65 V / 3,3 V × 4096       */
```

O firmware usa `LIMIAR_GAS_ADC` (1500). A comparação é **estritamente maior**
(`nivel > LIMIAR_GAS_ADC`), igual ao sketch original: exatamente 1500 ainda é
"ambiente seguro" — isso está coberto por teste.

---

## 3. Como compilar para o MSP430 (toolchain da TI)

> ⚠️ **O toolchain `msp430-elf-gcc` NÃO está instalado nesta máquina.**
> Nada neste repositório foi compilado para o MSP430. O que foi executado e
> provado aqui é o **simulador host** (seção 5).

Pré-requisitos (instalar em uma máquina com o toolchain):

1. **MSP430 GCC** da TI (`msp430-elf-gcc`, `msp430-elf-objcopy`)
   — pacote *MSP430 GCC Compiler* do site da TI ou `msp430-gcc` do MSYS2.
2. **msp430-gcc-support-files** (headers/ldscripts do F5529) e apontar o
   caminho na variável `SUPPORT_DIR`.

```bash
cd firmware
make all SUPPORT_DIR=/opt/ti/msp430-gcc/include     # gera build/gas-leak-detector.elf/.hex
make clean
```

Se o toolchain não estiver no PATH, o alvo `all` para com uma mensagem clara:

```
$ make all
ERRO: msp430-elf-gcc nao encontrado. Este alvo exige o toolchain da TI,
que NAO esta instalado nesta maquina. Use "make sim" para validar a logica no PC.
```

A saída real desse caso está em `build/saida-make-all.txt`.

### Como gravar

Com o toolchain e um MSP430F5529 LaunchPad (eZ-FET integrado):

```bash
# opção 1: gdb + openocd/DSLite
msp430-elf-gdb build/gas-leak-detector.elf -ex "target remote :55000" -ex load -ex run

# opção 2: TI UniFlash / MSP430 Flasher com o .hex
MSP430Flasher -n MSP430F5529 -w build/gas-leak-detector.hex -v -z [VCC]
```

Ligação mínima: sensor em P6.0/A0 (com o divisor 10 kΩ/18 kΩ se o módulo for
alimentado em 5 V), SDA/SCL em P3.0/P3.1 com pull-ups de 4,7 kΩ para **3,3 V**
(o módulo I²C do LCD costuma trazer pull-ups próprios, mas eles precisam ir para
3,3 V e não para 5 V), buzzer passivo em P2.4 com transistor NPN, e botão de
P2.1 para GND.

---

## 4. Como rodar a simulação e os testes no PC

O simulador compila `src/app.c` + `src/texto.c` com `-DSIM_HOST` — ou seja,
**as funções reais do firmware**, sem `<msp430.h>` — e liga o layout de texto a
um LCD 16x2 virtual (`sim/lcd_virtual.c`).

### 4.1 Caminho principal (C, com gcc)

```bash
cd firmware
make sim          # compila e roda os testes
make simulador    # demonstra o laço principal (10 leituras)
```

Nesta máquina o `gcc` **não está no PATH** (por isso `make sim` precisa de um
`gcc` acessível). O compilador usado foi o do MSYS2:

```
C:\msys64\mingw64\bin\gcc.exe   ->  gcc (Rev3, Built by MSYS2 project) 16.2.0
```

Comandos exatos executados (PATH com `C:\msys64\mingw64\bin` à frente):

```powershell
mingw32-make sim
mingw32-make simulador
```

Resultado real dos testes (log completo em `build/saida-testes.txt`):

```
-----------------------------------------------------------------
Verificacoes: 128 | falhas: 0
TODOS OS TESTES PASSARAM
```

Coleta: `rc_make_sim=0`, `rc_make_simulador=0`. A compilação roda com
`-std=c99 -Wall -Wextra -Wpedantic -O2` **sem nenhum aviso**.

> Nota de ambiente: o `make` do MSYS2 (`C:\msys64\usr\bin\make.exe`) **não
> roda neste sandbox** — ele aborta com `couldn't create signal pipe,
> Win32 error 5` (restrição do sandbox a pipes nomeados). Por isso foi usado o
> `mingw32-make` (GNU Make 4.4.1, que usa `cmd.exe`). Com `sh` no PATH o
> Makefile usa automaticamente o ramo POSIX (`mkdir -p` / `rm -rf`).

### 4.2 Fallback em Python puro (sem compilador C)

```powershell
C:\Users\victo\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe sim\testes.py
```

Resultado real (log em `build/saida-testes-py.txt`):

```
-----------------------------------------------------------------
Verificacoes: 119 | falhas: 0
TODOS OS TESTES PASSARAM
```

`sim/modelo.py` é um **espelho** fiel de `app.c`/`texto.c`/`lcd_virtual.c`,
incluindo o LED da seção 4. Este caminho existe para máquinas sem compilador C;
a prova principal continua sendo o programa em C, que chama o código do próprio
firmware. O `Makefile` também tem `make python-sim`.

### 4.3 O que os testes cobrem

`sim/testes.c` faz **128 verificações**; o espelho em Python
(`sim/testes.py`) faz **119** e cobre os mesmos grupos.

| # | Grupo | Casos |
|---|---|---|
| 1 | `config.h` | limiar 1500, teóricos 1650 mV/2048, LCD 16x2, P2.4, P2.1, `P2IV=0x04`, A0, tons 500/1000, **P1.0 (LED)** |
| 2 | Limiar | 0, 1499, **1500 (exato ⇒ seguro)**, 1501, 2048, 4095 |
| 3 | Mute | alarme sem mute, mute durante alarme, persistência, **rearme ao cair**, volta a soar no pico seguinte, mute com gás baixo |
| — | **LED1 (P1.0)** | apagado em 0 e no limiar exato; aceso acima e no fundo de escala; **continua aceso com o buzzer mutado**; apaga quando o gás cai |
| 4 | Formatação | 0, 1, 999, 1500, 4095 ⇒ `"Nivel Gas: NNNN"` (15 chars) + padding para 16 |
| 5 | 16 colunas | truncamento em 16, padding, 40 chars ⇒ 2 linhas, `\n`, excedente descartado |
| 6 | Telas | `"Ambiente Seguro "`, `"ALERTA DE GAS!  "`, `"ALERTA (MUTADO) "` (todas com 16 colunas) |
| 7 | Bug original | reprodução do `lcdWrite()` com bug × versão corrigida (restos, sobrescrita, coluna errada) |
| 8 | Equivalência | tela montada × sequência de escrita do driver dão o mesmo resultado |
| 9 | Simulação | 10 leituras em sequência com invariantes de tom e rearme |

> O bloco do LED é impresso sem número (`-- LED1 (P1.0) --`), logo depois do
> grupo [3], tanto no C quanto no Python.

**O que essas 128 verificações NÃO cobrem** (importante para não superestimar a
evidência):

| Arquivo | Coberto por teste? | Por quê |
|---|---|---|
| `app.c`, `texto.c` | **sim** | São compilados com `-DSIM_HOST` — os mesmos arquivos do alvo |
| `botao.c` | **não** | Não está no `SIM_COMUM` do Makefile. O mute testado é o `app_mutar()` (a *decisão*), não o caminho do botão: o debounce de 20 ms e a ISR de `P2IV` não são executados |
| `i2c.c`, `lcd.c` | parcialmente | O *layout* de 16 colunas é testado por `texto.c`; o barramento I²C e o driver do PCF8574, não |
| `adc.c`, `buzzer.c`, `main.c` | **não** | Dependem de registradores do MSP430 (`ADC12*`, `TA2*`, `WDTCTL`) que não existem no host |

Nenhum desses arquivos não cobertos passou por **compilador** neste ambiente,
porque o `msp430-elf-gcc` não está instalado — só o `app.c`/`texto.c` são
compilados de fato, para o PC.

---

## 5. Bugs corrigidos (antes → depois)

### 5.1 `config_Buzzer()`: comentário mentia sobre o pino

* **Antes:** `// P2.5 controlado pelo Timer A2 (TA2.2)` mas o código fazia
  `P2DIR |= BIT4; P2SEL |= BIT4;` e usava `TA2CCTL1`/`TA2CCR1` (canal 1).
* **Depois:** comentário e nomes descrevem o que o código faz — **P2.4**,
  canal **TA2.1** (`TA2CCTL1`/`TA2CCR1`), tudo em `config.h`
  (`PINO_BUZZER_BIT_NUM 4`, `PORTA_BUZZER 2`).
* **Por quê:** comentário errado é bug de manutenção: quem for trocar o pino
  mexe no lugar errado. O código estava correto para a placa montada — só o
  comentário estava errado, então nenhum pino foi alterado.

### 5.2 `lcdWrite()`: quebra de linha antes de escrever, sem limpeza e sem limite

* **Antes:**
  ```c
  if (*str == '\n' || col == 16) { lcdWriteByte(0xC0, 0); col = 0; ... }
  ```
  Problemas: (a) o `col` é local ao texto e ignora a coluna real do cursor;
  (b) texto mais curto **não limpa** o que estava antes (restos na tela);
  (c) texto maior que 32 caracteres volta a reposicionar em `0xC0` e
  **sobrescreve** o início da linha 2.
* **Depois:** `texto.c` — quebra **depois** de completar 16 colunas, preenche a
  última linha com espaços e **descarta** o excedente (nunca reposiciona para
  reescrever). `lcd.c` usa esse layout via `lcd_escreve_linha()`.
* **Prova:** teste [7] roda o algoritmo antigo e o novo sobre a mesma tela
  virtual: `Vazamento!eguro ` (restos) e `vazamento)GAS! (` (sobrescrita) no
  antigo; `Vazamento!      ` e `ALERTA DE GAS! (` no corrigido.

### 5.3 `main`: mensagens não limpavam a linha

* **Antes:** `lcdWrite("Ambiente Seguro ")` dependia de espaços escritos à mão
  no literal para "apagar" o texto anterior — frágil e incompleto.
* **Depois:** cada linha é escrita inteira (16 colunas, com padding) a cada
  ciclo; nada de resto de leitura anterior.

### 5.4 `config_ADC()`: faltava `ADC12ENC` e espera da referência

* **Antes:** `ADC12CTL0 = ADC12SHT0_2 | ADC12ON;` e nada mais; a primeira
  leitura podia ocorrer antes de a referência/ADC estabilizar.
* **Depois:** configuração + `ADC12CTL0 |= ADC12ENC` e
  `__delay_cycles(ADC12_CICLOS_REFERENCIA)` (~5 ms); `ADC12IFG` do canal é
  limpo após ler `ADC12MEM0`. Os registradores (`ADC12CTL0/1/2`, `ADC12MCTL0`,
  `P6SEL`) continuam os mesmos.

### 5.5 `lcdInit()`: "atraso de 1,53 ms" com 20000 ciclos

* **Antes:** comentário dizia 1,53 ms, mas `__delay_cycles(20000)` a 1 MHz são
  **20 ms** (o valor é seguro, o comentário é que estava errado).
* **Depois:** tempos nomeados em `config.h` (`LCD_CICLOS_POWER_ON`,
  `LCD_CICLOS_INIT`, `LCD_CICLOS_LIMPEZA`) com o valor em ms ao lado; a limpeza
  (`0x01`) usa 20 ms, acima do mínimo de 1,52 ms do HD44780.

### 5.6 ISR do botão: `__delay_cycles` de 20 ms dentro da interrupção

* **Antes:** a ISR chamava `__delay_cycles(20000)` — 20 ms de espera ocupada
  **dentro** da interrupção, segurando o processador e atrasando qualquer outra
  interrupção nesse intervalo.
* **Depois:** a ISR só sinaliza o evento (`switch (P2IV)` com
  `case BOTAO_P2IV_VALOR:`) e retorna; o debounce de 20 ms roda no laço
  principal (`botao_pressao_confirmada()`), que confirma se o pino continua em
  nível baixo. A ISR deixou de bloquear o sistema.
* **Ressalva:** `P2IV` devolve o vetor de maior prioridade pendente e é limpo
  pela própria leitura — por isso a ISR lê o registrador **uma única vez** e usa
  o valor no `switch`. O defeito real do original era o atraso bloqueante, não a
  comparação com `==` em si.

### 5.7 `i2cSend()`: ordem START/ACK e NACK frágil

* **Antes:** carregava `UCB0TXBUF` **antes** de o START+endereço terminarem,
  checava `UCNACKIFG` sem limpá-lo e usava `while` sem timeout (barramento
  preso = firmware travado).
* **Depois:** ordem correta (START → endereço → ACK → dado → ACK → STOP),
  `UCNACKIFG` limpo explicitamente, STOP sempre emitido e todas as esperas com
  timeout (`I2C_ERRO_TIMEOUT`). A convenção de retorno foi preservada
  (0 = OK, 1 = NACK, como no original).

### 5.8 `#include <stdio.h>` / `sprintf` não usados

* **Antes:** `#include <stdio.h>` só para um `sprintf` que nunca era chamado.
* **Depois:** removido — menos flash e nenhuma dependência de `printf` no alvo.

### 5.9 `int` para `ADC12MEM0`

* **Antes:** `int nivel_gas = ADC12MEM0;` (16 bits sem sinal na prática).
* **Depois:** `uint16_t` em todo o caminho (`adc_le_valor()`, `app_avalia()`,
  `formata_linha_gas()`), deixando claro o domínio 0..4095.

### 5.10 Extras (observações, sem mudança de comportamento)

* **`I2C_DIVISOR = 105`**: no USCI_B a taxa é `f_SCL = SMCLK / ((UCB0BRW + 1) × 16)`,
  ou seja `1.048.576 / (106 × 16)` ≈ **620 Hz**. O comentário original dizia
  "~10 kHz" (e a documentação derivou "~9,5 kHz") porque as duas contas
  esqueceram o fator **16** interno do periférico. Ainda está dentro da
  especificação I²C (máx. 100 kHz), só que lento — o que não atrapalha um display
  que atualiza duas linhas a cada 2 s. Foi **mantido 105** por fidelidade ao
  código que funcionou em bancada; para acelerar, `UCB0BRW = 0` já dá SMCLK/16
  ≈ 65 kHz. O comentário está em `config.h`.
* **Erro de I2C visível**: `lcd_erro_i2c()` guarda o último erro de
  `i2c_envia_byte()` em vez de descartá-lo por completo. **Ressalva honesta:** a
  função existe e pode ser consultada, mas **nenhum código do firmware a chama
  hoje** — o erro continua não sendo tratado em tempo de execução, e os retornos
  são convertidos em `(void)` dentro do driver. É um gancho para instrumentação,
  não uma correção completa.

---

## 6. O que foi mantido idêntico ao sketch original

Para não alterar o comportamento validado em bancada:

* comparação **estritamente maior** (`nivel > LIMIAR_GAS_ADC`);
* `TA2CCR1 = 500` no alarme e `1000` no repouso/mutado; `TA2CCR0 = 1000-1`;
  `TA2CCTL1 = OUTMOD_7`; `TASSEL_2 | MC_1 | TACLR`;
  com SMCLK de 1 MHz, `TA2CCR0 = 999` é um período de 1000 ticks ⇒ tom de
  **≈ 1 kHz** (os 2 kHz do relatório exigiriam `CCR0 = 499`, o que mudaria o
  comportamento de bancada);
* botão **silencia incondicionalmente** (mesmo com o ambiente seguro) e o mute
  só é desarmado na leitura seguinte com gás baixo;
* endereço do LCD `0x27`; sequência de inicialização em 4 bits (0x30/0x30/0x30/0x20,
  0x28, 0x0C, 0x06, 0x01); nibble alta + baixa com E pulsado;
* `UCB0*` (endereço 0x27, `UCB0BRW = 105`), `ADC12*`, `TA2*`, `P2*`/`P6SEL`;
* laço com `__delay_cycles(2000000)` (~2 s);
* formatação `"Nivel Gas: NNNN"` (15 caracteres, fiel ao original; o
  preenchimento para 16 colunas é feito na escrita).

**Única mudança de comportamento:** o **LED1 (P1.0)** passou a ser dirigido
pelo firmware (o sketch original citava o LED no relatório, mas nunca
configurava `P1.0`). Ele acende acima do limiar, apaga abaixo e **permanece
aceso mesmo com o buzzer silenciado** — o botão corta o som, nunca a indicação
visual de risco. Isso está descrito no §2 e coberto pelo bloco de testes do LED,
nos dois caminhos (C: 128 verificações; Python: 119).

O firmware **não** menciona 2 kHz em lugar nenhum: o tom real é ≈ 1 kHz
(ver a lista acima).

## 7. Limitações / o que **não** foi feito

* **Não foi compilado para o MSP430** e **não foi gravado na placa**: o
  toolchain `msp430-elf-gcc` não existe nesta máquina. Os módulos
  específicos do alvo (`i2c.c`, `lcd.c`, `adc.c`, `buzzer.c`, `botao.c`,
  `main.c`) foram escritos e revisados, mas **não passaram pelo compilador**.
  Somente `app.c` e `texto.c` são compilados e testados aqui (via `-DSIM_HOST`),
  o que cobre a lógica de decisão e o layout da tela — não os drivers.
* O Makefile tem dois ramos de shell; o ramo **`cmd.exe`/`mingw32-make` foi
  testado**, o ramo POSIX (`sh`) segue o padrão `mkdir -p`/`rm -rf` mas não pôde
  ser executado neste sandbox.
* O hardware não foi validado: pinagem real × relatório continua divergente de
  propósito (seção 2), e o mapeamento P2.4 ⇒ TA2.1 deve ser conferido na
  Tabela 4 do datasheet antes de qualquer remontagem.
* Não há tratamento de falha de I2C na interface do usuário (só o registro em
  `lcd_erro_i2c()`), nem calibração/linearização do sensor (o firmware compara
  a contagem crua do ADC, como no original).

## 8. Referências

* TI, *MSP430F552x, MSP430F551x Mixed-Signal Microcontrollers* — SLAS590
  (ADC12, Timer A2, USCI_B0, pinagem). <https://www.ti.com/document-viewer/MSP430F5529/datasheet>
* TI, *MSP-EXP430F5529LP LaunchPad Development Kit* — SLAU533 (botões S1/S2).
* Hitachi HD44780U / PCF8574 (sequência de inicialização em 4 bits, tempos de
  comando).
