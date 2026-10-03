#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Anonimização do CSV exportado pelo formulário
=============================================

O CSV exportado do Tally traz colunas de controle (Submission ID, Respondent ID,
data/hora de envio) que não são necessárias para a análise e que, em um
repositório público, expõem metadados dos participantes.

Este script gera `data/pesquisa_vazamento_gas.csv` a partir do arquivo bruto,
mantendo todas as respostas e removendo os identificadores. As perguntas são
renomeadas para um esquema curto e estável (`q01_tipo_gas`, ...), o que também
deixa a análise menos frágil do que depender do texto exato das colunas.

Uso:
    python analysis/anonimizar_csv.py caminho/do/export_bruto.csv
"""

import os
import sys

import pandas as pd

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DESTINO = os.path.join(RAIZ, "data", "pesquisa_vazamento_gas.csv")

# Colunas de controle que NÃO vão para o repositório público
COLUNAS_DESCARTADAS = {
    "Submission ID",
    "Respondent ID",
    "Submitted at",
}


def normalizar(texto: str) -> str:
    import unicodedata
    return "".join(
        c for c in unicodedata.normalize("NFKD", str(texto))
        if not unicodedata.combining(c)
    ).strip().lower()


# Mapeamento: início do nome original (normalizado) -> nome curto
MAPA = [
    ("voce utiliza botijao", "q01_tipo_gas"),
    ("qual e a sua idade", "q02_idade"),
    ("com que frequencia voce utiliza", "q03_frequencia_uso"),
    ("voce ja presenciou", "q04_ja_presenciou"),
    ("como voce identificaria um possivel vazamento de gas? (pelo cheiro)", "q05_det_cheiro"),
    ("como voce identificaria um possivel vazamento de gas? (pelo som", "q05_det_som"),
    ("como voce identificaria um possivel vazamento de gas? (por detector", "q05_det_eletronico"),
    ("como voce identificaria um possivel vazamento de gas? (mudanca na chama", "q05_det_chama"),
    ("como voce identificaria um possivel vazamento de gas? (nao saberia", "q05_det_nao_saberia"),
    ("como voce identificaria um possivel vazamento de gas? (outro", "q05_det_outro"),
    ("voce possui algum detector", "q06_possui_detector"),
    ("voce considera que apenas o cheiro", "q07_cheiro_suficiente"),
    ("em uma situacao de vazamento, qual tipo de alerta", "q08_tipo_alerta"),
    ("voce acha muito importante que o alarme", "q09_ouvir_outros_comodos"),
    ("um botao para silenciar", "q10_botao_silenciar"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (proximo ao fogao)", "q11_local_fogao"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (proximo ao botijao)", "q11_local_botijao"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (na cozinha em geral)", "q11_local_cozinha"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (proximo ao teto)", "q11_local_teto"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (proximo ao chao)", "q11_local_chao"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (em varios comodos)", "q11_local_varios_comodos"),
    ("onde voce acha que um dispositivo de alerta de gas deveria ser instalado? (outro", "q11_local_outro"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (baixo custo", "q12_carac_baixo_custo"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (simplicidade", "q12_carac_simplicidade"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (facilidade de instalacao", "q12_carac_facil_instalacao"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (aviso sonoro", "q12_carac_aviso_sonoro"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (aviso visual", "q12_carac_aviso_visual"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (tamanho reduzido", "q12_carac_tamanho"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (integracao com celular", "q12_carac_celular"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (baixo consumo", "q12_carac_baixo_consumo"),
    ("quais caracteristicas seriam mais importantes em um sistema desse tipo? (outro", "q12_carac_outro"),
]


def nome_curto(coluna: str) -> str:
    n = normalizar(coluna)
    for inicio, curto in MAPA:
        if n.startswith(normalizar(inicio)):
            return curto
    if n.startswith("como voce identificaria um possivel vazamento de gas"):
        return "q05_det_texto_livre"
    if n.startswith("onde voce acha que um dispositivo"):
        return "q11_local_texto_livre"
    if n.startswith("quais caracteristicas seriam mais importantes"):
        return "q12_carac_texto_livre"
    return "col_" + n.replace(" ", "_")[:40]


def main() -> int:
    if len(sys.argv) < 2:
        print("uso: python analysis/anonimizar_csv.py <csv_bruto>", file=sys.stderr)
        return 1

    origem = sys.argv[1]
    df = pd.read_csv(origem, encoding="utf-8-sig")

    descartadas = [c for c in df.columns if str(c).strip() in COLUNAS_DESCARTADAS]
    df = df.drop(columns=descartadas)

    # Primeiro resolve o nome curto de CADA coluna original.
    renomeadas = {col: nome_curto(col) for col in df.columns}

    # Depois garante unicidade. Isto é necessário porque o CSV do Tally pode
    # expor sub-opções fora de ordem: um campo de texto livre pode vir ANTES
    # das opções específicas, e a correspondência por prefixo daria o mesmo
    # nome a colunas diferentes.
    vistos: dict[str, int] = {}
    for col, curto in renomeadas.items():
        vistos[curto] = vistos.get(curto, 0) + 1
    duplicados = {k for k, v in vistos.items() if v > 1}
    if duplicados:
        print(f"[aviso] nomes curtos duplicados (serão desambiguados): "
              f"{sorted(duplicados)}", file=sys.stderr)

    # Desambiguação preferindo o mapa explícito: se duas colunas originais
    # deram o mesmo nome curto, a que casou um item específico do MAPA vence e
    # a outra recebe sufixo "_texto_livre".
    especificos = {}
    for col in df.columns:
        n = normalizar(col)
        for inicio, curto in MAPA:
            if n.startswith(normalizar(inicio)):
                especificos[col] = curto
                break

    final = {}
    contagem: dict[str, int] = {}
    for col in df.columns:
        curto = especificos.get(col, renomeadas[col])
        contagem[curto] = contagem.get(curto, 0) + 1
        if contagem[curto] > 1:
            base = renomeadas[col]
            if base in especificos.values():
                base = base + "_texto_livre"
            curto = f"{base}_{contagem[curto]}"
        final[col] = curto

    df = df.rename(columns=final)

    os.makedirs(os.path.dirname(DESTINO), exist_ok=True)
    df.to_csv(DESTINO, index=False, encoding="utf-8")
    print(f"[ok] {DESTINO}")
    print(f"     {len(df)} respostas, {len(df.columns)} colunas")
    print(f"     colunas de controle removidas: {descartadas}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
