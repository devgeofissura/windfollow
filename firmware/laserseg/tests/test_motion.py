"""Testes host da camada de movimento DM556D + NEMA 23 (T3).

Escritos ANTES da implementacao (TDD). O modulo ``motion_config`` sera a
referencia host dos constantes da spec; o cabecalho C++ ``driver_config.h``
deve reproduzi-los exatamente. As funcoes puras de conversao ja existem no
nucleo (``protocol_ref.py`` / ``controller/parser.cpp``) e sao reutilizadas,
nao duplicadas.

Execute com:

    powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1
"""

from __future__ import annotations

import pathlib
import re
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import motion_config as m  # noqa: E402
import protocol_ref as p  # noqa: E402


class Dm556dConstants(unittest.TestCase):
    def test_constants_match_approved_spec(self) -> None:
        self.assertEqual(m.STEPS_PER_REV, 3200)
        self.assertEqual(m.MAX_TRAVEL, 533)
        self.assertEqual(m.DEAD_BAND, 55)
        self.assertAlmostEqual(m.TRACK_GAIN, 0.75)
        self.assertEqual(m.MANUAL_STEP, 80)
        self.assertEqual(m.LINK_TIMEOUT_MS, 1200)

    def test_cpp_header_matches_host_reference(self) -> None:
        header = ROOT / "controller_dm556d" / "driver_config.h"
        self.assertTrue(header.is_file(), f"cabecalho ausente: {header}")
        text = header.read_text(encoding="utf-8")
        expected = {
            "LASERSEG_STEPS_PER_REV": m.STEPS_PER_REV,
            "LASERSEG_MAX_TRAVEL": m.MAX_TRAVEL,
            "LASERSEG_DEAD_BAND": m.DEAD_BAND,
            "LASERSEG_MANUAL_STEP": m.MANUAL_STEP,
            "LASERSEG_LINK_TIMEOUT_MS": m.LINK_TIMEOUT_MS,
        }
        for name, value in expected.items():
            match = re.search(rf"#define\s+{name}\s+(\d+)", text)
            self.assertIsNotNone(match, f"{name} ausente em driver_config.h")
            self.assertEqual(int(match.group(1)), value, name)
        gain = re.search(r"#define\s+LASERSEG_TRACK_GAIN\s+([0-9.]+)f", text)
        self.assertIsNotNone(gain, "LASERSEG_TRACK_GAIN ausente")
        self.assertAlmostEqual(float(gain.group(1)), m.TRACK_GAIN)


class MonotonicMillis(unittest.TestCase):
    def test_normal_increment_is_forward(self) -> None:
        self.assertEqual(m.unwrap_millis(1000, 900), 1000)

    def test_wrap_keeps_monotonic_time(self) -> None:
        # 2^32 - 10 -> 10: sao 20 ms, nao um retrocesso de ~49 dias.
        before = (1 << 32) - 10
        self.assertEqual(m.unwrap_millis(10, before), before + 20)

    def test_watchdog_boundary_survives_wrap(self) -> None:
        # Pacote recebido 100 ms antes do wrap; poll 1201 ms depois.
        before = (1 << 32) - 100
        now = m.unwrap_millis(1101, before)
        self.assertEqual(now - before, 1201)
        self.assertTrue(
            p.link_expired(last_packet_ms=before, now_ms=now)
        )


class ReusedCoreFunctions(unittest.TestCase):
    def test_target_from_error_uses_spec_constants(self) -> None:
        self.assertEqual(
            p.target_from_error(
                m.DEAD_BAND, max_travel=m.MAX_TRAVEL,
                gain=m.TRACK_GAIN, dead_band=m.DEAD_BAND,
            ),
            0,
        )
        self.assertEqual(
            p.target_from_error(
                1000, max_travel=m.MAX_TRAVEL,
                gain=m.TRACK_GAIN, dead_band=m.DEAD_BAND,
            ),
            399,
        )
        self.assertEqual(
            p.target_from_error(
                -1000, max_travel=m.MAX_TRAVEL,
                gain=1.0, dead_band=m.DEAD_BAND,
            ),
            -m.MAX_TRAVEL,
        )

    def test_manual_target_uses_spec_constants(self) -> None:
        self.assertEqual(
            p.manual_target(0, 1, max_travel=m.MAX_TRAVEL, step=m.MANUAL_STEP),
            m.MANUAL_STEP,
        )
        self.assertEqual(
            p.manual_target(500, 1, max_travel=m.MAX_TRAVEL,
                            step=m.MANUAL_STEP),
            m.MAX_TRAVEL,
        )

    def test_link_timeout_boundary(self) -> None:
        self.assertFalse(
            p.link_expired(last_packet_ms=1000, now_ms=2200)
        )
        self.assertTrue(
            p.link_expired(last_packet_ms=1000, now_ms=2201)
        )


if __name__ == "__main__":
    unittest.main()
