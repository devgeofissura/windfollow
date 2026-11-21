// Autoteste embarcado do parser ventiseg.
// Os vetores de parse vem de tools/protocol_ref.py (gerados no .h ao lado).
// Nao toca GPIO, motores, reles, fins de curso, luz nem WiFi: so Serial.
#ifndef VENTISEG_AUTOTEST_H
#define VENTISEG_AUTOTEST_H

#include <Arduino.h>

#include "autotest_vectors.h"
#include "parser.h"

namespace ventiseg {

struct AutoTestResult {
  size_t passed = 0;
  size_t failed = 0;
};

namespace autotest_detail {

inline bool sameResult(const Result& got, const TestVector& expected) {
  return got.kind == expected.kind && got.reason == expected.reason &&
         got.accepted == expected.accepted && got.seq == expected.seq &&
         got.x == expected.x && got.valid == expected.valid &&
         got.enabled == expected.enabled && got.direction == expected.direction;
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

inline void checkTelemetry(const char* expected, const Session& session,
                           const char* name, Print& out,
                           AutoTestResult* tally) {
  char telemetry[64];
  formatTelemetry(telemetry, sizeof(telemetry), session);
  if (strcmp(telemetry, expected) == 0) {
    ++tally->passed;
    return;
  }
  ++tally->failed;
  out.print("FAIL: ");
  out.print(name);
  out.print(" expected=");
  out.print(expected);
  out.print(" got=");
  out.println(telemetry);
}

}  // namespace autotest_detail

inline AutoTestResult runAutotest(Print& out) {
  using namespace autotest_detail;
  AutoTestResult tally;

  out.println("AUTOTEST ventiseg parser v0.1");

  for (size_t i = 0; i < kTestVectorCount; ++i) {
    const TestVector& v = kTestVectors[i];
    const Result got =
        parse(reinterpret_cast<const uint8_t*>(v.datagram), v.len, v.rx_ms,
              v.now_ms, v.has_last_seq, v.last_seq);
    check(sameResult(got, v), v.name, out, &tally);
  }

  // Boot seguro: eixo parado, SAFE, fault None, seq -1.
  Session s;
  check(s.mode() == State::kSafe && s.fault() == Fault::kNone &&
            s.seq() == -1 && s.axis() == 0 && !s.moving() &&
            !s.hasLastSample(),
        "session_boot_safe", out, &tally);
  checkTelemetry("ACK,-1,SAFE,None", s, "telemetry_boot", out, &tally);

  // TRACK nao altera o eixo; uma amostra valida em TRACK comanda o eixo.
  feedText(s, "TRACK,1", 100);
  check(s.mode() == State::kTrack && s.moving() && s.axis() == 0,
        "track_on_starts_track", out, &tally);
  feedText(s, "T,1,100,1", 101);
  check(s.seq() == 1 && s.moving() && s.axis() == trackSteps(100) &&
            s.lastSampleMs() == 101,
        "valid_sample_moves_axis", out, &tally);
  checkTelemetry("ACK,1,TRACK,None", s, "telemetry_track", out, &tally);

  // Comando invalido nao altera modo/eixo/sequencia/relogio; reporta Range.
  const Result bad_range = feedText(s, "T,2,1001,1", 102);
  const int32_t axis_before = s.axis();
  check(!bad_range.accepted && bad_range.reason == Reason::kRange &&
            s.mode() == State::kTrack && s.axis() == axis_before &&
            s.seq() == 1 && s.lastSampleMs() == 101,
        "rejected_range_keeps_state", out, &tally);
  check(s.fault() == Fault::kRange, "rejected_range_reports_fault", out,
        &tally);
  checkTelemetry("ACK,1,TRACK,Range", s, "telemetry_range_fault", out,
                 &tally);

  // Sequencia repetida nao muda estado; amostra valida=0 para o eixo.
  const Result repeated = feedText(s, "T,1,100,1", 103);
  check(!repeated.accepted && repeated.reason == Reason::kSequence &&
            s.seq() == 1 && s.axis() == axis_before,
        "sequence_rejected", out, &tally);
  feedText(s, "T,2,100,0", 103);
  check(s.seq() == 2 && s.axis() == 0 && !s.moving(),
        "invalid_sample_stops_axis", out, &tally);
  checkTelemetry("ACK,2,TRACK,None", s, "telemetry_after_sample", out,
                 &tally);

  // Watchdog: 1200 ms ainda dentro da janela, 1201 ms fora.
  check(!s.poll(1303), "watchdog_inside_window", out, &tally);
  check(s.poll(1304) && s.mode() == State::kSafe && !s.moving() &&
            s.seq() == 2,
        "watchdog_forces_safe", out, &tally);
  check(s.fault() == Fault::kLnk, "watchdog_fault_lnk", out, &tally);
  checkTelemetry("ACK,2,SAFE,Lnk", s, "telemetry_link_fault", out, &tally);
  check(!s.poll(9000), "watchdog_fires_once", out, &tally);

  // Seguranca pos-Lnk: comandos de movimento sao validos, mas inibidos.
  const Result blocked_track = feedText(s, "TRACK,1", 1400);
  check(blocked_track.accepted && s.fault() == Fault::kLnk &&
            s.mode() == State::kSafe && !s.moving(),
        "track_inhibited_after_lnk", out, &tally);
  const Result blocked_manual = feedText(s, "MANUAL,1", 1401);
  check(blocked_manual.accepted && s.fault() == Fault::kLnk &&
            s.mode() == State::kSafe && !s.moving() && s.axis() == 0,
        "manual_inhibited_after_lnk", out, &tally);
  const Result blocked_range = feedText(s, "TRACK,2", 1402);
  check(!blocked_range.accepted && blocked_range.reason == Reason::kRange &&
            s.fault() == Fault::kLnk,
        "range_does_not_override_lnk", out, &tally);
  checkTelemetry("ACK,2,SAFE,Lnk", s, "telemetry_link_inhibited", out, &tally);

  // Somente T aceito restabelece o link e ainda exige novo comando de movimento.
  const Result relink = feedText(s, "T,3,0,1", 1410);
  check(relink.accepted && s.fault() == Fault::kNone && !s.linkLost() &&
            s.mode() == State::kSafe && !s.moving(),
        "sample_clears_lnk", out, &tally);
  checkTelemetry("ACK,3,SAFE,None", s, "telemetry_relink", out, &tally);
  const Result retrack = feedText(s, "TRACK,1", 1411);
  check(retrack.accepted && s.mode() == State::kTrack && s.moving(),
        "retrack_after_relink", out, &tally);

  // Manual: passo de 80, parada e limites por software.
  Session manual;
  feedText(manual, "MANUAL,1", 10);
  check(manual.mode() == State::kManual && manual.moving() &&
            manual.axis() == kManualStep,
        "manual_forward", out, &tally);
  feedText(manual, "MANUAL,-1", 11);
  check(manual.axis() == 0, "manual_backward", out, &tally);
  feedText(manual, "MANUAL,0", 12);
  check(manual.mode() == State::kSafe && !manual.moving(),
        "manual_stop", out, &tally);
  for (int i = 0; i < 20; ++i) feedText(manual, "MANUAL,1", 20 + i);
  check(manual.axis() == kMaxTravel, "manual_clamp_forward", out, &tally);
  for (int i = 0; i < 30; ++i) feedText(manual, "MANUAL,-1", 60 + i);
  check(manual.axis() == -kMaxTravel, "manual_clamp_backward", out, &tally);

  // Idade expirada nunca atualiza estado, e TRACK nao renova o watchdog.
  Session expired;
  const Result old = feedText(expired, "T,2,100,1", 1000, 2201);
  check(!old.accepted && old.reason == Reason::kAge &&
            !expired.hasLastSample() && expired.seq() == -1,
        "expired_command_ignored", out, &tally);
  Session no_refresh;
  feedText(no_refresh, "T,1,100,1", 1000);
  feedText(no_refresh, "TRACK,1", 2000);
  check(no_refresh.poll(2300), "only_sample_refreshes_watchdog", out,
        &tally);

  // Helpers de movimento e watchdog com os limites do contrato.
  check(trackSteps(0) == 0 && trackSteps(55) == 0 &&
            trackSteps(56) == 22 && trackSteps(1000) == 399 &&
            trackSteps(-1000) == -399 && trackSteps(1000, kMaxTravel, 2.0f) ==
                                            kMaxTravel,
        "track_steps_limits", out, &tally);
  check(manualTarget(0, 1) == 80 && manualTarget(0, -1) == -80 &&
            manualTarget(500, 1) == kMaxTravel &&
            manualTarget(-500, -1) == -kMaxTravel,
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

}  // namespace ventiseg

#endif  // VENTISEG_AUTOTEST_H
