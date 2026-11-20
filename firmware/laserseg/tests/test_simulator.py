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


class PinoutSomenteLeitura(unittest.TestCase):
    """A aba Pinout espelha o mapa primario de 2026-11-20 sem virar controle."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        inicio = cls.html.find('<section id="tab-pinout"')
        fim = cls.html.find("</section>", inicio) if inicio >= 0 else -1
        cls.painel = cls.html[inicio:fim] if fim > inicio else ""
        cls.yaml_path = ROOT / "docs" / "wireviz" / "no-visao-s3-camsim.yaml"
        cls.yaml = (cls.yaml_path.read_text(encoding="utf-8")
                    if cls.yaml_path.is_file() else "")

    def test_aba_pinout_existe(self) -> None:
        self.assertIn('data-tab="pinout"', self.html)
        self.assertIn('<section id="tab-pinout"', self.html)
        self.assertIn('aria-labelledby="tabbtn-pinout"', self.html)
        self.assertTrue(self.painel, "painel de pinout ausente ou vazio")

    def test_todas_as_abas_tem_painel_correspondente(self) -> None:
        for nome in re.findall(r'data-tab="([^"]+)"', self.html):
            with self.subTest(aba=nome):
                self.assertIn(f'id="tab-{nome}"', self.html)

    def test_gpio_livres_e_uart_do_conector_jst(self) -> None:
        for pino in ("GPIO 3", "GPIO 21", "GPIO 45", "GPIO 46",
                     "GPIO 43", "GPIO 44"):
            with self.subTest(pino=pino):
                self.assertIn(pino, self.painel)

    def test_perifericos_internos_presentes(self) -> None:
        # Camera, microfone, microSD e mPCIe: todos os GPIOs ocupados.
        for token in ("GPIO 4", "GPIO 5", "GPIO 6", "GPIO 7", "GPIO 13",
                      "GPIO 14", "GPIO 11", "GPIO 9", "GPIO 8", "GPIO 10",
                      "GPIO 12", "GPIO 17", "GPIO 16", "GPIO 15", "GPIO 18",
                      "GPIO 41", "GPIO 42", "GPIO 2", "GPIO 47", "GPIO 38",
                      "GPIO 39", "GPIO 40", "GPIO 48", "GPIO 35"):
            with self.subTest(token=token):
                self.assertIn(token, self.painel)

    def test_divergencias_nao_sao_escondidas(self) -> None:
        for token in ("NAO CONFIRMADO", "strapping", "3,3 V",
                      "OV2640", "OV5640", "IR-CUT"):
            with self.subTest(token=token):
                self.assertIn(token, self.painel)

    def test_painel_e_somente_leitura(self) -> None:
        for proibido in ("<button", "<input", "<select", "<textarea",
                         "onclick", "innerHTML", "pinMode", "digitalWrite"):
            with self.subTest(proibido=proibido):
                self.assertNotIn(proibido, self.painel)
        self.assertIn("somente leitura", self.painel)

    def test_pares_sinal_gpio_do_yaml_aparecem_no_painel(self) -> None:
        # Compara pares sinal->GPIO, nao apenas numeros: trocar SIOD/SDA de 4
        # para 5 deve falhar. J_DIVERGENCIAS fica fora (texto, nao pinos).
        pinos = self.yaml.split("J_DIVERGENCIAS:", 1)[0]
        esperados: list[tuple[str, int]] = []
        for rotulo in re.findall(r'^\s*-\s+"([^"]+)"', pinos, flags=re.M):
            match = re.search(r"GPIO\s*(\d+)", rotulo)
            if not match:
                continue
            sinal = re.sub(r"\s+", " ", rotulo[:match.start()])
            sinal = sinal.strip().rstrip("/").strip().lower()
            esperados.append((sinal, int(match.group(1))))
        self.assertTrue(esperados, "pinout YAML sem pares sinal/GPIO")

        linhas: list[tuple[str, set[int]]] = []
        for bruto in re.findall(r"<tr[^>]*>(.*?)</tr>", self.painel, flags=re.S):
            celulas = [re.sub(r"<[^>]+>", "", c).strip()
                       for c in re.findall(r"<td[^>]*>(.*?)</td>", bruto, flags=re.S)]
            texto = " ".join(re.sub(r"GPIO\s*\d+", "", c) for c in celulas)
            gpios = {int(n) for c in celulas for n in re.findall(r"GPIO\s*(\d+)", c)}
            linhas.append((re.sub(r"\s+", " ", texto).strip().lower(), gpios))

        for sinal, gpio in esperados:
            with self.subTest(sinal=sinal, gpio=gpio):
                self.assertTrue(
                    any(gpio in gpios and sinal in texto for texto, gpios in linhas),
                    f"par {sinal} -> GPIO {gpio} do YAML ausente no painel")

    def test_painel_aponta_para_a_fonte_wireviz(self) -> None:
        self.assertIn("docs/wireviz/no-visao-s3-camsim.yaml", self.painel)
        self.assertTrue(self.yaml_path.is_file(), "YAML WireViz ausente")


if __name__ == "__main__":
    unittest.main()
