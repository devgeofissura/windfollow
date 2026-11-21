"""Oraculo Python da camada de saida STEP/DIR do ventiseg.

Espelha ``controller/step_dir.{h,cpp}`` e o nucleo ``StepDir`` do simulador.
Usado por ``tests/test_step_dir.py`` para comparar Python x JavaScript x C++.

Algoritmo propositalmente trivial (mesma ideia do AccelStepper, sem copiar
codigo): a cada chamada, no maximo um pulso; intervalo recalculado pela
velocidade atual; rampa limitada por aceleracao e pela distancia de frenagem.
Nenhum pino e tocado ate ``gate=True``. Mesmo assim, este modulo e logica pura.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

FULL_STEPS_PER_REV = 200
STEP_PULSE_US = 10
DIR_SETUP_US = 5
DEFAULT_SPEED = 800
DEFAULT_ACCEL = 4000
SPEED_MAX = 10000
ACCEL_MAX = 100000
PULSE_TEST_MAX = 1000


@dataclass
class StepDir:
    """Estado logico de um eixo STEP/DIR; sem hardware."""

    steps_per_rev: int = FULL_STEPS_PER_REV * 16
    speed: int = DEFAULT_SPEED
    accel: int = DEFAULT_ACCEL
    gate: bool = False
    target: int = 0
    position: int = 0
    direction: int = 0
    pulses: int = 0
    velocity: float = 0.0
    started: bool = False
    last_step_us: int = 0
    last_update_us: int = 0
    dir_ready_us: int = 0
    pulse_until_us: int = 0
    step_level: bool = False
    dir_level: bool = False
    test_mode: bool = False
    step_writes: int = 0
    dir_writes: int = 0
    step_high_writes: int = 0

    def write_step(self, level: bool) -> None:
        self.step_writes += 1
        if level:
            self.step_high_writes += 1

    def write_dir(self, level: bool) -> None:
        self.dir_writes += 1

    @property
    def test_remaining(self) -> int:
        if not self.test_mode:
            return 0
        return abs(self.target - self.position)

    def set_microstep(self, value: int) -> bool:
        if value not in (8, 16):
            return False
        if self.velocity != 0.0 or self.step_level or self.test_mode:
            return False
        self.steps_per_rev = FULL_STEPS_PER_REV * value
        return True

    def set_speed(self, value: int) -> bool:
        if value < 1 or value > SPEED_MAX:
            return False
        self.speed = value
        return True

    def set_accel(self, value: int) -> bool:
        if value < 1 or value > ACCEL_MAX:
            return False
        self.accel = value
        return True

    def set_gate(self, value: bool) -> bool:
        if self.gate == value:
            return True
        self.gate = value
        if not value:
            self.velocity = 0.0
            self.test_mode = False
            self.target = self.position
            if self.step_level:
                self.step_level = False
                self.write_step(False)
        return True

    def set_target(self, target: int) -> None:
        if self.test_mode:
            return
        self.target = target

    def test_pulse(self, steps: int, direction: int) -> bool:
        if steps < 1 or steps > PULSE_TEST_MAX:
            return False
        if direction not in (1, -1):
            return False
        if not self.gate or self.test_mode or self.velocity != 0.0:
            return False
        if self.step_level:
            return False
        destination = self.position + steps * direction
        if destination < -(2**31) or destination > 2**31 - 1:
            return False
        self.target = destination
        self.test_mode = True
        self.velocity = 0.0
        return True

    def update(self, now_us: int) -> int:
        if not self.started:
            self.started = True
            self.last_update_us = now_us
            self.last_step_us = now_us

        if self.step_level and now_us >= self.pulse_until_us:
            self.step_level = False
            self.write_step(False)

        if not self.gate:
            self.velocity = 0.0
            self.last_update_us = now_us
            return 0

        elapsed = now_us - self.last_update_us
        dt = elapsed / 1e6 if elapsed > 0 else 0.0
        self.last_update_us = now_us

        remaining = self.target - self.position
        if remaining == 0:
            self.velocity = 0.0
            if self.test_mode:
                self.test_mode = False
                self.target = self.position
            return 0

        sign = 1 if remaining > 0 else -1
        if sign != self.direction:
            self.direction = sign
            self.dir_level = sign > 0
            self.dir_ready_us = now_us + DIR_SETUP_US
            self.write_dir(self.dir_level)

        if now_us < self.dir_ready_us:
            return 0

        dist = abs(remaining)
        v_limit = math.sqrt(2.0 * self.accel * dist)
        wanted = sign * min(float(self.speed), v_limit)
        delta = self.accel * dt
        if wanted > self.velocity:
            self.velocity = min(wanted, self.velocity + delta)
        elif wanted < self.velocity:
            self.velocity = max(wanted, self.velocity - delta)

        speed_now = abs(self.velocity)
        if speed_now < 1e-9:
            return 0
        interval = 1e6 / speed_now
        if now_us - self.last_step_us < interval:
            return 0

        self.last_step_us = now_us
        self.position += sign
        self.pulses += 1
        self.step_level = True
        self.pulse_until_us = now_us + STEP_PULSE_US
        self.write_step(True)
        return 1

    def config(self, line: str) -> str:
        if line == "CFG,?":
            return (
                f"CFG,steps={self.steps_per_rev},speed={self.speed},"
                f"accel={self.accel},gate={int(self.gate)},"
                f"test={self.test_remaining}"
            )
        if line == "CFG,GATE,0":
            self.set_gate(False)
            return "CFG,GATE,0,OK"
        if line == "CFG,GATE,1,CONFIRM":
            self.set_gate(True)
            return "CFG,GATE,1,OK"
        if line.startswith("CFG,GATE,"):
            return "REASON,format"

        if line.startswith("CFG,PULSE,"):
            parts = line.split(",")
            if len(parts) != 4 or not parts[2].isdigit():
                return "REASON,format"
            direction_text = parts[3]
            if direction_text.startswith("-"):
                direction = -1
                digits = direction_text[1:]
            elif direction_text.startswith("+"):
                direction = 1
                digits = direction_text[1:]
            else:
                direction = 1
                digits = direction_text
            if digits != "1":
                return "REASON,format"
            steps = int(parts[2])
            if steps < 1 or steps > PULSE_TEST_MAX:
                return "REASON,format"
            if not self.gate:
                return "REASON,gate"
            if not self.test_pulse(steps, direction):
                return "REASON,busy"
            return f"CFG,PULSE,{steps},{direction},OK"

        for key, prefix, setter in (
            ("MICRO", "CFG,MICRO,", self.set_microstep),
            ("SPEED", "CFG,SPEED,", self.set_speed),
            ("ACCEL", "CFG,ACCEL,", self.set_accel),
        ):
            if line.startswith(prefix):
                tail = line[len(prefix):]
                if not tail.isdigit():
                    return "REASON,format"
                value = int(tail)
                if key == "MICRO" and value not in (8, 16):
                    return "REASON,range"
                if not setter(value):
                    return "REASON,busy" if key == "MICRO" else "REASON,range"
                return f"CFG,{key},{value},OK"
        return "REASON,format"

    def snapshot(self) -> dict[str, object]:
        return {
            "steps_per_rev": self.steps_per_rev,
            "speed": self.speed,
            "accel": self.accel,
            "gate": self.gate,
            "target": self.target,
            "position": self.position,
            "direction": self.direction,
            "pulses": self.pulses,
            "step": self.step_level,
            "dir": self.dir_level,
            "test": self.test_mode,
            "test_remaining": self.test_remaining,
            "step_writes": self.step_writes,
            "dir_writes": self.dir_writes,
            "step_high_writes": self.step_high_writes,
        }


# Roteiro determinista compartilhado pelas tres implementacoes.
SCENARIO: list[tuple[str, object]] = [
    ("update", 0),
    ("cfg", "CFG,?"),
    ("cfg", "CFG,GATE,1,CONFIRM"),
    ("update", 1000),
    ("cfg", "CFG,GATE,0"),
    ("update", 2000),
    ("cfg", "CFG,GATE,1,CONFIRM"),
    ("target", 300),
]

SCENARIO += [("update", 3000 + i * 1000) for i in range(250)]
SCENARIO += [
    ("cfg", "CFG,MICRO,8"),       # ocupado: ainda em movimento
    ("cfg", "CFG,SPEED,1500"),
    ("cfg", "CFG,ACCEL,8000"),
    ("target", -250),
]
SCENARIO += [("update", 253000 + i * 1000) for i in range(250)]
SCENARIO += [
    ("cfg", "CFG,GATE,0"),
    ("update", 503000),
    ("cfg", "CFG,MICRO,8"),       # agora em repouso
    ("cfg", "CFG,SPEED,0"),       # fora de faixa
    ("cfg", "CFG,ACCEL,200000"),  # fora de faixa
    ("cfg", "CFG,MICRO,32"),      # fora de faixa
    ("cfg", "CFG,MQC,1"),         # formato invalido
    ("target", 120),
    ("update", 504000),
    # Ensaio determinista: o alvo da FSM eh ignorado enquanto ele dura.
    ("target", 0),                # estaciona com gate fechado
    ("cfg", "CFG,GATE,1,CONFIRM"),
    ("update", 505000),
    ("cfg", "CFG,PULSE,25,-1"),
    ("cfg", "CFG,PULSE,10,1"),    # ocupado: ja ha um ensaio em curso
    ("target", 999),              # ignorado pelo ensaio
    ("cfg", "CFG,?"),             # test=25
]
SCENARIO += [("update", 506000 + i * 1000) for i in range(120)]
SCENARIO += [
    ("cfg", "CFG,?"),             # ensaio concluido, alvo devolvido
    ("cfg", "CFG,PULSE,0,1"),     # formato: zero pulsos
    ("cfg", "CFG,PULSE,1001,1"),  # formato: acima do teto
    ("cfg", "CFG,PULSE,5,2"),     # formato: direcao invalida
    ("cfg", "CFG,PULSE,5"),       # formato: sem direcao
    ("cfg", "CFG,GATE,0"),
    ("cfg", "CFG,PULSE,5,-1"),    # gate fechado
    ("update", 700000),
]

# Casos-limite de contrato e de rastreio de escrita de pino.
EDGE_SCENARIO: list[tuple[str, object]] = [
    ("cfg", "CFG,?"),
    ("update", 0),
    ("update", 1000),
    ("update", 2000),
    # Decimal decodificado fora da faixa deve ser `range`, nunca `format`.
    ("cfg", "CFG,SPEED,1000001"),
    ("cfg", "CFG,ACCEL,1000001"),
    ("cfg", "CFG,MICRO,1000001"),
    ("cfg", "CFG,SPEED,999999999999999999999"),
    ("cfg", "CFG,SPEED,0000000000000000000000001"),
    ("cfg", "CFG,SPEED,800"),
    ("cfg", "CFG,ACCEL,4000"),
    # Formato estrito do ensaio, com gate fechado.
    ("cfg", "CFG,PULSE,25,1"),
    ("cfg", "CFG,PULSE,024,+1"),
    ("cfg", "CFG,PULSE,25,-01"),
    ("cfg", "CFG,GATE,1"),
    ("cfg", "CFG,GATE,1,CONFIRM"),
    ("update", 3000),
    ("cfg", "CFG,PULSE,025,+1"),
    ("target", 999),              # ignorado pelo ensaio
    ("cfg", "CFG,MICRO,8"),       # recusado: ensaio em curso
    ("cfg", "CFG,?"),             # test=25
]
EDGE_SCENARIO += [("update", 4000 + i * 1000) for i in range(130)]
EDGE_SCENARIO += [
    ("cfg", "CFG,?"),             # ensaio concluido
    ("cfg", "CFG,PULSE,1,-1"),
    ("update", 180000),           # salto de tempo emite o pulso
    ("cfg", "CFG,GATE,0"),        # fecha com STEP em alto
    ("update", 181000),
    ("cfg", "CFG,?"),
]


def run_scenario(ops: list[tuple[str, object]] | None = None) -> list[dict]:
    """Aplica o roteiro e devolve um snapshot por operacao."""
    driver = StepDir()
    rows: list[dict] = []
    for index, (kind, value) in enumerate(ops or SCENARIO):
        reply = ""
        if kind == "update":
            driver.update(int(value))
        elif kind == "target":
            driver.set_target(int(value))
        elif kind == "cfg":
            reply = driver.config(str(value))
        else:
            raise ValueError(f"operacao desconhecida: {kind}")
        row = {"index": index, "op": kind, "reply": reply}
        row.update(driver.snapshot())
        rows.append(row)
    return rows
