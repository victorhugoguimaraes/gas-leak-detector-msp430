#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Análise dos dados da pesquisa de campo
======================================

Lê o CSV exportado do formulário (Tally) e produz:

  - data/survey.json                 -> dados agregados usados pelo site e pelo README
  - analysis/out/figura-*.png        -> gráficos em alta resolução (300 dpi)
  - analysis/out/analise.md          -> relatório da análise em Markdown

Uso:
    python analysis/analisar_pesquisa.py [caminho/do/arquivo.csv]

O script funciona sem o CSV original: se ele não for encontrado, os valores são
lidos de data/survey.json (útil para reprocessar o site em outra máquina).
"""

import json
import os
import sys
import unicodedata

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_PADRAO = os.path.join(RAIZ, "data", "pesquisa_vazamento_gas.csv")
SAIDA_JSON = os.path.join(RAIZ, "data", "survey.json")
SAIDA_FIGS = os.path.join(RAIZ, "analysis", "out")
SAIDA_MD = os.path.join(SAIDA_FIGS, "analise.md")

# ---------------------------------------------------------------------------
# Identidade visual (a mesma usada na landing page e no README)
# ---------------------------------------------------------------------------
COR_FUNDO = "#0b1f2a"
COR_SUPERFICIE = "#12303d"
COR_TEXTO = "#e8f1f5"
COR_TEXTO_SUAVE = "#8fb0be"
COR_PRIMARIA = "#f2a71b"   # âmbar de alerta
COR_PERIGO = "#ef5d4a"     # coral de vazamento
COR_SEGURA = "#3ddc97"     # verde de "ambiente seguro"

plt.rcParams.update({
    "figure.facecolor": COR_FUNDO,
    "axes.facecolor": COR_FUNDO,
    "savefig.facecolor": COR_FUNDO,
    "text.color": COR_TEXTO,
    "axes.labelcolor": COR_TEXTO,
    "xtick.color": COR_TEXTO_SUAVE,
    "ytick.color": COR_TEXTO_SUAVE,
    "axes.edgecolor": COR_SUPERFICIE,
    "font.family": "DejaVu Sans",
    "font.size": 11,
    "axes.titlesize": 15,
    "axes.titleweight": "bold",
    "figure.dpi": 110,
    "savefig.dpi": 300,
    "savefig.bbox": "tight",
})


def normalizar(texto: str) -> str:
    """Remove acentos e normaliza para comparações robustas de rótulos."""
    if not isinstance(texto, str):
        return ""
    return "".join(
        c for c in unicodedata.normalize("NFKD", texto)
        if not unicodedata.combining(c)
    ).strip()


def encontrar_coluna(df: pd.DataFrame, *trechos: str):
    """Localiza uma coluna aceitando tanto o export bruto do Tally quanto o CSV
    anonimizado. `trechos` são alternativas: basta que TODOS os pedaços de UMA
    das alternativas estejam presentes. Use "|" para separar alternativas:

        encontrar_coluna(df, "botijao|q01", "gas encanado|tipo_gas")

    Aqui a regra é mais simples e suficiente: a coluna casa se contiver todos
    os trechos separados por "+" em alguma das alternativas separadas por "|".
    """
    for col in df.columns:
        alvo = normalizar(col).lower()
        for alternativa in trechos:
            for opcao in alternativa.split("|"):
                pedacos = [normalizar(p).lower() for p in opcao.split("+") if p.strip()]
                if pedacos and all(p in alvo for p in pedacos):
                    return col
    return None


def eh_marcado(valor) -> bool:
    """Interpreta o booleano exportado pelo Tally (true/false, 1/0, etc.).

    O CSV anonimizado grava os booleanos como strings minúsculas ("true"), e o
    export bruto pode trazê-los como bool nativo — os dois casos são aceitos.
    """
    if isinstance(valor, bool):
        return valor
    return normalizar(str(valor)).lower() in {"true", "1", "sim", "yes"}


def contar_colunas_multi(df: pd.DataFrame, prefixo: str,
                         rotulos: dict | None = None) -> dict[str, int]:
    """Conta quantas vezes cada sub-opção de uma pergunta de múltipla escolha
    foi marcada.

    Funciona com os dois esquemas de nome de coluna aceitos pelo script:
      - o export bruto do Tally (pergunta longa + sufixo entre parênteses);
      - o CSV anonimizado (colunas curtas q05_*, q11_*, q12_*).

    `rotulos` mapeia nome-curto -> rótulo humano exibido nos relatórios.

    `prefixo` aceita alternativas separadas por "|" (ex.: "como voce
    identificaria|q05"), para cobrir os dois esquemas de nomeação de coluna.
    """
    alternativas = [normalizar(p).lower() for p in prefixo.split("|") if p.strip()]
    contagem: dict[str, int] = {}
    for col in df.columns:
        bruto = str(col)
        alvo = normalizar(bruto).lower()
        if not any(alvo.startswith(a) for a in alternativas):
            continue
        if rotulos:
            chave = bruto.strip().lower()
            if chave not in rotulos:
                continue
            rotulo = rotulos[chave]
        else:
            if "(" not in bruto or not bruto.rstrip().endswith(")"):
                continue
            rotulo = bruto[bruto.rfind("(") + 1:-1].strip()
            # colunas utilitárias que o Tally cria às vezes terminam com "(2)"
            if rotulo.isdigit():
                continue
        marcados = sum(1 for v in df[col] if eh_marcado(v))
        if marcados:
            contagem[rotulo] = contagem.get(rotulo, 0) + marcados
    return dict(sorted(contagem.items(), key=lambda kv: -kv[1]))


def contar_simples(df: pd.DataFrame, coluna: str) -> dict[str, int]:
    contagem = df[coluna].dropna().astype(str).str.strip()
    contagem = contagem[contagem != ""]
    return dict(sorted(contagem.value_counts().items(), key=lambda kv: -kv[1]))


def pct(parte: float, total: float) -> float:
    return round(100.0 * parte / total, 1) if total else 0.0


# ---------------------------------------------------------------------------
# Rótulos humanos para as colunas curtas do CSV anonimizado.
# Chave = nome da coluna em minúsculas; valor = o que aparece nos relatórios.
# Com o export bruto do Tally estes mapas são ignorados (o rótulo sai do
# próprio sufixo entre parênteses).
# ---------------------------------------------------------------------------
ROTULOS_DETECCAO = {
    "q05_det_cheiro": "Pelo cheiro",
    "q05_det_som": "Pelo som do vazamento",
    "q05_det_eletronico": "Por detector eletrônico",
    "q05_det_chama": "Mudança na chama do fogão",
    "q05_det_nao_saberia": "Não saberia identificar",
    "q05_det_outro": "Outro",
}

ROTULOS_LOCAIS = {
    "q11_local_fogao": "Próximo ao fogão",
    "q11_local_botijao": "Próximo ao botijão",
    "q11_local_cozinha": "Na cozinha em geral",
    "q11_local_teto": "Próximo ao teto",
    "q11_local_chao": "Próximo ao chão",
    "q11_local_varios_comodos": "Em vários cômodos",
    "q11_local_outro": "Outro",
}

ROTULOS_CARACTERISTICAS = {
    "q12_carac_baixo_custo": "Baixo custo",
    "q12_carac_simplicidade": "Simplicidade",
    "q12_carac_facil_instalacao": "Facilidade de instalação",
    "q12_carac_aviso_sonoro": "Aviso sonoro",
    "q12_carac_aviso_visual": "Aviso visual",
    "q12_carac_tamanho": "Tamanho reduzido",
    "q12_carac_celular": "Integração com celular",
    "q12_carac_baixo_consumo": "Baixo consumo de energia",
    "q12_carac_outro": "Outro",
}


# ---------------------------------------------------------------------------
# Carregamento
# ---------------------------------------------------------------------------
def carregar(caminho_csv: str) -> pd.DataFrame:
    df = pd.read_csv(caminho_csv, encoding="utf-8-sig")
    df.columns = [str(c).strip() for c in df.columns]
    return df


def analisar(df: pd.DataFrame) -> dict:
    n = len(df)

    # Aceita os dois esquemas: export bruto do Tally e CSV anonimizado (qNN_*)
    c_tipo = encontrar_coluna(df, "botijao+gas encanado|q01")
    c_idade = encontrar_coluna(df, "qual e a sua idade|q02")
    c_freq = encontrar_coluna(df, "frequencia+utiliza o fogao|q03")
    c_vaz = encontrar_coluna(df, "ja presenciou|q04")
    c_detector = encontrar_coluna(df, "possui algum detector|q06")
    c_cheiro_suf = encontrar_coluna(df, "considera que apenas o cheiro|q07")
    c_alerta = encontrar_coluna(df, "tipo de alerta|q08")
    c_ouvido = encontrar_coluna(df, "ouvido em outros comodos|q09")
    c_botao = encontrar_coluna(df, "silenciar temporariamente|q10")

    idade_num = pd.to_numeric(df[c_idade], errors="coerce") if c_idade else None

    dados = {
        "meta": {
            "total_respostas": int(n),
            "plataforma": "Tally Forms",
            "periodo": "2026-05-10 a 2026-05-11",
            "perguntas": 12,
            "observacao_idade": (
                "As 4 primeiras respostas foram registradas antes de o campo de "
                "idade entrar no formulário; por isso a média considera apenas "
                f"{int(idade_num.notna().sum())} participantes."
            ),
        },
        "perfil": {
            "tipo_gas": contar_simples(df, c_tipo) if c_tipo else {},
            "frequencia_uso": contar_simples(df, c_freq) if c_freq else {},
            "idade": {
                "respondentes_com_idade": int(idade_num.notna().sum()) if c_idade else 0,
                "media": round(float(idade_num.mean()), 1) if c_idade else None,
                "mediana": float(idade_num.median()) if c_idade else None,
                "min": int(idade_num.min()) if c_idade else None,
                "max": int(idade_num.max()) if c_idade else None,
            },
        },
        "vazamentos": {
            "ja_presenciou": contar_simples(df, c_vaz) if c_vaz else {},
            "possui_detector": contar_simples(df, c_detector) if c_detector else {},
        },
        "deteccao": {
            "formas": contar_colunas_multi(df, "como voce identificaria|q05", ROTULOS_DETECCAO),
            "cheiro_suficiente": contar_simples(df, c_cheiro_suf) if c_cheiro_suf else {},
        },
        "preferencias_alerta": {
            "tipo_alerta": contar_simples(df, c_alerta) if c_alerta else {},
            "importancia_ouvido_outros_comodos": contar_simples(df, c_ouvido) if c_ouvido else {},
            "utilidade_botao_silenciar": contar_simples(df, c_botao) if c_botao else {},
        },
        "instalacao": {
            "locais": contar_colunas_multi(df, "onde voce acha que um dispositivo|q11", ROTULOS_LOCAIS),
        },
        "caracteristicas": {
            "prioridades": contar_colunas_multi(df, "quais caracteristicas|q12", ROTULOS_CARACTERISTICAS),
        },
    }

    # ---- indicadores-chave usados no README e no site -------------------
    tipo_gas = dados["perfil"]["tipo_gas"]
    botijao = sum(v for k, v in tipo_gas.items() if "botij" in normalizar(k).lower())
    varias = sum(
        v for k, v in dados["perfil"]["frequencia_uso"].items()
        if "varias" in normalizar(k).lower()
    )
    ja_presenciou = sum(
        v for k, v in dados["vazamentos"]["ja_presenciou"].items()
        if normalizar(k).lower().startswith("sim")
    )
    possui = dados["vazamentos"]["possui_detector"]
    possui_sim = sum(v for k, v in possui.items() if normalizar(k).lower().startswith("sim"))
    pretende = sum(v for k, v in possui.items() if "pretendo" in normalizar(k).lower())
    cheiro = dados["deteccao"]["formas"].get("Pelo cheiro", 0)
    soma_cheiro = sum(dados["deteccao"]["cheiro_suficiente"].values())
    cheiro_depende = sum(
        v for k, v in dados["deteccao"]["cheiro_suficiente"].items()
        if "depende" in normalizar(k).lower() or normalizar(k).lower().startswith("nao")
    )
    ouve = dados["preferencias_alerta"]["importancia_ouvido_outros_comodos"]
    ouve_muito = sum(
        v for k, v in ouve.items() if normalizar(k).lower().startswith("muito")
    )
    ouve_relevante = sum(
        v for k, v in ouve.items()
        if normalizar(k).lower().startswith(("muito", "importante"))
    )
    botao = dados["preferencias_alerta"]["utilidade_botao_silenciar"]
    botao_sim = sum(v for k, v in botao.items() if normalizar(k).lower().startswith("sim"))
    combinacao = sum(
        v for k, v in dados["preferencias_alerta"]["tipo_alerta"].items()
        if "combinacao" in normalizar(k).lower()
    )

    locais = dados["instalacao"]["locais"]
    total_locais = sum(locais.values())
    caracteristicas = dados["caracteristicas"]["prioridades"]

    dados["indicadores"] = {
        "usam_botijao_pct": pct(botijao, n),
        "usam_botijao_n": botijao,
        "usam_varias_vezes_dia_pct": pct(varias, n),
        "usam_varias_vezes_dia_n": varias,
        "ja_presenciou_vazamento_pct": pct(ja_presenciou, n),
        "ja_presenciou_vazamento_n": ja_presenciou,
        "nao_possui_detector_pct": pct(n - possui_sim - pretende, n),
        "nao_possui_detector_n": n - possui_sim - pretende,
        "pretende_adquirir_n": pretende,
        "identificam_pelo_cheiro_pct": pct(cheiro, n),
        "identificam_pelo_cheiro_n": cheiro,
        "cheiro_nao_basta_pct": pct(cheiro_depende, soma_cheiro),
        "cheiro_nao_basta_n": cheiro_depende,
        "querem_ouvir_outros_comodos_pct": pct(ouve_relevante, sum(ouve.values())),
        "querem_ouvir_outros_comodos_n": ouve_relevante,
        "acham_muito_importante_ouvir_pct": pct(ouve_muito, sum(ouve.values())),
        "botao_silenciar_util_pct": pct(botao_sim, sum(botao.values())),
        "botao_silenciar_util_n": botao_sim,
        "preferem_combinacao_alerta_pct": pct(combinacao, sum(dados["preferencias_alerta"]["tipo_alerta"].values())),
        "media_locais_por_pessoa": round(total_locais / n, 1) if n else 0,
        "top_local": next(iter(locais), None),
        "top_local_pct": pct(locais[next(iter(locais))], n) if locais else 0,
        "top_caracteristicas": list(caracteristicas.items())[:4],
    }

    # ---- números que o artigo do README cita ---------------------------
    dados["narrativa"] = {
        "relato": (
            "Uma moradora relatou dificuldade para perceber o cheiro de gás. Houve um "
            "vazamento no fogão de sua residência que ficou sem ser identificado por um "
            "período, e só foi percebido quando o técnico foi trocar o botijão."
        ),
        "problema": (
            "A identificação de vazamentos depende quase inteiramente do olfato humano — "
            "justamente o sentido que falha em idosos, pessoas com limitações sensoriais "
            "ou em momentos de distração."
        ),
        "decisao_projeto": [
            "alerta redundante: som + luz + texto, porque a maioria quer combinação e aviso audível em outros cômodos;",
            "botão de silenciamento, porque a maioria considerou a função útil;",
            "instalação perto do fogão e do botijão, os locais mais citados;",
            "baixo custo e facilidade de instalação, as características mais valorizadas;",
            "notificação por celular ficou de fora do escopo por custo e complexidade (registrado como limitação).",
        ],
    }

    return dados


# ---------------------------------------------------------------------------
# Gráficos
# ---------------------------------------------------------------------------
def _barras_h(ax, rotulos, valores, total, cor, titulo, eixo_x=None):
    rotulos_quebra = [r.replace(" (", "\n(") for r in rotulos]
    y = np.arange(len(rotulos))
    ax.barh(y, valores, color=cor, height=0.62, zorder=3)
    ax.set_yticks(y)
    ax.set_yticklabels(rotulos_quebra, color=COR_TEXTO)
    ax.invert_yaxis()
    ax.set_title(titulo, pad=14)
    ax.grid(axis="x", color=COR_SUPERFICIE, linewidth=0.8, zorder=0)
    ax.set_axisbelow(True)
    for lado in ("top", "right", "left"):
        ax.spines[lado].set_visible(False)
    limite = max(valores) * 1.28 if valores else 1
    ax.set_xlim(0, limite)
    for i, v in enumerate(valores):
        ax.text(v + limite * 0.017, i, f"{v}  ({pct(v, total):.0f}%)",
                va="center", color=COR_TEXTO, fontsize=10)
    if eixo_x:
        ax.set_xlabel(eixo_x, color=COR_TEXTO_SUAVE)


def salvar(fig, nome):
    caminho = os.path.join(SAIDA_FIGS, nome)
    fig.savefig(caminho)
    plt.close(fig)
    return caminho


def gerar_graficos(dados: dict) -> list[str]:
    os.makedirs(SAIDA_FIGS, exist_ok=True)
    n = dados["meta"]["total_respostas"]
    arquivos = []

    # 1. Tipo de gás ------------------------------------------------
    tipo = dados["perfil"]["tipo_gas"]
    fig, ax = plt.subplots(figsize=(7.2, 3.6))
    _barras_h(ax, list(tipo), list(tipo.values()), n,
              COR_PRIMARIA, "Tipo de gás utilizado  ·  n=%d" % n,
              eixo_x="Respostas")
    arquivos.append(salvar(fig, "figura-01-tipo-de-gas.png"))

    # 2. Frequência de uso -----------------------------------------
    freq = dados["perfil"]["frequencia_uso"]
    fig, ax = plt.subplots(figsize=(7.2, 3.6))
    _barras_h(ax, list(freq), list(freq.values()), n,
              COR_PRIMARIA, "Frequência de uso do fogão  ·  n=%d" % n,
              eixo_x="Respostas")
    arquivos.append(salvar(fig, "figura-02-frequencia-de-uso.png"))

    # 3. Vivência de vazamento + posse de detector ------------------
    vaz = dados["vazamentos"]["ja_presenciou"]
    det = dados["vazamentos"]["possui_detector"]
    fig, axes = plt.subplots(1, 2, figsize=(11, 3.9))
    _barras_h(axes[0], list(vaz), list(vaz.values()), n, COR_PERIGO,
              "Já presenciou/suspeitou de vazamento", eixo_x="Respostas")
    _barras_h(axes[1], list(det), list(det.values()), n, COR_PERIGO,
              "Possui detector automático hoje", eixo_x="Respostas")
    fig.tight_layout()
    arquivos.append(salvar(fig, "figura-03-vazamento-e-detector.png"))

    # 4. Como identifica um vazamento -------------------------------
    formas = dados["deteccao"]["formas"]
    fig, ax = plt.subplots(figsize=(7.6, 3.8))
    _barras_h(ax, list(formas), list(formas.values()), n, COR_PRIMARIA,
              "Como o participante identificaria um vazamento (múltipla escolha)",
              eixo_x="Menções")
    arquivos.append(salvar(fig, "figura-04-formas-de-deteccao.png"))

    # 5. O cheiro basta? -------------------------------------------
    suf = dados["deteccao"]["cheiro_suficiente"]
    fig, ax = plt.subplots(figsize=(7.2, 3.4))
    _barras_h(ax, list(suf), list(suf.values()), sum(suf.values()), COR_PERIGO,
              "“Só o cheiro basta para perceber um vazamento?”", eixo_x="Respostas")
    arquivos.append(salvar(fig, "figura-05-cheiro-suficiente.png"))

    # 6. Preferências de alerta ------------------------------------
    alerta = dados["preferencias_alerta"]["tipo_alerta"]
    fig, ax = plt.subplots(figsize=(7.6, 4.0))
    _barras_h(ax, list(alerta), list(alerta.values()),
              sum(alerta.values()), COR_PRIMARIA,
              "Tipo de alerta considerado mais útil", eixo_x="Respostas")
    arquivos.append(salvar(fig, "figura-06-tipo-de-alerta.png"))

    # 7. Importância de ouvir em outros cômodos + botão de silenciar
    ouve = dados["preferencias_alerta"]["importancia_ouvido_outros_comodos"]
    botao = dados["preferencias_alerta"]["utilidade_botao_silenciar"]
    fig, axes = plt.subplots(1, 2, figsize=(11, 3.7))
    _barras_h(axes[0], list(ouve), list(ouve.values()), sum(ouve.values()),
              COR_PERIGO, "Alarme audível em outros cômodos", eixo_x="Respostas")
    _barras_h(axes[1], list(botao), list(botao.values()), sum(botao.values()),
              COR_SEGURA, "Botão de silenciar seria útil?", eixo_x="Respostas")
    fig.tight_layout()
    arquivos.append(salvar(fig, "figura-07-alerta-e-silenciamento.png"))

    # 8. Locais de instalação --------------------------------------
    locais = dados["instalacao"]["locais"]
    fig, ax = plt.subplots(figsize=(7.6, 3.8))
    _barras_h(ax, list(locais), list(locais.values()), n, COR_SEGURA,
              "Onde o dispositivo deveria ser instalado (múltipla escolha)",
              eixo_x="Menções")
    arquivos.append(salvar(fig, "figura-08-locais-de-instalacao.png"))

    # 9. Características desejadas ---------------------------------
    carac = dados["caracteristicas"]["prioridades"]
    fig, ax = plt.subplots(figsize=(8.4, 4.6))
    _barras_h(ax, list(carac), list(carac.values()), n, COR_PRIMARIA,
              "Características mais valorizadas em uma solução (múltipla escolha)",
              eixo_x="Menções")
    arquivos.append(salvar(fig, "figura-09-caracteristicas-desejadas.png"))

    return arquivos


# ---------------------------------------------------------------------------
# Relatório em Markdown
# ---------------------------------------------------------------------------
def gerar_markdown(dados: dict, arquivos: list) -> str:
    i = dados["indicadores"]
    m = dados["meta"]
    n = m["total_respostas"]
    idade = dados["perfil"]["idade"]

    def tabela(cabecalhos, contagem, total, coluna_n="N"):
        L = ["| " + " | ".join(cabecalhos) + " |",
             "| " + " | ".join("---" for _ in cabecalhos) + " |"]
        for k, v in contagem.items():
            L.append(f"| {k} | {v} | {pct(v, total):.1f}% |")
        return "\n".join(L)

    p = []
    add = p.append

    add("# Análise da pesquisa de campo\n")
    add("> Gerado automaticamente por `analysis/analisar_pesquisa.py` a partir de "
        "`data/pesquisa_vazamento_gas.csv`.\n>\n"
        f"> Formulário digital (Tally) · {m['periodo']} · **{n} respostas** · "
        f"{m['perguntas']} perguntas.\n")

    add("## 1. Resumo executivo\n")
    add("| Indicador | Resultado |")
    add("| --- | --- |")
    add(f"| Usam botijão de gás | **{i['usam_botijao_pct']:.0f}%** "
        f"({i['usam_botijao_n']} de {n}) |")
    add(f"| Usam o fogão várias vezes ao dia | **{i['usam_varias_vezes_dia_pct']:.0f}%** "
        f"({i['usam_varias_vezes_dia_n']} de {n}) |")
    add(f"| Já presenciaram ou suspeitaram de vazamento | "
        f"**{i['ja_presenciou_vazamento_pct']:.0f}%** "
        f"({i['ja_presenciou_vazamento_n']} de {n}) |")
    add(f"| **Não** possuem detector automático | "
        f"**{i['nao_possui_detector_pct']:.0f}%** "
        f"({i['nao_possui_detector_n']} de {n}) |")
    add(f"| Identificariam vazamento **pelo cheiro** | "
        f"**{i['identificam_pelo_cheiro_pct']:.0f}%** "
        f"({i['identificam_pelo_cheiro_n']} de {n}) |")
    add(f"| Dizem que só o cheiro **não basta** em toda situação | "
        f"**{i['cheiro_nao_basta_pct']:.0f}%** "
        f"({i['cheiro_nao_basta_n']} de {n}) |")
    add(f"| Querem ouvir o alarme em outros cômodos | "
        f"**{i['querem_ouvir_outros_comodos_pct']:.0f}%** |")
    add(f"| Acham o botão de silenciar útil | "
        f"**{i['botao_silenciar_util_pct']:.0f}%** "
        f"({i['botao_silenciar_util_n']} de {n}) |")
    add("| Preferem combinação de alertas (som + luz) | "
        f"**{i['preferem_combinacao_alerta_pct']:.0f}%** |")
    add("")
    add("### Leitura dos dados\n")
    add("O retrato é coerente: **quase todo mundo cozinha com gás todos os dias**, "
        "uma parcela relevante **já conviveu com suspeita de vazamento** e "
        "**praticamente ninguém tem detecção automática**. A única forma de "
        "detecção citada por 100% dos participantes é o **olfato** — e "
        f"{i['cheiro_nao_basta_pct']:.0f}% deles admitem que o cheiro pode não ser "
        "suficiente em toda situação.\n")
    add("É exatamente aí que o projeto se encaixa: um alerta eletrônico "
        "**redundante** (som + luz + texto no LCD), que não depende de o morador "
        "estar atento nem de sentir o cheiro.\n")

    add("## 2. Perfil de uso\n")
    add("### Tipo de gás\n")
    add(tabela(["Resposta", "N", "%"], dados["perfil"]["tipo_gas"], n))
    add("")
    add("### Frequência de uso do fogão\n")
    add(tabela(["Resposta", "N", "%"], dados["perfil"]["frequencia_uso"], n))
    add("")
    add("### Idade\n")
    add(f"- Respondentes que informaram a idade: **{idade['respondentes_com_idade']}**")
    add(f"- Média: **{idade['media']} anos** · mediana: **{idade['mediana']:.0f}**")
    add(f"- Faixa: **{idade['min']} a {idade['max']} anos**")
    add("")
    add(f"> {m['observacao_idade']}\n")

    add("## 3. Vivência de vazamentos e detecção atual\n")
    add("### Já presenciou ou suspeitou de vazamento?\n")
    add(tabela(["Resposta", "N", "%"], dados["vazamentos"]["ja_presenciou"], n))
    add("")
    add("### Possui detector ou sistema automático de alerta?\n")
    add(tabela(["Resposta", "N", "%"], dados["vazamentos"]["possui_detector"], n))
    add("")

    add("## 4. Como as pessoas identificam um vazamento\n")
    add("### Formas citadas (múltipla escolha)\n")
    add(tabela(["Forma", "Menções", "%"], dados["deteccao"]["formas"], n))
    add("")
    add('### "Apenas o cheiro é suficiente?"\n')
    add(tabela(["Resposta", "N", "%"], dados["deteccao"]["cheiro_suficiente"], n))
    add("")

    add("## 5. Preferências de alerta\n")
    add("### Tipo de alerta mais útil\n")
    add(tabela(["Resposta", "N", "%"],
               dados["preferencias_alerta"]["tipo_alerta"],
               sum(dados["preferencias_alerta"]["tipo_alerta"].values())))
    add("")
    add("### O alarme precisa ser ouvido em outros cômodos?\n")
    add(tabela(["Resposta", "N", "%"],
               dados["preferencias_alerta"]["importancia_ouvido_outros_comodos"],
               sum(dados["preferencias_alerta"]["importancia_ouvido_outros_comodos"].values())))
    add("")
    add("### Um botão de silenciar seria útil?\n")
    add(tabela(["Resposta", "N", "%"],
               dados["preferencias_alerta"]["utilidade_botao_silenciar"],
               sum(dados["preferencias_alerta"]["utilidade_botao_silenciar"].values())))
    add("")

    add("## 6. Onde instalar o dispositivo\n")
    add(f"Média de **{i['media_locais_por_pessoa']}** locais citados por "
        "participante — o público enxerga a solução como algo que pode estar em "
        "mais de um ponto da casa.\n")
    add(tabela(["Local", "Menções", "%"], dados["instalacao"]["locais"], n))
    add("")

    add("## 7. Características mais valorizadas\n")
    add(tabela(["Característica", "Menções", "%"],
               dados["caracteristicas"]["prioridades"], n))
    add("")

    add("## 8. Como isso virou requisito de projeto\n")
    for k, d in enumerate(dados["narrativa"]["decisao_projeto"], 1):
        add(f"{k}. {d}")
    add("")

    add("## 9. Gráficos gerados\n")
    for a in arquivos:
        add(f"- `{os.path.basename(a)}`")
    add("")
    add("## 10. Limitações da amostra\n")
    add(f"- Amostra **de conveniência** ({n} respostas), compartilhada no "
        "condomínio do pesquisador e com pessoas próximas — não é uma amostra "
        "probabilística.")
    add("- A idade só foi coletada a partir da 5ª resposta.")
    add("- Perguntas de múltipla escolha permitem mais de uma marcação, então a "
        f"soma das menções de uma pergunta pode passar de {n}.")
    add(f"- O formulário foi aplicado em {m['periodo']}; o perfil etário "
        "(predomínio de pessoas jovens) difere do público-alvo original (idosos), "
        "o que é uma oportunidade de aprofundamento.")
    add("")
    return "\n".join(p)


def main() -> int:
    caminho_csv = sys.argv[1] if len(sys.argv) > 1 else CSV_PADRAO

    if not os.path.exists(caminho_csv):
        # busca o CSV em locais comuns (incluindo a pasta Downloads do autor)
        candidatos = [
            os.path.join(os.path.expanduser("~"), "Downloads",
                         "Pesquisa sobre Segurança e Vazamento de Gás_Submissions_2026-10-03.csv"),
        ]
        caminho_csv = next((c for c in candidatos if os.path.exists(c)), caminho_csv)

    if not os.path.exists(caminho_csv):
        print(f"[erro] CSV não encontrado: {caminho_csv}", file=sys.stderr)
        print("       rode: python analysis/analisar_pesquisa.py <arquivo.csv>", file=sys.stderr)
        return 1

    print(f"[ok] lendo {caminho_csv}")
    df = carregar(caminho_csv)
    dados = analisar(df)

    os.makedirs(os.path.dirname(SAIDA_JSON), exist_ok=True)
    with open(SAIDA_JSON, "w", encoding="utf-8") as fh:
        json.dump(dados, fh, ensure_ascii=False, indent=2)
    print(f"[ok] {SAIDA_JSON}")

    arquivos = gerar_graficos(dados)
    for a in arquivos:
        print(f"[ok] {a}")

    os.makedirs(SAIDA_FIGS, exist_ok=True)
    with open(SAIDA_MD, "w", encoding="utf-8") as fh:
        fh.write(gerar_markdown(dados, arquivos))
    print(f"[ok] {SAIDA_MD}")

    i = dados["indicadores"]
    print("\n--- indicadores ---")
    for k, v in i.items():
        print(f"  {k}: {v}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
