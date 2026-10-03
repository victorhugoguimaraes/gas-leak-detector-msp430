/*
 * config.h - Detector de Vazamento de Gas (MSP430F5529)
 * ------------------------------------------------------------------
 * Ponto UNICO de configuracao do firmware: todos os pinos, limiares,
 * enderecos e tempos ficam aqui. Nenhum outro arquivo deve conter
 * numero magico de hardware.
 *
 * Compilacao dupla:
 *   - Firmware MSP430 : sem simbolo extra; este header inclui <msp430.h>
 *                       e expoe tambem os nomes de registradores.
 *   - Host (simulador/testes): compilar com -DSIM_HOST. Nesse modo o
 *                     bloco de registradores e omitido (nao existe
 *                     <msp430.h> no PC) e valem apenas as constantes
 *                     numericas, que sao as mesmas usadas no alvo.
 * ------------------------------------------------------------------
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#ifndef SIM_HOST
#include <msp430.h>
#endif

/* ==================================================================
 * 1. CLOCK
 * ------------------------------------------------------------------
 * O firmware NAO configura o clock: nao ha nenhuma escrita em UCSCTL*.
 * Ele roda com o DCO/calibracao de fabrica do MSP430F5529, que fica em
 * torno de 1,05 MHz. Adotamos 1048576 Hz (2^20) como valor de projeto:
 * e um dos taps de calibracao do DCO e fica a ~2,4% do "1 MHz" nominal
 * citado no relatorio.
 *
 * ATENCAO: os comentarios de outros arquivos e a documentacao falam em
 * "1 MHz" como ARREDONDAMENTO nominal. Todas as contas que importam
 * (tempo do laco, debounce, tom do buzzer, divisor do I2C) mudam ~2,4%
 * se voce usar o valor exato. Os numeros teoricos do relatorio
 * (CCR0 = 499 para 2 kHz, CCR0 = 12499 para 100 ms) assumem exatamente
 * 1 MHz, porque foram calculados ANTES da montagem.
 * ================================================================== */
#define FREQ_DCO_HZ              1048576UL  /* ~1,05 MHz (DCO de fabrica, 2^20) */
#define CICLOS_POR_MS            (FREQ_DCO_HZ / 1000UL)

/* ==================================================================
 * 2. PINAGEM (numero da porta e bit) - IMPLEMENTACAO REAL
 * ------------------------------------------------------------------
 * ATENCAO: estes pinos sao os do codigo original que funciona na
 * bancada. O relatorio do projeto cita outra pinagem
 * (LED1 P1.0 / buzzer P2.0-TA1.1 / botao S2 P1.1); a divergencia
 * esta documentada no README.md e NAO foi "corrigida" aqui, para
 * nao quebrar o hardware ja montado.
 * ================================================================== */
#define PORTA_ADC                6      /* P6.0 = A0 (sensor MQ-x)      */
#define PINO_ADC_BIT_NUM         0
#define PORTA_BUZZER             2      /* P2.4 = saida do Timer A2     */
#define PINO_BUZZER_BIT_NUM      4
#define PORTA_BOTAO              2      /* P2.1 = botao de mute         */
#define PINO_BOTAO_BIT_NUM       1
#define BOTAO_P2IV_VALOR         0x04   /* P2IV quando a interrupcao e P2.1 */
#define PORTA_LED                1      /* P1.0 = LED1 da LaunchPad      */
#define PINO_LED_BIT_NUM         0
#define PORTA_I2C                3      /* P3.0 = UCB0SDA, P3.1 = UCB0SCL */
#define PINO_I2C_SDA_BIT_NUM     0
#define PINO_I2C_SCL_BIT_NUM     1

/* ==================================================================
 * 3. LIMIAR DE GAS
 * ------------------------------------------------------------------
 * O relatorio descreve um limiar TEORICO de 1,65 V, que com VCC = 3,3 V
 * e ADC de 12 bits equivale a 1,65/3,3 * 4095 ~= 2048 contagens.
 * O valor usado e validado em bancada com o sensor real e 1500, que e
 * o que o firmware emprega. Os dois valores ficam registrados aqui para
 * que a divergencia seja explicita e rastreavel.
 * A comparacao e ESTRITAMENTE MAIOR (nivel > LIMIAR_GAS_ADC), igual ao
 * codigo original: exatamente 1500 ainda e considerado seguro.
 * ================================================================== */
