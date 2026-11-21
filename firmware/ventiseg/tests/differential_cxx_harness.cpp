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

int main() {
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
