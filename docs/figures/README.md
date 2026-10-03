# Figuras do projeto

Esta pasta guarda as imagens usadas na documentação e no relatório.

```
figures/
├── original/   # 11 figuras extraídas do PDF original do relatório (Seção 2.3)
└── results/    # 9 gráficos analíticos gerados por analysis/analisar_pesquisa.py
```

---

## 1. Figuras originais do relatório (`figures/original/`)

As figuras abaixo foram extraídas **diretamente do PDF original** do relatório acadêmico. Cada
arquivo corresponde a um *XObject* de imagem do PDF e recebeu o número da figura que aparece no
texto do relatório. A coluna “Objeto no PDF” é o identificador interno do objeto de imagem no
arquivo original — útil para rastrear a origem caso o PDF seja reextraído.

| Arquivo | Figura | Objeto no PDF | Pergunta do formulário / conteúdo | Legenda no relatório |
| --- | --- | --- | --- | --- |
| `figura-2.1.png` | Figura 2.1 | `obj220` | Tipo de gás (botijão × encanado) | Tipo de gás utilizado pelos participantes. |
| `figura-2.2.png` | Figura 2.2 | `obj222` | Frequência de uso do fogão | Frequência de uso do fogão a gás. |
| `figura-2.3.png` | Figura 2.3 | `obj239` | Experiência prévia com vazamento | Experiência prévia com suspeita ou vazamento de gás. |
| `figura-2.4.png` | Figura 2.4 | `obj240` | Uso de detector automático | Uso de detector ou sistema automático de alerta. |
| `figura-2.5.png` | Figura 2.5 | `obj249` | Formas de identificação | Formas de identificação de possível vazamento. |
| `figura-2.6.png` | Figura 2.6 | `obj250` | Suficiência do cheiro | Percepção sobre a suficiência do cheiro para detectar vazamentos. |
| `figura-2.7.png` | Figura 2.7 | `obj259` | Tipos de alerta | Tipos de alerta considerados mais úteis. |
| `figura-2.8.png` | Figura 2.8 | `obj260` | Importância de ouvir em outros cômodos | Importância de o alarme ser ouvido em outros cômodos. |
| `figura-2.9.png` | Figura 2.9 | `obj269` | Botão de silenciar | Utilidade de um botão para silenciar temporariamente o alarme. |
| `figura-2.10.png` | Figura 2.10 | `obj270` | Locais sugeridos de instalação | Locais sugeridos para instalação do dispositivo. |
| `figura-2.11.png` | Figura 2.11 | `obj279` | Características valorizadas | Características mais valorizadas em uma solução. |

### Sobre `capa-logo-unb.png`

O arquivo `capa-logo-unb.png` é **a mesma imagem** de `figura-2.1.png` (mesmo objeto `obj220`, mesmo
tamanho de 38.904 bytes, byte a byte idêntico). Isso acontece porque o PDF original reutilizou o
mesmo recurso de imagem na capa e no corpo do texto: o gráfico de “Tipo de gás” acabou aparecendo
também na página de capa, no lugar do logotipo institucional. A imagem foi salva duas vezes — com o
nome descritivo da capa e com o nome da figura — para que cada referência do relatório continue
funcionando sem ambiguidade. **Não se trata de um logotipo da UnB**: é o gráfico da Figura 2.1.

> Observação: as figuras estão no formato PNG tal como extraídas do PDF (sem recompressão), por isso
> os tamanhos em bytes coincidem com os objetos originais.

---

## 2. Gráficos analíticos (`figures/results/`)

Os nove gráficos abaixo foram gerados por
[`analysis/analisar_pesquisa.py`](../../analysis/analisar_pesquisa.py) a partir de
[`data/survey.json`](../../data/survey.json) e são a versão “de portfólio” dos resultados da pesquisa
de campo — mais legíveis e com os percentuais calculados. Os originais ficam em
`analysis/out/`; as cópias em `figures/results/` existem para que a documentação seja autossuficiente.

| Arquivo | Conteúdo | Fonte em `analysis/out/` |
| --- | --- | --- |
| `figura-01-tipo-de-gas.png` | Tipo de gás utilizado (botijão × encanado) | `figura-01-tipo-de-gas.png` |
| `figura-02-frequencia-de-uso.png` | Frequência de uso do fogão | `figura-02-frequencia-de-uso.png` |
| `figura-03-vazamento-e-detector.png` | Já presenciou vazamento × possui detector | `figura-03-vazamento-e-detector.png` |
| `figura-04-formas-de-deteccao.png` | Formas de identificação citadas (múltipla escolha) | `figura-04-formas-de-deteccao.png` |
| `figura-05-cheiro-suficiente.png` | “Apenas o cheiro é suficiente?” | `figura-05-cheiro-suficiente.png` |
| `figura-06-tipo-de-alerta.png` | Tipo de alerta considerado mais útil | `figura-06-tipo-de-alerta.png` |
| `figura-07-alerta-e-silenciamento.png` | Ouvir em outros cômodos × utilidade do botão | `figura-07-alerta-e-silenciamento.png` |
| `figura-08-locais-de-instalacao.png` | Locais sugeridos de instalação | `figura-08-locais-de-instalacao.png` |
| `figura-09-caracteristicas-desejadas.png` | Características mais valorizadas | `figura-09-caracteristicas-desejadas.png` |

Os números por trás de cada gráfico estão em [docs/pesquisa.md](../pesquisa.md) e no relatório de
análise [`analysis/out/analise.md`](../../analysis/out/analise.md).

---

## 3. Como regerar as figuras

| Etapa | Comando |
| --- | --- |
| Regenerar os gráficos analíticos | `python analysis/analisar_pesquisa.py` |
| Reextrair as figuras do PDF original | `python analysis/extrair_figuras_pdf.py report/relatorio-original.pdf` |

As figuras de `original/` foram extraídas do PDF preservado em
[`../../report/relatorio-original.pdf`](../../report/relatorio-original.pdf).
O PDF não traz as figuras como imagens separadas: cada uma é um stream de bitmap
cru comprimido com `FlateDecode`, dentro de um objeto `/XObject /Subtype /Image`,
com máscara de transparência em DeviceGray. O script
[`../../analysis/extrair_figuras_pdf.py`](../../analysis/extrair_figuras_pdf.py)
descomprime e remonta os PNGs com Pillow. Ele foi validado para ser
**bit a bit idêntico** ao que está versionado aqui, então rodar de novo não
altera nada — só é necessário se o PDF de origem for trocado.

O relatório em LaTeX (`report/relatorio.tex`) consome **apenas** as figuras de
`figures/original/`, garantindo que o PDF recompilado reproduza fielmente as figuras do documento
acadêmico original.
