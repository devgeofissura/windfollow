#ifndef DASHBOARD_SERVICE_H
#define DASHBOARD_SERVICE_H

#include "HardwareConfig.h"
#include "ConfigManager.h"
#include "SensorService.h"
#include "EnvironmentAdvisor.h"
#include <WebServer.h>
#include <ArduinoJson.h>

class DashboardService {
public:
  DashboardService();

  bool begin(ConfigManager* configMgr, SensorService* sensors,
             EnvironmentAdvisor* envAdvisor, MsaConfig* config);
  void loop();

private:
  WebServer* _server = nullptr;
  ConfigManager* _configMgr = nullptr;
  SensorService* _sensors = nullptr;
  EnvironmentAdvisor* _envAdvisor = nullptr;
  MsaConfig* _config = nullptr;

  void setupRoutes();
  void handleCorsPreflight();
  void sendWithCors(int code, const char* contentType, const String& body);

  void handleRoot();
  void handleStyleCss();
  void handleAppJs();
  void handleManifest();
  void handleFaviconSvg();
  void handleFavicon();
  void handleIcon192();
  void handleIcon512();
  void handleSwJs();

  void handleWifiGet();
  void handleWifiPost();
  void handleSystemRestart();
  void handleSensors();
  void handleConfigGet();
  void handleConfigPost();
  void handleSystemInfo();
  void handleNotFound();
};

#endif
