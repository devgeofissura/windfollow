// Autoteste embarcado do parser laserseg.
// Os vetores de parse vem de tools/protocol_ref.py (gerados no .h ao lado).
// Nao toca GPIO, motores, luz nem WiFi: apenas imprime no Serial.
#ifndef LASERSEG_AUTOTEST_H
#define LASERSEG_AUTOTEST_H

#include <Arduino.h>

#include "autotest_vectors.h"
#include "parser.h"

namespace laserseg {

struct AutoTestResult {
  size_t passed = 0;
  size_t failed = 0;
};

namespace autotest_detail {

inline bool sameResult(const Result& got, const TestVector& expected) {
  return got.kind == expected.kind && got.reason == expected.reason &&
         got.accepted == expected.accepted && got.seq == expected.seq &&
         got.x == expected.x && got.y == expected.y &&
         got.valid == expected.valid && got.enabled == expected.enabled &&
         got.axis == expected.axis && got.direction == expected.direction;
}

inline Result feedText(Session& session, const char* text, int64_t rx_ms,
                       int64_t now_ms = -1) {
  return session.feed(reinterpret_cast<const uint8_t*>(text), strlen(text),
                      rx_ms, now_ms);
}

inline void check(bool condition, const char* name, Print& out,
                  AutoTestResult* tally) {
  if (condition) {
    ++tally->passed;
  } else {
    ++tally->failed;
    out.print("FAIL: ");
    out.println(name);
  }
}

}  // namespace autotest_detail

inline AutoTestResult runAutotest(Print& out) {
  using namespace autotest_detail;
  AutoTestResult tally;

  out.println("AUTOTEST laserseg parser v0.1");

  for (size_t i = 0; i < kTestVectorCount; ++i) {
    const TestVector& v = kTestVectors[i];
    const Result got =
        parse(reinterpret_cast<const uint8_t*>(v.datagram), v.len, v.rx_ms,
              v.now_ms, v.has_last_seq, v.last_seq);
    check(sameResult(got, v), v.name, out, &tally);
  }

  // Estado da sessao: boot seguro.
  Session s;
  check(!s.tracking() && !s.light() && s.axesStopped() &&
            !s.hasLastPacket(),
        "session_boot_safe", out, &tally);
  char telemetry[64];
  formatTelemetry(telemetry, sizeof(telemetry), s);
  check(strcmp(telemetry, "ACK,-1,SAFE,NONE,0") == 0,
        "telemetry_boot", out, &tally);

  // Amostra valida libera eixos; amostra invalida volta a parar.
  feedText(s, "TRACK,1", 100);
  check(s.tracking() && s.axesStopped(), "track_on_keeps_axes_stopped", out,
        &tally);
  feedText(s, "T,1,100,0,1", 101);
  check(!s.axesStopped() && s.lastSeq() == 1 && s.lastPacketMs() == 101,
        "valid_sample_moves_axes", out, &tally);
  formatTelemetry(telemetry, sizeof(telemetry), s);
  check(strcmp(telemetry, "ACK,1,TRACK,NONE,0") == 0,
        "telemetry_track", out, &tally);

  // Amostra com faixa invalida nao muda sequencia, lastPacket nem o movimento.
  const Result bad_range = feedText(s, "T,2,1001,0,1", 102);
  check(!bad_range.accepted && bad_range.reason == Reason::kRange &&
            s.lastSeq() == 1 && s.lastPacketMs() == 101 && !s.axesStopped(),
        "rejected_range_keeps_sequence", out, &tally);

  feedText(s, "T,2,100,0,0", 103);
  check(s.axesStopped() && s.lastSeq() == 2 && s.lastPacketMs() == 103,
        "invalid_sample_stops_axes", out, &tally);

  // TRACK,0 com eixos em movimento deve parar na hora (sessao isolada).
  Session stop_session;
  feedText(stop_session, "TRACK,1", 200);
  feedText(stop_session, "T,1,100,0,1", 201);
  check(!stop_session.axesStopped(), "track_on_with_sample_moves_axes", out,
        &tally);
  feedText(stop_session, "TRACK,0", 202);
  check(stop_session.axesStopped() && !stop_session.tracking() &&
            stop_session.lastPacketMs() == 202,
        "track_off_stops_axes", out, &tally);

  // Comando rejeitado nao muda luz nem lastPacket.
  feedText(s, "LIGHT,1", 104);
  check(s.light() && s.lastPacketMs() == 104, "light_updates_last_packet", out,
        &tally);
  const Result bad = feedText(s, "LIGHT,2", 105);
  check(!bad.accepted && bad.reason == Reason::kRange && s.light() &&
            s.lastPacketMs() == 104,
        "rejected_command_keeps_state", out, &tally);
  formatTelemetry(telemetry, sizeof(telemetry), s);
  check(strcmp(telemetry, "ACK,2,SAFE,RANGE,1") == 0,
        "telemetry_fault", out, &tally);

  // Watchdog: parar eixos, luz OFF, fault LINK nos limites 1200/1201 ms.
  check(!s.poll(1304) && s.light(), "watchdog_inside_window", out, &tally);
  check(s.poll(1305) && !s.light() && !s.tracking() && s.axesStopped(),
        "watchdog_forces_safe", out, &tally);
  formatTelemetry(telemetry, sizeof(telemetry), s);
  check(strcmp(telemetry, "ACK,2,SAFE,LINK,0") == 0,
        "telemetry_link_fault", out, &tally);

  // Comando expirado nunca atualiza o estado.
  Session expired;
  const Result old = feedText(expired, "LIGHT,1", 1000, 2201);
  check(!old.accepted && old.reason == Reason::kAge &&
            !expired.hasLastPacket() && !expired.light(),
        "expired_command_ignored", out, &tally);

  // TRACK,1 reinicia a janela: T,1 -> TRACK,1 -> T,1 deve ser aceito de novo.
  // Sem o TRACK,1 o mesmo T,1 e rejeitado como sequencia repetida.
  Session restart;
  feedText(restart, "T,1,0,0,1", 10);
  feedText(restart, "TRACK,1", 11);
  const Result after_restart = feedText(restart, "T,1,0,0,1", 12);
  check(after_restart.accepted && restart.lastSeq() == 1,
        "track_on_resets_sequence", out, &tally);
  Session no_restart;
  feedText(no_restart, "T,1,0,0,1", 10);
  const Result repeated = feedText(no_restart, "T,1,0,0,1", 11);
  check(!repeated.accepted && repeated.reason == Reason::kSequence,
        "sequence_kept_without_track_on", out, &tally);

  // Limitacoes locais de movimento.
  constexpr int32_t kTravel = 533;
  check(targetFromError(0, kTravel, 0.75f) == 0 &&
            targetFromError(55, kTravel, 0.75f) == 0 &&
            targetFromError(56, kTravel, 0.75f) != 0 &&
            targetFromError(1000, kTravel, 0.75f) == 399 &&
            targetFromError(-1000, kTravel, 1.0f) == -533 &&
            targetFromError(1000, kTravel, 2.0f) == kTravel,
        "target_from_error_limits", out, &tally);
  check(manualTarget(0, 1, kTravel) == 80 &&
            manualTarget(0, -1, kTravel) == -80 &&
            manualTarget(500, 1, kTravel) == 533 &&
            manualTarget(-500, -1, kTravel) == -533,
        "manual_target_limits", out, &tally);
  check(linkExpired(false, 0, 0) && !linkExpired(true, 1000, 2200) &&
            linkExpired(true, 1000, 2201),
        "link_watchdog_boundaries", out, &tally);

  out.print("AUTOTEST total=");
  out.print(tally.passed + tally.failed);
  out.print(" passed=");
  out.print(tally.passed);
  out.print(" failed=");
  out.println(tally.failed);
  out.println(tally.failed == 0 ? "AUTOTEST PASS" : "AUTOTEST FAIL");
  return tally;
}

}  // namespace laserseg

#endif  // LASERSEG_AUTOTEST_H
