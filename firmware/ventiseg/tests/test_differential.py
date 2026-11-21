"""Testes diferenciais Python x JavaScript x C++ do protocolo ventiseg.

Um unico conjunto de datagramas alimenta tres implementacoes independentes:

* ``tools/protocol_ref.py`` (oraculo em Python);
* ``simulator/js/app.js`` (nucleo do simulador, executado em Node);
* ``controller/parser.cpp`` (firmware, compilado e executado no host).

O compilador C++ e descoberto em ``VENTISEG_CXX`` (caminho ou comando entre
aspas), depois em ``g++/clang++/c++`` no PATH. Quando nao ha compilador host,
o teste C++ e pulado com motivo explicito; o teste embarcado em placa real
continua sendo feito pelo autoteste (``controller/autotest.h``).

Execute com:

    python -m unittest discover -s firmware/ventiseg/tests -p 'test_*.py' -v
"""

from __future__ import annotations

import json
import os
import pathlib
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
JS_HARNESS = TESTS / "differential_js_harness.js"
CXX_HARNESS = TESTS / "differential_cxx_harness.cpp"

sys.path.insert(0, str(TOOLS))
import gen_autotest_vectors as generator  # noqa: E402
import protocol_ref as reference  # noqa: E402

# Casos extras que cobrem as divergencias historicamente encontradas entre
# as tres implementacoes. Nao entram no cabecalho gerado (52 vetores).
EXTRA_CASES = [
    ("diff_seq_ceiling", b"T,2147483648,0,1", 0, None, None),
    ("diff_seq_max", b"T,2147483647,0,1", 0, None, None),
    ("diff_track_leading_zero", b"TRACK,01", 0, None, None),
    ("diff_manual_leading_zero", b"MANUAL,01", 0, None, None),
    ("diff_track_two", b"TRACK,2", 0, None, None),
    ("diff_age_negative_track", b"TRACK,1", 1000, 999, None),
    ("diff_age_exact_track", b"TRACK,1", 1000, 2200, None),
    ("diff_age_negative_manual", b"MANUAL,-1", 1000, 999, None),
]

Case = tuple[str, bytes, int, int | None, int | None]


def all_cases() -> list[Case]:
    return list(generator.CASES) + EXTRA_CASES


def normalize(name: str, result: object) -> dict:
    """Uma unica forma de comparar as tres implementacoes."""
    reason = (result.reason or "none")
    return {
        "name": name,
        "kind": result.kind,
        "reason": reason,
        "accepted": bool(result.accepted),
        "seq": result.seq if result.seq is not None else 0,
        "x": result.x if result.x is not None else 0,
        "valid": bool(result.valid),
        "enabled": bool(result.enabled),
        "direction": result.direction if result.direction is not None else 0,
    }


def python_results(cases: list[Case]) -> list[dict]:
    out = []
    for name, data, rx_ms, now_ms, last_seq in cases:
        result = reference.parse(
            data,
            rx_ms=rx_ms,
            now_ms=now_ms,
            last_seq=last_seq,
        )
        out.append(normalize(name, result))
    return out


