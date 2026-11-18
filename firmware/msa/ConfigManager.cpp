#include "ConfigManager.h"

const char* ConfigManager::NVS_NAMESPACE = "msa";
const char* ConfigManager::NVS_KEY       = "msa_cfg";

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
  return _prefs.begin(NVS_NAMESPACE, false) == true;
}

MsaConfig ConfigManager::load() {
  MsaConfig cfg;
  if (!_prefs.begin(NVS_NAMESPACE, true)) {
    return cfg;
  }

  size_t len = _prefs.getBytesLength(NVS_KEY);
  if (len == sizeof(MsaConfig)) {
    _prefs.getBytes(NVS_KEY, &cfg, sizeof(MsaConfig));
    if (cfg.version != MSA_CONFIG_VERSION) {
      cfg = MsaConfig();
    }
  }
  _prefs.end();
  return cfg;
}

bool ConfigManager::save(const MsaConfig& config) {
  if (!_prefs.begin(NVS_NAMESPACE, false)) {
    return false;
  }
  bool ok = _prefs.putBytes(NVS_KEY, &config, sizeof(MsaConfig));
  _prefs.end();
  return ok;
}

void ConfigManager::resetToDefaults() {
  if (!_prefs.begin(NVS_NAMESPACE, false)) {
    return;
  }
  _prefs.remove(NVS_KEY);
  _prefs.end();
}
