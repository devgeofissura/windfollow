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

// Estado "nucleo": exclui fault, que muda em rejeicao por faixa.
function core(s) {
  return JSON.stringify({ mode: s.mode, seq: s.seq, axis: s.axis,
    valid: s.valid, moving: s.moving, lastAcceptedMs: s.lastAcceptedMs,
    linkLost: s.linkLost });
}

let s = C.createState();
out.boot = { mode: s.mode, fault: s.fault, seq: s.seq, axis: s.axis,
             telemetry: C.telemetry(s) };

out.trackOn = { ok: C.feed(s, 'TRACK,1', 10), mode: s.mode,
                telemetry: C.telemetry(s) };
out.seqBeforeSample = { seq: s.seq };

out.sample = { ok: C.feed(s, 'T,0,1000,1', 20), seq: s.seq, axis: s.axis,
               telemetry: C.telemetry(s) };
const congelado = core(s);
out.stale = { ok: C.feed(s, 'T,0,0,1', 30),
              coreUnchanged: core(s) === congelado,
              reason: C.lastReason(), fault: s.fault };
out.outOfRange = { ok: C.feed(s, 'T,1,1001,1', 30),
                   coreUnchanged: core(s) === congelado,
                   reason: C.lastReason(), fault: s.fault };
out.badTrack = { ok: C.feed(s, 'TRACK,2', 30),
                 coreUnchanged: core(s) === congelado,
                 reason: C.lastReason(), fault: s.fault };
out.badManual = { ok: C.feed(s, 'MANUAL,2', 30),
                  coreUnchanged: core(s) === congelado,
                  reason: C.lastReason(), fault: s.fault };

s = C.createState();
out.seqCeiling = { ok: C.feed(s, 'T,2147483648,0,1', 0),
                   reason: C.lastReason(), fault: s.fault, seq: s.seq };
s = C.createState();
out.seqMax = { ok: C.feed(s, 'T,2147483647,0,1', 0),
               reason: C.lastReason(), seq: s.seq };

s = C.createState();
out.leadingZeros = {
  track: C.feed(s, 'TRACK,01', 0), trackMode: s.mode,
  manual: C.feed(s, 'MANUAL,01', 10), manualMode: s.mode, manualAxis: s.axis,
  sample: C.feed(s, 'T,007,-01,01', 20), sampleSeq: s.seq,
  sampleX: s.targetX, reason: C.lastReason(),
};

s = C.createState();
out.ageExact = { ok: C.feed(s, 'TRACK,1', 2200, 1000),
                 reason: C.lastReason(), mode: s.mode };
s = C.createState();
out.agePast = { ok: C.feed(s, 'TRACK,1', 2201, 1000),
                reason: C.lastReason(), mode: s.mode, fault: s.fault };
s = C.createState();
out.ageNegative = { ok: C.feed(s, 'MANUAL,1', 999, 1000),
                    reason: C.lastReason(), mode: s.mode, axis: s.axis };
s = C.createState();
out.ageSample = { ok: C.feed(s, 'T,0,0,1', 5000, 1000),
                  reason: C.lastReason(), seq: s.seq,
                  lastAcceptedMs: s.lastAcceptedMs };
s = C.createState();
out.ageRenew = { ok: C.feed(s, 'T,0,0,1', 2000, 1000),
                 lastAcceptedMs: s.lastAcceptedMs,
                 beforeLimit: C.checkWatchdog(s, 2200),
                 pastLimit: C.checkWatchdog(s, 2201) };
s = C.createState();
out.ageValidManual = { ok: C.feed(s, 'MANUAL,1', 2200, 1000),
                       reason: C.lastReason(), axis: s.axis };

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
                       linkLost: s.linkLost, mode: s.mode,
                       moving: s.moving, telemetry: C.telemetry(s) };
out.rangeWhileLost = { ok: C.feed(s, 'TRACK,2', 1360), fault: s.fault };
out.recovery = { ok: C.feed(s, 'T,1,0,1', 1400), fault: s.fault,
                 linkLost: s.linkLost, mode: s.mode, moving: s.moving,
                 telemetry: C.telemetry(s) };
out.retrack = { ok: C.feed(s, 'TRACK,1', 1410), mode: s.mode,
                moving: s.moving, telemetry: C.telemetry(s) };

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