def js_results(cases: list[Case]) -> list[dict]:
    node = shutil.which("node")
    if not node:
        raise RuntimeError("node nao encontrado no PATH")
    payload = [
        {
            "name": name,
            "datagram": list(data),
            "rx_ms": rx_ms,
            "now_ms": now_ms,
            "last_seq": last_seq,
        }
        for name, data, rx_ms, now_ms, last_seq in cases
    ]
    proc = subprocess.run(
        [node, str(JS_HARNESS), str(APP_JS)],
        input=json.dumps(payload),
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness JS falhou: {proc.stderr}")
    out = []
    for item in json.loads(proc.stdout):
        decision = item["decision"]
        out.append(
            {
                "name": item["name"],
                "kind": decision["kind"],
                "reason": decision["reason"] or "none",
                "accepted": bool(decision["accepted"]),
                # createState() usa -1 como "sem amostra" e guarda last_seq
                # no estado; o Result de Python/C++ e sempre 0 em rejeicoes.
                "seq": max(decision["seq"], 0)
                if decision["accepted"] else 0,
                "x": decision["x"],
                "valid": bool(decision["valid"]),
                "enabled": bool(decision["enabled"]),
                "direction": decision["direction"],
            }
        )
    return out


def find_cxx() -> list[str] | None:
    configured = os.environ.get("VENTISEG_CXX", "").strip()
    if configured:
        if os.name == "nt":
            # shlex em modo POSIX come barras de caminho do Windows.
            configured = configured.replace("\\", "/")
        parts = shlex.split(configured)
        first = pathlib.Path(parts[0])
        if first.name.lower() == "zig.exe":
            parts.append("c++")  # zig c++ compila C++ como o host
        return parts
    for name in ("g++", "clang++", "c++"):
        found = shutil.which(name)
        if found:
            return [found]
    return None


def compile_cxx_harness(workdir: pathlib.Path) -> pathlib.Path | None:
    """Compila o harness C++ uma vez; None se nao ha compilador host."""
    compiler = find_cxx()
    if compiler is None:
        return None
    exe = workdir / ("differential_cxx.exe" if os.name == "nt"
                     else "differential_cxx")
    command = compiler + [
        "-std=c++17",
        "-I", str(CONTROLLER),
        str(CXX_HARNESS),
        "-o", str(exe),
    ]
    build = subprocess.run(
        command, text=True, capture_output=True, check=False
    )
    if build.returncode != 0:
        raise RuntimeError(
            "falha ao compilar harness C++: "
            + " ".join(command)
            + "\n"
            + build.stdout
            + build.stderr
        )
    return exe


def cxx_results(cases: list[Case], workdir: pathlib.Path) -> list[dict] | None:
    exe = compile_cxx_harness(workdir)
    if exe is None:
        return None
    lines = []
    for name, data, rx_ms, now_ms, last_seq in cases:
        now = -1 if now_ms is None else now_ms
        has_seq = 1 if last_seq is not None else 0
        last = 0 if last_seq is None else last_seq
        lines.append(
            f"{name}|{data.hex()}|{rx_ms}|{now}|{has_seq}|{last}"
        )
    proc = subprocess.run(
        [str(exe)],
        input="\n".join(lines) + "\n",
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"harness C++ falhou: {proc.stderr}")
    out = []
    for line in proc.stdout.splitlines():
        fields = line.split("|")
        if len(fields) != 9:
            raise RuntimeError(f"linha inesperada do harness C++: {line}")
        out.append(
            {
                "name": fields[0],
                "kind": fields[1],
                "reason": fields[2],
                "accepted": fields[3] == "1",
                "seq": int(fields[4]),
                "x": int(fields[5]),
                "valid": fields[6] == "1",
                "enabled": fields[7] == "1",
                "direction": int(fields[8]),
            }
        )
    return out


class Differential(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.cases = all_cases()
        cls.expected = python_results(cls.cases)
        cls.javascript = js_results(cls.cases)
        cls.temp = tempfile.TemporaryDirectory(prefix="ventiseg-diff-")
        try:
            cls.cpp = cxx_results(cls.cases, pathlib.Path(cls.temp.name))
        except Exception:
            cls.temp.cleanup()
            raise

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temp.cleanup()

    def assert_same(self, actual: list[dict], label: str) -> None:
        self.assertEqual(len(actual), len(self.expected))
        for want, got in zip(self.expected, actual):
            with self.subTest(caso=want["name"], impl=label):
                self.assertEqual(got, want)

    def test_python_e_javascript_concordam(self) -> None:
        self.assert_same(self.javascript, "JavaScript")

    def test_python_e_cpp_concordam(self) -> None:
        if self.cpp is None:
            self.skipTest(
                "compilador C++ host ausente; defina VENTISEG_CXX ou use "
                "g++/clang++/c++ no PATH. O autoteste em placa cobre o C++."
            )
        self.assert_same(self.cpp, "C++")

    def test_casos_cobrem_divergencias_conhecidas(self) -> None:
        por_nome = {item["name"]: item for item in self.expected}
        checagens = {
            "diff_seq_ceiling": ("rejected", "range"),
            "diff_seq_max": ("sample", "none"),
            "diff_track_leading_zero": ("track", "none"),
            "diff_manual_leading_zero": ("manual", "none"),
            "diff_track_two": ("rejected", "range"),
            "diff_age_negative_track": ("rejected", "age"),
            "diff_age_exact_track": ("track", "none"),
            "diff_age_negative_manual": ("rejected", "age"),
            "age_past_limit": ("rejected", "age"),
            "age_negative": ("rejected", "age"),
        }
        for name, (kind, reason) in checagens.items():
            with self.subTest(caso=name):
                item = por_nome[name]
                self.assertEqual(item["kind"], kind)
                self.assertEqual(item["reason"], reason)

    def test_vetores_gerados_estao_em_sincronia(self) -> None:
        destino = pathlib.Path(self.temp.name) / "autotest_vectors.h"
        proc = subprocess.run(
            [sys.executable, str(TOOLS / "gen_autotest_vectors.py"),
             str(destino)],
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(
            destino.read_bytes(),
            (CONTROLLER / "autotest_vectors.h").read_bytes(),
            "controller/autotest_vectors.h esta desatualizado",
        )


if __name__ == "__main__":
    unittest.main()
