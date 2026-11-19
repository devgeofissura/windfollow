// Parser UDP do laserseg - especificacao executavel em C++.
// Espelha firmware/laserseg/tools/protocol_ref.py (contrato v0.1).
// Nao usa WiFi, GPIO, motores nin luz: apenas logica pura e state machine.
#ifndef LASERSEG_PARSER_H
#define LASERSEG_PARSER_H

#include <stddef.h>
#include <stdint.h>

namespace laserseg {

constexpr size_t kMaxDatagramBytes = 96;
constexpr int32_t kLinkTimeoutMs = 1200;
constexpr int32_t kDeadBand = 55;
constexpr int32_t kManualStep = 80;
constexpr int32_t kInt32Max = 2147483647;

enum class Kind { kRejected, kTrack, kLight, kManual, kSample };
enum class Reason {
  kNone,
  kLength,
  kEncoding,
  kFormat,
  kRange,
  kSequence,
  kAge,
  kLink  // nao e motivo de rejeicao: watchdog de link disparou
};
enum class Axis { kNone, kPan, kTilt };

const char* kindName(Kind kind);
const char* reasonName(Reason reason);
const char* axisName(Axis axis);

struct Result {
  Kind kind = Kind::kRejected;
  Reason reason = Reason::kNone;
  bool accepted = false;
  int32_t seq = 0;
  int32_t x = 0;
  int32_t y = 0;
  bool valid = false;
  bool enabled = false;
  Axis axis = Axis::kNone;
  int8_t direction = 0;
};

// Ordem: tamanho/codificacao -> formato -> faixa -> sequencia -> idade.
// A idade e aplicada a QUALQUER comando aceito, por ultimo.
// now_ms < 0 significa "sem atraso de fila" (usa rx_ms).
Result parse(const uint8_t* bytes, size_t len, int64_t rx_ms, int64_t now_ms,
             bool has_last_seq, int32_t last_seq);

int32_t clampInt(int32_t value, int32_t limit);
int32_t targetFromError(int32_t error, int32_t max_travel, float gain,
                        int32_t dead_band = kDeadBand);
int32_t manualTarget(int32_t current, int8_t direction, int32_t max_travel,
                     int32_t step = kManualStep);
bool linkExpired(bool has_last_packet, int64_t last_packet_ms, int64_t now_ms);

// Estado da controladora. Boot seguro: eixos parados, luz apagada.
class Session {
 public:
  void reset();
  Result feed(const uint8_t* bytes, size_t len, int64_t rx_ms,
              int64_t now_ms = -1);
  // Watchdog: true exige parar AMBOS os eixos e desligar a luz.
  bool poll(int64_t now_ms);

  bool tracking() const { return tracking_; }
  bool light() const { return light_; }
  bool axesStopped() const { return axes_stopped_; }
  bool hasLastPacket() const { return has_last_packet_; }
  int64_t lastPacketMs() const { return last_packet_ms_; }
  bool hasLastSeq() const { return has_last_seq_; }
  int32_t lastSeq() const { return last_seq_; }
  Reason fault() const { return fault_; }

 private:
  bool has_last_packet_ = false;
  int64_t last_packet_ms_ = 0;
  bool has_last_seq_ = false;
  int32_t last_seq_ = 0;
  bool tracking_ = false;
  bool light_ = false;
  bool axes_stopped_ = true;
  Reason fault_ = Reason::kNone;
};

// Telemetria de retorno (ainda sem envio de rede):
//   ACK,<seq>,<state>,<fault>,<light>
// state: SAFE | TRACK | MANUAL ; fault: NONE | LENGTH | ENCODING |
// FORMAT | RANGE | SEQUENCE | AGE | LINK.
// seq = -1 enquanto nao houver amostra aceita.
void formatTelemetry(char* out, size_t out_size, const Session& session);

}  // namespace laserseg

#endif  // LASERSEG_PARSER_H
