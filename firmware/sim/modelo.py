"""
modelo.py - espelho em Python do firmware (fallback sem compilador C).
------------------------------------------------------------------
Reproduz, em Python puro, exatamente as rotinas de:
    firmware/include/config.h   -> constantes
    firmware/src/app.c          -> decisao, mute, formata_linha_gas
    firmware/src/texto.c        -> layout de 16 colunas
    firmware/sim/lcd_virtual.c  -> tela 2x16 e o lcdWrite() original (bug)

Use este modulo apenas quando nao houver gcc/clang disponivel. A prova
principal e o programa em C (sim/testes.c), que chama o codigo real do
firmware (src/app.c, src/texto.c) compilado com -DSIM_HOST.
------------------------------------------------------------------
"""

from collections import namedtuple

# ---------------------------------------------------------------- config.h
LIMIAR_GAS_ADC = 1500
LIMIAR_GAS_TEORICO_MV = 1650
LIMIAR_GAS_TEORICO_ADC = 2048

ADC12_MAX_CONTAGENS = 4095

LCD_COLUNAS = 16
LCD_LINHAS = 2
LCD_TAM_LINHA = LCD_COLUNAS + 1
LCD_I2C_ENDERECO = 0x27
LCD_CMD_LINHA1 = 0x80
LCD_CMD_LINHA2 = 0xC0
LCD_CMD_CLEAR = 0x01

PORTA_ADC = 6
PINO_ADC_BIT_NUM = 0
PORTA_BUZZER = 2
PINO_BUZZER_BIT_NUM = 4
PORTA_BOTAO = 2
PINO_BOTAO_BIT_NUM = 1
BOTAO_P2IV_VALOR = 0x04
PORTA_LED = 1
PINO_LED_BIT_NUM = 0
PORTA_I2C = 3
PINO_I2C_SDA_BIT_NUM = 0
PINO_I2C_SCL_BIT_NUM = 1

BUZZER_PERIODO_CCR0 = 1000
BUZZER_CCR1_ALARME = 500
BUZZER_CCR1_REPOUSO = 1000

BOTAO_DEBOUNCE_MS = 20
APP_CICLOS_ENTRE_LEITURAS = 2000000

TEXTO_CMD_POSICAO = 0x80
TEXTO_CMD_CHAR = 0x01

SEGURO = "SEGURO"
ALERTA = "ALERTA"
ALERTA_MUTADO = "ALERTA_MUTADO"


# ------------------------------------------------------------------- app.h
class Estado:
    """Estado persistente entre leituras (equivale a app_estado_t)."""

    def __init__(self):
        self.mutado = 0


Decisao = namedtuple("Decisao", "situacao tom_buzzer mutado mensagem_lcd led_alarme")


# ------------------------------------------------------------------- app.c
def acima_do_limiar(nivel):
    """Comparacao estritamente maior, igual ao sketch original."""
    return 1 if int(nivel) > LIMIAR_GAS_ADC else 0


def avalia(estado, nivel):
    if acima_do_limiar(nivel):
        if estado.mutado == 0:
            decisao = Decisao(ALERTA, BUZZER_CCR1_ALARME, estado.mutado,
                              "ALERTA DE GAS!", 1)
        else:
            decisao = Decisao(ALERTA_MUTADO, BUZZER_CCR1_REPOUSO, estado.mutado,
                              "ALERTA (MUTADO)", 1)
    else:
        estado.mutado = 0                      # rearme automatico
        decisao = Decisao(SEGURO, BUZZER_CCR1_REPOUSO, estado.mutado,
                          "Ambiente Seguro", 0)
    return decisao._replace(mutado=estado.mutado)


def mutar(estado):
    estado.mutado = 1


