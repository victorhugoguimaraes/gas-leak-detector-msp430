/*
 * testes.c - testes do simulador host (gcc/clang -std=c99 -Wall -Wextra).
 * ------------------------------------------------------------------
 * Os testes exercitam a MESMA logica que o firmware executa:
 *   - src/app.c   : decisao (limiar, mute, rearme, mensagens);
 *   - src/texto.c : layout de 16 colunas usado pelo driver do LCD;
 *   - sim/lcd_virtual.c : tela 2x16 que recebe o layout pelo mesmo
 *     emissor que o firmware usa (lcd.c).
 * Nao ha copia da regra de negocio: o simulador chama as funcoes reais
 * do firmware (compiladas com -DSIM_HOST).
 *
 * Casos exigidos: abaixo do limiar, exatamente no limiar, acima, mute
 * enquanto acima, mute rearmado apos o gas cair, formatacao com
 * 0/1/999/1500/4095 e estouro de 16 colunas. Ao final imprime
 * "TODOS OS TESTES PASSARAM" e retorna 0.
 * ==================================================================
 */
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "config.h"
#include "lcd_virtual.h"
#include "texto.h"

static int g_total  = 0;
static int g_falhas = 0;

static void verifica(int condicao, const char *descricao)
{
    g_total++;
    if (condicao) {
        printf("  ok    %s\n", descricao);
    } else {
        g_falhas++;
        printf("  FALHA %s\n", descricao);
    }
}

static void verifica_texto(const char *obtido, const char *esperado, const char *descricao)
{
    g_total++;
    if ((obtido != 0) && (esperado != 0) && (strcmp(obtido, esperado) == 0)) {
        printf("  ok    %s -> |%s|\n", descricao, obtido);
    } else {
        g_falhas++;
        printf("  FALHA %s -> obtido |%s|, esperado |%s|\n",
               descricao, (obtido != 0) ? obtido : "(nulo)",
               (esperado != 0) ? esperado : "(nulo)");
    }
}

static void verifica_u32(unsigned long obtido, unsigned long esperado, const char *descricao)
{
    g_total++;
    if (obtido == esperado) {
        printf("  ok    %s (%lu)\n", descricao, obtido);
    } else {
        g_falhas++;
        printf("  FALHA %s -> obtido %lu, esperado %lu\n", descricao, obtido, esperado);
    }
}

static const char *nome_situacao(app_situacao_t situacao)
{
    switch (situacao) {
        case APP_SEGURO:        return "SEGURO";
        case APP_ALERTA:        return "ALERTA";
        case APP_ALERTA_MUTADO: return "ALERTA_MUTADO";
        default:                return "?";
    }
}

/* ================================================================= */
static void teste_configuracao(void)
{
    printf("\n[1] config.h centraliza pinos, limiares e tempos\n");

    verifica_u32(LIMIAR_GAS_ADC, 1500u, "LIMIAR_GAS_ADC (bancada)");
    verifica_u32(LIMIAR_GAS_TEORICO_MV, 1650u, "LIMIAR_GAS_TEORICO_MV (relatorio)");
    verifica_u32(LIMIAR_GAS_TEORICO_ADC, 2048u, "LIMIAR_GAS_TEORICO_ADC (1,65 V)");

    verifica_u32(LCD_COLUNAS, 16u, "LCD_COLUNAS");
    verifica_u32(LCD_LINHAS, 2u, "LCD_LINHAS");
    verifica_u32(LCD_TAM_LINHA, 17u, "LCD_TAM_LINHA");
    verifica_u32(LCD_I2C_ENDERECO, 0x27u, "LCD_I2C_ENDERECO");

    verifica_u32(PORTA_BUZZER, 2u, "PORTA_BUZZER (implementacao real)");
    verifica_u32(PINO_BUZZER_BIT_NUM, 4u, "PINO_BUZZER_BIT_NUM = P2.4");
    verifica_u32(PORTA_BOTAO, 2u, "PORTA_BOTAO (implementacao real)");
    verifica_u32(PINO_BOTAO_BIT_NUM, 1u, "PINO_BOTAO_BIT_NUM = P2.1");
    verifica_u32(BOTAO_P2IV_VALOR, 0x04u, "BOTAO_P2IV_VALOR (P2.1)");
    verifica_u32(PORTA_ADC, 6u, "PORTA_ADC = P6.0/A0");
    verifica_u32(PORTA_LED, 1u, "PORTA_LED (implementacao real)");
    verifica_u32(PINO_LED_BIT_NUM, 0u, "PINO_LED_BIT_NUM = P1.0");

    verifica_u32(BUZZER_PERIODO_CCR0, 1000u, "BUZZER_PERIODO_CCR0");
    verifica_u32(BUZZER_CCR1_ALARME, 500u, "BUZZER_CCR1_ALARME");
    verifica_u32(BUZZER_CCR1_REPOUSO, 1000u, "BUZZER_CCR1_REPOUSO");
}