#define LIMIAR_GAS_ADC           1500u  /* limiar pratico de bancada   */
#define LIMIAR_GAS_TEORICO_MV    1650u  /* 1,65 V citado no relatorio  */
#define LIMIAR_GAS_TEORICO_ADC   2048u  /* 1,65 V / 3,3 V * 4096       */

/* ==================================================================
 * 4. ADC12 (sensor de gas)
 * ================================================================== */
#define ADC12_CANAL_CFG          ADC12INCH_0    /* A0                     */
#define ADC12_REFERENCIA_CFG     ADC12SREF_0    /* VR+ = AVCC, VR- = AVSS */
#define ADC12_RESOLUCAO_CFG      ADC12RES_2     /* 12 bits                */
#define ADC12_TEMPO_AMOSTRAGEM   ADC12SHT0_2    /* 16 ciclos de ADC12CLK  */
#define ADC12_MAX_CONTAGENS      4095u          /* 2^12 - 1               */
#define ADC12_CICLOS_REFERENCIA  5000UL         /* ~5 ms p/ referencia    */

/* ==================================================================
 * 5. LCD 16x2 via PCF8574 (I2C)
 * ================================================================== */
#define LCD_I2C_ENDERECO         0x27   /* pode ser 0x3F em outro modulo */
#define LCD_COLUNAS              16
#define LCD_LINHAS               2
#define LCD_TAM_LINHA            (LCD_COLUNAS + 1)  /* + '\0' */
#define LCD_BIT_BACKLIGHT        0x08   /* P7 do PCF8574 */
#define LCD_BIT_ENABLE           0x04   /* E             */
#define LCD_BIT_RS               0x01   /* RS: 1 = dado, 0 = comando */
#define LCD_CMD_LINHA1           0x80
#define LCD_CMD_LINHA2           0xC0
#define LCD_CMD_FUNCTION_SET     0x28   /* 4 bits, 2 linhas, 5x8 */
#define LCD_CMD_DISPLAY_ON       0x0C   /* display on, cursor off */
#define LCD_CMD_ENTRY_MODE       0x06   /* incrementa, sem shift  */
#define LCD_CMD_CLEAR            0x01

/* Tempos do LCD em ciclos de 1 MHz (__delay_cycles).
 * O comentario antigo dizia "atraso obrigatorio de 1,53 ms" e usava
 * 20000 ciclos = 20 ms; 20 ms e um valor seguro para o comando de
 * limpeza (datasheet pede > 1,52 ms), mas o numero nao era 1,53 ms. */
#define LCD_CICLOS_POWER_ON      50000UL  /* ~50 ms apos energizar     */
#define LCD_CICLOS_INIT          10000UL  /* ~10 ms entre comandos 0x30 */
#define LCD_CICLOS_LIMPEZA       20000UL  /* ~20 ms apos 0x01 (> 1,52 ms) */

/* ==================================================================
 * 6. BUZZER (Timer A2, canal TA2.1 -> registradores TA2CCTL1/TA2CCR1)
 * ------------------------------------------------------------------
 * BUG CORRIGIDO: o comentario original dizia "P2.5 controlado pelo
 * Timer A2 (TA2.2)", mas o codigo configura P2.4 (BIT4) e usa o canal
 * 1 do Timer A2 (TA2CCTL1/TA2CCR1). O comentario agora descreve o que
 * o codigo realmente faz.
 * ================================================================== */
#define BUZZER_PERIODO_CCR0      1000u  /* TA2CCR0 = periodo - 1          */
#define BUZZER_CCR1_REPOUSO      1000u  /* >= periodo => saida sempre alta (sem som) */
#define BUZZER_CCR1_ALARME       500u   /* 50% de duty => tom audivel      */

/* ==================================================================
 * 7. BOTAO / MUTE
 * ================================================================== */
