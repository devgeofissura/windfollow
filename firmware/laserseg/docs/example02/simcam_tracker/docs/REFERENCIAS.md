# Referências consultadas

1. Documentação LILYGO T-SIMCam, especificações, configuração Arduino, PSRAM/flash e pin diagram: https://wiki.lilygo.cc/products/t-sim-series/t-simcam/
2. Quick Start LILYGO T-SimCam, parâmetros Arduino e exemplo de captura: https://wiki.lilygo.cc/products/t-sim-series/t-simcam/quick-start.html
3. Exemplo oficial de pinagem da família `LilyGo-Camera-Series`, bloco `CAMERA_MODEL_TTGO_T_CAM_SIM`: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series/blob/master/examples/sketch/select_pins.h
4. Sketch oficial usado para checar inicialização de câmera e `PWR_ON_PIN`: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series/blob/master/examples/sketch/sketch.ino
5. Issue sobre GPIOs T-SIMCAM; o mantenedor confirma GPIO31 ligado à flash e indisponível; GPIOs 45/46 e 21 são relatados pelo autor da issue como em uso pelo modem/LED e devem ser confirmados na revisão física: https://github.com/Xinyuan-LilyGO/LilyGo-Camera-Series/issues/33
6. AccelStepper documentation/source: https://www.airspayce.com/mikem/arduino/AccelStepper/

**Nota de revisão de placa:** a wiki atual e o exemplo legado do repositório apresentam pinagem diferente em alguns pontos. O sketch usa os valores do bloco `CAMERA_MODEL_TTGO_T_CAM_SIM` (incluindo PWR_ON GPIO1 e RESET GPIO18). A versão física e o sensor precisam ser confirmados. Se a inicialização falhar, conferir o esquema da revisão V1.3 antes de alterar qualquer GPIO. A compilação não verifica pinagem elétrica nem captura real.

7. Nanotec, folha dimensional do motor NEMA 23 ST5918 (face 56,4 ±0,5 mm, furação 47,14 ±0,2 mm, piloto 38,1 mm): https://www.nanotec.com/fileadmin/files/Baureihenuebersichten/Schrittmotoren/Product_Overview_ST5918.pdf
8. Ampere Electronics, página comercial do motor 23HS7628 (lista 2,8 A/fase, resistência 1,13 Ω, indutância 3,6 mH, 1,8° e 76 mm; confirmar com etiqueta do exemplar): https://ampere-electronics.com/product/23hs7628-nema23-stepper-motor/
9. HANPOSE/Alibaba, anúncio do conjunto 23HS7628-TB6600 Set (indica kit de motor/driver; dados são de anúncio comercial): https://www.alibaba.com/product-detail/Hanpose-57-stepper-motor-23HS7628-TB6600_1600913940806.html
