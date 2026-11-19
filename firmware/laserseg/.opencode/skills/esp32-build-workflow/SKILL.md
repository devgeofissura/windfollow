---
name: esp32-build-workflow
description: Use ao compilar, detectar porta/placa, escolher FQBN, gravar firmware ESP32-S3 do laserseg ou resolver erro de build. Use antes de qualquer arduino-cli, tools/flash.ps1 ou chamada de upload.
---

# Fluxo de build e gravação — laserseg

## Estado atual

- `arduino-cli` **não está no PATH** nesta máquina (detectado em 2026-11-19).
- `COM3` é um USB-SERIAL CH340 visível, mas chip/flash/MAC não foram confirmados.
- `tools/flash.ps1` não aceita `-Sketch laserseg` (ValidateSet: irobot, msa, windfollow,
  windfollowblind). Não existe perfil de build/flash para este subprojeto.
- Porta COM nunca é permanente. Detectar sempre antes de gravar.

## Passos

1. Carregue `laserseg-context` se ainda não o fez.
2. Detecção obrigatória, a partir da raiz do repositório:
   ```powershell
   powershell -ExecutionPolicy Bypass -File tools/flash.ps1 -DetectOnly
   ```
   Se falhar por `arduino-cli` ausente, pare e informe: não dá para afirmar compatibilidade
   de placa. Não grave "no chute".
3. Confira as opções reais do core antes de fixar FQBN:
   ```powershell
   arduino-cli board details --fqbn esp32:esp32:esp32s3
   ```
   O protótipo sugere:
   ```text
   esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240
   ```
   O perfil `windfollow` da raiz usa `PartitionScheme=huge_app`. **Não misture** sem validar.
4. Compile em diretório temporário, nunca dentro do repositório:
   ```powershell
   arduino-cli compile `
     --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc" `
     --build-path "$env:TEMP\laserseg-build\<no>" `
     "firmware\laserseg\<no>"
   ```
   Substitua `<no>` por `camera_node`, `controller_drv8834` ou `controller_uln2003`.
   Ajuste `CPUFreq=240` apenas se `board details` confirmar a opção no core instalado.
5. Antes de gravar: foco desligado, eixos centralizados, limites lógicos conservadores e
   `hardware-safety` carregada. Redetecte a porta. Só então faça o upload.
6. Se for criar o perfil no `tools/flash.ps1`, adicione `laserseg` ao `ValidateSet`, ao mapa
   de pastas e ao FQBN, com o chip esperado ESP32-S3, e documente em `ESTADO.md`.

## Regras

- Compilar não valida pinout nem hardware: é apenas gate de sintaxe/biblioteca.
- Erro de biblioteca faltando → consultar `context7` ou documentação oficial antes de
  adicionar dependência. Nunca copiar biblioteca de fonte desconhecida para o repo.
- Não commite `.build/`, binários, logs de compilação nem cached do arduino-cli.

## Critério de saída

Build passa (ou a causa exata do bloqueio é registrada), FQBN está explícito e a decisão
de gravação está consciente. Se não há chip confirmado, o critério é "bloqueado com motivo".