/* ================================================================= */
static void teste_limiar(void)
{
    static const struct {
        uint16_t nivel;
        app_situacao_t situacao;
        uint16_t tom;
        const char *mensagem;
    } casos[] = {
        {    0u, APP_SEGURO, 1000u, "Ambiente Seguro" },
        { 1499u, APP_SEGURO, 1000u, "Ambiente Seguro" },
        { 1500u, APP_SEGURO, 1000u, "Ambiente Seguro" },  /* exatamente no limiar */
        { 1501u, APP_ALERTA,  500u, "ALERTA DE GAS!" },   /* acima do limiar      */
        { 2048u, APP_ALERTA,  500u, "ALERTA DE GAS!" },   /* limiar teorico       */
        { 4095u, APP_ALERTA,  500u, "ALERTA DE GAS!" }
    };
    app_estado_t estado;
    size_t i;

    printf("\n[2] Comparacao com o limiar (nivel > %u = alarme)\n", (unsigned)LIMIAR_GAS_ADC);
    app_inicia(&estado);

    for (i = 0u; i < (sizeof(casos) / sizeof(casos[0])); i++) {
        char descricao[96];
        app_decisao_t d = app_avalia(&estado, casos[i].nivel);

        (void)snprintf(descricao, sizeof(descricao), "nivel=%u => situacao %s",
                       (unsigned)casos[i].nivel, nome_situacao(casos[i].situacao));
        verifica(d.situacao == casos[i].situacao, descricao);

        (void)snprintf(descricao, sizeof(descricao), "nivel=%u => tom TA2CCR1 %u",
                       (unsigned)casos[i].nivel, (unsigned)casos[i].tom);
        verifica_u32(d.tom_buzzer, casos[i].tom, descricao);

        (void)snprintf(descricao, sizeof(descricao), "nivel=%u => mensagem \"%s\"",
                       (unsigned)casos[i].nivel, casos[i].mensagem);
        verifica_texto(d.mensagem_lcd, casos[i].mensagem, descricao);

        verifica(estado.mutado == 0u, "estado segue sem mute");
    }
}

