// Harness de testes diferenciais do parser C++ do ventiseg.
// Inclui a implementacao real (parser.cpp) e imprime uma decisao por linha.
// Formato de entrada (uma linha por caso):
//   nome|hex_do_datagrama|rx_ms|now_ms|tem_last_seq|last_seq
// now_ms = -1 significa "sem atraso de fila".
// Saida: nome|kind|reason|accepted|seq|x|valid|enabled|direction

#include "parser.cpp"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split(const std::string& text, char sep) {
  std::vector<std::string> parts;
  size_t begin = 0;
  for (size_t i = 0; i <= text.size(); ++i) {
    if (i == text.size() || text[i] == sep) {
      parts.push_back(text.substr(begin, i - begin));
      begin = i + 1;
    }
  }
  return parts;
}

int hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::vector<uint8_t> decodeHex(const std::string& text) {
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i + 1 < text.size(); i += 2) {
    const int high = hexDigit(text[i]);
    const int low = hexDigit(text[i + 1]);
    if (high < 0 || low < 0) return {};
    bytes.push_back(static_cast<uint8_t>((high << 4) | low));
  }
  return bytes;
}

}  // namespace

struct Step {
  bool feed = false;
  std::vector<uint8_t> bytes;
  int64_t rx_ms = 0;
  int64_t now_ms = 0;
};

struct Script {
  std::string name;
  std::vector<Step> steps;
};

void printTrace(const std::string& name, size_t index, const char* op,
                bool result, const ventiseg::Session& session) {
  char telemetry[64];
  ventiseg::formatTelemetry(telemetry, sizeof(telemetry), session);
  std::printf("%s|%zu|%s|%d|%s|%s|%s|%d|%d|%d|%d\n",
              name.c_str(), index, op, result ? 1 : 0, telemetry,
              ventiseg::stateName(session.mode()),
              ventiseg::faultName(session.fault()),
              session.seq(),
              session.axis(),
              session.moving() ? 1 : 0,
              session.linkLost() ? 1 : 0);
}

int runSessionMode() {
  std::vector<Script> scripts;
  std::string line;
  Script current;
  bool inside = false;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    if (line[0] == '@') {
      if (inside) scripts.push_back(current);
      current = Script{line.substr(1), {}};
      inside = true;
      continue;
    }
    if (!inside) {
      std::cerr << "script sem cabecalho: " << line << '\n';
      return 2;
    }
    const std::vector<std::string> f = split(line, '|');
    Step step;
    if (f[0] == "F" && f.size() == 4) {
      step.feed = true;
      step.bytes = decodeHex(f[1]);
      step.rx_ms = std::stoll(f[2]);
      step.now_ms = std::stoll(f[3]);
    } else if (f[0] == "P" && f.size() == 2) {
      step.now_ms = std::stoll(f[1]);
    } else {
      std::cerr << "passo invalido: " << line << '\n';
      return 2;
    }
    current.steps.push_back(step);
  }
  if (inside) scripts.push_back(current);

  for (const Script& script : scripts) {
    ventiseg::Session session;
    for (size_t i = 0; i < script.steps.size(); ++i) {
      const Step& step = script.steps[i];
      if (step.feed) {
        const ventiseg::Result r = session.feed(
            step.bytes.empty() ? nullptr : step.bytes.data(),
            step.bytes.size(), step.rx_ms, step.now_ms);
        printTrace(script.name, i, "feed", r.accepted, session);
      } else {
        const bool tripped = session.poll(step.now_ms);
        printTrace(script.name, i, "poll", tripped, session);
      }
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "session") {
    return runSessionMode();
  }
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> f = split(line, '|');
    if (f.size() != 6) {
      std::cerr << "caso invalido: " << line << '\n';
      return 2;
    }
    const std::vector<uint8_t> bytes = decodeHex(f[1]);
    const int64_t rx_ms = std::stoll(f[2]);
    const int64_t now_ms = std::stoll(f[3]);
    const bool has_last_seq = std::stoi(f[4]) != 0;
    const int32_t last_seq = static_cast<int32_t>(std::stoll(f[5]));

    const ventiseg::Result r = ventiseg::parse(
        bytes.empty() ? nullptr : bytes.data(), bytes.size(), rx_ms, now_ms,
        has_last_seq, last_seq);

    std::printf("%s|%s|%s|%d|%d|%d|%d|%d|%d\n",
                f[0].c_str(),
                ventiseg::kindName(r.kind),
                ventiseg::reasonName(r.reason),
                r.accepted ? 1 : 0,
                r.seq,
                r.x,
                r.valid ? 1 : 0,
                r.enabled ? 1 : 0,
                static_cast<int>(r.direction));
  }
  return 0;
}
