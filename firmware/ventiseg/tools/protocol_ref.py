"""Implementacao de referencia do protocolo serial do ventiseg.

Contrato v0.1 executavel: um unico eixo de pan do fluxo de ar, sem light,
tilt, relé, fim de curso nem GPIO. Este modulo e a especificacao usada por
``tests/test_protocol.py`` e por ``tools/gen_autotest_vectors.py``; o firmware
C++ em ``controller/parser.{h,cpp}`` deve reproduzir a mesma tabela.

Nao ha dependencias externas. Inteiros sao limitados a 32 bits para espelhar
``long`` no ESP32-S3 (Arduino-ESP32, Xtensa 32-bit).
"""

from __future__ import annotations

import re
from dataclasses import dataclass

MAX_DATAGRAM_BYTES = 96
LINK_TIMEOUT_MS = 1200
DEAD_BAND = 55
MANUAL_STEP = 80
STEPS_PER_REV = 3200
MAX_TRAVEL = 533
TRACK_GAIN = 0.75
X_LIMIT = 1000
INT32_MAX = 2_147_483_647

STATE_SAFE = "SAFE"
STATE_TRACK = "TRACK"
STATE_MANUAL = "MANUAL"
FAULT_NONE = "None"
FAULT_LNK = "Lnk"
FAULT_RANGE = "Range"

_INTEGER = re.compile(r"^-?[0-9]+$")


@dataclass(frozen=True)
class Result:
    """Decisao do parser.

    ``kind``: ``track``, ``manual``, ``sample`` ou ``rejected``.
    ``reason`` (somente em rejeicoes): ``length``, ``encoding``, ``format``,
    ``range``, ``sequence`` ou ``age``.
    """

    kind: str
    reason: str = ""
    accepted: bool = False
    seq: int | None = None
    x: int | None = None
    valid: bool | None = None
    enabled: bool | None = None
    direction: int | None = None


def _reject(reason: str) -> Result:
    return Result(kind="rejected", reason=reason)


def _integer(field: str) -> int | None:
    """Converte campo decimal estrito, ou retorna None se invalido."""
    if not _INTEGER.fullmatch(field):
        return None
    try:
        return int(field)
    except ValueError:
        return None


def _bit(field: str) -> tuple[int | None, str]:
    """Le campo 0/1. Sintaxe e validada antes da faixa."""
    value = _integer(field)
    if value is None:
        return None, "format"
    if value not in (0, 1):
        return None, "range"
    return value, ""


def _check_age(result: Result, *, rx_ms: int, now_ms: int | None) -> Result:
    """Aplica a janela de tempo a qualquer comando aceito, por ultimo."""
    if not result.accepted:
        return result
    now = rx_ms if now_ms is None else now_ms
    age = now - rx_ms
    if age < 0 or age > LINK_TIMEOUT_MS:
        return _reject("age")
    return result


def parse(
    datagram: bytes,
    *,
    rx_ms: int,
    now_ms: int | None = None,
    last_seq: int | None = None,
) -> Result:
    """Avalia um datagrama completo.

    Ordem das verificacoes: tamanho/codificacao -> formato -> faixa ->
    sequencia (somente em ``T``) -> idade (qualquer comando, por ultimo).
    """
    if not datagram or len(datagram) > MAX_DATAGRAM_BYTES:
        return _reject("length")
    try:
        text = datagram.decode("ascii")
    except UnicodeDecodeError:
        return _reject("encoding")
    if "\x00" in text:
        return _reject("encoding")

    fields = text.split(",")
    command = fields[0]

    if command == "TRACK":
        if len(fields) != 2:
            return _reject("format")
        value, reason = _bit(fields[1])
        if reason:
            return _reject(reason)
        result = Result(
            kind="track", accepted=True, enabled=bool(value)
        )

    elif command == "MANUAL":
        if len(fields) != 2:
            return _reject("format")
        direction = _integer(fields[1])
        if direction is None:
            return _reject("format")
        if direction not in (-1, 0, 1):
            return _reject("range")
        result = Result(
            kind="manual", accepted=True, direction=direction
        )

    elif command == "T":
        if len(fields) != 4:
            return _reject("format")
        seq = _integer(fields[1])
        x = _integer(fields[2])
        valid, reason = _bit(fields[3])
        if seq is None or x is None or reason == "format":
            return _reject("format")
        if (
            seq < 0
            or seq > INT32_MAX
            or not -X_LIMIT <= x <= X_LIMIT
            or reason == "range"
        ):
            return _reject("range")
        if last_seq is not None and seq <= last_seq:
            return _reject("sequence")
        result = Result(
            kind="sample",
            accepted=True,
            seq=seq,
            x=x,
            valid=bool(valid),
        )

    else:
        return _reject("format")

    return _check_age(result, rx_ms=rx_ms, now_ms=now_ms)