def formata_linha_gas(nivel):
    """Devolve 'Nivel Gas: NNNN' (15 caracteres), mesma matematica do C."""
    nivel = int(nivel) & 0xFFFF
    return (
        "Nivel Gas: "
        + str((nivel // 1000) % 10)
        + str((nivel // 100) % 10)
        + str((nivel // 10) % 10)
        + str(nivel % 10)
    )


# ------------------------------------------------------------- texto.c
def ajusta_linha16(origem):
    """Copia ate 16 caracteres e completa com espacos."""
    return (origem or "")[:LCD_COLUNAS].ljust(LCD_COLUNAS)


def _comando_da_linha(linha):
    return LCD_CMD_LINHA1 if linha == 0 else LCD_CMD_LINHA2


def escreve_em(emissor, texto, linha_inicial=0):
    """Mesma regra do texto.c: quebra DEPOIS da 16a coluna, preenche a
    ultima linha com espacos e descarta o excedente."""
    coluna = 0
    linha = linha_inicial if linha_inicial < LCD_LINHAS else LCD_LINHAS - 1
    emissor(TEXTO_CMD_POSICAO, _comando_da_linha(linha))

    i = 0
    while i < len(texto):
        c = texto[i]
        if c == "\n":
            while coluna < LCD_COLUNAS:
                emissor(TEXTO_CMD_CHAR, ord(" "))
                coluna += 1
            if linha + 1 >= LCD_LINHAS:
                return
            linha += 1
            coluna = 0
            emissor(TEXTO_CMD_POSICAO, _comando_da_linha(linha))
            i += 1
            continue

        if coluna >= LCD_COLUNAS:
            if linha + 1 >= LCD_LINHAS:
                return
            linha += 1
            coluna = 0
            emissor(TEXTO_CMD_POSICAO, _comando_da_linha(linha))

        emissor(TEXTO_CMD_CHAR, ord(c) & 0xFF)
        coluna += 1
        i += 1

    while coluna < LCD_COLUNAS:
        emissor(TEXTO_CMD_CHAR, ord(" "))
        coluna += 1


def escreve(emissor, texto):
    escreve_em(emissor, texto, 0)


# ------------------------------------------------------ tela 2x16 em RAM
class Tela:
    def __init__(self):
        self.linha = [[" "] * LCD_COLUNAS, [" "] * LCD_COLUNAS]
        self.linha_atual = 0
        self.coluna_atual = 0

    def emissor(self, comando, valor):
        if comando == TEXTO_CMD_POSICAO:
            self.linha_atual = 1 if valor == LCD_CMD_LINHA2 else 0
            self.coluna_atual = 0
        elif comando == TEXTO_CMD_CHAR:
            if self.coluna_atual < LCD_COLUNAS:
                self.linha[self.linha_atual][self.coluna_atual] = chr(valor)
                self.coluna_atual += 1

    def texto(self, indice):
        return "".join(self.linha[indice])

    def limpa(self):
        self.linha = [[" "] * LCD_COLUNAS, [" "] * LCD_COLUNAS]
        self.linha_atual = 0
        self.coluna_atual = 0


def tela_monta(linha1, linha2):
    """Equivale a app_tela_monta(): devolve as duas linhas de 16 colunas."""
    tela = Tela()
    tela.limpa()
    escreve_em(tela.emissor, linha1, 0)
    escreve_em(tela.emissor, linha2, 1)
    return tela.texto(0), tela.texto(1)


# -------------------------------------------------------- lcd_virtual.c
class LcdVirtual:
    """Modelo do LCD 16x2, incluindo a reproducao do lcdWrite() original."""

    def __init__(self):
        self.linha = [[" "] * LCD_COLUNAS, [" "] * LCD_COLUNAS]
        self.cursor_linha = 0
        self.cursor_coluna = 0

    # -- comandos/dados --
    def comando(self, comando):
        if comando == LCD_CMD_CLEAR:
            self.linha = [[" "] * LCD_COLUNAS, [" "] * LCD_COLUNAS]
            self.cursor_linha = 0
            self.cursor_coluna = 0
        elif comando == LCD_CMD_LINHA1:
            self.cursor_linha = 0
            self.cursor_coluna = 0
        elif comando == LCD_CMD_LINHA2:
            self.cursor_linha = 1
            self.cursor_coluna = 0

    def caractere(self, c):
        if self.cursor_coluna < LCD_COLUNAS:
            self.linha[self.cursor_linha][self.cursor_coluna] = chr(c)
            self.cursor_coluna += 1

    # -- emissor compativel com texto.escreve_em --
    def emissor(self, comando, valor):
        if comando == TEXTO_CMD_POSICAO:
            self.comando(valor)
        elif comando == TEXTO_CMD_CHAR:
            self.caractere(valor)

    # -- reproducao FIEL do lcdWrite() original (com bug) --
    def escreve_original(self, texto):
        col = 0
        i = 0
        while i < len(texto):
            if texto[i] == "\n" or col == LCD_COLUNAS:
                self.comando(LCD_CMD_LINHA2)
                col = 0
                if texto[i] == "\n":
                    i += 1
                    continue
            self.caractere(ord(texto[i]) & 0xFF)
            col += 1
            i += 1

    def texto(self, indice):
        return "".join(self.linha[indice])

    def imprime(self):
        return "|%s|\n|%s|" % (self.texto(0), self.texto(1))
