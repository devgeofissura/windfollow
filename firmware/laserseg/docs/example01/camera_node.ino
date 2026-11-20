#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>
#include "esp_camera.h"
#include "img_converters.h"

// Pinout do exemplo oficial LilyGO CAMERA_MODEL_TTGO_T_CAM_SIM.
#define CAM_PWDN   -1
#define CAM_RESET  18
#define CAM_XCLK   14
#define CAM_SIOD    4
#define CAM_SIOC    5
#define CAM_Y9     15
#define CAM_Y8     16
#define CAM_Y7     17
#define CAM_Y6     12
#define CAM_Y5     10
#define CAM_Y4      8
#define CAM_Y3      9
#define CAM_Y2     11
#define CAM_VSYNC   6
#define CAM_HREF    7
#define CAM_PCLK   13
#define CAM_PWR_ON  1

static const uint16_t CONTROL_PORT = 4210;
static const uint8_t GRID_W = 40, GRID_H = 30;
static const uint32_t DETECT_PERIOD_MS = 250;
static const uint8_t PIXEL_DIFF_THRESHOLD = 24;
static const uint16_t MIN_CHANGED_PIXELS = 10;

WebServer server(80);
WiFiUDP udp;
Preferences prefs;
String wifiMode = "ap", ssid = "", password = "", apName = "SIMCAM-SETUP", apPassword = "simcam123";
IPAddress controllerIP(192, 168, 4, 2);
uint8_t previousGray[GRID_W * GRID_H];
bool havePrevious = false;
bool trackingEnabled = false;
bool lightEnabled = false;
uint32_t lastDetect = 0, seqNo = 0;
int lastX = 0, lastY = 0;
bool lastValid = false;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-BR"><meta name="viewport" content="width=device-width,initial-scale=1"><meta charset="utf-8">
<title>SIMCAM tracker</title><style>body{font:16px Arial;margin:16px;max-width:850px;background:#101820;color:#eef}button,input,select{font-size:16px;padding:9px;margin:4px}button{cursor:pointer}img{width:100%;max-width:640px;background:#222}fieldset{margin:12px 0}small{color:#abc}</style>
<h2>S3_CAM_SIM — teste de câmera e pan-tilt</h2><p id="st">Carregando...</p><img id="im" alt="imagem da câmera">
<fieldset><legend>Rastreamento experimental</legend><button onclick="track(1)">Ativar movimento</button><button onclick="track(0)">Parar rastreamento</button><button onclick="light(1)">Luz ligada</button><button onclick="light(0)">Luz desligada</button><p><small>Algoritmo simples de diferença entre quadros: detecta movimento geral, não reconhece pessoa/animal.</small></p></fieldset>
<fieldset><legend>Movimento manual</legend><button onclick="cmd('pan',-1)">Pan −</button><button onclick="cmd('pan',1)">Pan +</button><button onclick="cmd('tilt',-1)">Tilt −</button><button onclick="cmd('tilt',1)">Tilt +</button></fieldset>
<fieldset><legend>Wi-Fi</legend><form action="/config" method="post"><label>Modo <select name="mode"><option value="ap">Ponto de acesso (AP)</option><option value="sta">Cliente (STA)</option></select></label><br>Rede STA <input name="ssid" placeholder="SSID"><br>Senha STA <input name="password" type="password"><br>Nome AP <input name="ap" value="SIMCAM-SETUP"><br>Senha AP <input name="appass" value="simcam123"><br>IP da controladora <input name="ip" value="192.168.4.2"><br><button>Salvar e reiniciar Wi-Fi</button></form></fieldset>
<script>const q=s=>document.querySelector(s);setInterval(()=>{q('#im').src='/jpg?t='+Date.now()},500);setInterval(async()=>{try{let s=await(await fetch('/status')).json();q('#st').textContent='Wi-Fi '+s.mode+' / '+s.ip+' | movimento '+(s.valid?'detectado':'não detectado')+' | xy '+s.x+','+s.y+' | seq '+s.seq}catch(e){}},1000);function track(v){fetch('/track?on='+v)}function light(v){fetch('/light?on='+v)}function cmd(a,d){fetch('/manual?axis='+a+'&dir='+d)}</script></html>
)HTML";

void sendUdp(const String &message) {
  if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP) return;
  udp.beginPacket(controllerIP, CONTROL_PORT);
  udp.print(message);
  udp.endPacket();
}

