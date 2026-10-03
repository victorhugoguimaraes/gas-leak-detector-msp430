# ======================================================================
# Detector de Vazamento de Gas - MSP430F5529
# ----------------------------------------------------------------------
# Alvos:
#   all        -> compila o firmware para o MSP430 (EXIGE msp430-elf-gcc)
#   clean      -> remove o diretorio build/
#   sim        -> compila e RODA os testes no PC (gcc/clang comum)
#   simulador  -> compila e roda a demonstracao do laco principal no PC
#   sim-build  -> apenas compila os dois programas de host
#   python-sim -> roda os MESMOS testes em Python puro (fallback)
#   help       -> mostra esta ajuda
#
# ATENCAO (importante): nesta maquina o toolchain da TI
# (msp430-elf-gcc + msp430-gcc-support-files) NAO esta instalado, logo
# `make all` NAO gera o .elf/.hex aqui (o alvo avisa e para). O alvo
# `sim` funciona com um gcc de PC; no MSYS2 ele fica em
# C:\msys64\mingw64\bin - adicione esse diretorio ao PATH.
#
# O Makefile funciona tanto com o shell do MSYS2 (sh) quanto com o
# cmd.exe do mingw32-make: os comandos de diretorio/limpeza sao
# escolhidos automaticamente pelo bloco de deteccao abaixo.
# ======================================================================

MCU          ?= msp430f5529
CC_MSP       ?= msp430-elf-gcc
OBJCOPY      ?= msp430-elf-objcopy
SUPPORT_DIR  ?= /opt/ti/msp430-gcc/include

HOST_CC      ?= gcc
PYTHON       ?= C:/Users/victo/.dsh/dsh-runtimes/dsh-primary-runtime/dependencies/python/python.exe

TARGET       := gas-leak-detector
SRC_DIR      := src
INC_DIR      := include
SIM_DIR      := sim
BUILD        := build

SRCS         := $(wildcard $(SRC_DIR)/*.c)
OBJS         := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))
SIM_COMUM    := $(SRC_DIR)/app.c $(SRC_DIR)/texto.c $(SIM_DIR)/lcd_virtual.c

# ---------- deteccao de shell (cmd.exe x sh) ----------
# Se `sh` estiver no PATH, o GNU Make o usa como shell; senao usa cmd.
SHELL_TEM_SH := $(shell command -v sh 2>NUL)
ifneq ($(strip $(SHELL_TEM_SH)),)
  SHELLKIND := sh
  MKDIR     := mkdir -p
  LIMPA     := rm -rf $(BUILD)
  NULO      := /dev/null
  RUN_DIR   := $(BUILD)
else
  SHELLKIND := cmd
  MKDIR     := mkdir
  LIMPA     := rmdir /s /q $(BUILD)
  NULO      := NUL
  RUN_DIR   := $(subst /,\,$(BUILD))
endif

ifeq ($(OS),Windows_NT)
EXE := .exe
endif
EXE ?=

CFLAGS_MSP   := -mmcu=$(MCU) -Os -g -Wall -Wextra -std=c99 -I$(INC_DIR) -I$(SUPPORT_DIR)
LDFLAGS_MSP  := -mmcu=$(MCU) -L$(SUPPORT_DIR)
CFLAGS_SIM   := -std=c99 -Wall -Wextra -Wpedantic -O2 -DSIM_HOST -I$(INC_DIR) -I$(SIM_DIR)

.PHONY: all clean sim sim-build simulador python-sim check-toolchain help

all: check-toolchain $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex
	@echo Firmware gerado em $(BUILD)/

# ---------- verificacao do toolchain da TI ----------
check-toolchain:
	@$(CC_MSP) --version >$(NULO) 2>&1 || (echo ERRO: $(CC_MSP) nao encontrado. Este alvo exige o toolchain da TI, que NAO esta instalado nesta maquina. Use "make sim" para validar a logica no PC. && exit 1)

# ---------- firmware MSP430 ----------
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC_MSP) $(CFLAGS_MSP) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC_MSP) $(OBJS) $(LDFLAGS_MSP) -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# ---------- simulador / testes no PC ----------
sim-build: $(BUILD)/testes$(EXE) $(BUILD)/simulador$(EXE)

$(BUILD)/testes$(EXE): $(SIM_COMUM) $(SIM_DIR)/testes.c | $(BUILD)
	$(HOST_CC) $(CFLAGS_SIM) $(SIM_COMUM) $(SIM_DIR)/testes.c -o $@

$(BUILD)/simulador$(EXE): $(SIM_COMUM) $(SIM_DIR)/simulador.c | $(BUILD)
	$(HOST_CC) $(CFLAGS_SIM) $(SIM_COMUM) $(SIM_DIR)/simulador.c -o $@

sim: $(BUILD)/testes$(EXE)
	@$(RUN_DIR)/testes$(EXE)

simulador: $(BUILD)/simulador$(EXE)
	@$(RUN_DIR)/simulador$(EXE)

# Fallback sem compilador C: mesmos testes em Python puro.
python-sim:
	$(PYTHON) $(SIM_DIR)/testes.py

$(BUILD):
	-@$(MKDIR) $@

clean:
	-@$(LIMPA)
	@echo Limpeza concluida.

help:
	@echo make sim        -^> compila e roda os testes no PC (gcc/clang)
	@echo make simulador  -^> compila e roda a demonstracao no PC
	@echo make python-sim -^> testes em Python puro (fallback)
	@echo make all        -^> firmware MSP430 (exige msp430-elf-gcc; AUSENTE aqui)
	@echo make clean      -^> apaga $(BUILD)/
