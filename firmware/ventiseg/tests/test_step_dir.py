"""Testes diferenciais Python x JavaScript x C++ da saida STEP/DIR.

A camada e logica pura (um pulso por chamada, rampa trapezoidal) e so
escreve em pino quando um gate de compilacao E um gate de runtime sao
liberados. Aqui as tres implementacoes recebem o mesmo roteiro determinista
de ``tools/step_dir_ref.py`` e devem produzir traces identicas.
"""

from __future__ import annotations

import json
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
CONTROLLER = ROOT / "controller"
TESTS = pathlib.Path(__file__).resolve().parent
APP_JS = ROOT / "simulator" / "js" / "app.js"
JS_HARNESS = TESTS / "differential_stepdir_js_harness.js"
CXX_HARNESS = TESTS / "differential_stepdir_cxx_harness.cpp"
DRIVER_CONFIG = CONTROLLER / "driver_config.h"

sys.path.insert(0, str(TOOLS))
import step_dir_ref as reference  # noqa: E402

KEYS = (
    "steps_per_rev", "speed", "accel", "gate", "target", "position",
    "direction", "pulses", "step", "dir", "test", "test_remaining",
)


def python_rows(ops: list[tuple[str, object]]) -> list[dict]:
    return reference.run_scenario(ops)


