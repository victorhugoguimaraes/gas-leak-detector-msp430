#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extração das figuras embutidas no PDF do relatório
==================================================

O PDF acadêmico original não tem as figuras como imagens separadas: cada uma
está embutida como um stream de bitmap cru comprimido com FlateDecode, dentro
de um objeto `/XObject /Subtype /Image`, com uma máscara de transparência
(`/SMask`) em DeviceGray. Este script localiza cada par, descomprime e remonta
os PNGs usados em `docs/figures/original/`.

As figuras já extraídas estão versionadas no repositório — este script só é
necessário se o PDF de origem for substituído por outro.

Uso:
    python analysis/extrair_figuras_pdf.py caminho/do/relatorio.pdf

Requer: Pillow (já disponível no ambiente do harness).
"""

import os
import re
import sys
import zlib

from PIL import Image

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DESTINO = os.path.join(RAIZ, "docs", "figures", "original")

# Mapa: figura do relatório -> objeto do PDF que contém o bitmap em cores.
# Levantado inspecionando os streams de conteúdo de cada página, que referenciam
# /Im1..Im12 na ordem em que as figuras aparecem no documento.
FIGURAS = {
    "figura-2.1.png": 220,
    "figura-2.2.png": 222,
    "figura-2.3.png": 239,
    "figura-2.4.png": 240,
    "figura-2.5.png": 249,
    "figura-2.6.png": 250,
    "figura-2.7.png": 259,
    "figura-2.8.png": 260,
    "figura-2.9.png": 269,
    "figura-2.10.png": 270,
    "figura-2.11.png": 279,
}

# A capa do PDF reutiliza o mesmo recurso de imagem da figura 2.1, então o
# gráfico aparece também como "logotipo" da capa. Guardamos uma cópia separada
# para deixar claro que é a mesma imagem, e não uma figura adicional.
COPIA_CAPA = ("capa-logo-unb.png", 220)


def objetos(pdf: bytes) -> dict:
    """Mapeia numero do objeto -> corpo bruto (dicionario + stream)."""
    objs = {}
    for m in re.finditer(rb"(\d+)\s+0\s+obj", pdf):
        num = int(m.group(1))
        inicio = m.end()
        fim = pdf.find(b"endobj", inicio)
        objs[num] = pdf[inicio:fim if fim != -1 else len(pdf)]
    return objs


def imagem(objs: dict, num: int):
    """Descomprime um XObject de imagem e devolve um PIL.Image, ou None."""
    corpo = objs.get(num)
    if not corpo:
        return None
    cabecalho = corpo[: corpo.find(b"stream")] if b"stream" in corpo else corpo
    if b"/Image" not in cabecalho:
        return None

    sm = re.search(rb"stream\r?\n", corpo)
    if not sm:
        return None
    fim = corpo.find(b"endstream", sm.end())
    bruto = corpo[sm.end():fim]
    if b"FlateDecode" not in cabecalho:
        return None
    try:
        dec = zlib.decompress(bruto)
    except zlib.error:
        return None

    largura = int(re.search(rb"/Width\s+(\d+)", cabecalho).group(1))
    altura = int(re.search(rb"/Height\s+(\d+)", cabecalho).group(1))
    cs = re.search(rb"/ColorSpace\s*/(\w+)", cabecalho).group(1)
    bits = int(re.search(rb"/BitsPerComponent\s+(\d+)", cabecalho).group(1))

    modo = "RGB" if cs == b"DeviceRGB" else "L"
    canais = 3 if modo == "RGB" else 1
    necessario = largura * altura * canais * (bits // 8)
    if len(dec) < necessario:
        return None
    return Image.frombytes(modo, (largura, altura), dec[:necessario])


def main() -> int:
    if len(sys.argv) < 2:
        print("uso: python analysis/extrair_figuras_pdf.py <relatorio.pdf>", file=sys.stderr)
        return 1

    caminho = sys.argv[1]
    if not os.path.exists(caminho):
        print(f"[erro] PDF não encontrado: {caminho}", file=sys.stderr)
        return 1

    pdf = open(caminho, "rb").read()
    objs = objetos(pdf)
    os.makedirs(DESTINO, exist_ok=True)

    gerados = 0
    faltando = []
    for nome, num in FIGURAS.items():
        img = imagem(objs, num)
        if img is None:
            faltando.append(f"{nome} (obj {num})")
            continue
        img.save(os.path.join(DESTINO, nome))
        print(f"[ok] {nome} <- obj {num} ({img.width}x{img.height})")
        gerados += 1

    nome, num = COPIA_CAPA
    img = imagem(objs, num)
    if img is not None:
        img.save(os.path.join(DESTINO, nome))
        print(f"[ok] {nome} <- obj {num} (mesma imagem da figura 2.1)")
        gerados += 1

    print(f"\n{gerados} imagens geradas em {DESTINO}")
    if faltando:
        print(f"[aviso] não extraídas: {', '.join(faltando)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
