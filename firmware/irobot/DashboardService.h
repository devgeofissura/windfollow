#ifndef DASHBOARD_SERVICE_H
#define DASHBOARD_SERVICE_H

#include "HardwareConfig.h"
#include "L298NDualDriver.h"
#include "MotorEngine.h"
#include "ConfigManager.h"
#include "SensorService.h"
#include "AutoNavigator.h"
#include <WebServer.h>
#include <ArduinoJson.h>

class DashboardService {
public:
  DashboardService();

  bool begin(MotorEngine* engine, L298NDualDriver* driver, ConfigManager* configMgr,
             SensorService* sensors, AutoNavigator* autoNav);
  void loop();

private:
  WebServer* _server = nullptr;
  MotorEngine* _engine = nullptr;
  L298NDualDriver* _driver = nullptr;
  ConfigManager* _configMgr = nullptr;
  SensorService* _sensors = nullptr;
  AutoNavigator* _autoNav = nullptr;

  void setupRoutes();
  void handleCorsPreflight();
  void handleFavicon();
  void sendWithCors(int code, const char* contentType, const String& body);
  String generateStatusJson();
  String generateConfigJson();
  bool handleMoveRequest(JsonDocument& doc);
  void handleRoot();
  void handleTeste();
  void handleStyleCss();
  void handleAppJs();
  void handleMotorStatus();
  void handleMotorMove();
  void handleMotorStop();
  void handleConfigGet();
  void handleConfigPost();
  void handleSensors();
  void handleAutoGet();
  void handleAutoPost();
  void handleSystemInfo();
  void handleNotFound();
};

#endif // DASHBOARD_SERVICE_H