# Nucleo de deteccao sintetica: mesmo padrao vm sem DOM da FSM acima.
DETECTION_HARNESS = r"""
const fs = require('fs');
const vm = require('vm');
const sandbox = { console };
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync(process.argv[1], 'utf8'), sandbox,
                 { filename: 'app.js' });
const C = sandbox.VentisegCore;
const out = { constants: C.CAM };

let d = C.createDetection();
out.boot = C.detectionSnapshot(d);

C.detectionSetBearing(d, 1000);
out.right = C.detectionSnapshot(d);

C.detectionSetPan(d, 399);
out.compensated = C.detectionSnapshot(d);

C.detectionSetBearing(d, 50);
C.detectionSetPan(d, 0);
out.dead = C.detectionSnapshot(d);

C.detectionSetBearing(d, 1000);
C.detectionSetPan(d, -533);
out.offframe = C.detectionSnapshot(d);

C.detectionSetBearing(d, 0);
C.detectionSetPan(d, 0);
C.detectionSetSeen(d, false);
out.dropout = C.detectionSnapshot(d);

d = C.createDetection();
C.detectionSetBearing(d, 1000);
out.closed = [];
for (let i = 0; i < 6; i += 1) {
  C.detectionCloseLoop(d);
  const v = C.detectionSnapshot(d);
  out.closed.push({ pan: v.pan_steps, apparent: v.apparent_x,
                    error: v.error_x, target: v.pan_target_steps });
}

d = C.createDetection();
C.detectionSetBearing(d, 5000);
out.clampBearing = C.detectionSnapshot(d).bearing_x;
C.detectionSetPan(d, -9999);
out.clampPan = C.detectionSnapshot(d).pan_steps;

// Proxy de webcam: funcao pura, mesma usada pela UI com arrays de cinza.
const W = C.WEBCAM;
const bw = W.SAMPLE_W;
const bh = W.SAMPLE_H;
const fundo = new Uint8Array(bw * bh).fill(20);
function comBloco(x0, valor, lado) {
  const quadro = new Uint8Array(bw * bh).fill(20);
  for (let y = 10; y < 10 + lado; y += 1) {
    for (let x = x0; x < x0 + lado; x += 1) quadro[y * bw + x] = valor;
  }
  return quadro;
}
out.webcam = {
  constants: W,
  right: C.webcamForegroundX(fundo, comBloco(30, 200, 5), bw, bh, W.DIFF_THRESHOLD),
  left: C.webcamForegroundX(fundo, comBloco(6, 200, 5), bw, bh, W.DIFF_THRESHOLD),
  none: C.webcamForegroundX(fundo, fundo, bw, bh, W.DIFF_THRESHOLD),
  global: C.webcamForegroundX(fundo, new Uint8Array(bw * bh).fill(200),
                              bw, bh, W.DIFF_THRESHOLD),
  tiny: C.webcamForegroundX(fundo, comBloco(20, 200, 2), bw, bh, W.DIFF_THRESHOLD),
};

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
                     "pinMode", "digitalWrite")
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
                self.assertTrue(caso["coreUnchanged"])
        self.assertEqual(self.out["stale"]["reason"], "sequence")
        self.assertEqual(self.out["stale"]["fault"], "None")
        for nome in ("outOfRange", "badTrack", "badManual"):
            with self.subTest(caso=nome):
                self.assertEqual(self.out[nome]["reason"], "range")
                self.assertEqual(self.out[nome]["fault"], "Range")

    def test_teto_de_seq_e_int32_max(self) -> None:
        ceiling = self.out["seqCeiling"]
        self.assertFalse(ceiling["ok"])
        self.assertEqual(ceiling["reason"], "range")
        self.assertEqual(ceiling["fault"], "Range")
        self.assertEqual(ceiling["seq"], -1)
        maximo = self.out["seqMax"]
        self.assertTrue(maximo["ok"])
        self.assertEqual(maximo["seq"], 2147483647)

    def test_zeros_a_esquerda_sao_aceitos(self) -> None:
        zeros = self.out["leadingZeros"]
        self.assertTrue(zeros["track"])
        self.assertEqual(zeros["trackMode"], "TRACK")
        self.assertTrue(zeros["manual"])
        self.assertEqual(zeros["manualMode"], "MANUAL")
        self.assertEqual(zeros["manualAxis"], 80)
        self.assertTrue(zeros["sample"])
        self.assertEqual(zeros["sampleSeq"], 7)
        self.assertEqual(zeros["sampleX"], -1)

    def test_idade_da_mensagem(self) -> None:
        self.assertTrue(self.out["ageExact"]["ok"])          # 1200 ms
        self.assertEqual(self.out["ageExact"]["mode"], "TRACK")
        past = self.out["agePast"]                            # 1201 ms
        self.assertFalse(past["ok"])
        self.assertEqual(past["reason"], "age")
        self.assertEqual(past["mode"], "SAFE")
        self.assertEqual(past["fault"], "None")
        negative = self.out["ageNegative"]
        self.assertFalse(negative["ok"])
        self.assertEqual(negative["reason"], "age")
        self.assertEqual(negative["axis"], 0)
        stale_sample = self.out["ageSample"]
        self.assertFalse(stale_sample["ok"])
        self.assertEqual(stale_sample["reason"], "age")
        self.assertEqual(stale_sample["seq"], -1)
        self.assertIsNone(stale_sample["lastAcceptedMs"])
        self.assertTrue(self.out["ageValidManual"]["ok"])
        self.assertEqual(self.out["ageValidManual"]["axis"], 80)

    def test_watchdog_renova_com_rx_ms(self) -> None:
        renew = self.out["ageRenew"]
        self.assertTrue(renew["ok"])
        self.assertEqual(renew["lastAcceptedMs"], 1000)
        self.assertFalse(renew["beforeLimit"])
        self.assertTrue(renew["pastLimit"])

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
        # TRACK e valido, mas inibido: nenhum movimento enquanto Lnk.
        noRecover = self.out["noRecoverTrack"]
        self.assertTrue(noRecover["ok"])
        self.assertEqual(noRecover["fault"], "Lnk")
        self.assertTrue(noRecover["linkLost"])
        self.assertEqual(noRecover["mode"], "SAFE")
        self.assertFalse(noRecover["moving"])
        self.assertEqual(noRecover["telemetry"], "ACK,0,SAFE,Lnk")
        # Range posterior nao apaga Lnk.
        self.assertFalse(self.out["rangeWhileLost"]["ok"])
        self.assertEqual(self.out["rangeWhileLost"]["fault"], "Lnk")
        # So T aceito recupera o link, e ainda exige novo comando de movimento.
        recuperou = self.out["recovery"]
        self.assertTrue(recuperou["ok"])
        self.assertEqual(recuperou["fault"], "None")
        self.assertFalse(recuperou["linkLost"])
        self.assertEqual(recuperou["mode"], "SAFE")
        self.assertFalse(recuperou["moving"])
        self.assertEqual(recuperou["telemetry"], "ACK,1,SAFE,None")
        self.assertEqual(self.out["retrack"]["telemetry"],
                         "ACK,1,TRACK,None")

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

    def test_webcam_sem_gravacao_ou_rede(self) -> None:
        js = self.textos.get("app.js", "")
        for proibido in ("MediaRecorder", "WebSocket", "RTCPeerConnection",
                         "ImageCapture", "encodeURIComponent"):
            with self.subTest(proibido=proibido):
                self.assertNotIn(proibido, js)

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


class DeteccaoSintetica(unittest.TestCase):
    """Geometria da deteccao sintetica executada no nucleo real do app.js."""

    @classmethod
    def setUpClass(cls) -> None:
        if shutil.which("node") is None:
            raise unittest.SkipTest("node indisponível para executar a deteccao JS")
        resultado = subprocess.run(
            ["node", "-e", DETECTION_HARNESS, str(JS)],
            capture_output=True, text=True, check=True,
        )
        cls.out = json.loads(resultado.stdout)

    def test_constantes_do_quadro(self) -> None:
        c = self.out["constants"]
        self.assertEqual(c["FRAME_W"], 320)
        self.assertEqual(c["FRAME_H"], 240)
        self.assertEqual(c["CENTER_BAND_PX"], 32)
        self.assertAlmostEqual(c["PAN_X_SCALE"], 1000 / 533)

    def test_boot_centralizado(self) -> None:
        v = self.out["boot"]
        self.assertEqual(v["bearing_x"], 0)
        self.assertEqual(v["pan_steps"], 0)
        self.assertTrue(v["seen"])
        self.assertEqual(v["apparent_x"], 0)
        self.assertTrue(v["detected"])
        self.assertEqual(v["error_x"], 0)
        self.assertEqual(v["error_px"], 0)
        self.assertEqual(v["pan_error_steps"], 0)
        self.assertEqual(v["pan_target_steps"], 0)
        self.assertEqual(v["pixel_x"], 160)

    def test_pessoa_a_direita_gera_erro_e_alvo(self) -> None:
        v = self.out["right"]
        self.assertEqual(v["pan_x"], 0)
        self.assertEqual(v["apparent_x"], 1000)
        self.assertTrue(v["detected"])
        self.assertEqual(v["error_x"], 1000)
        self.assertEqual(v["error_px"], 160)
        self.assertEqual(v["pan_error_steps"], 399)
        self.assertEqual(v["pan_target_steps"], 399)
        self.assertEqual(v["pixel_x"], 320)

    def test_pan_reduz_o_erro_por_compensacao(self) -> None:
        v = self.out["compensated"]
        self.assertEqual(v["pan_steps"], 399)
        self.assertEqual(v["pan_x"], 749)
        self.assertEqual(v["apparent_x"], 251)
        self.assertEqual(v["error_x"], 251)
        self.assertEqual(v["pan_error_steps"], 100)
        self.assertEqual(v["pan_target_steps"], 499)
        self.assertEqual(v["pixel_x"], 200)

    def test_zona_morta_nao_gera_alvo(self) -> None:
        v = self.out["dead"]
        self.assertEqual(v["apparent_x"], 50)
        self.assertEqual(v["error_x"], 50)  # erro e medido, alvo e zero
        self.assertEqual(v["pan_error_steps"], 0)
        self.assertEqual(v["pan_target_steps"], 0)

    def test_pessoa_fora_do_quadro_perde_deteccao(self) -> None:
        v = self.out["offframe"]
        self.assertEqual(v["pan_x"], -1000)
        self.assertEqual(v["apparent_x"], 1000)
        self.assertFalse(v["detected"])
        self.assertEqual(v["error_x"], 0)
        self.assertEqual(v["pan_error_steps"], 0)
        self.assertEqual(v["pan_target_steps"], -533)

    def test_queda_de_deteccao_zerra_erro_e_alvo(self) -> None:
        v = self.out["dropout"]
        self.assertFalse(v["seen"])
        self.assertFalse(v["detected"])
        self.assertEqual(v["error_x"], 0)
        self.assertEqual(v["error_px"], 0)
        self.assertEqual(v["pan_target_steps"], 0)

    def test_malha_fechada_converge_na_zona_morta(self) -> None:
        passos = self.out["closed"]
        self.assertEqual(len(passos), 6)
        self.assertEqual(passos[0]["pan"], 399)
        self.assertEqual(passos[1]["pan"], 499)
        self.assertEqual(passos[2]["pan"], 524)
        self.assertEqual(passos[2]["apparent"], 17)
        self.assertLessEqual(abs(passos[2]["apparent"]), 55)
        for ultimo in passos[3:]:
            self.assertEqual(ultimo["pan"], 524)
            self.assertEqual(ultimo["target"], 524)

    def test_limites_de_bearing_e_pan(self) -> None:
        self.assertEqual(self.out["clampBearing"], 1000)
        self.assertEqual(self.out["clampPan"], -533)

    def test_proxy_de_webcam_e_puro_e_limpo(self) -> None:
        w = self.out["webcam"]
        self.assertEqual(w["constants"]["SAMPLE_W"], 40)
        self.assertEqual(w["constants"]["SAMPLE_H"], 30)
        self.assertGreater(w["right"], 300)
        self.assertLess(w["left"], -300)
        for chave in ("right", "left"):
            with self.subTest(chave=chave):
                self.assertLessEqual(abs(w[chave]), 1000)
        for chave in ("none", "global", "tiny"):
            with self.subTest(chave=chave):
                self.assertIsNone(w[chave])


class EstruturaDeteccao(unittest.TestCase):
    """A aba de deteccao existe, e rotulada e nao toca hardware real."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        cls.css = CSS.read_text(encoding="utf-8") if CSS.is_file() else ""
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""
        inicio = cls.html.find('<section id="tab-deteccao"')
        fim = cls.html.find("</section>", inicio) if inicio >= 0 else -1
        cls.painel = cls.html[inicio:fim] if fim > inicio else ""

    def test_aba_existe_e_webcam_e_opcional(self) -> None:
        self.assertIn('data-tab="deteccao"', self.html)
        self.assertIn('id="tabbtn-deteccao"', self.html)
        self.assertTrue(self.painel, "painel de deteccao ausente ou vazio")
        self.assertIn("sintético", self.painel)
        self.assertIn("webcam", self.painel.lower())
        self.assertIn("Não há gravação", self.painel)

    def test_ids_e_estilos_presentes(self) -> None:
        ids = (
            "cam-frame", "cam-video", "cam-start", "cam-calibrate",
            "cam-stop", "cam-status", "cam-person", "cam-lost",
            "det-status", "det-bearing", "det-pan", "det-pan-x",
            "det-apparent", "det-error", "det-error-px", "det-target",
            "det-bearing-slider", "det-pan-slider", "det-seen",
            "det-closed-loop",
        )
        for ident in ids:
            with self.subTest(id=ident):
                self.assertIn(f'id="{ident}"', self.html)
                self.assertIn(ident, self.js)
        for classe in ("cam-frame", "cam-video", "cam-person",
                       "cam-lost", "cam-band"):
            with self.subTest(classe=classe):
                self.assertIn("." + classe, self.css)

    def test_webcam_local_sem_hardware(self) -> None:
        for proibido in ("ImageCapture", "MediaRecorder", "WebSocket",
                         "pinMode", "digitalWrite", "fetch(", "localStorage"):
            with self.subTest(proibido=proibido):
                self.assertNotIn(proibido, self.painel)
        self.assertIn("GPIO", self.painel)
        self.assertIn("comando de protocolo", self.painel)
        self.assertIn("navigator.mediaDevices.getUserMedia", self.js)
        self.assertIn(".stop()", self.js)


