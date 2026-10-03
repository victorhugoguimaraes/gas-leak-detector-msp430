# Lista de materiais (BOM)

Bill of materials do protótipo **Detector de Vazamento de Gás com MSP430**, com os custos estimados no
relatório acadêmico (Tabela 3.1). Valores em reais (R$), referência de 2026, sujeitos a variação de fornecedor.

- Arquitetura e pinagem: [`docs/engenharia.md`](../docs/engenharia.md)
- Diagrama de ligação: [`hardware/diagrama.svg`](diagrama.svg)
- Relatório acadêmico: [`docs/relatorio.md`](../docs/relatorio.md)

## 1. Itens principais

| # | Componente | Função no projeto | Qtd. | Custo unitário | Custo total |
| --- | --- | --- | --- | --- | --- |
| 1 | **LaunchPad MSP430F5529LP** | Microcontrolador principal, leitura do ADC e controle dos alertas | 1 | R$ 150,00 | R$ 150,00 |
| 2 | **Sensor de gás MQ-2** (módulo com saída analógica) | Detecção de gás inflamável por saída analógica | 1 | R$ 15,00 a R$ 25,00 | R$ 15,00 a R$ 25,00 |
| 3 | **Buzzer** ativo ou passivo | Alerta sonoro | 1 | R$ 5,00 a R$ 10,00 | R$ 5,00 a R$ 10,00 |
| 4 | **LED1 da LaunchPad** | Alerta visual por meio do LED1 ligado ao P1.0 | 1 | Incluso na placa | R$ 0,00 |
| 5 | **LCD 16x2** (módulo I²C) | Exibição do estado do sistema e do percentual relativo ao limite de gás | 1 | R$ 15,00 a R$ 30,00 | R$ 15,00 a R$ 30,00 |
| 6 | **Resistores** (10 kΩ e 18 kΩ para o divisor; resistor de base) | Adequação da tensão do sensor e apoio ao acionamento do buzzer, se necessário | kit | R$ 0,40 | R$ 0,40 |
| 7 | **Protoboard e jumpers** | Montagem e interligação dos componentes | 1 kit | R$ 20,00 | R$ 20,00 |
| | **TOTAL ESTIMADO** | | | | **R$ 205,40 a R$ 235,40** |

### Composição do total

| Cenário | Soma |
| --- | --- |
| **Mínimo** | 150,00 + 15,00 + 5,00 + 0,00 + 15,00 + 0,40 + 20,00 = **R$ 205,40** |
| **Máximo** | 150,00 + 25,00 + 10,00 + 0,00 + 30,00 + 0,40 + 20,00 = **R$ 235,40** |

> O valor do LED1 é **R$ 0,00** porque o LED já vem montado na LaunchPad MSP430F5529LP e é acessado pelo pino
> P1.0 — não é um componente externo.

## 2. Itens que já vêm com a LaunchPad

| Item | Observação |
| --- | --- |
| LED1 (P1.0) | Alerta visual — não precisa comprar |
| LED2 (P1.0/P4.7 conforme revisão) | Disponível para depuração |
| Botões S1 (P1.1/RST) e S2 (P2.1) | O **S2** é usado como botão de silenciamento |
| Alimentação 3,3 V e 5 V via USB | Alimenta o sensor e o LCD |
| Gravador/depurador eJTAG integrado | Programação e depuração pela própria USB |

## 3. Itens adicionais sugeridos (fora do total acima)

| Componente | Para quê | Custo aproximado |
| --- | --- | --- |
| Transistor NPN (ex.: 2N2222 / BC548) | Acionar o buzzer sem sobrecarregar o pino do MSP430 | R$ 1,00 |
| Resistor de base (1 kΩ) | Limitar a corrente da base do transistor | R$ 0,20 |
| Fonte/cabo USB extra | Alimentação dedicada após a montagem | R$ 10,00 |
| Caixa de proteção ventilada | Uso real fora da bancada (trabalho futuro) | R$ 20,00 a R$ 40,00 |

Esses itens **não** entram no total do relatório; ficam registrados porque são necessários para transformar o
protótipo de bancada em um dispositivo instalável — ver
[`docs/BUGS_E_MELHORIAS.md`](../docs/BUGS_E_MELHORIAS.md) § 20 (watchdog e diagnóstico) e § 21 (caixa e instalação).

## 4. Notas de tensão e compatibilidade

| Ponto | Regra |
| --- | --- |
| Alimentação do MSP430F5529 | 3,3 V (a LaunchPad fornece a linha de 3,3 V e a de 5 V do USB) |
| Entrada do ADC (P6.0/A0) | **Nunca acima de 3,3 V** — usar divisor 10 kΩ/18 kΩ se o módulo for alimentado em 5 V |
| MQ-2 | Pode ser alimentado em 5 V, mas a saída precisa ser condicionada; se alimentado em 3,3 V com saída ≤ 3,3 V, liga direto ao ADC |
| LCD 16x2 I²C | Níveis de 3,3 V; **pull-ups em 3,3 V**, nunca em 5 V |
| Buzzer | Acionamento por transistor NPN se a corrente exceder a capacidade do pino |
| GND | Referência comum obrigatória entre placa, sensor, LCD e buzzer |

## 5. Onde economizar e onde não economizar

| Economizar | Não economizar |
| --- | --- |
| Buzzer e protoboard genéricos são perfeitamente adequados | A LaunchPad é o item mais caro e o mais reutilizável — vale como investimento de longo prazo |
| Resistores avulsos custam centavos; comprar em kit é mais simples | O condicionamento de sinal (divisor/resistor de base) protege o microcontrolador: um erro aqui queima o pino |
| Um LCD 16x2 I²C reduz fios e solda em relação ao paralelo | Um sensor MQ-2 barato demais pode ter deriva grande — o procedimento de calibração compensa parcialmente |

## 6. Checklist de compra

- [ ] LaunchPad MSP430F5529LP (1×)
- [ ] Módulo sensor MQ-2 (1×)
- [ ] Buzzer ativo **ou** passivo (1×) — se passivo, o tom é gerado por PWM
- [ ] LCD 16x2 com módulo I²C (1×)
- [ ] Resistores 10 kΩ (1×) e 18 kΩ (1×) para o divisor
- [ ] Protoboard e jumpers macho–fêmea
- [ ] (Opcional) Transistor NPN + resistor de 1 kΩ para o buzzer
