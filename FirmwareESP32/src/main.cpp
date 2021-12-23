#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#include <HTTPClient.h>

#include <EEPROM.h>


#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 6

#define CLK_PIN   18  // or SCK
#define DATA_PIN  19  // or MOSI
#define CS_PIN    5   // or SS

/*
Pinout
ESP32 AZDelivery
VCC - 3,3v
GND - GND
DIN - G19
CS  - G5
CLK - G18
*/

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

bool rst = false;

const char* ssid     = "";
const char* password = "";

const char* CHANNEL_ID = "";
const char* API_KEY = "";

char ssid_global[100]     = "";
char password_global[100] = "";

char CHANNEL_ID_global[100] = "";
char API_KEY_global[100] = "";

const IPAddress remote_ip(8, 8, 8, 8);

WiFiClientSecure client;

int contatoreIscritti = 0;
int iscrittiPrecedenti = 0;

WebServer server(80);

uint64_t delayStart = 0;
bool delayRunning = false;

bool wifiAP = false;

void getYTData();
void getYTDataOLD();
void getDate();
void doAnim();
void handleRoot();
void handleSubmit();
void handleReset();


void setup() 
{
  delay(1000);
  Serial.begin(115200);
  delay(10);

  P.begin();

  struct {
    int check = 313;
    char ssid[32] = "";
    char pwd[64] = "";
    char channel_id[50] = "";
    char api_key[100] = "";
  } datiEEPROM,resetEEPROM;

  EEPROM.begin(512);

  EEPROM.get(0,datiEEPROM);
  Serial.println("Valori su EEPROM:\nSSID: " + String(datiEEPROM.ssid) + "\npwd:" + String(datiEEPROM.pwd) + "\nchannel_id: " + String(datiEEPROM.channel_id) + "\napi_key: " + String(datiEEPROM.api_key) + "\nCHECK: " + String(datiEEPROM.check));
  
  if(datiEEPROM.check != 313){
    Serial.println("Reset eeprom, first start");
    EEPROM.put(0,resetEEPROM);
    EEPROM.commit();
  }

  ssid = datiEEPROM.ssid;
  password = datiEEPROM.pwd;
  CHANNEL_ID = datiEEPROM.channel_id;
  API_KEY = datiEEPROM.api_key;

  strcpy(ssid_global, datiEEPROM.ssid);
  strcpy(password_global, datiEEPROM.pwd);
  strcpy(CHANNEL_ID_global, datiEEPROM.channel_id);
  strcpy(API_KEY_global, datiEEPROM.api_key);

  
#ifdef DEBUG
  Serial.println("'"+String(ssid)+"'");
  Serial.println("'"+String(password)+"'");
  Serial.println("'"+String(CHANNEL_ID)+"'");
  Serial.println("'"+String(API_KEY)+"'");
  
  
  Serial.println();
  Serial.print("WiFi Connection: ");
  Serial.print(ssid);
  Serial.print(" Progress [");
#endif

  P.print("WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int flipflop = 0;
  int connAttempt = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
#ifdef DEBUG
    Serial.print("=");
#endif
    switch(flipflop){
      case 0: P.print("WiFi .  ");
              flipflop = 1;
              break;
      case 1: P.print("WiFi .. ");
              flipflop = 2;
              break;
      case 2: P.print("WiFi ...");
              flipflop = 3;
              break;
      case 3: P.print("WiFi    ");
              flipflop = 0;
              break;
    }

    if(connAttempt > 80){
      wifiAP = true;
      break;
    }
    
    connAttempt++;
  }
  if(wifiAP == false){
    P.print("WiFi OK");

    delay(2000);
#ifdef DEBUG
    Serial.println("]");
    Serial.print("WiFi OK - IP:        ");  
    Serial.println(WiFi.localIP());
#endif
  } else {
#ifdef DEBUG
    Serial.println("]");
    Serial.print("Starting AP Mode: ");
#endif
    boolean result = WiFi.softAP("YoutubeButton", "YoutubeButton");
    if(result == true){
#ifdef DEBUG
      Serial.println("OK");
#endif
    }else{
#ifdef DEBUG
      Serial.println("Errore");
#endif
      while(1);
    }
    
    P.print("WiFi AP");
  #ifdef DEBUG
    Serial.print("WiFi AP - IP:        ");  
    Serial.println(WiFi.softAPIP());
  #endif
  }

  delay(1000);

  char IP[] = "xxx.xxx.xxx.xxx";
  if(wifiAP == false){
    IPAddress ip = WiFi.localIP();
    ip.toString().toCharArray(IP, 16);
  }else{
    IPAddress ip = WiFi.softAPIP();
    ip.toString().toCharArray(IP, 16);
  }
  
  P.displayClear();
  P.displayText(IP, PA_LEFT, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  doAnim();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", handleSubmit);
  server.on("/reset", handleReset);
  
  server.begin();
#ifdef DEBUG
  Serial.println("HTTP server started");
#endif
  delayStart = millis();
  delayRunning = true;


  if(wifiAP == true){
    P.print("Setup");
  }
}

unsigned long previousMillis = 0;
unsigned long interval = 10000;

void loop() 
{
  server.handleClient();

  if(rst){
    ESP.restart();
  }
  
  if(wifiAP == false){
    if(millis() - previousMillis >= interval){
      previousMillis = millis();
      getYTData();
      
    }
  }
}

void getYTData()
{
  HTTPClient httpClient;
  httpClient.begin("https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" + String(CHANNEL_ID_global) + "&key=" + String(API_KEY_global));

  DynamicJsonDocument doc(600);

  int httpResponseCode = httpClient.GET();
  if ( httpResponseCode == 200 ) {
#ifdef DEBUG
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
#endif
    String payload = httpClient.getString();
#ifdef DEBUG
    Serial.println(payload);
#endif
    deserializeJson(doc, payload);
  }

  if(!doc.isNull()){
    if(doc["items"][0]["statistics"]["hiddenSubscriberCount"] != "true"){
      const char* subs = doc["items"][0]["statistics"]["subscriberCount"];
#ifdef DEBUG
      Serial.println("Iscritti: " + String(subs));
#endif
      contatoreIscritti = atoi(doc["items"][0]["statistics"]["subscriberCount"]);

      if(contatoreIscritti != iscrittiPrecedenti){
          if(contatoreIscritti > iscrittiPrecedenti){
            P.displayText(subs, PA_CENTER, 20, 0, PA_SCROLL_DOWN, PA_NO_EFFECT);  // aumento
          } else {
            P.displayText(subs, PA_CENTER, 20, 0, PA_SCROLL_UP, PA_NO_EFFECT);    // diminuzione
          }
          iscrittiPrecedenti = contatoreIscritti;
        } else {
          P.displayText(subs, PA_CENTER, 20, 0, PA_PRINT, PA_NO_EFFECT);          // static
        }

      doAnim();
    }


  } else {
#ifdef DEBUG
    Serial.println("È vuoto!");
#endif
  }
}

void doAnim(){
  while(1){
    delay(1);
    if(P.displayAnimate()){
      break;
    }
  }
}

void handleRoot()
{
  char temp[4000];

#ifdef DEBUG
  Serial.println("Formazione pagina richiesta");
#endif
  sprintf(temp, "<html>"
  "<head>"
  "  <title>Youtube Counter</title>"
  "  <style>"
  "    :root{--input-color:#99A3BA;--input-border:#CDD9ED;--input-background:#fff;--input-placeholder:#CBD1DC;--input-border-focus:#275EFE;--group-color:var(--input-color);--group-border:var(--input-border);--group-background:#EEF4FF;--group-color-focus:#fff;--group-border-focus:var(--input-border-focus);--group-background-focus:#678EFE}.form-field{display:block;width:400;padding:8px 16px;line-height:25px;font-size:14px;font-weight:500;font-family:inherit;border-radius:6px;-webkit-appearance:none;color:var(--input-color);border:1px solid var(--input-border);background:var(--input-background);transition:border .3s ease;&::placeholder{color:var(--input-placeholder)}&:focus{outline:none;border-color:var(--input-border-focus)}}.form-group{position:relative;display:flex;width:400;&>span,.form-field{white-space:nowrap;display:block;&:not(:first-child):not(:last-child){border-radius:0}&:first-child{border-radius:6px 0 0 6px}&:last-child{border-radius:0 6px 6px 0}&:not(:first-child){margin-left:-1px}}.form-field{position:relative;z-index:1;flex:1 1 auto;width:1%;margin-top:0;margin-bottom:0}&>span{text-align:center;padding:8px 12px;font-size:14px;line-height:25px;color:var(--group-color);background:var(--group-background);border:1px solid var(--group-border);transition:background .3s ease,border .3s ease,color .3s ease}&:focus-within{&>span{color:var(--group-color-focus);background:var(--group-background-focus);border-color:var(--group-border-focus)}}}html{box-sizing:border-box;-webkit-font-smoothing:antialiased}*{box-sizing:inherit;&:before,&:after{box-sizing:inherit}}body{min-height:100vh;font-family:'Mukta Malar',Arial;display:flex;justify-content:center;align-items:center;flex-direction:column;background:#F5F9FF;.form-group{max-width:360px;&:not(:last-child){margin-bottom:32px}}}h1{color:#333;font-family:'Raleway',sans-serif;font-size:62px;font-weight:800;line-height:72px;margin:0 0 24px;text-align:center;text-transform:uppercase}ul{display:inline-grid;grid-auto-flow:row;grid-gap:24px;justify-items:center;margin:auto}a{color:#333;text-decoration:none;box-shadow:inset 0 -1px 0 hsla(0,0%,100%,.4)}a:hover{box-shadow:inset 0 -1.2em 0 hsla(0,0%,100%,.4)}li:last-child{grid-column:1 / 2;grid-row:1 / 2}.btn{box-shadow:0 10px 14px -7px #276873;background-color:#599bb3;border-radius:8px;display:inline-block;cursor:pointer;color:#fff;font-family:Arial;font-size:20px;font-weight:700;padding:13px 32px;text-decoration:none;text-shadow:0 1px 0 #3d768a}"
  "  </style>"
  "</head>"
  "<h1>Youtube Counter</h1>"
  "  <br>"
  "  <form action='/submit' method='POST'>"
  "          <span>SSID: </span>"
  "      <div class='form-group'>"
  "          <input class='form-field' type='text' name='sid' value='%s'>"
  "      </div>"
  "          <span>Password: </span>"
  "      <div class='form-group'>"
  "          <input class='form-field' type='text' name='pwd' value='%s'>"
  "      </div>"
  "          <span>Channel ID: </span>"
  "      <div class='form-group'>"
  "          <input class='form-field' type='text' name='cid' value='%s'>"
  "      </div>"
  "          <span>API Key: </span>"
  "      <div class='form-group'>"
  "          <input class='form-field' type='text' name='api' value='%s'>"
  "      </div>"
  "      <br>"
  "      <input class='btn' type='submit' value='Salva'>"
  "  </form>"
  "  <br><br>"
  "  <form action'/reset' method='POST'>"
  "      <input class='btn' type='submit' value='Reset'>"
  "  </form>"
  "  <br>"
  "  <div>"
  "    <ul>"
  "      <li><a href='https://foxsource.it/' target='_blank'>Proudly made by Luca Barsanti (FoxSource 3D Printing)</a></li>"
  "    </ul>"
  "  </div>"
"</html>",ssid_global, password_global, CHANNEL_ID_global, API_KEY_global);

  #ifdef DEBUG
  Serial.println("Invio pagina web al client");
  #endif
  server.send(200, "text/html", temp);
}


void handleSubmit()
{
  struct {
    int check = 313;
    char ssid[32] = "";
    char pwd[64] = "";
    char channel_id[50] = "";
    char api_key[100] = "";
  } datiEEPROM;
  
  
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {

      if (server.argName(i) == "sid" && server.arg(i) != "") {
#ifdef DEBUG
        Serial.print("SSID:     '");
        Serial.print(server.arg(i));
        Serial.println("'");
#endif
        rst = true;

        server.arg(i).toCharArray(datiEEPROM.ssid,32);
      }

      if (server.argName(i) == "pwd" && server.arg(i) != "") {
#ifdef DEBUG
        Serial.print("PASSWORD: '");
        Serial.print(server.arg(i));
        Serial.println("'");
#endif
        rst = true;

        server.arg(i).toCharArray(datiEEPROM.pwd,64);
      }

      if (server.argName(i) == "cid" && server.arg(i) != "") {
#ifdef DEBUG
        Serial.print("CHAN_ID:  '");
        Serial.print(server.arg(i));
        Serial.println("'");
#endif
        server.arg(i).toCharArray(datiEEPROM.channel_id,50);
      }

      if (server.argName(i) == "api" && server.arg(i) != "") {
#ifdef DEBUG
        Serial.print("API KEY:  '");
        Serial.print(server.arg(i));
        Serial.println("'");
#endif
        server.arg(i).toCharArray(datiEEPROM.api_key,100);
      }
    }
  }

#ifdef DEBUG
  Serial.println("NUOVI VALORI DA SCRIVERE:\nSSID: '" + String(datiEEPROM.ssid) + "'\npwd: '" + String(datiEEPROM.pwd) + "'\nchannel_id: '" + String(datiEEPROM.channel_id) + "'\napi_key: '" + String(datiEEPROM.api_key) + "'\nCHECK: " + String(datiEEPROM.check));
#endif

  EEPROM.put(0,datiEEPROM);
  EEPROM.commit();
  
  server.sendHeader("Location","/");
  server.send(303);
  
}

void handleReset(){
#ifdef DEBUG
  Serial.println("Reset");
#endif
  server.sendHeader("Location","/");
  server.send(303);
  ESP.restart();

}


