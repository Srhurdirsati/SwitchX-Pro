#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include "time.h"

RTC_DS3231 rtc;
WebServer server(80);

// WIFI
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASS";

// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;

// RELAYS
int relays[6] = {23,19,18,5,17,16};
bool relayState[6];

#define EEPROM_SIZE 10

// ---------- SAVE / LOAD ----------
void saveState(){
  for(int i=0;i<6;i++){
    EEPROM.write(i, relayState[i]);
  }
  EEPROM.commit();
}

void loadState(){
  for(int i=0;i<6;i++){
    relayState[i] = EEPROM.read(i);
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], relayState[i]);
  }
}

// ---------- TIME SYNC ----------
void syncRTC(){
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    rtc.adjust(DateTime(
      timeinfo.tm_year+1900,
      timeinfo.tm_mon+1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    ));
  }
}

// ---------- WEB UI ----------
String page(){

  DateTime now = rtc.now();

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  String deviceTime = String(timeinfo.tm_hour)+":"+String(timeinfo.tm_min)+":"+String(timeinfo.tm_sec);
  String rtcTime = String(now.hour())+":"+String(now.minute())+":"+String(now.second());

  String html = R"rawliteral(
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  body {
    margin:0;
    font-family:Segoe UI;
    background: linear-gradient(135deg,#0f2a2f,#2c4f56);
    color:#e8f0f2;
  }
  h1{text-align:center;margin-top:20px;}
  .clock{text-align:center;margin-bottom:15px;}
  .grid{
    display:grid;
    grid-template-columns:repeat(auto-fit,minmax(220px,1fr));
    gap:20px;padding:20px;
  }
  .card{
    background:rgba(255,255,255,0.08);
    border-radius:15px;padding:20px;position:relative;
  }
  .info{
    position:absolute;top:10px;right:15px;cursor:pointer;
  }
  .switch{
    position:relative;display:inline-block;width:50px;height:26px;margin-top:10px;
  }
  .switch input{display:none;}
  .slider{
    position:absolute;top:0;left:0;right:0;bottom:0;
    background:#666;border-radius:34px;
  }
  .slider:before{
    content:"";position:absolute;height:20px;width:20px;
    left:3px;bottom:3px;background:white;border-radius:50%;
    transition:.3s;
  }
  input:checked + .slider{background:#4cd964;}
  input:checked + .slider:before{transform:translateX(24px);}
  button{
    padding:6px 10px;margin-top:10px;border:none;border-radius:5px;
    background:#4cd964;cursor:pointer;
  }
  .footer{text-align:center;margin-top:20px;opacity:0.6;}
  </style>
  </head>

  <body>

  <h1>SwitchX Pro</h1>

  <div class="clock">
    RTC Time: )rawliteral";

  html += rtcTime;

  html += R"rawliteral(<br>
    Device Time: )rawliteral";

  html += deviceTime;

  html += R"rawliteral(
  </div>

  <div style="text-align:center;">
    <a href="/sync"><button>Sync Time</button></a>
  </div>

  <div class="grid">
  )rawliteral";

  for(int i=0;i<6;i++){
    String checked = relayState[i] ? "checked" : "";

    html += "<div class='card'>";
    html += "<h3>Relay "+String(i+1)+"</h3>";

    html += "<label class='switch'>";
    html += "<input type='checkbox' "+checked+" onclick='location.href=\"/toggle?r="+String(i)+"\"'>";
    html += "<span class='slider'></span></label>";

    html += "</div>";
  }

  html += R"rawliteral(
  </div>

  <div class="footer">
    Made by Sarthak Tripathi
  </div>

  </body>
  </html>
  )rawliteral";

  return html;
}

// ---------- HANDLERS ----------
void handleRoot(){ server.send(200,"text/html",page()); }

void handleToggle(){
  int r = server.arg("r").toInt();
  relayState[r] = !relayState[r];
  digitalWrite(relays[r], relayState[r]);
  saveState();
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSync(){
  syncRTC();
  server.sendHeader("Location","/");
  server.send(303);
}

// ---------- SETUP ----------
void setup(){

  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  loadState();

  Wire.begin();
  rtc.begin();

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
  }

  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec,0,ntpServer);

  server.on("/",handleRoot);
  server.on("/toggle",handleToggle);
  server.on("/sync",handleSync);

  server.begin();
}

// ---------- LOOP ----------
void loop(){
  server.handleClient();
}