#define BOTAO_DEBOUNCE_MS        20u
#define BOTAO_CICLOS_DEBOUNCE    (BOTAO_DEBOUNCE_MS * CICLOS_POR_MS)

/* ==================================================================
 * 8. TEMPO DO LACO PRINCIPAL
 * ================================================================== */
#define APP_CICLOS_ENTRE_LEITURAS 2000000UL  /* ~2 s @ 1 MHz (original) */

/* ==================================================================
 * 9. MAPA DOS REGISTRADORES (somente no alvo MSP430)
 * ================================================================== */
#ifndef SIM_HOST

/* --- I2C / UCB0 (P3.0 = SDA, P3.1 = SCL) --- */
#define I2C_SEL_REG              P3SEL
#define I2C_REN_REG              P3REN
#define I2C_OUT_REG              P3OUT
#define I2C_SEL_BITS             (BIT0 | BIT1)
#define I2C_CFG_CTL0             (UCMST | UCMODE_3 | UCSYNC)
#define I2C_CFG_CTL1             (UCSSEL_2 | UCSWRST)
/* Divisor do barramento I2C (UCB0BRW = UCB0BR1:UCB0BR0).
 *
 * No USCI_B do MSP430 a taxa em modo mestre e:
 *     f_SCL = SMCLK / ((UCB0BRW + 1) * 16)
 *     618 Hz = 1.048.576 / ((105 + 1) * 16)
 *
 * O comentario do codigo original dizia "~10 kHz", e a documentacao
 * derivou "~9,5 kHz" disso — as duas contas estao erradas, porque
 * esqueceram o fator 16 interno do periferico. O valor real e ~620 Hz.
 *
 * Mesmo ~620 Hz esta dentro da especificacao I2C (maximo 100 kHz), entao
 * o LCD funciona — apenas lento, o que nao atrapalha um display que
 * atualiza duas linhas a cada 2 s.
 *
 * Para chegar perto dos 100 kHz nominais com este clock o fator 16 ja
 * consome tudo: o maximo seria SMCLK/16 = 65,5 kHz com UCB0BRW = 0.
 * Ou seja, 100 kHz exatos exigiriam subir o SMCLK antes.
 *
 * O valor 105 foi MANTIDO por fidelidade ao prototipo que rodou na bancada.
 */
#define I2C_DIVISOR              105
#define I2C_TIMEOUT_CICLOS       20000u /* trava de seguranca contra barramento preso */

/* --- ADC12 (P6.0 = A0) --- */
#define ADC_SEL_REG              P6SEL
#define ADC_SEL_BIT              BIT0
#define ADC_BIT_CANAL            BIT0   /* ADC12IFG / ADC12MEM0 */

/* --- Buzzer (P2.4) --- */
#define BUZZER_DIR_REG           P2DIR
#define BUZZER_SEL_REG           P2SEL
#define BUZZER_PIN_BIT           BIT4
#define BUZZER_TA_CFG            (TASSEL_2 | MC_1 | TACLR)

/* --- Botao (P2.1) --- */
#define BOTAO_DIR_REG            P2DIR
#define BOTAO_REN_REG            P2REN
#define BOTAO_OUT_REG            P2OUT
#define BOTAO_IN_REG             P2IN
#define BOTAO_IES_REG            P2IES
#define BOTAO_IFG_REG            P2IFG
#define BOTAO_IE_REG             P2IE
#define BOTAO_PIN_BIT            BIT1

/* --- LED1 (P1.0) ---
 * Adicionado ao firmware organizado: o sketch original citava o LED1 no
 * relatorio mas nunca configurava P1.0. O comportamento escolhido segue
 * o que o relatorio descreve no secao 3.5.1 - o LED permanece ACESO
 * enquanto o nivel estiver acima do limiar, inclusive quando o buzzer
 * esta silenciado. Como o LED1 da LaunchPad acende com nivel ALTO em
 * P1.0, o estado do alarme mapeia direto para o pino. */
#define LED_DIR_REG              P1DIR
#define LED_OUT_REG              P1OUT
#define LED_PIN_BIT              BIT0

#endif /* SIM_HOST */

#endif /* CONFIG_H */
