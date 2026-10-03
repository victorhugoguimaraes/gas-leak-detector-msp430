#!/usr/bin/env python3
"""
simulador.py - demonstracao do detector de gas no PC (Python puro).
------------------------------------------------------------------
Equivale a sim/simulador.c: roda o laco principal com leituras
simuladas e mostra a tela 2x16, o tom do buzzer (TA2CCR1) e a decisao.
Fallback para maquinas sem compilador C.
------------------------------------------------------------------
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import modelo as m

ROTEIRO = [
    (120, 0),
    (900, 0),
    (1500, 0),   # exatamente no limiar => seguro
    (1501, 0),   # primeiro nivel de alerta
    (3200, 0),
    (3200, 1),   # botao de mute pressionado
    (2900, 0),
    (1500, 0),   # gas cai: mute rearmado
    (3400, 0),   # novo pico volta a soar
    (250, 0),
]


def main():
    print("=================================================================")
    print(" Simulador host (Python) - Detector de Vazamento de Gas")
    print(" limiar = %d contagens (ADC 12 bits) | LCD %dx%d"
          % (m.LIMIAR_GAS_ADC, m.LCD_LINHAS, m.LCD_COLUNAS))
    print("=================================================================")

    estado = m.Estado()
    for indice, (nivel, mutar) in enumerate(ROTEIRO, start=1):
        if mutar:
            print("\n-- botao de mute pressionado --")
            m.mutar(estado)

        decisao = m.avalia(estado, nivel)
        linha1 = m.formata_linha_gas(nivel)
        t1, t2 = m.tela_monta(linha1, decisao.mensagem_lcd)

        lcd = m.LcdVirtual()
        m.escreve_em(lcd.emissor, linha1, 0)
        m.escreve_em(lcd.emissor, decisao.mensagem_lcd, 1)

        print("\npasso %2d: ADC=%4d  TA2CCR1=%4d  LED1=%-7s  %s"
              % (indice, nivel, decisao.tom_buzzer,
                 "aceso" if decisao.led_alarme else "apagado",
                 decisao.situacao))
        print("           tela montada : |%s|%s|" % (t1, t2))
        print("           tela virtual : |%s|%s|" % (lcd.texto(0), lcd.texto(1)))

    print("\nSimulacao concluida: %d passos, mute final = %d" % (len(ROTEIRO), estado.mutado))
    return 0


if __name__ == "__main__":
    sys.exit(main())
