"""Referencia host dos parametros de movimento DM556D + NEMA 23.

Espelha ``controller_dm556d/driver_config.h``. As funcoes puras de conversao
de erro em passos continuam em ``protocol_ref.py``: aqui so ha constantes da
spec e o tratamento do wrap-around do ``millis()`` do ESP32.

O ``millis()`` embarcado e um ``uint32_t`` que volta a zero a cada ~49,7 dias.
O nucleo do parser recebe tempos ``int64_t``; o no DM556D converte a amostra
em tempo monotonico antes de chamar ``feed()`` / ``poll()``. Assim o watchdog
nao interpreta um wrap como "pacote recente" e nao deixa eixos soltos.
"""

from __future__ import annotations

MILLIS_WRAP = 1 << 32
MILLIS_MASK = MILLIS_WRAP - 1

# Spec SPEC-dm556d-nema23.md (aprovada em 2026-11-19).
STEPS_PER_REV = 3200  # DM556D DIP SW5-SW8 = OFF OFF ON ON
MAX_TRAVEL = 533      # limite local de deslocamento do protocolo v0.1
DEAD_BAND = 55        # kDeadBand do nucleo
TRACK_GAIN = 0.75
MANUAL_STEP = 80      # kManualStep do nucleo
LINK_TIMEOUT_MS = 1200


def unwrap_millis(sample: int, previous: int) -> int:
    """Converte uma amostra de ``millis()`` em tempo monotonico int64.

    ``previous`` e o ultimo valor monotonico devolvido por esta funcao.
    A subtracao em modulo 2**32 transforma o wrap em um delta positivo.
    """
    last = previous & MILLIS_MASK
    delta = (sample - last) & MILLIS_MASK
    return previous + delta
