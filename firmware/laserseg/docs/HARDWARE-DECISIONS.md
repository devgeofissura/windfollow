# Decisões de hardware — DM556D + NEMA 23 (T1)

Data: 2026-11-19  
Artefato da tarefa **T1** do plano `tasks/plan.md`, referenciado por
`docs/foco-luz-seguidor-001`.  
Spec de origem: `spec/SPEC-dm556d-nema23.md` (aprovada em 2026-11-19).

> Esta nota registra o que foi **verificado em fontes públicas** e o que
> continua **bloqueado**. Ela **não substitui a etiqueta física do motor nem o
> manual da unidade Azanelli DM556D**. Dados de variantes Leadshine,
> StepperOnline ou HANPOSE são citados como contexto — nunca como
> especificação confirmada do exemplar do usuário.

## Status das 7 perguntas da spec

| # | Pergunta | Situação | Bloqueia |
|---:|---|---|---|
| 1 | Etiqueta do motor | **Parcial** — 1,8° / 2,8 A confirmados para variante da família; sufixo e base RMS/pico não confirmados | T5 |
| 2 | DIP SW1–SW8 | **Bloqueado** — proposta conservadora existe; posição física não conferida | T5 |
| 3 | Interface 3,3 V → opto | **Bloqueado** — variantes documentam 7–16 mA e pulso ≥ 2,5 µs; nada prova o Azanelli | T5 |
| 4 | Placa controladora | **Bloqueado** — a placa conectada hoje é a T-SIMCam de bancada; a arquitetura pede controladora separada | T5, T6 |
| 5 | Pinos reais | **Bloqueado** — valores atuais vêm do protótipo `example02` | T5 |
| 6 | Rede (SSID/modo/IP) | **Parcial** — modo AP e IPs do protótipo conhecidos; credencial real fora do repo e modo final não decididos | T6 |
| 7 | Chave de emergência | **Ausente** — nenhum intertravamento físico descrito em código ou esquema | T5 |

Conclusão de gate: **T5 e T6 permanecem bloqueados**. O firmware gravado hoje é
serial-only (`LASERSEG_ENABLE_GPIO=0`); nenhum pino, motor ou luz foi ativado.

## 1. Motor 23HS7628

A página técnica HANPOSE localizada descreve a variante
`23HS7628-SFU1204` com **passo de 1,8°** e **corrente nominal de 2,8 A**. Ela
não informa se a corrente é RMS ou de pico e **não traz o sufixo exato**
`23HS7628-TB6600`. Portanto é confirmação comercial de uma variante da família,
não da etiqueta do exemplar em mãos. [1]

- **Registrado:** 1,8° (200 passos inteiros/volta) e 2,8 A como valores
  provisórios, coerentes com `STEPS_PER_REV=3200` se o driver estiver em
  1/16 micropasso.
- **Não registrado como fato:** base RMS/pico e torque real do exemplar.
- **Ação pendente:** fotografer/transcrever a etiqueta do motor antes de
  energizar os drivers.

## 2. Corrente e DIP switches

A proposta do protótipo é `SW1=OFF, SW2=OFF, SW3=ON` para **2,7 A RMS /
3,8 A pico**, ajuste conservador logo abaixo dos 2,8 A anunciados. O manual
Leadshine DM556 V3.0 contém a mesma linha 2,7 A / 3,8 A, mas é outro produto e
outra versão: isso **não prova** que a tabela do Azanelli DM556D seja idêntica. [2]

Há divergência documentada entre variantes, o que impede generalizar a tabela:

- O manual Leadshine DM556 V3.0 trata a corrente de repouso em **SW4**. [2]
- A variante StepperOnline **DM556N-L** descreve outra tabela e outra semântica. [3]
- Um fornecedor de DM556 publica uma tabela de micropasso diferente da
  combinação `OFF/OFF/ON/ON` anotada para a unidade fotografada. [4]
- O manual DM556E indica **SW4 OFF** para redução automática de corrente em
  repouso — interpretação diferente de "meia corrente / corrente plena". [6]

- **Registrado:** 3200 pulsos/volta como valor de projeto, dependente da
  combinação real de SW5–SW8.
- **Ação pendente:** ler a tabela impressa na tampa/unidade Azanelli e
  transcrever SW1–SW8 antes de aplicar alimentação aos motores.

## 3. Interface STEP/DIR (e ENABLE)

Manuais de variantes Leadshine confirmam **entradas isoladas** e corrente lógica
de **7–16 mA**. O manual DM556E cita pulso mínimo de **2,5 µs**, setup de direção
de **5 µs** e nível alto de PUL de **4–5 V**. Isso mostra que os **5 µs do sketch
do protótipo** atendem ao mínimo dessas variantes, mas **não valida 3,3 V
direto no Azanelli**. [3][5][6]

