#!/usr/bin/env python3
"""
testes.py - mesma bateria de testes do sim/testes.c, em Python puro.
------------------------------------------------------------------
Fallback para maquinas SEM compilador C. Roda com:
    <python do harness> sim/testes.py
Imprime "TODOS OS TESTES PASSARAM" e retorna 0 quando passa.
------------------------------------------------------------------
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import modelo as m


class Relatorio:
    def __init__(self):
        self.total = 0
        self.falhas = 0

    def ok(self, condicao, descricao):
        self.total += 1
        if condicao:
            print("  ok    %s" % descricao)
        else:
            self.falhas += 1
            print("  FALHA %s" % descricao)

    def texto(self, obtido, esperado, descricao):
        self.total += 1
        if obtido == esperado:
            print("  ok    %s -> |%s|" % (descricao, obtido))
        else:
            self.falhas += 1
            print("  FALHA %s -> obtido |%s|, esperado |%s|" % (descricao, obtido, esperado))

    def num(self, obtido, esperado, descricao):
        self.total += 1
        if obtido == esperado:
            print("  ok    %s (%s)" % (descricao, obtido))
        else:
            self.falhas += 1
            print("  FALHA %s -> obtido %s, esperado %s" % (descricao, obtido, esperado))


r = Relatorio()


def teste_configuracao():
    print("\n[1] configuracao (espelho de config.h)")
    r.num(m.LIMIAR_GAS_ADC, 1500, "LIMIAR_GAS_ADC (bancada)")
    r.num(m.LIMIAR_GAS_TEORICO_MV, 1650, "LIMIAR_GAS_TEORICO_MV (relatorio)")
    r.num(m.LIMIAR_GAS_TEORICO_ADC, 2048, "LIMIAR_GAS_TEORICO_ADC (1,65 V)")
    r.num(m.LCD_COLUNAS, 16, "LCD_COLUNAS")
    r.num(m.LCD_LINHAS, 2, "LCD_LINHAS")
    r.num(m.LCD_I2C_ENDERECO, 0x27, "LCD_I2C_ENDERECO")
    r.num(m.PORTA_BUZZER, 2, "PORTA_BUZZER (implementacao real)")
    r.num(m.PINO_BUZZER_BIT_NUM, 4, "PINO_BUZZER_BIT_NUM = P2.4")
    r.num(m.PORTA_BOTAO, 2, "PORTA_BOTAO (implementacao real)")
    r.num(m.PINO_BOTAO_BIT_NUM, 1, "PINO_BOTAO_BIT_NUM = P2.1")
    r.num(m.BOTAO_P2IV_VALOR, 0x04, "BOTAO_P2IV_VALOR (P2.1)")
    r.num(m.PORTA_ADC, 6, "PORTA_ADC = P6.0/A0")
    r.num(m.PORTA_LED, 1, "PORTA_LED (implementacao real)")
    r.num(m.PINO_LED_BIT_NUM, 0, "PINO_LED_BIT_NUM = P1.0")
    r.num(m.BUZZER_CCR1_ALARME, 500, "BUZZER_CCR1_ALARME")
    r.num(m.BUZZER_CCR1_REPOUSO, 1000, "BUZZER_CCR1_REPOUSO")


def teste_limiar():
    print("\n[2] Comparacao com o limiar (nivel > %d = alarme)" % m.LIMIAR_GAS_ADC)
    casos = [
        (0, m.SEGURO, 1000, "Ambiente Seguro"),
        (1499, m.SEGURO, 1000, "Ambiente Seguro"),
        (1500, m.SEGURO, 1000, "Ambiente Seguro"),   # exatamente no limiar
        (1501, m.ALERTA, 500, "ALERTA DE GAS!"),
        (2048, m.ALERTA, 500, "ALERTA DE GAS!"),     # limiar teorico
        (4095, m.ALERTA, 500, "ALERTA DE GAS!"),
    ]
    estado = m.Estado()
    for nivel, situacao, tom, mensagem in casos:
        d = m.avalia(estado, nivel)
        r.ok(d.situacao == situacao, "nivel=%d => situacao %s" % (nivel, situacao))
        r.num(d.tom_buzzer, tom, "nivel=%d => tom TA2CCR1" % nivel)
        r.texto(d.mensagem_lcd, mensagem, "nivel=%d => mensagem" % nivel)
        r.ok(estado.mutado == 0, "estado segue sem mute")


def teste_mute():
    print("\n[3] Mute do alarme e rearme automatico")
    estado = m.Estado()
    d = m.avalia(estado, 3000)
    r.ok(d.situacao == m.ALERTA, "gas alto sem mute => ALERTA")
    r.num(d.tom_buzzer, 500, "gas alto sem mute => buzzer soando (500)")

    m.mutar(estado)
    d = m.avalia(estado, 3000)
    r.ok(d.situacao == m.ALERTA_MUTADO, "mute + gas alto => ALERTA_MUTADO")
    r.num(d.tom_buzzer, 1000, "mute + gas alto => buzzer silencioso (1000)")
    r.texto(d.mensagem_lcd, "ALERTA (MUTADO)", "mute + gas alto => mensagem")
    r.ok(d.mutado == 1, "estado continua mutado")

    d = m.avalia(estado, 3000)
    r.ok(d.situacao == m.ALERTA_MUTADO, "mute persiste na leitura seguinte")
    r.num(d.tom_buzzer, 1000, "mute persiste => ainda silencioso")

    d = m.avalia(estado, 1500)
    r.ok(d.situacao == m.SEGURO, "gas no limiar exato => SEGURO")
    r.ok(d.mutado == 0, "gas caiu => mute REARMADO")

    d = m.avalia(estado, 2000)
    r.ok(d.situacao == m.ALERTA, "novo pico apos rearme => ALERTA")
    r.num(d.tom_buzzer, 500, "novo pico apos rearme => buzzer soando (500)")

    m.mutar(estado)
    d = m.avalia(estado, 200)
    r.ok(d.situacao == m.SEGURO, "mute + gas baixo => SEGURO")
    r.ok(d.mutado == 0, "mute + gas baixo => rearmado")


def teste_formatacao():
    print("\n[4] Formatacao da linha 1 (15 caracteres + preenchimento)")
    casos = [
        (0, "Nivel Gas: 0000"),
        (1, "Nivel Gas: 0001"),
        (999, "Nivel Gas: 0999"),
        (1500, "Nivel Gas: 1500"),
        (4095, "Nivel Gas: 4095"),
    ]
    for nivel, esperado in casos:
        obtido = m.formata_linha_gas(nivel)
        r.texto(obtido, esperado, "formata_linha_gas(%d)" % nivel)
        r.num(len(obtido), 15, "texto tem 15 caracteres")
        r.num(len(m.ajusta_linha16(obtido)), 16, "linha ajustada tem 16 colunas")
        r.ok(m.ajusta_linha16(obtido)[15] == " ", "coluna 16 preenchida com espaco")


def teste_estouro():
    print("\n[5] Estouro de 16 colunas")
    r.texto(m.ajusta_linha16("0123456789ABCDEFGHIJ"), "0123456789ABCDEF",
            "linha de 20 chars e truncada em 16")
    r.texto(m.ajusta_linha16("abc"), "abc             ",
            "linha curta e preenchida com espacos")

    tela = m.Tela()
    m.escreve(tela.emissor, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ0123")
    r.texto(tela.texto(0), "0123456789ABCDEF", "40 chars: linha 1 completa")
    r.texto(tela.texto(1), "GHIJKLMNOPQRSTUV", "40 chars: linha 2 completa")

    tela = m.Tela()
    m.escreve(tela.emissor, "abc\ndef")
    r.texto(tela.texto(0), "abc             ", "\\n: linha 1 preenchida")
    r.texto(tela.texto(1), "def             ", "\\n: linha 2 preenchida")

    tela = m.Tela()
    m.escreve(tela.emissor, "0123456789ABCDEF\n0123456789ABCDEFGHIJ")
    r.texto(tela.texto(0), "0123456789ABCDEF", "excedente: linha 1 intacta")
    r.texto(tela.texto(1), "0123456789ABCDEF", "excedente: linha 2 intacta")


def teste_mensagens():
    print("\n[6] Telas 2x16 com as mensagens do firmware")
    l1, l2 = m.tela_monta("Nivel Gas: 0000", "Ambiente Seguro")
    r.texto(l1, "Nivel Gas: 0000 ", "seguro: linha 1 preenchida")
    r.texto(l2, "Ambiente Seguro ", "seguro: linha 2 preenchida")

    _, l2 = m.tela_monta("Nivel Gas: 1501", "ALERTA DE GAS!")
    r.texto(l2, "ALERTA DE GAS!  ", "alarme: mensagem com 16 colunas")

    _, l2 = m.tela_monta("Nivel Gas: 4095", "ALERTA (MUTADO)")
    r.texto(l2, "ALERTA (MUTADO) ", "mutado: mensagem com 16 colunas")


def teste_bug_original():
    print("\n[7] Bug do lcdWrite() original x correcao")
    lcd = m.LcdVirtual()
    lcd.comando(m.LCD_CMD_LINHA2)
    lcd.escreve_original("Ambiente Seguro")
    lcd.comando(m.LCD_CMD_LINHA2)
    lcd.escreve_original("Vazamento!")
    r.texto(lcd.texto(1), "Vazamento!eguro ",
            "original: restos de texto permanecem na linha")

    lcd = m.LcdVirtual()
    lcd.comando(m.LCD_CMD_LINHA2)
    lcd.escreve_original("ALERTA DE GAS! (vazamento)")
    r.texto(lcd.texto(1), "vazamento)GAS! (",
            "original: excedente sobrescreve o inicio da linha")

    lcd = m.LcdVirtual()
    lcd.comando(m.LCD_CMD_LINHA2)
    lcd.caractere(ord("#"))
    lcd.escreve_original("0123456789ABCDEFG")
    r.texto(lcd.texto(1), "G0123456789ABCDE",
            "original: quebra na coluna errada (col local != cursor)")

    lcd = m.LcdVirtual()
    m.escreve_em(lcd.emissor, "Nivel Gas: 1500", 0)
    m.escreve_em(lcd.emissor, "Ambiente Seguro", 1)
    m.escreve_em(lcd.emissor, "Nivel Gas: 0100", 0)
    m.escreve_em(lcd.emissor, "Vazamento!", 1)
    r.texto(lcd.texto(0), "Nivel Gas: 0100 ", "corrigido: linha 1 reescrita por inteiro")
    r.texto(lcd.texto(1), "Vazamento!      ", "corrigido: sem restos de texto")

    lcd = m.LcdVirtual()
    m.escreve_em(lcd.emissor, "Nivel Gas: 1501", 0)
    m.escreve_em(lcd.emissor, "ALERTA DE GAS! (vazamento)", 1)
    r.texto(lcd.texto(0), "Nivel Gas: 1501 ", "corrigido: linha 1 preservada")
    r.texto(lcd.texto(1), "ALERTA DE GAS! (", "corrigido: excedente truncado")


def teste_equivalencia():
    print("\n[8] Mesma logica: tela montada x sequencia de escrita do driver")
    for nivel, mensagem in [(0, "Ambiente Seguro"), (1501, "ALERTA DE GAS!"),
                            (4095, "ALERTA (MUTADO)")]:
        linha1 = m.formata_linha_gas(nivel)
        t1, t2 = m.tela_monta(linha1, mensagem)
        lcd = m.LcdVirtual()
        m.escreve_em(lcd.emissor, linha1, 0)
        m.escreve_em(lcd.emissor, mensagem, 1)
        r.texto(lcd.texto(0), t1, "nivel=%d: linha 1 igual nos dois caminhos" % nivel)
        r.texto(lcd.texto(1), t2, "nivel=%d: linha 2 igual nos dois caminhos" % nivel)


def teste_simulacao():
    print("\n[9] Simulacao do laco principal (10 leituras)")
    roteiro = [
        (100, 0), (1499, 0), (1500, 0), (1501, 0), (3500, 0),
        (3500, 1), (3000, 0), (1500, 0), (3800, 0), (250, 0),
    ]
    print("    nivel  tom   situacao       |linha 1         |linha 2         |")
    estado = m.Estado()
    for nivel, mutar in roteiro:
        if mutar:
            m.mutar(estado)
        d = m.avalia(estado, nivel)
        l1, l2 = m.tela_monta(m.formata_linha_gas(nivel), d.mensagem_lcd)
        print("    %5d  %4d  %-13s |%s|%s|" % (nivel, d.tom_buzzer, d.situacao, l1, l2))
        if d.situacao == m.ALERTA:
            r.num(d.tom_buzzer, 500, "invariante: alarme ativo => tom 500")
        else:
            r.num(d.tom_buzzer, 1000, "invariante: sem alarme ativo => tom 1000")
    r.ok(estado.mutado == 0, "invariante: leitura baixa no fim do roteiro rearma o mute")


def teste_led():
    """LED1 (P1.0): permanece aceso enquanto houver risco, inclusive com o
    buzzer silenciado - o botao corta o SOM, nunca a indicacao visual."""
    print("\n-- LED1 (P1.0) --")
    estado = m.Estado()

    d = m.avalia(estado, 0)
    r.num(d.led_alarme, 0, "seguro (0) => LED apagado")

    d = m.avalia(estado, 1500)
    r.num(d.led_alarme, 0, "exatamente no limiar => LED apagado")

    d = m.avalia(estado, 1501)
    r.num(d.led_alarme, 1, "acima do limiar => LED aceso")

    d = m.avalia(estado, 4095)
    r.num(d.led_alarme, 1, "fundo de escala => LED aceso")

    m.mutar(estado)
    d = m.avalia(estado, 3000)
    r.ok(d.situacao == m.ALERTA_MUTADO, "mutado + gas alto => situacao mutada")
    r.num(d.tom_buzzer, 1000, "mutado + gas alto => buzzer silencioso")
    r.num(d.led_alarme, 1, "mutado + gas alto => LED CONTINUA aceso")

    d = m.avalia(estado, 400)
    r.num(d.led_alarme, 0, "gas cai => LED apaga")
    r.num(estado.mutado, 0, "gas cai => mute rearmado")


def main():
    print("=================================================================")
    print(" Testes do firmware - Detector de Vazamento de Gas (Python)")
    print(" Alvo real: MSP430F5529 | limiar=%d | LCD %dx%d"
          % (m.LIMIAR_GAS_ADC, m.LCD_LINHAS, m.LCD_COLUNAS))
    print("=================================================================")

    teste_configuracao()
    teste_limiar()
    teste_mute()
    teste_led()
    teste_formatacao()
    teste_estouro()
    teste_mensagens()
    teste_bug_original()
    teste_equivalencia()
    teste_simulacao()

    print("\n-----------------------------------------------------------------")
    print("Verificacoes: %d | falhas: %d" % (r.total, r.falhas))
    if r.falhas == 0:
        print("TODOS OS TESTES PASSARAM")
        return 0
    print("RESULTADO: HOUVE FALHA")
    return 1


if __name__ == "__main__":
    sys.exit(main())