def clamp(value: int, limit: int) -> int:
    """Limita ``value`` a [-limit, limit]."""
    return max(-limit, min(limit, value))


def track_steps(
    x: int,
    *,
    max_travel: int = MAX_TRAVEL,
    gain: float = TRACK_GAIN,
    dead_band: int = DEAD_BAND,
) -> int:
    """Converte erro x em passos do eixo unico (zona morta inclusiva)."""
    if abs(x) <= dead_band:
        return 0
    raw = x * max_travel * gain / 1000.0
    return clamp(int(raw), max_travel)


def manual_target(
    current: int,
    direction: int,
    *,
    max_travel: int = MAX_TRAVEL,
    step: int = MANUAL_STEP,
) -> int:
    """Aplica um passo manual com limite por software."""
    if direction not in (-1, 1):
        raise ValueError("direction deve ser -1 ou 1")
    return clamp(current + direction * step, max_travel)


def link_expired(*, last_sample_ms: int | None, now_ms: int) -> bool:
    """Verdadeiro quando nao ha amostra ``T`` aceita dentro da janela."""
    if last_sample_ms is None:
        return True
    return now_ms - last_sample_ms > LINK_TIMEOUT_MS


@dataclass
class Session:
    """Estado da controladora: um eixo, tres modos e tres faults.

    Somente amostras ``T`` aceitas alimentam o watchdog de 1200 ms. Boot
    seguro: eixo parado, ``SAFE``, ``fault=None``, ``seq=-1``. Um comando
    rejeitado nunca altera modo, eixo, sequencia ou relogio do link; apenas
    uma rejeicao por faixa reporta ``fault=Range`` como diagnostico.
    """

    mode: str = STATE_SAFE
    fault: str = FAULT_NONE
    seq: int = -1
    axis: int = 0
    moving: bool = False
    last_sample_ms: int | None = None
    link_lost: bool = False

    def feed(
        self, datagram: bytes, *, rx_ms: int, now_ms: int | None = None
    ) -> Result:
        """Processa uma linha; retorna o resultado da decisao."""
        last_seq = self.seq if self.seq >= 0 else None
        result = parse(
            datagram, rx_ms=rx_ms, now_ms=now_ms, last_seq=last_seq
        )
        if not result.accepted:
            # Lnk e dominante: um Range posterior nao apaga a perda de link.
            if result.reason == "range" and not self.link_lost:
                self.fault = FAULT_RANGE
            return result

        # Seguranca pos-Lnk: TRACK/MANUAL continuam sendo comandos validos,
        # mas sao inibidos. so T aceito restaura o link e o eixo so volta a
        # se mover apos um novo TRACK/MANUAL.
        if self.link_lost and result.kind != "sample":
            return result

        if result.kind == "track":
            self.mode = STATE_TRACK if result.enabled else STATE_SAFE
            self.moving = bool(result.enabled)
            if not self.link_lost:
                self.fault = FAULT_NONE
        elif result.kind == "manual":
            direction = result.direction or 0
            if direction == 0:
                self.mode = STATE_SAFE
                self.moving = False
            else:
                self.mode = STATE_MANUAL
                self.moving = True
                self.axis = manual_target(self.axis, direction)
            if not self.link_lost:
                self.fault = FAULT_NONE
        elif result.kind == "sample":
            # Unica entrada que renova o watchdog e limpa fault=Lnk.
            self.seq = result.seq if result.seq is not None else -1
            self.last_sample_ms = rx_ms
            self.link_lost = False
            self.fault = FAULT_NONE
            if result.valid:
                self.axis = track_steps(result.x or 0)
                self.moving = self.mode == STATE_TRACK
            else:
                self.axis = 0
                self.moving = False
        return result

    def poll(self, *, now_ms: int) -> bool:
        """Aplica o watchdog; True indica transicao para SAFE."""
        if self.link_lost or self.last_sample_ms is None:
            return False
        if not link_expired(last_sample_ms=self.last_sample_ms, now_ms=now_ms):
            return False
        self.mode = STATE_SAFE
        self.moving = False
        self.fault = FAULT_LNK
        self.link_lost = True
        return True


def format_telemetry(session: Session) -> str:
    """Formato exato: ``ACK,<seq>,<state>,<fault>``."""
    return f"ACK,{session.seq},{session.mode},{session.fault}"