A variante DM556N-L cita sinais de **3,3–5 V** e ilustra ligações em
comum-ânodo e comum-cátodo — novamente, outro modelo. [3] O manual DM556 V1.0
também especifica 7–16 mA de corrente lógica e 4,0 A RMS como valor indicado
para aquela variante. [5]

- **Regra mantida:** não ligar GPIO 3,3 V diretamente em PUL/DIR/ENA sem o
  manual da unidade. A topologia (comum-ânodo ou comum-cátodo), a corrente do
  opto e o pulso mínimo devem vir do manual Azanelli.
- **Ação pendente:** obter o manual/esquema da unidade e dimensionar a
  interface de sinal antes de qualquer `LASERSEG_ENABLE_GPIO=1`.

## 4. Dados existentes no protótipo (não confirmados em bancada)

Estes valores aparecem em `docs/example02/simcam_tracker` e são **referência de
protótipo**, não pinagem de produção:

| Item | Valor do protótipo |
|---|---|
| STEP/DIR pan | GPIO 4 / 5 |
| STEP/DIR tilt | GPIO 6 / 7 |
| Fins de curso NF | GPIO 13/14 e 15/16 |
| Foco LED | GPIO 12 |
| Micropasso | `STEPS_PER_REV=3200` |
| Rede | `NET_MODE=0` (AP da câmera); câmera `192.168.4.1`, controladora `192.168.4.2`, UDP 4210 |
| Modo STA | não configurado |

Os pinos **não** foram transferidos para `controller_dm556d/`: o nó atual não
configura GPIO. A definição definitiva depende da placa controladora escolhida
(item 4 da tabela) e da revisão do `hardware-auditor`.

## 5. Credenciais e intertravamento

- Os sketches e documentos do protótipo contêm uma credencialWi-Fi de bancada
  Wi-Fi em texto claro (SSID e senha). Para não repetir o segredo neste repositório,
  o valor **não é transcrito aqui**. Antes de compartilhar/publicar, remover os
  defaults e movê-los para arquivo local ignorado ou `Preferences`.
- O timeout UDP (> 1200 ms) apaga a luz e interrompe comandos de movimento, mas
  **não corta a alimentação dos drivers nem desabilita os motores
  fisicamente**. Não substitui chave de emergência ou intertravamento.
- Não existe chave física descrita em código ou esquema. Enquanto não houver
  corte físico, qualquer ensaio com motores energizados deve usar fonte
  limitada, sem carga mecânica e com o operador pronto para desligar.

## 6. Próximos passos (gate T5/T6)

1. Foto/transcrição da etiqueta do motor — fecha item 1.
2. Tabela DIP impressa na unidade Azanelli — fecha item 2.
3. Manual da unidade: comum-ânodo/cátodo, corrente do opto, pulso mínimo —
   fecha item 3 e permite especificar a interface 3,3 V → driver.
4. Escolher a controladora física (ESP32-S3 separada) — fecha item 4.
5. Pinos reais no esquema revisado — fecha item 5; atualizar
   `docs/foco-luz-seguidor-001` e submeter ao `hardware-auditor`.
6. Definir modo AP/STA e credenciais fora do repo — fecha item 6.
7. Instalar chave de emergência/intertravamento que corte alimentação dos
   drivers — fecha item 7.
8. Somente depois: `LASERSEG_ENABLE_GPIO=1`, ensaio sem carga, luz OFF,
   watchdog por perda de pacote e verificação de corrente/temperatura.

## Referências

[1]: https://www.hanpose.com/product-hanpose-2-phase-57-stepper-motor-18-degree-23hs7628-sfu1204-28a-189ncm-300mmfor-3d-printer-monitor-ball-screw-motor-nema23 "HANPOSE 23HS7628-SFU1204"
[2]: https://kitaez-cnc.com/f/dm556_v30.pdf "Leadshine DM556 V3.0 User Manual"
[3]: https://www.omc-stepperonline.com/download/DM556N-L.pdf "StepperOnline DM556N-L Manual"
[4]: https://vallder.com/product/dm556-stepper-driver/ "Vallder DM556 technical page"
[5]: https://leadshineusa.com/UploadFile/Down/DM556m.pdf "Leadshine DM556 V1.0 User Manual"
[6]: https://cdn.automationdirect.com/static/manuals/leadshinestepper/dm556e.pdf "Leadshine DM556E User Manual"
