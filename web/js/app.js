/* ==========================================================================
   Detector de vazamento de gás com MSP430 — comportamento da página
   1) Simulador do firmware (ADC 0–4095, limiar 1500, botão de silenciamento)
   2) Buzzer via Web Audio API, criado somente depois de um gesto do usuário
   3) Números da pesquisa: embutidos + refinamento opcional via survey.json
   4) Destaque de sintaxe do trecho de firmware
   Sem dependências externas. Nada de rede em file://.
   ========================================================================== */
(function () {
  'use strict';

  var LIMIAR = 1500;
  var MAX_ADC = 4095;
  var SEM_ANIMACAO = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  function $(sel, ctx) { return (ctx || document).querySelector(sel); }
  function $$(sel, ctx) { return Array.prototype.slice.call((ctx || document).querySelectorAll(sel)); }

  /* ======================================================================
     1. Números reais da pesquisa, embutidos como fallback
     Copiados de gas-leak-detector-msp430/data/survey.json.
     Em file:// o navegador bloqueia fetch, então estes valores são a fonte
     garantida — a página nunca fica vazia.
     ====================================================================== */
  var PESQUISA = {
    meta: {
      total_respostas: 21,
      plataforma: 'Tally Forms',
      periodo: '2026-05-10 a 2026-05-11',
      perguntas: 12
    },
    indicadores: {
      usam_botijao_pct: 85.7,
      usam_botijao_n: 18,
      nao_possui_detector_pct: 95.2,
      nao_possui_detector_n: 20,
      identificam_pelo_cheiro_pct: 100.0,
      identificam_pelo_cheiro_n: 21,
      cheiro_nao_basta_pct: 81.0,
      cheiro_nao_basta_n: 17,
      querem_ouvir_outros_comodos_pct: 95.2,
      querem_ouvir_outros_comodos_n: 20,
      botao_silenciar_util_pct: 71.4,
      botao_silenciar_util_n: 15,
      preferem_combinacao_alerta_pct: 61.9,
      preferem_combinacao_alerta_n: 13,
      ja_presenciou_vazamento_pct: 52.4,
      ja_presenciou_vazamento_n: 11,
      usam_varias_vezes_dia_pct: 85.7,
      usam_varias_vezes_dia_n: 18
    },
    instalacao: {
      locais: {
        'Na cozinha em geral': 13,
        'Próximo ao botijão': 8,
        'Próximo ao fogão': 6,
        'Em vários cômodos': 3,
        'Próximo ao teto': 2
      }
    },
    caracteristicas: {
      prioridades: {
        'Facilidade de instalação': 17,
        'Aviso sonoro': 15,
        'Baixo custo': 12,
        'Integração com celular': 12,
        'Simplicidade': 10,
        'Tamanho reduzido': 10,
        'Baixo consumo de energia': 10,
        'Aviso visual': 7
      }
    }
  };

  /* ======================================================================
     2. Buzzer (Web Audio API)
     O AudioContext só é criado dentro de um gesto do usuário; portanto o
     som nunca começa ao carregar a página.
     ====================================================================== */
  var Buzzer = (function () {
    var ctx = null;
    var osc = null;
    var amp = null;
    var beep = null;
    var tocando = false;
    var liberado = false;      /* só vira true dentro de um gesto do usuário */

    function criar() {
      if (ctx) { return ctx; }
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) { return null; }
      try { ctx = new AC(); } catch (e) { ctx = null; }
      return ctx;
    }

    /* Chamado apenas em manipuladores de clique/tecla/arraste. */
    function despertar() {
      var c = criar();
      if (!c) { return false; }
      liberado = true;
      if (c.state === 'suspended' && c.resume) { c.resume(); }
      return true;
    }

    function ligar() {
      /* Sem gesto do usuário, nem cria o contexto: silêncio absoluto. */
      if (!liberado) { return; }
      var c = criar();
      if (!c || tocando) { return; }
      if (c.state === 'suspended' && c.resume) { c.resume(); }
      osc = c.createOscillator();
      amp = c.createGain();
      osc.type = 'square';
      osc.frequency.value = 1000;          /* mesmo tom do firmware: 1 MHz / 1000 */
      amp.gain.value = 0;
      osc.connect(amp);
      amp.connect(c.destination);
      osc.start();
      tocando = true;

      /* Intermitente, como o alarme real: 0,3 s de tom a cada 0,6 s. */
      beep = window.setInterval(function () {
        if (!ctx || !amp) { return; }
        var t = ctx.currentTime;
        try {
          amp.gain.cancelScheduledValues(t);
          amp.gain.setValueAtTime(0.0001, t);
          amp.gain.linearRampToValueAtTime(0.055, t + 0.02);
          amp.gain.setValueAtTime(0.055, t + 0.24);
          amp.gain.linearRampToValueAtTime(0.0001, t + 0.3);
        } catch (e) { /* contexto fechado pelo navegador */ }
      }, 620);
    }

    function desligar() {
      if (beep) { window.clearInterval(beep); beep = null; }
      if (osc) {
        try { osc.stop(); } catch (e) { /* já parado */ }
        try { osc.disconnect(); } catch (e) { /* ignore */ }
        osc = null;
      }
      if (amp) { try { amp.disconnect(); } catch (e) { /* ignore */ } amp = null; }
      tocando = false;
    }

    return {
      despertar: despertar,
      ligar: ligar,
      desligar: desligar,
      estaTocando: function () { return tocando; }
    };
  }());

  /* ======================================================================
     3. Simulador
     ====================================================================== */
  var sim = {
    slider: $('#adc'),
    lcd: $('#lcd-sim'),
    led: $('#sim-led'),
    buzzer: $('#sim-buzzer'),
    leitura: $('#sim-leitura'),
    percentual: $('#sim-percentual'),
    decisao: $('#sim-decisao'),
    fill: $('#gauge-fill'),
    botao: $('#btn-mute'),
    status: $('#sim-status'),
    ajuda: $('#mute-ajuda'),
    mutado: false,
    estadoAtual: ''
  };

  function pad4(n) { return ('000' + n).slice(-4); }

  function estadoDe(valor, mutado) {
    if (valor > LIMIAR) { return mutado ? 'mutado' : 'alarme'; }
    return 'seguro';
  }

  var TEXTO_LINHA2 = {
    seguro: 'Ambiente Seguro',
    alarme: 'ALERTA DE GAS!',
    mutado: 'ALERTA (MUTADO)'
  };

  /* Escreve no visor respeitando as 16 colunas do módulo 16x2. */
  function escreverLcd(lcd, linha1, linha2, estado) {
    var celulas = $$('.lcd-row', lcd);
    var textos = [linha1, linha2];
    for (var i = 0; i < celulas.length; i++) {
      celulas[i].textContent = ((textos[i] || '') + '                ').slice(0, 16);
    }
    lcd.classList.toggle('is-alert', estado === 'alarme');
    lcd.setAttribute('data-estado', estado || 'seguro');
  }

  function renderSimulador() {
    if (!sim.slider) { return; }
    var valor = parseInt(sim.slider.value, 10);
    if (isNaN(valor)) { valor = 0; }

    /* Regra do firmware: abaixo do limiar o silenciamento é rearmado. */
    if (valor <= LIMIAR) { sim.mutado = false; }
    var estado = estadoDe(valor, sim.mutado);
    var alarme = estado !== 'seguro';

    escreverLcd(sim.lcd, 'Nivel Gas: ' + pad4(valor), TEXTO_LINHA2[estado], estado);
    var linha2 = $$('.lcd-row', sim.lcd)[1];
    if (linha2) { linha2.classList.toggle('is-blinking', estado === 'alarme' && !SEM_ANIMACAO); }

    var pct = valor / LIMIAR * 100;

    if (sim.leitura) { sim.leitura.textContent = pad4(valor); }
    if (sim.percentual) { sim.percentual.textContent = (pct < 10 ? pct.toFixed(1) : String(Math.round(pct))) + '%'; }
    if (sim.decisao) { sim.decisao.textContent = alarme ? 'acima do limiar' : 'abaixo do limiar'; }
    $$('.readout[data-alerta]').forEach(function (el) {
      el.classList.toggle('is-alert', alarme);
    });

    if (sim.fill) {
      sim.fill.style.width = (Math.min(valor / MAX_ADC, 1) * 100) + '%';
      sim.fill.classList.toggle('is-alert', alarme);
    }
    sim.slider.style.setProperty('--fill', (valor / MAX_ADC * 100) + '%');
    sim.slider.setAttribute('aria-valuetext',
      pad4(valor) + ' de ' + MAX_ADC + ', ' + (alarme ? 'acima' : 'abaixo') + ' do limiar de ' + LIMIAR);

    /* LED1 segue o risco, não o som: fica aceso também no alarme mutado. */
    if (sim.led) {
      sim.led.classList.toggle('is-on', estado === 'alarme' || estado === 'mutado');
      sim.led.classList.toggle('is-safe', estado === 'seguro');
    }
    if (sim.buzzer) { sim.buzzer.classList.toggle('is-on', estado === 'alarme'); }

    /* Rótulos em texto: a cor não é a única pista do estado. */
    var ledTexto = $('#sim-led-estado');
    if (ledTexto) { ledTexto.textContent = estado === 'seguro' ? 'apagado' : 'aceso'; }
    var buzTexto = $('#sim-buzzer-estado');
    if (buzTexto) {
      buzTexto.textContent = estado === 'alarme' ? 'soando' : (estado === 'mutado' ? 'silenciado' : 'mudo');
    }

    /* Som: só depois de um gesto e apenas quando o alarme está de fato ativo. */
    if (estado === 'alarme') { Buzzer.ligar(); } else { Buzzer.desligar(); }

    /* Botão de silenciar */
    if (sim.botao) {
      sim.botao.setAttribute('aria-pressed', estado === 'mutado' ? 'true' : 'false');
      sim.botao.disabled = estado === 'seguro';
      sim.botao.textContent = estado === 'mutado' ? 'Buzzer silenciado' : 'Silenciar buzzer';
    }
    if (sim.ajuda) {
      sim.ajuda.textContent = estado === 'mutado'
        ? 'Silenciado até a leitura voltar para 1500 ou menos — aí o alarme se rearma sozinho.'
        : 'O botão só silencia; quem rearma o alarme é a própria leitura cair abaixo de 1500.';
    }

    /* Região viva: anuncia apenas quando o estado muda. */
    if (sim.status && estado !== sim.estadoAtual) {
      sim.estadoAtual = estado;
      var frases = {
        seguro: 'Leitura ' + pad4(valor) + ': ambiente seguro, buzzer desligado.',
        alarme: 'Leitura ' + pad4(valor) + ': alerta de gás, buzzer tocando.',
        mutado: 'Leitura ' + pad4(valor) + ': alerta de gás com buzzer silenciado.'
      };
      sim.status.textContent = frases[estado];
    }
  }

  function ligarControlesSimulador() {
    if (!sim.slider) { return; }

    var painel = sim.slider.closest('.sim-panel') || document;

    /* Qualquer gesto no painel libera o áudio do navegador (sem som até aqui). */
    ['pointerdown', 'touchstart', 'keydown'].forEach(function (evt) {
      painel.addEventListener(evt, function () { Buzzer.despertar(); }, { passive: true });
    });

    sim.slider.addEventListener('input', function () {
      Buzzer.despertar();
      renderSimulador();
    });

    if (sim.botao) {
      sim.botao.addEventListener('click', function () {
        Buzzer.despertar();
        var valor = parseInt(sim.slider.value, 10) || 0;
        if (valor > LIMIAR) { sim.mutado = true; }
        renderSimulador();
      });
    }

    $$('.scenario').forEach(function (botao) {
      botao.addEventListener('click', function () {
        Buzzer.despertar();
        sim.slider.value = botao.getAttribute('data-value');
        renderSimulador();
        sim.slider.focus();
      });
    });

    /* Aba em segundo plano: corta o som. */
    document.addEventListener('visibilitychange', function () {
      if (document.hidden) { Buzzer.desligar(); } else { renderSimulador(); }
    });

    renderSimulador();
  }

  /* ======================================================================
     4. Demonstração do hero (sem áudio, em laço)
     ====================================================================== */
  var HERO_CENAS = [
    ['Nivel Gas: 0312', 'Ambiente Seguro', 'seguro'],
    ['Nivel Gas: 0987', 'Ambiente Seguro', 'seguro'],
    ['Nivel Gas: 1204', 'Ambiente Seguro', 'seguro'],
    ['Nivel Gas: 1583', 'ALERTA DE GAS!', 'alarme'],
    ['Nivel Gas: 2410', 'ALERTA DE GAS!', 'alarme'],
    ['Nivel Gas: 1140', 'Ambiente Seguro', 'seguro']
  ];

  function ligarHero() {
    var lcd = $('#lcd-hero');
    if (!lcd) { return; }
    var led = $('#hero-led');
    var buzzer = $('#hero-buzzer');
    var linha2 = $$('.lcd-row', lcd)[1];

    function pintar(cena) {
      escreverLcd(lcd, cena[0], cena[1], cena[2]);
      linha2.classList.toggle('is-blinking', cena[2] === 'alarme' && !SEM_ANIMACAO);
      if (led) {
        led.classList.toggle('is-on', cena[2] === 'alarme');
        led.classList.toggle('is-safe', cena[2] === 'seguro');
      }
      if (buzzer) { buzzer.classList.toggle('is-on', cena[2] === 'alarme'); }
    }

    if (SEM_ANIMACAO) {
      /* Sem animação: mostra o quadro mais informativo da sequência, parado. */
      pintar(HERO_CENAS[4]);
      return;
    }

    /* Uma única sequência de abertura: o auto-teste do aparelho. */
    var BOOT = [
      ['Detector de Gas', 'MSP430F5529LP', 'seguro'],
      ['Aquecendo MQ-2', 'aguarde 24 h...', 'seguro']
    ];
    var indice = 0;
    var fase = 'boot';
    var timer = null;

    function passo() {
      if (fase === 'boot') {
        pintar([BOOT[indice][0], BOOT[indice][1], BOOT[indice][2]]);
        indice += 1;
        if (indice >= BOOT.length) { fase = 'demo'; indice = 0; }
        timer = window.setTimeout(passo, 900);
        return;
      }
      pintar(HERO_CENAS[indice]);
      indice = (indice + 1) % HERO_CENAS.length;
      timer = window.setTimeout(passo, 1700);
    }

    passo();

    /* Pausa quando o hero sai da tela. */
    if ('IntersectionObserver' in window) {
      new IntersectionObserver(function (entradas) {
        entradas.forEach(function (e) {
          if (e.isIntersecting) {
            if (!timer) { timer = window.setTimeout(passo, 200); }
          } else if (timer) {
            window.clearTimeout(timer);
            timer = null;
          }
        });
      }, { threshold: 0.15 }).observe($('.hero') || lcd);
    }
  }

  /* ======================================================================
     5. Pesquisa de campo
     ====================================================================== */
  /* Percentuais arredondados, como no relatório (85,7% -> 86%). */
  function pct(v) { return Math.round(parseFloat(v)) + '%'; }

  var CAMPOS = {
    total_respostas: function (d) { return String(d.meta.total_respostas); },
    total_periodo: function (d) { return d.meta.plataforma + ' · ' + d.meta.periodo; },
    usam_botijao: function (d) { return pct(d.indicadores.usam_botijao_pct); },
    usam_botijao_n: function (d) { return d.indicadores.usam_botijao_n + ' de ' + d.meta.total_respostas + ' respostas'; },
    cheiro: function (d) { return pct(d.indicadores.identificam_pelo_cheiro_pct); },
    cheiro_n: function (d) { return d.indicadores.identificam_pelo_cheiro_n + ' de ' + d.meta.total_respostas + ' citaram o cheiro'; },
    sem_detector: function (d) { return pct(d.indicadores.nao_possui_detector_pct); },
    sem_detector_n: function (d) { return d.indicadores.nao_possui_detector_n + ' de ' + d.meta.total_respostas + ' não têm nenhum detector'; },
    cheiro_nao_basta: function (d) { return pct(d.indicadores.cheiro_nao_basta_pct); },
    cheiro_nao_basta_n: function (d) { return d.indicadores.cheiro_nao_basta_n + ' de ' + d.meta.total_respostas + ' dizem que depende ou não basta'; },
    outros_comodos: function (d) { return pct(d.indicadores.querem_ouvir_outros_comodos_pct); },
    outros_comodos_n: function (d) { return d.indicadores.querem_ouvir_outros_comodos_n + ' de ' + d.meta.total_respostas + ' querem ouvir de outros cômodos'; },
    silenciar: function (d) { return pct(d.indicadores.botao_silenciar_util_pct); },
    silenciar_n: function (d) { return d.indicadores.botao_silenciar_util_n + ' de ' + d.meta.total_respostas + ' acharam o botão útil'; },
    combinacao: function (d) { return pct(d.indicadores.preferem_combinacao_alerta_pct); },
    combinacao_n: function (d) { return d.indicadores.preferem_combinacao_alerta_n + ' de ' + d.meta.total_respostas + ' preferem combinação de alertas'; }
  };

  function aplicarPesquisa(d) {
    if (!d || !d.indicadores) { return; }

    $$('[data-stat]').forEach(function (el) {
      var fn = CAMPOS[el.getAttribute('data-stat')];
      if (fn) { el.textContent = fn(d); }
    });

    var locais = (d.instalacao && d.instalacao.locais) || {};
    $$('[data-local]').forEach(function (linha) {
      var n = locais[linha.getAttribute('data-local')];
      if (typeof n !== 'number') { return; }
      var celulas = $$('td', linha);
      if (celulas[1]) { celulas[1].textContent = String(n); }
      if (celulas[2]) { celulas[2].textContent = Math.round(n / d.meta.total_respostas * 100) + '%'; }
    });

    var carac = (d.caracteristicas && d.caracteristicas.prioridades) || {};
    $$('[data-carac]').forEach(function (linha) {
      var n = carac[linha.getAttribute('data-carac')];
      if (typeof n !== 'number') { return; }
      var celulas = $$('td', linha);
      if (celulas[1]) { celulas[1].textContent = String(n); }
      if (celulas[2]) { celulas[2].textContent = Math.round(n / d.meta.total_respostas * 100) + '%'; }
    });
  }

  function marcarFonte(texto) {
    var el = $('#fonte-dados');
    if (el) { el.textContent = texto; }
  }

  function carregarPesquisa() {
    /* Primeiro os números embutidos: a seção nunca depende da rede. */
    aplicarPesquisa(PESQUISA);
    marcarFonte('Números embutidos na página.');

    /* Depois, se a página for servida por http(s), tenta ler o survey.json.
       Em file:// o fetch é bloqueado pelo navegador — e nem é tentado, para
       não sujar o console. */
    if (!/^https?:$/.test(window.location.protocol) || typeof window.fetch !== 'function') { return; }

    window.fetch('../data/survey.json', { cache: 'no-cache' })
      .then(function (resp) {
        if (!resp.ok) { throw new Error('HTTP ' + resp.status); }
        return resp.json();
      })
      .then(function (d) {
        aplicarPesquisa(d);
        marcarFonte('Números lidos de ../data/survey.json.');
      })
      .catch(function () {
        marcarFonte('Números embutidos na página (survey.json indisponível).');
      });
  }

  /* ======================================================================
     6. Destaque de sintaxe do firmware (sem biblioteca)
     ====================================================================== */
  function realcarCodigo() {
    var alvo = $('#firmware');
    if (!alvo) { return; }
    var fonte = alvo.textContent;

    var padrao = new RegExp([
      '(\\/\\*[\\s\\S]*?\\*\\/|\\/\\/[^\\n]*)',            /* 1 comentário   */
      '("(?:[^"\\\\]|\\\\.)*")',                           /* 2 string       */
      '(^[ \\t]*#[^\\n]*)',                                /* 3 pré-processador */
      '(\\b(?:include|define|pragma|if|else|for|while|switch|case|break|default|do|continue|return|sizeof|static|const|void|volatile|uint8_t|uint16_t|uint32_t|int8_t|int16_t|int|char)\\b)',
      '(\\b(?:0[xX][0-9a-fA-F]+|\\d+)[uUlL]*\\b)',         /* 5 número       */
      '(\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\())'                 /* 6 chamada      */
    ].join('|'), 'gm');

    function esc(s) {
      return s.replace(/[&<>]/g, function (c) {
        return c === '&' ? '&amp;' : c === '<' ? '&lt;' : '&gt;';
      });
    }

    alvo.innerHTML = fonte.replace(padrao, function (m, com, str, pre, kw, num, fn) {
      if (com) { return '<span class="tok-com">' + esc(com) + '</span>'; }
      if (str) { return '<span class="tok-str">' + esc(str) + '</span>'; }
      if (pre) { return '<span class="tok-pre">' + esc(pre) + '</span>'; }
      if (kw) { return '<span class="tok-kw">' + esc(kw) + '</span>'; }
      if (num) { return '<span class="tok-num">' + esc(num) + '</span>'; }
      if (fn) { return '<span class="tok-fn">' + esc(fn) + '</span>'; }
      return esc(m);
    });
  }

  /* ======================================================================
     7. Navegação: marca a seção visível
     ====================================================================== */
  function ligarNavegacao() {
    var links = $$('.topnav a[href^="#"]');
    if (!links.length || !('IntersectionObserver' in window)) { return; }

    var mapa = {};
    links.forEach(function (link) {
      var secao = document.getElementById(link.getAttribute('href').slice(1));
      if (secao) { mapa[secao.id] = link; }
    });

    var observador = new IntersectionObserver(function (entradas) {
      entradas.forEach(function (e) {
        var link = mapa[e.target.id];
        if (!link) { return; }
        if (e.isIntersecting) {
          links.forEach(function (l) { l.removeAttribute('aria-current'); });
          link.setAttribute('aria-current', 'true');
        }
      });
    }, { rootMargin: '-20% 0px -70% 0px' });

    Object.keys(mapa).forEach(function (id) { observador.observe(document.getElementById(id)); });
  }

  /* ====================================================================== */
  function iniciar() {
    ligarHero();
    ligarControlesSimulador();
    carregarPesquisa();
    realcarCodigo();
    ligarNavegacao();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', iniciar);
  } else {
    iniciar();
  }
}());
