#include "parser.h"

#include <stdio.h>
#include <string.h>

namespace laserseg {

namespace {

Result rejected(Reason reason) {
  Result r;
  r.kind = Kind::kRejected;
  r.reason = reason;
  return r;
}

// Inteiro decimal rigoroso: sem espacos, sem '+', sem ponto, sem lixo.
// Satura em vez de estourar para que a checagem de faixa continue decidindo.
bool readInteger(const char* text, size_t len, int64_t* out) {
  if (len == 0) return false;
  size_t i = 0;
  bool negative = false;
  if (text[0] == '-') {
    negative = true;
    i = 1;
    if (i == len) return false;
  }
  int64_t magnitude = 0;
  // Tudo acima de 2^31 ja sera rejeitado pela checagem de faixa; saturar aqui
  // evita estouro de int64 sem alterar nenhuma decisao do parser.
  constexpr int64_t kSaturateAt = 2147483648LL;
  for (; i < len; ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    if (magnitude < kSaturateAt) {
      magnitude = magnitude * 10 + static_cast<int64_t>(text[i] - '0');
    }
  }
  *out = negative ? -magnitude : magnitude;
  return true;
}

bool readBit(const char* text, size_t len, int64_t* value, Reason* reason) {
  if (!readInteger(text, len, value)) {
    *reason = Reason::kFormat;
    return false;
  }
  if (*value != 0 && *value != 1) {
    *reason = Reason::kRange;
    return false;
  }
  return true;
}

struct Fields {
  const char* data[6];
  size_t len[6];
  size_t count = 0;
  bool too_many = false;
};

Fields splitFields(const uint8_t* bytes, size_t len) {
  Fields fields;
  size_t begin = 0;
  for (size_t i = 0; i <= len; ++i) {
    if (i == len || bytes[i] == ',') {
      if (fields.count < 6) {
        fields.data[fields.count] =
            reinterpret_cast<const char*>(bytes + begin);
        fields.len[fields.count] = i - begin;
        ++fields.count;
      } else {
        fields.too_many = true;
      }
      begin = i + 1;
    }
  }
  return fields;
}

bool equals(const char* text, size_t len, const char* literal) {
  return strlen(literal) == len && memcmp(text, literal, len) == 0;
}

Result checkAge(Result result, int64_t rx_ms, int64_t now_ms) {
  if (!result.accepted) return result;
  const int64_t effective_now = now_ms < 0 ? rx_ms : now_ms;
  const int64_t age = effective_now - rx_ms;
  if (age < 0 || age > kLinkTimeoutMs) return rejected(Reason::kAge);
  return result;
}

}  // namespace

const char* kindName(Kind kind) {
  switch (kind) {
    case Kind::kTrack: return "track";
    case Kind::kLight: return "light";
    case Kind::kManual: return "manual";
    case Kind::kSample: return "sample";
    case Kind::kRejected: return "rejected";
  }
  return "rejected";
}

const char* reasonName(Reason reason) {
  switch (reason) {
    case Reason::kNone: return "none";
    case Reason::kLength: return "length";
    case Reason::kEncoding: return "encoding";
    case Reason::kFormat: return "format";
    case Reason::kRange: return "range";
    case Reason::kSequence: return "sequence";
    case Reason::kAge: return "age";
    case Reason::kLink: return "link";
  }
  return "none";
}

const char* axisName(Axis axis) {
  switch (axis) {
    case Axis::kPan: return "pan";
    case Axis::kTilt: return "tilt";
    case Axis::kNone: return "none";
  }
  return "none";
}

Result parse(const uint8_t* bytes, size_t len, int64_t rx_ms, int64_t now_ms,
             bool has_last_seq, int32_t last_seq) {
  if (bytes == nullptr || len == 0 || len > kMaxDatagramBytes) {
    return rejected(Reason::kLength);
  }
  for (size_t i = 0; i < len; ++i) {
    if (bytes[i] > 0x7f || bytes[i] == 0x00) {
      return rejected(Reason::kEncoding);
    }
  }

  const Fields fields = splitFields(bytes, len);
  if (fields.too_many || fields.count == 0) {
    return rejected(Reason::kFormat);
  }
  const char* command = fields.data[0];
  const size_t command_len = fields.len[0];

  Result result;
  if (equals(command, command_len, "TRACK") ||
      equals(command, command_len, "LIGHT")) {
    if (fields.count != 2) return rejected(Reason::kFormat);
    int64_t value = 0;
    Reason reason = Reason::kNone;
    if (!readBit(fields.data[1], fields.len[1], &value, &reason)) {
      return rejected(reason);
    }
    result.kind = equals(command, command_len, "TRACK") ? Kind::kTrack
                                                        : Kind::kLight;
    result.accepted = true;
    result.enabled = value == 1;

  } else if (equals(command, command_len, "MANUAL")) {
    if (fields.count != 3) return rejected(Reason::kFormat);
    if (!equals(fields.data[1], fields.len[1], "pan") &&
        !equals(fields.data[1], fields.len[1], "tilt")) {
      return rejected(Reason::kRange);
    }
    int64_t direction = 0;
    if (!readInteger(fields.data[2], fields.len[2], &direction)) {
      return rejected(Reason::kFormat);
    }
    if (direction != -1 && direction != 1) return rejected(Reason::kRange);
    result.kind = Kind::kManual;
    result.accepted = true;
    result.axis = equals(fields.data[1], fields.len[1], "pan") ? Axis::kPan
                                                               : Axis::kTilt;
    result.direction = static_cast<int8_t>(direction);

  } else if (equals(command, command_len, "T")) {
    if (fields.count != 5) return rejected(Reason::kFormat);
    int64_t seq = 0;
    int64_t x = 0;
    int64_t y = 0;
    int64_t valid = 0;
    Reason reason = Reason::kNone;
    if (!readInteger(fields.data[1], fields.len[1], &seq) ||
        !readInteger(fields.data[2], fields.len[2], &x) ||
        !readInteger(fields.data[3], fields.len[3], &y)) {
      return rejected(Reason::kFormat);
    }
    // _bit distingue sintaxe (format) de faixa (range); o valor de retorno
    // so diz "nao leu". O motivo real esta em reason, como em protocol_ref.py.
    readBit(fields.data[4], fields.len[4], &valid, &reason);
    if (reason == Reason::kFormat) return rejected(Reason::kFormat);
    if (seq < 0 || seq > kInt32Max || x < -1000 || x > 1000 || y < -1000 ||
        y > 1000 || reason == Reason::kRange) {
      return rejected(Reason::kRange);
    }
    if (has_last_seq && seq <= last_seq) return rejected(Reason::kSequence);

    result.kind = Kind::kSample;
    result.accepted = true;
    result.seq = static_cast<int32_t>(seq);
    result.x = static_cast<int32_t>(x);
    result.y = static_cast<int32_t>(y);
    result.valid = valid == 1;

  } else {
    return rejected(Reason::kFormat);
  }

  return checkAge(result, rx_ms, now_ms);
}

int32_t clampInt(int32_t value, int32_t limit) {
  if (value > limit) return limit;
  if (value < -limit) return -limit;
  return value;
}

int32_t targetFromError(int32_t error, int32_t max_travel, float gain,
                        int32_t dead_band) {
  if (error >= -dead_band && error <= dead_band) return 0;
  const float raw = static_cast<float>(error) * static_cast<float>(max_travel) *
                    gain / 1000.0f;
  return clampInt(static_cast<int32_t>(raw), max_travel);
}

int32_t manualTarget(int32_t current, int8_t direction, int32_t max_travel,
                     int32_t step) {
  if (direction != -1 && direction != 1) return current;
  const int64_t next = static_cast<int64_t>(current) +
                       static_cast<int64_t>(direction) * step;
  const int64_t limit = max_travel < 0 ? 0 : max_travel;
  if (next > limit) return static_cast<int32_t>(limit);
  if (next < -limit) return static_cast<int32_t>(-limit);
  return static_cast<int32_t>(next);
}

bool linkExpired(bool has_last_packet, int64_t last_packet_ms, int64_t now_ms) {
  if (!has_last_packet) return true;
  return now_ms - last_packet_ms > kLinkTimeoutMs;
}

void Session::reset() {
  has_last_packet_ = false;
  last_packet_ms_ = 0;
  has_last_seq_ = false;
  last_seq_ = 0;
  tracking_ = false;
  light_ = false;
  axes_stopped_ = true;
  fault_ = Reason::kNone;
}

Result Session::feed(const uint8_t* bytes, size_t len, int64_t rx_ms,
                     int64_t now_ms) {
  const Result result =
      parse(bytes, len, rx_ms, now_ms, has_last_seq_, last_seq_);
  if (!result.accepted) {
    fault_ = result.reason;
    return result;
  }

  fault_ = Reason::kNone;
  has_last_packet_ = true;
  last_packet_ms_ = rx_ms;
  if (result.kind == Kind::kTrack) {
    tracking_ = result.enabled;
    if (result.enabled) {
      has_last_seq_ = false;
      last_seq_ = 0;
    } else {
      axes_stopped_ = true;
    }
  } else if (result.kind == Kind::kLight) {
    light_ = result.enabled;
  } else if (result.kind == Kind::kManual) {
    axes_stopped_ = false;
  } else if (result.kind == Kind::kSample) {
    has_last_seq_ = true;
    last_seq_ = result.seq;
    axes_stopped_ = !(tracking_ && result.valid);
  }
  return result;
}

bool Session::poll(int64_t now_ms) {
  if (!linkExpired(has_last_packet_, last_packet_ms_, now_ms)) return false;
  tracking_ = false;
  light_ = false;
  axes_stopped_ = true;
  fault_ = Reason::kLink;
  return true;
}

void formatTelemetry(char* out, size_t out_size, const Session& session) {
  if (out_size == 0) return;
  const char* state = "SAFE";
  if (!session.axesStopped()) {
    state = session.tracking() ? "TRACK" : "MANUAL";
  }
  const Reason fault = session.fault();
  const char* fault_name = reasonName(fault);
  const int seq = session.hasLastSeq() ? session.lastSeq() : -1;
  snprintf(out, out_size, "ACK,%d,%s,%s,%d", seq, state, fault_name,
           session.light() ? 1 : 0);
}

}  // namespace laserseg
