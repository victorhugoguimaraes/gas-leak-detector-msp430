# Análise

Dois scripts, sem dependências além do Python padrão de ciência de dados
(`pandas`, `numpy`, `matplotlib`).

## `anonimizar_csv.py`

Prepara o CSV público a partir do export bruto do formulário.

O export do Tally traz `Submission ID`, `Respondent ID` e `Submitted at`. Essas
colunas não participam da análise e expõem metadados dos participantes, então
são descartadas antes de qualquer coisa entrar no repositório. As colunas
restantes são renomeadas para um esquema curto e estável
(`q01_tipo_gas`, `q05_det_cheiro`, `q11_local_cozinha`, ...).

```bash
python analysis/anonimizar_csv.py caminho/para/export_bruto.csv
# gera data/pesquisa_vazamento_gas.csv
```

## `analisar_pesquisa.py`

Lê o CSV (anonimizado ou o export bruto — os dois funcionam) e gera tudo o que
o projeto publica:

| Saída | Conteúdo |
| :--- | :--- |
| `data/survey.json` | Indicadores agregados, consumidos pelo site e pelo README |
| `analysis/out/figura-01..09-*.png` | Nove gráficos em 300 dpi, na paleta do projeto |
| `analysis/out/analise.md` | Relatório da análise com todas as tabelas |

```bash
python analysis/analisar_pesquisa.py
```

### Princípio: nenhum número na mão

Todos os valores citados no README, no site e na documentação saem deste
script. Se o CSV mudar, rodar o script de novo recalcula tudo — gráficos
inclusive. Não existe planilha intermediária nem valor copiado manualmente.

### Robustez

O script aceita os dois esquemas de nome de coluna e não depende de acentuação
(a comparação de rótulos é feita sobre texto normalizado), então continua
funcionando se o formulário for editado ou reexportado com pequenas diferenças
de redação nas perguntas.

### Verificando a consistência

Os indicadores calculados devem reproduzir os gráficos originais do relatório.
Dois pontos de checagem úteis:

- **Tipo de gás:** 18 de 21 = 86% usam botijão — bate com a Figura 2.1 do
  relatório original.
- **Frequência de uso:** 18 de 21 = 86% usam o fogão várias vezes ao dia — bate
  com a Figura 2.2.

Se algum desses divergir, ou o CSV mudou ou a leitura das colunas precisa de
ajuste.
