# Base de conhecimento — rastreamento com S3_CAM_SIM

**Elaborada em:** 2026-11-19  
**Estado:** documento de referência inicial; decisões e fatos abaixo refletem o contexto disponível nessa data.

## 1. Propósito e escopo

Esta base registra uma arquitetura para rastrear um alvo visual com a placa marcada **S3_CAM_SIM V1.3**, transmitir posições por Wi‑Fi a uma controladora ESP32-S3 separada e acionar uma montagem pan-tilt de dois motores de passo e um foco LED branco difuso. Destina-se a orientar implementação e validação por outra IA ou equipe técnica.

Não é um esquema elétrico, projeto mecânico dimensionado nem confirmação de compatibilidade de firmware. O sensor exato, pinagem integral, requisitos elétricos e desempenho ainda não foram verificados. As referências são pontos de partida, não prova de suporte direto ao conjunto final.

## 2. Resumo executivo

A orientação adotada é preservar a T-SIMCAM para câmera/rastreamento e separar o controle de movimento/iluminação em outra ESP32-S3. A comunicação proposta é Wi‑Fi, com posições e estado enviados da unidade de visão à controladora. No issue #33 do repositório LilyGo-Camera-Series, o mantenedor corrige que GPIO31 está ligado à flash e indisponível. O autor do issue, não o mantenedor, relata que GPIO45/46 são usados pelo modem e GPIO21 pelo LED mPCIe. Trate essas atribuições como informação de issue a confirmar no esquema e na revisão física. Não presuma que pinos de borda sejam GPIOs extras nem conclua que a placa não pode fazer visão. [1]

A página comercial da LILYGO oferece variantes com sensores OV2640 IR-CUT e OV5640 IR-CUT; a unidade concreta precisa ser inspecionada antes de fixar driver/configuração. [11] A documentação atual do ESP-WHO lista S3-EYE e Korvo-2, não S3_CAM_SIM. Portanto, não se afirma existir BSP pronto para essa placa: integração/port da câmera permanece pendente. [3]

## 3. Fatos confirmados e evidência

1. **Identificação física observada:** a fotografia da placa do usuário mostra a marcação `S3_CAM_SIM V1.3`. Este fato provém do contexto do projeto, não de inferência sobre o sensor ou revisão elétrica.
2. **Restrições de pinos:** no issue #33, o mantenedor LilyGO confirma que GPIO31 está ligado à flash e indisponível; o autor do issue relata que GPIO45/46 são usados pela UART do modem e GPIO21 pelo LED mPCIe. Não atribuir ao mantenedor os relatos do autor. Confirmar tudo no esquema da revisão. [1]
3. **Capacidade versus expansão:** essas restrições não demonstram incapacidade de captura ou visão; significam que não se deve projetar a placa para acionar diretamente todas as cargas externas previstas sem validar pinos. [1]
4. **Variantes de câmera:** a página de venda oferece configurações OV2640 IR-CUT e OV5640 IR-CUT. O sensor realmente instalado ainda precisa ser confirmado. [11]
5. **BSP ESP-WHO:** a documentação consultada lista S3-EYE e Korvo-2 como placas-alvo documentadas; não lista S3_CAM_SIM. Não presumir BSP pronto para esta placa. [3]
6. **Decisão arquitetural:** manter câmera/rastreamento na T-SIMCAM e enviar posições por Wi‑Fi a uma controladora ESP32-S3 separada. Esta é a orientação adotada para o projeto, não um resultado experimental.
7. **Cargas previstas:** a controladora separada deverá comandar dois motores de passo/driver e um foco LED branco difuso. Modelos, tensões, correntes, pinagem e dimensões ainda não definidos.

## 4. Arquitetura proposta