/* ================================================================= */
static void teste_mute(void)
{
    app_estado_t estado;

    printf("\n[3] Mute do alarme e rearme automatico\n");
    app_inicia(&estado);

    /* Alarme ativo, sem mute. */
    {
        app_decisao_t d = app_avalia(&estado, 3000u);
        verifica(d.situacao == APP_ALERTA, "gas alto sem mute => ALERTA");
        verifica_u32(d.tom_buzzer, 500u, "gas alto sem mute => buzzer soando (500)");
    }

    /* Pressiona o botao de mute. */
    app_mutar(&estado);
    {
        app_decisao_t d = app_avalia(&estado, 3000u);
        verifica(d.situacao == APP_ALERTA_MUTADO, "mute + gas alto => ALERTA_MUTADO");
        verifica_u32(d.tom_buzzer, 1000u, "mute + gas alto => buzzer silencioso (1000)");
        verifica_texto(d.mensagem_lcd, "ALERTA (MUTADO)", "mute + gas alto => mensagem");
        verifica(d.mutado == 1u, "estado continua mutado");
    }

    /* Mute persiste enquanto o gas continuar alto. */
    {
        app_decisao_t d = app_avalia(&estado, 3000u);
        verifica(d.situacao == APP_ALERTA_MUTADO, "mute persiste na leitura seguinte");
        verifica_u32(d.tom_buzzer, 1000u, "mute persiste => ainda silencioso");
    }

    /* Gas cai => rearme do mute (mesmo no limiar exato). */
    {
        app_decisao_t d = app_avalia(&estado, 1500u);
        verifica(d.situacao == APP_SEGURO, "gas no limiar exato => SEGURO");
        verifica(d.mutado == 0u, "gas caiu => mute REARMADO");
    }

    /* Prova do rearme: novo pico volta a soar. */
    {
        app_decisao_t d = app_avalia(&estado, 2000u);
        verifica(d.situacao == APP_ALERTA, "novo pico apos rearme => ALERTA");
        verifica_u32(d.tom_buzzer, 500u, "novo pico apos rearme => buzzer soando (500)");
    }

    /* Mute com gas baixo (fiel ao original: silencia e cai no else). */
    app_mutar(&estado);
    {
        app_decisao_t d = app_avalia(&estado, 200u);
        verifica(d.situacao == APP_SEGURO, "mute + gas baixo => SEGURO");
        verifica(d.mutado == 0u, "mute + gas baixo => rearmado");
    }
}

/* ================================================================= */
static void teste_formatacao(void)
{
    static const struct {
        uint16_t nivel;
        const char *esperado;
    } casos[] = {
        {    0u, "Nivel Gas: 0000" },
        {    1u, "Nivel Gas: 0001" },
        {  999u, "Nivel Gas: 0999" },
        { 1500u, "Nivel Gas: 1500" },
        { 4095u, "Nivel Gas: 4095" }
    };
    size_t i;

    printf("\n[4] Formatacao da linha 1 (\"Nivel Gas: NNNN\", 15 caracteres)\n");

    for (i = 0u; i < (sizeof(casos) / sizeof(casos[0])); i++) {
        char buffer[LCD_TAM_LINHA];
        char ajustada[LCD_TAM_LINHA];
        char descricao[96];

        memset(buffer, '?', sizeof(buffer));
        formata_linha_gas(casos[i].nivel, buffer);

        (void)snprintf(descricao, sizeof(descricao), "formata_linha_gas(%u)",
                       (unsigned)casos[i].nivel);
        verifica_texto(buffer, casos[i].esperado, descricao);
        verifica_u32((unsigned long)strlen(buffer), 15u, "texto tem 15 caracteres");
        verifica(buffer[15] == '\0', "buffer termina em '\\0' na posicao 15");

        texto_ajusta_linha16(ajustada, buffer);
        verifica_u32((unsigned long)strlen(ajustada), 16u, "linha ajustada tem 16 colunas");
        verifica(ajustada[15] == ' ', "coluna 16 preenchida com espaco");
    }
}

