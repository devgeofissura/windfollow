#include "DashboardService.h"
#include "WebAssets.h"
#include <WiFi.h>
#include <Preferences.h>

DashboardService::DashboardService() {}

bool DashboardService::begin(ConfigManager* configMgr, SensorService* sensors,
                             EnvironmentAdvisor* envAdvisor, MsaConfig* config,
                             DisplayService* display) {
  _configMgr = configMgr;
  _sensors = sensors;
  _envAdvisor = envAdvisor;
  _config = config;
  _display = display;

  _server = new WebServer(80);
  setupRoutes();
  _server->begin();
  return true;
}

void DashboardService::loop() {
  _server->handleClient();
}

void DashboardService::setupRoutes() {
  _server->on("/", HTTP_GET, std::bind(&DashboardService::handleRoot, this));
  _server->on("/style.css", HTTP_GET, std::bind(&DashboardService::handleStyleCss, this));
  _server->on("/app.js", HTTP_GET, std::bind(&DashboardService::handleAppJs, this));
  _server->on("/manifest.json", HTTP_GET, std::bind(&DashboardService::handleManifest, this));
  _server->on("/favicon.svg", HTTP_GET, std::bind(&DashboardService::handleFaviconSvg, this));
  _server->on("/favicon.png", HTTP_GET, std::bind(&DashboardService::handleFavicon, this));
  _server->on("/favicon.ico", HTTP_GET, std::bind(&DashboardService::handleFavicon, this));
  _server->on("/icon-192.png", HTTP_GET, std::bind(&DashboardService::handleIcon192, this));
  _server->on("/icon-512.png", HTTP_GET, std::bind(&DashboardService::handleIcon512, this));
  _server->on("/sw.js", HTTP_GET, std::bind(&DashboardService::handleSwJs, this));

  _server->on("/api/sensors", HTTP_GET, std::bind(&DashboardService::handleSensors, this));
  _server->on("/api/sensors", HTTP_OPTIONS, std::bind(&DashboardService::handleCorsPreflight, this));
  _server->on("/api/config", HTTP_GET, std::bind(&DashboardService::handleConfigGet, this));
  _server->on("/api/config", HTTP_POST, std::bind(&DashboardService::handleConfigPost, this));
  _server->on("/api/config", HTTP_OPTIONS, std::bind(&DashboardService::handleCorsPreflight, this));
  _server->on("/api/system/info", HTTP_GET, std::bind(&DashboardService::handleSystemInfo, this));
  _server->on("/api/system/info", HTTP_OPTIONS, std::bind(&DashboardService::handleCorsPreflight, this));
  _server->on("/api/wifi", HTTP_GET, std::bind(&DashboardService::handleWifiGet, this));
  _server->on("/api/wifi", HTTP_POST, std::bind(&DashboardService::handleWifiPost, this));
  _server->on("/api/wifi", HTTP_OPTIONS, std::bind(&DashboardService::handleCorsPreflight, this));
  _server->on("/api/system/restart", HTTP_POST, std::bind(&DashboardService::handleSystemRestart, this));
  _server->on("/api/system/restart", HTTP_OPTIONS, std::bind(&DashboardService::handleCorsPreflight, this));

  _server->onNotFound(std::bind(&DashboardService::handleNotFound, this));
}

void DashboardService::handleCorsPreflight() {
  sendWithCors(204, "text/plain", "");
}

void DashboardService::sendWithCors(int code, const char* contentType, const String& body) {
  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  _server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
  _server->send(code, contentType, body);
}

void DashboardService::handleRoot() {
  sendWithCors(200, "text/html", index_html);
}

void DashboardService::handleStyleCss() {
  sendWithCors(200, "text/css", style_css);
}

void DashboardService::handleAppJs() {
  sendWithCors(200, "application/javascript", app_js);
}

void DashboardService::handleManifest() {
  sendWithCors(200, "application/manifest+json", manifest_json);
}

void DashboardService::handleFaviconSvg() {
  sendWithCors(200, "image/svg+xml", favicon_svg);
}

void DashboardService::handleFavicon() {
  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send_P(200, "image/png", reinterpret_cast<const char*>(favicon_png), favicon_png_len);
}

void DashboardService::handleIcon192() {
  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send_P(200, "image/png", reinterpret_cast<const char*>(icon_192_png), icon_192_png_len);
}

void DashboardService::handleIcon512() {
  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send_P(200, "image/png", reinterpret_cast<const char*>(icon_512_png), icon_512_png_len);
}

void DashboardService::handleSwJs() {
  _server->sendHeader("Cache-Control", "no-cache");
  sendWithCors(200, "application/javascript", sw_js);
}

void DashboardService::handleWifiGet() {
  Preferences prefs;
  prefs.begin("msa-wifi", true);
  String mode = prefs.getString("mode", "ap");
  String staSsid = prefs.getString("ssid", "");
  String apSsid = prefs.getString("apSsid", MSA_AP_SSID);
  prefs.end();

  if (apSsid.length() == 0) apSsid = MSA_AP_SSID;

  DynamicJsonDocument doc(384);
  doc["mode"] = mode;
  doc["ssid"] = mode == "sta" ? staSsid : apSsid;
  doc["staSsid"] = staSsid;
  doc["apSsid"] = apSsid;
  doc["configured"] = staSsid.length() > 0 || apSsid.length() > 0;
  String json;
  serializeJson(doc, json);
  sendWithCors(200, "application/json", json);
}

