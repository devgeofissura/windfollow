"""Testes host do simulador local do ventiseg (contrato v0.1).

O teste verifica que a pagina e um espelho fiel de docs/PROTOCOLO.md e
executa o nucleo da FSM em JS com tempo determinístico (via Node + vm), sem
DOM, sem placa e sem rede.

Execute com:

    python -m unittest discover -s firmware/ventiseg/tests -p 'test_simulator.py' -v
"""

from __future__ import annotations

import json
import pathlib
import re
import shutil
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SIM = ROOT / "simulator"
HTML = SIM / "index.html"
CSS = SIM / "css/style.css"
JS = SIM / "js/app.js"
README = SIM / "README.md"
PROTOCOLO = ROOT / "docs" / "PROTOCOLO.md"

# Carrega app.js em um contexto sem `document`: so o VentisegCore e exposto.
# Todos os tempos sao injetados explicitamente — nenhum Date.now() na FSM.
NODE_HARNESS = r"""
const fs = require('fs');
const vm = require('vm');
const sandbox = { console };
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync(process.argv[1], 'utf8'), sandbox,
                { filename: 'app.js' });
const C = sandbox.VentisegCore;
const out = { constants: C.SIM };

let s = C.createState();
out.boot = { mode: s.mode, fault: s.fault, seq: s.seq, axis: s.axis,
             telemetry: C.telemetry(s) };

out.trackOn = { ok: C.feed(s, 'TRACK,1', 10), mode: s.mode,
                telemetry: C.telemetry(s) };
out.seqBeforeSample = { seq: s.seq };

out.sample = { ok: C.feed(s, 'T,0,1000,1', 20), seq: s.seq, axis: s.axis,
               telemetry: C.telemetry(s) };
const congelado = JSON.stringify(s);
out.stale = { ok: C.feed(s, 'T,0,0,1', 30),
              unchanged: JSON.stringify(s) === congelado };
out.outOfRange = { ok: C.feed(s, 'T,1,1001,1', 30),
                   unchanged: JSON.stringify(s) === congelado };
out.badTrack = { ok: C.feed(s, 'TRACK,2', 30),
                 unchanged: JSON.stringify(s) === congelado };
out.badManual = { ok: C.feed(s, 'MANUAL,2', 30),
                  unchanged: JSON.stringify(s) === congelado };

s = C.createState();
C.feed(s, 'TRACK,1', 0);
C.feed(s, 'T,0,55,1', 10);
out.deadBand = { axis: s.axis };
C.feed(s, 'T,1,56,1', 20);
out.justAbove = { axis: s.axis };

s = C.createState();
C.feed(s, 'MANUAL,1', 0);
out.manualFirst = { axis: s.axis, mode: s.mode };
for (let i = 0; i < 20; i += 1) C.feed(s, 'MANUAL,1', 10 + i);
out.manualClamped = { axis: s.axis };
C.feed(s, 'MANUAL,0', 100);
out.manualStop = { mode: s.mode, moving: s.moving };

s = C.createState();
C.feed(s, 'T,0,0,1', 100);      // so T aceito renova o watchdog
C.feed(s, 'TRACK,1', 110);      // TRACK apenas muda o modo
out.watchdogBefore = { tripped: C.checkWatchdog(s, 1300) }; // 1200 ms desde T
out.watchdog = { tripped: C.checkWatchdog(s, 1301), mode: s.mode,
                 fault: s.fault, moving: s.moving,
                 telemetry: C.telemetry(s) };
out.noRecoverTrack = { ok: C.feed(s, 'TRACK,1', 1350), fault: s.fault,
                       linkLost: s.linkLost };
out.recovery = { ok: C.feed(s, 'T,1,0,1', 1400), fault: s.fault,
                 linkLost: s.linkLost, telemetry: C.telemetry(s) };

s = C.createState();
C.feed(s, 'T,0,0,1', 0);
C.feed(s, 'TRACK,1', 1100);     // nao renova
C.feed(s, 'MANUAL,1', 1150);    // nao renova
out.noRenew = { tripped: C.checkWatchdog(s, 1201), fault: s.fault };

s = C.createState();
const longa = 'T,0,' + '1'.repeat(100) + ',1';
out.longLine = { bytes: C.byteLength(longa), ok: C.feed(s, longa, 0),
                 mode: s.mode, seq: s.seq };

s = C.createState();
C.setSimFault(s, 'Range');
out.rangeFault = { fault: s.fault, telemetry: C.telemetry(s) };

process.stdout.write(JSON.stringify(out));
"""


