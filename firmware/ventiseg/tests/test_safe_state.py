"""Diferencial de sessao: comportamento seguro depois de fault=Lnk.

As tres implementacoes (Python, JavaScript e C++ ) recebem o mesmo roteiro de
comandos e poll do watchdog. Cada passo produz um trace com telemetria, modo,
fault, eixo, movimento e estado do link; os tres traces devem ser iguais.

Regra definida em docs/PROTOCOLO.md: apos `poll` detectar 1200 ms sem `T`
aceito, o eixo fica em SAFE/parado com `fault=Lnk`. `TRACK` e `MANUAL`
continuam sendo comandos sintaticamente validos, mas sao **inibidos** pela
sessao enquanto o link estiver perdido; `Range` nao apaga `Lnk`. Somente um
`T` aceito restaura o link, e ainda exige um novo `TRACK`/`MANUAL` para
voltar a comandar movimento.
"""

from __future__ import annotations

import json
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

TESTS = pathlib.Path(__file__).resolve().parent
ROOT = TESTS.parent
sys.path.insert(0, str(TESTS))
sys.path.insert(0, str(ROOT / "tools"))

import protocol_ref as reference  # noqa: E402
from test_differential import compile_cxx_harness, JS_HARNESS  # noqa: E402

APP_JS = ROOT / "simulator" / "js" / "app.js"
CXX_HARNESS = TESTS / "differential_cxx_harness.cpp"


def feed(data: bytes, rx: int, now: int | None = None) -> dict:
    return {"op": "feed", "datagram": data, "rx": rx, "now": now}


def poll(now: int) -> dict:
    return {"op": "poll", "now": now}


SCRIPTS = [
    {
        "name": "lnk_inibe_track_e_manual",
        "steps": [
            feed(b"TRACK,1", 10),
            feed(b"T,1,100,1", 11),
            poll(1300),
            feed(b"TRACK,1", 1400),
            feed(b"MANUAL,1", 1410),
            feed(b"TRACK,2", 1420),
        ],
    },
    {
        "name": "recuperacao_exige_novo_comando",
        "steps": [
            feed(b"TRACK,1", 10),
            feed(b"T,1,100,1", 11),
            poll(1300),
            feed(b"T,2,100,1", 1400),
            feed(b"TRACK,1", 1410),
            feed(b"T,3,100,1", 1420),
            poll(2621),
        ],
    },
    {
        "name": "track_e_manual_nao_renovam",
        "steps": [
            feed(b"T,1,0,1", 100),
            feed(b"TRACK,1", 1100),
            feed(b"MANUAL,1", 1150),
            poll(1300),
            feed(b"T,2,0,1", 1310),
            poll(2510),
        ],
    },
]


def snapshot(mode: str, fault: str, seq: int, axis: int, moving: bool,
             link_lost: bool, telemetry: str, result: bool) -> dict:
    return {
        "result": bool(result),
        "telemetry": telemetry,
        "mode": mode,
        "fault": fault,
        "seq": seq,
        "axis": axis,
        "moving": bool(moving),
        "linkLost": bool(link_lost),
    }


def python_traces() -> list[dict]:
    out = []
    for script in SCRIPTS:
        session = reference.Session()
        trace = []
        for step in script["steps"]:
            if step["op"] == "feed":
                now = step["now"]
                result = session.feed(
                    step["datagram"], rx_ms=step["rx"],
                    now_ms=None if now is None else now,
                ).accepted
            else:
                result = session.poll(now_ms=step["now"])
            trace.append(snapshot(
                session.mode, session.fault, session.seq, session.axis,
                session.moving, session.link_lost,
                reference.format_telemetry(session), result,
            ))
        out.append({"name": script["name"], "trace": trace})
    return out


