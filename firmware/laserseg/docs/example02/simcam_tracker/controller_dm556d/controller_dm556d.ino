#include <WiFi.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>

// Se a câmera estiver em AP, mantenha NET_MODE=0 e use o SSID/senha abaixo.
// Para a câmera em STA, defina NET_MODE=1 e informe o mesmo roteador.
#define NET_MODE 0
#define WIFI_SSID "SIMCAM-SETUP"
#define WIFI_PASSWORD "simcam123"
static const uint16_t UDP_PORT = 4210;

// Driver Azanelli DM556D em STEP/DIR. Os sinais da ESP32 devem passar por interface compatível com as entradas optoacopladas.
static const uint8_t X_STEP=4, X_DIR=5, Y_STEP=6, Y_DIR=7;
static const uint8_t LIGHT_PIN=12;
static const uint8_t X_MIN_SW=13, X_MAX_SW=14, Y_MIN_SW=15, Y_MAX_SW=16;
static const long STEPS_PER_REV=3200; // Se usar motor 1.8° e DM556D configurado em 3200 pulsos/volta (SW5..SW8: OFF OFF ON ON).
static const long MAX_TRAVEL=STEPS_PER_REV/6; // Limite lógico inicial: ±60° a partir do centro manual.
static const float TRACK_GAIN=0.75f;
static const int DEAD_BAND=55;
static const uint32_t LINK_TIMEOUT_MS=1200;

AccelStepper pan(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper tilt(AccelStepper::DRIVER, Y_STEP, Y_DIR);
WiFiUDP udp;
bool tracking=false;
uint32_t lastPacket=0;
long panTarget=0, tiltTarget=0;

void connectNetwork() {
  WiFi.setSleep(false);
  if (NET_MODE==0) {
    WiFi.mode(WIFI_STA);
    IPAddress ip(192,168,4,2), gw(192,168,4,1), mask(255,255,255,0);
    WiFi.config(ip,gw,mask);
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  } else {
    WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  }
  uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<20000) delay(250);
  if(WiFi.status()==WL_CONNECTED) Serial.printf("Controlador conectado: %s\n",WiFi.localIP().toString().c_str());
  else Serial.println("Wi-Fi não conectado; motores permanecerão sem comandos.");
  udp.begin(UDP_PORT);
}

void stopAxis(AccelStepper &axis, long &target) { target=axis.currentPosition(); axis.moveTo(target); }

void parsePacket(String m) {
  lastPacket=millis();
  if(m.startsWith("TRACK,")) { tracking=m.substring(6).toInt()==1; if(!tracking){stopAxis(pan,panTarget);stopAxis(tilt,tiltTarget);} return; }
  if(m.startsWith("LIGHT,")) { digitalWrite(LIGHT_PIN,m.substring(6).toInt()?HIGH:LOW); return; }
  if(m.startsWith("MANUAL,")) {
    int a=m.indexOf(',',7); if(a<0)return; String axis=m.substring(7,a); int d=m.substring(a+1).toInt()<0?-1:1;
    if(axis=="pan") panTarget=constrain(panTarget+d*80,-MAX_TRAVEL,MAX_TRAVEL);
    if(axis=="tilt") tiltTarget=constrain(tiltTarget+d*80,-MAX_TRAVEL,MAX_TRAVEL);
    pan.moveTo(panTarget); tilt.moveTo(tiltTarget); return;
  }
  if(m.startsWith("T,")) {
    int a=m.indexOf(',',2), b=m.indexOf(',',a+1), c=m.indexOf(',',b+1); if(a<0||b<0||c<0)return;
    int ex=m.substring(a+1,b).toInt(), ey=m.substring(b+1,c).toInt(); bool valid=m.substring(c+1).toInt()==1;
    if(!tracking || !valid) { stopAxis(pan,panTarget); stopAxis(tilt,tiltTarget); return; }
    if(abs(ex)>DEAD_BAND) panTarget=constrain((long)(ex*MAX_TRAVEL*TRACK_GAIN/1000.0f),-MAX_TRAVEL,MAX_TRAVEL);
    if(abs(ey)>DEAD_BAND) tiltTarget=constrain((long)(ey*MAX_TRAVEL*TRACK_GAIN/1000.0f),-MAX_TRAVEL,MAX_TRAVEL);
    pan.moveTo(panTarget); tilt.moveTo(tiltTarget);
  }
}

void readUdp() {
  int packetSize=udp.parsePacket(); if(!packetSize)return;
  char buf[128]; int n=udp.read(buf,sizeof(buf)-1); if(n<=0)return; buf[n]=0;
  parsePacket(String(buf));
}

void runLimited(AccelStepper &axis, long &target, uint8_t minPin, uint8_t maxPin) {
  long pos=axis.currentPosition();
  bool minHit=digitalRead(minPin)==HIGH; // contatos NF para GND; aberto = atingiu/fio rompido
  bool maxHit=digitalRead(maxPin)==HIGH;
  if(minHit && target<pos) { target=pos; axis.moveTo(pos); }
  if(maxHit && target>pos) { target=pos; axis.moveTo(pos); }
  axis.run();
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN,OUTPUT); digitalWrite(LIGHT_PIN,LOW);
  for(uint8_t p: {X_MIN_SW,X_MAX_SW,Y_MIN_SW,Y_MAX_SW}) pinMode(p,INPUT_PULLUP);
  pan.setMinPulseWidth(5); tilt.setMinPulseWidth(5); // Pulso conservador; conferir mínimo no manual exato do DM556D.
  pan.setMaxSpeed(700); pan.setAcceleration(450); tilt.setMaxSpeed(700); tilt.setAcceleration(450);
  pan.setCurrentPosition(0); tilt.setCurrentPosition(0);
  connectNetwork();
  Serial.println("DM556D pronto. Eixos centralizados; instale quatro fins-de-curso NF e interface elétrica STEP/DIR aprovada.");
}

void loop() {
  readUdp();
  if(millis()-lastPacket>LINK_TIMEOUT_MS) { tracking=false; stopAxis(pan,panTarget); stopAxis(tilt,tiltTarget); digitalWrite(LIGHT_PIN,LOW); }
  runLimited(pan,panTarget,X_MIN_SW,X_MAX_SW);
  runLimited(tilt,tiltTarget,Y_MIN_SW,Y_MAX_SW);
}