- **Nó de visão:** T-SIMCAM S3_CAM_SIM V1.3, câmera a confirmar, executando captura e o processamento de rastreamento que vier a ser validado.
- **Link:** Wi‑Fi entre os nós. Transporte, formato, frequência, autenticação e comportamento de perda de conexão ainda são decisões de projeto.
- **Controladora de movimento:** placa ESP32-S3 separada, dedicada à recepção de comandos, geração de movimento para dois drivers/motores de passo, gestão do LED e intertravamentos.
- **Mecânica:** conjunto pan-tilt dimensionado para câmera/carga e limites de curso definidos durante projeto mecânico.
- **Iluminação:** LED branco difuso com acionamento adequado ao componente selecionado; evitar ligação direta a GPIO sem circuito dimensionado.

### Diagrama ASCII textual

```text
[Cena / alvo]
      │ luz
      ▼
[Câmera da T-SIMCAM: OV2640 IR-CUT ou OV5640 IR-CUT — confirmar]
      │ quadros
      ▼
[Captura + detecção/localização de alvo na T-SIMCAM]
      │ posição/estado (Wi-Fi; protocolo ainda proposto)
      ▼
[ESP32-S3 controladora separada]
      ├──► [Driver motor eixo pan] ──► [Motor de passo pan]
      ├──► [Driver motor eixo tilt] ──► [Motor de passo tilt]
      └──► [Estágio de acionamento] ──► [Foco LED branco difuso]
                ▲
       alimentação e proteções
       a dimensionar/validar
```

## 5. Fluxo de dados e controle

1. A T-SIMCAM inicializa a câmera conforme o sensor e a configuração efetivamente identificados.
2. Captura quadros e executa o algoritmo de localização escolhido. Capturar imagem não equivale a detectar um objeto; detectar/localizar não equivale a identificar uma pessoa ou reconhecer sua identidade.
3. Converte a localização da imagem em coordenadas normalizadas ou erro relativo ao centro, com timestamp/contador e validade. A transformação para ângulos/unidades mecânicas depende de calibração.
4. Envia a observação por Wi‑Fi. A controladora valida formato, faixa, idade da amostra e estado de segurança antes de aceitar movimento.
5. O controlador aplica limites, perfil de movimento e política para amostras perdidas; comanda os drivers de forma não bloqueante e atualiza iluminação de acordo com política explicitamente definida.
6. Telemetria/acknowledgement pode ser retornada para diagnóstico. Esses passos descrevem fluxo proposto, não implementação verificada.

## 6. Decisão de hardware

A separação de responsabilidades reduz a dependência de GPIOs externos da placa de câmera e permite escolher uma controladora com pinos/periféricos apropriados ao movimento. A T-SIMCAM continua sendo o nó de visão; não se propõe descartá-la nem afirmar que é incapaz de visão. A controladora ESP32-S3 adicional e o enlace Wi‑Fi são a orientação adotada.

Ainda selecionar: controladora/placa exata; sensores de fim de curso ou referência; dois drivers compatíveis com motores e alimentação; motores e mecânica; alimentação e proteções; LED e estágio de corrente; conectores/cabeamento e estratégia térmica. Nenhum valor elétrico foi verificado, portanto não se especificam tensões, correntes, microstepping ou limites de curso.

## 7. Câmera e visão: captura, detecção e identificação

**Captura** é a aquisição de quadros pelo sensor e sua entrega ao software. **Detecção/localização** é encontrar uma região/alvo no quadro e estimar posição. **Identificação** é atribuir identidade a uma pessoa/objeto; é uma capacidade diferente, com requisitos, riscos de privacidade e suporte algorítmico que não estão estabelecidos neste projeto. A arquitetura mínima pretendida é rastreamento/localização, não identificação biométrica.

A variante do sensor deve ser verificada fisicamente ou por documentação confiável da unidade. A oferta lista OV2640 IR-CUT e OV5640 IR-CUT, sem confirmar qual equipa a placa do usuário. [11] O repositório esp32-camera é referência para o componente de câmera, mas sua presença não prova configuração pronta para esta placa. [2] A documentação ESP-WHO lista S3-EYE e Korvo-2; BSP/port e configuração para S3_CAM_SIM continuam pendentes. [3]