void DashboardService::handleWifiPost() {
  String body = _server->arg("plain");
  DynamicJsonDocument doc(384);
  if (deserializeJson(doc, body)) {
    sendWithCors(400, "application/json", "{\"error\":\"json invalido\"}");
    return;
  }

  String mode = doc["mode"] | "ap";
  String ssid = doc["ssid"] | "";
  String pass = doc["pass"] | "";
  if (mode != "ap" && mode != "sta") {
    sendWithCors(400, "application/json", "{\"error\":\"modo invalido\"}");
    return;
  }
  if (ssid.length() == 0) {
    sendWithCors(400, "application/json", "{\"error\":\"ssid obrigatorio\"}");
    return;
  }

  Preferences prefs;
  prefs.begin("msa-wifi", false);
  prefs.putString("mode", mode);
  if (mode == "sta") {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
  } else {
    prefs.putString("apSsid", ssid);
    prefs.putString("apPass", pass);
  }
  prefs.end();

  sendWithCors(200, "application/json", "{\"ok\":true,\"restart\":true}");
  delay(200);
  ESP.restart();
}

void DashboardService::handleSystemRestart() {
  sendWithCors(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

void DashboardService::handleSensors() {
  DynamicJsonDocument doc(512);
  EnvironmentReading r = _sensors ? _sensors->reading() : EnvironmentReading();

  doc["aht"] = r.ahtOk;
  doc["bmp"] = r.bmpOk;
  if (!isnan(r.temperatureC)) doc["temperature"] = r.temperatureC;
  if (!isnan(r.humidityPct)) doc["humidity"] = r.humidityPct;
  if (!isnan(r.pressureHpa)) doc["pressure"] = r.pressureHpa;
  if (!isnan(r.bmpTemperatureC)) doc["bmpTemperature"] = r.bmpTemperatureC;
  doc["ageMs"] = r.ageMs;
  doc["uptime"] = millis();

  JsonObject alerts = doc.createNestedObject("alerts");
  EnvAlerts a = _envAdvisor ? _envAdvisor->alerts() : EnvAlerts();
  alerts["trendReady"] = a.trendReady;
  alerts["trend3h"] = a.trend3hHpa;
  alerts["trend30m"] = a.trend30mHpa;
  alerts["tempRise"] = a.tempRiseC;
  alerts["rain"] = a.rainLikely;
  alerts["hot"] = a.hot;
  alerts["cold"] = a.cold;
  alerts["dry"] = a.dry;
  alerts["fire"] = a.fire;

  String json;
  serializeJson(doc, json);
  sendWithCors(200, "application/json", json);
}

void DashboardService::handleConfigGet() {
  MsaConfig cfg = _configMgr ? _configMgr->load() : MsaConfig();
  DynamicJsonDocument doc(256);
  doc["readIntervalMs"] = cfg.readIntervalMs;
  doc["hotC"] = cfg.hotC;
  doc["coldC"] = cfg.coldC;
  doc["dryHumidityPct"] = cfg.dryHumidityPct;
  doc["rainHumidityPct"] = cfg.rainHumidityPct;
  String json;
  serializeJson(doc, json);
  sendWithCors(200, "application/json", json);
}

void DashboardService::handleConfigPost() {
  if (!_configMgr) {
    sendWithCors(500, "application/json", "{\"error\":\"config indisponivel\"}");
    return;
  }

  String body = _server->arg("plain");
  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body)) {
    sendWithCors(400, "application/json", "{\"error\":\"json invalido\"}");
    return;
  }

  MsaConfig cfg = _configMgr->load();
  if (doc.containsKey("readIntervalMs")) {
    long v = doc["readIntervalMs"].as<long>();
    cfg.readIntervalMs = static_cast<uint16_t>(constrain(v,
        static_cast<long>(SENSOR_INTERVAL_MIN_MS),
        static_cast<long>(SENSOR_INTERVAL_MAX_MS)));
  }
  if (doc.containsKey("hotC")) {
    float v = doc["hotC"].as<float>();
    if (!isnan(v) && v >= -40.0f && v <= 85.0f) cfg.hotC = v;
  }
  if (doc.containsKey("coldC")) {
    float v = doc["coldC"].as<float>();
    if (!isnan(v) && v >= -40.0f && v <= 85.0f) cfg.coldC = v;
  }
  if (doc.containsKey("dryHumidityPct")) {
    float v = doc["dryHumidityPct"].as<float>();
    if (!isnan(v) && v >= 0.0f && v <= 100.0f) cfg.dryHumidityPct = v;
  }
  if (doc.containsKey("rainHumidityPct")) {
    float v = doc["rainHumidityPct"].as<float>();
    if (!isnan(v) && v >= 0.0f && v <= 100.0f) cfg.rainHumidityPct = v;
  }
  cfg.version = MSA_CONFIG_VERSION;

  if (!_configMgr->save(cfg)) {
    sendWithCors(500, "application/json", "{\"error\":\"falha ao salvar\"}");
    return;
  }
  if (_config) *_config = cfg;

  sendWithCors(200, "application/json", "{\"ok\":true}");
}

void DashboardService::handleSystemInfo() {
  DynamicJsonDocument doc(384);
  doc["uptime"] = millis();
  doc["heap"] = ESP.getFreeHeap();
  doc["minHeap"] = ESP.getMinFreeHeap();
  doc["wifiClients"] = WiFi.getMode() == WIFI_AP ? WiFi.softAPgetStationNum() : 0;
  doc["wifiMode"] = WiFi.getMode() == WIFI_AP ? "ap" : "sta";
  doc["display"] = _display && _display->available();
  if (_display && _display->available()) {
    char addr[8];
    snprintf(addr, sizeof(addr), "0x%02X", _display->address());
    doc["displayAddr"] = addr;
  }
  String json;
  serializeJson(doc, json);
  sendWithCors(200, "application/json", json);
}

void DashboardService::handleNotFound() {
  sendWithCors(404, "text/plain", "Nao encontrado");
}