/* ================================================================= */
static void teste_estouro_16_colunas(void)
{
    char ajustada[LCD_TAM_LINHA];
    app_tela_t tela;

    printf("\n[5] Estouro de 16 colunas (quebra, preenchimento e truncamento)\n");

    texto_ajusta_linha16(ajustada, "0123456789ABCDEFGHIJ");
    verifica_texto(ajustada, "0123456789ABCDEF", "linha de 20 chars e truncada em 16");

    texto_ajusta_linha16(ajustada, "abc");
    verifica_texto(ajustada, "abc             ", "linha curta e preenchida com espacos");

    /* 40 caracteres: 16 na linha 1, 16 na linha 2, o resto e descartado. */
    app_tela_limpa(&tela);
    texto_escreve(app_tela_emissor, &tela, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ0123");
    verifica_texto(tela.linha[0], "0123456789ABCDEF", "40 chars: linha 1 completa");
    verifica_texto(tela.linha[1], "GHIJKLMNOPQRSTUV", "40 chars: linha 2 completa");
    verifica_u32((unsigned long)strlen(tela.linha[0]), 16u, "linha 1 com 16 colunas");
    verifica_u32((unsigned long)strlen(tela.linha[1]), 16u, "linha 2 com 16 colunas");

    /* Quebra explicita com '\n'. */
    app_tela_limpa(&tela);
    texto_escreve(app_tela_emissor, &tela, "abc\ndef");
    verifica_texto(tela.linha[0], "abc             ", "\\n: linha 1 preenchida");
    verifica_texto(tela.linha[1], "def             ", "\\n: linha 2 preenchida");

    /* Excedente depois da linha 2 nao volta para o inicio da tela. */
    app_tela_limpa(&tela);
    texto_escreve(app_tela_emissor, &tela, "0123456789ABCDEF\n0123456789ABCDEFGHIJ");
    verifica_texto(tela.linha[0], "0123456789ABCDEF", "excedente: linha 1 intacta");
    verifica_texto(tela.linha[1], "0123456789ABCDEF", "excedente: linha 2 intacta, resto descartado");
}

/* ================================================================= */
static void teste_mensagens_da_tela(void)
{
    app_tela_t tela;

    printf("\n[6] Telas 2x16 montadas com as mensagens do firmware\n");

    app_tela_monta(&tela, "Nivel Gas: 0000", "Ambiente Seguro");
    verifica_texto(tela.linha[0], "Nivel Gas: 0000 ", "seguro: linha 1 preenchida");
    verifica_texto(tela.linha[1], "Ambiente Seguro ", "seguro: linha 2 preenchida");

    app_tela_monta(&tela, "Nivel Gas: 1501", "ALERTA DE GAS!");
    verifica_texto(tela.linha[1], "ALERTA DE GAS!  ", "alarme: mensagem com 16 colunas");

    app_tela_monta(&tela, "Nivel Gas: 4095", "ALERTA (MUTADO)");
    verifica_texto(tela.linha[1], "ALERTA (MUTADO) ", "mutado: mensagem com 16 colunas");
}

/* ================================================================= */
static void teste_bug_lcdwrite_original(void)
{
    lcd_virtual_t lcd;

    printf("\n[7] Bug do lcdWrite() original x correcao (texto.c)\n");

    /* 7.1 restos: texto mais curto nao limpa o que estava antes. */
    lcd_virtual_inicia(&lcd);
    lcd_virtual_comando(&lcd, (uint8_t)LCD_CMD_LINHA2);
    lcd_virtual_escreve_original(&lcd, "Ambiente Seguro");
    lcd_virtual_comando(&lcd, (uint8_t)LCD_CMD_LINHA2);
    lcd_virtual_escreve_original(&lcd, "Vazamento!");
    verifica_texto(lcd.linha[1], "Vazamento!eguro ",
                   "original: restos de texto permanecem na linha");

    /* 7.2 estouro: o 17o caractere reposiciona em 0xC0 e sobrescreve. */
    lcd_virtual_inicia(&lcd);
    lcd_virtual_comando(&lcd, (uint8_t)LCD_CMD_LINHA2);
    lcd_virtual_escreve_original(&lcd, "ALERTA DE GAS! (vazamento)");
    verifica_texto(lcd.linha[1], "vazamento)GAS! (",
                   "original: excedente sobrescreve o inicio da linha");

    /* 7.3 desincronia: `col` local ignora a coluna real do cursor. */
    lcd_virtual_inicia(&lcd);
    lcd_virtual_comando(&lcd, (uint8_t)LCD_CMD_LINHA2);
    lcd_virtual_caractere(&lcd, '#');
    lcd_virtual_escreve_original(&lcd, "0123456789ABCDEFG");
    verifica_texto(lcd.linha[1], "G0123456789ABCDE",
                   "original: quebra na coluna errada (col local != cursor)");

    /* 7.4 correcao: preenchimento com espacos elimina os restos. */
    lcd_virtual_inicia(&lcd);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "Nivel Gas: 1500", 0u);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "Ambiente Seguro", 1u);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "Nivel Gas: 0100", 0u);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "Vazamento!", 1u);
    verifica_texto(lcd.linha[0], "Nivel Gas: 0100 ", "corrigido: linha 1 reescrita por inteiro");
    verifica_texto(lcd.linha[1], "Vazamento!      ", "corrigido: sem restos de texto");

    /* 7.5 correcao: excedente e truncado em vez de sobrescrever. */
    lcd_virtual_inicia(&lcd);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "Nivel Gas: 1501", 0u);
    texto_escreve_em(lcd_virtual_emissor, &lcd, "ALERTA DE GAS! (vazamento)", 1u);
    verifica_texto(lcd.linha[0], "Nivel Gas: 1501 ", "corrigido: linha 1 preservada");
    verifica_texto(lcd.linha[1], "ALERTA DE GAS! (", "corrigido: excedente truncado");
}