## 8. Controle pan-tilt, motores, alimentação e LED

A ESP32-S3 separada recebe erro/posição e converte esses dados em comandos de movimento nos dois eixos. Deve haver limites físicos e lógicos, comportamento definido para partida/referência e perda de observações, prevenção de movimentos abruptos e uma estratégia para impedir que o rastreamento integre comandos fora do curso.

FastAccelStepper é uma referência de biblioteca para avaliar geração de passos, sem garantia de compatibilidade final ou de configuração. [4] Pololu DRV8834 é uma referência de driver; a seleção final depende de motor, fonte, dissipação, corrente e requisitos do sistema, ainda não validados. [5]

Motores e drivers devem receber alimentação dimensionada separadamente dos sinais de lógica, com aterramento, proteção e desacoplamento projetados conforme os componentes escolhidos. O LED branco difuso requer estágio de acionamento apropriado e controle térmico/óptico; não assumir que GPIO o alimenta diretamente. Valores elétricos e topologia definitiva ficam em aberto.

## 9. Software e bibliotecas

- **LilyGo-Camera-Series:** documentação/código e issue relevantes para a placa e restrições de pinos; verificar revisão e variante concreta. [1]
- **esp32-camera:** referência de captura/driver de câmera para ESP32; adaptar e validar com sensor e placa reais. [2]
- **ESP-WHO:** referência de visão/IA embarcada; suporte documentado citado para S3-EYE/Korvo-2 não equivale a BSP S3_CAM_SIM. [3]
- **FastAccelStepper:** avaliar para controle de motores de passo no nó dedicado. [4]
- **DRV8834:** referência de driver a avaliar; não assumir adequação sem dados do motor/alimentação. [5]
- **Projetos de rastreamento:** face-track-demo, Pi_Servo_Tracker, Object-Tracking-OpenCV_Arduino, Arduino-Face-Tracking e SaraKIT MediaPipe podem inspirar abordagens, mas não são prova de compatibilidade de placa, sensor, atuadores ou ambiente de execução. [6–10]

## 10. Pinagem conhecida e restrições

| Sinal | Informação disponível | Implicação |
|---|---|---|
| GPIO31 | Ligado à flash e indisponível, segundo issue do mantenedor | Não usar como GPIO de aplicação. [1] |
| GPIO45/46 | UART do modem, segundo issue do mantenedor | Não assumir disponibilidade para atuadores. [1] |
| GPIO21 | Ligado ao LED mPCIe, segundo issue do mantenedor | Não tratar como saída livre sem análise do circuito. [1] |
| Pinos de borda | Não são GPIOs extras por estarem expostos na borda | Não inferir pinagem/recursos pela aparência. [1] |

A lista acima não é pinout completo. A conclusão de projeto é não depender da T-SIMCAM para acionar diretamente todas as cargas externas; validar cada sinal com esquema/documentação da revisão e medições seguras. Isso não impede o processamento visual na placa.

## 11. Riscos e segurança

- **Iluminação:** usar difusão para reduzir pontos brilhantes e ofuscamento; controlar temperatura e evitar olhar diretamente para o LED. Especificação óptica/térmica ainda não feita.
- **Mecânica:** risco de beliscamento, colisão, cabos enroscados e sobrecurso. Adotar limites, parada segura, montagem estável e teste inicial sem carga/velocidade elevada.
- **Motores:** ruído elétrico, aquecimento, vibração e travamento podem afetar eletrônica e estrutura. Dimensionar drivers, fontes, dissipação e proteção com dados reais.
- **Perda de Wi‑Fi/dados obsoletos:** expirar comandos e entrar em estado seguro; não continuar indefinidamente com a última posição.
- **Visão/privacidade:** priorizar localização sem identificação; restringir armazenamento/transmissão de imagens e obter consentimento apropriado para qualquer uso com pessoas.
- **Pinos/placa:** pinos em uso interno podem causar conflito ou dano se reutilizados; confirmar documentação da revisão antes de conectar.

