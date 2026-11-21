// Harness diferencial da camada STEP/DIR em C++.
// Entrada: uma operacao por linha -> indice|kind|valor
// Saida:   indice|kind|reply|steps|speed|accel|gate|target|pos|dir|pulses|step|dirlevel|test|remaining|stepwrites|dirwrites|stephigh
#include "step_dir.cpp"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_step_writes = 0;
int g_dir_writes = 0;
int g_step_high_writes = 0;

void onStep(bool level) {
  ++g_step_writes;
  if (level) ++g_step_high_writes;
}

void onDir(bool) { ++g_dir_writes; }

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

}  // namespace

int main() {
  ventiseg::StepDir driver(onStep, onDir);
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> fields = split(line, '|');
    if (fields.size() != 3) {
      std::cerr << "operacao invalida: " << line << '\n';
      return 2;
    }
    const std::string& index = fields[0];
    const std::string& kind = fields[1];
    const std::string& value = fields[2];
    std::string reply;
    if (kind == "update") {
      driver.update(std::stoll(value));
    } else if (kind == "target") {
      driver.setTarget(std::stoi(value));
    } else if (kind == "cfg") {
      reply = driver.config(value.c_str());
    } else {
      std::cerr << "kind desconhecido: " << kind << '\n';
      return 2;
    }
    std::printf("%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                index.c_str(), kind.c_str(), reply.c_str(),
                driver.stepsPerRev(), driver.speed(), driver.accel(),
                driver.gate() ? 1 : 0, driver.target(), driver.position(),
                driver.direction(), driver.pulses(),
                driver.stepLevel() ? 1 : 0, driver.dirLevel() ? 1 : 0,
                driver.testMode() ? 1 : 0, driver.testRemaining(),
                g_step_writes, g_dir_writes, g_step_high_writes);
  }
  return 0;
}
