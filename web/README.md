# Página do projeto — detector de vazamento de gás com MSP430

Página de apresentação de um arquivo só de conteúdo: `index.html` com o CSS em
`css/estilo.css` e o JavaScript em `js/app.js`. Nada de build, nada de CDN, nada
de framework. Abrir `index.html` com duplo clique já funciona (protocolo `file://`).

## O que tem aqui

| Arquivo | O que é |
| --- | --- |
| `index.html` | a página inteira: hero, simulador, como funciona, hardware, pesquisa, código e limitações |
| `css/estilo.css` | paleta industrial escura, tipografia e todos os componentes |
| `js/app.js` | simulador do firmware, buzzer (Web Audio API), números da pesquisa e destaque de sintaxe |
| `assets/img/` | os 9 gráficos da pesquisa e o logo da UnB |

## Como colocar no site pessoal

**1. Copiar a pasta (mais simples).** Jogue a pasta `web/` inteira no seu site e
linke a página:

```html
<a href="/projetos/detector-gas/index.html">Detector de vazamento de gás com MSP430</a>
```

**2. Embutir com iframe** (útil para manter a página isolada dentro de outro
layout). A altura sugerida é grande porque a página é longa:

```html
<iframe src="/projetos/detector-gas/index.html" title="Detector de vazamento de gás com MSP430"
        style="width:100%;height:1400px;border:0" loading="lazy"></iframe>
```

**3. Colar o conteúdo** dentro de uma página existente: copie o `<main>` do
`index.html` para o seu template e mantenha as referências a `css/estilo.css` e
`js/app.js` (ou cole os dois inline em `<style>` e `<script>`). Os caminhos das
imagens são relativos à pasta `web/`.

### O que precisa ir junto

- `assets/img/` — sem os PNGs a galeria da pesquisa fica quebrada.
- `../data/survey.json` — opcional. Se os números não puderem viajar junto, nada
  quebra: a página traz os mesmos números embutidos no `js/app.js`
  (`PESQUISA`, copiado de `data/survey.json`).

### Um ajuste ao publicar

1. **Pinagem.** A tabela de hardware foi montada a partir do protótipo. Se o
   firmware final mudar de pino, ajuste a tabela em `index.html` para não
   divergir do relatório.

O link do repositório no rodapé já aponta para
`https://github.com/victorhugoguimaraes/gas-leak-detector-msp430`. Se o endereço
mudar, troque o `href` do link “Código, relatório e dados da pesquisa”.

## Detalhes que valem saber

- **Números da pesquisa:** aparecem sempre. O JavaScript aplica primeiro os
  valores embutidos e só tenta `fetch('../data/survey.json')` quando a página é
  servida por `http(s)`. Em `file://` o navegador bloqueia o `fetch`, então ele
  nem é tentado — evitando erro no console.
- **Som:** o `AudioContext` só é criado no primeiro clique/tecla dentro do painel
  do simulador. Ao carregar a página o silêncio é absoluto; se a leitura passar de
  1500, o buzzer toca em ~1 kHz (o mesmo tom do Timer A2 no P2.4) de forma
  intermitente, e o botão “Silenciar buzzer”
  corta o som.
- **Acessibilidade:** slider e botão operáveis por teclado, foco visível, região
  viva anunciando a mudança de estado e `prefers-reduced-motion` respeitado (a
  demonstração automática do hero para de piscar e mostra um quadro fixo).

## Como conferir rápido

1. Abra `index.html` no navegador e olhe o console: deve estar vazio.
2. Mova o slider até passar de 1500: o LCD muda para “ALERTA DE GAS!”, o LED
   acende e o buzzer toca (depois do seu clique).
3. Clique em “Silenciar buzzer”: o som para e a linha vira “ALERTA (MUTADO)”.
4. Volte o slider para menos de 1500: volta a “Ambiente Seguro” e o botão rearma.
