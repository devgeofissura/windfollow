"""Implementacao de referencia do parser UDP do laserseg.

Este modulo e a especificacao executavel do contrato descrito em
``docs/PROTOCOLO_UDP.md``. Os testes em ``tests/test_protocol.py`` rodam no
host; o firmware C++ de producao deve reproduzir a mesma tabela de decisao.

Nao ha dependencias externas. Valores inteiros sao limitados a 32 bits para
espelhar ``long`` no ESP32-S3 (Arduino-ESP32, Xtensa 32-bit).
"""

from __future__ import annotations

import re
from dataclasses import dataclass

MAX_DATAGRAM_BYTES = 96
LINK_TIMEOUT_MS = 1200
DEAD_BAND = 55
MANUAL_STEP = 80
INT32_MAX = 2_147_483_647

_INTEGER = re.compile(r"^-?[0-9]+$")


@dataclass(frozen=True)
class Result:
    """Resultado de uma decisao do parser.

    ``kind`` e um de: ``track``, ``light``, ``manual``, ``sample``,
    ``rejected``. Em caso de rejeicao, ``reason`` explica o motivo:
    ``length``, ``encoding``, ``format``, ``range``, ``sequence`` ou ``age``.
    """

    kind: str
    reason: str = ""
    accepted: bool = False
    seq: int | None = None
    x: int | None = None
    y: int | None = None
    valid: bool | None = None
    enabled: bool | None = None
    axis: str | None = None
    direction: int | None = None


def _reject(reason: str) -> Result:
    return Result(kind="rejected", reason=reason)


def _integer(field: str) -> int | None:
    """Converte um campo estritamente decimal, ou retorna None se invalido."""
    if not _INTEGER.fullmatch(field):
        return None
    try:
        return int(field)
    except ValueError:
        return None


def _bit(field: str) -> tuple[int | None, str]:
    """Le um campo 0/1. Retorna (valor, motivo_de_rejeicao).

    A sintaxe e validada antes da faixa: ``-abc`` e erro de formato,
    enquanto ``-1`` e erro de faixa.
    """
    value = _integer(field)
    if value is None:
        return None, "format"
    if value not in (0, 1):
        return None, "range"
    return value, ""


def _check_age(result: Result, *, rx_ms: int, now_ms: int | None) -> Result:
    """Aplica a janela de tempo a QUALQUER comando aceito, por ultimo."""
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

    Ordem das verificacoes, igual a do firmware:

    1. tamanho e codificacao;
    2. formato (nome do comando, numero de campos, sintaxe decimal);
    3. faixa numerica;
    4. sequencia (somente em amostras ``T``);
    5. idade do datagrama (qualquer comando, aplicada por ultimo).
    """
    if not datagram:
        return _reject("length")
    if len(datagram) > MAX_DATAGRAM_BYTES:
        return _reject("length")

    try:
        text = datagram.decode("ascii")
    except UnicodeDecodeError:
        return _reject("encoding")
    if "\x00" in text:
        return _reject("encoding")

    fields = text.split(",")
    command = fields[0]

    if command in ("TRACK", "LIGHT"):
        if len(fields) != 2:
            return _reject("format")
        value, reason = _bit(fields[1])
        if reason:
            return _reject(reason)
        kind = "track" if command == "TRACK" else "light"
        result = Result(kind=kind, accepted=True, enabled=bool(value))

    elif command == "MANUAL":
        if len(fields) != 3:
            return _reject("format")
        axis = fields[1]
        if axis not in ("pan", "tilt"):
            return _reject("range")
        direction = _integer(fields[2])
        if direction is None:
            return _reject("format")
        if direction not in (-1, 1):
            return _reject("range")
        result = Result(
            kind="manual", accepted=True, axis=axis, direction=direction
        )

    elif command == "T":
        if len(fields) != 5:
            return _reject("format")

        seq = _integer(fields[1])
        x = _integer(fields[2])
        y = _integer(fields[3])
        valid, reason = _bit(fields[4])
        if seq is None or x is None or y is None or reason == "format":
            return _reject("format")
        if (
            seq < 0
            or seq > INT32_MAX
            or not -1000 <= x <= 1000
            or not -1000 <= y <= 1000
        ):
            return _reject("range")
        if reason:
            return _reject(reason)

        if last_seq is not None and seq <= last_seq:
            return _reject("sequence")

        result = Result(
            kind="sample",
            accepted=True,
            seq=seq,
            x=x,
            y=y,
            valid=bool(valid),
        )

    else:
        return _reject("format")

    return _check_age(result, rx_ms=rx_ms, now_ms=now_ms)


def clamp(value: int, limit: int) -> int:
    """Limita ``value`` a [-limit, limit]."""
    return max(-limit, min(limit, value))


def target_from_error(
    error: int, *, max_travel: int, gain: float, dead_band: int = DEAD_BAND
) -> int:
    """Converte erro normalizado em alvo de passos.

    Zona morta inclusive; truncamento para zero, igual ao ``(long)`` do C++.
    """
    if abs(error) <= dead_band:
        return 0
    raw = error * max_travel * gain / 1000.0
    return clamp(int(raw), max_travel)


def manual_target(
    current: int, direction: int, *, max_travel: int, step: int = MANUAL_STEP
) -> int:
    """Aplica um passo manual de ``step`` no sentido dado, com limite local."""
    if direction not in (-1, 1):
        raise ValueError("direction deve ser -1 ou 1")
    return clamp(current + direction * step, max_travel)


def link_expired(*, last_packet_ms: int | None, now_ms: int) -> bool:
    """Verdadeiro quando nao ha pacote aceito dentro da janela de seguranca."""
    if last_packet_ms is None:
        return True
    return now_ms - last_packet_ms > LINK_TIMEOUT_MS


@dataclass
class Session:
    """Estado da controladora. Espelha as variaveis globais do firmware.

    Somente datagramas aceitos atualizam ``last_packet_ms``. ``TRACK,1``
    reinicia a janela de sequencia, permitindo que a camera reinicie seu
    contador sem exigir um reboot da controladora.
    """

    # Estado seguro no boot: eixos parados, luz apagada, sem rastreamento.
    last_packet_ms: int | None = None
    last_seq: int | None = None
    tracking: bool = False
    light: bool = False
    axes_stopped: bool = True

    def feed(
        self, datagram: bytes, *, rx_ms: int, now_ms: int | None = None
    ) -> Result:
        result = parse(
            datagram, rx_ms=rx_ms, now_ms=now_ms, last_seq=self.last_seq
        )
        if not result.accepted:
            return result

        self.last_packet_ms = rx_ms
        if result.kind == "track":
            self.tracking = bool(result.enabled)
            if result.enabled:
                self.last_seq = None
            else:
                self.axes_stopped = True
        elif result.kind == "light":
            self.light = bool(result.enabled)
        elif result.kind == "manual":
            self.axes_stopped = False
        elif result.kind == "sample":
            self.last_seq = result.seq
            # So uma amostra valida com rastreamento ativo libera os eixos.
            self.axes_stopped = not (self.tracking and result.valid)
        return result

    def poll(self, *, now_ms: int) -> bool:
        """Aplica o watchdog. True indica que a controladora deve ir a seguro.

        A acao de seguranca e explicita: parar AMBOS os eixos na posicao
        atual e desligar a luz. O firmware deve executar as tres acoes antes
        de qualquer chamada a ``run()`` dos motores no mesmo ciclo.
        """
        if not link_expired(
            last_packet_ms=self.last_packet_ms, now_ms=now_ms
        ):
            return False
        self.tracking = False
        self.light = False
        self.axes_stopped = True
        return True