def js_traces() -> list[dict]:
    node = shutil.which("node")
    if not node:
        raise RuntimeError("node nao encontrado no PATH")
    payload = [
        {
            "name": script["name"],
            "steps": [
                {
                    "op": step["op"],
                    "datagram": list(step["datagram"])
                    if step["op"] == "feed" else [],
                    "rx": step["rx"] if step["op"] == "feed" else -1,
                    "now": step["now"],
                }
                for step in script["steps"]
            ],
        }
        for script in SCRIPTS
    ]
    proc = subprocess.run(
        [node, str(JS_HARNESS), str(APP_JS), "session"],
        input=json.dumps(payload), text=True, capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness JS (session) falhou: {proc.stderr}")
    return json.loads(proc.stdout)


def cxx_traces(workdir: pathlib.Path) -> list[dict] | None:
    exe = compile_cxx_harness(workdir)
    if exe is None:
        return None
    blocks = []
    for script in SCRIPTS:
        blocks.append("@" + script["name"])
        for step in script["steps"]:
            if step["op"] == "feed":
                now = -1 if step["now"] is None else step["now"]
                blocks.append(f"F|{step['datagram'].hex()}|{step['rx']}|{now}")
            else:
                blocks.append(f"P|{step['now']}")
    proc = subprocess.run(
        [str(exe), "session"],
        input="\n".join(blocks) + "\n", text=True, capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness C++ (session) falhou: {proc.stderr}")
    grouped: dict[str, list[dict]] = {s["name"]: [] for s in SCRIPTS}
    for line in proc.stdout.splitlines():
        fields = line.split("|")
        if len(fields) != 11:
            raise RuntimeError(f"linha inesperada do harness C++: {line}")
        grouped[fields[0]].append(
            snapshot(
                fields[5], fields[6], int(fields[7]), int(fields[8]),
                fields[9] == "1", fields[10] == "1", fields[4],
                fields[3] == "1",
            )
        )
    return [{"name": s["name"], "trace": grouped[s["name"]]}
            for s in SCRIPTS]


class SafeState(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.expected = python_traces()
        cls.javascript = js_traces()
        cls.temp = tempfile.TemporaryDirectory(prefix="ventiseg-safe-")
        try:
            cls.cpp = cxx_traces(pathlib.Path(cls.temp.name))
        except Exception:
            cls.temp.cleanup()
            raise

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temp.cleanup()

    def assert_same(self, actual: list[dict], label: str) -> None:
        self.assertEqual(len(actual), len(self.expected))
        for want, got in zip(self.expected, actual):
            self.assertEqual(got["name"], want["name"])
            self.assertEqual(len(got["trace"]), len(want["trace"]))
            for index, (exp_step, got_step) in enumerate(
                    zip(want["trace"], got["trace"])):
                with self.subTest(script=want["name"], passo=index,
                                  impl=label):
                    self.assertEqual(got_step, exp_step)

    def test_python_e_javascript_concordam(self) -> None:
        self.assert_same(self.javascript, "JavaScript")

    def test_python_e_cpp_concordam(self) -> None:
        if self.cpp is None:
            self.skipTest(
                "compilador C++ host ausente; defina VENTISEG_CXX. "
                "O autoteste em placa cobre o parser C++."
            )
        self.assert_same(self.cpp, "C++")

    def test_lnk_trava_movimento(self) -> None:
        script = next(s for s in self.expected
                      if s["name"] == "lnk_inibe_track_e_manual")
        after_watchdog = script["trace"][2]
        self.assertEqual(after_watchdog["telemetry"], "ACK,1,SAFE,Lnk")
        for step in script["trace"][3:5]:
            self.assertTrue(step["result"])  # comandos validos...
        for step in script["trace"][3:]:
            self.assertEqual(step["telemetry"], "ACK,1,SAFE,Lnk")
            self.assertFalse(step["moving"])  # ...mas sem mover o eixo

    def test_recuperacao_so_com_t(self) -> None:
        script = next(s for s in self.expected
                      if s["name"] == "recuperacao_exige_novo_comando")
        self.assertEqual(script["trace"][3]["telemetry"],
                         "ACK,2,SAFE,None")
        self.assertEqual(script["trace"][4]["telemetry"],
                         "ACK,2,TRACK,None")
        self.assertTrue(script["trace"][4]["moving"])
        self.assertEqual(script["trace"][6]["telemetry"],
                         "ACK,3,SAFE,Lnk")


if __name__ == "__main__":
    unittest.main()
