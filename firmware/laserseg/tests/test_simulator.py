"""Testes host do simulador web do laserseg (SPEC-simulador-web.md V1).

Escritos ANTES da implementacao: o teste verifica que a pagina e um espelho
fiel do contrato UDP v0.1 e das constantes aprovadas, sem depender de placa.

Execute com:

    powershell -ExecutionPolicy Bypass -File firmware/laserseg/tests/run.ps1
"""

from __future__ import annotations

import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SIM = ROOT / "simulator"
HTML = SIM / "index.html"
CSS = SIM / "css/style.css"
JS = SIM / "js/app.js"
README = SIM / "README.md"


class Estrutura(unittest.TestCase):
    def test_arquivos_da_pagina_existem(self) -> None:
        for path in (HTML, CSS, JS, README):
            self.assertTrue(path.is_file(), f"arquivo ausente: {path}")


class ConstantesEspelhadas(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""
        cls.parser = (ROOT / "controller" / "parser.h").read_text(encoding="utf-8")
        cls.driver = (ROOT / "controller_dm556d" / "driver_config.h").read_text(
            encoding="utf-8"
        )

    def _js_const(self, name: str, pattern: str = r"(\d+)") -> str:
        match = re.search(rf"\b{name}:\s*{pattern}", self.js)
        self.assertIsNotNone(match, f"{name} ausente em js/app.js")
        return match.group(1)

    def test_limites_vem_do_nucleo(self) -> None:
        pares = (
            ("MAX_DATAGRAM_BYTES", "kMaxDatagramBytes", "96"),
            ("LINK_TIMEOUT_MS", "kLinkTimeoutMs", "1200"),
            ("DEAD_BAND", "kDeadBand", "55"),
            ("MANUAL_STEP", "kManualStep", "80"),
        )
        for js_name, cpp_name, esperado in pares:
            valor = self._js_const(js_name)
            self.assertEqual(int(valor), int(esperado), js_name)
            origem = re.search(rf"{cpp_name}\s*=\s*(\d+)", self.parser)
            self.assertIsNotNone(origem, f"{cpp_name} ausente no parser.h")
            self.assertEqual(valor, origem.group(1), cpp_name)

    def test_constantes_de_movimento_vem_do_driver(self) -> None:
        pares = (
            ("MAX_TRAVEL", "LASERSEG_MAX_TRAVEL", 533),
            ("STEPS_PER_REV", "LASERSEG_STEPS_PER_REV", 3200),
        )
        for js_name, define, esperado in pares:
            self.assertEqual(int(self._js_const(js_name)), esperado, js_name)
            origem = re.search(rf"#define\s+{define}\s+(\d+)", self.driver)
            self.assertIsNotNone(origem, f"{define} ausente em driver_config.h")
            self.assertEqual(int(origem.group(1)), esperado, define)

        gain = self._js_const("TRACK_GAIN", r"([0-9.]+)")
        self.assertAlmostEqual(float(gain), 0.75)
        origem = re.search(
            r"#define\s+LASERSEG_TRACK_GAIN\s+([0-9.]+)f", self.driver
        )
        self.assertIsNotNone(origem, "LASERSEG_TRACK_GAIN ausente")
        self.assertAlmostEqual(float(gain), float(origem.group(1)))


class ContratoDeMensagens(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""

    def test_comandos_e_telemetria_presentes(self) -> None:
        for token in ("TRACK,", "LIGHT,", "MANUAL,", "T,", "ACK,"):
            self.assertIn(token, self.js, f"mensagem ausente: {token}")

    def test_maquina_de_estados_e_watchdog_presentes(self) -> None:
        for token in ("SAFE", "TRACK", "MANUAL", "linkExpired", "'LINK'"):
            self.assertIn(token, self.js, f"estado/fault ausente: {token}")

    def test_log_de_provisionamento_presente(self) -> None:
        for token in (
            "PROV AP_UP",
            "PROV SCAN",
            "PROV SAVED",
            "PROV STA_CONNECTED",
            "PROV FALLBACK_AP",
        ):
            self.assertIn(token, self.js, f"log PROV ausente: {token}")


class Seguranca(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.textos = {
            path.name: path.read_text(encoding="utf-8")
            for path in (HTML, CSS, JS, README)
            if path.is_file()
        }

    def test_ap_publica_e_unica(self) -> None:
        js = self.textos.get("app.js", "")
        self.assertIn("AP_SSID: 'laserseg'", js)
        self.assertIn("AP_PASSWORD: '123456'", js)
        for nome, texto in self.textos.items():
            for proibida in ("SIMCAM-SETUP", "simcam123", "WindFollow-Setup"):
                self.assertNotIn(proibida, texto, f"{proibida} em {nome}")

    def test_senha_da_rede_nao_tem_valor_inicial(self) -> None:
        html = self.textos.get("index.html", "")
        entradas = re.findall(r"<input[^>]*type=\"password\"[^>]*>", html)
        self.assertTrue(entradas, "campo password ausente")
        for entrada in entradas:
            self.assertNotRegex(entrada, r"\bvalue=", "senha nao deve ter value")

    def test_sem_persistencia_e_sem_innerHTML(self) -> None:
        js = self.textos.get("app.js", "")
        self.assertNotIn("localStorage", js, "credencial deve viver so em memoria")
        self.assertNotIn("sessionStorage", js)
        self.assertNotRegex(js, r"\.innerHTML\s*=", "use textContent")


class Acessibilidade(unittest.TestCase):
    def test_marcacao_basica(self) -> None:
        html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        self.assertIn('lang="pt-BR"', html)
        self.assertIn('name="viewport"', html)
        self.assertGreaterEqual(html.count("<label"), 3, "labels nos formularios")
        self.assertIn("aria-live", html, "status/log devem ser anunciados")
        self.assertIn("aria-pressed", html, "toggles devem expor estado")


if __name__ == "__main__":
    unittest.main()
