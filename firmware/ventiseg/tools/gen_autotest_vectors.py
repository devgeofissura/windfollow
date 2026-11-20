"""Gera os vetores do autoteste embarcado a partir da referencia Python.

O firmware nao tem uma copia independente da tabela de decisao: ele recebe
os casos e os resultados esperados calculados por ``protocol_ref.py``.
Assim uma divergencia entre host e placa aparece como falha no autoteste.

Uso (Python LTS absoluto):
    python tools/gen_autotest_vectors.py controller/autotest_vectors.h
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import protocol_ref as p  # noqa: E402

# (nome, datagrama, rx_ms, now_ms ou None, last_seq ou None)
CASES: list[tuple[str, bytes, int, int | None, int | None]] = [
    ("track_on", b"TRACK,1", 0, None, None),
    ("track_off", b"TRACK,0", 0, None, None),
    ("track_two", b"TRACK,2", 0, None, None),
    ("track_negative", b"TRACK,-1", 0, None, None),
    ("track_missing", b"TRACK", 0, None, None),
    ("track_empty", b"TRACK,", 0, None, None),
    ("track_extra", b"TRACK,1,0", 0, None, None),
    ("track_leading_space", b"TRACK, 1", 0, None, None),
    ("track_trailing_space", b"TRACK,1 ", 0, None, None),
    ("track_garbage", b"TRACK,on", 0, None, None),
    ("track_float", b"TRACK,1.0", 0, None, None),
    ("manual_forward", b"MANUAL,1", 0, None, None),
    ("manual_backward", b"MANUAL,-1", 0, None, None),
    ("manual_stop", b"MANUAL,0", 0, None, None),
    ("manual_two", b"MANUAL,2", 0, None, None),
    ("manual_minus_two", b"MANUAL,-2", 0, None, None),
    ("manual_missing", b"MANUAL", 0, None, None),
    ("manual_extra", b"MANUAL,1,0", 0, None, None),
    ("manual_garbage", b"MANUAL,front", 0, None, None),
    ("manual_float", b"MANUAL,1.0", 0, None, None),
    ("manual_axis_field", b"MANUAL,pan,1", 0, None, None),
    ("sample_center", b"T,1,0,1", 0, None, None),
    ("sample_extreme", b"T,2,1000,1", 0, None, None),
    ("sample_invalid_flag", b"T,3,1000,0", 0, None, None),
    ("sample_leading_zeros", b"T,007,-01,01", 0, None, None),
    ("sample_bad_seq", b"T,abc,0,1", 0, None, None),
    ("sample_bad_x", b"T,1,abc,1", 0, None, None),
    ("sample_x_above", b"T,3,1001,1", 0, None, None),
    ("sample_x_below", b"T,3,-1001,1", 0, None, None),
    ("sample_valid_two", b"T,3,0,2", 0, None, None),
    ("sample_missing_field", b"T,1,0", 0, None, None),
    ("sample_extra_field", b"T,1,0,1,9", 0, None, None),
    ("sample_negative_seq", b"T,-1,0,1", 0, None, None),
    ("sample_float", b"T,1,0.5,1", 0, None, None),
    ("sequence_repeated", b"T,5,0,1", 0, None, 5),
    ("sequence_regressing", b"T,4,0,1", 0, None, 5),
    ("sequence_advancing", b"T,6,0,1", 0, None, 5),
    ("age_exact_limit", b"T,1,0,1", 1000, 2200, None),
    ("age_past_limit", b"T,1,0,1", 1000, 2201, None),
    ("age_track", b"TRACK,1", 1000, 2201, None),
    ("age_manual", b"MANUAL,1", 1000, 2201, None),
    ("age_negative", b"T,1,0,1", 1000, 999, None),
    ("missing_now_large_rx", b"T,1,0,1", 5000000, None, None),
    ("expired_large_rx", b"T,1,0,1", 5000000, 5001201, None),
    ("empty", b"", 0, None, None),
    ("oversized", b"T," + b"1" * 200, 0, None, None),
    ("nul_byte", b"TRACK,1\x00blocked", 0, None, None),
    ("high_byte", b"TRACK,\xff", 0, None, None),
    ("newline", b"TRACK,1\n", 0, None, None),
    ("unknown_command", b"BOGUS,1", 0, None, None),
    ("lowercase_command", b"track,1", 0, None, None),
    ("prefix_without_comma", b"TRACKX,1", 0, None, None),
]

KIND = {
    "rejected": "ventiseg::Kind::kRejected",
    "track": "ventiseg::Kind::kTrack",
    "manual": "ventiseg::Kind::kManual",
    "sample": "ventiseg::Kind::kSample",
}
REASON = {
    "": "ventiseg::Reason::kNone",
    "length": "ventiseg::Reason::kLength",
    "encoding": "ventiseg::Reason::kEncoding",
    "format": "ventiseg::Reason::kFormat",
    "range": "ventiseg::Reason::kRange",
    "sequence": "ventiseg::Reason::kSequence",
    "age": "ventiseg::Reason::kAge",
}


def c_string(data: bytes) -> tuple[str, int]:
    """Literal C seguro, com octal de 3 digitos para bytes nao imprimiveis."""
    parts: list[str] = []
    for byte in data:
        if byte == 0x22:
            parts.append('\\"')
        elif byte == 0x5C:
            parts.append("\\\\")
        elif 0x20 <= byte <= 0x7E:
            parts.append(chr(byte))
        else:
            parts.append("\\%03o" % byte)
    return "".join(parts), len(data)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("uso: gen_autotest_vectors.py <arquivo.h>")
    out = Path(sys.argv[1])
    lines = [
        "// GERADO POR tools/gen_autotest_vectors.py - NAO EDITAR A MAO.",
        "// Fonte: tools/protocol_ref.py (contrato v0.1).",
        "#ifndef VENTISEG_AUTOTEST_VECTORS_H",
        "#define VENTISEG_AUTOTEST_VECTORS_H",
        "",
        '#include "parser.h"',
        "",
        "struct TestVector {",
        "  const char* name;",
        "  const char* datagram;",
        "  size_t len;",
        "  int64_t rx_ms;",
        "  int64_t now_ms;",
        "  bool has_last_seq;",
        "  int32_t last_seq;",
        "  ventiseg::Kind kind;",
        "  ventiseg::Reason reason;",
        "  bool accepted;",
        "  int32_t seq;",
        "  int32_t x;",
        "  bool valid;",
        "  bool enabled;",
        "  int8_t direction;",
        "};",
        "",
        "const TestVector kTestVectors[] = {",
    ]
    for name, data, rx_ms, now_ms, last_seq in CASES:
        result = p.parse(
            data, rx_ms=rx_ms, now_ms=now_ms, last_seq=last_seq
        )
        literal, length = c_string(data)
        lines.append(
            "  {{\"{name}\", \"{literal}\", {len}, {rx}, {now}, "
            "{has_seq}, {last}, {kind}, {reason}, {accepted}, {seq}, "
            "{x}, {valid}, {enabled}, {dir}}},".format(
                name=name,
                literal=literal,
                len=length,
                rx=rx_ms,
                now=-1 if now_ms is None else now_ms,
                has_seq="true" if last_seq is not None else "false",
                last=0 if last_seq is None else last_seq,
                kind=KIND[result.kind],
                reason=REASON[result.reason],
                accepted="true" if result.accepted else "false",
                seq=result.seq if result.seq is not None else 0,
                x=result.x if result.x is not None else 0,
                valid="true" if result.valid else "false",
                enabled="true" if result.enabled else "false",
                dir=result.direction if result.direction is not None else 0,
            )
        )
    lines += [
        "};",
        "",
        "constexpr size_t kTestVectorCount =",
        "    sizeof(kTestVectors) / sizeof(kTestVectors[0]);",
        "",
        "#endif  // VENTISEG_AUTOTEST_VECTORS_H",
        "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"{out}: {len(CASES)} vetores")


if __name__ == "__main__":
    main()