## 12. Projetos GitHub de referência e limitações

As URLs abaixo identificam os projetos fornecidos como referências. Foram tratados como inspiração/referência, não como evidência de que funcionem diretamente na S3_CAM_SIM.

| Projeto | URL | Uso cauteloso |
|---|---|---|
| LilyGO camera series | https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series | Referência da família; issue do mantenedor é a origem das restrições informadas. [1] |
| esp32-camera | https://github.com/espressif/esp32-camera | Referência de driver/captura; sensor e configuração da placa precisam ser validados. [2] |
| ESP-WHO | https://github.com/espressif/esp-who | Referência de visão; documentação atual citada não lista S3_CAM_SIM como BSP pronto. [3] |
| FastAccelStepper | https://github.com/gin66/FastAccelStepper | Avaliar para geração de passos na controladora; compatibilidade deve ser testada. [4] |
| Pololu DRV8834 | https://github.com/pololu/drv8834-stepper-motor-driver-carrier | Referência do driver, não seleção definitiva. [5] |
| pageauc/face-track-demo | https://github.com/pageauc/face-track-demo | Exemplo Raspberry Pi legado com detecção de movimento/face e pan-tilt por servos; não é firmware S3. [6] |
| Gill003/Pi_Servo_Tracker | https://github.com/Gill003/Pi_Servo_Tracker | Referência conceitual; plataforma/atuadores podem diferir. [7] |
| aju22/Object-Tracking-OpenCV_Arduino | https://github.com/aju22/Object-Tracking-OpenCV_Arduino | Referência conceitual; não prova desempenho ou suporte ao hardware. [8] |
| LanaDelSlay/Arduino-Face-Tracking | https://github.com/LanaDelSlay/Arduino-Face-Tracking | Referência conceitual; não implica identificação nem compatibilidade. [9] |
| SaraEye SaraKIT MediaPipe | https://github.com/SaraEye/SaraKIT-Face-Tracking-MediaPipe-Raspberry-Pi-64bit | Referência Raspberry Pi/BLDC; não presumir port para S3_CAM_SIM. [10] |

## 13. Incógnitas priorizadas

1. Qual sensor está instalado: OV2640 IR-CUT ou OV5640 IR-CUT? Confirmar também interface, foco/lente e filtro IR-CUT da unidade.
2. Qual esquema/pinout corresponde exatamente à S3_CAM_SIM V1.3 e quais GPIOs de câmera/modem/LED estão ocupados além dos três relatados?
3. Qual combinação firmware, versão de ESP-IDF/Arduino core e esp32-camera inicializa a câmera real? É necessário port/BSP específico?
4. Qual algoritmo de detecção/localização atende ao alvo e taxa de quadros desejada, sem pressupor identificação de pessoa?
5. Qual placa ESP32-S3 de controle, motor, driver, alimentação e LED serão selecionados após levantar carga, curso, velocidade e ambiente?
6. Como será feita referência/limitação de curso dos eixos (sensores, batentes ou outro método seguro)?
7. Qual protocolo, autenticação, frequência, timeout, reconexão e política de comando obsoleto serão implementados?
8. Quais geometria, massa, centro de gravidade, curso e montagem mecânica devem ser suportados?
9. Quais requisitos de privacidade, retenção de imagens e condições de iluminação/cena se aplicam?

## 14. Plano incremental de testes

