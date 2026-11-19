#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240'
for sketch in camera_node controller_drv8834 controller_uln2003; do
  echo "=== Compilando ${sketch} ==="
  arduino-cli compile --fqbn "$FQBN" "$sketch"
done
