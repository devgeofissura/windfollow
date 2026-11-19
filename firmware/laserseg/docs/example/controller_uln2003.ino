#include <WiFi.h>
#include <WiFiUdp.h>
#include <AccelStepper.h>

// Use os parâmetros do ponto de acesso da câmera ou da rede STA compartilhada.
#define NET_MODE 0
#define WIFI_SSID "SIMCAM-SETUP"
#define WIFI_PASSWORD "simcam123"
static const uint16_t UDP_PORT=4210;

// ULN2003: ordem HALF4WIRE = IN1, IN3, IN2, IN4.
static const uint8_t PAN_IN1=4, PAN_IN2=5, PAN_IN3=6, PAN_IN4=7;
static const uint8_t TILT_IN1=8, TILT_IN2=9, TILT_IN3=10, TILT_IN4=11;
static const uint8_t LIGHT_PIN=12;
static const uint8_t X_MIN_SW=13, X_MAX_SW=14, Y_MIN_SW=15, Y_MAX_SW=16;
static const long STEPS_PER_REV=4096; // aproximação em meia etapa; calibrar cada unidade 28BYJ-48.
static const long MAX_TRAVEL=STEPS_PER_REV/6; // ±60° estimados a partir do centro manual.
static const float TRACK_GAIN=0.55f;
static const int DEAD_BAND=55;
static const uint32_t LINK_TIMEOUT_MS=1200;

AccelStepper pan(AccelStepper::HALF4WIRE,PAN_IN1,PAN_IN3,PAN_IN2,PAN_IN4);
AccelStepper tilt(AccelStepper::HALF4WIRE,TILT_IN1,TILT_IN3,TILT_IN2,TILT_IN4);
WiFiUDP udp;
bool tracking=false;
uint32_t lastPacket=0;
long panTarget=0,tiltTarget=0;

void connectNetwork(){
  WiFi.setSleep(false); WiFi.mode(WIFI_STA);
  if(NET_MODE==0){IPAddress ip(192,168,4,2),gw(192,168,4,1),mask(255,255,255,0);WiFi.config(ip,gw,mask);}
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD); uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<20000)delay(250);
  if(WiFi.status()==WL_CONNECTED)Serial.printf("Controlador conectado: %s\n",WiFi.localIP().toString().c_str());
  else Serial.println("Wi-Fi não conectado; motores permanecerão sem comandos.");
  udp.begin(UDP_PORT);
}
void stopAxis(AccelStepper &axis,long &target){target=axis.currentPosition();axis.moveTo(target);}
void parsePacket(String m){
  lastPacket=millis();
  if(m.startsWith("TRACK,")){tracking=m.substring(6).toInt()==1;if(!tracking){stopAxis(pan,panTarget);stopAxis(tilt,tiltTarget);}return;}
  if(m.startsWith("LIGHT,")){digitalWrite(LIGHT_PIN,m.substring(6).toInt()?HIGH:LOW);return;}
  if(m.startsWith("MANUAL,")){int a=m.indexOf(',',7);if(a<0)return;String axis=m.substring(7,a);int d=m.substring(a+1).toInt()<0?-1:1;
    if(axis=="pan")panTarget=constrain(panTarget+d*80,-MAX_TRAVEL,MAX_TRAVEL);
    if(axis=="tilt")tiltTarget=constrain(tiltTarget+d*80,-MAX_TRAVEL,MAX_TRAVEL);
    pan.moveTo(panTarget);tilt.moveTo(tiltTarget);return;}
  if(m.startsWith("T,")){int a=m.indexOf(',',2),b=m.indexOf(',',a+1),c=m.indexOf(',',b+1);if(a<0||b<0||c<0)return;
    int ex=m.substring(a+1,b).toInt(),ey=m.substring(b+1,c).toInt();bool valid=m.substring(c+1).toInt()==1;
    if(!tracking||!valid){stopAxis(pan,panTarget);stopAxis(tilt,tiltTarget);return;}
    if(abs(ex)>DEAD_BAND)panTarget=constrain((long)(ex*MAX_TRAVEL*TRACK_GAIN/1000.0f),-MAX_TRAVEL,MAX_TRAVEL);
    if(abs(ey)>DEAD_BAND)tiltTarget=constrain((long)(ey*MAX_TRAVEL*TRACK_GAIN/1000.0f),-MAX_TRAVEL,MAX_TRAVEL);
    pan.moveTo(panTarget);tilt.moveTo(tiltTarget);}
}
void readUdp(){int n=udp.parsePacket();if(!n)return;char b[128];int r=udp.read(b,sizeof(b)-1);if(r<=0)return;b[r]=0;parsePacket(String(b));}
void runLimited(AccelStepper &axis,long &target,uint8_t minPin,uint8_t maxPin){long pos=axis.currentPosition();bool mn=digitalRead(minPin)==HIGH,mx=digitalRead(maxPin)==HIGH;
  if(mn&&target<pos){target=pos;axis.moveTo(pos);}if(mx&&target>pos){target=pos;axis.moveTo(pos);}axis.run();}
void setup(){Serial.begin(115200);pinMode(LIGHT_PIN,OUTPUT);digitalWrite(LIGHT_PIN,LOW);
  for(uint8_t p:{X_MIN_SW,X_MAX_SW,Y_MIN_SW,Y_MAX_SW})pinMode(p,INPUT_PULLUP);
  pan.setMaxSpeed(420);pan.setAcceleration(220);tilt.setMaxSpeed(420);tilt.setAcceleration(220);
  pan.setCurrentPosition(0);tilt.setCurrentPosition(0);connectNetwork();
  Serial.println("28BYJ-48/ULN2003 pronto. Eixos centralizados; instale quatro fins-de-curso NF.");}
void loop(){readUdp();if(millis()-lastPacket>LINK_TIMEOUT_MS){tracking=false;stopAxis(pan,panTarget);stopAxis(tilt,tiltTarget);digitalWrite(LIGHT_PIN,LOW);}
  runLimited(pan,panTarget,X_MIN_SW,X_MAX_SW);runLimited(tilt,tiltTarget,Y_MIN_SW,Y_MAX_SW);}