1. **Inspeção sem alimentação:** confirmar marcação, sensor, conectores e documentação/revisão; registrar fotos e pinout. Não deduzir pinagem por posição física.
2. **Inicialização da câmera:** com configuração mínima e fonte apropriada, verificar sensor detectado, captura estável e formato/resolução escolhidos; comparar configuração para cada variante apenas se aplicável.
3. **Visão isolada:** avaliar localização em cenas controladas, iluminação variada e alvos sem usar motores; distinguir falha de captura de falha algorítmica.
4. **Link Wi‑Fi:** testar conexão, sequência, atraso, pacote inválido, perda e reconexão com mensagens sintéticas antes de ligar atuadores.
5. **Controladora sem carga mecânica:** validar parser, limites, watchdog/timeout e saída de passo com instrumentação/condição segura; confirmar compatibilidade de biblioteca e driver.
6. **Um eixo e carga progressiva:** verificar direção, referência, curso, aquecimento e ruído; repetir separadamente no segundo eixo e depois em conjunto.
7. **LED:** testar estágio de acionamento, difusão, temperatura e comportamento de falha sem expor pessoas a ofuscamento.
8. **Integração fechada:** habilitar movimento com limites conservadores, registrar quadros/posições/telemetria sem armazenar dados desnecessários; testar alvo perdido, Wi‑Fi perdido, reinicialização e parada.
9. **Revisão de segurança:** confirmar fixação, cabos, proteção elétrica, limites e política de privacidade antes de uso operacional.

## 15. Proposta de protocolo Wi‑Fi (não é fato nem implementação existente)

**Proposta inicial:** mensagens JSON pequenas sobre UDP ou TCP, a decidir por ensaio. Campos de observação: `version`, `seq`, `timestamp_ms`, `valid`, `x_norm`, `y_norm`, `confidence` e estado opcional. Coordenadas normalizadas poderiam ocupar faixa convencionada como intervalo unitário, mas o sentido/origem e a escala devem ser formalmente definidos antes da implementação. A controladora rejeita sequência repetida, amostra expirada, valores fora da faixa e comandos enquanto não estiver habilitada; aplica timeout e limites mecânicos locais. Resposta opcional: `ack_seq`, `state`, `fault`. O protocolo deve evitar receber comandos arbitrários de posição absoluta sem limites locais. Autenticação, transporte, codificação final, limites numéricos e tempos de expiração são decisões pendentes, não valores confirmados.

## 16. Fontes numeradas

As fontes abaixo são URLs de repositórios/documentação identificados para esta base. A atribuição específica do issue do mantenedor e das variantes comerciais decorre do contexto fornecido; não se inventa número de issue ou URL de página de venda quando não foi fornecido.

[1] LilyGO, **LilyGo-Camera-Series** (inclui issue do mantenedor citado no contexto; número/URL específico do issue não fornecido): https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series

[2] Espressif, **esp32-camera**: https://github.com/espressif/esp32-camera

[3] Espressif, **ESP-WHO**: https://github.com/espressif/esp-who

[4] **FastAccelStepper**: https://github.com/gin66/FastAccelStepper

[5] Pololu, **DRV8834 Stepper Motor Driver Carrier**: https://github.com/pololu/drv8834-stepper-motor-driver-carrier

[6] pageauc, **face-track-demo** (projeto legado para Raspberry Pi): https://github.com/pageauc/face-track-demo

[7] Gill003, **Pi_Servo_Tracker**: https://github.com/Gill003/Pi_Servo_Tracker

[8] aju22, **Object-Tracking-OpenCV_Arduino**: https://github.com/aju22/Object-Tracking-OpenCV_Arduino

[9] LanaDelSlay, **Arduino-Face-Tracking**: https://github.com/LanaDelSlay/Arduino-Face-Tracking

[10] SaraEye, **SaraKIT Face Tracking MediaPipe Raspberry Pi 64bit**: https://github.com/SaraEye/SaraKIT-Face-Tracking-MediaPipe-Raspberry-Pi-64bit

[11] LILYGO, **T-SIMCam product page and camera variants**: https://lilygo.cc/en-us/products/t-simcam

## Changelog

- **2026-11-19:** primeira edição; consolidada arquitetura separando visão e controle, restrições de GPIO informadas, pendências de sensor/BSP e plano incremental. Protocolo Wi‑Fi explicitamente identificado como proposta.
