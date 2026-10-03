# Dados

## `pesquisa_vazamento_gas.csv`

Respostas do formulário digital (Tally Forms) aplicado em **10 e 11 de maio de
2026** com moradores de um condomínio e público externo.

| | |
| :--- | :--- |
| Respostas | **21** |
| Perguntas | 12 |
| Colunas no arquivo | 35 (pergunta principal + sub-opções de múltipla escolha) |

### Sobre privacidade

O export original do Tally traz três colunas de controle que **não** fazem parte
da pesquisa: `Submission ID`, `Respondent ID` e `Submitted at` (data/hora exata
do envio). Elas foram **removidas** antes de publicar este arquivo, e as colunas
restantes foram renomeadas para um esquema curto e estável
(`q01_tipo_gas`, `q05_det_cheiro`, ...).

O que isso significa na prática:

- **Nenhum identificador de participante** está neste repositório.
- **Nenhuma data/hora individual** está neste repositório — só o período agregado.
- Todas as **respostas** foram preservadas integralmente, então a análise é
  reproduzível e idêntica à que gerou os gráficos do projeto.
- O formulário não coletou nome, e-mail, telefone ou endereço. A idade é a única
  informação pessoal, e aparece apenas agregada (média, mediana e faixa).

A anonimização é reprodutível pelo script
[`../analysis/anonimizar_csv.py`](../analysis/anonimizar_csv.py), que documenta
exatamente o que é descartado.

## `survey.json`

Dados agregados em formato consumível por programas e pelo site. É gerado pelo
script [`../analysis/analisar_pesquisa.py`](../analysis/analisar_pesquisa.py) e
usado pela página em [`../web/index.html`](../web/index.html).

Estrutura:

| Chave | Conteúdo |
| :--- | :--- |
| `meta` | Total de respostas, plataforma, período, número de perguntas |
| `perfil` | Tipo de gás, frequência de uso, faixa etária |
| `vazamentos` | Vivência prévia de vazamento e posse de detector |
| `deteccao` | Formas de identificação e suficiência do cheiro |
| `preferencias_alerta` | Tipo de alerta, audibilidade, botão de silenciamento |
| `instalacao` | Locais sugeridos |
| `caracteristicas` | Características valorizadas |
| `indicadores` | Percentuais consolidados usados no README e no site |
| `narrativa` | O relato que originou o projeto e como os dados viraram requisitos |

### Reprodutibilidade

```bash
# a partir da raiz do repositório
python analysis/analisar_pesquisa.py
```

O script lê o CSV, recalcula todos os indicadores e reescreve
`data/survey.json`, os gráficos em `analysis/out/` e o relatório
`analysis/out/analise.md`. Nada é digitado à mão — se o CSV mudar, tudo é
recalculado.