def js_rows(ops: list[tuple[str, object]]) -> list[dict]:
    node = shutil.which("node")
    if not node:
        raise RuntimeError("node nao encontrado no PATH")
    payload = [[kind, value] for kind, value in ops]
    proc = subprocess.run(
        [node, str(JS_HARNESS), str(APP_JS)],
        input=json.dumps(payload),
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness JS da saida falhou: {proc.stderr}")
    return json.loads(proc.stdout)


def find_cxx() -> list[str] | None:
    configured = os.environ.get("VENTISEG_CXX", "").strip()
    if configured:
        if os.name == "nt":
            configured = configured.replace("\\", "/")
        parts = shlex.split(configured)
        if pathlib.Path(parts[0]).name.lower() == "zig.exe":
            parts.append("c++")
        return parts
    for name in ("g++", "clang++", "c++"):
        found = shutil.which(name)
        if found:
            return [found]
    return None


def cxx_rows(ops: list[tuple[str, object]], workdir: pathlib.Path) -> list[dict] | None:
    compiler = find_cxx()
    if compiler is None:
        return None
    exe = workdir / ("stepdir_diff.exe" if os.name == "nt" else "stepdir_diff")
    command = compiler + [
        "-std=c++17", "-I", str(CONTROLLER), str(CXX_HARNESS), "-o", str(exe)
    ]
    build = subprocess.run(command, text=True, capture_output=True, check=False)
    if build.returncode != 0:
        raise RuntimeError(
            "falha ao compilar harness C++ da saida: "
            + " ".join(command) + "\n" + build.stdout + build.stderr
        )
    lines = [f"{i}|{kind}|{value}" for i, (kind, value) in enumerate(ops)]
    proc = subprocess.run(
        [str(exe)], input="\n".join(lines) + "\n", text=True,
        capture_output=True, check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness C++ da saida falhou: {proc.stderr}")
    rows = []
    for line in proc.stdout.splitlines():
        fields = line.split("|")
        if len(fields) != 15:
            raise RuntimeError(f"linha inesperada do harness C++: {line}")
        rows.append({
            "index": int(fields[0]),
            "op": fields[1],
            "reply": fields[2],
            "steps_per_rev": int(fields[3]),
            "speed": int(fields[4]),
            "accel": int(fields[5]),
            "gate": fields[6] == "1",
            "target": int(fields[7]),
            "position": int(fields[8]),
            "direction": int(fields[9]),
            "pulses": int(fields[10]),
            "step": fields[11] == "1",
            "dir": fields[12] == "1",
            "test": fields[13] == "1",
            "test_remaining": int(fields[14]),
        })
    return rows


class DifferentialStepDir(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.ops = reference.SCENARIO
        cls.expected = python_rows(cls.ops)
        cls.javascript = js_rows(cls.ops)
        cls.temp = tempfile.TemporaryDirectory(prefix="ventiseg-stepdir-")
        try:
            cls.cpp = cxx_rows(cls.ops, pathlib.Path(cls.temp.name))
        except Exception:
            cls.temp.cleanup()
            raise

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temp.cleanup()

    def assert_same(self, actual: list[dict], label: str) -> None:
        self.assertEqual(len(actual), len(self.expected))
        for want, got in zip(self.expected, actual):
            with self.subTest(indice=want["index"], impl=label):
                self.assertEqual(got["index"], want["index"])
                self.assertEqual(got["op"], want["op"])
                self.assertEqual(got["reply"], want["reply"])
                for key in KEYS:
                    self.assertEqual(got[key], want[key], key)

    def test_python_e_javascript_concordam(self) -> None:
        self.assert_same(self.javascript, "JavaScript")

    def test_python_e_cpp_concordam(self) -> None:
        if self.cpp is None:
            self.skipTest(
                "compilador C++ host ausente; defina VENTISEG_CXX. "
                "O autoteste em placa cobre o firmware."
            )
        self.assert_same(self.cpp, "C++")

    def test_gate_desligado_nao_emite_pulso(self) -> None:
        before = self.expected[0]
        after = self.expected[4]  # logo apos CFG,GATE,0
        self.assertFalse(before["gate"])
        self.assertEqual(before["pulses"], 0)
        self.assertEqual(after["pulses"], 0)

    def test_pulso_aparece_so_com_gate_e_alvo(self) -> None:
        self.assertGreater(self.expected[-1]["pulses"], 0)
        self.assertTrue(self.expected[-1]["gate"] is False)

    def test_configuracoes_rejeitadas(self) -> None:
        replies = [row["reply"] for row in self.expected]
        self.assertIn("REASON,busy", replies)
        self.assertIn("REASON,range", replies)
        self.assertIn("REASON,format", replies)

    def cfg_replies(self) -> dict[str, str]:
        return {
            str(self.ops[row["index"]][1]): row["reply"]
            for row in self.expected
            if row["op"] == "cfg"
        }

    def test_ensaio_de_pulsos_e_deterministico(self) -> None:
        started = next(
            row for row in self.expected
            if row["reply"] == "CFG,PULSE,25,-1,OK"
        )
        self.assertTrue(started["test"])
        self.assertEqual(started["test_remaining"], 25)
        self.assertEqual(started["target"], started["position"] - 25)

        # O alvo enviado pela FSM durante o ensaio nao pode substitui-lo.
        after_target = next(
            row for row in self.expected
            if row["index"] > started["index"] and row["op"] == "target"
        )
        self.assertEqual(after_target["target"], started["target"])

        ended = self.expected[-1]
        self.assertFalse(ended["test"])
        self.assertEqual(ended["test_remaining"], 0)
        self.assertEqual(ended["target"], ended["position"])
        self.assertEqual(ended["pulses"] - started["pulses"], 25)
        self.assertEqual(ended["position"] - started["position"], -25)

        # Cada implementacao deve terminar no mesmo trace.
        for actual, label in ((self.javascript, "JavaScript"), (self.cpp, "C++")):
            if actual is None:
                continue
            with self.subTest(impl=label):
                final = actual[-1]
                self.assertEqual(final["pulses"], ended["pulses"])
                self.assertEqual(final["position"], ended["position"])
                self.assertFalse(final["test"])

    def test_cfg_query_informa_ensaio_em_curso(self) -> None:
        started = next(
            row for row in self.expected
            if row["reply"] == "CFG,PULSE,25,-1,OK"
        )
        query = next(
            row for row in self.expected
            if row["index"] > started["index"]
            and row["reply"].startswith("CFG,steps=")
        )
        self.assertTrue(query["reply"].endswith(",test=25"))

    def test_ensaio_exige_gate_e_formato_valido(self) -> None:
        replies = self.cfg_replies()
        self.assertEqual(replies["CFG,PULSE,25,-1"], "CFG,PULSE,25,-1,OK")
        self.assertEqual(replies["CFG,PULSE,10,1"], "REASON,busy")
        self.assertEqual(replies["CFG,PULSE,5,-1"], "REASON,gate")
        for command in (
            "CFG,PULSE,0,1",
            "CFG,PULSE,1001,1",
            "CFG,PULSE,5,2",
            "CFG,PULSE,5",
        ):
            with self.subTest(cmd=command):
                self.assertEqual(replies[command], "REASON,format")


class GateDeCompilacao(unittest.TestCase):
    def test_gates_padrao_sao_desligados(self) -> None:
        text = DRIVER_CONFIG.read_text(encoding="utf-8") if DRIVER_CONFIG.is_file() else ""
        self.assertIn("#define VENTISEG_ENABLE_GPIO 0", text)
        self.assertIn("#define VENTISEG_HW_AUDIT_OK 0", text)

    def test_sem_arduino_na_camada_de_saida(self) -> None:
        source = (CONTROLLER / "step_dir.cpp").read_text(encoding="utf-8")
        header = (CONTROLLER / "step_dir.h").read_text(encoding="utf-8")
        text = source + header
        for token in ("Arduino.h", "pinMode", "digitalWrite"):
            with self.subTest(token=token):
                self.assertNotIn(token, text)
        self.assertIsNone(
            re.search(r"\bdelay\s*\(\s*[^)]+\s*\)", text),
            "delay() bloqueante nao permitido na camada de saida",
        )


if __name__ == "__main__":
    unittest.main()