void handleJpeg() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { server.send(503, "text/plain", "camera frame unavailable"); return; }
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  server.sendContent((const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStatus() {
  String ip = WiFi.localIP().toString();
  if (WiFi.getMode() == WIFI_AP) ip = WiFi.softAPIP().toString();
  String json = "{\"mode\":\"" + wifiMode + "\",\"ip\":\"" + ip + "\",\"valid\":" + String(lastValid ? "true" : "false") + ",\"x\":" + String(lastX) + ",\"y\":" + String(lastY) + ",\"seq\":" + String(seqNo) + "}";
  server.send(200, "application/json", json);
}

void handleConfig() {
  String newMode = server.arg("mode");
  if (newMode != "sta") newMode = "ap";
  prefs.putString("mode", newMode);
  prefs.putString("ssid", server.arg("ssid"));
  prefs.putString("pass", server.arg("password"));
  String ap = server.arg("ap"); if (ap.length()) prefs.putString("ap", ap);
  String appass = server.arg("appass"); if (appass.length() >= 8) prefs.putString("appass", appass);
  IPAddress ip;
  if (ip.fromString(server.arg("ip"))) {
    prefs.putString("ctrlip", ip.toString());
  }
  server.send(200, "text/html", "<meta charset='utf-8'><p>Configuração salva. Reiniciando em 1 segundo…</p>");
  delay(1000);
  ESP.restart();
}

void initWeb() {
  server.on("/", HTTP_GET, [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/jpg", HTTP_GET, handleJpeg);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/track", HTTP_GET, [](){
    trackingEnabled = server.arg("on") == "1";
    sendUdp(String("TRACK,") + (trackingEnabled ? "1" : "0"));
    server.send(200, "text/plain", "OK");
  });
  server.on("/light", HTTP_GET, [](){
    lightEnabled = server.arg("on") == "1";
    sendUdp(String("LIGHT,") + (lightEnabled ? "1" : "0"));
    server.send(200, "text/plain", "OK");
  });
  server.on("/manual", HTTP_GET, [](){
    String axis = server.arg("axis");
    int dir = server.arg("dir").toInt() < 0 ? -1 : 1;
    if (axis == "pan" || axis == "tilt") sendUdp("MANUAL," + axis + "," + String(dir));
    server.send(200, "text/plain", "OK");
  });
  server.begin();
}

bool startCamera() {
  pinMode(CAM_PWR_ON, OUTPUT);
  digitalWrite(CAM_PWR_ON, HIGH);
  delay(100);
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = CAM_Y2; c.pin_d1 = CAM_Y3; c.pin_d2 = CAM_Y4; c.pin_d3 = CAM_Y5;
  c.pin_d4 = CAM_Y6; c.pin_d5 = CAM_Y7; c.pin_d6 = CAM_Y8; c.pin_d7 = CAM_Y9;
  c.pin_xclk = CAM_XCLK; c.pin_pclk = CAM_PCLK; c.pin_vsync = CAM_VSYNC; c.pin_href = CAM_HREF;
  c.pin_sccb_sda = CAM_SIOD; c.pin_sccb_scl = CAM_SIOC;
  c.pin_pwdn = CAM_PWDN; c.pin_reset = CAM_RESET;
  c.xclk_freq_hz = 20000000; c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_QVGA; c.jpeg_quality = 12; c.fb_count = 2;
  c.fb_location = CAMERA_FB_IN_PSRAM; c.grab_mode = CAMERA_GRAB_LATEST;
  if (!psramFound()) { c.fb_count = 1; c.fb_location = CAMERA_FB_IN_DRAM; }
  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) { Serial.printf("esp_camera_init error 0x%x\n", err); return false; }
  return true;
}

void startNetwork() {
  wifiMode = prefs.getString("mode", "ap");
  ssid = prefs.getString("ssid", ""); password = prefs.getString("pass", "");
  apName = prefs.getString("ap", "SIMCAM-SETUP"); apPassword = prefs.getString("appass", "simcam123");
  String ipText = prefs.getString("ctrlip", "192.168.4.2"); controllerIP.fromString(ipText);
  WiFi.setSleep(false);
  if (wifiMode == "sta" && ssid.length()) {
    WiFi.mode(WIFI_STA); WiFi.begin(ssid.c_str(), password.c_str());
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) { delay(250); Serial.print('.'); }
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.println("\nSTA falhou; iniciando AP temporário");
  }
  wifiMode = "ap";
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName.c_str(), apPassword.c_str());
}

void analyzeMotion() {
  uint32_t now = millis();
  if (now - lastDetect < DETECT_PERIOD_MS) return;
  lastDetect = now;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;
  const size_t rgbLen = (size_t)fb->width * fb->height * 3;
  uint8_t *rgb = (uint8_t *)ps_malloc(rgbLen);
  if (!rgb) { esp_camera_fb_return(fb); return; }
  bool converted = fmt2rgb888(fb->buf, fb->len, fb->format, rgb);
  int64_t sumX = 0, sumY = 0; uint16_t changed = 0;
  if (converted) {
    for (int gy = 0; gy < GRID_H; ++gy) {
      int y = gy * fb->height / GRID_H;
      for (int gx = 0; gx < GRID_W; ++gx) {
        int x = gx * fb->width / GRID_W;
        size_t i = ((size_t)y * fb->width + x) * 3;
        uint8_t gray = (uint8_t)((rgb[i] * 30u + rgb[i+1] * 59u + rgb[i+2] * 11u) / 100u);
        size_t gi = (size_t)gy * GRID_W + gx;
        if (havePrevious && abs((int)gray - (int)previousGray[gi]) > PIXEL_DIFF_THRESHOLD) {
          ++changed; sumX += gx; sumY += gy;
        }
        previousGray[gi] = gray;
      }
    }
    havePrevious = true;
  }
  free(rgb); esp_camera_fb_return(fb);
  lastValid = converted && changed >= MIN_CHANGED_PIXELS && changed < (GRID_W * GRID_H * 3 / 4);
  if (lastValid) {
    int cx = (int)(sumX / changed), cy = (int)(sumY / changed);
    lastX = (cx - (GRID_W / 2)) * 1000 / (GRID_W / 2);
    lastY = (cy - (GRID_H / 2)) * 1000 / (GRID_H / 2);
  }
  ++seqNo;
  if (trackingEnabled) sendUdp("T," + String(seqNo) + "," + String(lastX) + "," + String(lastY) + "," + (lastValid ? "1" : "0"));
}

void setup() {
  Serial.begin(115200);
  prefs.begin("simcam", false);
  if (!startCamera()) { Serial.println("Falha na câmera; confira sensor, versão e pinout."); delay(3000); ESP.restart(); }
  startNetwork();
  udp.begin(0);
  initWeb();
  Serial.print("UI: http://"); Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  Serial.println("Aviso: rastreamento é somente detecção de movimento, sem classes pessoa/animal.");
}

void loop() {
  server.handleClient();
  analyzeMotion();
  delay(1);
}