/* ================================================================= */
static void teste_equivalencia_firmware_simulador(void)
{
    static const struct {
        uint16_t nivel;
        const char *mensagem;
    } passos[] = {
        {    0u, "Ambiente Seguro" },
        { 1501u, "ALERTA DE GAS!" },
        { 4095u, "ALERTA (MUTADO)" }
    };
    size_t i;

    printf("\n[8] Mesma logica: tela montada x sequencia de escrita do driver\n");

    for (i = 0u; i < (sizeof(passos) / sizeof(passos[0])); i++) {
        char linha1[LCD_TAM_LINHA];
        app_tela_t tela;
        lcd_virtual_t lcd;
        char descricao[96];

        formata_linha_gas(passos[i].nivel, linha1);

        /* (a) tela montada em memoria */
        app_tela_monta(&tela, linha1, passos[i].mensagem);

        /* (b) exatamente as duas chamadas que main.c faz via lcd.c */
        lcd_virtual_inicia(&lcd);
        texto_escreve_em(lcd_virtual_emissor, &lcd, linha1, 0u);
        texto_escreve_em(lcd_virtual_emissor, &lcd, passos[i].mensagem, 1u);

        (void)snprintf(descricao, sizeof(descricao),
                       "nivel=%u: linha 1 igual nos dois caminhos", (unsigned)passos[i].nivel);
        verifica_texto(lcd.linha[0], tela.linha[0], descricao);
        (void)snprintf(descricao, sizeof(descricao),
                       "nivel=%u: linha 2 igual nos dois caminhos", (unsigned)passos[i].nivel);
        verifica_texto(lcd.linha[1], tela.linha[1], descricao);
    }
}

/* ================================================================= */
static void teste_simulacao_completa(void)
{
    static const struct {
        uint16_t nivel;
        uint8_t mutar;     /* 1 = botao pressionado antes da leitura */
    } roteiro[] = {
        {  100u, 0u },
        { 1499u, 0u },
        { 1500u, 0u },
        { 1501u, 0u },
        { 3500u, 0u },
        { 3500u, 1u },
        { 3000u, 0u },
        { 1500u, 0u },
        { 3800u, 0u },
        {  250u, 0u }
    };
    app_estado_t estado;
    size_t i;

    printf("\n[9] Simulacao do laco principal (10 leituras)\n");
    printf("    nivel  tom   situacao       |linha 1         |linha 2         |\n");

    app_inicia(&estado);

    for (i = 0u; i < (sizeof(roteiro) / sizeof(roteiro[0])); i++) {
        char linha1[LCD_TAM_LINHA];
        app_tela_t tela;
        app_decisao_t d;

        if (roteiro[i].mutar != 0u) {
            app_mutar(&estado);
        }
        d = app_avalia(&estado, roteiro[i].nivel);
        formata_linha_gas(roteiro[i].nivel, linha1);
        app_tela_monta(&tela, linha1, d.mensagem_lcd);

        printf("    %5u  %4u  %-13s |%s|%s|\n",
               (unsigned)roteiro[i].nivel, (unsigned)d.tom_buzzer,
               nome_situacao(d.situacao), tela.linha[0], tela.linha[1]);

        /* Invariantes da regra de decisao. */
        if (d.situacao == APP_ALERTA) {
            verifica_u32(d.tom_buzzer, 500u, "invariante: alarme ativo => tom 500");
        } else {
            verifica_u32(d.tom_buzzer, 1000u, "invariante: sem alarme ativo => tom 1000");
        }
    }

    verifica(estado.mutado == 0u, "invariante: leitura baixa no fim do roteiro rearma o mute");
}

