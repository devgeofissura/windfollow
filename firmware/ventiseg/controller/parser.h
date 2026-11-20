// Parser serial do ventiseg - especificacao executavel em C++.
// Espelha firmware/ventiseg/tools/protocol_ref.py (contrato v0.1).
// Um eixo de pan do fluxo de ar. Nao usa GPIO, WiFi, motor, luz, rele,
// fim de curso nem tilt: apenas logica pura e maquina de estados.
#ifndef VENTISEG_PARSER_H
#define VENTISEG_PARSER_H

#include <stddef.h>
#include <stdint.h>

namespace ventiseg {

constexpr size_t kMaxDatagramBytes = 96;
constexpr int32_t kLinkTimeoutMs = 1200;
constexpr int32_t kDeadBand = 55;
constexpr int32_t kManualStep = 80;
constexpr int32_t kStepsPerRev = 3200;
constexpr int32_t kMaxTravel = 533;
constexpr float kTrackGain = 0.75f;
constexpr int32_t kXLimit = 1000;
constexpr int32_t kInt32Max = 2147483647;

enum class Kind { kRejected, kTrack, kManual, kSample };
enum class Reason {
  kNone,
  kLength,
  kEncoding,
  kFormat,
  kRange,
  kSequence,
  kAge
};
enum class State { kSafe, kTrack, kManual };
enum class Fault { kNone, kLnk, kRange };

const char* kindName(Kind kind);
const char* reasonName(Reason reason);
const char* stateName(State state);
const char* faultName(Fault fault);

struct Result {
  Kind kind = Kind::kRejected;
  Reason reason = Reason::kNone;
  bool accepted = false;
  int32_t seq = 0;
  int32_t x = 0;
  bool valid = false;
  bool enabled = false;
  int8_t direction = 0;
};

// Ordem: tamanho/codificacao -> formato -> faixa -> sequencia -> idade.
// A idade e aplicada a QUALQUER comando aceito, por ultimo.
// now_ms < 0 significa "sem atraso de fila" (usa rx_ms).
Result parse(const uint8_t* bytes, size_t len, int64_t rx_ms, int64_t now_ms,
             bool has_last_seq, int32_t last_seq);

int32_t clampInt(int32_t value, int32_t limit);
int32_t trackSteps(int32_t x, int32_t max_travel = kMaxTravel,
                   float gain = kTrackGain, int32_t dead_band = kDeadBand);
int32_t manualTarget(int32_t current, int8_t direction,
                     int32_t max_travel = kMaxTravel,
                     int32_t step = kManualStep);
bool linkExpired(bool has_last_sample, int64_t last_sample_ms, int64_t now_ms);

// Estado da controladora. Boot seguro: eixo parado, SAFE, fault None, seq -1.
// Somente amostras T aceitas renovam o watchdog de 1200 ms.
class Session {
 public:
  Result feed(const uint8_t* bytes, size_t len, int64_t rx_ms,
              int64_t now_ms = -1);
  // Watchdog: true exige parar o eixo e ir para SAFE com fault=Lnk.
  bool poll(int64_t now_ms);

  State mode() const { return mode_; }
  Fault fault() const { return fault_; }
  int32_t seq() const { return seq_; }
  int32_t axis() const { return axis_; }
  bool moving() const { return moving_; }
  bool hasLastSample() const { return has_last_sample_; }
  int64_t lastSampleMs() const { return last_sample_ms_; }
  bool hasLastSeq() const { return has_last_seq_; }
  bool linkLost() const { return link_lost_; }

 private:
  bool has_last_sample_ = false;
  int64_t last_sample_ms_ = 0;
  bool has_last_seq_ = false;
  int32_t seq_ = -1;
  int32_t axis_ = 0;
  State mode_ = State::kSafe;
  Fault fault_ = Fault::kNone;
  bool moving_ = false;
  bool link_lost_ = false;
};

// Telemetria de retorno (sem rede): ACK,<seq>,<state>,<fault>
// state: SAFE | TRACK | MANUAL ; fault: None | Lnk | Range.
// seq = -1 enquanto nao houver amostra T aceita.
void formatTelemetry(char* out, size_t out_size, const Session& session);

}  // namespace ventiseg

#endif  // VENTISEG_PARSER_H