class ReforcoDaInterface(unittest.TestCase):
    """Navegacao por teclado, foco e contraste nos dois temas."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.html = HTML.read_text(encoding="utf-8") if HTML.is_file() else ""
        cls.css = CSS.read_text(encoding="utf-8") if CSS.is_file() else ""
        cls.js = JS.read_text(encoding="utf-8") if JS.is_file() else ""

    def test_tabs_navegaveis_por_teclado(self) -> None:
        self.assertIn('aria-describedby="tabs-help"', self.html)
        self.assertIn('id="tabs-help" class="visually-hidden"', self.html)
        self.assertGreaterEqual(self.html.count('tabindex="-1"'), 4)
        self.assertIn('tabindex="0"', self.html)
        for token in ("ArrowRight", "ArrowLeft", "Home", "End", "tabIndex"):
            with self.subTest(token=token):
                self.assertIn(token, self.js)

    def test_skip_link_e_regioes_rolaveis(self) -> None:
        self.assertIn('class="skip-link" href="#conteudo"', self.html)
        self.assertIn('<main id="conteudo">', self.html)
        self.assertIn('role="log"', self.html)
        self.assertIn('class="table-scroll"', self.html)
        for classe in ("visually-hidden", "skip-link", "table-scroll"):
            with self.subTest(classe=classe):
                self.assertIn("." + classe, self.css)

    def test_webcam_com_status_e_video_rotulado(self) -> None:
        self.assertIn('id="cam-status" role="status" aria-live="polite"', self.html)
        self.assertIn('aria-label="Vídeo local da webcam', self.html)
        for ident in ("cam-start", "cam-calibrate", "cam-stop"):
            with self.subTest(id=ident):
                self.assertIn(f'id="{ident}"', self.html)

    def test_valores_dos_sliders_e_foco_do_dialogo(self) -> None:
        for ident in ("det-bearing-value", "det-pan-value"):
            with self.subTest(id=ident):
                self.assertIn(f'id="{ident}"', self.html)
                self.assertIn(ident, self.js)
        self.assertIn('aria-describedby="gate-dialog-note"', self.html)
        self.assertIn("addEventListener('close'", self.js)
        for token in ("--color-warning-text", "--color-success-text"):
            with self.subTest(token=token):
                self.assertGreaterEqual(self.css.count(token), 3)


if __name__ == "__main__":
    unittest.main()