/* =================================================================
 * LED1 (P1.0) - comportamento de seguranca
 * -----------------------------------------------------------------
 * O LED deve permanecer ACESO enquanto o nivel estiver acima do limiar,
 * inclusive quando o buzzer esta silenciado: o botao corta o som, nunca
 * a indicacao visual de risco (relatorio, secao 3.4.7).
 * ================================================================= */
static void teste_led(void)
{
    app_estado_t estado;
    app_decisao_t d;

    app_inicia(&estado);

    printf("\n-- LED1 (P1.0) --\n");

    d = app_avalia(&estado, 0u);
    verifica_u32(d.led_alarme, 0u, "seguro (0) => LED apagado");

    d = app_avalia(&estado, 1500u);
    verifica_u32(d.led_alarme, 0u, "exatamente no limiar => LED apagado");

    d = app_avalia(&estado, 1501u);
    verifica_u32(d.led_alarme, 1u, "acima do limiar => LED aceso");

    d = app_avalia(&estado, 4095u);
    verifica_u32(d.led_alarme, 1u, "fundo de escala => LED aceso");

    /* o ponto critico: silenciar o som NAO apaga o LED */
    app_mutar(&estado);
    d = app_avalia(&estado, 3000u);
    verifica_u32(d.situacao, APP_ALERTA_MUTADO, "mutado + gas alto => situacao mutada");
    verifica_u32(d.tom_buzzer, 1000u, "mutado + gas alto => buzzer silencioso");
    verifica_u32(d.led_alarme, 1u, "mutado + gas alto => LED CONTINUA aceso");

    /* e volta a apagar quando o gas cai */
    d = app_avalia(&estado, 400u);
    verifica_u32(d.led_alarme, 0u, "gas cai => LED apaga");
    verifica_u32(estado.mutado, 0u, "gas cai => mute rearmado");

    printf("    (LED aceso = P1OUT |= BIT0; apagado = P1OUT &= ~BIT0)\n");
}

/* ================================================================= */
int main(void)
{
    printf("=================================================================\n");
    printf(" Testes do firmware - Detector de Vazamento de Gas (host)\n");
    printf(" Alvo real: MSP430F5529 | limiar=%u | LCD %dx%d\n",
           (unsigned)LIMIAR_GAS_ADC, (unsigned)LCD_LINHAS, (unsigned)LCD_COLUNAS);
    printf("=================================================================\n");

    teste_configuracao();
    teste_limiar();
    teste_mute();
    teste_led();
    teste_formatacao();
    teste_estouro_16_colunas();
    teste_mensagens_da_tela();
    teste_bug_lcdwrite_original();
    teste_equivalencia_firmware_simulador();
    teste_simulacao_completa();

    printf("\n-----------------------------------------------------------------\n");
    printf("Verificacoes: %d | falhas: %d\n", g_total, g_falhas);
    if (g_falhas == 0) {
        printf("TODOS OS TESTES PASSARAM\n");
        return 0;
    }
    printf("RESULTADO: HOUVE FALHA\n");
    return 1;
}