class Estrutura(unittest.TestCase):
    def test_arquivos_da_pagina_existem(self) -> None:
        for path in (HTML, CSS, JS, README, PROTOCOLO):
            self.assertTrue(path.is_file(), f"arquivo ausente: {path}")


class ConstantesEspelhadas(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""
        cls.doc = PROTOCOLO.read_text(encoding="utf-8") if PROTOCOLO.is_file() else ""

    def _js_const(self, name: str, pattern: str = r"(\d+)") -> str:
        match = re.search(rf"\b{name}:\s*{pattern}", self.js)
        self.assertIsNotNone(match, f"{name} ausente em js/app.js")
        return match.group(1)

    def test_valores_do_contrato_no_js(self) -> None:
        esperados = {
            "MAX_DATAGRAM_BYTES": 96,
            "LINK_TIMEOUT_MS": 1200,
            "DEAD_BAND": 55,
            "MANUAL_STEP": 80,
            "MAX_TRAVEL": 533,
        }
        for name, valor in esperados.items():
            with self.subTest(constante=name):
                self.assertEqual(int(self._js_const(name)), valor)
        self.assertAlmostEqual(
            float(self._js_const("TRACK_GAIN", r"([0-9.]+)")), 0.75)

    def test_js_nao_diverge_do_protocolo_md(self) -> None:
        pares = (
            ("kDeadBand", "DEAD_BAND"),
            ("kTrackGain", "TRACK_GAIN"),
            ("kMaxTravel", "MAX_TRAVEL"),
            ("kLinkTimeoutMs", "LINK_TIMEOUT_MS"),
        )
        for doc_name, js_name in pares:
            with self.subTest(constante=doc_name):
                origem = re.search(rf"{doc_name}\s*=\s*([0-9.]+)", self.doc)
                self.assertIsNotNone(origem, f"{doc_name} ausente no PROTOCOLO.md")
                self.assertAlmostEqual(
                    float(self._js_const(js_name, r"([0-9.]+)")),
                    float(origem.group(1)),
                )
        self.assertIn("96 bytes", self.doc)


class ContratoDeMensagens(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""
        cls.html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        cls.textos = {
            path.name: path.read_text(encoding="utf-8")
            for path in (HTML, CSS, JS, README) if path.is_file()
        }

    def test_comandos_e_telemetria_no_js(self) -> None:
        for token in ("TRACK,", "MANUAL,", "T,", "ACK,"):
            with self.subTest(token=token):
                self.assertIn(token, self.js)

    def test_estados_e_falhas_exatos(self) -> None:
        for token in ("'SAFE'", "'TRACK'", "'MANUAL'",
                      "'None'", "'Lnk'", "'Range'"):
            with self.subTest(token=token):
                self.assertIn(token, self.js)

    def test_formatos_no_painel_de_protocolo(self) -> None:
        for token in (
            "TRACK,&lt;0|1&gt;",
            "MANUAL,&lt;-1|0|1&gt;",
            "T,&lt;seq&gt;,&lt;x&gt;,&lt;valid&gt;",
            "ACK,&lt;seq&gt;,&lt;state&gt;,&lt;fault&gt;",
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.html)

    def test_sem_campos_de_outros_projetos(self) -> None:
        proibidos = ("LIGHT,", "LASER,", "TILT", "LED_EN", "'LINK'",
                     "getUserMedia", "pinMode", "digitalWrite")
        for nome, texto in self.textos.items():
            for token in proibidos:
                with self.subTest(arquivo=nome, token=token):
                    self.assertNotIn(token, texto, f"{token} em {nome}")


class MaquinaDeEstadosDeterministica(unittest.TestCase):
    """Executa a FSM real do app.js com now_ms controlado, sem DOM."""

    @classmethod
    def setUpClass(cls) -> None:
        if shutil.which("node") is None:
            raise unittest.SkipTest("node indisponível para executar a FSM JS")
        resultado = subprocess.run(
            ["node", "-e", NODE_HARNESS, str(JS)],
            capture_output=True, text=True, check=True,
        )
        cls.out = json.loads(resultado.stdout)

    def test_boot_seguro(self) -> None:
        boot = self.out["boot"]
        self.assertEqual(boot["mode"], "SAFE")
        self.assertEqual(boot["fault"], "None")
        self.assertEqual(boot["seq"], -1)
        self.assertEqual(boot["axis"], 0)
        self.assertEqual(boot["telemetry"], "ACK,-1,SAFE,None")

    def test_track_e_amostra_aceita(self) -> None:
        self.assertTrue(self.out["trackOn"]["ok"])
        self.assertEqual(self.out["trackOn"]["mode"], "TRACK")
        self.assertEqual(self.out["seqBeforeSample"]["seq"], -1)
        sample = self.out["sample"]
        self.assertTrue(sample["ok"])
        self.assertEqual(sample["seq"], 0)
        self.assertEqual(sample["axis"], 399)  # trunc(1000*533*0.75/1000)
        self.assertEqual(sample["telemetry"], "ACK,0,TRACK,None")

    def test_comandos_invalidos_nao_mudam_estado(self) -> None:
        for nome in ("stale", "outOfRange", "badTrack", "badManual"):
            with self.subTest(caso=nome):
                caso = self.out[nome]
                self.assertFalse(caso["ok"])
                self.assertTrue(caso["unchanged"])

    def test_zona_morta_e_ganho(self) -> None:
        self.assertEqual(self.out["deadBand"]["axis"], 0)   # x = 55
        self.assertEqual(self.out["justAbove"]["axis"], 22)  # x = 56

    def test_manual_passo_e_limite(self) -> None:
        self.assertEqual(self.out["manualFirst"]["axis"], 80)
        self.assertEqual(self.out["manualFirst"]["mode"], "MANUAL")
        self.assertEqual(self.out["manualClamped"]["axis"], 533)
        self.assertEqual(self.out["manualStop"]["mode"], "SAFE")
        self.assertFalse(self.out["manualStop"]["moving"])

    def test_watchdog_e_recuperacao(self) -> None:
        self.assertFalse(self.out["watchdogBefore"]["tripped"])  # 1200 ms
        watchdog = self.out["watchdog"]
        self.assertTrue(watchdog["tripped"])  # 1201 ms
        self.assertEqual(watchdog["mode"], "SAFE")
        self.assertEqual(watchdog["fault"], "Lnk")
        self.assertFalse(watchdog["moving"])
        self.assertEqual(watchdog["telemetry"], "ACK,0,SAFE,Lnk")
        # TRACK nao limpa fault=Lnk: so T aceito recupera o link.
        noRecover = self.out["noRecoverTrack"]
        self.assertTrue(noRecover["ok"])
        self.assertEqual(noRecover["fault"], "Lnk")
        self.assertTrue(noRecover["linkLost"])
        recuperou = self.out["recovery"]
        self.assertTrue(recuperou["ok"])
        self.assertEqual(recuperou["fault"], "None")
        self.assertFalse(recuperou["linkLost"])
        self.assertEqual(recuperou["telemetry"], "ACK,1,TRACK,None")

    def test_track_e_manual_nao_renovam_watchdog(self) -> None:
        noRenew = self.out["noRenew"]
        self.assertTrue(noRenew["tripped"])
        self.assertEqual(noRenew["fault"], "Lnk")

    def test_datagrama_acima_de_96_bytes(self) -> None:
        longa = self.out["longLine"]
        self.assertGreater(longa["bytes"], 96)
        self.assertFalse(longa["ok"])
        self.assertEqual(longa["mode"], "SAFE")
        self.assertEqual(longa["seq"], -1)

    def test_falha_range_na_telemetria(self) -> None:
        self.assertEqual(self.out["rangeFault"]["fault"], "Range")
        self.assertEqual(self.out["rangeFault"]["telemetry"], "ACK,-1,SAFE,Range")


class Seguranca(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.textos = {
            path.name: path.read_text(encoding="utf-8")
            for path in (HTML, CSS, JS, README) if path.is_file()
        }

    def test_sem_persistencia_ou_inner_html(self) -> None:
        js = self.textos.get("app.js", "")
        for proibido in ("localStorage", "sessionStorage", "fetch(", "XMLHttpRequest"):
            self.assertNotIn(proibido, js, proibido)
        self.assertNotRegex(js, r"\.innerHTML\s*=", "use textContent")

    def test_sem_recursos_externos(self) -> None:
        html = self.textos.get("index.html", "")
        self.assertNotRegex(html, r'(?:src|href)="https?://', "recurso externo")
        self.assertIn('<script src="js/app.js"></script>', html)
        self.assertIn('<link rel="stylesheet" href="css/style.css">', html)

    def test_badge_e_gpio_desligado(self) -> None:
        html = self.textos.get("index.html", "")
        self.assertIn("SIMULADOR", html)
        self.assertIn("VENTISEG_ENABLE_GPIO = 0", html)
        self.assertIn("file://", self.textos.get("README.md", ""))


class Acessibilidade(unittest.TestCase):
    def test_marcacao_basica(self) -> None:
        html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        self.assertIn('lang="pt-BR"', html)
        self.assertIn('name="viewport"', html)
        self.assertIn("aria-live", html)
        self.assertIn("aria-pressed", html)
        self.assertIn('role="tablist"', html)
        # Ventiseg tem um unico campo de formulario; o que importa e que ele
        # esteja rotulado e associado ao input.
        self.assertGreaterEqual(html.count("<label"), 1)
        self.assertIn('<label for="cmd-line">', html)
        self.assertIn('id="cmd-line"', html)


class PinagemSomenteLeitura(unittest.TestCase):
    """A aba Pinagem replica a serigrafia sem virar controle de hardware."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        inicio = cls.html.find('<section id="tab-pinagem"')
        fim = cls.html.find("</section>", inicio) if inicio >= 0 else -1
        cls.painel = cls.html[inicio:fim] if fim > inicio else ""

    def test_aba_existe_e_e_somente_leitura(self) -> None:
        self.assertIn('data-tab="pinagem"', self.html)
        self.assertIn('aria-labelledby="tabbtn-pinagem"', self.html)
        self.assertTrue(self.painel, "painel de pinagem ausente ou vazio")
        self.assertIn("somente leitura", self.painel)
        for proibido in ("<button", "<input", "<select", "<textarea",
                         "onclick", "innerHTML", "pinMode", "digitalWrite"):
            with self.subTest(proibido=proibido):
                self.assertNotIn(proibido, self.painel)

    def test_pinagem_confirmada(self) -> None:
        for token in ("STEP (PUL)", "IO3 / GPIO3", "IO21 / GPIO21", "DIR",
                      "IO45 / GPIO45", "IO46 / GPIO46", "strapping",
                      "GND", "3V3", "3,3 V", "DM556D", "24 VDC"):
            with self.subTest(token=token):
                self.assertIn(token, self.painel)

    def test_sem_hardware_adicional(self) -> None:
        for token in ("fim de curso", "relé", "iluminador", "laser", "tilt"):
            with self.subTest(token=token):
                self.assertIn(token, self.painel)
        self.assertIn("VENTISEG_ENABLE_GPIO = 0", self.painel)

    def test_todas_as_abas_tem_painel_correspondente(self) -> None:
        for nome in re.findall(r'data-tab="([^"]+)"', self.html):
            with self.subTest(aba=nome):
                self.assertIn(f'id="tab-{nome}"', self.html)


if __name__ == "__main__":
    unittest.main()
